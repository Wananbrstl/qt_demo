#!/usr/bin/env bash
set -euo pipefail

# Run this script from an Ubuntu WSL terminal. Package installation is kept
# explicit because sudo credentials must never be embedded in project files.
if ! grep -qi microsoft /proc/version; then
    echo "warning: this script was designed for WSL2 Ubuntu" >&2
fi

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    gdb \
    qt6-base-dev \
    qt6-base-dev-tools \
    libqt6sql6-sqlite

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "${project_root}"

cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

echo
echo "DeviceStudio is ready."
echo "Simulator: ${project_root}/build/linux-debug/DeviceSimulator --port 45454"
echo "GUI:       ${project_root}/build/linux-debug/DeviceStudio"
