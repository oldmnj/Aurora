# AuroraLauncherCore 开发规范文档

## 环境要求

### 编译器

本项目使用C++23标准，标准库使用WG21所规定的标准头文件，不得使用编译器扩展


- **clang:**  clang >= 18
- **gcc**: gcc >= 14

- **MSVC** >= Visual Stidio 2022 17.10+


### 构建工具

本项目使用CMake和Ninja构建工具

CMake >= 4.1.0

Ninja: 无具体要求，但是尽量使用较新的版本


## git规范

### 合并分支

合并分支前必须要提Pull Request且在二人审查并批准后才可合并分支


### commit


commit信息要求使用Conventional Commits风格，
即，message要求写：
```
<type>(<scope>): <subject>

<body>

<footer>

```
此类风格，
其中，<type>为提交类型，如：feat, fix, docs, style, test, ci, build等等
<scope>为影响范围，在此项目中可以为模块名，如: launcher.auth, c_abi, launcher.base等等
<subject>为描述，尽量做到50字/词以内
<body>为详细说明，比如原因，怎么实现等等
<footer>元信息，关联issue等等
然后，<subject>,<body>可以使用任何语言，如果使用有动词时态的语言，按照祈使句来写，切勿写成added等等

其中，<type>与<subject>为必须项

此外，如果可以，每次commit尽量使用gnupg签名并提交


### 分支

主分支为 'main' 分支，开发人员不得直接push到main分支，main分支提交只能通过提Pull Request来接受审查并合并，希望commit能呈线性，如果合并的该分支commit状态不佳建议使用 压缩合并[^1] commit状态良好使用 变基合并[^2]
此外，个人开发所负责的分支，命名应为 username/dev 或 username.dev


## 代码规范

### 命名规范

#### 类名，结构体命令: 使用大驼峰命名，如下

```cpp
class Logger {};
enum class LogLevel {};
struct PathConfig {};
```
等等

#### 函数命名: 函数命名同类名一样，使用大驼峰命名，如下

```cpp
void Initialize();
void Reset();
```
等等

#### 变量命名: 变量命名使用无驼峰＋下划线_ 命名，如下

```cpp
u32 worker_count; // 工作线程数
bool is_async; // 是否启用异步日志
```

其中，建议多以全小写字母＋下划线_ 命名，布尔类型变量可使用 [is_, has_, enable_, should_]前缀命名，静态变量同样使用如上命名，静态常量请使用k＋大驼峰命名，类的成员变量在此命名规范技术上，变量名最后还需下划线_ 

#### 文件命名
模块文件应以.cppm为后缀，且位于./modules/目录下，
实现文件若需以.cc为后缀，且必须位于./src/下，
非模块实现文件的cc文件，tests文件，examples文件等等都应以全小写字母＋下划线方式命名，不得使用大写字母和除下划线_外的特殊字符命名
模块文件及实现文件命名规范见下方 模块规范

#### namespace
命名空间使用全小写命名，尽量使用一个单词，如

```cpp
namespace launcher {}
namespace details {}
```
等等

#### 注意
命名时若要使用缩写请使用辨识度较高的缩写，如
dev, init, msg等等

#### 测试规范

测试文件需处于./tests/目录下
具体可以测试内容单独在./tests/目录下建子目录

### 模块规范
module命名: 
模块命名使用launcher.xxx命名，文件名为launcher.xxx.cppm
子模块命名使用launcher.xxx:xxxx命名，其中xxxx为子模块名，launcher.xxx是父模块名，子模块文件命名使用launcher.xxx.xxxx.cppm，xxxx为子模块名，launcher.xxx为父模块名，建议父模块文件仅作为导出子模块使用，否则可能容易导致模块间的循环依赖问题

此处需注意： 写模块的实现文件.cc时，文件头部声明模块名因使用父模块名module launcher.xxx;，不应当使用module launcher.xxx:xxxx;，这是由于C++模块的限制，使用launcher.xxx:xxxx会导致编译失败，原因是编译器对于C++Module的规则集不同，这样写可以有效避免编译失败排查不出原因

文件: 以.cppm结尾的模块声明接口文件，需位于./modules/目录下，且未在./modules/CMakeLists.txt文件target_sources()中的，应当按照模块依赖顺序排列模块文件，否则会导致编译失败，实现文件.cc，应当在src/xxx/下，子模块实现文件.cc可以在./src/xxx/xxxx/目录下，此处可以按子模块类名等等方式细分，或者亦可在./src/xxx/目录下，此时，此子模块不应有多个实现文件，仅可以有一个./src/xxx/xxxx.cc实现文件

### 代码风格规范
本项目使用LLVM代码规范
在项目根目录提供.clang-format文件，用于commit之前格式化代码


### 异常规范
本项目提供包装std::expected的类型Result<T>类型返回错误，具体定义可见./modules/launcher.base.error.cppm
本项目禁止异常作为函数接口，原因为为外部提供稳定的C_ABI，
模块内部异常必须转换为std::unexpected<T>{};类型返回，同时函数返回值类型也得为Result<T>
本项目允许最顶层catch，用以
- 防止未知异常致使程序崩溃
- 记录日志
- 安全退出


### 类型规范
本项目提供统一的基础类型供使用，具体可见./modules/launcher.base.types.cppm中的定义，如
```cpp
u32
u8
String
StringView
Path
Vector<T>
```
在实际编写过程中尽量避免直接std::vector之类以保持风格统一


### PR规范

要求：
- 描述修改内容
- 提供较为完整的测试文件
- 不允许破坏原有API

### C_ABI
此外，本项目仅作为启动器核心，并无GUI界面，本项目为来也将提供统一的接口供非C++语言使用，本项目未来将提供C_ABI风格函数供外部调用，其中，一切返回类型为非C的，将使用C风格指针返回，但是Error不能暴露外界，以后可以绑定语言









[^1]: merge

[^2]: rebase