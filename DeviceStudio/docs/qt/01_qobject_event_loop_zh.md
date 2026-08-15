# QObject 所有权、信号与事件循环

## 学习目标

- 区分 C++ 对象生命周期与 Qt 父子所有权。
- 理解直接连接和队列连接的投递语义。
- 理解 GUI 事件循环为什么不能阻塞。
- 判断 Qt 代码中何时适合使用裸指针。

## 工程映射

- `MainWindow` 通过 QObject 树拥有 UI 对象。
- `DeviceApplicationService` 拥有 `QThread`，但 worker 没有 QObject 父对象，因为带父对象的 QObject 不能移动到另一线程。
- `DeviceSession::initialize` 只在 worker 已进入网络线程后创建 socket 与 timer。

## 所有权规则

带 parent 创建的控件由 parent 管理：

```cpp
stateValue_ = new QLabel(this);
```

`stateValue_` 是便于后续访问的非拥有型指针。再用 `std::unique_ptr` 包装会形成两套相互竞争的所有权。非 QObject 资源以及没有 parent 的 QObject 仍应使用 RAII。

设计时要分别回答两个问题：谁决定对象何时销毁？谁只是在生命周期内观察它？不要把“用了裸指针”等同于“泄漏”。

## 队列投递

`DeviceSession` 的信号从网络线程跨到应用服务和 UI。Qt 会向接收者所属线程的事件循环投递元调用事件。自定义参数必须在 worker 启动前由 `domain::registerMetaTypes()` 注册。

队列连接传递的是调用发生时的参数副本。引用捕获、指向可变缓冲区的裸指针或未注册类型都会破坏边界。接收槽何时执行取决于目标事件循环，而不是发送者。

## 常见错误

- 在 GUI 线程创建 `QTcpSocket`，随后只移动包装对象。
- 在 GUI 槽中调用 `waitForConnected`、`waitForReadyRead` 或执行长 SQL。
- 对正在另一线程处理事件的 QObject 直接 `delete`。
- 在队列 lambda 中捕获短生命周期引用。
- worker 线程退出前仍有定时器或 socket 活动。

## 实验

在遥测生产者、数据库写入器、应用服务和监控文档中记录 `QThread::currentThread()->objectName()`。先写出预测，再运行验证。随后故意把一个 timer 提前到构造函数创建，记录警告并解释其根因，最后恢复正确实现。

验收：能够画出对象所有权图和线程亲和图，并说明两张图为什么不完全相同。
