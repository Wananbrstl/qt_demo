# TCP 渐进实验与验收标准

每个实验都要求记录环境、命令、参数、原始输出和结论。只写“连接成功”不算完成。

## 实验 1：异步连接生命周期

步骤：

1. 只启动上位机，不启动模拟器。
2. 点击 Connect，观察 Connecting、错误和 Reconnecting。
3. 启动模拟器，观察自动恢复到 Online。
4. 点击 Disconnect，等待 30 秒。
5. 退出上位机并检查没有 `QThread: Destroyed while thread is still running`。

记录：每次状态、原因、重连延迟、connection generation。

通过标准：用户断开后重连次数保持不变；网络失败后使用抖动退避；退出无 timer/thread 警告。

## 实验 2：所有拆包边界

自动化基线：

```bash
./build/linux-debug/examples/tcp/test_tcp_frame_codec everyFragmentBoundary
```

手工运行一字节分片：

```bash
./build/linux-debug/examples/tcp/TcpDeviceSimulator \
  --port 45455 --chunk-bytes 1 --chunk-delay 1
```

记录：握手时间、decoder 最大缓存、CRC 错误、遥测数量。

通过标准：握手和遥测正常；不同应用帧的 chunk 不交错；decoder 没有把 readyRead 当成帧。

## 实验 3：粘包

把模拟器 `--telemetry-ms` 调小为 20，`--chunk-bytes` 设为 0。一次 readyRead 很可能包含多个帧。

通过标准：sequence 连续，单次 append 可以返回多帧；帧数不依赖 readyRead 次数。

## 实验 4：CRC 与重新同步

```bash
./build/linux-debug/examples/tcp/TcpDeviceSimulator \
  --port 45455 --corrupt-every 10
```

记录：损坏帧编号、CRC 错误数、后续合法帧恢复时间。

通过标准：损坏帧不进入业务层；进程不崩溃；后续 magic 能重新同步；错误计数可见。

## 实验 5：心跳与重连

```bash
./build/linux-debug/examples/tcp/TcpDeviceSimulator \
  --port 45455 --drop-every 4 --disconnect-after 12
```

记录：Online 持续时间、静默检测时间、重连尝试分布和 generation。

通过标准：状态不依赖 socket 布尔量猜测；旧 generation 的响应不能完成新请求；资源始终有界。

## 实验 6：命令超时与幂等

```bash
./build/linux-debug/examples/tcp/TcpDeviceSimulator \
  --port 45455 --reply-delay 5000
```

配置 `requestTimeoutMs=1000`，发送 `start-spindle`。

通过标准：UI 显示 `TimedOutUnknown`，不得显示“未执行”；未知或迟到响应被记录；非幂等命令不会自动重发。

随后使用同一 request ID 重发测试 simulator 的有界去重缓存，设备应返回第一次结果而不重复副作用。

## 实验 7：发送背压

步骤：

1. 把 queue 上限临时改为 4 条/512 字节。
2. 暂停或显著降低对端读取能力。
3. 快速提交控制命令和遥测。
4. 观察 queue items、bytes、high water 和 rejected。

通过标准：内存不会持续增长；控制命令优先；遥测同 key 合并；拒绝具有明确结果，不静默丢命令。

## 实验 8：配置边界

依次把 JSON 改为：

- port 70000；
- heartbeat 大于 silence timeout；
- maximum buffer 小于 maximum frame；
- queue 字段改为字符串。

通过标准：程序在创建网络 worker 前退出并指出字段错误，不出现“部分启动”。

## 实验 9：GDB 与抓包对齐

1. 在 `onReadyRead` 和 `StreamFrameDecoder::append` 断点。
2. 同时运行 tcpdump。
3. 记录某个 sequence 的 pcap 时间、日志时间和断点调用。
4. 比较 TCP segment、readyRead 和应用帧数量。

通过标准：能够解释三者为何不同，并从连续字节重建应用帧。

## 实验 10：八小时稳定性

周期性执行：延迟、丢应用帧、CRC 损坏、模拟器重启和命令响应延迟。

每分钟采集：RSS、decoder 缓冲、发送队列、pending 数、帧率、协议错误、P95/P99 RTT 和重连次数。

通过标准：

- RSS 不呈持续线性增长；
- 所有队列和缓冲不超过配置；
- GUI 可交互；
- 无线程/定时器警告；
- 每次故障可以从日志恢复时间线；
- 非幂等命令没有无法解释的重复执行。

## 最终完成定义

能够不看代码解释以下问题：

1. 为什么一次 write 和一次 readyRead 都不是一帧？
2. decoder 如何在任意输入下保持时间和空间有界？
3. TCP connected 与业务 Online 有何区别？
4. 超时为什么是未知执行状态？
5. 背压为什么不能只依赖 QTcpSocket 写缓冲？
6. connection generation 如何阻止旧响应污染新会话？
7. 如何用 GDB、日志、`ss` 和 pcap 证明问题在哪一层？

全部能回答并通过上述实验，才完成这套上位机 TCP 学习用例。
