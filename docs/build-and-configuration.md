# 构建与配置说明

## 1. 环境

- Windows 10/11；
- CMake 3.20 或更高；
- C++17 编译器；
- 已验证 preset：MSYS2 UCRT64 MinGW + Ninja；
- Visual Studio 2026 x64。

## 2. MinGW 构建

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --preset mingw-debug
```

若编译器不位于 `C:/msys64/ucrt64/bin/g++.exe`，修改
[`CMakePresets.json`](../CMakePresets.json) 中的 `CMAKE_CXX_COMPILER`，或建立个人
`CMakeUserPresets.json`，不要把个人路径提交为团队统一配置。

## 3. MSVC 构建

在 Visual Studio 2026 Developer PowerShell 中运行：

```powershell
cmake --preset msvc-debug
cmake --build --preset msvc-debug
ctest --preset msvc-debug
```

`msvc-debug` 使用 VS 附带的 Ninja。当前 preset 中的 VS 安装和 toolset 路径是本机验证值；
团队工程宜用 `CMakeUserPresets.json` 或统一 toolchain 文件消除机器路径差异。
本仓库还提供 [`tools/build-msvc.bat`](../tools/build-msvc.bat)，用于在普通终端初始化本机
VS 环境后依次配置、构建和测试。

公共 target 使用：

- `/W4`：较严格警告；
- `/permissive-`：提高标准一致性；
- `/Zc:__cplusplus`：正确报告语言版本。

若 CAD SDK 要求特定 runtime library、异常或 RTTI 设置，应在 SDK toolchain/preset 中统一，
不要由单个库偷偷覆盖。跨 DLL 边界尤其需要统一 CRT、架构、calling convention 和结构布局。

## 4. CMake 选项

| 选项 | 默认值 | 用途 |
|---|---:|---|
| `CADSTUDY_BUILD_EXAMPLES` | `ON` | 构建三个示例程序 |
| `CADSTUDY_BUILD_TESTS` | `ON` | 构建并注册 CTest |

## 5. 示例程序

```powershell
$env:TEMP\cad-class-design\mingw-debug\dynamic_library_example.exe
$env:TEMP\cad-class-design\mingw-debug\declarative_commands_example.exe
$env:TEMP\cad-class-design\mingw-debug\constraint_graph_example.exe
```

## 6. 配置文件语义

### `plugins.json`

- `schemaVersion`：配置 schema 版本，不是插件 ABI 版本；
- `minimumHostApi`：宿主能力下限；
- `loadPolicy`：装配策略，如 eager/on-demand；
- `requiredExports`：缺失即拒绝插件；
- `optionalExports`：缺失只关闭能力。

配置中的 calling convention 只用于验证/生成，不能在运行时安全地把错误签名修正为正确签名。
真实签名应由版本化头文件或 IDL 生成。

### `commands.json`

`handlerId` 是稳定逻辑 ID，不是 C++ 函数地址。加载后必须把它绑定到插件已提供的 handler，
并在原子提交 catalog 前验证重复名称、权限和事务属性。

### `entity_schema.json`

属性 `id` 用于持久化和代码生成，显示名称应位于本地化资源。`derived` 属性不能由通用属性面板
直接写入；它由领域逻辑计算。

## 7. 生产配置管线建议

```text
JSON/DSL
  → 带 source location 的解析树
  → schema 版本迁移
  → 强类型 declaration model
  → 交叉引用与领域验证
  → compiled catalog/graph
  → 原子发布给运行时
```

配置失败应保留文件、行列、字段路径和领域对象 ID。不要让运行时组件直接持有通用 JSON DOM；
这样会把 schema 知识扩散到全系统，也会让热路径重复解析字符串。

## 8. 动态库部署

- 系统组件使用 `SearchPolicy::system32`；
- 自研插件使用规范化绝对路径和
  `absolute_path_with_safe_dependencies`；
- `legacy_default` 仅用于明确记录风险的旧部署；
- 插件对象、回调、线程和函数指针全部释放后，模块才允许卸载；
- 插件更新应使用新路径/版本目录，避免覆盖仍被进程映射的 DLL。
