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
PI_SENDER_PATH="${PI_SENDER_PATH:-/home/pi/pi_sender}"
LOG_FILE="${LOG_FILE:-/var/log/pi_sender_auto.log}"

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
    
    # Check if it's a keyboard, mouse, or touchpad
    if echo "$device_info" | grep -qi -E "(keyboard|mouse|touchpad|synaptics)"; then
        return 0
    fi
    
    # Also check if it has key or relative events
    if echo "$device_info" | grep -q "EV=.*[13]"; then  # EV=3 (key+rel) or EV=1 (key)
        return 0
    fi
    
    return 1
}

# Function to start pi_sender for a device
start_pi_sender() {
    local device="$1"
    local device_name
    
    # Get friendly device name
    device_name=$(cat /proc/bus/input/devices | grep -B5 "$(basename "$device")" | grep "Name=" | cut -d'"' -f2)
    
    if [ -z "$device_name" ]; then
        device_name="Unknown Device"
    fi
    
    log_message "Starting pi_sender for $device ($device_name)"
    
    # Start pi_sender in background
    nohup "$PI_SENDER_PATH" "$device" "$DEST_IP" "$DEST_PORT" >> "$LOG_FILE" 2>&1 &
    local pid=$!
    
    # Store the PID with device path as key
    running_pids["$device"]=$pid
    
    log_message "Started pi_sender for $device with PID $pid"
}

# Function to stop pi_sender for a device
stop_pi_sender() {
    local device="$1"
    local pid="${running_pids[$device]}"
    
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        log_message "Stopping pi_sender for $device (PID $pid)"
        kill "$pid"
        unset running_pids["$device"]
    fi
}

# Function to scan for input devices
scan_devices() {
    local current_devices=()
    local device
    
    # Find all event devices
    for device in /dev/input/event*; do
        if [ -e "$device" ] && is_input_device "$device"; then
            current_devices+=("$device")
        fi
    done
    
    # Start pi_sender for new devices
    for device in "${current_devices[@]}"; do
        if [ -z "${running_pids[$device]}" ]; then
            start_pi_sender "$device"
        fi
    done
    
    # Stop pi_sender for removed devices
    for device in "${!running_pids[@]}"; do
        if [ ! -e "$device" ] || ! is_input_device "$device"; then
            stop_pi_sender "$device"
        fi
    done
}

# Function to wait for network
wait_for_network() {
    log_message "Waiting for network connectivity to $DEST_IP..."
    
    while ! ping -c 1 -W 2 "$DEST_IP" &> /dev/null; do
        log_message "Network not ready, waiting..."
        sleep 5
    done
    
    log_message "Network connectivity established"
}

# Function to cleanup on exit
cleanup() {
    log_message "Shutting down auto_pi_sender..."
    
    for device in "${!running_pids[@]}"; do
        stop_pi_sender "$device"
    done
    
    log_message "All pi_sender processes stopped"
    exit 0
}

# Set up signal handlers
trap cleanup SIGTERM SIGINT

# Main execution
log_message "Starting auto_pi_sender monitor"

# Check if pi_sender exists
if [ ! -x "$PI_SENDER_PATH" ]; then
    log_message "ERROR: pi_sender not found at $PI_SENDER_PATH"
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
    # Check if any of our processes died
    for device in "${!running_pids[@]}"; do
        local pid="${running_pids[$device]}"
        if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
            log_message "Process for $device (PID $pid) died, restarting..."
            unset running_pids["$device"]
            if [ -e "$device" ] && is_input_device "$device"; then
                start_pi_sender "$device"
            fi
        fi
    done
    
    sleep 10
done
