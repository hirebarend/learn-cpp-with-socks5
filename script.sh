#!/usr/bin/env bash
set -e

# chmod +x ./script.sh
clang++ -Wall -std=c++23 src/main.cpp -o build/server

./build/server &
SERVER_PID=$!

node client/main.js &
NODE_PID=$!

trap "kill $SERVER_PID $NODE_PID 2>/dev/null" EXIT INT TERM

wait