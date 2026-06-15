# HW-Diag - 硬件故障检测工具

> A lightweight Windows hardware diagnostic tool with GUI, built on WMI.
>
> 一款基于 WMI 的轻量级 Windows 硬件故障检测工具，提供图形化界面，支持一键生成安装包。

---

## Project Structure / 项目结构

```
project_root/
├── hw-diag/                  # Application source code / 应用源码
│   ├── main.cpp              # Entry point / 程序入口
│   ├── gui.h / gui.cpp       # Win32 GUI (TreeView, RichEdit, StatusBar)
│   ├── detect.h / detect.cpp # Hardware detection logic / 硬件检测逻辑
│   ├── wmi.h / wmi.cpp       # WMI query wrapper / WMI 查询封装
│   ├── report.h / report.cpp # Report export (HTML / TXT) / 报告导出
│   ├── build.bat             # Source build script / 源码编译脚本
│   └── hw_diag.exe           # Pre-built executable / 预编译可执行文件
│
├── installer/                # Installer builder / 安装包构建工具
│   ├── build-installer.bat   # One-click installer build / 一键构建安装包
│   ├── build-installer.ps1   # Build logic (PowerShell) / 构建逻辑
│   └── license.txt           # License for installer / 安装许可协议
│
└── output/                   # (generated) Build output / 构建输出目录
    └── HW-Diag-Setup.exe     # Final Windows installer / 最终安装包
```

---

## Quick Start / 快速开始

### Option A: Run directly / 直接运行

```
hw-diag\hw_diag.exe
```

Double-click the pre-built executable. No installation required.
双击预编译好的可执行文件即可使用，无需安装。

### Option B: Build from source / 从源码编译

1. Install [MSYS2](https://www.msys2.org/) with g++ (C++17)
2. Run `hw-diag\build.bat`
3. Output: `hw-diag\hw_diag.exe`

### Option C: Build a Windows installer / 构建安装包

```
installer\build-installer.bat
```

Automatically downloads Inno Setup if needed, compiles and outputs `output\HW-Diag-Setup.exe`.
自动下载 Inno Setup，编译生成标准 Windows 安装包。

---

## Features / 功能特性

| Category | Items |
|----------|-------|
| **CPU** | Model, cores, threads, frequency, temperature, load, power estimate |
| **Memory** | Total/used/available, per-DIMM details (manufacturer, speed, voltage) |
| **Disk** | Model, interface, capacity, partition usage, RPM, power estimate |
| **GPU** | Model, driver version/date, VRAM, power estimate |
| **Motherboard** | Manufacturer, model, BIOS version/date, system info |
| **Battery** | Charge level, charge/discharge status, wear percentage, design capacity |
| **Network** | Adapter name/type, connection status, link speed, MAC address |
| **Report** | Export diagnostic report as HTML or TXT |

---

## How It Works / 工作原理

The tool uses **WMI (Windows Management Instrumentation)** to query hardware information.
通过 WMI 接口读取系统硬件信息，无需安装额外驱动或服务。

Key WMI classes used:
- `Win32_Processor` - CPU info
- `Win32_PhysicalMemory` - RAM details
- `Win32_DiskDrive` / `Win32_LogicalDisk` - Storage
- `Win32_VideoController` - GPU
- `Win32_BaseBoard` / `Win32_BIOS` - Motherboard
- `Win32_Battery` / `Win32_PortableBattery` - Battery
- `Win32_NetworkAdapter` - Network adapters

Anomalies (high temperature, high load, disk space low, battery wear) are automatically flagged as **Warning** or **Error**.
异常状态（温度过高、负载过高、磁盘空间不足、电池损耗）自动标记为警告或故障。

---

## Tech Stack / 技术栈

| Component | Technology |
|-----------|-----------|
| Language | C++17 |
| GUI | Win32 API (native controls) |
| Data source | WMI (COM) |
| Compiler | MSYS2 MinGW-w64 g++ |
| Linking | Static (no external DLLs) |
| Installer | Inno Setup 6 |

---

## Notes / 注意事项

- CPU temperature requires BIOS/UEFI support; some systems may not report it
  CPU 温度依赖 BIOS 支持，部分机型无法读取

- Power values are estimates based on hardware specs, not actual measurements
  功耗为基于硬件参数的估算值，非实测

- Battery detection is for laptops only; desktops show "not detected"
  电池检测仅适用于笔记本

- Some WMI queries (ACPI thermal zone) may require administrator privileges
  部分 WMI 查询需要管理员权限

---

## License / 许可证

This project is for educational and personal use only.
本项目仅供学习和个人使用。
