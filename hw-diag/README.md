# 硬件故障检测工具 / Hardware Fault Diagnostic Tool

> 一款轻量级的 Windows 硬件检测工具，基于 WMI 查询实现，提供图形化界面，可快速检测硬件状态并导出检测报告。
>
> A lightweight Windows hardware diagnostic tool built on WMI, featuring a native Win32 GUI for real-time hardware status detection and report export.

---

## 快速安装 / Quick Install

双击运行 `installer/build-installer.bat`，自动完成：
- 下载并安装 Inno Setup 6
- 编译生成标准 Windows 安装包
- 输出到 `output/HW-Diag-Setup.exe`

Double-click `installer/build-installer.bat` to automatically:
- Download and install Inno Setup 6
- Compile a standard Windows installer
- Output to `output/HW-Diag-Setup.exe`

---

## 功能特性 / Features

| 功能 | Feature |
|------|---------|
| CPU 检测（型号/核心/线程/频率/温度/负载/功耗） | CPU detection (model, cores, threads, frequency, temperature, load, power) |
| 内存检测（容量/使用率/每条内存详情/电压/功耗） | Memory detection (capacity, usage, per-DIMM details, voltage, power) |
| 硬盘检测（型号/接口/容量/分区空间/转速/功耗） | Disk detection (model, interface, capacity, partition space, RPM, power) |
| 显卡检测（型号/驱动版本/显存/功耗） | GPU detection (model, driver version, VRAM, power) |
| 主板/BIOS 信息（厂商/型号/版本/日期） | Motherboard & BIOS info (manufacturer, model, version, date) |
| 电池检测（电量/充放电状态/损耗率/设计容量） | Battery detection (charge, charge/discharge status, wear level, design capacity) |
| 网络适配器检测（连接状态/链路速度/MAC 地址） | Network adapter detection (connection status, link speed, MAC address) |
| 导出 HTML / TXT 格式检测报告 | Export diagnostic report as HTML or TXT |
| 故障与警告自动标注 | Automatic fault and warning annotation |

---

## 截图 / Screenshots

程序启动后点击「开始检测」即可扫描所有硬件，左侧树形导航可切换查看各项详情：

After launching, click "开始检测" (Start Detection) to scan all hardware. Use the left tree panel to navigate between sections:

```
+----------------------------------------------------------+
|  [🔍 开始检测]  [🔄 刷新]  [📄 导出报告]                 |
+-------------+--------------------------------------------+
| 检测总览     |  === CPU 处理器 ===                        |
| [OK] CPU    |                                            |
| [!] 内存    |  设备型号:  Intel Core i7-12700K           |
| [OK] 硬盘   |  当前状态:  [OK] 正常                      |
| [OK] 显卡   |  ----------------------------------------  |
| [OK] 主板   |  型号        Intel Core i7-12700K          |
| [?] 电池    |  核心数      12 核                          |
| [!] 网络    |  线程数      20 线程                        |
|             |  最大频率    5000 MHz                       |
|             |  当前温度    45.00 °C                       |
+-------------+--------------------------------------------+
| 检测时间: 2026-06-14 | 检测项目: 7 项 | [!] 警告: 2 项  |
+----------------------------------------------------------+
```

---

## 编译与运行 / Build & Run

### 环境要求 / Prerequisites

- **操作系统**: Windows 10 / 11
- **编译器**: MSYS2 MinGW-w64 (g++ with C++17 support)
- **依赖库**: 系统内置 (ole32, oleaut32, wbemuuid, comctl32, shell32, comdlg32, pdh)

### 编译步骤 / Build Steps

