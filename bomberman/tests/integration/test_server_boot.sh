#!/bin/bash
# System UI & Integration Test
# Asserts that the compiled server binary boots successfully and yield expected exit codes.

# Certs are read from CWD (auth/server_signed.crt), so run the server from
# the repo root, matching run-server in the Makefile.
cd "$(dirname "${BASH_SOURCE[0]}")/../../.." || exit 1

SERVER_BIN="./bomberman/build/bin/bombd"

# 1. Check if make successfully generated the binary
if [ ! -f "$SERVER_BIN" ]; then
    echo "[FAIL] Binary not found! Did 'make all' succeed?"
    exit 1
fi

# 2. Make sure certs exist before launching the server
./scripts/generate_auth.sh

# 3. Boot the server in the background
$SERVER_BIN > bomberman/server_test.log 2>&1 &
SERVER_PID=$!

# Give server 1 second to bind to port
sleep 1

# 4. Verify server hasn't crashed immediately
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "[FAIL] bombd crashed immediately upon boot."
    cat bomberman/server_test.log
    exit 1
fi

# 5. Clean up background process
kill $SERVER_PID
rm -f bomberman/server_test.log

echo "[PASS] System Integration Boot check succeeded."
exit 0
