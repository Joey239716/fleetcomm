#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "Starting C++ sim, Go gateway, and frontend dev server..."
echo "Press Ctrl+C to stop all."

"$ROOT/build/fleetcomm" &
PID_CPP=$!

(cd "$ROOT/gateway" && go run .) &
PID_GO=$!

(cd "$ROOT/frontend" && npm run dev) &
PID_NPM=$!

trap "echo 'Stopping...'; kill $PID_CPP $PID_GO $PID_NPM 2>/dev/null; exit 0" INT TERM

echo "Open http://localhost:5173"

wait
