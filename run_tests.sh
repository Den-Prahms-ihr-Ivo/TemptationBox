#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/firmware/test/host"
BUILD_DIR="$TEST_DIR/build"

echo "╔══════════════════════════════════════╗"
echo "║     Temptation Box — Host Tests      ║"
echo "╚══════════════════════════════════════╝"
echo ""

echo "▶ Configuring..."
cmake -B "$BUILD_DIR" -S "$TEST_DIR" -DCMAKE_BUILD_TYPE=Debug --log-level=WARNING

echo "▶ Building..."
cmake --build "$BUILD_DIR" --parallel

echo ""
echo "▶ Running..."
echo "──────────────────────────────────────"
"$BUILD_DIR/test_schedule"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_cache"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_presence"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_lock"