# 证据与资料索引

本文档记录研究结论的来源类别。正文中的结论是对多个来源和 CAD 约束的综合，不是文献摘抄。
在线资料检索日期为 2026-07-22；涉及“当前版本”的描述以该日期页面为准。

## 1. 规范与官方指南

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)：
  P.1/P.3（表达意图）、C.40-C.42（不变量和构造）、R.1-R.5（资源和所有权）。指南是活文档；
  本次检索版本页面标记为 2026-06-14。
- [Microsoft `GetProcAddress`](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress)：
  模块句柄、符号名/ordinal 和失败语义。
- [Microsoft `FreeLibrary`](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary)：
  模块引用计数和卸载语义。
- [Microsoft Run-Time Dynamic Linking](https://learn.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking)：
  `LoadLibrary`、`GetProcAddress`、`FreeLibrary` 的基本协议。

## 2. LLVM 与 MLIR 官方资料

- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual.html)：
  `StringRef`、`Twine`、`function_ref`、`Error/Expected`、fallible constructor、容器选择。
- [LLVM TableGen Overview](https://llvm.org/docs/TableGen/) 与
  [Programmer's Reference](https://llvm.org/docs/TableGen/ProgRef.html)：
  声明式记录、类型系统和 backend。
- [Writing an LLVM New-PM Pass](https://llvm.org/docs/WritingAnLLVMNewPMPass.html)：
  concept-based pass、analysis 与 `PreservedAnalyses`。
- [MLIR Defining Dialects](https://mlir.llvm.org/docs/DefiningDialects/) 与
  [Operation Definition Specification](https://mlir.llvm.org/docs/DefiningDialects/Operations/)：
  用 TableGen 生成类、builder、验证和文档。
- [MLIR Declarative Rewrite Rules](https://mlir.llvm.org/docs/DeclarativeRewrites/)：
  DAG rewrite 的优势与公开限制。

## 3. 本地 LLVM 10 源码证据

本地根目录：`F:\llvm-master`，`CMakeLists.txt:18-25` 声明版本 10.0.0。

| 主题 | 本地位置 | 观察 |
|---|---|---|
| 动态库 | `include/llvm/Support/DynamicLibrary.h:27-117` | 永久加载、opaque handle、全局符号搜索 |
| Windows 实现 | `lib/Support/Windows/DynamicLibrary.inc` | 平台加载和符号解析边界 |
| 错误模型 | `include/llvm/Support/Error.h` | `Error`、`Expected<T>`、显式消费 |
| fallible constructor | `docs/ProgrammersManual.rst:835-873` | 命名工厂返回 `Expected<T>` |
| view 语义 | `docs/ProgrammersManual.rst:192-273` | `StringRef`/`Twine` 生命周期限制 |
| callback view | `docs/ProgrammersManual.rst:1055-1093` | `function_ref` 不应无证明地保存 |
| 容器决策 | `docs/ProgrammersManual.rst:1399-1470` | 先算法类别，再常数和缓存行为 |
| 注册表 | `include/llvm/Support/Registry.h:22-113` | 静态构造、全局链表、工厂条目 |
| Pass Manager | `include/llvm/IR/PassManager.h` | pass/analysis/失效关系 |
| arena | `include/llvm/Support/Allocator.h:141` | bump allocation 区域生命周期 |
| TableGen | `docs/TableGen/`、`lib/TableGen/` | 声明前端和多 backend |

## 4. 成熟开源库对照

- [Boost.DLL Getting Started](https://www.boost.org/doc/libs/latest/doc/html/boost_dll/getting_started.html)（检索时 latest 为 1.91.0）：
  `shared_library`、typed `get` 和 import。
- [Boost.DLL `import_symbol`](https://www.boost.org/doc/libs/1_77_0/doc/html/boost/dll/import_symbol.html)：
  导入对象引用计数持有库，防止符号先于模块失效。
- [Qt `QLibrary`](https://doc.qt.io/qt-6/qlibrary.html)（检索页面为 Qt 6.11.1）：
  显式/隐式加载、resolve、共享物理库和 unload 语义。
- [POCO `SharedLibrary`](https://docs.pocoproject.org/current/Poco.SharedLibrary.html)（检索页面为 1.15.3-all）：
  load/unload/hasSymbol/getSymbol 跨平台接口。
- [Abseil Tip #1: string_view](https://abseil.io/tips/1)、
  [Tip #101: Return Values, References, and Lifetimes](https://abseil.io/tips/101)、
  [Tip #180: Avoiding Dangling References](https://abseil.io/tips/180)：
  view 适合参数，保存 view 必须证明底层寿命。

## 5. 演讲

- Titus Winters, [Modern C++ Design, CppCon 2018](https://isocpp.org/blog/2019/09/cppcon-2018-modern-cpp-design-titus-winters)：
  现代参数传递、非拥有引用和 API 设计。
- Sean Parent, [Better Code: Relationships, CppCon 2019](https://isocpp.org/blog/2019/09/cppcon-2019-better-code-relationships-sean-parent)：
  把对象间关系当作主要设计对象。
- Andrei Alexandrescu,
  [Declarative Control Flow, CppCon 2015](https://www.youtube.com/watch?v=WjTrfoiB0MQ)：
  通过抽象表达控制意图；本文只借鉴“表达意图”的方向，不把 scope guard 等同于 CAD DSL。
- Klaus Iglberger,
  [Back to Basics: C++ Value Semantics, CppCon 2022](https://www.youtube.com/watch?v=G9MxNwUoSt0)：
  值语义、引用语义与现代标准库类型。
- Klaus Iglberger,
  [Designing Classes, CppCon 2021 slides](https://cppcon.digital-medium.co.uk/wp-content/uploads/2021/10/Designing-Classes-part-2.pdf)：
  类设计、不变量和语义讨论。

演讲用于补充设计语言和案例，不作为平台行为的唯一证据；平台事实优先采用 Microsoft/LLVM
等官方文档和源码。

## 6. 书籍

- Martin Reddy, *API Design for C++*, Morgan Kaufmann, 2011：API 契约、封装、版本和测试；
- Klaus Iglberger, *C++ Software Design*, O'Reilly Media, 2022：依赖管理、值语义、非侵入式设计、
  类型擦除和现代模式；
- John Lakos et al., *Large-Scale C++, Volume I*, Addison-Wesley, 2019：物理依赖、组件和大型系统；
- Herb Sutter & Andrei Alexandrescu, *C++ Coding Standards*, Addison-Wesley, 2004：资源、异常安全和
  接口规则；
- John Ousterhout, *A Philosophy of Software Design*, 2nd ed., Yaknyam Press, 2021：深模块、复杂度
  与信息隐藏。它不是 C++ 专著，但适合校验类边界是否真正隐藏复杂度。

## 7. 证据使用原则

1. Windows 行为以 Microsoft 文档和实测为准；
2. LLVM 结论注明本地 10.0.0 或当前文档，避免时代混淆；
3. 第三方库只证明某个取舍被成熟实现采用，不证明它适合所有 CAD；
4. 演讲和书籍提供设计框架，源码和测试用于校验具体行为；
5. CAD 映射均标为推导，应通过本项目的对象规模、插件生命周期和性能数据再验证。
