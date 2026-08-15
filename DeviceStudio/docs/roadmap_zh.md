# 学习路线

## 阶段 0：可复现环境与声明式边界

1. 执行 `scripts/bootstrap-wsl.sh`，阅读 `setup/01_wsl_vscode_zh.md`。
2. 阅读 `declarative/01_declarative_boundaries_zh.md`。
3. 区分哪些工程制品声明策略，哪些组件负责执行策略。
4. 确认 VS Code 启动的程序正是 CMake Debug preset 生成的二进制文件。

验收：能从 preset、目标、依赖和运行配置解释一次 F5 背后发生的工作。

## 阶段 1：Qt 对象模型与布局

1. 阅读 `qt/01_qobject_event_loop_zh.md`。
2. 跟踪 `main.cpp → MainWindow → DocumentManager`。
3. 使用 `QFormLayout` 重写一个摘要区，比较不同窗口尺寸下的行为。
4. 在调试器中检查 QObject 的父子所有权。

验收：能解释子控件为何使用非拥有型裸指针，以及应用服务为何由主窗口拥有，而跨线程 worker 不能带父对象。

## 阶段 2：MDI、停靠窗口与 Model/View

1. 阅读 `qt/02_handwritten_layout_mdi_zh.md` 和 `qt/03_model_view_threads_zh.md`。
2. 在标签页和子窗口两种 MDI 模式间切换。
3. 打开两个历史文档，验证监控/协议文档唯一，而历史文档可以多开。
4. 使用代理模型为遥测表增加过滤，不修改源模型。

## 阶段 3：TCP 与应用协议

1. 启动 GUI 前先运行 `test_protocol`。
2. 依次阅读 `network/` 下 01～03 的中文文档，再学习 `04_industrial_tcp_advanced_zh.md`。
3. 在 `FrameDecoder::append` 设置断点，逐字节输入一帧。
4. 注入拆包、粘包、CRC 错误、延迟、断线和重连风暴。
5. 记录吞吐、P95/P99 延迟、队列高水位和恢复时间，不只观察“是否连上”。
6. 按 `../examples/tcp/LABS_zh.md` 运行完整 Qt 5.15 TCP 用例。

验收：能说明 TCP 字节流、应用帧、请求关联、超时、幂等与背压之间的关系。

## 阶段 4：SQLite

1. 阅读两份基础数据库文档，再学习 `database/03_sqlite_engineering_advanced_zh.md`。
2. 检查 `QStandardPaths::AppLocalDataLocation` 下生成的数据库。
3. 对比每条数据一个事务与现有 250 ms 批量事务。
4. 数据产生后只新增迁移，不修改已应用的 `001`。
5. 使用 `EXPLAIN QUERY PLAN`、WAL checkpoint 指标和分位延迟验证优化结果。

验收：能从查询形状推导索引，解释事务边界、WAL、分页、保留策略和崩溃恢复。

## 阶段 5：集成与故障恢复

1. 连接 GUI 并启动主轴，然后终止模拟器。
2. 观察 `Online → Reconnecting → Connecting → Online`。
3. 重启模拟器，验证采集恢复且数据继续持久化。
4. 在历史 MDI 文档中查询断线时间段，区分“设备无数据”和“存储失败”。

## 完成定义

不查看源码时，能够解释每个线程、所有权边界、状态转换、TCP 帧边界和 SQL 事务边界；能够用可复现实验说明系统在高负载、慢设备、数据库锁竞争和进程崩溃后的行为，才算完成本工程的初阶与进阶学习。
