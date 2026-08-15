# WSL2、VS Code、CMake 与 GDB 配置

必须通过 **VS Code Remote - WSL** 打开 `/home/wananbrstl/projects/DeviceStudio`。不要让普通 Windows VS Code 直接打开 Windows 路径，否则会混用 Windows 扩展、Linux 路径、编译器、动态库与调试符号。

## 初始化

在 Ubuntu WSL 中执行：

```bash
cd /home/wananbrstl/projects/DeviceStudio
bash scripts/bootstrap-wsl.sh
```

脚本安装可复现工具链，配置 Debug preset，构建全部目标并运行测试。脚本有意使用普通 `sudo`；请交互输入密码，禁止把密码写入任务、配置或脚本。

## VS Code 扩展

以下扩展必须安装在 **WSL: Ubuntu** 中：

- CMake Tools（`ms-vscode.cmake-tools`）
- C/C++（`ms-vscode.cpptools`），提供 `cppdbg` GDB 适配器
- clangd（`llvm-vs-code-extensions.vscode-clangd`），用于语义导航

工程关闭 Microsoft IntelliSense 引擎以避免两个语言服务器同时分析，但保留 C/C++ 扩展用于调试。

## 构建与调试契约

`linux-debug` preset 始终在 `build/linux-debug` 生成未剥离符号的 Debug 程序。launch 配置带有 `preLaunchTask`，F5 会先构建 GDB 随后启动的同一个二进制文件，避免源码已更新但调试器仍运行旧程序。

检查命令：

```bash
file build/linux-debug/DeviceStudio
readelf -S build/linux-debug/DeviceStudio | grep debug_info
gdb -q -batch -ex 'info sources' build/linux-debug/DeviceStudio
```

`file` 应包含 `with debug_info, not stripped`，`readelf` 应找到 `.debug_info`，GDB 列出的源码路径应以 WSL 工程路径开头。

## 推荐流程

1. 修改协议或数据库后，运行 `DeviceStudio: test debug` task。
2. 集成调试选择 `DeviceStudio + Simulator` compound；只调 GUI 时可在终端单独启动模拟器。
3. 第一个断点放在函数体可执行语句上，不要放在声明或可能被优化掉的代码上。
4. 空心断点出现时，先比较 Debug Console 的源文件路径和 `info sources`，不要立即随意修改调试配置。
5. 使用 `info sharedlibrary` 判断模块是否装载，使用 `info line 文件:行号` 判断该行是否有调试行表。

## WSLg 诊断

Qt 进程启动但无窗口时检查：

```bash
printf 'DISPLAY=%s WAYLAND_DISPLAY=%s\n' "$DISPLAY" "$WAYLAND_DISPLAY"
ls -l /mnt/wslg
```

这是显示通道问题，与 GDB 断点和业务代码不同。只有 Qt 明确报告平台插件加载失败时，`QT_DEBUG_PLUGINS=1` 才是有效诊断手段。
