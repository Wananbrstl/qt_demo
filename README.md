# 现代 CAD C++ 类设计研究

本仓库是一份面向 C++17、自研现代 CAD 框架的类设计研究及可运行示例。
它不把设计模式当成起点，而是从对象语义、不变量、所有权、错误、时间语义和
依赖关系推导类型与接口。

## 阅读顺序

1. [类设计决策框架](docs/class-design-decision-framework.md)
2. [`GetProcAddress` 案例](docs/get-proc-address-case-study.md)
3. [声明式编程与 CAD](docs/declarative-programming.md)
4. [LLVM 设计到 CAD 的映射](docs/llvm-to-cad.md)
5. [构建与配置](docs/build-and-configuration.md)
6. [证据与资料索引](docs/evidence-index.md)

## 示例覆盖范围

- `SharedLibrary`：拥有 DLL，并在最后一个导入函数销毁后卸载；
- `ImportedFunction<T*>`：构造完成即代表可调用且模块仍存活的函数；
- `Result<T>`：C++17 下的显式成功/失败通道；
- `CommandCatalog`：声明、验证、编译、执行四阶段分离；
- `ParameterGraph`：把参数规则声明编译为拓扑执行顺序，并提前拒绝环。

这些类型是研究样例，不是承诺稳定 ABI 的完整 CAD SDK。

## 快速构建

```powershell
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --preset mingw-debug
```

Visual Studio 2026 开发者命令行可改用 `msvc-debug` preset。详细说明见
[构建与配置](docs/build-and-configuration.md)。

Preset 把构建产物放在 `%TEMP%/cad-class-design/<preset>`，避免污染源码目录。
