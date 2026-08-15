# TCP 字节流与 QTcpSocket

## 核心事实

TCP 保证有序字节流，不保留应用消息边界。一次 `write()` 可能对应多次 `readyRead`，多次 write 也可能在一次 readyRead 中到达。把 `readAll()` 当成一条消息必然会在真实网络中出错。

因此 `DeviceSession::onReadyRead` 把所有字节追加给持久存在的 `FrameDecoder`，只有完整且校验通过的帧才能进入 `handleFrame`。

TCP 还不承诺应用级“只处理一次”。连接断开时，发送方无法仅凭本地状态知道最后一条消息是否已被对端处理，命令协议必须自行设计请求 ID、确认与幂等。

## 异步使用

会话连接 `connected`、`disconnected`、`readyRead` 和 `errorOccurred`，不调用阻塞式 `waitFor...`。socket 所在线程的事件循环驱动状态变化。所有 socket 操作必须发生在其线程亲和线程。

## 背压

当前版本的小流量命令和心跳依赖 QTcpSocket 写缓冲。生产扩展必须限制应用发送队列，监控 `bytesToWrite()` 和 `bytesWritten`，并定义高水位策略：拒绝、降采样、覆盖旧遥测或断开慢消费者。任何遥测和命令队列都不能无限增长。

## 调试清单

1. 确认 socket 与 session 的 `thread()` 相同。
2. 检查 `state()`、`error()` 和 `errorString()`。
3. 在 Protocol Inspector 中观察帧，而不是只看 readyRead 大小。
4. 先验证应用分帧，再使用 Wireshark 排查网络。
5. 确认双方字节序、协议版本、最大帧长一致。
6. 记录发送队列、接收缓存、序列号缺口和连接代次。

## 实验

把一个编码帧按所有可能边界拆分，并随机把多个帧粘接后输入 decoder。验收条件不是“运行一次成功”，而是固定随机种子执行至少 10 万组输入，无越界、无丢帧、无重复帧，缓存始终受上限约束。
