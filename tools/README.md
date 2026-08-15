# =============================================================================
# Aurora Launcher Core — 静态分析与质量保障工具
# =============================================================================

## 快速开始

### 1. clang-tidy（编译期规则检查）
```bash
# 安装依赖（Termux）
pkg install clang
pip install clang-tidy   # 通常随 clang 安装

# 运行检查
./tools/run-clang-tidy.sh

# 自动修复
./tools/run-clang-tidy.sh --fix
```
配置文件：`.clang-tidy`（已配置，聚焦内存安全/空指针/生命周期）

---

### 2. scan-build（跨函数数据流分析）
```bash
# 首次运行会重新构建项目（~2-5分钟）
./tools/run-scan-build.sh

# 完整分析（含头文件，更慢）
./tools/run-scan-build.sh --full

# 打开 HTML 报告
./tools/run-scan-build.sh --html

# CI 集成（SARIF 格式）
./tools/run-scan-build.sh --sarif
```
报告目录：`scan-reports/`
查看报告：`scan-view scan-reports/HTML-<date>/`

---

### 3. ASan + UBSan（运行时检测）
```bash
# 构建带 sanitizer 的调试版本
./tools/build-sanitized.sh

# 运行测试（自动检测内存错误）
ASAN_OPTIONS=detect_leaks=1 ./build-sanitized/tests/LoggerTest
```
检测内容：
- **ASan**：use-after-free、double-free、heap-buffer-overflow、memory-leak
- **UBSan**：空指针解引用、有符号整数溢出、对齐错误、类型转换非法

---

### 4. cppcheck（补充静态分析）
```bash
# 安装
pkg install cppcheck

# 运行（对 .cc 文件，跳过 .cppm）
cppcheck --enable=warning,style,performance --std=c++23 \
         --inline-suppr \
         src/base/ src/aurora/service/ \
         2>&1 | grep -v "style\|warning: .* naming"
```
注意：cppcheck 对 C++ Modules（.cppm）支持不稳定，建议只分析 .cc 文件。

---

## 推荐工作流

| 场景 | 工具 | 命令 |
|---|---|---|
| 日常开发 | clang-tidy | `./tools/run-clang-tidy.sh` |
| Merge 前 | scan-build | `./tools/run-scan-build.sh` |
| 调试内存问题 | ASan | `./tools/build-sanitized.sh && ASAN_OPTIONS=detect_leaks=1 ./...` |
| CI 流水线 | scan-build (sarif) | `./tools/run-scan-build.sh --sarif` |

---

## 已知的误报/需排除项

以下问题在 `.clang-tidy` 中已通过规则配置缓解，若仍出现可忽略：

1. **`cert-dcl16-c`**：项目内部使用 `details::` 命名空间，下划线前缀是约定而非标准库冲突
2. **`hicpp-named-parameter`**：项目采用 positional 构造（如 `Result{InPlaceValueTag{}, val}`），非 named parameter 风格
3. **`cppcoreguidelines-non-copyable-classes`**：`Result<T,E>` 是 intentionally copyable 的值类型
4. **`readability-identifier-length`**：项目内部类型如 `has_value_`、`storage_` 是有意命名