1. 安装 [MSYS2](https://www.msys2.org/)，打开 UCRT64 终端，安装 g++：

   Install [MSYS2](https://www.msys2.org/), open UCRT64 terminal and install g++:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc
   ```

2. 进入 `hw-diag` 目录，运行编译脚本：

   Enter the `hw-diag` directory and run the build script:

   ```bash
   cd hw-diag
   build.bat
   ```

   或手动编译 / Or compile manually:

   ```bash
   g++ -std=c++17 -O2 -static -DUNICODE -D_UNICODE -mwindows -municode \
       main.cpp gui.cpp wmi.cpp detect.cpp report.cpp \
       -lole32 -loleaut32 -lwbemuuid -lcomctl32 -lshell32 -lcomdlg32 -lpdh \
       -o hw_diag.exe
   ```

3. 双击 `hw_diag.exe` 运行 / Double-click `hw_diag.exe` to run.

> **静态链接**: 生成的 exe 无需额外 DLL，可直接分发使用。
>
> **Statically linked**: The resulting exe requires no external DLLs and is fully portable.

---

## 使用说明 / Usage

1. **启动程序** — 双击 `hw_diag.exe`
   **Launch** — Double-click `hw_diag.exe`

2. **开始检测** — 点击左上角「🔍 开始检测」按钮，程序将自动扫描所有硬件
   **Start detection** — Click the "🔍 开始检测" button to scan all hardware

3. **查看详情** — 在左侧树形列表中点击各硬件项，右侧面板展示详细信息
   **View details** — Click any hardware item in the left tree to see details on the right

4. **刷新检测** — 点击「🔄 刷新」重新检测
   **Refresh** — Click "🔄 刷新" to re-scan

5. **导出报告** — 点击「📄 导出报告」，可选择保存为 HTML 或 TXT 格式
   **Export report** — Click "📄 导出报告" to save as HTML or TXT format

---

## 项目结构 / Project Structure

```
hw-diag/
├── main.cpp          # 程序入口 / Entry point
├── gui.h / gui.cpp   # Win32 GUI 界面 / GUI layer (TreeView, RichEdit, StatusBar)
├── detect.h / detect.cpp  # 硬件检测逻辑 / Hardware detection logic (CPU, RAM, Disk, GPU, etc.)
├── wmi.h / wmi.cpp   # WMI 查询封装 / WMI query wrapper
├── report.h / report.cpp  # 报告导出 (HTML / TXT) / Report export (HTML / TXT)
├── build.bat         # 一键编译脚本 / One-click build script
└── hw_diag.exe       # 预编译可执行文件 / Pre-built executable
```

---

## 检测原理 / How It Works

程序通过 **WMI (Windows Management Instrumentation)** 接口查询系统硬件信息，核心查询类位于 `wmi.cpp`。检测模块 (`detect.cpp`) 使用以下 WMI 类：

The tool queries hardware information via **Windows Management Instrumentation (WMI)**. The detection module (`detect.cpp`) uses the following WMI classes:

| WMI 类 / Class | 用途 / Purpose |
|----------------|---------------|
| `Win32_Processor` | CPU 型号、核心数、频率、负载 / CPU model, cores, frequency, load |
| `Win32_PhysicalMemory` | 内存条详情 / Physical memory details |
| `Win32_DiskDrive` | 硬盘信息 / Physical disk info |
| `Win32_LogicalDisk` | 分区空间 / Logical disk space |
| `Win32_VideoController` | 显卡信息 / GPU info |
| `Win32_BaseBoard` / `Win32_BIOS` | 主板与 BIOS / Motherboard and BIOS |
| `Win32_Battery` / `Win32_PortableBattery` | 电池状态 / Battery status |
| `Win32_NetworkAdapter` | 网络适配器 / Network adapters |
| `Win32_PerfFormattedData_Counters_ThermalZoneInformation` | CPU 温度 / CPU temperature |

温度检测、负载阈值、电池损耗等异常状态会自动标记为 **警告** 或 **故障**。

Abnormal conditions (temperature spikes, high load, battery wear) are automatically flagged as **Warning** or **Error**.

---

## 状态说明 / Status Levels

| 状态 / Status | 含义 / Meaning |
|---------------|---------------|
| ✅ 正常 / Normal | 硬件工作正常 / Hardware functioning normally |
| ⚠️ 警告 / Warning | 存在潜在问题，建议关注 / Potential issue detected, attention recommended |
| ❌ 故障 / Error | 检测到异常，需要排查 / Abnormality detected, investigation needed |
| ❓ 未知 / Unknown | 无法获取状态信息 / Status information unavailable |

---

## 注意事项 / Notes

- 部分检测项（如 ACPI 温度）需要 **管理员权限** 才能获取完整数据
  Some detection items (e.g., ACPI thermal zone) require **administrator privileges** for full data

- CPU 温度读取依赖系统 BIOS/UEFI 支持，部分机型可能无法获取
  CPU temperature reading depends on BIOS/UEFI support; some systems may not report it

- 功耗数据为基于硬件参数的 **估算值**，非实测数据
  Power consumption values are **estimates** based on hardware specifications, not actual measurements

- 电池检测仅适用于笔记本电脑，台式机将显示「未检测到电池」
  Battery detection applies to laptops only; desktops will show "未检测到电池"

---

## 许可证 / License

本项目仅供学习和个人使用。
This project is for educational and personal use only.
