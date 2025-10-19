#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${ENV_FILE:-$SCRIPT_DIR/.env}"

# Load .env if present
[ -f "$ENV_FILE" ] && source "$ENV_FILE"

DEST_IP="${DEST_IP:-192.168.1.100}"
DEST_PORT="${DEST_PORT:-50000}"
PI_CLIENT_PATH="${PI_CLIENT_PATH:-/home/pi/pi_client}"
LOG_FILE="${LOG_FILE:-/var/log/pi_client_auto.log}"

# Track running process
PI_CLIENT_PID=""

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $*" | tee -a "$LOG_FILE"
}

cleanup() {
    log "Shutting down auto_pi_client..."
    if [ -n "$PI_CLIENT_PID" ] && kill -0 "$PI_CLIENT_PID" 2>/dev/null; then
        log "Stopping pi_client (PID $PI_CLIENT_PID)"
        kill "$PI_CLIENT_PID"
    fi
    exit 0
}

trap cleanup SIGINT SIGTERM

wait_for_network() {
    log "Waiting for UDP server at $DEST_IP:$DEST_PORT..."
    until nc -u -z "$DEST_IP" "$DEST_PORT" 2>/dev/null; do
        log "UDP server not ready yet..."
        sleep 5
    done
    log "UDP server is reachable."
}

get_input_devices() {
    local devices=()
    for f in /dev/input/by-id/*; do
        [[ -e "$f" ]] || continue
        case "$f" in
            *-event-kbd | *-event-mouse)
                local realdev
                realdev=$(readlink -f "$f")
                devices+=("$realdev")
                ;;
        esac
    done
    echo "${devices[@]}"
}

start_pi_client() {
    local devices=("$@")
    if [ "${#devices[@]}" -eq 0 ]; then
        log "No input devices found, not starting pi_client."
        return
    fi

    # Stop old one
    if [ -n "$PI_CLIENT_PID" ] && kill -0 "$PI_CLIENT_PID" 2>/dev/null; then
        log "Stopping existing pi_client (PID $PI_CLIENT_PID)"
        kill "$PI_CLIENT_PID"
        wait "$PI_CLIENT_PID" 2>/dev/null || true
    fi

    log "Starting pi_client with devices: ${devices[*]}"
    nohup "$PI_CLIENT_PATH" "$DEST_IP" "$DEST_PORT" "${devices[@]}" >>"$LOG_FILE" 2>&1 &
    PI_CLIENT_PID=$!
    log "Started pi_client (PID $PI_CLIENT_PID)"
}

monitor_loop() {
    local prev_devices=""
    while true; do
        local devices
        devices=$(get_input_devices)

        if [ "$devices" != "$prev_devices" ]; then
            log "Device change detected!"
            log "Detected devices: $devices"
            start_pi_client $devices
            prev_devices="$devices"
        fi

        sleep 3
    done
}

main() {
    log "==== Starting auto_pi_client monitor ===="
    [ -x "$PI_CLIENT_PATH" ] || { log "ERROR: pi_client not found at $PI_CLIENT_PATH"; exit 1; }

    wait_for_network
    monitor_loop
}

main
