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
echo "──────────────────────────────────────"
echo "▶ Running SCHDULE TESTS"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_schedule"
echo ""
echo "──────────────────────────────────────"
echo "▶ Running CACHE TESTS"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_cache"
echo ""
echo "──────────────────────────────────────"
echo "▶ Running PRESENCE TESTS"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_presence"
echo ""
echo "──────────────────────────────────────"
echo "▶ Running LOCK TESTS"
echo "──────────────────────────────────────"
"$BUILD_DIR/test_lock"