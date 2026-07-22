# LLVM 设计到现代 CAD 的映射

## 1. 样本说明

本地 `F:\llvm-master` 经 `CMakeLists.txt` 确认为 LLVM 10.0.0 源码快照。研究同时参考当前 LLVM
官方文档。本文会区分：

- LLVM 10 本地代码中实际存在的设计；
- 当前 LLVM/MLIR 文档中继续使用或演化的设计；
- 面向自研 CAD 的推导。

LLVM 是编译器基础设施，不是 CAD 应用框架。借鉴重点是语义、数据关系和扩展边界，而不是复制
命名和容器。

## 2. 值、视图和拥有者分离

### LLVM 观察

本地 `StringRef` 是指针加长度的非拥有 view。`ProgrammersManual.rst` 明确要求它通常按值传递，
且除非证明存储寿命，否则不应保存。`ArrayRef` 对连续只读序列提供同类接口。`function_ref`
则是对 callable 的非拥有引用；若需要保存，应选择拥有型 callable。

### CAD 推导

建议形成一致词汇：

- `XxxView`：只在调用期间借用；
- `XxxSpan`：借用且可能修改；
- `XxxId`：稳定身份，不代表对象已打开；
- `XxxHandle`：持有资源或会延长生命周期；
- 无后缀领域值：默认拥有自身状态。

例如几何算法可接受 `PointSpan`，数据库实体长期关联则保存 `EntityId`，不要把事务内
`DbEntity*` 或 `string_view` 塞进长期反应器对象。

### 不应照搬

LLVM 的 `Twine` 以极窄用途换取字符串拼接性能，其文档也称这种临时生命周期 API inherently
dangerous。CAD SDK 的公共接口通常不应自创类似表达式临时树，除非基准证明字符串分配是瓶颈，
且使用面被严格限制在单次调用。

## 3. 错误是返回协议的一部分

### LLVM 观察

本地 `llvm/Support/Error.h` 和 Programmer's Manual 区分可恢复错误与程序错误；`Expected<T>`
用于返回值或错误。错误必须被消费或传播，命名工厂用于 fallible construction。

### CAD 推导

错误类型应包含领域上下文，而不仅是整数：

```text
DatabaseConflict { objectId, transactionId, operation }
PluginAbiMismatch { pluginId, expectedVersion, actualVersion }
ConstraintConflict { conflictingConstraintIds }
```

宿主边界可把这些错误投影为 SDK 状态码或诊断对象，但内核内部应保留结构化信息。本项目
[`Result<T>`](../include/cadstudy/result.h) 是教学型最小实现；生产中可使用成熟 `expected`
实现，并增加组合操作、source location、错误链和日志策略。

## 4. 按规模选择数据结构

### LLVM 观察

LLVM Programmer's Manual 先按算法需求选择 sequential/set/map，再比较常数、分配和缓存行为。
`SmallVector` 针对通常很小但上界不固定的序列，把一部分存储内嵌，避免常见路径堆分配。

### CAD 推导

可能适用：

- polyline 邻接边通常很少；
- constraint node 的 incident edges 通常很少；
- command 的 alias 数量通常很少；
- BRep 拓扑局部邻接列表通常较短。

但内嵌容量会放大每个对象。如果百万级 entity 每个都内嵌 8 个槽，可能比偶发分配更糟。
因此先采集分布（P50/P95/P99）、遍历频率和对象总数，再选择 inline capacity。

## 5. arena 与上下文生命周期

### LLVM 观察

`BumpPtrAllocator` 让大量对象共享区域生命周期，单个释放退化为整区回收。这与编译过程中
IR/AST 的阶段性生命周期高度契合。

### CAD 推导

适用于：

- 一次布尔运算的临时拓扑；
- 一帧渲染命令；
- 一次导入解析的中间节点；
- 一轮约束求解的工作集。

不适用于任意独立删除、长期跨事务引用或需要稳定析构副作用的数据库对象。arena 应由明确的
`OperationContext`/`FrameContext` 拥有，任何逃出上下文的指针都必须被禁止或复制为拥有值。

## 6. Pass Manager：算法与调度分离

### LLVM 观察

当前 LLVM 新 Pass Manager 采用 concept-based polymorphism；pass 提供 `run()`，manager 负责
组合、分析缓存和失效。`PreservedAnalyses` 使变换明确声明哪些分析仍有效。

### CAD 推导

这比“每个图元都有几十个 virtual optimize/export/repair”更适合流水线：

```text
CAD Model
  → ValidateTopology
  → HealGeometry
  → Tessellate
  → BuildAccelerationStructure
  → Export
```

