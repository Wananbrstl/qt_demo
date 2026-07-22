# `GetProcAddress` 封装：从语义推导设计

## 1. 先拆开四个不同概念

同一个“封装类”常被要求同时表示以下概念，迷茫由此产生：

1. DLL 路径或加载请求；
2. 已加载模块；
3. 模块中的符号请求；
4. 已解析、可调用且生命周期安全的函数。

它们不是同一种对象。构造函数应该做多少工作，取决于类型名字选择了哪一种语义。

| 类型语义 | 构造时保存名称 | 构造时解析 | 合理性 |
|---|---:|---:|---|
| `SymbolRequest` | 是 | 否 | 描述对象，允许之后选择模块/策略 |
| `LazyFunction` | 是 | 否 | 需要定义首次失败、重试和并发 |
| `ResolvedSymbol` | 可选，仅用于诊断 | 是 | 构造后保证地址非空 |
| `ImportedFunction<F>` | 可选 | 是 | 还保证类型化调用和模块存活 |

所以不存在“构造函数原则上应只保存名称”的规则。关键是类型承诺。如果类名和接口表示
已经可调用的函数，却只保存名称，它就在构造后违反了自身语义。

## 2. 推荐的生产方向

本项目采用两层类型：

```text
SharedLibrary ──shared state── ImportedFunction<FunctionPointer>
      │                              │
   HMODULE owner                  typed FARPROC
      └──────── last owner releases with FreeLibrary
```

创建协议如下：

```cpp
using GetCurrentProcessIdPointer = DWORD(WINAPI*)();

auto library = cadstudy::SharedLibrary::load(
    L"kernel32.dll",
    cadstudy::SharedLibrary::SearchPolicy::system32);

if (!library) {
  // 处理 LoadLibraryExW 错误
}

auto function = library.value().resolve<GetCurrentProcessIdPointer>(
    "GetCurrentProcessId");

if (!function) {
  // 处理 GetProcAddress 错误
}

DWORD process_id = function.value()();
```

完整代码位于：

- [`dynamic_library.h`](../include/cadstudy/dynamic_library.h)
- [`dynamic_library.cpp`](../src/dynamic_library.cpp)
- [`main.cpp`](../examples/dynamic_library/main.cpp)

## 3. 为什么使用命名工厂

加载失败是 CAD 插件生态中的正常环境错误：部署缺失、版本不匹配、安全策略拦截都可能发生。
C++17 构造函数不能返回结构化错误；而让调用者在构造后检查 `isValid()` 很容易遗漏。因此：

```cpp
static Result<SharedLibrary> load(path, policy);
```

表达的是“成功得到满足不变量的模块，或得到错误”。这与 LLVM Programmer's Manual 对
fallible constructor 的建议一致。在允许异常且缺失 DLL 被视为异常的项目中，直接构造并
抛异常同样能维护不变量；本样例选择 `Result<T>` 是错误策略，不是宣称异常错误。

## 4. 为什么导入函数持有模块状态

Microsoft 文档说明 `FreeLibrary` 会减少模块引用计数，计数为零时模块被卸载。卸载后继续
使用旧函数地址是悬空代码指针。单纯返回函数指针会把这个时间关系留给调用者记忆。

Boost.DLL 提供了两个很有价值的对照：

- `shared_library::get<T>()` 返回符号，但调用者必须自行保证 library 存活；
- `import_symbol<T>()` 返回的可调用对象会引用计数持有 library。

本项目选择第二种语义。它付出一次共享控制块和引用计数的成本，换取“可调用对象意味着
模块仍存活”的局部推理。如果 CAD 宿主规定插件永不卸载，则也可以像 LLVM 10
`DynamicLibrary::getPermanentLibrary()` 一样选择进程级生命周期，代价是无法热卸载，且测试
与资源回收语义不同。

## 5. eager 与 lazy 的选择

### 推荐 eager 的情形

- 符号是插件协议必需入口；
- 希望加载阶段一次报告全部兼容性错误；
- 函数几乎一定会使用；
- 对象对外声称自己是 callable；
- 不希望在首次交互命令中突然加载和失败。

### 可以 lazy 的情形

- 能力是昂贵且极少使用的可选功能；
- 缺少符号表示“功能不可用”，不是插件无效；
- 模块可能随运行环境变化而出现；
- 已定义线程安全、缓存失败、重试和卸载策略。

更清楚的模型通常是：加载插件时 eager 解析最小必需 ABI 查询入口；查询能力后，为可选能力
建立显式的 `OptionalCapability`，而不是让每个函数包装类都隐式 lazy。

## 6. ABI 与类型安全的真实边界

模板只能保证调用端使用同一个函数指针类型，无法证明 DLL 实际导出的函数确实具有该 ABI。
生产级 CAD 插件协议建议：

1. 导出 `extern "C"` 的版本化入口，例如 `cadPluginQueryV3`；
2. 明确调用约定；
3. 入口返回带 `size`、`abi_version` 的函数表；
4. 跨边界使用固定宽度整数、opaque handle 和 POD；
5. 内存由分配方释放，或提供成对的分配/释放函数；
6. 异常不得跨 DLL ABI 边界；
7. 可选函数以函数表尾部扩展或能力位表达。

示意：

```cpp
extern "C" {

struct CadPluginApiV3 {
  std::uint32_t size;
  std::uint32_t abi_version;
  void* plugin_context;
  int (*execute_command)(void*, const char* command_utf8) noexcept;
  void (*shutdown)(void*) noexcept;
};

using QueryPluginV3 = int (*)(std::uint32_t host_abi,
                              CadPluginApiV3* out_api) noexcept;

}
```

比起逐个按名字导入几十个 C++ 函数，单一版本化查询入口更容易原子验证、演进和诊断。

## 7. DLL 搜索安全

示例显式提供三个策略：

- `system32`：加载系统 DLL；
- `absolute_path_with_safe_dependencies`：要求主 DLL 使用绝对路径，并为依赖项启用安全目录；
- `legacy_default`：仅为兼容遗留部署显式选择。

默认搜索路径会影响安全性和可重复部署，不能藏在无名布尔参数中。生产系统还应规范插件根目录、
签名/哈希、允许列表以及依赖冲突隔离策略。

## 8. 对成熟库的结论

| 实现 | 主要语义 | 可借鉴点 | CAD 中的限制 |
|---|---|---|---|
| Win32 API | 原始模块句柄和地址 | 平台事实、搜索策略 | 所有权和类型关系靠调用者维护 |
| Boost.DLL | RAII module + typed import | import 可持有模块 | 引入 Boost，仍需自定插件 ABI |
| Qt `QLibrary` | 可设置名称、隐式 load、resolve | 易用、引用计数式卸载 | 对象可处多状态，依赖 Qt |
| POCO `SharedLibrary` | load/unload/has/getSymbol | 跨平台封装清晰 | 裸符号与 owner 关系仍需上层设计 |
| LLVM 10 `DynamicLibrary` | 永久加载、全局符号搜索 | 适合编译器插件和进程级注册 | 不适合作为可卸载 CAD 插件默认模型 |

结论不是某个库“最佳”，而是它们选择了不同生命周期和错误语义。应先选择 CAD 插件协议，
再选择或封装最匹配的库。
