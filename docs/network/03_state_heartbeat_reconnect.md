# Connection state, heartbeat, and reconnect

## State flow

```text
Disconnected → Connecting → Handshaking → Online
                      ↓                     ↓
                 Reconnecting ← Degraded ← timeout
```

State is explicit so UI action enablement and diagnostics do not infer behavior
from scattered socket flags.

## Handshake

TCP connected does not mean protocol-compatible. The session becomes Online only
after receiving HelloAck. Future versions can negotiate capabilities, auth, and
protocol versions here.

## Heartbeat

The client sends a heartbeat every two seconds and checks time since any inbound
frame. Six seconds of silence marks the connection degraded and forces reconnect.
Real deployments should configure these values per device profile.

## Reconnect

Delay grows exponentially and includes random jitter. Jitter prevents hundreds
of devices from reconnecting in lockstep after a network outage. A user-requested
disconnect sets `shutdownRequested_`, preventing automatic reconnect.

## Exercises

- Inject 30% heartbeat loss in the simulator.
- Add a maximum reconnect-attempt policy.
- Make timing injectable so tests do not wait for real seconds.
- Decide which commands are safe to retry and document idempotency rules.

