#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "Building C++, Go gateway, and frontend in parallel..."

cmake --build "$ROOT/build" > /tmp/build_cpp.log 2>&1 &
PID_CPP=$!

(cd "$ROOT/gateway" && go build ./...) > /tmp/build_go.log 2>&1 &
PID_GO=$!

(cd "$ROOT/frontend" && npm run build) > /tmp/build_npm.log 2>&1 &
PID_NPM=$!

FAILED=0

wait $PID_CPP && echo "✓ C++" || { echo "✗ C++ — see /tmp/build_cpp.log"; FAILED=1; }
wait $PID_GO  && echo "✓ Go"  || { echo "✗ Go  — see /tmp/build_go.log";  FAILED=1; }
wait $PID_NPM && echo "✓ npm" || { echo "✗ npm — see /tmp/build_npm.log"; FAILED=1; }

exit $FAILED
