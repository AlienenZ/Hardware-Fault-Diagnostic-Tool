# HW-Diag Installer Builder

> One-click tool to build a standard Windows installer (.exe) for the HW-Diag hardware diagnostic tool.
>
> 一键构建标准 Windows 安装包（.exe）的自动化工具。

---

## Quick Start / 快速开始

```
双击 build-installer.bat
Double-click build-installer.bat
```

脚本会自动完成以下步骤 / The script automatically:

1. 检测 `hw-diag\hw_diag.exe` 是否存在
   Check that `hw-diag\hw_diag.exe` exists

2. 检测 Inno Setup 6 是否已安装；若未安装则自动下载并静默安装
   Check if Inno Setup 6 is installed; if not, download and install it silently

3. 动态生成 Inno Setup 编译脚本（自动适配中/英文语言包）
   Dynamically generate the Inno Setup script (auto-detect Chinese/English language pack)

4. 编译生成 `output\HW-Diag-Setup.exe`
   Compile and output `output\HW-Diag-Setup.exe`

---

## Prerequisites / 前置条件

| 条件 / Requirement | 说明 / Notes |
|--------------------|-------------|
| Windows 10 / 11    | 64-bit system |
| 管理员权限 / Admin  | 用于安装 Inno Setup（bat 自动请求 UAC 提权） |
| `hw_diag.exe`       | 需先运行 `hw-diag\build.bat` 编译源码 |

> 如果 Inno Setup 下载失败（网络问题），可手动从 https://jrsoftware.org/isdl.php 安装后重新运行脚本。
>
> If Inno Setup download fails (network issue), manually install from the URL above and re-run the script.

---

## File Structure / 文件结构

```
installer/
├── build-installer.bat   # Entry point - double-click to run / 入口，双击运行
├── build-installer.ps1   # Build logic (PowerShell) / 构建逻辑
├── license.txt           # License agreement shown during install / 安装时显示的许可协议
└── test-env.bat          # [DEBUG] Environment check script, safe to delete / 环境诊断脚本，可删除
```

### Output (after build) / 输出目录

```
output/
└── HW-Diag-Setup.exe     # Final installer / 最终安装包
```

---

## Installer Features / 安装包特性

生成的 `HW-Diag-Setup.exe` 具备：

- Modern wizard-style UI / 现代向导界面
- Chinese + English language selection / 中英文语言选择（取决于系统语言包）
- Custom install path / 自定义安装路径
- Desktop shortcut (optional) / 可选创建桌面快捷方式
- Start menu group / 开始菜单程序组
- Built-in uninstaller / 内置卸载程序
- License agreement page / 许可协议页面
- Launch after install (optional) / 安装后可选直接启动

---

## Troubleshooting / 常见问题

**Q: bat 文件双击后闪退 / Window flashes and disappears**
A: 已修复。若仍出现，请右键选择「以管理员身份运行」。

**Q: Inno Setup 下载失败 / Download fails with 404**
A: 脚本会自动从官网抓取最新链接。若网络受限，手动安装 Inno Setup 6 后重新运行即可。

**Q: 安装包只有英文界面 / Installer shows English only**
A: Inno Setup 6 默认不带中文语言包，脚本会自动检测。如需中文界面，可从 [Inno Setup Translations](https://jrsoftware.org/files/istrans/) 下载 `ChineseSimplified.isl` 放入 Inno Setup 的 `Languages` 目录。
