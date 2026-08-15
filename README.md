# DeviceStudio

[中文说明与进阶教程](README_zh.md)

DeviceStudio is an industrial-style learning workstation for Qt 6, asynchronous
TCP, and SQLite. It is intentionally small enough to study end to end while
retaining production-oriented boundaries: UI widgets do not own sockets or SQL
connections, protocol parsing is tested independently, and background resources
respect Qt thread affinity.

## What is included

- Handwritten Qt Widgets UI; no `.ui` files and no QML.
- `QMainWindow`, `QMdiArea`, `QMdiSubWindow`, and three `QDockWidget` sidebars.
- A bounded Model/View telemetry table and a custom-painted live chart.
- A versioned binary TCP envelope with CRC32, CBOR payloads, and stream decoding.
- Heartbeat, reconnect backoff, command request/reply, and protocol inspection.
- A dedicated SQLite worker thread, migrations, WAL, prepared statements, and
  batched telemetry transactions.
- A local TCP device simulator and Qt Test unit/integration tests.
- Learning guides under `docs/` that map concepts to concrete source files.

## WSL prerequisites

Run the following yourself because `sudo` may require your password:

```bash
sudo apt-get update
sudo apt-get install -y qt6-base-dev qt6-base-dev-tools libqt6sql6-sqlite
```

Alternatively, `bash scripts/bootstrap-wsl.sh` installs the complete toolchain,
configures the project, builds it, and runs its tests. Detailed VS Code/GDB setup
is documented in `docs/setup/01_wsl_vscode.md`.

Build inside the WSL Linux filesystem rather than `/mnt/c` or `/mnt/e`:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

Start the simulator first:

```bash
./build/linux-debug/DeviceSimulator --port 45454
```

In another terminal, start the workstation:

```bash
./build/linux-debug/DeviceStudio
```

WSLg displays the application directly on Windows. The GUI connects to
`127.0.0.1:45454` when **Device → Connect** is selected.

## Source map

```text
src/domain           Value types and business data
src/protocol         TCP envelope, CRC, CBOR, stream decoder
src/network          QTcpSocket session, heartbeat, reconnect
src/persistence      SQLite thread and repositories
src/application      UI-facing asynchronous facade
src/ui               Handwritten MDI, docks, models, and widgets
simulator            Local TCP device simulator
tests                Qt Test protocol and database tests
docs                 Guided learning material and exercises
```

The guides also include `docs/declarative/01_declarative_boundaries.md`, which
applies declarative design to CMake targets, typed runtime policy, migrations,
and handwritten Qt layouts without requiring QML.

## Learning order

Read `docs/roadmap.md`. Do not begin by reading `MainWindow.cpp` linearly; follow
the data flow from protocol tests to the simulator, service, models, and UI.

## Scope and safety

This is industrial-style learning code, not a certified machine-safety system.
Never connect teaching commands to hazardous equipment without authentication,
authorization, TLS, safety interlocks, independent emergency stops, and a formal
hazard analysis.
