# DeviceStudio 中文指南

[English README](README.md)

DeviceStudio 是一个基于 Qt 5.15、异步 TCP 与 SQLite 的工业风格上位机学习工程。工程规模适合从头到尾阅读，同时保留生产代码应有的边界：界面控件不拥有套接字或数据库连接，协议解析可以独立测试，后台对象严格遵守 Qt 线程亲和性。

## 工程包含的内容

- 全部使用 C++ 手写 Qt Widgets 界面，不使用 `.ui` 文件和 QML。
- 使用 `QMainWindow`、`QMdiArea`、`QMdiSubWindow` 和三个 `QDockWidget` 构建 MDI 工作区。
- 有界的 Model/View 实时数据表，以及使用 `QPainter` 自绘的实时曲线。
- 带版本号、CRC32、CBOR 载荷和流式解码器的二进制 TCP 协议。
- 心跳检测、指数退避重连、命令请求/响应和协议观察器。
- 独立 SQLite 工作线程、迁移、WAL、预编译语句和遥测批量事务。
- 本地 TCP 设备模拟器，以及 Qt Test 单元测试和集成测试。
- `docs/` 下与具体源码对应的学习文档和实验。

## WSL 环境准备

`sudo` 可能要求输入密码，请在 WSL 终端中亲自执行：

```bash
sudo apt-get update
sudo apt-get install -y qtbase5-dev qtbase5-dev-tools qt5-qmake libqt5sql5-sqlite
```

也可以执行 `bash scripts/bootstrap-wsl.sh`，一次性安装工具链、配置、构建并运行测试。VS Code 与 GDB 的配置见 `docs/setup/01_wsl_vscode_zh.md`。

建议把工程放在 WSL 的 Linux 文件系统中，而不是 `/mnt/c` 或 `/mnt/e`：

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

先启动模拟器：

```bash
./build/linux-debug/DeviceSimulator --port 45454
```

再在另一个终端启动上位机：

```bash
./build/linux-debug/DeviceStudio
```

WSLg 会把窗口直接显示在 Windows 桌面。选择 **Device → Connect** 后，上位机连接 `127.0.0.1:45454`。

## 源码地图

```text
src/domain           值类型与业务数据
src/protocol         TCP 帧、CRC、CBOR 与流式解码
src/network          QTcpSocket 会话、心跳与重连
src/persistence      SQLite 线程与持久化实现
src/application      面向 UI 的异步应用服务
src/ui               手写 MDI、停靠窗口、模型和控件
simulator            本地 TCP 设备模拟器
tests                Qt Test 协议与数据库测试
docs                 配套教程、实验与故障演练
```

声明式设计并不等于 QML。`docs/declarative/01_declarative_boundaries_zh.md` 说明如何在 CMake、配置、迁移和 C++ 手写布局中使用声明式思想。

## 推荐学习顺序

先阅读 `docs/roadmap_zh.md`，不要从头线性阅读 `MainWindow.cpp`。应沿着“协议测试 → 模拟器 → 应用服务 → 数据模型 → UI”的数据流理解系统。

网络和数据库的工程化进阶内容见：

- `docs/network/04_industrial_tcp_advanced_zh.md`
- `docs/database/03_sqlite_engineering_advanced_zh.md`

## 使用边界与安全

本工程是工业风格的学习代码，不是经过认证的机器安全系统。将教学命令连接到可能伤害人员或设备的执行机构之前，必须补充身份认证、权限控制、TLS、安全联锁、独立急停和正式的风险分析。
