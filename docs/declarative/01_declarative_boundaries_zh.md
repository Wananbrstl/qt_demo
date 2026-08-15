# C++ 手写 Qt 应用中的声明式编程

声明式编程不等于 QML。只要代码或数据描述“应满足什么”，由复用引擎决定“如何实现”，就是声明式设计。DeviceStudio 保持 C++ Widgets，同时在稳定的策略边界采用声明式思想。

## 工程中的四个层次

### 1. 构建图

`CMakeLists.txt` 声明目标、源文件、资源和链接依赖，由 CMake 决定执行顺序。优先使用 `target_link_libraries` 等目标级声明，避免修改全局编译和链接状态。

### 2. 运行策略

`config/application.json` 声明设备端点、心跳、帧上限、数据库批处理间隔与保留策略。策略应集中为数据，不应散落为各个控件中的分支。

理想依赖方向是：

```text
JSON 文档 → 解析/校验 → 不可变强类型设置 → 服务
```

控件只接收校验后的值，不直接查询 JSON、不自行发明默认值，也不知道配置文件位置。

### 3. 持久化结构演进

`migrations/001_initial.sql` 声明数据库结构；`DatabaseWorker` 实现事务、执行、回滚和版本记录。迁移发布后只可追加，修改已应用迁移会破坏版本含义和可重放性。

### 4. UI 约束

即使在 C++ 中创建，Qt 布局仍是约束声明：

```cpp
auto* form = new QFormLayout;
form->addRow(tr("Temperature"), temperatureValue);
form->addRow(tr("Spindle speed"), spindleValue);
```

代码声明行之间的关系，布局引擎根据字体、翻译、DPI 和窗口尺寸计算几何。对每个子控件调用 `setGeometry()` 只是在固化某张截图，而不是表达布局意图。

## 何时使用声明式表示

以下条件大多成立时适合声明式表示：

- 信息属于策略或结构，而不是一次性算法；
- 需要校验、差异比较、工具处理或运行时替换；
- 顺序与资源生命周期可交给边界清晰的引擎；
- 非法状态能在单一边界被拒绝。

生命周期转换、错误恢复、线程亲和操作和协议状态机更适合显式命令式 C++。把 TCP 重连顺序隐藏在配置驱动的回调中不会自动变得更“声明式”，反而更难审计。

## 强类型配置边界

不要在应用内传递 `QJsonObject`，而应只解析一次：

```cpp
struct NetworkSettings {
    std::chrono::milliseconds heartbeatInterval;
    std::chrono::milliseconds heartbeatTimeout;
    qsizetype maximumFrameBytes;
};

struct ApplicationSettings {
    DeviceEndpoint endpoint;
    NetworkSettings network;
};
```

校验必须表达跨字段不变量：

```cpp
if (settings.network.heartbeatTimeout
        <= settings.network.heartbeatInterval) {
    return error("heartbeatTimeout must exceed heartbeatInterval");
}
```

校验后优先使用不可变设置或构造注入，将大量运行期检查收敛为一次启动决策，也使测试摆脱进程级全局配置。

## 实验：数据驱动设备目录

使用 JSON 数组替代单一演示设备，同时保证 `DeviceTreeModel` 不依赖 JSON：

1. 定义值类型 `DeviceDefinition`。
2. 启动时解析并校验 `QVector<DeviceDefinition>`。
3. 将值注入应用服务与模型。
4. 把重复 ID、无效端口作为结构化诊断报告。
5. 使用内存 JSON 编写测试。

验收条件：任何非法配置都不能让网络或数据库 worker 部分启动；错误信息必须指出字段路径和约束，而不是只返回“配置错误”。
