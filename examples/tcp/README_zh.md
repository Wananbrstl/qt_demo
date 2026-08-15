# Qt 5.15 工业 TCP 学习用例

这套用例不是 echo 客户端。它以一个可运行的设备上位机为最终入口，把 TCP 字节流、应用分帧、状态机、背压、请求关联、故障模拟、调试和测试组织成同一套代码。

## 运行基线

- Ubuntu 24.04 / WSL2
- Qt 5.15.13
- C++17
- CMake 3.20+
- Qt Widgets、Qt Network、Qt Test

验证当前工具链：

```bash
qmake --version
# Using Qt version 5.15.13 ...

cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

## 启动完整用例

终端一启动故障可配置设备：

```bash
./build/linux-debug/examples/tcp/TcpDeviceSimulator \
  --port 45455 \
  --telemetry-ms 250
```

终端二启动上位机：

```bash
./build/linux-debug/examples/tcp/TcpWorkstation \
  --config examples/tcp/config/tcp_lab.json
```

点击 Connect 后，状态应经过：

```text
Disconnected → Connecting → Handshaking → Online
```

遥测表最多保留 500 行，协议窗口最多保留 2000 个文本块，发送队列、decoder 缓冲和 pending request 均有明确上限。

若要先观察未经 `DeviceSession` 封装的 `QTcpSocket` 信号，可在模拟器运行时执行：

```bash
./build/linux-debug/examples/tcp/TcpConnectionProbe \
  --host 127.0.0.1 --port 45455 --timeout 5000
```

该工具异步完成连接、发送 Hello、处理 readyRead 和解析 HelloAck；它有总超时与有界 decoder，不使用任何 `waitFor...`。

## 代码地图

```text
common/TcpTypes               强类型配置、状态、遥测、结果与指标
common/SessionConfigLoader    JSON → 校验 → 强类型配置边界
common/BackoffPolicy          可复现的 Full Jitter 指数退避
protocol/FrameCodec           帧编码、CRC32、有界增量解析
transport/BoundedWriteQueue   有界队列、优先级和遥测合并
session/DeviceSession         socket 生命周期、握手、心跳、重连和命令
simulator                     设备行为与拆包/损坏/延迟/断线注入
workstation                   不持有 socket 的手写 Qt 上位机界面
tools/TcpConnectionProbe      可逐步调试的 QTcpSocket 基础生命周期
tests                         协议、策略和端到端集成验证
```

## 最重要的边界

### UI 与网络

`TcpWorkbenchWindow` 只发出信号。`DeviceSession` 被移动到网络线程后才在 `initialize()` 中创建 socket 和 timer。这样不是依靠约定“尽量别跨线程”，而是让错误构造方式难以发生。

### transport 与 protocol

`QTcpSocket::readAll()` 返回当前可用字节，不表示一条消息。所有字节进入持久存在的 `StreamFrameDecoder`，只有完整、长度合法且 CRC 正确的帧才进入会话层。

### 会话与业务命令

TCP connected 不等于设备可用。收到 `HelloAck` 才进入 Online。每条命令包含 UUID，并记录 connection generation 与 deadline。超时结果明确命名为 `TimedOutUnknown`，因为设备可能已经执行，只是响应丢失。

### 配置与运行代码

JSON 只存在于 `SessionConfigLoader`。worker 接收校验后的 `SessionConfig`，不会在运行过程中查找字符串键，也不会由不同控件各自发明默认值。

## 帧格式

所有多字节整数使用网络字节序：

```text
magic:u32 | version:u16 | type:u16 | flags:u16 | reserved:u16
sequence:u32 | payload-size:u32 | crc32:u32 | payload:N
```

固定头为 24 字节。CRC 覆盖前 20 字节头和载荷。CRC 用于发现偶然损坏，不提供身份认证，生产部署仍需 TLS、证书和命令授权。

## 故障模式

模拟器参数：

```text
--chunk-bytes N       每次 socket write 最多 N 字节，用于稳定制造拆包
--chunk-delay MS      chunk 之间的延迟
--corrupt-every N     每 N 个出站应用帧损坏一个字节
--drop-every N        每 N 个出站应用帧不发送
--disconnect-after N  收到 N 帧后主动断线
--reply-delay MS      延迟命令响应，用于验证超时未知状态
--telemetry-ms MS     遥测周期
```

chunk 使用单一串行队列发送。不同帧绝不能因为故障调度而字节交错；那不是 TCP 拆包，而是发送端自己破坏协议。

模拟器也执行资源契约：每个连接最多保留 4 MiB 未发送应用数据，`QTcpSocket` 写缓存达到 256 KiB 后暂停灌入；幂等响应缓存同时受 1024 条和 2 MiB 两个上限约束。分片通过“整帧队列 + 当前偏移”实现，因此 `--chunk-bytes 1` 不会为每个字节创建一个队列节点。

## 自动化测试覆盖

- CRC32 标准测试向量。
- 编码/解码 round trip。
- 一帧的所有拆分边界。
- 100 帧一次粘接。
- 垃圾前缀后的重新同步。
- CRC、超长帧和 decoder 缓冲上限。
- Full Jitter 的确定性和范围。
- 控制优先队列、条目/字节上限、遥测合并。
- 配置解析与跨字段校验。
- 一字节分片下的握手、遥测和命令。
- 命令响应晚于 deadline 时的未知执行状态。

## 推荐阅读顺序

1. `common/TcpTypes.h`：先理解状态、配置和结果语义。
2. `protocol/FrameCodec.cpp`：理解 TCP 字节流如何恢复为帧。
3. `tests/test_frame_codec.cpp`：从不变量而不是实现细节学习 parser。
4. `transport/BoundedWriteQueue.cpp`：理解背压为何必须在应用层出现。
5. `session/DeviceSession.cpp`：连接、握手、心跳、重连、pending 的组合。
6. `simulator/IndustrialDeviceSimulator.cpp`：学习合法故障注入。
7. `workstation/TcpWorkbenchWindow.cpp`：理解 GUI 与 worker 的线程边界。
8. `DEBUGGING_zh.md` 和 `LABS_zh.md`：按实验获取证据。

## 明确未宣称的能力

本用例不宣称 TCP 能提供“业务恰好一次”，不把 CRC 当成安全机制，也不把无限重试当成可靠性。生产设备控制还必须补充 TLS、双向身份、授权、安全联锁、审计、证书轮换和正式风险分析。
