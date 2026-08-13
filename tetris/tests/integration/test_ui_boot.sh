#!/bin/bash

# =============================================================================
# Integration Test: Daemon Boot Sequence
# Purpose: Ensures the compiled tetrisd binary can launch without instantly crashing.
# =============================================================================

echo "[*] Running Integration Test: test_ui_boot.sh"

# 1. Define the path to the compiled daemon binary
# (Adjust this path if your Makefile outputs to a different bin folder)
DAEMON_BIN="./build/bin/tetrisd"

# 2. Check if the binary actually compiled and exists
if [ ! -f "$DAEMON_BIN" ]; then
    echo "[-] FAILED: Binary $DAEMON_BIN does not exist. Did it compile?"
    exit 1
fi

# 3. Launch the daemon in the background (&)
echo "[*] Launching $DAEMON_BIN..."
$DAEMON_BIN &
DAEMON_PID=$!

# 4. Wait for 2 seconds to see if it survives initialization
sleep 2

# 5. Check if the process is still running
if kill -0 $DAEMON_PID 2>/dev/null; then
    echo "[+] SUCCESS: Daemon successfully booted and held its state."
    
    # Clean up: Kill the test daemon so it doesn't hang in the background
    kill -9 $DAEMON_PID
    exit 0
else
    echo "[-] FAILED: Daemon crashed immediately after boot."
    exit 1
fi