# Aurora Launcher Core 设计文档

| 项 | 内容 |
| :--- | :--- |
| 项目 | Aurora Launcher Core |
| 文档版本 | 0.1.0 |
| 状态 | 定稿待评审 |
| 日期 | 2026-08-11 |
| 读者 | 项目内部开发人员 |
| 配套文档 | `DEVELOPMENT.md`（代码规范、git 规范、模块命名规范） |

> 本文档为"目标设计"，描述 Aurora Launcher Core 的完整架构蓝图，包含尚未实现的模块。实现进度以 `src/CMakeLists.txt` 与各模块文件现状为准，本设计文档不保证与当前代码行级一致，但所有已实现模块的对外 API 均以当前代码为准。

---

## 目录

1. [项目概述](#1-项目概述)
2. [设计原则与总体架构](#2-设计原则与总体架构)
3. [基座层 Base Layer](#3-基座层-base-layer)
4. [服务层 Service Layer](#4-服务层-service-layer)
5. [核心层 Core Layer](#5-核心层-core-layer)
6. [运行时层 Runtime Layer](#6-运行时层-runtime-layer)
7. [插件系统](#7-插件系统)
8. [C ABI](#8-c-abi)
9. [认证专项设计](#9-认证专项设计)
10. [下载与缓存专项设计](#10-下载与缓存专项设计)
11. [事件系统设计](#11-事件系统设计)
12. [配置体系设计](#12-配置体系设计)
13. [目录与文件布局](#13-目录与文件布局)
14. [构建与依赖管理](#14-构建与依赖管理)
15. [测试与质量保障](#15-测试与质量保障)
16. [开发路线图](#16-开发路线图)
17. [风险与开放问题](#17-风险与开放问题)
18. [并发与线程模型](#18-并发与线程模型)
19. [内存管理与对象生命周期](#19-内存管理与对象生命周期)
20. [安全设计](#20-安全设计)
21. [性能设计](#21-性能设计)
22. [可测试性设计](#22-可测试性设计)
23. [日志与诊断设计](#23-日志与诊断设计)
24. [模块实现规范](#24-模块实现规范implementation-notes)
25. [编码与工程规范](#25-编码与工程规范)
26. [兼容性与演进策略](#26-兼容性与演进策略)

附录 A [错误码与错误分类总表](#附录-a-错误码与错误分类总表)（含 A.2 错误分类）
附录 B [JS 桥接 API 参考](#附录-b-js-桥接-api-参考)
附录 C [模块文件清单](#附录-c-模块文件清单)
附录 D [version.json 字段映射表](#附录-d-versionjson-字段映射表详细)
附录 E [事件结构体定义](#附录-e-事件结构体定义)
附录 F [API 索引](#附录-f-api-索引)
附录 G [参考文档](#附录-g-参考文档)
# 1. 项目概述

## 1.1 项目定位

Aurora Launcher Core 是一个基于 C++23 Modules 构建的、跨平台的 **Minecraft Java 版 Runtime SDK**。

它不是某个固定 UI 的 Minecraft 启动器，而是一个可被嵌入、可扩展、可跨语言调用的 **Minecraft Runtime Framework**。宿主应用（未来的 CLI、Web、GUI）负责全部用户交互，Aurora Core 只负责 **Minecraft Runtime Lifecycle**：

```
用户请求启动游戏
        ↓
版本解析 Version Resolve
        ↓
资源准备 Asset Prepare
        ↓
Java 环境检测 JVM Prepare
        ↓
参数生成 Argument Generate
        ↓
进程创建 Process Spawn
        ↓
游戏运行管理 Runtime Monitor
        ↓
退出/崩溃处理 Exit Handle
```

## 1.2 目标

- 支持 Minecraft Java 版**全部正式版本与快照**的启动，覆盖从远古版本（`old_alpha` / `old_beta` / 1.0–1.6 legacy 参数与资源格式）到最新版本的全部协议差异。
- 支持四种加载器：Vanilla、Fabric、Quilt、Forge（含 1.13+ 现代 Forge）、NeoForge。
- 支持三种认证方式：微软 OAuth（设备码流）、离线模式、第三方皮肤站（authlib-injector 兼容 API）。
- 无 GUI、无窗口管理；以库的形式嵌入，提供稳定的 **C ABI** 供非 C++ 语言（Rust / Python / C# / Java 等）绑定。
- 以 Drogon 的 App 风格提供统一的 `Context` 应用对象：服务注册、生命周期、事件分发、插件注册集中管理。
- 提供 C++ 编译期插件与 QuickJS JavaScript 插件两套扩展机制。
- 进程内禁止异常作为接口，统一使用 `Result<T, E>` 错误模型，保证 C ABI 稳定。

## 1.3 非目标

- 不提供用户界面、窗口管理、用户交互、UI 状态管理（CLI / Web / GUI 由宿主自行实现）。
- 不提供游戏服务端、联机服务端、Mod 分发平台。
- 不内置微软/第三方账号 UI 流程（只提供设备码等无 UI 的认证流，并暴露事件由宿主展示）。
- 第一版不提供二进制插件（DLL/SO 动态加载），插件以**编译期 C++ 插件**与 **QuickJS JS 插件**两种形式存在。
- 不承诺支持除 Minecraft Java 版以外的游戏。

## 1.4 术语表

| 术语 | 含义 |
| :--- | :--- |
| Runtime Lifecycle | 从收到启动请求到游戏进程退出/崩溃的完整生命周期 |
| Context | 进程级应用对象（Drogon `app()` 风格），服务注册表与生命周期管理者 |
| Service | 由 Context 管理的基础服务（网络、下载、认证、日志等） |
| LaunchProfile | 一次启动的完整配置（版本、加载器、内存、游戏目录、认证等） |
| LaunchRecipe | 版本解析完成后生成的"可启动配方"（类路径、参数、主类、资源索引等） |
| VersionProfile | 单个 Minecraft 版本的 `version.json` 解析模型 |
| AssetIndex | 资源索引 JSON（objects: path → hash/size） |
| Loader | Forge/Fabric/NeoForge/Quilt 等对版本 JSON 的修补层 |
| authlib-injector | 第三方皮肤站注入器，通过 `-javaagent` 改变认证服务地址 |
| 设备码流 | Microsoft OAuth Device Code Flow，适合无浏览器的库环境 |
| C ABI | 以 C 语言约定（extern "C" + 纯 C 类型）导出的稳定二进制接口 |

---

# 2. 设计原则与总体架构

## 2.1 设计原则

1. **单向分层依赖**：上层依赖下层，下层禁止反向依赖；基础设施必须稳定。
2. **Context 为中心**：一切可变全局状态收敛到 `Context`；业务代码通过 `context.Get<T>()` 获取服务，禁止散落全局单例。
3. **面向服务设计**：服务只提供"能力"，不感知 Minecraft 业务；Minecraft 业务全部位于 Runtime 层。
4. **错误显式化**：进程内禁止异常作为接口，统一 `Result<T, E>` + `Error`；第三方库异常在封装边界内捕获并转换。
5. **RAII 与智能指针**：内存一律使用 `UniquePtr` / `SharedPtr` / RAII，禁止裸 `new`/`delete`、`malloc`/`free`。
6. **第三方库不泄漏**：第三方 API 不得直接暴露给用户，必须经 `launcher` 封装。
7. **ABI 稳定优先**：对外只暴露 C ABI；C++ API 仅面向 C++ 宿主，不承诺 ABI。
8. **事件驱动**：耗时流程（下载、认证、启动）通过事件对外报告进度与结果，宿主只订阅不轮询。

## 2.2 分层架构

```
┌─────────────────────────────────────────────┐
│            Host Application (宿主)           │  CLI / Web / GUI（非本库范围）
└─────────────────────────────────────────────┘
                    │  C ABI / C++ API
┌─────────────────────────────────────────────┐
│               Runtime Layer                 │  aurora.runtime  Minecraft 业务
│   version assets jvm process launch loader  │
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│                Core Layer                   │  aurora.core 运行内核
│   context task pool scheduler event         │
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│              Service Layer                  │  aurora.service 通用能力
│ logger random uuid crypto io network        │
│ download archive cache quickjs auth         │
└─────────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────────┐
│               Base Layer                    │  launcher.base 基础
│   types error platform build config         │
└─────────────────────────────────────────────┘
```

## 2.3 依赖规则

允许的依赖方向：

```
Runtime → Core → Service → Base
```

| 方向 | 允许 | 说明 |
| :--- | :--- | :--- |
| base → base | ✅ | 基座内部模块可互相依赖 |
| service → base | ✅ | 服务层只能依赖基座 |
| core → base / core → service | ✅ | 核心层可依赖服务层能力（调度、事件） |
| runtime → core / runtime → service / runtime → base | ✅ | 运行时层使用核心与服务能力 |
| 任何反向依赖 | ❌ | 禁止，编译期由模块导入检查保证 |

> 工程约束：`.cppm` 文件在 `src/CMakeLists.txt` 中必须按模块依赖顺序排列（已实现），新增模块必须遵守该顺序。

## 2.4 核心概念

### 2.4.1 Context（应用对象）

`Context` 是 Aurora 的运行时环境，等价于 Drogon 的 `app()`、Tokio 的 Runtime。它拥有：

```
Context
 ├── Config           配置（只读视图）
 ├── ServiceRegistry  服务注册表：context.Get<T>()
 ├── Scheduler        调度器（线程池 + 任务队列）
 ├── EventDispatcher  事件分发器
 ├── PluginRegistry   插件注册表
 └── Lifecycle        初始化/启动/停止/反初始化
```

设计决策：**Context 采用进程级默认单例**（`Context::Instance()`），同时保留 `Context::Create(config)` 创建隔离实例的能力，用于嵌入与测试。第一版以单默认实例为主，多实例并发运行不在本期承诺范围内。

### 2.4.2 Service（服务）

- 服务 = "能力"，不感知 Minecraft。
- 有状态服务（`Network`、`DownloadManager`、`AuthManager`、`Cache`、`QuickJS`）以实例形式注册进 Context，通过 `context.Get<T>()` 获取。
- 纯函数服务（`Random`、`UUID`、`Crypto`、`IO`、`Archive`）保持静态工具类形态，不持有全局状态。
- `Logger` 提供静态门面（`Logger::Info(...)`），内部路由到当前 Context 持有的日志实现，以兼容现有调用方式。

### 2.4.3 Task（协程任务）

- `Task<T, E>` 是基于 C++20 协程的统一异步抽象（`launcher.base.error` 的 `Result` 作为协程的 value/error 载体）。
- Task 只表示"可暂停可恢复的计算"，**不负责线程**；调度交给 `Scheduler`。

### 2.4.4 Event（事件）

- 类型安全的事件总线：`context.Events().Publish<E>(...)`、`context.Events().Subscribe<E>(handler)`。
- 事件是普通 struct；用于下载进度、启动阶段、游戏输出、认证流程、插件生命周期等。

### 2.4.5 Plugin（插件）

- C++ 编译期插件：在宿主编译期集成，Drogon 风格 `context.RegisterPlugin<MyPlugin>("name")`。
- JS 插件：QuickJS 沙箱执行，通过一组 C 桥接函数（与 C ABI 同源）访问核心能力。

## 2.5 模块总览

| 层 | 模块文件 | 职责 | 现状 |
| :--- | :--- | :--- | :--- |
| base | launcher.base.types | 统一基础类型 | ✅ 已实现 |
| base | launcher.base.error | `Error` / `Result<T,E>` 错误系统 | ✅ 已实现 |
| base | launcher.base.platform | 平台/架构/版本枚举与探测 | ✅ 已实现 |
| base | launcher.base.build | 构建信息 BuildInfo | 🟡 声明完整，实现待补 |
| base | launcher.base.config | 配置模型与 ConfigManager | 🟡 部分实现 |
| service | aurora.service.random | 安全随机（OpenSSL） | ✅ 已实现 |
| service | aurora.service.uuid | UUID v4/v7 | ✅ 已实现 |
| service | aurora.service.logger | 日志（spdlog 封装） | ✅ 已实现 |
| service | aurora.service.crypto | 哈希/编码/加密 | ⬜ 空壳 |
| service | aurora.service.io | 文件与路径操作 | ⬜ 空壳 |
| service | aurora.service.network | HTTP 客户端（libcurl） | ⬜ 空壳 |
| service | aurora.service.download | 下载管理器 | ⬜ 空壳（暂未导出） |
| service | aurora.service.archive | 压缩解压（minizip-ng） | ⬜ 空壳 |
| service | aurora.service.cache | 磁盘缓存 | ⬜ 空壳 |
| service | aurora.service.quickjs | JS 引擎封装 | ⬜ 空壳 |
| service | aurora.service.auth | 认证抽象与实现 | ⬜ 空壳 |
| core | aurora.core.task | 协程任务 | 🟡 有基础实现，待完善 |
| core | aurora.core.pool | 线程池 | ⬜ 空壳 |
| core | aurora.core.scheduler | 任务调度 | ⬜ 空壳 |
| core | aurora.core.context | 应用对象 Context | ⬜ 空壳 |
| core | aurora.core.event | 事件总线 | ⬜ 空壳 |
| runtime | aurora.runtime.version | 版本清单与解析 | ⬜ 空壳 |
| runtime | aurora.runtime.assets | 资源准备 | ⬜ 空壳 |
| runtime | aurora.runtime.jvm | JVM 探测与参数 | ⬜ 空壳 |
| runtime | aurora.runtime.process | 子进程管理 | ⬜ 空壳 |
| runtime | aurora.runtime.launch | 启动流水线 | ⬜ 空壳 |
| runtime | aurora.runtime.loader | 加载器修补 | ⬜ 空壳 |
| runtime | aurora.runtime.plugin | 运行时插件契约 | ⬜ 空壳 |
| capi | include/export.h 等 | C ABI | ⬜ 空壳 |

> 注：`aurora.service.download` 当前未在 `aurora.service.cppm` 中 export，属遗漏；设计上 `download` 属于 Service 层并被 `aurora.service` 统一导出。

## 2.6 进程生命周期

```
宿主进程启动
   ↓
ConfigManager::Load()            加载配置（默认 ./config.json）
   ↓
Context::Initialize(config)      创建默认 Context
   ├── 注册内置服务（依赖序：logger → random/uuid/crypto → io → network → archive/cache/download → quickjs → auth）
   ├── 创建 Scheduler（线程池按 config.runtime.worker_threads）
   └── 创建 EventDispatcher / PluginRegistry
   ↓
Context::Start()                 启动调度器与后台服务
   ↓
加载 C++ 插件 / 扫描并加载 JS 插件（plugins 目录）
   ↓
宿主业务（创建 LaunchProfile → 提交 LaunchController）
   ↓
Context::Shutdown()              停止所有任务 → 卸载插件 → 停止调度器 → 反初始化服务（倒序）→ Logger Flush
   ↓
进程退出
```

---

# 3. 基座层 Base Layer

模块：`launcher.base`（`src/launcher.base/*.cppm` + `src/base/*.cc`）

定位：Aurora 最底层公共基础模块。**Base 不包含任何业务**，禁止网络、IO、日志、第三方服务调用。

提供：基础类型、错误系统、平台抽象、构建信息、配置模型。

## 3.1 类型系统 `launcher.base:types`

目标：统一项目类型，避免 `std::string` / `QString` / `boost::string` 混用，保证 API 稳定与 C 转换清晰。

| 类别 | 别名 | 底层 |
| :--- | :--- | :--- |
| 有符号整数 | `i8 i16 i32 i64` | `std::int8_t` 等 |
| 无符号整数 | `u8 u16 u32 u64` | `std::uint8_t` 等 |
| 特殊整数 | `isize usize` | `std::ptrdiff_t` / `std::size_t` |
| 浮点 | `f32 f64` | `float` / `double` |
| 字节 | `Byte ByteSpan Bytes` | `std::byte` / `span<byte>` / `vector<byte>` |
| 字符串 | `String StringView` | `std::string` / `std::string_view` |
| 宽/UTF8 字符串 | `WString WStringView U8String U8StringView` | 标准库对应类型 |
| 路径 | `Path` | `std::filesystem::path` |
| 字符串常量 | `StringLiteral CString` | `const char *` |
| Span | `Span<T> ConstSpan<T> U8Span CharSpan ConstByteSpan` | `std::span` |
| 智能指针 | `UniquePtr<T> SharedPtr<T> WeakPtr<T>` | `std::unique_ptr` 等 |
| 容器 | `Vector<T> Optional<T>` | `std::vector` / `std::optional` |

规则：

- 新代码一律使用上述别名，禁止裸写 `std::vector` / `std::string` 等（保持风格统一）。
- 跨 C ABI 边界时，`String` 以 UTF-8 `const char *` 传递。
- `launcher.base:types` 不得依赖 `launcher.base:error` 等其他模块（避免循环）。

## 3.2 错误系统 `launcher.base:error`

### 3.2.1 设计目标

所有层级的错误（Service / Core / Runtime）最终统一转换为 `launcher::Error`，以 `Result<T, E>` 返回。进程内**禁止 `throw` 作为函数接口**。

### 3.2.2 `Error`

```cpp
class Error {
  public:
    Error(ErrorCategory category, ErrorCode code, StringView message,
          SharedPtr<Error> cause,
          std::source_location location = std::source_location::current());
    Error(ErrorCategory category, ErrorCode code, StringView message,
          std::source_location location = std::source_location::current());

    [[nodiscard]] ErrorCode Code() const noexcept;
    [[nodiscard]] StringView Message() const noexcept;
    [[nodiscard]] const std::source_location &Location() const noexcept;
    [[nodiscard]] ErrorCategory Category() const noexcept;
    [[nodiscard]] String ToString() const;
    [[nodiscard]] static constexpr StringView ToString(ErrorCode) noexcept;
    [[nodiscard]] static constexpr StringView ToString(ErrorCategory) noexcept;
};
```

- `cause_` 支持错误链（`SharedPtr<Error>`），用于包装第三方/底层错误时保留上下文。
- `source_location` 记录产生位置，便于日志定位；不参与相等性比较。

### 3.2.3 `Result<T, E = Error>`

自定义实现（基于 union 存储 + `std::construct_at`，非 `std::expected`，`std::expected` 已弃用并注释）：

- 构造：`Result()`（仅 `void` 特化，默认为成功）、`InPlaceValueTag` / `InPlaceErrorTag`、值/错误隐式构造、拷贝/移动、析构（按 `has_value_` 正确析构）。
- 查询：`HasValue()` / `HasError()`、`Value()` / `Error()`（`[[nodiscard]]`）。
- 组合：`Map` / `AndThen` / `OrElse`（`&&` / `const &` 重载）、`IfValue` / `IfError` 副作用链。
- 工厂：`Ok<T>()` / `Err<T>(err)` / `OkEmplace<T>(args...)` / `ErrEmplace<T>(args...)`。

**接口规范**（全库强制）：

```cpp
[[nodiscard]] Result<T> Function(...) const noexcept;   // public API 形态
```

**错误传播示例**：

```cpp
[[nodiscard]] Result<String> ReadConfig(Path path) noexcept {
    auto content = IO::ReadText(path);
    if (content.HasError()) {
        return Err<String>(content.Error());          // 透传
    }
    auto parsed = ParseJson(content.Value());
    if (parsed.HasError()) {
        return Err<String>({ErrorCategory::Config, ErrorCode::ParseError,
                            "配置 JSON 解析失败", parsed.Error()});  // 包装 + 错误链
    }
    return Ok<String>(std::move(parsed.Value()));
}
```

**第三方库异常转换规则**：

```
第三方库（nlohmann_json / OpenSSL / spdlog / libcurl ...）
        ↓ try / catch（仅限封装边界）
        ↓
Error（转换并记录 cause）
        ↓
Err(...) 返回
```

**顶层 catch 约定**：允许且仅在 `Context::Run()` / C ABI 入口的最顶层 catch，用于防止未知异常导致崩溃、记录日志、安全退出。

### 3.2.4 错误码与错误分类

完整错误码表与分类表见 [附录 A](#附录-a-错误码与错误分类总表)（错误码 A.1、分类 A.2）。设计要点：

- `ErrorCode` 按领域分组（通用 / IO / 网络 / 数据 / 下载 / 认证 / 运行时 / Java / 启动 / 插件 / 脚本），新增错误码只追加不修改既有枚举值，保证 ABI。
- `ErrorCategory` 用于日志聚合与 C ABI 分类返回：`None System Parse IO Network Security Config Runtime Minecraft Auth Plugin Script`。

## 3.3 平台抽象 `launcher.base:platform`

```cpp
export struct Version {
    u32 major{}, minor{}, patch{};
    constexpr auto operator<=>(const Version &) const = default;
    [[nodiscard]] String ToString() const;   // "{major}.{minor}.{patch}"
};

export enum class Platform { Windows, Linux, MacOS, Android, IOS, Unknown };
export enum class Architecture { X86, X64, ARM, ARM64, RISCV64, Unknown };

export [[nodiscard]] Platform CurrentPlatform() noexcept;
export [[nodiscard]] Architecture CurrentArchitecture() noexcept;
export [[nodiscard]] bool Is64Bit() noexcept;
export [[nodiscard]] bool IsLittleEndian() noexcept;
export constexpr StringView ToString(Platform) noexcept;
export constexpr StringView ToString(Architecture) noexcept;
```

- 平台探测宏：`_WIN32` / `__ANDROID__` / `__APPLE__` / `__linux__`。
- 架构探测宏：`__x86_64__` / `_M_X64`、`__i386__`、`__aarch64__`、`__arm__`、`__riscv`。
- `ToString` 输出小写稳定字符串（`"windows"` / `"linux"` / `"macos"` / `"android"` / `"ios"` / `"unknown"`），供规则求值（Rules Engine）与配置匹配使用。

## 3.4 构建信息 `launcher.base:build`

```cpp
export struct BuildInfo {
    Version version;      // 内核版本（当前 0.1.0）
    StringView name;      // 项目名（KernelName）
    StringView compiler;  // __clang_version__ / __VERSION__ / _MSC_VER 等
    StringView build_type;// Debug / Release / RelWithDebInfo ...
};
```

- 由 CMake 生成头/定义注入，`BuildInfo::Current()` 返回编译期常量。
- 用于日志头、诊断信息、C ABI `au_version()` 输出。

## 3.5 配置模型 `launcher.base:config`

完整配置 JSON Schema 见 [第 12 章](#12-配置体系设计)。此处定义模型与规则。

```cpp
export struct PathConfig {
    Path cache_directory   = "./cache";
    Path temp_directory    = "./tmp";
    Path log_directory     = "./log";
    Path runtime_directory = "./runtime";
};

export struct LoggerConfig {
    LogLevel level         = LogLevel::Info;
    bool flush_immediately = false;
    bool console_output    = true;
    bool file_output       = true;
    Path file_name         = "launcher.log";
    bool is_async          = false;
};

export struct NetworkConfig {
    std::chrono::seconds timeout = std::chrono::seconds{30};
    u32 retry_count              = 3;
    bool verify_ssl              = true;
};

export struct RuntimeConfig {
    u32 worker_threads = std::max(4u, std::thread::hardware_concurrency());
    bool debug_mode    = false;
    bool enable_cache  = true;
};

export class Config {
  public:
    Config() = default;
    Result<void> Validate() const;  // 校验合法性
    void Reset();

    PathConfig path;
    LoggerConfig logger;
    NetworkConfig network;
    RuntimeConfig runtime;
};

export class ConfigManager {
  public:
    static void Load();                       // 从默认位置加载（失败时回退默认配置）
    static const Config &Get();
    static Result<void> Load(Path);           // 显式路径加载
    static Result<void> Save(Path);           // 原子写入
};
```

**路径解析规则**：

- `runtime_directory`：相对路径基于进程 CWD 解析。
- `cache/temp/log_directory`：相对路径基于 `runtime_directory` 解析（`runtime/cache`、`runtime/tmp`、`runtime/log`）；绝对路径原样使用。
- `temp_directory` 语义：下载临时目录，进程退出后须清理。

**校验规则**（`Validate()`）：

- 四个路径字段不得为空。
- `network.timeout > 0`、`runtime.worker_threads >= 1`。
- 非法则返回 `ErrorCategory::Config` + `ErrorCode::InvalidArgument`。

**ConfigManager 行为**：

- `Load()` 默认查找顺序：`$AURORA_CONFIG` 环境变量 → 进程 CWD `config.json` → 不存在则使用默认配置（不报错）。
- 加载 = 默认值 + 文件覆盖（JSON 缺省字段不覆盖默认值）。
- `Save()` 使用"临时文件 + 原子重命名"，避免写坏配置文件。

**内部存储**：`ConfigManager` 内部使用 `details::g_config`（`SharedPtr<Config>`）存放于 `details` 命名空间，避免 `.cppm` 导出全局符号。

---

# 4. 服务层 Service Layer

模块：`aurora.service`（`src/aurora.service/*.cppm` + `src/aurora/service/*.cc`）

定位：提供通用基础服务。**Service 不感知 Minecraft**，只提供"能力"。

## 4.0 服务层约定

1. **注册与获取**：有状态服务由 `Context::Initialize` 按依赖顺序注册，业务通过 `context.Get<T>()` 获取；禁止自定义全局单例。
2. **线程安全**：所有服务接口线程安全（内部加锁或串行化）；不保证跨线程的返回对象安全（如 `StringView` 仅在调用期有效）。
3. **第三方封装**：第三方库 API 不得直接暴露，全部在服务内部封装。
4. **错误返回**：一律 `Result<T>`，禁止异常外泄。
5. **异步能力**：`network`、`download`、`auth` 提供异步接口（`Task<T,E>`），由 `aurora.core:scheduler` 调度。

## 4.1 日志 `aurora.service:logger`

### 现状

已实现：基于 spdlog + fmt 的静态门面，`console` / `file` 双 sink，异步开关，级别控制。实现文件：`src/aurora/service/logger.cc`、`logger_pattern.cc`、`logger_sink.cc`。

### 接口

```cpp
export class Logger {
  public:
    static Result<void> Initialize(const LoggerConfig &config);
    static bool IsInitialized() noexcept;
    static void Shutdown();
    static void SetLevel(LogLevel level) noexcept;
    static LogLevel Level() noexcept;

    template <typename... Args>
    static void Trace(fmt::format_string<Args...> fmt, Args &&...args);
    template <typename... Args>
    static void Debug(fmt::format_string<Args...> fmt, Args &&...args);
    template <typename... Args>
    static void Info(fmt::format_string<Args...> fmt, Args &&...args);
    template <typename... Args>
    static void Warn(fmt::format_string<Args...> fmt, Args &&...args);
    template <typename... Args>
    static void Error(fmt::format_string<Args...> fmt, Args &&...args);
    template <typename... Args>
    static void Critical(fmt::format_string<Args...> fmt, Args &&...args);
};
```

### 设计要点

- **门面与实现分离**：`Logger::*` 静态门面路由到当前 Context 的日志实现；`Initialize` 幂等（重复调用返回 `Ok`）。
- **Sink 体系**：`Console Sink`（stdout 彩色）、`File Sink`（`file_name`，UTF-8）；预留 `Plugin Sink`（插件可注册自定义 sink，后续版本）。
- **异步日志**：`is_async = true` 时使用 spdlog async logger（队列容量 8192，满时丢弃最旧），`Shutdown` 时必须 flush。
- **无目标时报错**：`console_output` 与 `file_output` 均为 false 时返回 `Err(InvalidArgument, "Logger没有任何输出目标")`。
- **pattern 与等级颜色**：由 `logger_pattern.cc` / `logger_sink.cc` 统一管理；`debug_mode`（RuntimeConfig）打开时级别降为 `Debug`。
- **内部实现**：`details::g_logger`（`SharedPtr<spdlog::logger>`）+ `details::GetLogger()` 供门面查询；`ToSpdlogLevel` 做等级映射。日志实现不依赖 Core / Runtime。

## 4.2 安全随机 `aurora.service:random`

已实现，基于 OpenSSL `RAND_bytes`。

```cpp
export class Random {
  public:
    static auto Bytes(usize size) -> Result<launcher::Bytes>;
    static auto UInt32() -> Result<u32>;
    static auto UInt64() -> Result<u64>;
    static auto Fill(ByteSpan buffer) -> Result<void>;
};
```

规则：

- 禁止使用 `rand()` / `std::random_device` 作为安全随机源。
- `Bytes(0)` 返回 `Err(InvalidArgument)`（保留现状语义）。
- 失败返回 `ErrorCategory::Runtime` + `InternalError`（RAND_bytes 内部失败）。

## 4.3 UUID `aurora.service:uuid`

已实现（V4 / V7）。**只支持 V4 / V7**，其余版本（v1/v3/v5/v6/v8）不在设计范围内（proj.md 中的 v1/v6/v8 规划废弃）。

```cpp
export enum class UUIDVersion : u8 { Nil = 0, Random = 4, UnixTime = 7 };
export enum class UUIDVariant : u8 { NCS, RFC4122, Microsoft, Future };

export class UUID {
  public:
    constexpr UUID() noexcept;                       // Nil
    explicit UUID(const std::array<u8, 16> &data);
    static Result<UUID> Parse(StringView);
    String ToString() const;                         // RFC 4122 8-4-4-4-12 小写
    ConstSpan<u8> Bytes() const noexcept;
    UUIDVersion Version() const noexcept;
    UUIDVariant Variant() const noexcept;
    bool operator==(const UUID &) const noexcept;
};

export class UUIDGenerator {
  public:
    static auto V4() -> Result<UUID>;   // 随机
    static auto V7() -> Result<UUID>;   // Unix 时间戳 + 随机
};
```

位布局：

- **V4**：`data[6] = (data[6] & 0x0F) | 0x40`，`data[8] = (data[8] & 0x3F) | 0x80`。
- **V7**：前 48 bit 为 Unix 毫秒时间戳（big-endian），`data[6]` 版本位 0x70，`data[8]` 变体位 0x80。

用途：下载任务 ID、认证会话 ID、启动会话 ID、事件追踪。

## 4.4 密码学 `aurora.service:crypto`

空壳，设计如下。基于 OpenSSL EVP API 封装。

```cpp
export class Crypto {
  public:
    // 哈希（流式读文件，避免大文件整读）
    static auto Sha1(ConstByteSpan data) -> Result<Bytes>;
    static auto Sha256(ConstByteSpan data) -> Result<Bytes>;
    static auto Sha512(ConstByteSpan data) -> Result<Bytes>;
    static auto Md5(ConstByteSpan data) -> Result<Bytes>;        // 仅兼容旧协议
    static auto Sha1File(Path file) -> Result<Bytes>;
    static auto Sha256File(Path file) -> Result<Bytes>;

    // 十六进制 / Base64
    static auto HexEncode(ConstByteSpan data) -> Result<String>;
    static auto Base64Encode(ConstByteSpan data) -> Result<String>;
    static auto Base64Decode(StringView data) -> Result<Bytes>;

    // MAC / 对称加密（供账号凭据脱敏与未来扩展）
    static auto HmacSha256(ConstByteSpan key, ConstByteSpan data) -> Result<Bytes>;
    static auto AesGcmEncrypt(ConstByteSpan key, ConstByteSpan iv,
                              ConstByteSpan plain, ConstByteSpan aad) -> Result<Bytes>;
    static auto AesGcmDecrypt(ConstByteSpan key, ConstByteSpan iv,
                              ConstByteSpan cipher, ConstByteSpan aad) -> Result<Bytes>;
};
```

用途：下载校验（sha1/sha256）、Mojang 版本/资源校验、日志与配置脱敏、`account.json` 刷新令牌静态加密（见 9.5）。

## 4.5 IO `aurora.service:io`

空壳，设计如下。

```cpp
export class IO {
  public:
    // 文件
    static auto ReadFile(Path file) -> Result<Bytes>;
    static auto WriteFile(Path file, ConstByteSpan data) -> Result<void>;
    static auto WriteFileAtomic(Path file, ConstByteSpan data) -> Result<void>;  // tmp+rename
    static auto ReadText(Path file) -> Result<String>;
    static auto WriteText(Path file, StringView text) -> Result<void>;

    // 路径
    static auto Mkdirs(Path dir) -> Result<void>;
    static auto RemoveAll(Path path) -> Result<void>;
    static auto Exists(Path path) -> Result<bool>;
    static auto IsDirectory(Path path) -> Result<bool>;
    static auto CopyFile(Path from, Path to) -> Result<void>;
    static auto FileSize(Path file) -> Result<u64>;

    // JSON
    static auto JsonRead(Path file) -> Result<nlohmann::json>;
    static auto JsonWrite(Path file, const nlohmann::json &value) -> Result<void>;
};
```

要点：

- `WriteFileAtomic` 写 `path.tmp` 后 rename，保证下载校验失败不污染目标。
- 所有文件错误映射为 `ErrorCategory::IO`（`IOError` / `FileNotFound` / `PermissionDenied`）。
- 路径操作基于 `std::filesystem`，错误经 `error_code` 转换，禁止抛 `filesystem_error`。
- 为 C ABI 与 JS 桥提供 `fs.*` 能力（权限受插件清单控制）。

## 4.6 网络 `aurora.service:network`

空壳，设计如下。基于 libcurl **multi 接口**封装，异步完成回调投递到 Scheduler。

```cpp
export struct HttpHeader { String name; String value; };

export struct RequestOptions {
    Vector<HttpHeader> headers;
    std::chrono::seconds timeout{30};      // 覆盖 NetworkConfig 默认
    bool verify_ssl = true;
    bool follow_redirect = true;
    Optional<StringView> proxy;            // 覆盖配置代理
    Optional<u64> range_start;             // 断点续传
    Optional<u64> range_end;
};

export struct Response {
    i32 status_code = 0;
    Vector<HttpHeader> headers;
    Bytes body;
    String final_url;                      // 重定向后最终 URL
};

export class Network {
  public:
    // 同步（内部阻塞当前线程，适用于非关键路径）
    static auto Get(StringView url, const RequestOptions &opts) -> Result<Response>;
    static auto Post(StringView url, ConstByteSpan body, const RequestOptions &opts) -> Result<Response>;
    static auto PostJson(StringView url, StringView body, const RequestOptions &opts) -> Result<Response>;

    // 异步（协程）
    static auto GetAsync(StringView url, const RequestOptions &opts) -> Task<Response, Error>;
    static auto PostAsync(StringView url, ConstByteSpan body, const RequestOptions &opts) -> Task<Response, Error>;

    // 流式下载（交给 DownloadManager 使用）
    static auto OpenStream(StringView url, const RequestOptions &opts,
                           Callback<void(ConstByteSpan chunk)> on_data) -> Result<u64>;  // 返回 Content-Length
};
```

要点：

- 全局初始化（`curl_global_init`）由 Context 生命周期管理，单次。
- 超时/重试由 `NetworkConfig` 控制；重试策略：指数退避（1s → 2s → 4s，上限 `retry_count`）。
- 错误映射：DNS/连接失败 → `NetworkError` / `ConnectionFailed`；超时 → `Timeout`；TLS → `Security`。
- 代理支持：`http` / `https` / `socks5` 代理，来自配置 `network.proxy`。
- `verify_ssl` 默认 true；仅在用户显式配置时关闭。
- 为 C ABI 与 JS 桥提供 `http.*` 能力。

## 4.7 下载管理器 `aurora.service:download`

空壳，设计如下。详细策略见 [第 10 章](#10-下载与缓存专项设计)。

```cpp
export struct DownloadTask {
    String url;                  // 主 URL（首选）
    Vector<String> mirrors;      // 备用镜像（含 BMCLAPI/MCBBS 等镜像站规则）
    Path target_path;            // 最终落盘路径
    Optional<String> expected_sha1;
    Optional<u64> expected_size;
    Vector<HttpHeader> headers;  // 附加头（如 User-Agent: AuroraLauncher/0.1.0）
    i32 priority = 0;            // 越高越优先
    bool use_cache = true;       // 命中 Cache 则直接使用
};

export enum class DownloadState { Pending, Running, Verifying, Completed, Failed, Cancelled };

export struct DownloadProgress {
    StringView task_id;
    u64 downloaded;              // 本次会话已下载字节（不含断点续传前部分）
    u64 total;                   // 0 表示未知（chunked）
    f64 speed;                   // 字节/秒（滑动窗口估算）
};

export class DownloadManager {
  public:
    static auto Submit(DownloadTask task) -> Result<DownloadHandle>;
    static auto Cancel(DownloadHandle handle) -> Result<void>;
    static auto State(DownloadHandle handle) -> DownloadState;
    static auto SyncWait(DownloadHandle handle, Optional<std::chrono::milliseconds> timeout)
        -> Result<Path>;         // 阻塞等待完成，返回落盘路径
    // 异步形式（协程）
    static auto Await(DownloadHandle handle) -> Task<Path, Error>;
};
```

事件（见 [第 11 章](#11-事件系统设计)）：`download_progress`、`download_completed`、`download_failed`、`download_checksum_mismatch`。

要点：

- 并发受 `RuntimeConfig` 线程池约束（下载走 Scheduler 高优先级队列）。
- 断点续传：`.part` 临时文件 + `Range` 请求；完成且校验通过后原子 rename。
- sha1 校验失败：记录事件 → 按镜像列表切换下一镜像重试 → 全部失败返回 `ChecksumMismatch`。
- `use_cache`：命中 `Cache` 且校验通过则跳过下载（见 4.9）。
- `aurora.service.download` 必须被 `aurora.service.cppm` export（当前遗漏，需修复）。

## 4.8 压缩解压 `aurora.service:archive`

空壳，设计如下。基于 minizip-ng。

```cpp
export struct ExtractOptions {
    bool overwrite = true;
    bool keep_legacy_symlinks = false;   // 默认拒绝符号链接条目
};

export class Archive {
  public:
    static auto ExtractZip(Path archive, Path dest, const ExtractOptions &opts = {})
        -> Result<void>;
    static auto CreateZip(Path archive, Span<const Path> files, Path base_dir) -> Result<void>;
    static auto ListZip(Path archive) -> Result<Vector<String>>;

    static auto CompressGzip(ConstByteSpan data) -> Result<Bytes>;
    static auto DecompressGzip(ConstByteSpan data) -> Result<Bytes>;
};
```

安全规则（强制）：

- **Zip-Slip 防护**：拒绝绝对路径条目与包含 `..` 的条目；解压目标必须严格位于 `dest` 内。
- 默认拒绝符号链接/设备文件条目（`keep_legacy_symlinks = false`），防止解压逃逸。
- 条目名统一转换为 `Path` 后校验。

用途：解压 natives（`libraries/**/natives-*`）、Forge 安装器、JS 插件包、`.minecraft` 资源包等。

## 4.9 磁盘缓存 `aurora.service:cache`

空壳，设计如下。为可重复下载的资产（版本 JSON、资源索引、库、JRE 包）提供内容寻址缓存。

```cpp
export class Cache {
  public:
    // 键 = URL（规范化）或内容哈希；校验 = sha1（可选）
    static auto Get(StringView key) -> Result<Optional<Path>>;          // 命中返回缓存路径
    static auto Put(StringView key, Path file) -> Result<Path>;
    static auto Contains(StringView key) -> Result<bool>;
    static auto Remove(StringView key) -> Result<void>;
    static auto Clear() -> Result<void>;
    static auto TotalSize() -> Result<u64>;
};
```

要点：

- 存储位置：`PathConfig::cache_directory`（默认 `runtime/cache`），子目录按 key 哈希分片（`cache/<key[:2]>/<key>`）。
- 命中判定：存在 + 可选 sha1 校验通过；校验失败视为未命中并删除坏条目。
- 清理策略：本期实现 `Clear()` 与按 key 删除；LRU 容量上限为后续版本。
- 与 `DownloadManager` 协作：`DownloadTask.use_cache = true` 时先查缓存。

## 4.10 QuickJS 引擎封装 `aurora.service:quickjs`

空壳，设计如下。封装 quickjs-ng，供 **JS 插件**（第 7 章）使用。**不作为配置求值引擎**。

```cpp
export struct QuickJSLimits {
    u64 max_memory = 64 * 1024 * 1024;   // 64 MB 堆上限
    u32 max_execution_ms = 5000;          // 单次调用超时
};

export struct ScriptContext;              // 不透明句柄（QuickJSContext*）

export class QuickJS {
  public:
    static auto Create(const QuickJSLimits &limits) -> Result<ScriptContext>;
    static void Destroy(ScriptContext ctx);

    // 模块与执行
    static auto Eval(ScriptContext ctx, StringView source, Path module_path) -> Result<JSValueHandle>;
    static auto LoadModule(ScriptContext ctx, Path module_path) -> Result<JSValueHandle>;

    // 宿主函数注册（JS 桥的 C 函数挂载点）
    static auto RegisterFunction(ScriptContext ctx, StringView name,
                                 void (*fn)(...), void *userdata) -> Result<void>;

    // 调用与取值
    static auto Call(ScriptContext ctx, JSValueHandle func, Span<JSValueHandle> args) -> Result<JSValueHandle>;
    static auto ToString(ScriptContext ctx, JSValueHandle value) -> Result<String>;
};
```

要点：

- **单线程模型**：每个 JS 插件拥有独立 Runtime + Context；所有 JS 调用在专用插件线程串行执行，异步操作通过"事件循环 + Promise"模式桥接（见 7.3）。
- **沙箱**：默认不暴露 FS / 网络全局对象；能力仅通过桥接函数按插件清单权限开放。
- **资源限制**：内存上限 + 执行超时（QuickJS 中断回调实现），超时返回 `ScriptError`。
- **错误转换**：JS 异常转换为 `Error{Plugin/Script, ScriptError, 堆栈摘要}`。

## 4.11 认证 `aurora.service:auth`

空壳，设计如下。详细流程见 [第 9 章](#9-认证专项设计)。

```cpp
export enum class AuthService { Offline, Microsoft, ThirdParty };

export struct AuthSession {
    AuthService service;
    String username;
    String uuid;               // 无横线小写
    String access_token;       // 离线模式为空
    String refresh_token;      // 离线模式为空
    Optional<std::chrono::system_clock::time_point> expiry;
    Optional<String> yggdrasil_base_url;   // 仅 ThirdParty
    String display_name;       // 皮肤站/微软档案名
};

export class AuthProvider {   // 抽象接口
  public:
    virtual ~AuthProvider() = default;
    virtual auto Authenticate(AuthFlowContext &flow) -> Task<AuthSession, Error> = 0;
    virtual auto Refresh(AuthSession session) -> Result<AuthSession> = 0;
    virtual auto Invalidate(AuthSession session) -> Result<void> = 0;
};

export class AuthManager {
  public:
    static auto Accounts() -> Result<Vector<AuthSession>>;
    static auto ActiveAccount() -> Optional<AuthSession>;
    static auto SetActiveAccount(AuthSession session) -> Result<void>;
    static auto RemoveAccount(AuthSession session) -> Result<void>;
    static auto Authenticate(AuthService service, const AuthFlowOptions &opts)
        -> Task<AuthSession, Error>;
    static auto RefreshActive() -> Task<AuthSession, Error>;
    static auto Persist() -> Result<void>;   // 写 accounts.json
};
```

要点：

- 会话持久化于 `runtime/accounts.json`（刷新令牌经 `Crypto::AesGcmEncrypt` 静态加密，见 9.5）。
- 认证交互（设备码展示、用户确认）通过事件 `auth_device_code_ready` 等上报宿主，库自身不渲染 UI。
- `AuthProvider` 三个实现：`OfflineAuthProvider` / `MicrosoftAuthProvider` / `ThirdPartyAuthProvider`，在 Context 初始化时注册，`AuthManager::Authenticate` 按 `AuthService` 分发。

---

# 5. 核心层 Core Layer

模块：`aurora.core`（`src/aurora.core/*.cppm` + 实现位于 `src/aurora/` 下）

定位：Aurora 的运行内核，负责生命周期、任务调度、线程管理、服务管理。

```
aurora.core
 ├── :context   应用对象（服务注册表 + 生命周期 + 插件注册）
 ├── :task      协程任务抽象
 ├── :pool      线程池
 ├── :scheduler 任务调度器
 └── :event     类型安全事件总线
```

依赖：`aurora.core` → `aurora.service`（network/download 等能力）→ `launcher.base`。

## 5.1 Context `aurora.core:context`

Drogon App 风格的应用对象。

```cpp
export class Context {
  public:
    // ---- 生命周期 ----
    static auto Instance() -> Context &;                       // 默认单例
    static auto Initialize(const Config &config) -> Result<void>;
    static void Shutdown();

    // 隔离实例（嵌入/测试用）
    static auto Create(const Config &config) -> Result<SharedPtr<Context>>;
    static void SetActive(SharedPtr<Context> ctx);             // 指定默认实例

    auto Config() const -> const launcher::Config &;
    auto IsRunning() const -> bool;

    // ---- 服务注册表 ----
    template <typename T, typename... Args>
    auto Register(Args &&...args) -> Result<void>;             // 注册服务实例
    template <typename T>
    auto Get() -> T &;                                         // 获取服务，未注册返回错误/断言
    template <typename T>
    auto TryGet() -> Optional<T *>;

    // ---- 调度 ----
    auto Scheduler() -> aurora::Scheduler &;
    auto Pool() -> aurora::ThreadPool &;

    // ---- 事件 ----
    auto Events() -> aurora::EventDispatcher &;

    // ---- 插件 ----
    template <typename PluginT, typename... Args>
    auto RegisterPlugin(StringView name, Args &&...args) -> Result<void>;
    auto Plugins() -> aurora::PluginRegistry &;
};
```

设计决策：

- **单默认实例**：`Context::Instance()` 返回默认上下文（首次 `Initialize` 创建）；`Create` 用于测试与未来多实例。
- **服务注册**：内置服务在 `Initialize` 时按固定依赖序注册：

  ```
  Logger → Random/UUID/Crypto → IO → Network → Archive/Cache → Download → QuickJS → Auth
  ```

- **Get 语义**：`Get<T>()` 返回注册实例引用；未注册属于编程错误（Debug 断言 + Release 记录致命日志）。
- **线程亲和**：Context 本身线程安全；`Shutdown` 必须在宿主主线程调用，且只能调用一次。
- **初始化失败策略**：`Initialize` 返回错误时不做任何资源残留（内部 RAII 回滚）；调用方必须处理错误。

## 5.2 任务 `aurora.core:task`

基于 C++20 协程的异步任务抽象，`Result<T, E>` 作为协程结果载体（现状实现 + 完善）。

```cpp
export template <typename T, typename E = Error>
class Task {
  public:
    using promise_type = details::TaskPromise<T, E>;
    using Handle       = std::coroutine_handle<promise_type>;

    explicit Task(Handle h);
    Task(Task &&) noexcept;
    Task &operator=(Task &&) noexcept;

    // 完善项
    auto StartOn(Scheduler &scheduler) -> Result<void>;   // 将任务投递到调度器执行
    auto SyncWait() -> Result<T, E>;                      // 阻塞等待（禁止在池线程调用）
    auto Then(F func) -> Task<U, E>;                      // 链式延续
    auto Catch(F func) -> Task<T, E>;                     // 错误处理
    bool Done() const noexcept;
    auto Result() -> Optional<Result<T, E>>;              // 已完成时取结果
};
```

Promise 语义（与现状对齐并固化）：

| 钩子 | 语义 |
| :--- | :--- |
| `initial_suspend` | `suspend_never`：任务在创建线程上立即开始（eager） |
| `final_suspend` | `suspend_always`：协程帧保持直至句柄被销毁；由 Task 析构/`Done` 路径统一 `destroy()` |
| `return_value` / `return_void` | 构造 `Ok<T,E>` 存入 `result` |
| `unhandled_exception` | 捕获为 `Err(InternalError, "unhandle exception in coroutine")`，**禁止异常逃逸** |

规则：

- `co_await` 只能在任务协程内使用；Task **不负责线程**，继续执行点由调度器决定。
- `SyncWait` 实现必须检测"当前在池线程"并返回 `Err(InvalidState)`，避免死锁。
- 错误经 `Result` 传播，协程内禁止裸 `throw`；需要取消的场景显式传递 `CancellationToken`（本期提供参数约定，不实现自动取消）。

辅助工具（`aurora.core:task` 内提供）：

```cpp
export template <typename T, typename E = Error>
auto WhenAll(Vector<Task<T, E>> tasks) -> Task<Vector<T>, E>;   // 全部成功或首个失败

export template <typename T, typename E = Error>
auto WhenAny(Vector<Task<T, E>> tasks) -> Task<usize, E>;       // 返回完成索引

export template <typename F>
auto Async(F &&f) -> Task<std::invoke_result_t<F>, Error>;      // 将同步函数包为任务
```

## 5.3 线程池 `aurora.core:pool`

```cpp
export enum class TaskPriority { Low = 0, Normal = 1, High = 2 };

export class ThreadPool {
  public:
    explicit ThreadPool(u32 worker_count, StringView name = "aurora-pool");
    ~ThreadPool();

    // 禁用拷贝
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    auto Submit(Function<void()> task, TaskPriority priority = TaskPriority::Normal)
        -> Result<void>;
    void Stop();                        // 停止接收新任务，等待在跑任务完成
    void Shutdown();                    // 立即停止，丢弃队列（危险操作，仅退出路径）
    auto WorkerCount() const -> u32;
};
```

要点：

- 工作线程命名 `aurora-pool-N`（调试便利）。
- **业务禁止直接创建 `std::thread`**：一律经 `Context → Pool`。
- 队列：三优先级 FIFO；高优先级优先，同优先级 FIFO。第一版不实现工作窃取（work-stealing 为后续优化项）。
- 任务异常兜底：任务函数体由 `try/catch(...)` 包裹，异常转为 `Logger::Error`（不崩溃、不逃逸）。
- `Stop` 幂等且线程安全；`Shutdown` 仅用于 `Context::Shutdown` 的收尾路径。

## 5.4 调度器 `aurora.core:scheduler`

```cpp
export class Scheduler {
  public:
    explicit Scheduler(ThreadPool &pool);

    // 任务投递
    auto Schedule(Task<void, Error> task, TaskPriority priority = TaskPriority::Normal) -> Result<void>;
    auto Schedule(Function<void()> fn, TaskPriority priority = TaskPriority::Normal) -> Result<void>;
    auto DispatchAfter(std::chrono::milliseconds delay, Function<void()> fn) -> Result<void>;
    auto DispatchOn(TaskPriority priority, Function<void()> fn) -> Result<void>;  // 保留

    // 控制
    void Start();
    void Stop();
};
```

要点：

- 职责：决定"任务在哪里执行"——把 `Task` / 函数投递到 `ThreadPool`，并负责协程续体（continuation）的恢复。
- 延时任务：内部定时器线程（或池内专门线程）维护最小堆，到期投递。
- **续体恢复规则**：`co_await` 完成后，续体默认投回调度器执行（不做线程内联优化，第一版保证语义简单、无栈溢出风险）。
- 与 `Network` / `Download` 集成：libcurl multi 的完成回调以"投递任务到调度器"的形式对外通知，不占用 IO 线程。

## 5.5 事件总线 `aurora.core:event`

```cpp
export template <typename E>
using EventHandler = Function<void(const E &)>;

export class EventSubscription {
  public:
    void Unsubscribe();           // 取消订阅
    bool IsActive() const;
    // 拷贝禁用，移动允许
};

export enum class PublishMode { Sync, Async };

export class EventDispatcher {
  public:
    template <typename E>
    auto Subscribe(EventHandler<E> handler) -> EventSubscription;

    template <typename E>
    void Publish(const E &event, PublishMode mode = PublishMode::Sync) const;

    template <typename E>
    void PublishAsync(const E &event) const;   // 等价 Publish(mode=Async)，经 Scheduler 投递

    usize SubscriberCount() const;             // 诊断用
};
```

要点：

- 类型安全：事件以**类型**区分，`Publish` 只通知订阅该类型的处理器；错误类型的事件（订阅回调抛异常）由总线捕获并记日志。
- `Sync`：在发布线程同步调用处理器（默认，保证顺序与低延迟）；`Async`：投递到 Scheduler。
- 订阅者生命周期：`EventSubscription` 析构自动退订；事件处理器内部禁止持有悬垂引用（订阅方负责捕获引用生命周期）。
- 事件列表与结构定义见 [第 11 章](#11-事件系统设计)。

---

# 6. 运行时层 Runtime Layer

模块：`aurora.runtime`（`src/aurora.runtime/*.cppm`）

定位：**Minecraft 业务实现层**，负责版本解析、资源准备、JVM、进程、启动流水线、加载器、运行时插件。全部空壳，本章为完整目标设计。

依赖：`aurora.runtime` → `aurora.core` / `aurora.service` / `launcher.base`。

## 6.1 版本 `aurora.runtime:version`

### 职责

- 拉取与缓存 Mojang 版本清单（`version_manifest_v2.json`）。
- 解析单个版本 `version.json` 为 `VersionProfile`。
- 规则求值（Rules Engine）与参数合并，产出 `LaunchRecipe`。

### 数据模型

```cpp
export struct VersionManifestEntry {
    String id;            // "1.21.4" / "24w14a"
    String type;          // release / snapshot / old_beta / old_alpha
    String url;           // version.json 地址
    String sha1;
    u64 size;
};

export struct Library {
    String name;               // Maven 坐标 group:artifact:version
    Optional<String> url;      // 缺省用默认库源
    String path;               // 相对库目录路径（Maven 布局）
    Optional<String> sha1;
    Optional<u64> size;
    Vector<Rule> rules;        // 平台/特性规则（可为空 = 全平台）
    Optional<String> natives_classifier;  // 如 "natives-windows"
    Vector<String> classifiers;           // 额外分类器（如 sources）
    bool is_native = false;               // natives 库（需解压）
    bool has_checksum = false;
};

export struct AssetIndexRef {
    String id;                 // 索引 id（如 "1.21"、"legacy"、"pre-1.6"）
    String url;
    String sha1;
    u64 size;
    u64 total_size;            // 索引内对象总大小
};

export struct VersionProfile {
    String id;
    String type;
    String main_class;
    String assets;                       // 资产索引 id
    Optional<AssetIndexRef> asset_index;
    Vector<Library> libraries;
    Vector<String> game_args;            // 1.13+ 合并后的 game arguments
    Vector<String> jvm_args;             // 1.13+ 合并后的 jvm arguments
    Optional<String> legacy_minecraft_arguments;  // <1.13
    Optional<String> logging_client_url;          // log4j 配置文件
    Optional<String> logging_client_sha1;
    Optional<JavaVersionReq> java_version;        // 1.20.5+ 的 javaVersion 字段
    Version version;                   // 元版本（可选）
};

export struct JavaVersionReq {
    u32 major;          // 如 21
    Optional<String> component;   // 如 "java-runtime-gamma"
};

export struct LaunchRecipe {
    String version_id;
    String main_class;
    Vector<Path> classpath;        // 库 + client jar
    Path natives_directory;        // 已解压 natives
    Vector<String> game_args;
    Vector<String> jvm_args;
    String asset_index_id;
    Optional<Path> logging_config;
    Optional<JavaVersionReq> java_version;
    String game_directory;
    String assets_directory;
};
```

### Rules Engine

`Rule { action: allow|disallow, os?: { name, arch, version }, features?: {...} }`

- 求值输入：`CurrentPlatform()` / `CurrentArchitecture()` / 特性标志（如 `is_demo_user`、`has_custom_resolution`）。
- 命中即生效；默认（无规则）为 allow。
- 结果缓存：同一 `VersionProfile` 在生命周期内只求值一次（按平台缓存）。

### 版本兼容矩阵（"全部协议"）

| 版本段 | 参数格式 | 资源格式 | Java | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| old_alpha / old_beta | `minecraftArguments` 或空 | `legacy` / `pre-1.6` | 任意 JRE | 远古版本，资源索引可能缺失 |
| 1.0 – 1.6.x | `minecraftArguments` | `legacy` / `pre-1.6` | JRE 6/7/8 | 无 assetIndex 时用 `legacy` |
| 1.7.2 – 1.12.x | `minecraftArguments` | 索引化（1.7.10+ 有索引） | JRE 8 | 首次引入 asset index |
| 1.13 – 1.16.x | `arguments.game/jvm` | 索引化 | JRE 8 | 参数模型切换 |
| 1.17 – 1.20.4 | `arguments.game/jvm` | 索引化 | JRE 16/17 | 依赖 Java 17 |
| 1.20.5+ | `arguments.game/jvm` | 索引化 | `javaVersion.major` 指定 | 携带组件名 |

> 设计约束：解析器必须"缺省容忍"——未知字段不导致解析失败；对缺失资源/索引走 legacy 回退路径。

### 版本解析流程

```
VersionManifest.Refresh()          拉取 manifest（失败用本地缓存）
        ↓
VersionResolver.Resolve(profile)
  ├── 下载/命中缓存 version.json
  ├── 解析为 VersionProfile
  ├── 应用 Loader 修补（若有，见 6.6）
  ├── Rules 求值 + 参数合并（game/jvm 或 legacy）
  ├── 组装 classpath（库路径 + client jar；natives 标记解压）
  └── 产出 LaunchRecipe → 事件 launch_progress(phase=resolved)
```

## 6.2 资源 `aurora.runtime:assets`

### 职责

- 下载与校验 `asset_index.json`。
- 下载缺失的 asset object（`assets/objects/<hash[:2]>/<hash>`）。
- legacy 版本生成 `virtual/legacy` 虚拟布局。

```cpp
export struct AssetObject {
    String hash;
    u64 size;
};

export class AssetManager {
  public:
    static auto EnsureIndex(StringView index_id) -> Result<Path>;       // 下载/校验索引
    static auto EnsureAssets(VersionProfile &profile,
                             Callback<DownloadProgress> progress) -> Result<void>;
    static auto MissingObjects(VersionProfile &profile) -> Result<Vector<AssetObject>>;
    static auto BuildLegacyVirtual(VersionProfile &profile) -> Result<void>;
    static auto VerifyAll(VersionProfile &profile) -> Result<void>;
};
```

要点：

- object 存储：`assets/objects/<hash[:2]>/<hash>`；索引缓存于 `cache/asset_index/<id>.json`。
- `MissingObjects` 通过 `Cache` + 磁盘扫描计算缺失集，只下载缺失项。
- legacy：`assets/virtual/legacy/` 下建立 `<namespace>/<path>` 到 object 的链接（Windows 用拷贝），供 `--assetIndex legacy` 使用。
- 全部下载任务经 `DownloadManager`，事件 `download_progress` 透出。

## 6.3 JVM `aurora.runtime:jvm`

### 职责

- JRE 定位（探测候选顺序）。
- Java 版本/架构探测。
- JVM 启动参数生成。

```cpp
export struct JvmInfo {
    Path java_executable;      // java 可执行文件路径
    u32 major_version;         // 如 21
    String vendor;             // "OpenJDK"/"Microsoft"/"Oracle"...
    String runtime_name;       // -XshowSettings 输出
    Architecture architecture;
};

export class JvmManager {
  public:
    static auto Locate(const LaunchProfile &profile) -> Result<JvmInfo>;
    static auto Probe(Path java_executable) -> Result<JvmInfo>;
    static auto BuildJvmArgs(const LaunchRecipe &recipe, const LaunchProfile &profile,
                             const JvmInfo &jvm) -> Result<Vector<String>>;
};
```

### JRE 候选顺序

1. `LaunchProfile.java_path`（用户显式指定）
2. `runtime/jre`（Aurora 管理的 JRE，目录内 `java` 可执行文件）
3. `JAVA_HOME` 环境变量
4. `PATH` 中的 `java`
5. 平台已知目录：Windows `%ProgramFiles%\Java`、macOS `/Library/Java/JavaVirtualMachines`（后续版本）

### 版本探测

- 执行 `java -XshowSettings:properties -version 2>&1`，解析 `java.specification.version` / `java.vendor` / `os.arch`。
- 版本不满足 `JavaVersionReq` 时返回 `Err(JvmIncompatible)`，错误信息包含"需要 X，找到 Y"。
- 探测结果按 `(path, mtime)` 缓存，生命周期内不重复探测。

### JVM 参数生成规则

- 基础：`-Xms` / `-Xmx`（来自 LaunchProfile，默认 512M / 2G）、`-XX:+UnlockExperimentalVMOptions`（按版本）。
- natives：`-Djava.library.path=<natives_directory>`。
- classpath：`-cp <classpath 拼接>`（Windows 分号、POSIX 冒号）。
- log4j：`-Dlog4j.configurationFile=<logging_config>`（存在时）。
- 版本差异表（内置默认，可被 profile 覆盖）：
  - Java 8：追加 `-XX:PermSize=...`（旧 Forge）、`-Dfml.ignoreInvalidMinecraftCertificates=true` 等由 loader 注入。
  - Java 17+：`--add-opens` 系列（Fabric/Forge 所需）由 loader 注入。
- 用户自定义参数（`LaunchProfile.jvm_args_override`）追加在末尾，可覆盖默认（同一参数后者生效）。

## 6.4 进程 `aurora.runtime:process`

### 职责

跨平台子进程管理：创建、管道、环境、退出监控。

```cpp
export struct ProcessOptions {
    Path cwd;
    Vector<String> args;                 // 不含 argv[0]
    String env_overrides;                // 追加/覆盖环境变量（KEY=VALUE;...）
    bool capture_stdout = true;
    bool capture_stderr = true;
    bool merge_stderr_to_stdout = false;
    bool inherit_gui = false;            // 游戏需要 GUI 句柄（Windows）
};

export class Process {
  public:
    static auto Spawn(Path executable, const ProcessOptions &opts) -> Result<Process>;

    ~Process();                          // 未退出则 Kill

    auto Pid() const -> u64;
    auto WriteStdin(StringView data) -> Result<void>;
    auto Wait() -> Result<i32>;          // 阻塞等待，返回退出码
    auto Kill() -> Result<void>;
    auto StopGracefully(std::chrono::milliseconds timeout) -> Result<i32>;  // SIGTERM→Kill
    auto IsRunning() const -> bool;
    auto Stdout() -> Result<String>;     // 已捕获输出（行缓冲）
    auto Stderr() -> Result<String>;
};
```

要点：

- 后端：Windows `CreateProcessW`（参数引用规则：双引号转义、`\\` 处理）；POSIX `posix_spawn` + 管道；Android `vfork` 路径后续版本。
- 输出捕获：行缓冲，UTF-8 解码（Windows 上处理控制台代码页 → UTF-8），经事件 `game_output` 推送，同时保留环形缓冲供查询。
- 环境：继承父进程环境 + `env_overrides` 覆盖。
- 退出监控：`Wait` / 事件 `game_exit`；进程句柄 RAII 管理，防止僵尸进程。
- 超时管理：`StopGracefully` 先温和终止（POSIX SIGTERM / Windows CloseMainWindow 能力外则直接 Terminate），超时后 `Kill`。

## 6.5 启动 `aurora.runtime:launch`

### 职责

启动流水线编排器：把 Version + Assets + JVM + Auth + Process 串成完整生命周期，全程事件驱动。

```cpp
export enum class LaunchPhase {
    Idle, ResolvingVersion, PreparingAssets, PreparingJvm,
    GeneratingArguments, Spawning, Running, Exited, Failed
};

export struct LaunchProfile {
    String name;                 // 启动配置名
    String version_id;           // MC 版本 id
    Optional<LoaderKind> loader; // 加载器（见 6.6）
    Optional<String> loader_version;
    Path game_directory;         // 默认 runtime/minecraft
    Path java_path;              // 可选，覆盖 JVM 定位
    u64 min_memory_mb = 512;
    u64 max_memory_mb = 2048;
    bool fullscreen = false;
    u32 window_width = 854;
    u32 window_height = 480;
    Optional<String> server_address;   // 直接进服
    Vector<String> jvm_args_override;
    Vector<String> game_args_override;
    String launcher_name = "Aurora";
};

export class LaunchController {
  public:
    static auto Create(LaunchProfile profile) -> Result<SharedPtr<LaunchController>>;
    static auto Launch(SharedPtr<LaunchController> ctl) -> Result<void>;  // 异步，事件驱动

    auto Phase() const -> LaunchPhase;
    auto Recipe() const -> Optional<LaunchRecipe>;
    auto Process() const -> Optional<Process>;
    auto Cancel() -> Result<void>;
    auto ExitCode() const -> Optional<i32>;
};
```

### 启动流水线状态机

```
Idle
 ├─> ResolvingVersion   （版本解析 + Loader 修补 + 规则求值）
 ├─> PreparingAssets    （asset index + objects + natives 解压）
 ├─> PreparingJvm       （JRE 定位 + 版本探测）
 ├─> GeneratingArguments（JVM/game 参数 + 认证注入）
 ├─> Spawning           （Process::Spawn）
 ├─> Running            （输出转发 + 退出监控）
 ├─> Exited             （正常退出 / 崩溃，带退出码）
 └─> Failed             （任一阶段错误，携带 Error 链）
```

### 认证注入（GeneratingArguments 阶段）

- 游戏参数：`--username` / `--uuid` / `--accessToken` / `--version` / `--gameDir` / `--assetsDir` / `--assetIndex` / `--userType`（`msa` / `mojang` / `legacy`）/ `--versionType`（release/snapshot）。
- JVM 参数：微软登录注入 `-Dminecraft.api.auth.host=...`、`-Dminecraft.api.session.host=...`、`-Dminecraft.api.services.host=...`；离线不注入。
- 第三方皮肤站：注入 `-javaagent:<authlib-injector.jar>=<base_url>`，并设置上述 `-Dminecraft.api.*` 到该站对应地址（base_url 派生规则见 9.4）。
- 启动器品牌：`--versionType` 由 profile.launcher_name 派生，`user-agent` 统一 `AuroraLauncher/<version>`。

### 事件（启动相关）

`launch_progress(phase, percent, message)`、`game_output(line)`、`game_exit(code)`、`launch_failed(error)`（见第 11 章）。

## 6.6 加载器 `aurora.runtime:loader`

### 职责

把 Vanilla `VersionProfile` 修补为带加载器的版本（类路径、主类、参数、额外库）。

```cpp
export enum class LoaderKind { Vanilla, Fabric, Quilt, Forge, NeoForge };

export class LoaderPatcher {
  public:
    virtual ~LoaderPatcher() = default;
    virtual auto Kind() const -> LoaderKind = 0;
    virtual auto Patch(VersionProfile &profile, const LaunchProfile &lp)
        -> Result<void> = 0;                 // 原地修补
};
```

### 各加载器修补策略

| 加载器 | 元数据源 | 主类 | 说明 |
| :--- | :--- | :--- | :--- |
| Fabric | `fabric-meta` API（loader + intermediary） | `net.fabricmc.loader.impl.launch.knot.KnotClient` | 追加 fabric loader + intermediary 库；合并参数 |
| Quilt | quilt-meta API | `org.quiltmc.loader.impl.launch.knot.KnotClient` | 同上 |
| Forge (1.12.2-) | installer / universal jar + launchwrapper | `net.minecraft.launchwrapper.Launch` | legacy：追加 launchwrapper + forge universal；参数 `--tweakClass net.minecraftforge.fml.common.launcher.FMLTweaker` |
| Forge (1.13+) | 安装器处理产物（`--installClient` 或解包元数据） | 安装器生成 | 需先执行安装器（子进程 java -jar）或使用缓存产物；追加 forge 库与 `--tweakClass`/`-Dforge.logging...` |
| NeoForge | 同现代 Forge | 安装器生成 | 1.20.1+ 分叉的 Forge |

要点：

- **安装器缓存**：Forge/NeoForge 安装器运行结果缓存于 `cache/loader/<loader>/<mc_version>/<loader_version>/`，避免重复执行。
- **版本兼容**：loader 与 MC 版本不匹配返回 `Err(Unsupported)`，并给出可用版本提示（来自元数据）。
- **规则独立**：每个 LoaderPatcher 独立模块化，可插拔注册到 `LoaderRegistry`。
- 修补顺序：Vanilla 解析 → Loader 修补 → Rules 求值 → Recipe 组装（与 6.1 流程一致）。

## 6.7 运行时插件契约 `aurora.runtime:plugin`

运行时层提供"启动生命周期钩子"接口，供 C++ 插件与 JS 插件实现（完整插件系统见第 7 章）。

```cpp
export class ILaunchHook {
  public:
    virtual ~ILaunchHook() = default;

    virtual auto OnResolvingVersion(LaunchProfile &profile) -> Result<void> { return Ok(); }
    virtual auto OnVersionResolved(LaunchRecipe &recipe) -> Result<void> { return Ok(); }
    virtual auto OnPreparingAssets(LaunchRecipe &recipe) -> Result<void> { return Ok(); }
    virtual auto OnGeneratingArguments(LaunchRecipe &recipe, Vector<String> &game_args,
                                       Vector<String> &jvm_args) -> Result<void> { return Ok(); }
    virtual auto OnPreLaunch(LaunchRecipe &recipe, ProcessOptions &opts) -> Result<void> { return Ok(); }
    virtual auto OnGameExit(i32 exit_code) -> void {}
};

export class PluginRegistry {
  public:
    auto RegisterHook(SharedPtr<ILaunchHook> hook) -> Result<void>;
    auto UnregisterHook(SharedPtr<ILaunchHook> hook) -> void;
    // 插件生命周期
    auto LoadPlugins() -> Result<void>;    // 加载 C++ 注册插件 + 扫描 JS 插件目录
    auto UnloadPlugins() -> Result<void>;
};
```

要点：

- 钩子返回 `Result<void>`：错误将**中止当前启动阶段**并以 `launch_failed` 事件上报（插件错误不崩溃内核）。
- 钩子执行顺序 = 注册顺序；JS 插件钩子经 QuickJS 桥转调（见 7.4）。
- 插件注册表由 `Context` 持有，`context.Plugins()` 访问。

---

# 7. 插件系统

## 7.1 插件模型总览

Aurora 提供两类插件：

| 类型 | 集成方式 | 性能 | 能力 | 目录 |
| :--- | :--- | :--- | :--- | :--- |
| C++ 编译期插件 | 宿主编译期 `RegisterPlugin` | 高 | 全部 C++ API | `src/plugins/<name>` / `include/plugins/<name>` |
| JS 运行时插件 | 运行时扫描加载（QuickJS） | 中 | 桥接 API（附录 B） | `<runtime>/plugins/<name>/` |

共同点：

- 插件清单 `manifest.json`（名称、版本、作者、权限、钩子声明）。
- 生命周期：`OnLoad(Context&)` → 注册钩子/事件 → `OnUnload()`。
- 权限模型：C++ 插件默认全权限；JS 插件**必须显式声明权限**，未声明的能力调用返回 `PluginError/PermissionDenied`。
- 插件错误不崩溃内核：钩子返回 `Result`，JS 异常转 `ScriptError`。

## 7.2 C++ 编译期插件

```cpp
// include/plugins/my_plugin/my_plugin.h
export namespace plugins {

class MyPlugin : public launcher::IPlugin {
  public:
    // IPlugin
    auto OnLoad(launcher::Context &ctx) -> launcher::Result<void> override;
    auto OnUnload() -> void override;

    // 可选：实现 ILaunchHook 获得启动钩子
    auto OnVersionResolved(launcher::LaunchRecipe &recipe)
        -> launcher::Result<void> override;

    StringView Name() const override { return "my_plugin"; }
};

}  // namespace plugins
```

```cpp
// 宿主注册（Drogon 风格）
app().RegisterPlugin<plugins::MyPlugin>("my_plugin");
```

接口定义（`aurora.runtime:plugin` / `aurora.core` 协作）：

```cpp
export class IPlugin {
  public:
    virtual ~IPlugin() = default;
    virtual auto OnLoad(Context &ctx) -> Result<void> = 0;
    virtual auto OnUnload() -> void = 0;
    virtual StringView Name() const = 0;
};
```

要点：

- C++ 插件与内核**同进程、同 ABI**，可自由使用 `launcher` 命名空间全部 API（仅限 `aurora.*` 与 `launcher.base`）。
- 二进制插件（DLL/SO 动态加载）为后续版本，本期不设计。

## 7.3 JS 插件

### 目录约定

```
<runtime>/plugins/<plugin_name>/
 ├── manifest.json      # 必填
 ├── main.js            # 入口（manifest.entry）
 └── ...                # 其他资源（只读）
```

`manifest.json` 示例：

```json
{
  "name": "greeter",
  "version": "1.0.0",
  "author": "oldmnj",
  "entry": "main.js",
  "permissions": ["log", "event", "http.get", "fs.read", "launch.hook"],
  "hooks": ["launch.resolved"]
}
```

### 运行时模型

- 每个插件独立 QuickJS Runtime + Context（`QuickJSLimits`：默认 64MB 堆、5s 超时）。
- 插件运行于**专用 JS 线程**（Scheduler 普通优先级）；对内核的调用经桥接层同步执行（短操作）或返回 Promise（异步操作）。
- 异步桥：JS 侧 `aurora.http.get(...)` 返回 Promise → 桥接层发起内核异步请求 → 完成后把结果**投回 JS 线程**恢复 Promise（消息队列 + 事件循环轮询）。

### 钩子

- 清单声明 `hooks`，插件内导出同名函数：

```js
export function launchResolved(recipe) {
    recipe.gameArgs.push("--demo");
    return recipe;   // 返回修改后的 recipe 或 null（不修改）
}
```

- 内核在 `LaunchController` 各阶段调用钩子；返回对象会合并回内核结构（按白名单字段）。

## 7.4 JS 桥接 API

桥接函数是一组 **C 风格函数**（`extern "C"`，与 C ABI 同源实现，函数名 `aurora_*`），注册进 QuickJS 成为 JS 全局对象 `aurora`。完整参考见 [附录 B](#附录-b-js-桥接-api-参考)。

设计原则：

1. **权限门控**：每个桥接函数在进入时检查插件清单权限，无权限返回 `Promise.reject(PermissionDenied)`。
2. **与 C ABI 共享实现**：桥接层 = C ABI 实现 + 回调适配层，避免两套逻辑。
3. **值转换**：JS 对象 ↔ C 结构按约定映射（数字 ↔ `u64/i64` 需经字符串或 BigInt，避免精度丢失）。

---

# 8. C ABI

## 8.1 目标与约束

- 为 Rust / Python / C# / Java 等语言提供稳定绑定接口。
- **C11 兼容、无 C++ 类型泄漏**：头文件只含 `extern "C"`（C++ 编译时包裹）与纯 C 类型。
- **ABI 稳定性**：语义化版本；小版本只追加函数与枚举值，不修改既有签名/枚举值。
- 错误不暴露 `launcher::Error` 对象本身，通过错误码 + 错误消息字符串透出。

## 8.2 头文件与命名

```
include/
 ├── export.h          # 现有占位，改为统一导出宏入口
 ├── aurora.h          # 总入口（include 全部）
 ├── au_export.h       # AU_API / AU_CALL 可见性与调用约定宏
 ├── au_types.h        # 基础类型（au_u32、au_string、句柄 typedef）
 ├── au_error.h        # au_error_code / au_error_category / au_error
 ├── au_version.h      # au_version() / AU_VERSION_MAJOR|MINOR|PATCH
 ├── au_context.h      # 上下文生命周期
 ├── au_config.h       # 配置读写
 ├── au_logger.h       # 日志
 ├── au_auth.h         # 认证
 ├── au_download.h     # 下载
 ├── au_launch.h       # 启动
 ├── au_event.h        # 事件订阅
 └── au_plugin.h       # 插件（预留）
```

命名约定：

- 函数前缀 `au_`；类型前缀 `au_`；回调类型 `au_*_callback`；句柄 `au_*_handle`。
- 平台宏：`AU_API`（`__declspec(dllexport/dllimport)` / `__attribute__((visibility("default")))`）、`AU_CALL`（默认 cdecl，Windows 上无需 stdcall）。

## 8.3 类型与错误约定

```c
typedef uint32_t au_u32;
typedef uint64_t au_u64;
typedef int32_t  au_i32;
typedef const char *au_string;   /* UTF-8，所有权见 8.5 */

typedef enum au_error_code {
    AU_OK = 0,
    AU_ERR_INVALID_ARGUMENT = 1,
    AU_ERR_INVALID_STATE = 2,
    /* ... 与 C++ ErrorCode 一一对应，见附录 A */
} au_error_code;

typedef enum au_error_category {
    AU_CAT_NONE = 0, AU_CAT_SYSTEM, AU_CAT_PARSE, AU_CAT_IO,
    AU_CAT_NETWORK, AU_CAT_SECURITY, AU_CAT_CONFIG, AU_CAT_RUNTIME,
    AU_CAT_MINECRAFT, AU_CAT_AUTH, AU_CAT_PLUGIN, AU_CAT_SCRIPT,
} au_error_category;

typedef struct au_error {
    au_error_code code;
    au_error_category category;
    au_string message;      /* 错误消息；由库持有，有效期内可读 */
} au_error;
```

**返回约定**：所有函数返回 `au_error`（`AU_OK` 且 `code == AU_OK` 表示成功）；成功数据经**出参**返回。

## 8.4 API 分组（设计示例）

```c
/* 上下文 */
au_error au_context_create(const au_config *cfg, au_context_handle *out);
au_error au_context_start(au_context_handle ctx);
au_error au_context_stop(au_context_handle ctx);
au_error au_context_destroy(au_context_handle ctx);

/* 版本信息 */
au_string au_version(void);            /* 静态字符串 "0.1.0" */

/* 配置 */
au_error au_config_load(au_context_handle ctx, au_string path);
au_error au_config_get(au_context_handle ctx, au_config *out);       /* 深拷贝，au_config_free 释放 */

/* 日志 */
au_error au_logger_set_level(au_context_handle ctx, au_log_level level);

/* 事件订阅 */
typedef void (*au_event_callback)(au_event_id id, const void *payload, au_u64 payload_size, void *user);
au_error au_event_subscribe(au_context_handle ctx, au_event_id id, au_event_callback cb, void *user, au_subscription_handle *out);
au_error au_event_unsubscribe(au_subscription_handle sub);

/* 下载 */
au_error au_download_submit(au_context_handle ctx, const au_download_task *task, au_download_handle *out);
au_error au_download_cancel(au_download_handle handle);
au_error au_download_state(au_download_handle handle, au_download_state *out);
au_error au_download_release(au_download_handle handle);   /* 释放句柄（非取消） */

/* 认证 */
au_error au_auth_authenticate(au_context_handle ctx, au_auth_service service,
                              const au_auth_options *opts, au_auth_handle *out);
au_error au_auth_session(au_auth_handle handle, au_auth_session *out);   /* 深拷贝 */
au_error au_auth_release(au_auth_handle handle);

/* 启动 */
au_error au_launch_create(au_context_handle ctx, const au_launch_profile *profile, au_launch_handle *out);
au_error au_launch_start(au_launch_handle handle);
au_error au_launch_cancel(au_launch_handle handle);
au_error au_launch_phase(au_launch_handle handle, au_launch_phase *out);
au_error au_launch_wait(au_launch_handle handle, au_u64 timeout_ms, au_launch_state *out);
au_error au_launch_release(au_launch_handle handle);
```

## 8.5 线程与内存规则

| 规则 | 说明 |
| :--- | :--- |
| 线程安全 | 除明确标注外，全部函数线程安全 |
| 回调线程 | 事件/进度回调投递到 Scheduler 工作线程；回调内禁止调用阻塞 API |
| 字符串所有权 | `au_string` 由库持有，有效期至"下一次同句柄调用或上下文销毁"；需要长期持有必须复制 |
| 结构体所有权 | 出参结构体若含指针（如 `au_config`），用配套 `au_*_free` 释放 |
| 句柄所有权 | 句柄由 `au_*_release` 释放；释放后继续使用为未定义行为 |
| 回调 user 指针 | 由调用方管理生命周期，库不释放 |

## 8.6 绑定示例

### Rust（示意）

```rust
#[repr(C)]
pub struct au_error { pub code: au_error_code, pub category: au_error_category, pub message: *const c_char }

extern "C" {
    fn au_version() -> *const c_char;
    fn au_context_create(cfg: *const au_config, out: *mut *mut au_context) -> au_error;
}
```

### Python（ctypes 示意）

```python
lib = ctypes.CDLL("libAuroraCore.so")
lib.au_version.restype = ctypes.c_char_p
lib.au_context_create.argtypes = [ctypes.POINTER(au_config), ctypes.POINTER(ctypes.c_void_p)]
```

### C#（DllImport 示意）

```csharp
[DllImport("AuroraCore.dll")]
internal static extern au_error au_launch_start(IntPtr handle);
```

> 各语言正式绑定（Rust crate / Python wheel / C# NuGet）为后续交付物，不在本设计文档范围内；文档只约束 C ABI 本身。

---

# 9. 认证专项设计

## 9.1 认证模型

- `AuthSession`（见 4.11）是一次认证的完整结果，由 `AuthManager` 管理（多账户、激活账户、持久化）。
- `AuthProvider` 抽象三个实现：`OfflineAuthProvider` / `MicrosoftAuthProvider` / `ThirdPartyAuthProvider`。
- 认证交互事件：`auth_device_code_ready`（设备码流）、`auth_failed`、`auth_expired`、`auth_refreshed`。

## 9.2 微软 OAuth（设备码流）

库环境无浏览器，采用 OAuth 2.0 Device Code Flow。状态机：

```
1. POST https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode
     body: client_id=<AURORA_CLIENT_ID>&scope=XboxLive.signin offline_access
     → device_code / user_code / verification_uri / expires_in / interval
        ↓ 事件 auth_device_code_ready（宿主展示 user_code 与 verification_uri）
2. 轮询 POST https://login.microsoftonline.com/consumers/oauth2/v2.0/token
     grant_type=urn:ietf:params:oauth:grant-type:device_code
     → access_token / refresh_token / 错误（authorization_pending / slow_down）
3. XSTS 交换：
     POST https://xsts.auth.xboxlive.com/xsts/authorize
     RelyingParty=rp://api.minecraftservices.com/
     → XSTS token
4. Minecraft 登录：
     POST https://api.minecraftservices.com/authentication/login_with_xbox
     → access_token（MC 令牌）
5. 档案：
     GET https://api.minecraftservices.com/minecraft/profile
     → uuid / name / skins（→ AuthSession）
```

- 轮询间隔遵循服务端 `interval`，`slow_down` 时加倍等待；超时/过期 → `AuthFailed`。
- 刷新：`grant_type=refresh_token`（有效期约 90 天），刷新失败 → `AuthExpired` 事件，宿主需重新登录。
- `client_id` 为 Aurora 固定应用 ID（常量，文档占位 `<AURORA_CLIENT_ID>`，落地时替换为注册值）。
- 会话续期由 `AuthManager` 在启动前自动执行（`RefreshActive`）。

## 9.3 离线模式

- 用户名：用户输入（任意非空字符串）。
- UUID：`MD5("OfflinePlayer:" + username)` 生成 128 位，按 UUID v3 规范设置版本位（`data[6] & 0x0F | 0x30`、`data[8] & 0x3F | 0x80`），小写无横线输出（与 Minecraft 离线逻辑一致）。
- `access_token` / `refresh_token` 为空，`expiry` 为空。
- 无网络依赖，永不失效；不触发任何服务端调用。

## 9.4 第三方皮肤站（authlib-injector 兼容）

- 用户在配置中维护皮肤站列表：`{ name, yggdrasil_base_url }`（如 LittleSkin `https://littleskin.cn/api/yggdrasil`）。
- 认证流程（Yggdrasil API）：
  1. `POST {base}/authserver/authenticate`（username/password）→ `accessToken` / `selectedProfile`；
  2. 刷新：`POST {base}/authserver/refresh`；
  3. 校验/登出：`POST {base}/authserver/validate|invalidate`；
  4. 皮肤：`GET {base}/sessionserver/session/minecraft/profile/<uuid>`（仅展示用，启动不需要）。
- 启动注入（LaunchController）：
  - `-javaagent:<authlib-injector.jar>=<base_url>`（injector jar 由 Aurora 首次使用时下载并缓存到 `runtime/`）；
  - `-Dminecraft.api.auth.host={base}/authserver`、`-Dminecraft.api.session.host={base}/sessionserver`、`-Dminecraft.api.services.host={base}/minecraftservices`（按 base_url 派生）。
- 未知/不可达皮肤站：`AuthFailed` + 明确错误消息；支持多站切换与"默认站"配置。

## 9.5 多账户与会话持久化

- 存储：`runtime/accounts.json`，结构：

```json
{
  "active": "<uuid>",
  "accounts": [
    {
      "service": "microsoft",
      "username": "...",
      "uuid": "...",
      "access_token": "<encrypted>",
      "refresh_token": "<encrypted>",
      "expiry": "2026-11-01T00:00:00Z",
      "yggdrasil_base_url": null
    }
  ]
}
```

- 令牌加密：`Crypto::AesGcmEncrypt`（密钥来自 `Crypto` 派生/机器绑定，本期使用编译期固定密钥 + 可配置外部密钥文件；OS 密钥链集成列为后续版本）。
- 文件权限：POSIX `0600`；Windows 依赖用户目录 ACL。
- 写入：原子写（`IO::WriteFileAtomic`）；损坏时回退为空账户列表并记日志，不阻塞启动。

---

# 10. 下载与缓存专项设计

## 10.1 下载任务模型

```
DownloadTask
 ├── url / mirrors[] / headers
 ├── target_path / expected_sha1 / expected_size
 ├── priority / use_cache
 └── 生命周期：Pending → Running → Verifying → Completed | Failed | Cancelled
```

## 10.2 并发与优先级

- 并发数：`RuntimeConfig.worker_threads` 的上限约束，下载任务走 Scheduler **高优先级**队列。
- 优先级：版本/索引/核心库 > 资源 object > 可选内容（皮肤、声音），用 `priority` 字段表达；同优先级 FIFO。
- 下载组：启动流水线按组提交（组内依赖：索引先于 object），组间并发。

## 10.3 断点续传与校验

1. 目标不存在或已存在且校验通过 → 直接完成（跳过）。
2. 否则写入 `<target>.part`，断点续传（`Range: bytes=<done>-`）。
3. 完成后 sha1 校验（`expected_sha1` 存在时）；通过 → 原子 rename；失败 → 删除 `.part`，切换镜像重试。
4. 全部镜像失败 → `DownloadFailed`；校验失败无镜像可换 → `ChecksumMismatch`。
5. 取消：删除 `.part` 或保留（可配置 `keep_part_on_cancel`，默认删除）。

## 10.4 镜像与源

- 默认源：Mojang 官方（版本/资源/库）。
- 镜像配置（`config.json` → `download.mirrors`）：URL 前缀映射表：

```json
{ "https://piston-meta.mojang.com": "https://bmclapi2.bangbang93.com" }
```

- 镜像选择：主 URL 失败后按列表顺序尝试；镜像规则支持前缀替换与整站替换两种模式。
- 所有下载请求携带 `User-Agent: AuroraLauncher/<version>`。

## 10.5 缓存策略

- 可缓存对象：version manifest、version.json、asset index、库、natives 压缩包、JRE 包、authlib-injector jar。
- 缓存键：URL 规范化字符串（去 query 参数中可忽略项）或内容 sha1。
- 命中：存在 + sha1 校验通过（或大小一致且无 sha1 要求）。
- 容量：本期不设硬上限（`Cache::Clear()` 手动清理）；LRU 淘汰为后续版本。

---

# 11. 事件系统设计

事件定义位于 `aurora.core:event`（结构体）与各服务模块（载荷类型）。发布方式默认 `Sync`。

| 事件 | 载荷要点 | 发布者 | 用途 |
| :--- | :--- | :--- | :--- |
| `config_loaded` | `Config` 引用（只读） | ConfigManager | 宿主感知配置生效 |
| `context_initialized` | `-` | Context | 生命周期 |
| `context_shutting_down` | `-` | Context | 插件/宿主清理 |
| `download_progress` | `task_id, downloaded, total, speed` | DownloadManager | 进度条 |
| `download_completed` | `task_id, path` | DownloadManager | 完成通知 |
| `download_failed` | `task_id, error` | DownloadManager | 失败通知 |
| `download_checksum_mismatch` | `task_id, expected, actual` | DownloadManager | 镜像切换提示 |
| `launch_progress` | `phase, percent, message` | LaunchController | 启动阶段进度 |
| `launch_failed` | `error` | LaunchController | 启动失败 |
| `game_output` | `line` | Process | 游戏输出转发（日志/宿主 UI） |
| `game_exit` | `exit_code` | LaunchController | 退出码/崩溃 |
| `auth_device_code_ready` | `user_code, verification_uri` | MicrosoftAuthProvider | 无 UI 认证展示 |
| `auth_expired` | `account_uuid` | AuthManager | 令牌过期提示 |
| `auth_refreshed` | `account_uuid` | AuthManager | 会话更新 |
| `auth_failed` | `service, error` | AuthManager | 认证失败 |
| `plugin_loaded` / `plugin_unloaded` | `plugin_name` | PluginRegistry | 插件生命周期 |
| `plugin_error` | `plugin_name, error` | PluginRegistry | 插件异常告警 |
| `version_manifest_updated` | `manifest_path` | Version | 清单刷新完成 |

订阅示例：

```cpp
app().Events().Subscribe<launcher::GameOutputEvent>([](const auto &e) {
    Logger::Info("game: {}", e.line);
});
```

事件命名规则：`<领域>_<动作>`，结构体命名 `<领域>Event`（如 `GameOutputEvent`）。

---

# 12. 配置体系设计

## 12.1 配置文件

默认路径：进程 CWD `config.json`（`$AURORA_CONFIG` 可覆盖）。加载 = 默认值 + 文件覆盖。

## 12.2 JSON Schema（完整示例）

```json
{
  "path": {
    "runtime_directory": "./runtime",
    "cache_directory": "./cache",
    "temp_directory": "./tmp",
    "log_directory": "./log"
  },
  "logger": {
    "level": "info",
    "flush_immediately": false,
    "console_output": true,
    "file_output": true,
    "file_name": "launcher.log",
    "is_async": false
  },
  "network": {
    "timeout_seconds": 30,
    "retry_count": 3,
    "verify_ssl": true,
    "proxy": "",
    "user_agent": "AuroraLauncher/0.1.0"
  },
  "runtime": {
    "worker_threads": 4,
    "debug_mode": false,
    "enable_cache": true,
    "download_concurrency": 8
  },
  "auth": {
    "default_service": "offline",
    "third_party_servers": [
      { "name": "LittleSkin", "yggdrasil_base_url": "https://littleskin.cn/api/yggdrasil" }
    ]
  },
  "download": {
    "mirrors": [
      { "from": "https://piston-meta.mojang.com", "to": "https://bmclapi2.bangbang93.com" }
    ]
  }
}
```

## 12.3 字段语义

| 字段 | 默认 | 语义 |
| :--- | :--- | :--- |
| `path.*` | 见 3.5 | 相对路径解析规则见 3.5 |
| `logger.is_async` | false | 异步日志开关 |
| `network.timeout_seconds` | 30 | 请求超时（秒） |
| `network.proxy` | "" | 空 = 直连；`http://host:port` / `socks5://host:port` |
| `network.user_agent` | `AuroraLauncher/0.1.0` | 全局 UA，可被 DownloadTask.headers 覆盖 |
| `runtime.download_concurrency` | 8 | 下载并发上限 |
| `auth.default_service` | offline | 默认认证方式（`offline`/`microsoft`/第三方站名） |
| `download.mirrors` | [] | 镜像前缀映射表 |

## 12.4 配置校验

- `Validate()` 规则见 3.5；新增：`auth.default_service` 必须为合法值；`third_party_servers[].yggdrasil_base_url` 必须为 `http(s)://` 前缀。
- 非法字段：加载时记 `Warn` 并回退该字段默认值；仅"结构性非法"（路径为空、超时非正）才拒绝加载。

---

# 13. 目录与文件布局

目标布局（与现状对齐，新增 `capi/`、`src/plugins/`、`include/*.h`）：

```
OlLauncherCore/
├── CMakeLists.txt
├── cmake/
│   ├── Settings.cmake          # C++23 / C++ 模块标准 / 编译选项
│   ├── DepPackage.cmake        # FetchContent 依赖策略
│   └── QuickJS.cmake           # quickjs-ng 构建
├── include/                    # 公开头文件（C ABI）
│   ├── export.h
│   ├── aurora.h
│   ├── au_export.h
│   ├── au_types.h
│   ├── au_error.h
│   ├── au_version.h
│   ├── au_context.h
│   ├── au_config.h
│   ├── au_logger.h
│   ├── au_auth.h
│   ├── au_download.h
│   ├── au_launch.h
│   ├── au_event.h
│   └── au_plugin.h
├── src/
│   ├── launcher.base/         # *.cppm：types/error/platform/build/config + 汇总
│   ├── aurora.service/        # *.cppm：random/uuid/logger/io/network/crypto/
│   │                          #          download/archive/cache/quickjs/auth + 汇总
│   ├── aurora.core/           # *.cppm：task/pool/scheduler/context/event + 汇总
│   ├── aurora.runtime/        # *.cppm：version/assets/jvm/process/launch/loader/plugin + 汇总
│   ├── base/                  # 实现 .cc（launcher.base 实现）
│   ├── aurora/                # 实现 .cc（service/core 实现，可按子目录细分）
│   ├── capi/                  # C ABI 实现 .c/.cc
│   └── plugins/               # C++ 编译期插件（预留）
├── tests/                     # 测试（按模块子目录）
├── examples/
├── docs/                      # 本文档 + DEVELOPMENT.md
├── resources/                 # 静态资源
├── third_party/               # 依赖源码（quickjs-ng 等）
└── tools/                     # 脚本/工具
```

## 13.1 文件命名与模块规范

（与 `DEVELOPMENT.md` 一致，摘要如下）

- 模块文件：`launcher.xxx.cppm` / `aurora.xxx.cppm`，子模块 `aurora.xxx:yyyy.cppm`；父模块文件仅作导出汇总。
- 实现文件 `.cc` 位于 `src/base/` / `src/aurora/`（及子目录），文件头声明 `module launcher.xxx;` / `module aurora.xxx;`（**必须用父模块名**，子模块名会导致编译失败）。
- 测试/示例文件全小写下划线命名。
- `src/CMakeLists.txt` 中模块文件必须按依赖顺序排列（已实践，新增模块遵循）。

---

# 14. 构建与依赖管理

## 14.1 工具链

| 项 | 要求 |
| :--- | :--- |
| 编译器 | Clang ≥ 18 / GCC ≥ 14 / MSVC ≥ VS2022 17.10 |
| 标准 | C++23（含 C++ Modules，`CMAKE_CXX_MODULE_STANDARD 23`） |
| CMake | ≥ 3.30 |
| Ninja | 推荐新版 |

## 14.2 依赖清单与策略

| 依赖 | 用途 | 引入方式 |
| :--- | :--- | :--- |
| fmt | 格式化 | FetchContent（系统缺失时） |
| spdlog | 日志后端 | FetchContent |
| nlohmann_json | JSON | FetchContent |
| libcurl | HTTP | `find_package` 系统库 |
| minizip-ng | 压缩解压 | `find_package`，缺失时降级（见 DepPackage.cmake 现状） |
| OpenSSL | 密码学/安全随机 | `find_package` 系统库 |
| quickjs-ng | JS 引擎 | `third_party/` 内嵌源码（QuickJS.cmake 构建） |

策略（沿用 `DEVELOPMENT.md`）：

1. 第三方 API **禁止直接暴露**给用户，必须经 `launcher` 封装（Service 层）。
2. 系统优先、FetchContent 兜底（`cmake/DepPackage.cmake` 的 `find_or_fetch`）。
3. 版本锁定：FetchContent 固定 tag（fmt 11.2.0、spdlog 1.17.0、nlohmann_json 3.12.0）。
4. 平台差异收敛在封装层，业务代码零平台宏（`launcher.base:platform` 例外）。

## 14.3 CI/CD（规划）

`docs/.github/workflows`（当前为空）规划：

| 工作流 | 平台 | 内容 |
| :--- | :--- | :--- |
| build | ubuntu-latest / windows-latest / macos-latest | Clang 18 / MSVC 编译 + 模块构建验证 |
| test | ubuntu-latest | 单测（tests/） |
| nightly-launch | ubuntu-latest（自托管可选） | 真实版本冒烟启动（离线模式，夜间） |

`main` 分支受保护：CI 通过 + ≥1 review 后才可合并（见 DEVELOPMENT.md）。

---

# 15. 测试与质量保障

## 15.1 单元测试

- 位置：`tests/<module>/`，命名 `main.cc`（现有惯例），按模块子目录组织。
- 框架：第一版使用**自研轻量宏框架**（`tests/test_framework.h`，`TEST_CASE(name)` + `CHECK/REQUIRE`），零第三方依赖；若后期需要参数化/子进程测试再评估 doctest。
- 必测范围：
  - `launcher.base:error`：Result 构造/拷贝/移动/组合子/void 特化。
  - `launcher.base:config`：默认值、JSON 覆盖、Validate 分支、路径解析。
  - `aurora.service:uuid`：V4 版本位/变体位、V7 时间戳、Parse/ToString 往返。
  - `aurora.service:random`：长度、填充、空输入错误。
  - `aurora.service:archive`：Zip-Slip 拒绝、往返解压。
  - `aurora.runtime:version`：Rules 求值矩阵、legacy 参数合并、兼容矩阵样例版本。
  - `aurora.core:event`：订阅/退订/Async 发布。
  - `aurora.core:task`：协程成功/错误路径、SyncWait 死锁检测。

## 15.2 集成测试

- 下载/缓存：本地 HTTP fixture（Python `http.server` 或 in-process 测试服务器）验证断点续传、校验失败切换镜像、缓存命中。
- 认证：离线模式全流程（纯本地）；微软/第三方用 mock 端点验证状态机与错误路径。
- 启动冒烟：CI nightly 用固定版本（如 1.8.9 / 1.21.4）离线启动至 `Running` 后强杀（自托管 runner，网络受限环境）。

## 15.3 质量规范

- 格式化：提交前 `clang-format`（LLVM 风格，根目录 `.clang-format`）。
- 编译告警：`-Wall -Wextra -Wpedantic`（GNU/Clang）/ `/W4`（MSVC），告警视为错误（CI 打开 `-Werror`，本地可关）。
- 静态分析：`clang-tidy` 可选接入；内存用 ASan 单测运行（Linux CI 步骤）。
- 覆盖：单测覆盖新增模块 ≥ 70%（后续配置 lcov）。

---

# 16. 开发路线图

## Phase 0 — 基座稳固（当前所处）

- 完成 `launcher.base`：`build` 实现、`ConfigManager::Load(Path)/Save(Path)`、`Validate` 全分支、单测。
- `aurora.service`：`logger` 收尾（异步、pattern、sink 测试）、`random`/`uuid` 单测。
- 落地 `test_framework.h` 与第一个测试目标。

**退出条件**：base + logger/random/uuid 单测全绿；CI build 通过。

## Phase 1 — 服务层补齐

- `crypto`、`io`、`network`（libcurl multi）、`archive`（含 Zip-Slip 测试）、`cache`、`download`、`quickjs`、`auth`（先离线 + 第三方，微软随后）。
- 修复 `aurora.service.cppm` 未导出 `download` 的问题。
- 每个服务配套单测。

**退出条件**：服务层接口齐备、单测覆盖 70%+；`download` 经本地 fixture 验证断点续传与镜像切换。

## Phase 2 — 核心层

- `task` 完善（`StartOn`/`SyncWait`/`Then`/`Catch`/`WhenAll`/`WhenAny`）。
- `pool` → `scheduler` → `context` → `event` 依序实现。
- 统一生命周期：`Context::Initialize/Start/Shutdown` 全流程可用。

**退出条件**：协程在池上调度正确、事件订阅/发布单测通过、Context 生命周期可被 CLI 示例驱动。

## Phase 3 — 运行时层

- `version`（manifest + 解析 + Rules + Recipe）、`assets`、`jvm`、`process`、`launch`（状态机 + 认证注入）。
- 用离线模式跑通 **1.8.9**（legacy）与 **1.21.4**（现代）两个冒烟版本。

**退出条件**：`LaunchController` 全流程事件序列正确，两次冒烟启动到达 `Running`。

## Phase 4 — 加载器与插件

- `loader`：Fabric → Quilt → Forge(legacy+现代) → NeoForge，安装器缓存。
- 插件：C++ 插件接口 + JS 插件运行时 + 桥接层（附录 B API）。

**退出条件**：Fabric 1.21.4 与 Forge 1.12.2/1.20.1 冒烟启动通过；示例 JS 插件（钩子 + HTTP + 事件）可用。

## Phase 5 — C ABI

- `include/` 头文件族、`src/capi/` 实现、导出符号控制（`AU_API`）。
- Rust / Python / C# 各一个示例绑定（examples/）。

**退出条件**：外部绑定可完成 初始化 → 配置 → 下载 → 离线启动 全链路。

## Phase 6 — 打磨

- CI 三平台矩阵、nightly 冒烟、覆盖率、文档补全（本设计文档随实现更新）。
- 性能（下载并发调优、协程续体内联）、安全审计（凭据存储、解压路径、SSRF 面）。

**退出条件**：发布 1.0.0 候选。

---

# 17. 风险与开放问题

| # | 风险/问题 | 影响 | 缓解 |
| :--- | :--- | :--- | :--- |
| 1 | C++ Modules 在 MSVC 上的成熟度 | 模块编译失败/行为差异 | 子模块保持薄导出；实现文件统一 `module aurora.xxx;`；CI 覆盖 MSVC |
| 2 | 协程 + 模块组合在 4 家工具链的差异 | Task 语义漂移 | promise 语义固化（5.2）；跨工具链单测 |
| 3 | 微软认证流变更（XSTS/Minecraft 服务） | 登录中断 | 端点集中常量 + 版本探测；错误码细化；跟随官方变更 |
| 4 | 全部 MC 版本兼容（legacy 资源、旧 Java、快照） | 启动失败面大 | 兼容矩阵测试（6.1）；冒烟测试固定版本库 |
| 5 | authlib-injector 与各皮肤站差异 | 第三方登录不稳定 | 仅依赖标准 Yggdrasil 端点；站点差异白名单 |
| 6 | QuickJS 单线程 + 异步桥 | 插件卡死内核 | 超时中断 + Promise 投递回 JS 线程；插件线程隔离 |
| 7 | 凭据静态加密强度不足 | 令牌泄露 | 明确文档风险；OS 密钥链列为后续版本 |
| 8 | 多实例 Context 未承诺 | 嵌入方多实例诉求 | 设计保留 `Context::Create`，接口先行，实现后续 |
| 9 | `download` 依赖 libcurl 异步 + 调度器 | 时序复杂度 | 状态机收敛在 DownloadManager 内部；对外只暴露句柄与事件 |

**开放问题**（需在实现过程中决策）：

- `AURORA_CLIENT_ID`（微软应用注册）落地值。
- 默认库源/镜像站的维护策略（国内网络环境）。
- 是否支持版本隔离的游戏目录（每个版本独立 `game_dir`），设计上以 `LaunchProfile.game_directory` 支持，默认行为待定。

---


# 18. 并发与线程模型

> 本章是全库并发语义的**总规范**。任何模块的线程行为不得与本章冲突；模块级细节见第 24 章实现规范。

## 18.1 线程清单

进程内线程全部由 Aurora 管理（宿主主线程除外），命名规则：`aurora-<role>-<n>`。

| 线程 | 数量 | 职责 | 归属 |
| :--- | :--- | :--- | :--- |
| 宿主主线程 | 1 | 调用 `Context::Initialize/Start/Shutdown`；宿主业务 | 宿主 |
| `aurora-pool-N` | `worker_threads` | 执行 Scheduler 投递的普通/高/低优先级任务 | `aurora.core:pool` |
| `aurora-timer` | 1 | 延时任务最小堆；到期后投递到池 | `aurora.core:scheduler` |
| `aurora-proc-<pid>` | 每游戏进程 1 | 阻塞读取子进程 stdout/stderr，行缓冲解码 | `aurora.runtime:process` |
| `aurora-js-<plugin>` | 每 JS 插件 1 | 执行该插件全部 JS 代码 | `aurora.service:quickjs` |
| libcurl 内部线程 | curl 决定 | multi 接口事件轮询；完成回调转为任务投递 | `aurora.service:network` |

约束：

- 业务代码**禁止自行创建 `std::thread`**（一次性同步探测等例外须文档说明）。
- `aurora-timer`、`aurora-proc-*`、`aurora-js-*` 均为"专用线程"：只做自己的事，不得执行任意回调。

## 18.2 线程安全分级

所有 API 标注为以下四级之一（文档注释中必须写明）：

| 级别 | 语义 | 示例 |
| :--- | :--- | :--- |
| S1 全线程安全 | 任意线程可调用 | `Logger::*` 门面、`Result` 值类型、`EventDispatcher::Publish`、C ABI 全部函数、`ConfigManager::Get` |
| S2 上下文线程安全 | 仅"正在运行的 Context"内任意线程可调用（生命周期外为 UB） | `context.Get<T>()`、`Scheduler::Schedule`、`DownloadManager` |
| S3 单线程（对象） | 仅对象所属线程可调用 | `QuickJS::Eval`（JS 线程）、`ScriptContext` |
| S4 只读共享 | 初始化后只读，可被任意线程读 | `Config` 只读视图、`BuildInfo`、`VersionProfile`（解析完成后不可变） |

规则：

- 接口注释必须声明级别；实现标注 `// Thread-safety: S1`。
- S3 对象跨线程访问属编程错误：Debug 断言线程 id，Release 记录致命日志并返回 `Err(InvalidState)`。

## 18.3 锁顺序与死锁避免

全局加锁顺序（从外到内）：

```
1. PluginRegistry 锁
2. EventDispatcher 锁
3. Scheduler/Pool 队列锁
4. 各 Service 内部锁（Network 句柄表、Download 任务表、Auth 会话表、Cache 索引、Logger 状态）
```

强制规则：

- **禁止**以相反顺序加锁。
- **禁止**在持有 1–3 级锁时阻塞等待任务完成（`SyncWait`、`Wait`）——必然死锁。
- 回调（事件处理器、进度回调）视为"无锁上下文"：进入回调时不得假定持有任何锁；回调内只能获取最内层（第 4 级）锁且必须短促。
- 锁粒度：队列锁只保护"队列结构"，不保护任务执行；任务执行期间不得持锁。

## 18.4 线程亲和与回调规则

1. **回调投递线程**：事件默认在发布线程同步调用（S1 语义由事件总线保证）；异步发布投递到池线程。
2. **回调禁止**：阻塞（`Sleep`/同步 IO/`SyncWait`/`Wait`）、调用 `Context::Shutdown`、销毁自己仍在使用的资源。
3. **跨线程结果传递**：一律经"值拷贝进任务/事件"；禁止跨线程传递 `StringView` / 引用 / 指针（S4 只读对象除外）。
4. **C ABI 回调**：投递到池线程，同规则 2；`user` 指针生命周期由调用方管理（见 8.5）。

## 18.5 并发工具约定

- 加锁统一 `std::scoped_lock`（多锁时按 18.3 顺序一次锁定）；需要条件变量时用 `std::unique_lock` + 谓词等待（防伪唤醒）。
- 原子变量仅用于统计（计数、字节数、标志位）；禁止用原子模拟锁语义。
- 禁止自旋锁、`volatile` 同步；`condition_variable` 等待必须带超时兜底（防悬挂）。
- 共享状态变化后必须 `notify_all`（保守选择）或精确 `notify_one`。

---

# 19. 内存管理与对象生命周期

## 19.1 所有权规则

| 对象 | 所有者 | 跨线程传递方式 | 生命周期 |
| :--- | :--- | :--- | :--- |
| `Context` | 默认单例（静态）/ `SharedPtr`（隔离实例） | `SharedPtr` | `Initialize` → `Shutdown` |
| 服务实例 | `Context` 注册表（`SharedPtr`） | `Get<T>()` 返回引用（S2） | 随 Context |
| `Task<T,E>` | 移动语义所有者 | 移动（禁止拷贝） | 完成/析构时销毁协程帧 |
| `EventSubscription` | 订阅者 | 移动 | 析构自动退订 |
| `DownloadHandle` | 调用方 | 拷贝引用计数句柄 | `Release`/析构 |
| `Process` | 调用方（RAII） | 移动 | 析构未退出则 `Kill` |
| `LaunchController` | 调用方 + LaunchManager 弱引用 | `SharedPtr` | 退出/失败后仍可查询，直至释放 |
| C ABI 句柄 | 调用方 | 不透明指针 | `au_*_release` |

规则：

- 新建对象默认 `UniquePtr`；确需共享才 `SharedPtr`；**禁止裸指针所有权**。
- 禁止 `new/delete`、`malloc/free`（`launcher.base:types` 与第三方封装边界除外，且须 RAII 包裹）。
- 回调捕获的共享资源必须用 `WeakPtr` 弱化，回调入口先 `lock()` 再使用，防止悬垂。

## 19.2 对象生命周期图

```
Context (进程)
 ├── ConfigManager::Get() ──────► Config（S4 只读）
 ├── ServiceRegistry
 │    ├── Logger（门面 → 内部 SharedPtr<spdlog::logger>）
 │    ├── Network（curl 句柄池）
 │    ├── DownloadManager（任务表）
 │    ├── AuthManager（accounts 缓存）
 │    ├── Cache（索引 + 磁盘）
 │    └── QuickJS（JS 插件运行时表）
 ├── Scheduler ──► ThreadPool
 ├── EventDispatcher（订阅表）
 └── PluginRegistry（插件实例表 + 钩子表）

LaunchController（每次启动 1 个，SharedPtr）
 ├── LaunchProfile（S4）
 ├── LaunchRecipe（S4，解析后不可变）
 ├── VersionProfile（S4）
 └── Process（RAII）
```

销毁顺序（`Context::Shutdown`，严格倒序）：

```
1. PluginRegistry::UnloadPlugins()      // 插件先释放（可能持有服务引用）
2. Scheduler::Stop() + Pool 停止         // 不再投递新任务
3. DownloadManager 取消全部任务并等待收尾
4. Network 关闭（curl_global_cleanup）
5. AuthManager 持久化 accounts.json
6. 服务反初始化（注册逆序）
7. EventDispatcher 清空订阅表
8. Logger::Shutdown()（最后，flush）
```

## 19.3 协程帧与回调生命周期

- `Task` 的协程帧：由 Task 唯一拥有，析构或 `Done()` 路径调用 `handle_.destroy()`；`final_suspend` 恒 `suspend_always` 保证帧在结果被读取前不释放。
- 调度器投递的闭包（`Function<void()>`）：`std::function` 值拷贝进队列；任务执行完毕即析构，禁止队列外借用。
- 事件订阅处理器捕获的资源：捕获 `WeakPtr`；事件载荷为值类型（拷贝进 `std::function` 捕获表），发布返回后载荷失效。

## 19.4 防泄漏清单

- 每新增一个"创建型 API"，必须同时提供对应释放路径（RAII 或显式 `Release`），并在代码审查清单（25.4）勾选。
- `Context::Shutdown` 后不得残留：运行中的任务、未退订的订阅、未销毁的 JS 运行时。
- CI 单测以 ASan/LSan 运行（Linux），泄漏视为失败。
- 启动冒烟测试结束后断言：进程内线程数回到基线（宿主主线程 + pool + timer）。


---

# 20. 安全设计

## 20.1 威胁模型

Aurora 作为启动器核心库，主要暴露面：不可信网络数据（Mojang/镜像/皮肤站响应）、不可信本地文件（version.json、资源索引、插件脚本、压缩包）、宿主传入输入（配置、URL、路径、凭据）。

| # | 威胁 | 影响 | 缓解 |
| :--- | :--- | :--- | :--- |
| T1 | 恶意 version.json/索引（路径穿越、超长字段、zip bomb） | 任意写盘 / 内存耗尽 | 20.2 输入验证、20.4 解压上限 |
| T2 | 中间人篡改下载内容 | 投毒（加载恶意 jar） | TLS 校验默认开启 + sha1 校验 + 来源固定（20.3） |
| T3 | 恶意 JS 插件逃逸沙箱 | 任意代码执行 | 20.5 沙箱设计 |
| T4 | 本地凭据泄露（accounts.json） | 账号被盗 | 20.6 凭据存储 |
| T5 | 依赖供应链（FetchContent/镜像） | 构建期投毒 | 20.7 供应链 |
| T6 | SSRF（插件/配置诱导请求内网） | 内网探测 | 20.2 URL 校验 + 插件权限门控 |
| T7 | 日志泄露令牌/密钥 | 凭据泄露 | 20.6 脱敏规则 |
| T8 | 解压符号链接/硬链接逃逸 | 任意写盘 | 20.4 条目白名单 |

## 20.2 输入验证

**JSON 解析**（所有远程/本地 JSON）：

- 解析用 `IO::JsonRead`（内部捕获 `nlohmann_json` 异常 → `ParseError`）。
- 结构校验：必需字段缺失按类型处理——核心字段（如 `main_class`）缺失 → `InvalidFormat`；可选字段缺失 → 默认值。
- 字段类型不匹配：`InvalidFormat` + 字段名（错误消息必须包含字段路径，如 `libraries[3].name`）。
- 字符串长度上限：`version.json` 单字段 ≤ 64KB；数组元素数上限（libraries ≤ 4096、asset objects ≤ 200000）。

**路径安全**：

- 所有从外部输入构造的路径必须规范化：`lexically_normal()` + 拒绝 `..` 逃逸 + 必须位于允许根（解压目标、插件目录、runtime 白名单）内。
- 统一入口 `IO::SafeResolve(Path root, StringView rel)`（实现见 24.16），返回 `Result<Path>`；失败 `InvalidArgument`。

**URL 安全**：

- 仅允许 `http://` / `https://`；拒绝包含 `userinfo@`、换行、控制字符的 URL。
- 重定向：跟随但受限（最多 5 跳，禁止 `file://` 等非 http(s) 协议跳转）。
- 插件发起的请求：目标主机必须非内网/环回（`127.0.0.0/8`、`10/8`、`172.16/12`、`192.168/16`、`169.254/16`、`::1`、`fc00::/7` 等），除非宿主显式放行。

## 20.3 下载安全

- TLS：`verify_ssl` 默认开启；证书链校验失败 → `Security` 分类错误，**禁止静默降级**。
- 完整性：`expected_sha1` 存在则强制校验；校验失败宁可失败也不落盘（原子 rename 前校验）。
- 来源固定：核心资产（version/asset index/库）只允许从 manifest 中声明的 URL 或配置镜像下载；禁止任意 URL 注入。
- 断点续传：服务端返回 `206` 才继续；`200` 时从头下载（校验 `.part` 大小可能不匹配）。

## 20.4 解压安全

- Zip-Slip：拒绝绝对路径与 `..` 条目（见 4.8）。
- 符号链接/硬链接/设备文件条目：默认拒绝（`keep_legacy_symlinks=false`）。
- **Zip Bomb 防护**：解压前读中央目录，校验总解压大小 ≤ `max_total_size`（默认 8GB，可配置）、条目数 ≤ 200000、单条目录深度 ≤ 16；超限 → `InvalidFormat`。
- 解压进度：超过 `extract_progress` 阈值可被取消（返回 `Cancelled`，已解压文件由调用方决定清理）。

## 20.5 JS 沙箱

- **运行时隔离**：每插件独立 `JSContext`；插件间无法互相访问。
- **模块系统受限**：自定义模块加载器只允许加载插件目录内相对路径；禁止 `require("fs")` 等 Node 风格模块；不暴露 QuickJS std/os 扩展（编译 quickjs 时禁用 `quickjs-libc` 的 std/os 模块导出，仅保留 `QJS_BUILD_LIBC` 必要部分——实现时确认只注册 Aurora 桥）。
- **能力门控**：全局对象只有 `aurora`（桥接对象）与标准 ECMAScript 内建；所有桥函数入口做权限检查（20.2 的 `SafeResolve` + 清单权限）。
- **资源限制**：堆上限（默认 64MB，超限中断 + `ScriptError`）、单次调用超时（默认 5s，中断回调检查 tick 计数）。
- **禁止逃逸向量**：`Function.prototype.constructor` 相关利用由 QuickJS 版本修复跟进；关闭 `eval`/`new Function` 的宿主扩展（保留标准语义但受超时/内存限制约束）。
- 桥接返回值校验：从 JS 返回内核的值（如钩子修改的 recipe）只合并白名单字段（见 7.3）。

## 20.6 凭据存储

- `accounts.json`：`access_token` / `refresh_token` 经 `Crypto::AesGcmEncrypt` 加密存储（密钥策略见 9.5）；文件权限 `0600`。
- **日志脱敏（强制）**：`access_token`、`refresh_token`、`device_code`、密码永不打日志；错误消息中若含令牌必须替换为 `<redacted>`（统一工具 `details::Redact(StringView)`）。
- 内存中令牌仅存在于 `AuthSession` 与加密缓冲；`AuthSession` 析构不清零（性能取舍，文档明示），但禁止拷贝进日志/事件。
- C ABI 层 `au_auth_session` 返回的令牌由调用方负责安全存储；文档标注。

## 20.7 供应链

- FetchContent 全部固定 tag（fmt 11.2.0 / spdlog 1.17.0 / nlohmann_json 3.12.0），可选项：`GIT_TAG` 后附提交哈希校验（实现时评估 `FETCHCONTENT_FULLY_DISCONNECTED` 缓存）。
- `third_party/` 内嵌源码（quickjs-ng）定期同步上游安全修复，记录版本于 `third_party/README.md`。
- CI 依赖审计：`pip-audit` 不适用（C++）；改为依赖清单 + tag 变更必须出现在 PR 描述中。

---

# 21. 性能设计

## 21.1 目标预算

| 指标 | 目标 | 测量方式 |
| :--- | :--- | :--- |
| 下载吞吐 | 单任务 ≥ 80% 链路带宽（局域网/千兆） | benchmarks/download |
| 日志门面开销（级别关闭） | ≤ 20ns/次 | benchmarks/logger |
| 事件发布延迟（无订阅） | ≤ 100ns | benchmarks/event |
| 版本解析（1.21.4） | ≤ 100ms（缓存命中）/ ≤ 3s（冷启动含下载） | benchmarks/version |
| 启动流水线额外开销 | ≤ 200ms（解析→Spawn，全缓存命中） | benchmarks/launch |
| 内存峰值（解析 1.21.4 索引） | ≤ 300MB（含 assets objects 表） | ASan/基准 |

## 21.2 热路径与优化原则

热路径（按频率排序）：

1. 日志门面（每次调用都要经过，级别关闭时必须接近零成本——先查 `details::GetLogger()` 与级别，再格式化）。
2. 事件分发（发布频率高：下载进度默认节流 100ms 一次）。
3. 下载 IO（零拷贝缓冲链：curl 写回调 → `Bytes` 预分配；`WriteFile` 用单次 `fwrite` 大块）。
4. 资源对象表查询（`unordered_map<hash, size>`；hash 为 40 字符 sha1 字符串，用 `std::string_view` 索引避免拷贝）。

原则：

- 优化必须有基准数据支撑；禁止"先优化后测量"。
- 正确性 > 性能：任何优化不得改变错误语义（见第 3 章）。
- 大文件（资源 object、jar）禁止整读进内存（流式哈希/流式解压）；JSON 解析目标 ≤ 100MB 文件例外（仍建议流式，后续版本）。
- 缓冲区复用：下载写入用池化 `Bytes`；日志格式化为栈上小缓冲（`fmt::memory_buffer`）。

## 21.3 并发度模型

- 下载并发：`min(download_concurrency, worker_threads * 2)`；超上限任务排队（高优先级队列）。
- 版本/资源解析：单任务流水线（无并行）；下载阶段并行（多 object 并发）。
- 事件订阅数无硬上限；发布复杂度 O(订阅者数)。
- 线程池默认 `max(4, hardware_concurrency)`；小核设备（2 核）至少 4 线程避免下载饿死。

## 21.4 缓存与预取

- 版本 manifest 缓存 TTL：24h（`version_manifest_updated` 事件）；启动时异步刷新不阻塞。
- 资产对象按"最近启动版本"预取策略（后续版本）：记录 `last_launched.json`，下次启动前后台预热缺失对象。
- 日志文件：异步模式下批量落盘（spdlog 默认行为），`Shutdown` 强制 flush。

## 21.5 基准测试

- 位置：`benchmarks/`（独立目标 `aurora_bench`，仅 CI nightly 或手动运行，不进常规测试）。
- 每个基准输出 JSON 结果；历史对比存 `benchmarks/results/`；性能回归（>10%）在 CI 标注警告。
- 基准运行环境固定（CPU 型号、Release 构建、-O3）。


---

# 22. 可测试性设计

## 22.1 测试分层

| 层 | 位置 | 运行 | 依赖 | 目标 |
| :--- | :--- | :--- | :--- | :--- |
| 单元测试 | `tests/<module>/` | 每次提交（CI） | 无网络、无真实磁盘（用临时目录） | 模块行为与边界 |
| 集成测试 | `tests/integration/` | 每次提交（CI） | 本地 HTTP fixture、临时文件系统 | 跨模块协作（下载/缓存/认证 mock） |
| 冒烟测试 | `tests/smoke/` | nightly（自托管） | 真实网络（或镜像）+ 真实 MC 版本 | 端到端启动 |
| 基准测试 | `benchmarks/` | nightly | 固定硬件 | 性能回归 |

框架：第一版自研 `tests/test_framework.h`（`TEST_CASE` / `CHECK` / `REQUIRE` / `SECTION`），支持：

- `RUN_ALL_TESTS()` 主入口；失败返回非零并打印失败表达式与位置。
- 断言失败带 `source_location`；支持 `CHECK_THROWS` 不可用（无异常）→ 用 `CHECK_RESULT_ERROR(expr, ErrorCode)`。
- 每个测试文件独立可执行（现有 `tests/CMakeLists.txt` 模式扩展）。

## 22.2 测试替身

| 替身 | 用途 | 实现 |
| :--- | :--- | :--- |
| `FakeNetwork` | 测试下载/缓存/版本解析不依赖真实网络 | 实现 `Network` 同接口，本地 `http::server`（Python fixture 或 in-process 迷你 HTTP）返回预置响应、可控延迟/断连 |
| `MemoryCache` | 缓存逻辑单测 | 实现 `Cache` 接口，内存 map |
| `FakeScheduler` | 任务/协程时序单测 | 同步执行投递任务，可手动触发延时任务 |
| `FakeClock` | 超时/令牌过期/节流测试 | 时间抽象 `Clock::Now()` 可注入（默认 `system_clock`） |
| `FakeProcess` | 启动流水线测试 | 脚本假进程（输出预置行、指定退出码、可挂起） |

注入方式：

- 服务接口设计为可替换：`Context::Register<T>` 允许测试注册替身（替换内置实现）；生产代码一律经接口取服务。
- 时钟：`launcher.base:time` 提供 `Clock` 抽象（`Now()/Sleep()`），生产用真实实现，测试注入 `FakeClock`。**禁止直接调用 `std::chrono::system_clock::now()` 于业务逻辑**（UUID v7 例外，见 4.3）。
- 随机：`Random` 提供 `Debug` 构建下的 `SetTestMode(seed)`（确定性序列），Release 下该接口不存在（编译期隔离）。

## 22.3 确定性规则

- 所有含并发的测试默认使用单线程池（`worker_threads=1`），时序确定。
- 事件订阅顺序测试：同订阅者在同一发布内按注册顺序调用（文档化承诺，见 24.4）。
- 下载测试：fixture 对相同 URL 返回相同字节；断点续传测试控制 `Content-Range` 行为。
- 时间相关测试：一律 `FakeClock`；禁止 `sleep(100ms)` 式等待断言，用条件变量 + 超时谓词。

## 22.4 覆盖率目标

- 新模块单测行覆盖 ≥ 70%（`--coverage` + lcov，Linux CI 生成报告）。
- 关键路径 100%：`Error/Result` 全部组合子、`UUID::Parse` 非法输入、`SafeResolve` 穿越用例、下载校验失败分支、`Context::Shutdown` 错误路径。
- 覆盖率不达标模块禁止合入 `main`（PR 检查项）。

## 22.5 测试组织与命名

- 目录：`tests/<module>/<case>.cc`（每文件一个主题）；`tests/CMakeLists.txt` 自动 glob 注册。
- 命名：`TEST_CASE("<module>:<subject>")`，如 `TEST_CASE("uuid:parse-invalid")`。
- 集成测试 fixture 目录：`tests/fixtures/`（预生成 version.json、索引、zip 样本）；样本文件生成脚本入 `tools/`，禁止手工二进制入库（用脚本生成 + 校验）。
- 冒烟测试的版本清单固定于 `tests/smoke/versions.json`（1.8.9、1.12.2、1.21.4、最新快照各一），版本变更走 PR。

---

# 23. 日志与诊断设计

## 23.1 日志格式

统一格式（spdlog pattern）：

```
[2026-08-11 22:15:03.123] [info ] [launcher.core:context] Context initialized (workers=4)
```

字段：

| 字段 | 说明 |
| :--- | :--- |
| 时间戳 | `YYYY-MM-DD HH:MM:SS.mmm`，本地时区 |
| 级别 | `trace/debug/info/warn/error/critical`，宽度对齐 |
| 模块标签 | `launcher.<layer>:<module>` 或 `aurora.<layer>:<module>`，用于过滤 |
| 消息 | UTF-8；禁止非打印字符（控制字符转义） |

## 23.2 日志级别语义

| 级别 | 语义 | 示例 |
| :--- | :--- | :--- |
| Trace | 每调用级细节（开发） | 请求头、协程切换点 |
| Debug | 调试信息（`debug_mode` 开启） | 解析字段、下载分片 |
| Info | 关键流程节点 | 生命周期、启动阶段、完成 |
| Warn | 可恢复异常 | 镜像失败切换、字段回退默认值 |
| Error | 操作失败（可恢复到错误路径） | 下载最终失败、认证失败 |
| Critical | 不可恢复/需立即注意 | 初始化失败、内部不变量破坏 |

规则：

- `debug_mode=false` 时 `Debug/Trace` 编译为近零成本（门面内先查级别）。
- 关键流程必须成对：`Info("...开始")` / `Info("...完成")`，带耗时（`elapsed_ms`）。
- 错误日志必须可操作：包含错误码、分类、上下文对象（版本 id、URL、路径），并附带 `Error::ToString()` 完整错误链（23.3）。

## 23.3 错误链格式化

`Error::ToString()` 输出规范：

```
Error[Config:ParseError] 配置 JSON 解析失败
  at src/base/config.cc:123
  caused by: Error[Parse:ParseError] unexpected end of JSON input
    at src/base/config.cc:118
```

实现要求：

- 顶层消息：`Error[<Category>:<Code>] <message>`。
- `location` 附于首行后（`at <file>:<line>`）。
- `cause_` 链递归缩进（每层 +2 空格，最多 8 层防环/过深）。
- 日志输出错误对象时**必须**用 `ToString()`（含链）；禁止只打 `Message()`。

## 23.4 诊断转储

- 位置：`runtime/diagnostics/`（目录不存在则创建）。
- `last-run.json`（每次启动写）：`{ timestamp, version_id, loader, java: {path, major}, recipe: {main_class, jvm_args, game_args}, exit_code }`。
- 崩溃/失败时：`crash-<yyyyMMdd-HHmmss>.json` 追加错误链快照与进程状态（线程数、下载中任务数）。
- 诊断文件写入失败仅记 `Warn`，不得阻断主流程。
- 隐私：诊断文件**不含**令牌、账号 uuid 之外的任何凭据（见 20.6）。


---

# 24. 模块实现规范（Implementation Notes）

> 本章给出各模块的**实现级规范**：数据结构、状态机、不变量、算法伪代码。实现必须与本章一致；发现冲突时以本章为权威并回改设计文档（同步修订）。

## 24.1 Context 实现规范

### 24.1.1 状态机

```
Created ──Initialize(config)──► Initialized ──Start()──► Started
  │                                │                        │
  │                                │◄───────Stop()──────────┤
  │                                ▼                        ▼
  └──(析构)                    Shutdown()              Stopping ──► Stopped ──(可析构)
```

- `Initialize`：创建服务注册表、Scheduler、EventDispatcher、PluginRegistry；**不启动线程**。
- `Start`：启动池线程与 timer；开始接受任务。
- `Stop`：停止接受新任务；等待在跑任务完成（可配置超时，超时强制 `Shutdown` 语义）。
- `Shutdown`：执行第 19.2 节销毁顺序；幂等（重复调用返回 `Ok`）；状态非法时（未初始化）返回 `Err(InvalidState)`。

### 24.1.2 服务注册表实现

```cpp
namespace details {
// type_index → 工厂与实例；实例以 SharedPtr<void> 存储，Get<T> 时 static_pointer_cast
using ServiceFactory = Function<SharedPtr<void>(Context &)>;
struct ServiceSlot {
    SharedPtr<void> instance;
    StringView name;          // 调试/诊断名，如 "aurora.service:network"
};
// 注册表：插入序 = 初始化依赖序；Shutdown 时逆序销毁
Vector<Entry<type_index, ServiceSlot>> services_;
}  // namespace details
```

- `Register<T>(args...)`：若 `T` 已注册 → `Err(InvalidState, "service already registered")`；否则按构造完成插入。
- `Get<T>()`：线性/哈希查找 `type_index`；未注册：Debug 断言 + Release 返回 `nullptr` 并记 `Critical`（文档约定 Get 返回引用，此路径视为致命编程错误）。
- 内置服务注册顺序（固定，见 2.6）：`Logger → Random/UUID/Crypto → IO → Network → Archive/Cache → Download → QuickJS → Auth`。

### 24.1.3 单例与隔离实例

```cpp
static SharedPtr<Context> g_default;                 // 默认实例
static SharedPtr<Context> g_active;                  // Instance() 返回对象
static std::mutex g_context_mutex;
```

- `Instance()`：返回 `g_active`；未初始化 → 断言 + 返回"僵尸"上下文（所有调用返回 `Err(InvalidState)`，防静默空指针）。
- `SetActive(ctx)`：允许宿主切换默认实例（多实例宿主用）；切换前 `g_active` 必须已 Stopped。
- 测试建议：`Create(config)` + `SetActive(ctx)`，测试结束 `SetActive(nullptr)` + `Shutdown`。

## 24.2 Task 实现规范

### 24.2.1 awaitable 契约

`Task<T,E>` 可被 `co_await`（`Task<void,E>` 同），内部提供：

```cpp
struct TaskAwaiter {
    Task<T, E> &task;
    bool await_ready() const noexcept { return task.Done(); }
    void await_suspend(Handle caller) noexcept {
        // 续体注册：当前协程恢复调度到 Scheduler（投递 continuation）
        // 实现：task 完成时（final_suspend 中）把 caller 投递到 Scheduler::Schedule
        task.SetContinuation(caller);
    }
    Result<T, E> await_resume() { return task.TakeResult(); }
};
```

- 续体注册由 `promise_type::final_suspend` 完成：若注册了 continuation，则 `Scheduler::Schedule([handle]{ handle.resume(); })`；否则保持挂起直至 Task 析构。
- **不变量**：`co_await task` 只允许一次；重复 await → `Err(InvalidState)`（Debug 断言）。
- **调度语义**：续体在池线程执行（第一版不做线程内联优化，见 5.4）；`co_await` 返回后 `Thread::id` 可能变化，业务代码不得依赖线程连续性。

### 24.2.2 SyncWait 实现与死锁防护

```cpp
Result<T, E> SyncWait() {
    // 防护：若当前线程是池工作线程 → 立即返回 Err(InvalidState)
    if (ThreadPool::IsPoolThread()) {
        return Err(InvalidState, "SyncWait on pool thread would deadlock");
    }
    std::mutex m; std::condition_variable cv; bool done = false;
    SetContinuation([&](Task &t) { { std::lock_guard g(m); done = true; } cv.notify_one(); });
    // 若任务已在池上执行，此处阻塞等待；调用方必须是宿主线程/非池线程
    { std::unique_lock lk(m); cv.wait(lk, [&]{ return done; }); }
    return TakeResult();
}
```

- `ThreadPool::IsPoolThread()`：线程局部 `thread_local bool`，工作线程入口置 true。
- `SyncWait` 的调用线程必须不是池线程（宿主主线程或外部线程）；违反 → 直接错误返回（不挂死）。

### 24.2.3 内存与调试

- 协程帧分配：默认 `operator new`；Debug 构建启用 `-fsanitize=address` 检测帧泄漏。
- `Task` 可打印句柄地址（`ToString()` 输出 `task@0x...`），配合 `logger` Debug 级排查悬挂续体。
- `WhenAll` 实现：N 个共享计数 + 结果槽；全部完成或首个 `Err` → 唤醒等待者；必须处理"等待者已析构"（WeakPtr 回调）。

## 24.3 Scheduler / Pool 实现规范

### 24.3.1 队列结构

```cpp
class ThreadPool {
    struct TaskSlot { Function<void()> fn; u64 seq; };   // seq = 插入序号，FIFO 稳定
    std::array<std::deque<TaskSlot>, 3> queues_;         // [Low, Normal, High]
    std::mutex mu_; std::condition_variable cv_;
    std::atomic<bool> stopping_{false};
    Vector<std::thread> workers_;
    thread_local inline bool t_is_pool_thread = false;
};
```

### 24.3.2 取任务算法

```
worker loop:
  lock mu_
  wait until: stopping_ || 任一队列非空
  取任务：优先 High → Normal → Low；同级取队首（FIFO）
  unlock
  执行 fn（try/catch(...) 包裹，异常 → Logger::Error，不崩溃）
  若 High 队列持续非空 → 低优先级可能饥饿：每 1024 次 High 取任务后检查 Low 是否超时
    （第一版策略：High 连续取 N=512 次后强制取一次 Normal/Low，防饥饿）
```

- **防饥饿**：高优先级任务连续执行上限 512 个后，必须服务一次低优先级队列。
- **Stop 协议**：`stopping_ = true` + `notify_all`；在跑任务自然完成；队列剩余任务丢弃并记 `Warn`（数量）。
- `Submit` 返回 `Result<void>`：`stopping_` 时为 `Err(InvalidState)`。

### 24.3.3 延时任务

```cpp
class Scheduler {
    struct TimerTask { std::chrono::steady_clock::time_point at; Function<void()> fn; u64 seq; };
    std::priority_queue<TimerTask, ..., greater> timer_heap_;   // 按 (at, seq)
};
```

- `aurora-timer` 线程循环：`cv.wait_until(heap.top().at)`；到期弹任务投递到 Pool。
- 取消：本期不支持按句柄取消延时任务（执行时检查取消标志位 `std::shared_ptr<std::atomic<bool>>`，宿主可置位跳过执行）。
- 时钟统一 `Clock::Now()`（22.2），测试可注入。

## 24.4 Event 实现规范

### 24.4.1 数据结构

```cpp
class EventDispatcher {
    struct Slot {
        std::type_index type;
        Function<void(const void *)> invoke;   // 类型擦除：static_cast<E*> 后调用
        u64 id;                                // 递增，用于退订
    };
    // 发布期间订阅变更：先拷贝快照发布，变更延迟到发布返回后应用（写后发布/发布后写）
    std::mutex mu_;
    Vector<Slot> slots_;                       // 按订阅顺序
    bool publishing_ = false;
    Vector<Slot> pending_add_; Vector<u64> pending_remove_;
};
```

### 24.4.2 语义承诺

- 同一订阅者在一次发布内**按注册顺序**收到事件（文档化承诺，测试依赖此行为）。
- 发布期间订阅/退订：不影响当前发布（快照语义）；下次发布生效。
- `Publish(mode=Sync)`：遍历快照调用；处理器抛异常 → 总线捕获、`Logger::Error`、继续下一个（单处理器异常不中断发布）。
- `PublishAsync`：拷贝事件值 → `Scheduler::Schedule([e = std::move(ev)]{ dispatcher.DispatchSync(e); })`；发布顺序不保证（异步语义），需顺序的用 Sync。
- 退订幂等：`EventSubscription::Unsubscribe` 多次调用安全；析构自动退订（`publishing_` 期间延迟执行）。

### 24.4.3 死锁防护

- 事件处理器内**禁止** `Publish` 同一类型事件（重入）：Debug 断言（`thread_local` 重入深度），Release 记录 `Warn` 并跳过重入发布。
- 事件处理器内禁止 `SyncWait`/阻塞（18.4 规则 2）；违规由 24.2.2 防护兜底返回错误。


## 24.5 Network 实现规范

### 24.5.1 生命周期与句柄池

```cpp
class Network {
    // curl_global_init 由 Context 在 Initialize 时调用一次，Shutdown 时 curl_global_cleanup
    CURLM *multi_;                                  // multi 句柄
    std::unordered_map<CURL *, SharedPtr<EasyRequest>> easy_map_;   // 在飞请求表
    std::mutex mu_;                                 // 保护 easy_map_
    std::atomic<u64> next_request_id_;
};
```

- 每次请求创建独立 `CURL*`（easy 句柄池复用为后续优化）；请求结束从 `easy_map_` 移除并 `curl_easy_cleanup`。
- `multi` 事件循环：`aurora-timer` 或独立 `aurora-curl` 线程执行 `curl_multi_perform`（第一版复用 `aurora-timer` 的 10ms tick 轮询 `curl_multi_poll`）。

### 24.5.2 完成回调协议

```
curl_multi_info_read 返回 CURLMSG_DONE
   → 收集响应（header list → Vector<HttpHeader>，body buffer → Bytes）
   → 投递 Scheduler::Schedule([req]{ req->OnDone(result); })     // 不占用 curl 线程
```

- 写回调（`CURLOPT_WRITEFUNCTION`）：追加到预分配 `Bytes`；累计超过上限（默认 512MB，防恶意响应）→ 中断并 `Err(InvalidFormat)`。
- 进度回调（`CURLOPT_XFERINFOFUNCTION`）：节流 100ms 触发下载进度事件（21.2）。

### 24.5.3 超时与重试矩阵

| 场景 | 行为 | 结果 |
| :--- | :--- | :--- |
| 连接超时（`connect_timeout`=10s） | 重试（`retry_count`） | `ConnectionFailed` |
| 读超时（`timeout`） | 重试（指数退避 1s/2s/4s） | `Timeout` |
| HTTP 5xx | 重试（退避） | `NetworkError`（含 status） |
| HTTP 4xx | 不重试 | `NetworkError`（含 status） |
| TLS 校验失败 | 不重试 | `Security` 分类 |
| 重定向 > 5 跳 | 不重试 | `NetworkError` |

### 24.5.4 错误映射表

| libcurl 错误 | Aurora 错误 |
| :--- | :--- |
| `CURLE_COULDNT_CONNECT` / `CURLE_COULDNT_RESOLVE_HOST` | `NetworkError` / `ConnectionFailed` |
| `CURLE_OPERATION_TIMEDOUT` | `Timeout` |
| `CURLE_SSL_*` | `Security` + `NetworkError` |
| `CURLE_WRITE_ERROR` / `CURLE_PARTIAL_FILE` | `NetworkError` |
| `CURLE_ABORTED_BY_CALLBACK` | `Cancelled` |

## 24.6 Download 实现规范

### 24.6.1 任务状态机

```
Pending ──submit──► Queued ──调度──► Downloading
                                        │ 完成+校验
                                        ├──► Verifying ──通过──► Completed
                                        │        └──失败──► MirrorSwitch ──► Downloading(下一镜像)
                                        │                              └──无镜像──► Failed(ChecksumMismatch)
                                        ├──取消──► Cancelled
                                        └──网络错误×重试耗尽──► MirrorSwitch ──无镜像──► Failed(DownloadFailed)
```

- 任务表：`unordered_map<DownloadHandle, SharedPtr<TaskEntry>>`；句柄为 `shared_ptr` 引用计数包装（`DownloadHandle` 拷贝安全）。
- `SyncWait(handle, timeout)`：条件变量等待终态；超时返回 `Timeout`（任务继续运行，不取消）。

### 24.6.2 `.part` 文件协议

- 命名：`<target_path>.part`。
- 断点 = `.part` 当前大小；请求 `Range: bytes=<size>-`。
- 服务端响应：
  - `206 Partial Content`：从 `size` 续写。
  - `200 OK`：丢弃 `.part`，从头写（`size` 不可信）。
  - `416 Range Not Satisfiable`：`.part` 完整 → 直接进校验。
- 完成且校验通过：`IO::WriteFileAtomic` 的 rename 语义（`.part` → target）。
- 取消：默认删除 `.part`（`keep_part_on_cancel` 可选保留）。

### 24.6.3 镜像切换算法（伪代码）

```
func DownloadWithMirrors(task, on_progress):
    for url in [task.url] + task.mirrors:
        if ApplyMirrorPrefix(url) is mapped: url = mapped
        err = DownloadOnce(url, task, on_progress)
        if err == OK:
            if Verify(task.expected_sha1, task.target_path) == OK: return OK
            else: Remove(task.target_path + ".part"); continue   # 镜像内容损坏
        if err in {Cancelled, Security}: return err              # 不可重试类
    return Err(DownloadFailed or ChecksumMismatch)
```

### 24.6.4 与缓存协作

- `use_cache=true`：先 `Cache::Get(key)`；命中（校验通过）→ 直接 `Completed`（拷贝/硬链接到 target，第一版用拷贝保证目标独立）。
- 写入缓存：校验通过后 `Cache::Put(key, target_path)`（内容寻址，重复下载复用）。

## 24.7 Auth 实现规范

### 24.7.1 微软设备码状态机

```
Init ──请求 devicecode──► AwaitUser ──(auth_device_code_ready 事件)──► Polling
  │                                                                     │ 轮询 token
  │                                                                     ├──授权成功──► Xsts ──► MinecraftLogin ──► FetchProfile ──► Done
  │                                                                     ├──authorization_pending──► 继续轮询
  │                                                                     ├──slow_down──► 加倍间隔
  │                                                                     ├──expired_token/授权超时──► Failed(AuthExpired)
  │                                                                     └──其他错误──► Failed(AuthFailed)
```

- 轮询间隔：服务端 `interval`（默认 5s），`slow_down` 后 ×2；最大等待 = `devicecode.expires_in`（默认 900s）。
- 状态持久化：`device_code` + 轮询状态可写入 `runtime/auth_flow.json`，宿主重启后可继续（`AuthManager::ResumeDeviceFlow()`）。
- 客户端断言：`client_id` 常量、`scope = XboxLive.signin offline_access`。

### 24.7.2 令牌生命周期

| 令牌 | 来源 | 有效期 | 刷新方式 |
| :--- | :--- | :--- | :--- |
| OAuth access | devicecode/token | ~1h | `refresh_token`（~90d） |
| XSTS | xsts/authorize | ~几小时 | 由 OAuth 令牌重换 |
| MC access | login_with_xbox | 24h | 由 XSTS 重换 |

- 启动前策略：MC access 剩余 < 10min → 用 refresh_token 全链刷新；OAuth refresh 过期 → `AuthExpired` 事件，宿主引导重新登录。
- 并发刷新保护：`AuthManager` 内每账户单飞（in-flight 去重），避免多 LaunchController 并发刷新。

### 24.7.3 错误矩阵（OAuth token 端点）

| 错误码 | 动作 |
| :--- | :--- |
| `authorization_pending` | 继续轮询 |
| `slow_down` | 间隔 ×2 后继续 |
| `expired_token` | 终止 → `AuthExpired` |
| `invalid_grant` / `unauthorized_client` | 终止 → `AuthFailed`（消息含可操作建议） |
| 网络/超时 | 重试（退避），不超过 3 次 |

## 24.8 Version 实现规范

### 24.8.1 解析算法（伪代码）

```
func ResolveVersion(manifest, version_id, loader, platform):
    entry = manifest.Find(version_id)              # 失败 → VersionNotFound
    profile = ParseVersionJson(FetchJson(entry.url))   # 失败 → VersionResolveFailed
    if loader != Vanilla: LoaderPatcher[loader].Patch(profile)
    recipe = BuildRecipe(profile, platform)
    recipe.classpath  = Libraries.Resolve(profile.libraries, platform)   # 规则求值 + 路径
    recipe.natives    = Natives.Extract(profile.libraries, platform)     # 下载 + 解压到 natives 目录
    recipe.game_args  = MergeGameArgs(profile)     # 见 24.8.3
    recipe.jvm_args   = MergeJvmArgs(profile)
    return recipe
```

- manifest 缓存：`cache/version_manifest.json` + 元信息（`fetched_at`）；刷新失败用缓存（`Warn`）。
- `version.json` 磁盘缓存：`cache/versions/<id>/<id>.json`；sha1 校验（manifest 提供时）。
- 解析器对未知字段：忽略（前向兼容）；对已知字段类型错误：`InvalidFormat`（字段路径入错误消息）。

### 24.8.2 字段映射表（version.json → VersionProfile）

| Mojang 字段 | 映射 | 缺失行为 |
| :--- | :--- | :--- |
| `id` | `VersionProfile.id` | 必填，缺失 → InvalidFormat |
| `type` | `type` | 默认 `release` |
| `mainClass` | `main_class` | 必填 |
| `assets` | `assets` | 默认 `legacy` |
| `assetIndex` | `asset_index` | 使用 `assets` 字段 + legacy 回退 |
| `libraries[]` | `libraries` | 空数组 |
| `arguments.game` / `minecraftArguments` | `game_args` / `legacy_minecraft_arguments` | 空 |
| `arguments.jvm` | `jvm_args` | 空 |
| `logging.client.file.url/sha1` | `logging_client_url/sha1` | 无日志配置 |
| `javaVersion.major` | `java_version.major` | 无要求 |
| `downloads.client` | （recipe 组装用） | 客户端 jar 缺失 → VersionResolveFailed |

### 24.8.3 参数合并算法

```
MergeGameArgs(profile):
    if profile.legacy_minecraft_arguments:
        args = Split(legacy_minecraft_arguments)      # 按空格分词（支持引号）
    else:
        args = [a for a in arguments.game if RulePass(a.rules)]
    # 占位符替换（启动前，见 24.12 注入表）
    args = SubstitutePlaceholders(args, session, profile)
    return args

MergeJvmArgs(profile):
    args = [a for a in arguments.jvm if RulePass(a.rules)]
    args = Deduplicate(args)                          # 同名参数后者覆盖，保留顺序
    return args
```

- 分词规则：空格分隔、双引号成组、反斜杠转义（与 Windows 命令行兼容，跨平台一致实现于 `launcher.base` 工具 `details::SplitArgs`）。
- 占位符：`${auth_player_name}`、`${auth_uuid}`、`${auth_access_token}`、`${version_name}`、`${game_directory}`、`${assets_root}`、`${assets_index_name}`、`${launcher_name}`、`${launcher_version}`、`${natives_directory}`、`${classpath}`、`${resolution_width/height}`、`${user_type}`、`${version_type}`。
- 未识别占位符：保留原样（不报错，兼容未来版本）；空值占位符：替换为空串并记 `Warn`（除 `${classpath}` 必填，空 → `InvalidState`）。


## 24.9 Assets 实现规范

### 24.9.1 对象布局

```
<runtime>/assets/
 ├── indexes/<index_id>.json            # 索引缓存（校验 sha1）
 └── objects/<hash[:2]>/<hash>          # 对象内容寻址
     （legacy 虚拟布局：assets/virtual/legacy/<namespace>/<path> → 对象链接/拷贝）
```

- 索引结构：`{ "objects": { "<path>": { "hash": "<sha1>", "size": n } }, "virtual": bool }`。
- `virtual=true`（legacy 索引）才生成 `virtual/legacy` 布局；`virtual=false` 直接用 `--assetsDir assets --assetIndex <id>`。

### 24.9.2 缺失计算与下载

```
MissingObjects(profile):
    index = LoadIndex(profile.asset_index)            # 缓存未命中则下载 + 校验
    missing = []
    for path, obj in index.objects:
        file = objects/<hash[:2]>/<hash>
        if not exists(file) or Sha1(file) != hash: missing.append(obj)
    return missing
```

- 下载并发：`runtime.download_concurrency` 上限；按 `size` 降序优先（大文件先开）。
- 进度事件：`download_progress` 携带 `task_id = "asset:<hash>"`；总量 = 缺失对象 size 之和（节流 100ms）。

### 24.9.3 legacy 虚拟布局算法

```
BuildLegacyVirtual(profile):
    if not index.virtual: return
    for path, obj in index.objects:
        src  = objects/<hash[:2]>/<hash>
        dest = virtual/legacy/<path>
        if exists(dest) and Size(dest) == obj.size: continue
        POSIX: symlink(dest → src)                     # Windows: CopyFile（避免权限/兼容问题）
```

- Windows 不建符号链接（默认权限限制），用硬链接优先、失败回退拷贝。

## 24.10 JVM 实现规范

### 24.10.1 定位算法（伪代码）

```
Locate(profile):
    candidates = []
    if profile.java_path: candidates += [profile.java_path]
    candidates += [runtime/jre, JAVA_HOME, PATH java]
    for c in candidates:                                # 去重（realpath）
        if IsJavaExecutable(c):
            info = Probe(c)
            if Satisfies(info, recipe.java_version): return info
            last_incompatible = info
    return Err(JvmIncompatible, 需要 X 找到 Y)
```

- `IsJavaExecutable`：文件存在 + 可执行位（POSIX）/ `.exe` 或 `java`（Windows）+ 文件名匹配 `java`。
- 探测缓存：`(realpath, mtime, size)` → `JvmInfo`，存 `details::jvm_cache_`；缓存失效 = mtime/size 变化。
- 版本判定：`java.specification.version`（`1.8` → major 8；`17` → 17）。

### 24.10.2 参数生成（伪代码）

```
BuildJvmArgs(recipe, profile, jvm):
    args = ["-Xms" + profile.min_memory_mb + "M", "-Xmx" + profile.max_memory_mb + "M"]
    args += ["-Djava.library.path=" + recipe.natives_directory]
    args += ["-cp", JoinClasspath(recipe.classpath)]    # Windows ';' / POSIX ':'
    if recipe.logging_config: args += ["-Dlog4j.configurationFile=" + recipe.logging_config]
    if jvm.major >= 17: args += DefaultAddOpens()        # 版本相关 add-opens（见 24.13 表）
    if authlib injector: args += ["-javaagent:" + injector + "=" + base_url]
    args += recipe.jvm_args                             # 版本 JSON 提供的 JVM 参数
    args += profile.jvm_args_override                   # 用户覆盖（末尾优先）
    return args
```

- 版本差异内置表（可被 loader/用户覆盖）：

| MC 版本段 | 默认 JVM 调整 |
| :--- | :--- |
| ≤ 1.12.2（Forge） | `-XX:PermSize=256m`（Java 8）、`-Dfml.ignoreInvalidMinecraftCertificates=true`（loader 注入） |
| 1.17+ | 无特殊默认 |
| 1.20.5+ | 遵循 `javaVersion.major`；组件名仅作提示，不自动下载 JRE（第一版） |

## 24.11 Process 实现规范

### 24.11.1 平台后端

| 平台 | 创建 | 管道 | 终止 |
| :--- | :--- | :--- | :--- |
| Windows | `CreateProcessW` | `CreatePipe` + 读取线程 | `TerminateProcess`；`StopGracefully` 先 `GenerateConsoleCtrlEvent`（同进程组） |
| POSIX | `posix_spawn`（`POSIX_SPAWN_USEVFORK` 视平台） | `pipe2(O_CLOEXEC)` | `SIGTERM` → 超时 → `SIGKILL` |

### 24.11.2 Windows 参数引用算法

```
QuoteArg(arg):
    if arg 不含空格/制表/引号/反斜杠末尾: return arg
    out = "\"" + arg 逐字符处理：
        '\' 串 → 若后随 '"' 则翻倍，否则原样
        '"' → "\\\""
        '"' 后处理：末尾 '\' 串翻倍
    return out + "\""
```

- 命令行拼接：`executable`（引用）+ 空格 + 各 arg 引用；长度上限 32767（Windows）。
- 环境块：UTF-16 `KEY=VALUE\0...\0\0`，排序后拼接（Windows 要求排序）。

### 24.11.3 输出捕获与解码

- 读取线程：`ReadFile` 循环 → 行缓冲（`\n` 分界）→ 解码（Windows：控制台代码页/GBK 探测失败按 UTF-8；POSIX：UTF-8 直接）→ `game_output` 事件 + 环形缓冲（`deque<String>`，上限 1024 行）。
- 解码失败字节：替换 `U+FFFD`，不中断流。
- 退出码：`Wait()` 返回；进程被信号杀死（POSIX）→ 退出码 `128+signum`；Windows 异常终止 → 原退出码。

## 24.12 Launch 实现规范

### 24.12.1 状态转移表

| 当前态 | 事件/条件 | 下一态 | 动作 |
| :--- | :--- | :--- | :--- |
| Idle | `Launch()` | ResolvingVersion | 校验 profile（版本/认证/目录） |
| ResolvingVersion | 解析完成 | PreparingAssets | 发 `launch_progress(10%)`；缓存 recipe |
| ResolvingVersion | 错误 | Failed | 发 `launch_failed` |
| PreparingAssets | 资源就绪 | PreparingJvm | 发 `launch_progress(40%)` |
| PreparingJvm | JVM 就绪 | GeneratingArguments | 发 `launch_progress(60%)` |
| GeneratingArguments | 参数组装完成 | Spawning | 认证注入 + 占位符替换 |
| Spawning | `Process::Spawn` 成功 | Running | 发 `launch_progress(80%)` + `process_spawned` |
| Running | 子进程退出 | Exited | 发 `game_exit(code)`；写诊断 |
| 任意态 | `Cancel()` | Failed(Cancelled) | 取消下载/终止子进程 |

- 状态迁移全部在"状态线程"串行执行（第一版：LaunchController 内部专用状态机，每次迁移经 Scheduler 投递，保证无并发迁移）。
- `LaunchController` 句柄在 `Exited/Failed` 后保持可查询（phase、exit_code、recipe），由宿主释放。

### 24.12.2 认证注入表（占位符 → 实际值）

| 占位符 | 来源 |
| :--- | :--- |
| `${auth_player_name}` | session.username / display_name |
| `${auth_uuid}` | session.uuid（无横线） |
| `${auth_access_token}` | session.access_token |
| `${user_type}` | msa / legacy / offline（微软/第三方=msa，离线=legacy） |
| `${version_type}` | release / snapshot / old_beta / old_alpha（+loader 后缀） |
| `${assets_root}` | `<runtime>/assets` |
| `${game_directory}` | profile.game_directory |

- 第三方皮肤站附加：`-Dminecraft.api.auth.host={base}/authserver` 等（见 9.4）+ `-javaagent` 注入到 JVM 参数。

## 24.13 Loader 实现规范

### 24.13.1 修补算法（伪代码）

```
PatchFabric(profile, lp):
    meta = Fetch(fabric-meta, loader=lp.loader_version, mc=profile.id)
    profile.libraries += [fabric-loader, intermediary]           # 按 meta 坐标
    profile.main_class = "net.fabricmc.loader.impl.launch.knot.KnotClient"
    profile.jvm_args  += ["-Dfabric.skipMcProvider=true"（如 meta 要求）]

PatchForgeLegacy(profile, lp):      # MC ≤ 1.12.2
    # 依赖 launcherwrapper + forge universal；主类 launchwrapper.Launch
    profile.main_class = "net.minecraft.launchwrapper.Launch"
    profile.game_args += ["--tweakClass", "net.minecraftforge.fml.common.launcher.FMLTweaker"]

PatchForgeModern(profile, lp):      # MC ≥ 1.13
    dir = cache/loader/forge/<mc>/<ver>/
    if 无安装产物: RunInstaller(dir)             # java -jar forge-installer.jar --installClient
    merge = LoadJson(dir/version.json)
    profile.libraries += merge.libraries; profile.main_class = merge.mainClass
```

- `RunInstaller` 产物缓存协议：`cache/loader/<kind>/<mc>/<loader_version>/{version.json, libraries/}`；再次启动命中缓存跳过安装器（校验目录标记文件 `installer.ok`）。
- 版本匹配：loader 元数据声明支持的 MC 范围；超出 → `Err(Unsupported, "Forge 不支持 MC X，可用范围 Y")`。

## 24.14 QuickJS 实现规范

### 24.14.1 运行时隔离

```cpp
class QuickJS {
    struct PluginRuntime {
        JSRuntime *rt; JSContext *ctx;            // 每插件独立
        std::atomic<bool> interrupt_flag;         // 超时中断
        u64 call_seq;
    };
};
```

- 创建：`JS_NewRuntime` + `JS_SetMemoryLimit(limits.max_memory)` + `JS_SetInterruptHandler(rt, InterruptCB, &interrupt_flag)` + `JS_SetMaxStackSize`（默认 512KB）。
- 执行超时：调用前置 `interrupt_flag=false`；`aurora-js-<plugin>` 线程的"看门狗"（timer）在 `max_execution_ms` 后置位；中断回调返回 `!=0` 终止执行。
- 模块加载：自定义 loader 只接受插件目录相对路径（`SafeResolve(plugin_dir, path)`），其余拒绝。

### 24.14.2 异步桥协议

```
JS: aurora.http.get(url).then(...)
  ──► 桥函数（JS 线程）发起内核异步请求（Network::GetAsync → Scheduler）
      返回 JS Promise（pending）
  ──► 内核完成 → 投递"完成事件"到 JS 插件线程的消息队列
  ──► JS 线程事件循环（每 5ms 检查队列）：取结果 → Promise resolve/reject
```

- 桥函数必须**不阻塞 JS 线程**（除短同步操作如 `random.bytes`）。
- Promise 结果值转换：`u64` → 字符串（避免精度丢失）；`Bytes` → `ArrayBuffer`（拷贝）。
- 桥函数调用内核返回 `Result`：`Ok` → resolve（C 结构 → JS 对象）；`Err` → reject（`{code, category, message}`）。

### 24.14.3 沙箱关闭清单

- 不注册：`std`、`os` 全局（quickjs-ng libc 扩展裁剪）。
- 不暴露：`print` 之外的调试输出（`print` 定向到 `Logger::Debug`，前缀插件名）。
- 定时器：不暴露 `setTimeout`（宿主侧用 `aurora.scheduler.delay` 代替，受权限控制）。

## 24.15 Cache 实现规范

### 24.15.1 键规范化

```
NormalizeKey(key):
    if key 是 URL: 小写 host、去默认端口、去尾斜杠、query 参数排序后拼接
    else: 原样（内容哈希键已是规范形式）
```

- 键 → 路径：`cache/<sha1(key)[:2]>/<sha1(key)>`；sha1 用 `Crypto::Sha1`。
- 元数据：旁路 `.meta.json`（`{ key, sha1?, size, cached_at }`），损坏时按"未命中"处理并删除。

### 24.15.2 并发安全

- 索引：`unordered_map<key, Entry>` + `std::mutex`；磁盘操作在锁外执行（索引锁只保护表）。
- 同键并发 `Put`：幂等（同内容覆盖）；`Get` 期间 `Put` 同键：以磁盘校验结果为准。
- 清理：`Clear()` 删除整个 `cache/` 子目录后重建；运行中清理与下载并发 → 下载目标已被删则重新下载（失败重试一次）。

## 24.16 Crypto / IO / Archive 实现规范

### 24.16.1 Crypto

- 一次性哈希：`EVP_Digest`（SHA1/SHA256/SHA512/MD5）；文件哈希：`EVP_DigestInit/Update/Final` 流式（64KB 缓冲）。
- AES-GCM：`EVP_EncryptInit_ex(EVP_aes_256_gcm)` + AAD + tag 校验；失败返回 `Security` 分类错误。
- 错误处理：所有 OpenSSL 调用检查返回值；失败 `Err(Security/InternalError)` + `ERR_get_error` 数字并入消息。

### 24.16.2 IO

- `SafeResolve(root, rel)`（20.2 统一入口）：

```
SafeResolve(root, rel):
    p = (root / rel).lexically_normal()
    if p.string().starts_with(root.string()) or p == root: return p
    return Err(InvalidArgument, "路径逃逸")
```

- 原子写协议：写 `path + ".tmp-<pid>-<seq>"` → `fsync`（可选）→ `rename`；失败清理临时文件。
- 统一错误映射：`fs::error_code` → `ErrorCategory::IO`（`FileNotFound`/`PermissionDenied`/`IOError`）。

### 24.16.3 Archive

- 解压前预检：读中央目录 → 统计条目数/总解压大小/深度 → 超限即 `InvalidFormat`（20.4）。
- 逐条处理：`Zip-Slip` 检查 → 目录创建 → 流式写出（4MB 缓冲）→ CRC32 校验（minizip-ng 提供）。
- 错误中途：已解压文件保留（调用方决定清理），返回错误；`overwrite=false` 时目标存在 → 跳过（不视为错误）。


---

# 25. 编码与工程规范

> 本章补充 `DEVELOPMENT.md` 未覆盖的工程实践，面向 4–5 万行规模下的可维护性。命名/模块/格式规范以 `DEVELOPMENT.md` 为准，本章不重复。

## 25.1 代码组织

- **模块文件职责单一**：`.cppm` 只声明接口；实现 `.cc` 同模块名下；一个 `.cc` 文件职责内聚（>800 行必须拆分，如 `logger.cc` 按 `logger/logger_sink.cc` 拆分）。
- **include 边界**：C++ 模块内部禁止 `#include` 其他 `launcher` 模块头；跨模块一律 `import`。第三方头只在实现 `.cc` 中 `#include`（公共 API 不泄漏第三方类型，见 2.1-6）。
- **内部符号**：不导出实现细节——`details` 命名空间、`static` 文件内符号；`export` 仅用于公共接口。
- **前向声明优先**：接口中用引用/指针时优先前向声明，减少模块间编译耦合。
- **目录对应**：`src/<layer>/<module>/` 目录结构与模块名一一对应（见附录 C）。

## 25.2 注释与文档规范（LLVM 风格）

- **公共 API 注释**（`.cppm` 中每个导出实体）必须包含：语义说明、前置条件、后置条件、错误条件（错误码）、线程安全级别（18.2）、示例（复杂接口）。
- **不变量注释**：数据结构/状态机的不变量用 `// Invariant:` 标注；实现中破坏不变量处必须断言。
- **禁止**：无意义注释（`// increment i`）、注释掉的代码（用 git 历史）、与代码矛盾的注释。
- **模块头注释**：`.cppm` 顶部保留 SPDX + 版权头（现状一致，`LICENSE` MIT）。
- **提交注释**：遵循 `DEVELOPMENT.md` Conventional Commits；破坏性变更必须带 `BREAKING CHANGE:` footer。

## 25.3 错误处理工程实践

- **错误消息规范**：`<模块>:<动作> 失败：<原因>`，可操作（给出建议，如"检查网络或配置镜像"）；禁止仅"内部错误"而无上下文。
- **错误包装**：跨层包装必须保留 `cause`（错误链，见 23.3）；同层透传不包装。
- **Result 使用检查清单**：
  - 所有可能失败的函数返回 `Result<T>`（禁止 `bool` + out 参数的模糊语义）。
  - 纯查询函数（不失败）用值返回 + `noexcept`。
  - `[[nodiscard]]` 全覆盖（编译器强制 + CI `-Werror` 兜底）。
  - 忽略错误必须显式 `Ignore()`（自定义工具函数，日志 `Warn`），禁止空语句。
- **异常边界**：每个 Service 封装层入口 try/catch（见 3.2）；`Context` 顶层与 C ABI 入口兜底 catch。

## 25.4 代码审查清单

提交 PR 时逐项核对（模板放 `.github/pull_request_template.md`）：

1. 模块文件在 `src/CMakeLists.txt` 按依赖序排列？
2. 新 API 有文档注释（语义/前置/后置/错误/线程级别）？
3. 新 API 带 `[[nodiscard]]`、`noexcept`（如适用）、`const`（如适用）？
4. 错误路径全部返回 `Result`，无异常泄漏？
5. 无裸 `new/delete`、`malloc/free`；资源有 RAII/释放路径？
6. 路径操作走 `SafeResolve`；URL 校验规则通过？
7. 日志无凭据（Redact 工具使用）？
8. 线程安全级别标注；新锁符合 18.3 顺序？
9. 测试覆盖：新增/修改路径至少一个用例？
10. 无第三方 API 泄漏到公共接口？
11. 格式化：`clang-format` 通过；无告警（`-Werror`）？
12. 性能热路径无明显拷贝/锁（21.2）？

## 25.5 发布流程

- 版本：SemVer（`0.x` 阶段：小版本可含破坏性变更，但须文档记录；`1.0` 后严格兼容）。
- 发布清单：更新 `CHANGELOG.md`（新增/变更/破坏性）、`BuildInfo`、C ABI 头文件版本宏、`docs/DESIGN.md` 状态戳。
- 标签：`v<major>.<minor>.<patch>` + GPG 签名（遵循 DEVELOPMENT.md）。
- 发布产物：`AuroraCore` 动态库 + `include/` 头文件族 + 绑定示例；`libAuroraCore` 命名（`lib` 前缀 POSIX / `AuroraCore.dll` Windows）。

---

# 26. 兼容性与演进策略

## 26.1 C++ API 兼容策略

- **不承诺 ABI**：`AuroraCore` 为共享库，C++ 宿主必须与库同编译器同标准编译（文档明确）；`0.x` 阶段 API 可演进。
- **API 演进规则**：
  - 加东西（新类/方法/枚举值/默认参数）→ 小版本。
  - 删/改名/改签名 → 大版本（`0.x` 内允许但必须 `BREAKING CHANGE` 记录 + 迁移说明）。
  - 枚举**只追加不重排**（值稳定，利于 C ABI 映射）。
- **模块演进**：新增子模块不破坏父模块导出；父模块仅作汇总导出（`export import`），避免循环依赖（DEVELOPMENT.md 约束）。

## 26.2 C ABI 兼容策略

- **稳定承诺从 1.0 开始**：`0.x` 阶段也尽量追加式演进。
- 规则：
  - 只追加：新函数、新枚举值（追加在末尾）、新结构体字段（追加在末尾）。
  - 禁止：修改既有签名、重排结构体字段、改变函数语义。
  - 头文件自包含：每个 `au_*.h` 可独立 include，含必要类型（`au_types.h` 兜底）。
  - 版本查询：`au_version()` 返回字符串；`AU_VERSION_*` 宏供编译期判断。
- **绑定层独立仓库**（未来）：Rust crate / Python wheel / C# 包各自版本化，仅依赖 C ABI 头。

## 26.3 配置向后兼容

- 解析未知字段：忽略（保留在内存但不用，写回时丢弃——避免"读旧写新"污染）。
- 字段改名：`v0` 读兼容映射表（如 `worker_threads` → `runtime.worker_threads` 的旧路径读取）；写出一律新格式。
- 类型变更：宽松解析（数字可作字符串读）；写回新格式。
- 配置版本：文件头可选 `"version": 1`；无版本按 v1 解析。

## 26.4 数据格式版本化

| 文件 | 版本字段 | 迁移策略 |
| :--- | :--- | :--- |
| `config.json` | `version`（可选） | 宽松解析 |
| `accounts.json` | `version`（必填） | 低版本自动迁移（加密字段格式变更时） |
| `cache/**/.meta.json` | `version` | 不匹配 → 视为未命中 |
| `runtime/diagnostics/*.json` | 无（一次性） | 不迁移 |
| `auth_flow.json` | `version` | 过期（> expires_in）即丢弃 |

# 附录 A 错误码与错误分类总表

`launcher::ErrorCode`（枚举值已按序编号，**只追加不修改**，与 C ABI `au_error_code` 一一对应）：

| 值 | 名称 | 分类 | 说明 |
| :--- | :--- | :--- | :--- |
| 0 | Ok | - | 成功 |
| 1 | InvalidArgument | 通用 | 参数非法 |
| 2 | InvalidState | 通用 | 状态非法 |
| 3 | Unsupported | 通用 | 不支持的操作/平台/组合 |
| 4 | IOError | IO | IO 失败 |
| 5 | FileNotFound | IO | 文件不存在 |
| 6 | PermissionDenied | IO/系统 | 权限不足 |
| 7 | NetworkError | 网络 | 网络失败 |
| 8 | Timeout | 网络 | 超时 |
| 9 | ConnectionFailed | 网络 | 连接失败 |
| 10 | ParseError | 解析 | 解析失败 |
| 11 | InvalidFormat | 解析 | 格式非法 |
| 12 | DownloadFailed | 下载 | 下载失败（镜像耗尽） |
| 13 | ChecksumMismatch | 下载 | 校验和不匹配 |
| 14 | InternalError | 运行时 | 内部错误 |
| 15 | Unknown | 运行时 | 未知错误 |
| 16 | Cancelled | 通用 | 操作被取消 |
| 17 | JvmNotFound | Java | 未找到 JRE |
| 18 | JvmIncompatible | Java | JRE 版本不满足要求 |
| 19 | ProcessCreateFailed | 启动 | 进程创建失败 |
| 20 | ProcessLaunchFailed | 启动 | 启动后立即退出/异常 |
| 21 | VersionNotFound | Minecraft | 版本不存在 |
| 22 | VersionResolveFailed | Minecraft | 版本解析失败 |
| 23 | AssetMissing | Minecraft | 资源缺失且无法获取 |
| 24 | AuthFailed | 认证 | 认证失败 |
| 25 | AuthExpired | 认证 | 令牌过期 |
| 26 | PluginLoadFailed | 插件 | 插件加载失败 |
| 27 | PluginError | 插件 | 插件运行错误 |
| 28 | ScriptError | 脚本 | JS 执行错误/超时/权限拒绝 |

> 现状代码仅含 0–15；16–28 为本设计新增，实现时按表追加。

## A.2 错误分类总表

`launcher::ErrorCategory`（与 C ABI `au_error_category` 一一对应）：

| 值 | 名称 | 说明 |
| :--- | :--- | :--- |
| 0 | None | 未分类 |
| 1 | System | 系统错误 |
| 2 | Parse | 解析类 |
| 3 | IO | 输入输出 |
| 4 | Network | 网络 |
| 5 | Security | 安全/TLS |
| 6 | Config | 配置 |
| 7 | Runtime | 运行时 |
| 8 | Minecraft | MC 业务 |
| 9 | Auth | 认证 |
| 10 | Plugin | 插件 |
| 11 | Script | JS 脚本 |

# 附录 B JS 桥接 API 参考

权限名 → `aurora.*` 函数映射（`manifest.json` 中声明；函数返回 Promise 或同步值）：

| 权限 | JS API | 说明 |
| :--- | :--- | :--- |
| `log` | `aurora.log.trace/debug/info/warn/error(msg)` | 日志 |
| `event` | `aurora.event.on(name, handler)` / `aurora.event.off(name, handler)` | 订阅内核事件（名称 = 事件表键） |
| `config` | `aurora.config.get()` | 只读配置视图 |
| `http.get` | `aurora.http.get(url, opts?) → Promise<{status, headers, body}>` | GET |
| `http.post` | `aurora.http.post(url, body, opts?) → Promise<...>` | POST |
| `fs.read` | `aurora.fs.read(path) → Promise<string>` | 插件沙箱内只读（路径限制在插件目录与 runtime 白名单） |
| `fs.write` | `aurora.fs.write(path, data)` | 沙箱内写（仅插件目录） |
| `fs.list` | `aurora.fs.list(path) → Promise<string[]>` | 列目录（沙箱内） |
| `download` | `aurora.download.submit({url, path, sha1?}) → Promise<{path}>` | 下载（目标限沙箱目录） |
| `auth` | `aurora.auth.session()` | 当前会话只读视图 |
| `launch.hook` | 钩子函数（见 7.3） | 启动钩子：`launchResolved/onGameOutput/onGameExit` |
| `random` | `aurora.uuid.v4()` / `aurora.random.bytes(n)` | 工具 |
| `scheduler` | `aurora.scheduler.delay(ms) → Promise` | 延时 |

# 附录 C 模块文件清单

（目标状态；✅=已实现 🟡=部分 ⬜=空壳/待实现，见 2.5 模块总览表）

```
src/launcher.base/
  launcher.base.types.cppm        launcher.base.error.cppm
  launcher.base.platform.cppm     launcher.base.build.cppm
  launcher.base.config.cppm       launcher.base.cppm
src/base/
  error.cc  config.cc  system.cc  build.cc  version.cc
src/aurora.service/
  aurora.service.random.cppm      aurora.service.uuid.cppm
  aurora.service.logger.cppm      aurora.service.io.cppm
  aurora.service.network.cppm     aurora.service.crypto.cppm
  aurora.service.download.cppm    aurora.service.archive.cppm
  aurora.service.cache.cppm       aurora.service.quickjs.cppm
  aurora.service.auth.cppm        aurora.service.cppm
src/aurora/service/
  logger.cc  logger_pattern.cc  logger_sink.cc
  random.cc  uuid.cc
src/aurora.core/
  aurora.core.task.cppm           aurora.core.pool.cppm
  aurora.core.scheduler.cppm      aurora.core.context.cppm
  aurora.core.event.cppm          aurora.core.cppm
src/aurora.runtime/
  aurora.runtime.version.cppm     aurora.runtime.assets.cppm
  aurora.runtime.jvm.cppm         aurora.runtime.process.cppm
  aurora.runtime.launch.cppm      aurora.runtime.loader.cppm
  aurora.runtime.plugin.cppm      aurora.runtime.cppm
src/capi/                         (新增)
  au_context.c  au_config.c  au_logger.c  au_auth.c
  au_download.c au_launch.c  au_event.c   au_version.c
tests/
  test_framework.h
  test_error/  test_config/  test_uuid/  test_random/
  test_archive/  test_version/  test_event/  test_task/
```

---

# 附录 D version.json 字段映射表（详细）

来源：Mojang 官方 `version.json` 结构（含 `1.8.9`、`1.12.2`、`1.16.5`、`1.21.4` 实测差异说明）。

| 字段路径 | 类型 | 映射目标 | 版本段 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| `id` | string | `VersionProfile.id` | 全部 | 版本 id |
| `type` | string | `type` | 全部 | `release/snapshot/old_beta/old_alpha` |
| `time` / `releaseTime` | string | （诊断） | 全部 | ISO8601 |
| `mainClass` | string | `main_class` | 全部 | 主类 |
| `assets` | string | `assets` | 1.7.2+ | asset index id |
| `assetIndex.id/url/sha1/size/totalSize` | object | `asset_index` | 1.7.10+ | 缺失时回退 legacy |
| `downloads.client.url/sha1/size` | object | recipe.client | 全部 | 客户端 jar |
| `downloads.server` | object | （预留） | 1.7+ | 服务端 jar |
| `downloads.client_mappings/server_mappings` | object | （预留） | 1.14.4+ | 混淆映射 |
| `libraries[]` | array | `libraries` | 全部 | 见下 |
| `libraries[].name` | string | `Library.name` | 全部 | Maven 坐标 |
| `libraries[].downloads.artifact.path/sha1/size/url` | object | `Library.path/sha1/size/url` | 全部 | 缺省按默认库源推导 |
| `libraries[].downloads.classifiers` | map | `Library.classifiers` | 全部 | 含 `natives-<os>` |
| `libraries[].natives.<os>` | map | `natives_classifier` | ≤1.16.5 | 旧式 natives 声明 |
| `libraries[].rules[]` | array | `Library.rules` | 全部 | 平台/特性规则 |
| `arguments.game[]` | array | `game_args` | 1.13+ | 元素可为 string 或 `{rules, value}` |
| `arguments.jvm[]` | array | `jvm_args` | 1.13+ | 同上 |
| `minecraftArguments` | string | `legacy_minecraft_arguments` | ≤1.12.2 | 空格分隔 + 占位符 |
| `logging.client.file.id/sha1/size/url` | object | `logging_client_*` | 1.7+ | log4j 配置 |
| `javaVersion.major` | int | `java_version.major` | 1.20.5+ | 最低 Java 大版本 |
| `javaVersion.component` | string | `java_version.component` | 1.20.5+ | 官方 JRE 组件名（仅提示） |
| `inheritsFrom` | string | （合并处理） | 快照/预览 | 继承另一 version.json（解析时递归合并一次，防环） |
| `jar` | string | （合并处理） | 快照 | 继承 jar 名 |

> 解析器对 `inheritsFrom`/`jar`：递归合并（深度 ≤ 8），合并规则 = 子覆盖父（libraries 追加、arguments 追加、scalar 子优先）。

---

# 附录 E 事件结构体定义

（`aurora.core:event` 与各模块导出；命名 `<领域>Event`）

```cpp
// 生命周期
export struct ContextInitializedEvent {};
export struct ContextShuttingDownEvent {};

// 下载（aurora.service:download）
export struct DownloadProgressEvent {
    String task_id; u64 downloaded; u64 total; f64 speed;
};
export struct DownloadCompletedEvent { String task_id; Path path; };
export struct DownloadFailedEvent { String task_id; SharedPtr<Error> error; };
export struct DownloadChecksumMismatchEvent {
    String task_id; String expected; String actual;
};

// 启动（aurora.runtime:launch）
export struct LaunchProgressEvent {
    LaunchPhase phase; u32 percent; String message;
};
export struct LaunchFailedEvent { SharedPtr<Error> error; };
export struct ProcessSpawnedEvent { u64 pid; };
export struct GameOutputEvent { String line; };
export struct GameExitEvent { i32 exit_code; bool crashed; };

// 认证（aurora.service:auth）
export struct AuthDeviceCodeReadyEvent { String user_code; String verification_uri; };
export struct AuthExpiredEvent { String account_uuid; };
export struct AuthRefreshedEvent { String account_uuid; };
export struct AuthFailedEvent { AuthService service; SharedPtr<Error> error; };

// 插件（aurora.runtime:plugin）
export struct PluginLoadedEvent { String plugin_name; };
export struct PluginUnloadedEvent { String plugin_name; };
export struct PluginErrorEvent { String plugin_name; SharedPtr<Error> error; };

// 版本（aurora.runtime:version）
export struct VersionManifestUpdatedEvent { Path manifest_path; };
export struct ConfigLoadedEvent {};   // 配置生效（载荷由 ConfigManager 只读视图承担）
```

---

# 附录 F API 索引

| 模块 | 类/接口 | 关键方法 |
| :--- | :--- | :--- |
| launcher.base:types | （别名） | `u8..u64 i8..i64 f32 f64 Byte Bytes Path String ...` |
| launcher.base:error | `Error` | `Code/Message/Category/Location/ToString` |
| | `Result<T,E>` | `Value/Error/HasValue/HasError/Map/AndThen/OrElse/IfValue/IfError` |
| | 工厂 | `Ok/Err/OkEmplace/ErrEmplace` |
| launcher.base:platform | （函数） | `CurrentPlatform/CurrentArchitecture/Is64Bit/IsLittleEndian/ToString` |
| | `Version` | `ToString/operator<=>` |
| launcher.base:build | `BuildInfo` | `Current()` |
| launcher.base:config | `Config/PathConfig/LoggerConfig/NetworkConfig/RuntimeConfig` | `Validate/Reset` |
| | `ConfigManager` | `Load/Get/Load(Path)/Save(Path)` |
| aurora.service:random | `Random` | `Bytes/UInt32/UInt64/Fill` |
| aurora.service:uuid | `UUID/UUIDGenerator` | `Parse/ToString/Bytes/Version/Variant`；`V4/V7` |
| aurora.service:logger | `Logger` | `Initialize/Shutdown/SetLevel/Trace..Critical` |
| aurora.service:crypto | `Crypto` | `Sha1/Sha256/Sha512/Md5/Sha1File/Base64/HmacSha256/AesGcm*` |
| aurora.service:io | `IO` | `ReadFile/WriteFileAtomic/ReadText/Mkdirs/RemoveAll/Exists/CopyFile/JsonRead/JsonWrite/SafeResolve` |
| aurora.service:network | `Network` | `Get/Post/PostJson/GetAsync/PostAsync/OpenStream` |
| aurora.service:download | `DownloadManager` | `Submit/Cancel/State/SyncWait/Await` |
| aurora.service:archive | `Archive` | `ExtractZip/CreateZip/ListZip/CompressGzip/DecompressGzip` |
| aurora.service:cache | `Cache` | `Get/Put/Contains/Remove/Clear/TotalSize` |
| aurora.service:quickjs | `QuickJS` | `Create/Destroy/Eval/LoadModule/RegisterFunction/Call/ToString` |
| aurora.service:auth | `AuthManager/AuthProvider/AuthSession` | `Accounts/ActiveAccount/Authenticate/RefreshActive/Persist` |
| aurora.core:task | `Task<T,E>` | `StartOn/SyncWait/Then/Catch/Done/Result`；`WhenAll/WhenAny/Async` |
| aurora.core:pool | `ThreadPool` | `Submit/Stop/Shutdown/WorkerCount/IsPoolThread` |
| aurora.core:scheduler | `Scheduler` | `Schedule/DispatchAfter/DispatchOn/Start/Stop` |
| aurora.core:context | `Context` | `Instance/Initialize/Shutdown/Create/SetActive/Get/Register/Events/Scheduler/Pool/Plugins/RegisterPlugin` |
| aurora.core:event | `EventDispatcher/EventSubscription` | `Subscribe/Publish/PublishAsync/Unsubscribe` |
| aurora.runtime:version | `VersionManifest/VersionResolver/VersionProfile/Library/Rule/LaunchRecipe` | `Refresh/Resolve` |
| aurora.runtime:assets | `AssetManager` | `EnsureIndex/EnsureAssets/MissingObjects/BuildLegacyVirtual/VerifyAll` |
| aurora.runtime:jvm | `JvmManager/JvmInfo` | `Locate/Probe/BuildJvmArgs` |
| aurora.runtime:process | `Process/ProcessOptions` | `Spawn/Wait/Kill/StopGracefully/WriteStdin/Stdout/Stderr` |
| aurora.runtime:launch | `LaunchController/LaunchProfile/LaunchPhase` | `Create/Launch/Cancel/Phase/Recipe/Process/ExitCode` |
| aurora.runtime:loader | `LoaderPatcher/LoaderRegistry/LoaderKind` | `Patch/Kind` |
| aurora.runtime:plugin | `PluginRegistry/IPlugin/ILaunchHook` | `LoadPlugins/UnloadPlugins/RegisterHook/UnregisterHook` |
| capi | `au_*` | 见 8.4（`au_context_*`、`au_launch_*`、`au_download_*`、`au_auth_*`、`au_event_*`） |

---

# 附录 G 参考文档

| 参考 | 用途 | 链接/来源 |
| :--- | :--- | :--- |
| Mojang Version Manifest | 版本清单协议 | `https://piston-meta.mojang.com/mc/game/version_manifest_v2.json` |
| Minecraft Launcher Protocol 文档 | version.json 字段语义（社区） | Minecraft Wiki / minecraft-launcher-lib 文档 |
| Microsoft OAuth Device Code Flow | 设备码流 | `https://learn.microsoft.com/zh-cn/entra/identity-platform/v2-oauth2-device-code` |
| Xbox Live / Minecraft 认证 API | XSTS 与 login_with_xbox | 社区逆向文档（mc-oauth / PrismLauncher 实现） |
| authlib-injector | 第三方皮肤站协议 | `https://github.com/yushijinhun/authlib-injector` |
| Fabric Meta API | Fabric 加载器元数据 | `https://meta.fabricmc.net/v2/versions/loader/` |
| Quilt Meta API | Quilt 加载器元数据 | `https://meta.quiltmc.org/v3/versions/loader/` |
| Forge / NeoForge 安装器 | 现代 Forge 修补 | 官方 maven / 安装器 |
| LLVM 文档风格 | 本文档结构参考 | `https://llvm.org/docs/`（Programmer's Manual、LangRef 的精确语义风格） |
| QuickJS / quickjs-ng | JS 引擎 API | `https://github.com/quickjs-ng/quickjs` |
| minizip-ng | 压缩解压 API | `https://github.com/zlib-ng/minizip-ng` |
| spdlog / fmt | 日志与格式化 | `https://github.com/gabime/spdlog` / `https://github.com/fmtlib/fmt` |
