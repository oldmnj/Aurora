#!/bin/zsh
# =============================================================================
# .clang-tidy 批量检查脚本 — Aurora Launcher Core
#
# 用法：
#   ./tools/run-clang-tidy.sh                     # 检查所有源文件
#   ./tools/run-clang-tidy.sh src/base/error.cc   # 检查单个文件
#   ./tools/run-clang-tidy.sh --fix               # 自动修复可修复的问题
#
# 输出写入文件：
#   ./tools/run-clang-tidy.sh src/base/error.cc > /path/to/report.txt
# =============================================================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "${PROJECT_DIR}"

FIX_ARGS=""
SOURCES=()

for arg in "$@"; do
    case $arg in
        --fix) FIX_ARGS="--fix" ;;
        *) [[ -f "$arg" ]] && SOURCES+=("$arg") ;;
    esac
done

# 默认扫描所有非空 .cc 文件
if [[ ${#SOURCES[@]} -eq 0 ]]; then
    while IFS= read -r -d '' f; do
        [[ -s "$f" ]] && SOURCES+=("$f")
    done < <(find src/ tests/ examples/ -name '*.cc' -print0 2>/dev/null)
fi

echo "=== clang-tidy Check ==="
echo "Files: ${#SOURCES[@]}"
[[ -n "$FIX_ARGS" ]] && echo "Mode: --fix"
echo ""

CHECKS='bugprone-*,cppcoreguidelines-*,modernize-*,readability-*,clang-analyzer.*'

for src in "${SOURCES[@]}"; do
    echo "--- $src ---"
    timeout 60 clang-tidy "$src" \
        -p build-release \
        --checks="$CHECKS" \
        --header-filter='src/.*' \
        $FIX_ARGS \
        2>&1 | grep -v "redundant.*operator" \
               | grep -v "note: previously declared" \
               | grep -v "Suppressed" \
               | grep -v "^Use -header-filter" \
               | grep -v "^warning: redundant" \
               || true
    echo ""
done

echo "Done."
