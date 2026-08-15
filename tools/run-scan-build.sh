#!/bin/zsh
# =============================================================================
# scan-build 分析脚本 — Aurora Launcher Core
#
# 用法：
#   ./tools/run-scan-build.sh              # 快速分析（只分析 src/ 下的 .cc）
#   ./tools/run-scan-build.sh --full       # 完整分析（含 tests/ examples/）
#   ./tools/run-scan-build.sh --html       # 分析并自动打开 HTML 报告
#   ./tools/run-scan-build.sh --sarif      # 输出 SARIF 格式（供 CI 集成）
#
# 前置要求：
#   pkg install clang        # scan-build 已内置于 clang 包
#   pip install scanview     # scan-view 用于查看结果
# =============================================================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-scan"
REPORT_DIR="${PROJECT_DIR}/scan-reports"

# --- 参数解析 ---
MODE="quick"   # quick | full | sarif
OPEN_HTML=false

for arg in "$@"; do
    case $arg in
        --full)   MODE="full" ;;
        --sarif)  MODE="sarif" ;;
        --html)   OPEN_HTML=true ;;
        *) echo "Unknown arg: $arg"; exit 1 ;;
    esac
done

echo "=== Aurora Scan-Build Analysis ==="
echo "Mode: $MODE"
echo "Project: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"
echo "Report dir: $REPORT_DIR"
echo ""

# --- 清理旧构建 ---
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${REPORT_DIR}"

# --- 可选：分析头文件（慢，全量时用）---
ANALYZE_HEADERS_FLAG=""
if [[ "$MODE" == "full" ]]; then
    ANALYZE_HEADERS_FLAG="--analyze-headers"
fi

# --- 运行 scan-build + cmake build ---
echo "[1/3] Running scan-build with cmake..."

SCAN_BUILD_OPTS="-o ${REPORT_DIR} ${ANALYZE_HEADERS_FLAG}"

if [[ "$MODE" == "sarif" ]]; then
    SCAN_BUILD_OPTS="${SCAN_BUILD_OPTS} --sarif -stats -statsdir ${REPORT_DIR}/stats"
fi

scan-build ${SCAN_BUILD_OPTS} \
    cmake -S "${PROJECT_DIR}" \
          -B "${BUILD_DIR}" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DCMAKE_C_COMPILER=clang

scan-build ${SCAN_BUILD_OPTS} \
    cmake --build "${BUILD_DIR}" \
          -j"$(nproc 2>/dev/null || echo 4)"

echo ""
echo "[2/3] Analysis complete."

# --- 统计结果 ---
LATEST_REPORT=$(ls -td "${REPORT_DIR}"/HTML* 2>/dev/null | head -1)
if [[ -z "$LATEST_REPORT" ]]; then
    # sarif 模式
    LATEST_SARIF=$(ls -td "${REPORT_DIR}"/results-* 2>/dev/null | head -1)
    if [[ -n "$LATEST_SARIF" ]]; then
        echo "SARIF report: ${LATEST_SARIF}"
    else
        echo "WARNING: No analysis reports found!"
        exit 1
    fi
else
    REPORT_COUNT=$(find "$LATEST_REPORT" -name "*.html" 2>/dev/null | wc -l)
    BUG_COUNT=$(grep -r "class=\"bugreport\|class=\"bug\|status.*error\|Issue" "$LATEST_REPORT" 2>/dev/null | grep -c "." || true)
    echo "Report directory: ${LATEST_REPORT}"
    echo "HTML files: ${REPORT_COUNT}"
    echo ""

    # 显示关键摘要
    if command -v python3 &>/dev/null; then
        python3 - <<'PYEOF'
import os, glob, re

report_dir = None
for d in sorted(glob.glob("scan-reports/HTML*/index.html"), key=os.path.getmtime, reverse=True):
    report_dir = os.path.dirname(d)
    break

if not report_dir:
    print("(no reports found)")
    exit(0)

print(f"Latest report: {report_dir}/")
print("=" * 60)

# 统计 bug 数量
bug_count = 0
files_with_bugs = set()
for html_file in glob.glob(f"{report_dir}/**/*.html", recursive=True):
    try:
        with open(html_file, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
        # 查找缺陷条目
        bugs = re.findall(r'class="bugreport|class="issue|data-bug-id', content)
        if bugs:
            bug_count += len(bugs)
            files_with_bugs.add(os.path.basename(html_file))
    except:
        pass

print(f"Total issues found: {bug_count}")
if files_with_bugs:
    print(f"Files with issues: {len(files_with_bugs)}")
    for f in sorted(files_with_bugs)[:20]:
        print(f"  - {f}")
PYEOF
    fi

    echo ""
    echo "[3/3] Opening report..."
    if [[ "$OPEN_HTML" == true ]]; then
        scan-view "${LATEST_REPORT}" &
        echo "scan-view started in background."
    else
        echo "View report with: scan-view ${LATEST_REPORT}"
    fi
fi

echo ""
echo "Done."
