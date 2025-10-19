#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Load environment variables
ENV_FILE="${ENV_FILE:-$SCRIPT_DIR/.env}"
if [ -f "$ENV_FILE" ]; then
    source "$ENV_FILE"
fi

# Fallback to defaults if not set
DEST_IP="${DEST_IP:-192.168.1.100}"
DEST_PORT="${DEST_PORT:-50000}"
PI_CLIENT_PATH="${PI_CLIENT_PATH:-/home/pi/pi_client}"
LOG_FILE="${LOG_FILE:-/var/log/pi_client_auto.log}"

# Array to track running processes
declare -A running_pids

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Function to check if device is suitable for monitoring
is_input_device() {
    local device="$1"
    local device_info
    
    # Get device info from /proc/bus/input/devices
    device_info=$(cat /proc/bus/input/devices | grep -A10 -B5 "$(basename "$device")")
    
    # Restrict to keyboard and mouse devices only
    if echo "$device_info" | grep -qi -E "(keyboard|mouse)"; then
        return 0
    fi
    
    return 1
}

# Function to start pi_client for devices
start_pi_client() {
    # Kill any existing pi_client process
    if [ -n "${running_pids["current"]}" ]; then
        stop_pi_client "current"
    fi
    
    log_message "Starting pi_client with devices: $*"
    
    # Start pi_client in background
    nohup "$PI_CLIENT_PATH" "$DEST_IP" "$DEST_PORT" "$@" >> "$LOG_FILE" 2>&1 &
    local pid=$!
    
    # Store the PID with a fixed key "current"
    running_pids["current"]=$pid
    
    log_message "Started pi_client with PID $pid"
}

# Function to stop pi_client for a device
stop_pi_client() {
    local key="$1"
    local pid="${running_pids[$key]}"
    
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        log_message "Stopping pi_client (PID $pid)"
        kill "$pid"
        unset running_pids["$key"]
    fi
}

# Function to scan for input devices
scan_devices() {
    local devices=()
    local device
    
    # Find all suitable event devices (mouse and keyboard)
    for device in /dev/input/event*; do
        if [ -e "$device" ] && is_input_device "$device"; then
            devices+=("$device")
        fi
    done
    
    # Start pi_client if both mouse and keyboard are found
    if [ ${#devices[@]} -gt 0 ]; then
        log_message "Detected devices: ${devices[*]}"
        start_pi_client "${devices[@]}"
    fi
}

# Function to wait for network
wait_for_network() {
    log_message "Waiting for UDP server at $DEST_IP:$DEST_PORT..."
    
    local attempts=0
    
    while true; do
        # Test if the UDP port is reachable by trying to connect
        if timeout 10 nc -u -z "$DEST_IP" "$DEST_PORT" 2>/dev/null; then
            log_message "UDP server at $DEST_IP:$DEST_PORT is ready"
            return 0
        fi
        
        attempts=$((attempts + 1))
        log_message "UDP server not ready, waiting... (attempt $attempts)"
        sleep 5
    done
    
    log_message "WARNING: Could not reach UDP server at $DEST_IP:$DEST_PORT after 3 minutes"
    log_message "Starting anyway - will attempt to connect when server becomes available"
    return 1
}

# Function to cleanup on exit
cleanup() {
    log_message "Shutting down auto_pi_client..."
    
    for key in "${!running_pids[@]}"; do
        stop_pi_client "$key"
    done
    
    log_message "All pi_client processes stopped"
    exit 0
}

# Set up signal handlers
trap cleanup SIGTERM SIGINT

# Main execution
log_message "Starting auto_pi_client monitor"

# Check if pi_client exists
if [ ! -x "$PI_CLIENT_PATH" ]; then
    log_message "ERROR: pi_client not found at $PI_CLIENT_PATH"
    exit 1
fi

# Wait for network (with timeout)
wait_for_network

# Initial device scan
log_message "Performing initial device scan..."
scan_devices

# Monitor for device changes using inotify
log_message "Starting device monitoring..."

# Monitor /dev/input for changes
inotifywait -m -e create,delete,modify /dev/input --format '%e %w%f' 2>/dev/null |
while read event file; do
    # Only process event* files
    if [[ "$file" =~ /dev/input/event[0-9]+ ]]; then
        log_message "Device change detected: $event $file"
        
        # Small delay to let device settle
        sleep 1
        
        # Rescan devices
        scan_devices
    fi
done &

# Keep the script running and periodically check processes
while true; do
    # Check if our process died
    pid="${running_pids["current"]}"
    if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
        log_message "Process (PID $pid) died, restarting..."
        unset running_pids["current"]
        scan_devices
    fi
    
    sleep 10
done
