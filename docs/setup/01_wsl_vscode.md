# WSL2, VS Code, CMake, and GDB setup

This project must be opened through **VS Code Remote - WSL**, with the folder
`/home/wananbrstl/projects/DeviceStudio`. Do not open the Windows path through a
normal Windows VS Code process: that mixes Windows extensions with Linux paths,
compilers, shared libraries, and debug symbols.

## Bootstrap

From Ubuntu WSL:

```bash
cd /home/wananbrstl/projects/DeviceStudio
bash scripts/bootstrap-wsl.sh
```

The script installs the reproducible toolchain, configures the Debug preset,
builds every target, and runs the tests. It intentionally invokes ordinary
`sudo`; enter the password interactively and never store it in a task or file.

## VS Code extensions

Install these extensions **in WSL: Ubuntu**, not only on Windows:

- CMake Tools (`ms-vscode.cmake-tools`)
- C/C++ (`ms-vscode.cpptools`), used by the `cppdbg` GDB adapter
- clangd (`llvm-vs-code-extensions.vscode-clangd`), used for code navigation

The project disables the Microsoft IntelliSense engine to avoid running two
language servers, but keeps the C/C++ extension for debugging.

## Build and debug contract

The `linux-debug` CMake preset always produces unstripped Debug binaries under
`build/linux-debug`. The launch configurations have a `preLaunchTask`, so F5
first configures and builds the same binary that GDB launches. This prevents the
common failure where breakpoints refer to current source but the debugger runs
an older executable.

Useful terminal checks:

```bash
file build/linux-debug/DeviceStudio
readelf -S build/linux-debug/DeviceStudio | grep debug_info
gdb -q -batch -ex 'info sources' build/linux-debug/DeviceStudio
```

`file` should mention `with debug_info, not stripped`; `readelf` should find a
`.debug_info` section. Source paths printed by GDB should begin with the WSL
project path.

## Recommended workflow

1. Run the `DeviceStudio: test debug` task after protocol or database changes.
2. Select the `DeviceStudio + Simulator` compound configuration for integration
   debugging, or start the simulator in a terminal when only the GUI needs GDB.
3. Put the first breakpoint in a function body, not a declaration or a line that
   was optimized away.
4. When a breakpoint is hollow, inspect the Debug Console for the exact source
   path and compare it with `info sources` before changing debugger settings.

## WSLg diagnostics

Qt Widgets normally appears through WSLg. If the process starts but no window is
visible, check:

```bash
printf 'DISPLAY=%s WAYLAND_DISPLAY=%s\n' "$DISPLAY" "$WAYLAND_DISPLAY"
ls -l /mnt/wslg
```

This is a display-path problem, distinct from a GDB breakpoint or Qt plugin
problem. Setting `QT_DEBUG_PLUGINS=1` is useful only when Qt reports a platform
plugin loading error.
