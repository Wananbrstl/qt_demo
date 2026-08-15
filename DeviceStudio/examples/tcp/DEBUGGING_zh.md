# TCP 调试与证据链

调试目标不是“多打日志直到能跑”，而是逐层回答：进程是否正确、连接是否存在、字节是否到达、帧是否完整、会话状态是否允许、业务响应是否匹配。

## 1. 确认二进制和 Qt 版本

```bash
qmake --version
file build/linux-debug/examples/tcp/TcpWorkstation
ldd build/linux-debug/examples/tcp/TcpWorkstation | grep Qt5
readelf -S build/linux-debug/examples/tcp/TcpWorkstation | grep debug_info
```

如果 `ldd` 指向 Qt 6，就没有在本用例要求的 Qt 5 基线上运行。若没有 `.debug_info`，先修复构建类型，不要继续调整断点。

## 2. Qt 分类日志

会话使用：

```text
devicestudio.tcp.session
devicestudio.tcp.protocol
```

启用规则：

```bash
QT_LOGGING_RULES='devicestudio.tcp.*=true' \
  ./build/linux-debug/examples/tcp/TcpWorkstation \
  --config examples/tcp/config/tcp_lab.json
```

日志应包含状态原因、sequence、队列与协议错误。默认不打印完整载荷，生产参数和密钥不得进入日志。

## 3. GDB 断点路径

```bash
gdb --args build/linux-debug/examples/tcp/TcpWorkstation \
  --config examples/tcp/config/tcp_lab.json
```

建议断点：

```gdb
break ds::tcp::DeviceSession::startConnectionAttempt
break ds::tcp::DeviceSession::onConnected
break ds::tcp::DeviceSession::onReadyRead
break ds::tcp::StreamFrameDecoder::append
break ds::tcp::DeviceSession::handleCommandReply
run
```

条件断点：

```gdb
break ds::tcp::DeviceSession::handleFrame if frame.sequence == 42
info threads
thread apply all bt
info sharedlibrary
info sources
```

在 `initialize()` 检查：

```gdb
print this->thread()
print QThread::currentThread()
print socket_->thread()
```

三个对象的亲和关系应符合设计。不要因为看到多个线程就假设 queued connection 正确，必须检查发送者和接收者实际 thread。

## 4. 操作系统连接证据

```bash
ss -tinp '( sport = :45455 or dport = :45455 )'
lsof -nP -iTCP:45455
strace -f -e trace=network \
  ./build/linux-debug/examples/tcp/TcpWorkstation \
  --config examples/tcp/config/tcp_lab.json
```

`ss -tin` 可观察 send/receive queue、RTT、重传等内核状态。应用显示 Online 但系统没有连接，说明状态机存在缺陷；系统已连接但停留 Handshaking，应检查应用帧。

## 5. tcpdump 与 Wireshark

抓包：

```bash
sudo tcpdump -i lo -nn -s 0 -w tcp-lab.pcap port 45455
```

过滤器：

```text
tcp.port == 45455
tcp.analysis.retransmission
tcp.flags.reset == 1
tcp.window_size_value == 0
```

使用 Follow TCP Stream 观察连续字节。不要把 Wireshark 中的 TCP segment 当作应用帧：GRO/TSO、拥塞和接收调度都会改变分段形状。正确协议必须在任何分段方式下工作。

## 6. 网络故障注入

内核级故障使用 netem：

```bash
sudo tc qdisc add dev lo root netem delay 100ms 30ms loss 5% duplicate 1%
sudo tc qdisc show dev lo
sudo tc qdisc del dev lo root
```

执行前确认目标是 `lo`，实验结束必须删除 qdisc。应用层拆包、CRC 损坏和响应延迟使用模拟器参数，二者解决的问题不同。

## 7. Sanitizer

单独创建构建目录，避免污染普通 Debug：

```bash
cmake -S . -B build/asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/asan --parallel
ctest --test-dir build/asan --output-on-failure
```

parser 测试通过不代表任意输入安全；应在 Sanitizer 下增加随机截断、位翻转和长度字段变异。

## 8. 常见误判

### readyRead 次数少于发送次数

正常。TCP 没有消息边界，检查 decoder 产出的帧和 sequence，而不是回调次数。

### write 返回成功，所以设备执行了命令

错误。write 只表示数据进入本地 Qt/socket 缓冲。设备执行需要应用响应；响应超时仍可能是“已执行但响应丢失”。

### CRC 正确，所以数据可信

错误。攻击者可以重算 CRC。CRC 只发现偶然损坏。

### 增大 busy/timeout 就能提高稳定性

可能只是延迟暴露故障。应测量队列、RTT、处理时间和失败类型，再决定超时。

### 一台设备一个线程最安全

不一定。事件驱动 socket 可以共享网络线程；线程数量应由连接规模、每帧 CPU 和事件循环延迟决定。

## 9. 建议的故障时间线字段

```text
monotonic timestamp
device ID
connection generation
session state
direction
message type / sequence / request ID
decoder buffered bytes
application queue items / bytes
socket bytesToWrite
error category and reason
```

这些字段足以把 GDB、应用日志、内核连接与 pcap 对齐，形成可复核证据链。
