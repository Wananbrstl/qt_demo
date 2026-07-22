# 声明式编程与现代 CAD

## 1. 定义

声明式设计表达“系统中有哪些事实、关系和约束”，把遍历、注册、排序、校验和执行顺序交给
统一引擎。它不是“把代码搬进 JSON”，也不等于函数式编程。

一个可维护的声明式系统通常包含五层：

```text
声明源 → 解析 → 语义验证 → 编译后的领域模型 → 执行器
```

真正的复用点是中间的领域模型与验证规则。若运行时到处直接读取 JSON 字段，系统只是把
分支逻辑从 C++ 扩散成字符串逻辑。

## 2. 四种常见形式

| 形式 | 例子 | 优点 | 风险 |
|---|---|---|---|
| C++ 描述表 | `constexpr CommandSpec[]` | 类型检查、调试简单 | 修改需重新编译 |
| 外部数据 | JSON/YAML 插件清单 | 部署期可变 | 类型弱、迁移和诊断成本 |
| 内部 DSL | builder/expression template | 与 C++ 类型系统结合 | 编译错误和模板复杂度 |
| 外部 DSL + 代码生成 | LLVM TableGen、MLIR ODS | 单一事实源，可生成代码/文档 | 需要工具链和稳定 schema |

选择依据是修改者是谁、修改发生在编译期还是部署期、错误应何时发现，以及是否值得维护生成器。

## 3. 示例一：声明式命令目录

[`declarative_commands/main.cpp`](../examples/declarative_commands/main.cpp) 只声明命令事实：

```cpp
constexpr cadstudy::CommandSpec commands[] = {
    {"LINE", "Create a line entity",
     cadstudy::CommandFlag::requires_document |
         cadstudy::CommandFlag::modifies_database,
     &draw_line},
};
```

`CommandCatalog::compile()` 负责验证空名称、空 handler 和重复名称，然后复制字符串并生成
查询索引。这里有两个重要设计点：

1. `CommandSpec` 是借用型声明，只用于一次编译；
2. `CommandCatalog` 是拥有型、已验证的运行时对象。

这与 LLVM 的 `StringRef`/`ArrayRef` 使用原则相符：view 很适合调用边界，但长期保存需要明确
底层存储。若命令来自 [`commands.json`](../config/commands.json)，解析器也应产生同一个中间
`CommandDefinition`，再走相同验证器，而不是另写一套注册逻辑。

## 4. 示例二：声明式参数依赖图

[`constraint_graph/main.cpp`](../examples/constraint_graph/main.cpp) 声明参数和规则：

```cpp
constexpr cadstudy::RuleSpec rules[] = {
    {"area", RuleOperation::multiply, "width", "height"},
    {"double_area", RuleOperation::add, "area", "area"},
};
```

`ParameterGraph::compile()` 将声明编译为拓扑顺序，并在执行前拒绝：

- 未声明参数；
- 同一输出存在多个生产者；
- 依赖环；
- 重复或空参数名称。

`evaluate()` 只执行已验证计划，并处理除零和非有限结果。这种“两阶段”结构适合 CAD 的参数化
重算：低频编辑阶段承担验证和计划生成，高频求值阶段使用紧凑、可信的执行结构。

本示例不是几何约束求解器；真实系统还需要自由度分析、方程组、增量失效、数值容差和冲突集
诊断。但“声明 → 验证 → 执行计划”的边界仍然成立。

## 5. LLVM TableGen 的启示

LLVM 官方把 TableGen 定义为维护领域记录并生成复杂输出的工具。其关键并非 `.td` 语法，
而是以下架构：

1. 用带类型的记录表达领域事实；
2. 前端完成解析和实例化；
3. 多个 backend 消费同一记录集合；
4. 生成 C++、表、匹配器或文档；
5. 手写代码只保留真正无法声明的算法。

MLIR ODS 进一步用声明生成 operation 类、builder、验证器和文档。它证明“声明式”应当减少
重复事实并增加自动验证，而不是隐藏控制流。MLIR DRR 文档也明确列出其不擅长的场景，例如
复杂 region/loop 转换；这提醒 CAD 团队必须给 DSL 划边界。

## 6. CAD 中优先级较高的应用

### 6.1 图元属性 schema

单一 schema 可驱动：

- C++ 属性 ID；
- 序列化和反序列化；
- 属性面板；
- 默认值和范围验证；
- SDK 文档；
- 文件版本迁移测试。

示例见 [`entity_schema.json`](../config/entity_schema.json)。建议 schema 中使用稳定 ID，显示名称
只用于 UI；派生属性必须标注只读，并由领域算法计算。

### 6.2 插件 ABI 描述

[`plugins.json`](../config/plugins.json) 可描述宿主 API 下限、加载策略和必需/可选入口。但函数
签名本身最好由 IDL/头文件生成，而不是让 JSON 字符串决定 C++ cast。

### 6.3 命令、菜单与快捷键

命令语义、UI 放置和用户快捷键变化频率不同，应使用不同声明层。不要让菜单 JSON 成为命令
事务属性的唯一真相；核心命令目录应在插件装配时验证。

### 6.4 参数化和依赖重算

把约束关系声明成图，再由编译器产生增量求值计划、失效传播和并行分区。执行器应消费数字 ID
和连续数据，不应在热路径反复查询字符串。

### 6.5 渲染与导出流水线

固定种类、经常重新组合的 pass 很适合声明式 pipeline。LLVM 新 Pass Manager 把 pass 本体、
分析缓存和 pipeline 组合分离，CAD 可对应为几何清理、拓扑修复、网格化、可见性和导出 pass。

## 7. 什么时候不要声明式

- 只有一两个实例，重复事实尚不存在；
- 算法高度动态，声明会退化为另一门通用编程语言；
- 无法给出稳定的领域词汇和 schema；
- 错误只有进入 C++ 深处才能发现；
- 团队没有能力维护解析器、版本迁移、诊断和生成测试；
- 热路径每次解释字符串配置，而没有编译阶段。

判断准则：新增一个领域概念时，如果必须同步修改解析器、十个 switch、文档和 UI，声明式的
单一事实源可能有价值；如果新增声明只是把一行清楚的 C++ 变成五层生成链，则不值得。

## 8. 声明式系统审查清单

- [ ] 声明表达的是领域事实，还是把控制流伪装成数据？
- [ ] schema 是否有版本和稳定 ID？
- [ ] 解析错误与语义错误是否分开？
- [ ] 是否在执行前完成交叉引用、唯一性和环检查？
- [ ] 编译后的模型是否拥有所需数据，不保存短命 view？
- [ ] 一个声明能否生成至少两个一致产物，从而真正消除重复？
- [ ] 是否有逃生口，且逃生口不会吞掉全部类型检查？
- [ ] 是否记录来源位置，让诊断能指回配置？
- [ ] 是否有版本迁移和 golden tests？
- [ ] 热路径是否脱离字符串和通用 JSON 树？