每个 pass 声明读取、修改和保留的数据域；manager 负责缓存包围盒、拓扑索引、质量统计等分析。
关键借鉴点不是 CRTP，而是“变换、分析、缓存失效和 pipeline 装配”四个职责分开。

## 7. Registry 与插件注册

### LLVM 10 观察

本地 `llvm/Support/Registry.h` 使用全局链表和静态构造注册插件条目；条目包含名称、描述和
无参工厂。它让链接进来的组件自动可发现，但依赖静态初始化和进程级注册表。

### CAD 推导

现代 CAD 更适合显式装配：

1. 宿主加载版本化 C ABI 入口；
2. 插件返回 descriptor/function table；
3. `PluginCatalog::compile()` 验证 ID、版本和冲突；
4. 成功后一次性提交注册；
5. 卸载前撤销命令、反应器和对象工厂。

静态注册可保留给单一二进制内部、进程终身组件；不应作为可卸载 DLL 的唯一协议，因为卸载后
全局链表节点和函数地址可能悬空。

## 8. 永久动态库的适用边界

LLVM 10 `DynamicLibrary` 文档明确没有临时加载接口，library 直到 `llvm_shutdown()` 才卸载。
这消除了大量函数地址生命周期问题，适合编译器工具的插件模型。

CAD 若支持热加载/卸载、插件更新或文档级扩展，应使用显式生命周期和撤销协议。本项目让
`ImportedFunction` 延长模块寿命，是介于裸地址与永久加载之间的选择；它仍不能自动解决插件
创建对象的销毁顺序，插件对象也必须持有模块 token。

## 9. TableGen：单一事实源和多 backend

### LLVM 观察

本地包含 TableGen 前端、记录模型和多个 backend；当前官方文档仍强调以声明记录领域信息，
由 backend 生成更复杂且重复的输出。MLIR ODS 用同一声明生成操作类、builder、验证器与文档。

### CAD 推导

可建立轻量 `cad-tblgen`/schema generator，优先处理高重复且规则稳定的领域：

- entity type ID、属性、序列化标签和 UI 元数据；
- 命令 ID、事务属性、权限和文档；
- 文件格式 chunk、版本和迁移；
- 插件 ABI function table；
- 几何 kernel operation 的输入输出约束。

生成器必须保留手写扩展点，并产生可读诊断。不要从一开始复制 TableGen 完整语言；先用简单
schema 和两三个 backend 验证收益。

## 10. 稳定身份与内部对象关系

LLVM IR 广泛使用指针身份、父子关系和 use-def 图，适合一次编译过程内、由 context 控制的对象。
CAD 数据库则需要持久化、undo/redo、跨文档隔离和对象删除，因此更适合：

- 外部长期引用使用 `DatabaseId + EntityId + generation`；
- 事务内打开后才获得短期 `Entity&`/handle；
- generation 或 tombstone 防止 ID 重用后的 ABA；
- 关系图保存 ID，求值阶段批量解析为局部指针/view；
- undo 记录领域操作或增量，不向外暴露旧对象地址。

这里应借鉴 LLVM 对关系图和上下文的重视，但不能照搬“裸指针即长期身份”。

## 11. 借鉴优先级

### 优先采用

1. 值/视图/拥有者的命名与边界；
2. fallible factory + 结构化错误；
3. 数据结构按规模和访问模式选择；
4. 声明先验证/编译，运行时执行已验证模型；
5. pass、分析和失效协议分离；
6. arena 与 operation context 绑定。

### 经过适配采用

1. `SmallVector`：先测量容量分布；
2. registry：改为显式、事务式注册；
3. `isa/cast/dyn_cast`：仅用于受控闭集层次；
4. intrusive list/refcount：只用于布局或所有权收益明确的内核区域；
5. TableGen：从小 schema 和少量 backend 开始。

### 默认不要照搬

1. 永久加载作为所有 CAD 插件的默认；
2. 可长期保存的 `StringRef`/裸 span；
3. `Twine` 式危险临时表达式进入公共 SDK；
4. 静态构造完成可卸载插件注册；
5. 裸 IR 指针作为持久 CAD 对象身份；
6. 依赖断言和进程退出处理用户文档或插件错误。

## 12. 建议的 CAD 基础层

```text
cad::core
  Result / Error / stable IDs / owned values / views

cad::model
  Database / Transaction / EntityStore / ChangeSet

cad::pipeline
  Pass / Analysis / Invalidation / Scheduler

cad::schema
  Entity / Property / Command / Serialization descriptors

cad::plugin
  SharedModule / ABI table / PluginToken / Catalog

cad::geometry
  Value geometry / operation contexts / temporary arenas
```

依赖方向应从插件、UI和具体工具指向稳定的 core/model contracts，几何和数据库核心不反向依赖
命令或插件加载器。
