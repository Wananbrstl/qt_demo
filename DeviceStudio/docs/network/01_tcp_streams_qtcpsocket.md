# TCP streams and QTcpSocket

## Core fact

TCP preserves byte order, not application message boundaries. One `write()` may
arrive through multiple `readyRead` emissions, and multiple writes may arrive in
one emission. Code that treats `readAll()` as one message is incorrect.

`DeviceSession::onReadyRead` therefore passes every byte to a persistent
`FrameDecoder`. Only complete frames reach `handleFrame`.

## Asynchronous use

The session connects `connected`, `disconnected`, `readyRead`, and
`errorOccurred`. It never blocks with `waitFor...`. The owning network thread's
event loop drives the socket.

## Backpressure

This initial version relies on QTcpSocket's write buffer for small command and
heartbeat traffic. A production extension should enforce a maximum outbound
queue and respond to `bytesWritten`; never allow telemetry or command queues to
grow without a limit.

## Debugging checklist

1. Confirm socket and session have the same `thread()`.
2. Inspect `state()` and `errorString()`.
3. Observe frames in Protocol Inspector, not just raw `readyRead` sizes.
4. Use Wireshark only after confirming the application framing logic.
5. Verify simulator and workstation agree on byte order and protocol version.

