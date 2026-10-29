#!/bin/sh
# Runs the XIAO ESP32-C3 game logic on your computer, with the Arduino libraries replaced by stubs (stub/).
# Needs a C++17 compiler. Usage: test/host_esp32c3/run.sh
set -e
cd "$(dirname "$0")"
for t in tetris features games; do
  c++ -std=c++17 -w -fsanitize=address,undefined -I stub "$t.cpp" -o "/tmp/host_esp32c3_$t"
  "/tmp/host_esp32c3_$t"
done
