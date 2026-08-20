#!/usr/bin/env bash
# =============================================================================
# ASan + UBSan 运行时检测构建脚本 — Aurora Launcher Core
#
# 用法：
#   ./tools/build-sanitized.sh              # Debug + ASan + UBSan
#   ./tools/build-sanitized.sh release      # Release + ASan + UBSan（慢）
#
# 之后运行测试：
#   ./tests/LoggerTest
#
# 内存泄漏报告由 ASan 自动输出到终端。
# =============================================================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-sanitized"
MODE="${1:-debug}"

SAN_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize=recover"

echo "=== Sanitized Build (${MODE}) ==="
echo "Project: $PROJECT_DIR"
echo "Build:   $BUILD_DIR"
echo "San flags: $SAN_FLAGS"
echo ""

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

cmake -S "${PROJECT_DIR}" \
      -B "${BUILD_DIR}" \
      -DCMAKE_BUILD_TYPE="${MODE}" \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_FLAGS="${SAN_FLAGS}" \
      -DCMAKE_C_FLAGS="${SAN_FLAGS}" \
      -DCMAKE_EXE_LINKER_FLAGS="${SAN_FLAGS}"

cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"

echo ""
echo "Build complete. Run tests:"
echo "  ${BUILD_DIR}/tests/LoggerTest"
echo ""
echo "To check for leaks specifically:"
echo "  ASAN_OPTIONS=detect_leaks=1 ${BUILD_DIR}/tests/LoggerTest"
