#include "detect.h"
#include <windows.h>
#include <pdh.h>
#include <ctime>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "pdh.lib")

bool HardwareDetector::Init() {
    return wmi.Init();
}

std::wstring HardwareDetector::GetStatusStr(HwStatus s) {
    switch (s) {
        case HwStatus::Normal:  return L"✅ 正常";
        case HwStatus::Warning: return L"⚠️ 警告";
        case HwStatus::Error:   return L"❌ 故障";
        default:                return L"❓ 未知";
    }
}

static std::wstring FormatBytes(uint64_t bytes) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (bytes >= 1099511627776ULL)
        ss << (double)bytes / 1099511627776.0 << L" TB";
    else if (bytes >= 1073741824ULL)
        ss << (double)bytes / 1073741824.0 << L" GB";
    else if (bytes >= 1048576ULL)
        ss << (double)bytes / 1048576.0 << L" MB";
    else if (bytes >= 1024ULL)
        ss << (double)bytes / 1024.0 << L" KB";
    else
        ss << (double)bytes << L" B";
    return ss.str();
}

static std::wstring Fmt2(double v) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
}

static std::wstring RowGet(const std::vector<std::wstring>& r, const wchar_t* key) {
    for (size_t i = 0; i + 1 < r.size(); i += 2)
        if (r[i] == key) return r[i + 1];
    return L"";
}

// Get integer value from WMI VARIANT (handles signed/unsigned properly)
static int64_t RowGetInt(const std::vector<std::wstring>& r, const wchar_t* key) {
    std::wstring s = RowGet(r, key);
    if (s.empty()) return 0;
    return _wtoi64(s.c_str());
}

std::wstring HardwareDetector::LookupWmi(const std::wstring& wql, const std::wstring& field) {
    auto rows = wmi.Query(wql);
    for (auto& row : rows) {
        for (size_t i = 0; i + 1 < row.size(); i += 2) {
            if (row[i] == field && !row[i + 1].empty())
                return row[i + 1];
        }
    }
    return L"未检测到";
}

std::vector<std::wstring> HardwareDetector::GetWmiFieldValues(const std::wstring& wql, const std::wstring& field) {
    std::vector<std::wstring> vals;
    auto rows = wmi.Query(wql);
    for (auto& row : rows) {
        for (size_t i = 0; i + 1 < row.size(); i += 2) {
            if (row[i] == field)
                vals.push_back(row[i + 1].empty() ? L"未知" : row[i + 1]);
        }
    }
    return vals;
}

// ============================================================
// CPU
// ============================================================
void HardwareDetector::DetectCPU() {
    HwSection sec;
    sec.name = L"CPU 处器";
    sec.status = HwStatus::Normal;

    auto rows = wmi.Query(L"SELECT * FROM Win32_Processor");
    if (!rows.empty()) {
        auto& r = rows[0];
        sec.model = RowGet(r, L"Name");
        if (sec.model.empty()) sec.model = L"未知 CPU";

        std::wstring cores    = RowGet(r, L"NumberOfCores");
        std::wstring threads  = RowGet(r, L"NumberOfLogicalProcessors");
        std::wstring maxClock = RowGet(r, L"MaxClockSpeed");
        std::wstring curClock = RowGet(r, L"CurrentClockSpeed");
        std::wstring loadPct  = RowGet(r, L"LoadPercentage");
        std::wstring voltage  = RowGet(r, L"CurrentVoltage");
        std::wstring status   = RowGet(r, L"Status");

        sec.items.push_back(HwItem{L"型号", sec.model});
        sec.items.push_back(HwItem{L"核心数", cores + L" 核"});
        sec.items.push_back(HwItem{L"线程数", threads + L" 线程"});
        if (!maxClock.empty())
            sec.items.push_back(HwItem{L"最大频率", maxClock + L" MHz"});
        if (!curClock.empty())
            sec.items.push_back(HwItem{L"当前频率", curClock + L" MHz"});
        if (!voltage.empty()) {
            double v = _wtof(voltage.c_str()) / 10.0;
            sec.items.push_back(HwItem{L"当前电压", Fmt2(v) + L" V"});
        }
        if (!loadPct.empty()) {
            sec.items.push_back(HwItem{L"当前负载", loadPct + L"%"});
            // CPU Power = TDP * load% (use 65W as typical desktop CPU)
            double load = _wtof(loadPct.c_str());
            double cpuPower = 65.0 * load / 100.0;
            sec.items.push_back(HwItem{L"估算功率", Fmt2(cpuPower) + L" W"});
        }

        if (status == L"Degraded" || status == L"Error") {
            sec.status = HwStatus::Error;
            sec.statusText = L"CPU 状态异常: " + status;
            result.errorCount++;
        } else if (!loadPct.empty() && _wtoi(loadPct.c_str()) >= 95) {
            sec.status = HwStatus::Warning;
            sec.statusText = L"CPU 负载过高 (" + loadPct + L"%)";
            result.warnCount++;
        }
    }

    // CPU Temperature: Win32_PerfFormattedData_Counters_ThermalZoneInformation
    auto thermRows = wmi.Query(L"SELECT * FROM Win32_PerfFormattedData_Counters_ThermalZoneInformation");
    for (auto& r : thermRows) {
        std::wstring tempStr = RowGet(r, L"Temperature");
        if (!tempStr.empty()) {
            double raw = _wtof(tempStr.c_str());
            double celsius = raw / 10.0; // value is in decikelvins / 10
            if (celsius > 0 && celsius < 200) {
                sec.items.push_back(HwItem{L"温度", Fmt2(celsius) + L" °C"});
                if (celsius > 90) {
                    sec.status = HwStatus::Error;
                    sec.statusText = L"CPU 温度过高 (" + Fmt2(celsius) + L" °C)";
                    result.errorCount++;
                } else if (celsius > 80) {
                    sec.status = HwStatus::Warning;
                    sec.statusText = L"CPU 温度偏高 (" + Fmt2(celsius) + L" °C)";
                    result.warnCount++;
                }
                break;
            }
        }
    }

    // Fan speed: try Win32_Fan, then ACPI namespace
    auto fanRows = wmi.Query(L"SELECT * FROM Win32_Fan");
    if (!fanRows.empty()) {
        std::wstring speed = RowGet(fanRows[0], L"DesiredSpeed");
        if (!speed.empty()) {
            double rpm = _wtof(speed.c_str());
            sec.items.push_back(HwItem{L"风扇转速", Fmt2(rpm) + L" RPM"});
        }
    }
    // Try MSAcpi_ThermalZoneTemperature (requires admin)
    if (fanRows.empty()) {
        auto acpiRows = wmi.Query(L"SELECT * FROM MSAcpi_ThermalZoneTemperature");
        if (!acpiRows.empty()) {
            for (auto& r : acpiRows) {
                std::wstring t = RowGet(r, L"CurrentTemperature");
                if (!t.empty()) {
                    double raw = _wtof(t.c_str());
                    double celsius = raw / 10.0 - 273.15;
                    if (celsius > 0 && celsius < 200) {
                        // Already have temperature above, just note fan not available
                        break;
                    }
                }
            }
        }
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    if (!sec.statusText.empty())
        sec.items.push_back(HwItem{L"故障说明", sec.statusText});

    result.sections.push_back(sec);
}

// ============================================================
// Memory
// ============================================================
void HardwareDetector::DetectMemory() {
    HwSection sec;
    sec.name = L"内存 (RAM)";
    sec.status = HwStatus::Normal;

    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);

    sec.model = FormatBytes((uint64_t)mem.ullTotalPhys) + L" 内存";
    sec.items.push_back(HwItem{L"总容量", FormatBytes((uint64_t)mem.ullTotalPhys)});
    sec.items.push_back(HwItem{L"已使用", FormatBytes((uint64_t)(mem.ullTotalPhys - mem.ullAvailPhys))});
    sec.items.push_back(HwItem{L"可用", FormatBytes((uint64_t)mem.ullAvailPhys)});
    sec.items.push_back(HwItem{L"使用率", Fmt2((double)mem.dwMemoryLoad) + L"%"});

    auto rows = wmi.Query(L"SELECT * FROM Win32_PhysicalMemory");
    for (size_t idx = 0; idx < rows.size(); idx++) {
        auto& r = rows[idx];
        std::wstring speed = RowGet(r, L"Speed");
        std::wstring mfr   = RowGet(r, L"Manufacturer");
        std::wstring part  = RowGet(r, L"PartNumber");
        std::wstring cap   = RowGet(r, L"Capacity");
        std::wstring volt  = RowGet(r, L"ConfiguredVoltage");

        std::wstring label = L"内存条 #" + std::to_wstring(idx + 1);
        std::wstring val = part;
        if (!mfr.empty() && mfr != L"Unknown") val = mfr + L" " + val;
        if (!speed.empty()) val += L" (" + speed + L" MHz)";
        if (!cap.empty()) {
            uint64_t c = _wcstoui64(cap.c_str(), nullptr, 10);
            val += L" " + FormatBytes(c);
        }
        sec.items.push_back(HwItem{label, val});
        if (!volt.empty()) {
            double v = _wtof(volt.c_str());
            if (v > 1000) v /= 1000.0;
            sec.items.push_back(HwItem{L"  电压", Fmt2(v) + L" V"});
        }
    }

    // Memory power: ~4W per DIMM
    if (!rows.empty())
        sec.items.push_back(HwItem{L"估算功率", Fmt2(rows.size() * 4.0) + L" W"});

    if (mem.dwMemoryLoad >= 95) {
        sec.status = HwStatus::Warning;
        sec.statusText = L"内存使用率过高";
        result.warnCount++;
    }

    auto errRows = wmi.Query(L"SELECT * FROM Win32_MemoryDevice");
    for (auto& r : errRows) {
        for (size_t i = 0; i + 1 < r.size(); i += 2) {
            if (r[i] == L"Status" && r[i + 1] == L"Error") {
                sec.status = HwStatus::Error;
                sec.statusText = L"检测到内存硬件错误";
                result.errorCount++;
            }
        }
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    if (!sec.statusText.empty())
        sec.items.push_back(HwItem{L"故障说明", sec.statusText});

    result.sections.push_back(sec);
}

// ============================================================
// Disk
// ============================================================
void HardwareDetector::DetectDisk() {
    HwSection sec;
    sec.name = L"硬盘 存储";
    sec.status = HwStatus::Normal;

    auto rows = wmi.Query(L"SELECT * FROM Win32_DiskDrive");

    if (rows.empty()) {
        sec.model = L"未检测到硬盘";
        sec.status = HwStatus::Unknown;
    } else {
        sec.model = L"";
        for (size_t idx = 0; idx < rows.size(); idx++) {
            auto& r = rows[idx];
            std::wstring model     = RowGet(r, L"Model");
            std::wstring size      = RowGet(r, L"Size");
            std::wstring iface     = RowGet(r, L"InterfaceType");
            std::wstring mediaType = RowGet(r, L"MediaType");
            std::wstring status    = RowGet(r, L"Status");
            std::wstring serial    = RowGet(r, L"SerialNumber");
            std::wstring rpm       = RowGet(r, L"RotationRate");

            if (idx == 0) sec.model = model;
            std::wstring label = L"硬盘 #" + std::to_wstring(idx + 1);
            std::wstring val = model;
            if (!size.empty()) {
                uint64_t s = _wcstoui64(size.c_str(), nullptr, 10);
                val += L" (" + FormatBytes(s) + L")";
            }
            sec.items.push_back(HwItem{label, val});

            if (!iface.empty())
                sec.items.push_back(HwItem{L"  接口", iface});
            if (!mediaType.empty())
                sec.items.push_back(HwItem{L"  类型", mediaType});
            if (!serial.empty())
                sec.items.push_back(HwItem{L"  序列号", serial});
            if (!rpm.empty()) {
                double rVal = _wtof(rpm.c_str());
                if (rVal > 0)
                    sec.items.push_back(HwItem{L"  转速", Fmt2(rVal) + L" RPM"});
                else
                    sec.items.push_back(HwItem{L"  转速", L"SSD (无机械部件)"});
            }
            // Disk power
            bool isSSD = (mediaType.find(L"SSD") != std::wstring::npos ||
                          iface.find(L"USB") != std::wstring::npos);
            double diskPower = isSSD ? 3.0 : 8.0;
            sec.items.push_back(HwItem{L"  估算功率", Fmt2(diskPower) + L" W"});

            if (status != L"OK" && !status.empty()) {
                sec.status = HwStatus::Error;
                sec.statusText = L"硬盘状态异常: " + status;
                result.errorCount++;
            }
        }
    }

    // Logical disk free space
    auto ldRows = wmi.Query(L"SELECT * FROM Win32_LogicalDisk WHERE DriveType=3");
    for (auto& r : ldRows) {
        std::wstring drive = RowGet(r, L"DeviceID");
        std::wstring total = RowGet(r, L"Size");
        std::wstring free  = RowGet(r, L"FreeSpace");
        std::wstring fs    = RowGet(r, L"FileSystem");

        if (!drive.empty()) {
            uint64_t t = 0, f = 0;
            std::wstring info = L"总容量: ";
            if (!total.empty()) {
                t = _wcstoui64(total.c_str(), nullptr, 10);
                info += FormatBytes(t);
            }
            info += L" | 可用: ";
            if (!free.empty()) {
                f = _wcstoui64(free.c_str(), nullptr, 10);
                info += FormatBytes(f);
                if (t > 0) {
                    double pct = (double)f * 100.0 / (double)t;
                    info += L" (" + Fmt2(pct) + L"%)";
                    if (pct < 5.0 && sec.status == HwStatus::Normal) {
                        sec.status = HwStatus::Warning;
                        sec.statusText = drive + L" 剩余空间不足 5%";
                        result.warnCount++;
                    }
                }
            }
            if (!fs.empty()) info += L" | " + fs;
            sec.items.push_back(HwItem{drive, info});
        }
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    if (!sec.statusText.empty())
        sec.items.push_back(HwItem{L"故障说明", sec.statusText});

    result.sections.push_back(sec);
}

// ============================================================
// GPU
// ============================================================
void HardwareDetector::DetectGPU() {
    HwSection sec;
    sec.name = L"显卡 (GPU)";
    sec.status = HwStatus::Normal;

    auto rows = wmi.Query(L"SELECT * FROM Win32_VideoController");

    if (rows.empty()) {
        sec.model = L"未检测到显卡";
        sec.status = HwStatus::Unknown;
    } else {
        sec.model = L"";
        for (size_t idx = 0; idx < rows.size(); idx++) {
            auto& r = rows[idx];
            std::wstring name    = RowGet(r, L"Name");
            std::wstring driver  = RowGet(r, L"DriverVersion");
            std::wstring status  = RowGet(r, L"Status");
            std::wstring drvDate = RowGet(r, L"DriverDate");

            if (idx == 0) sec.model = name;
            sec.items.push_back(HwItem{L"显卡 #" + std::to_wstring(idx + 1), name});

            if (!driver.empty())
                sec.items.push_back(HwItem{L"  驱动版本", driver});
            if (!drvDate.empty() && drvDate.size() >= 8) {
                std::wstring dt = drvDate.substr(0, 4) + L"-" + drvDate.substr(4, 2) + L"-" + drvDate.substr(6, 2);
                sec.items.push_back(HwItem{L"  驱动日期", dt});
            }

            // Fix VRAM: use QueryField to get raw uint32, cast to unsigned
            VARIANT vt = wmi.QueryField(
                L"SELECT AdapterRAM FROM Win32_VideoController WHERE DeviceID='" +
                RowGet(r, L"DeviceID") + L"'", L"AdapterRAM");
            uint64_t vramBytes = 0;
            if (vt.vt == VT_UI4)
                vramBytes = (uint64_t)(uint32_t)vt.ulVal;
            else if (vt.vt == VT_I4)
                vramBytes = (uint64_t)(uint32_t)vt.intVal;
            else if (vt.vt == VT_UI8)
                vramBytes = vt.ullVal;
            else if (vt.vt == VT_I8)
                vramBytes = (uint64_t)vt.ullVal;
            VariantClear(&vt);

            if (vramBytes > 0)
                sec.items.push_back(HwItem{L"  显存", FormatBytes(vramBytes)});

            // GPU power estimate
            double gpuPower = 0;
            if (vramBytes >= 8ULL * 1073741824)      gpuPower = 250.0;
            else if (vramBytes >= 6ULL * 1073741824)  gpuPower = 200.0;
            else if (vramBytes >= 4ULL * 1073741824)  gpuPower = 150.0;
            else if (vramBytes >= 2ULL * 1073741824)  gpuPower = 120.0;
            else if (vramBytes >= 1073741824ULL)       gpuPower = 75.0;
            else                                       gpuPower = 35.0;
            sec.items.push_back(HwItem{L"  估算功率", Fmt2(gpuPower) + L" W"});

            if (status != L"OK" && !status.empty()) {
                sec.status = HwStatus::Error;
                sec.statusText = L"显卡状态异常: " + status;
                result.errorCount++;
            }
        }
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    if (!sec.statusText.empty())
        sec.items.push_back(HwItem{L"故障说明", sec.statusText});

    result.sections.push_back(sec);
}

// ============================================================
// Motherboard
// ============================================================
void HardwareDetector::DetectMotherboard() {
    HwSection sec;
    sec.name = L"主板 / BIOS";
    sec.status = HwStatus::Normal;

    auto bbRows = wmi.Query(L"SELECT * FROM Win32_BaseBoard");
    if (!bbRows.empty()) {
        auto& r = bbRows[0];
        std::wstring mfr     = RowGet(r, L"Manufacturer");
        std::wstring product = RowGet(r, L"Product");
        std::wstring serial  = RowGet(r, L"SerialNumber");
        std::wstring status  = RowGet(r, L"Status");

        sec.model = mfr + L" " + product;
        sec.items.push_back(HwItem{L"制造商", mfr});
        sec.items.push_back(HwItem{L"型号", product});
        if (!serial.empty())
            sec.items.push_back(HwItem{L"序列号", serial});

        if (status != L"OK" && !status.empty()) {
            sec.status = HwStatus::Error;
            sec.statusText = L"主板状态异常: " + status;
            result.errorCount++;
        }
    }

    auto biosRows = wmi.Query(L"SELECT * FROM Win32_BIOS");
    if (!biosRows.empty()) {
        auto& r = biosRows[0];
        std::wstring biosVendor = RowGet(r, L"Manufacturer");
        std::wstring biosVer    = RowGet(r, L"SMBIOSBIOSVersion");
        std::wstring biosDate   = RowGet(r, L"ReleaseDate");

        if (!biosVendor.empty())
            sec.items.push_back(HwItem{L"BIOS 厂商", biosVendor});
        if (!biosVer.empty())
            sec.items.push_back(HwItem{L"BIOS 版本", biosVer});
        if (!biosDate.empty() && biosDate.size() >= 8) {
            std::wstring dt = biosDate.substr(0, 4) + L"-" + biosDate.substr(4, 2) + L"-" + biosDate.substr(6, 2);
            sec.items.push_back(HwItem{L"BIOS 日期", dt});
        }
    }

    auto sysRows = wmi.Query(L"SELECT * FROM Win32_ComputerSystem");
    if (!sysRows.empty()) {
        auto& r = sysRows[0];
        std::wstring sysName  = RowGet(r, L"Name");
        std::wstring sysMfr   = RowGet(r, L"Manufacturer");
        std::wstring sysModel = RowGet(r, L"Model");

        if (!sysName.empty())
            sec.items.push_back(HwItem{L"计算机名", sysName});
        if (!sysMfr.empty() && !sysModel.empty())
            sec.items.push_back(HwItem{L"系统型号", sysMfr + L" " + sysModel});
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    result.sections.push_back(sec);
}

// ============================================================
// Battery
// ============================================================
void HardwareDetector::DetectBattery() {
    HwSection sec;
    sec.name = L"电池";
    sec.status = HwStatus::Normal;

    auto rows = wmi.Query(L"SELECT * FROM Win32_Battery");
    if (rows.empty()) {
        sec.model = L"未检测到电池";
        sec.status = HwStatus::Unknown;
        sec.items.push_back(HwItem{L"设备", L"此设备无电池"});
        result.sections.push_back(sec);
        return;
    }

    auto& r = rows[0];
    std::wstring name         = RowGet(r, L"Name");
    std::wstring status       = RowGet(r, L"Status");
    std::wstring chem         = RowGet(r, L"Chemistry");
    std::wstring estCharge    = RowGet(r, L"EstimatedChargeRemaining");
    std::wstring batteryStatus = RowGet(r, L"BatteryStatus");

    sec.model = name.empty() ? L"笔记本电池" : name;
    sec.items.push_back(HwItem{L"型号", sec.model});

    if (!estCharge.empty())
        sec.items.push_back(HwItem{L"当前电量", Fmt2(_wtof(estCharge.c_str())) + L"%"});

    // Chemistry
    if (!chem.empty()) {
        std::wstring chemStr;
        int c = _wtoi(chem.c_str());
        switch (c) {
            case 1: chemStr = L"其他"; break;
            case 2: chemStr = L"锂聚合物电池"; break;
            case 3: chemStr = L"锂离子电池"; break;
            case 4: chemStr = L"镍铁电池"; break;
            case 5: chemStr = L"镍镉电池"; break;
            case 6: chemStr = L"锂聚合物电池"; break;
            case 7: chemStr = L"锌空气电池"; break;
            case 8: chemStr = L"镍氢电池"; break;
            default: chemStr = chem; break;
        }
        sec.items.push_back(HwItem{L"电池类型", chemStr});
    }

    // Battery status
    std::wstring bsStr;
    int bs = _wtoi(batteryStatus.c_str());
    switch (bs) {
        case 1:  bsStr = L"正在放电"; break;
        case 2:  bsStr = L"AC 电源供电"; break;
        case 3:  bsStr = L"已充满"; break;
        case 4:  bsStr = L"低电量"; break;
        case 5:  bsStr = L"电量过低"; break;
        case 6:  bsStr = L"正在充电"; break;
        case 7:  bsStr = L"充电中"; break;
        case 8:  bsStr = L"充电中"; break;
        case 10: bsStr = L"部分充电"; break;
        default: bsStr = L"未知 (" + batteryStatus + L")"; break;
    }
    sec.items.push_back(HwItem{L"充放电状态", bsStr});

    // DesignCapacity / FullChargeCapacity: Win32_PortableBattery
    auto portRows = wmi.Query(L"SELECT * FROM Win32_PortableBattery");
    if (!portRows.empty()) {
        auto& pr = portRows[0];
        std::wstring designCap = RowGet(pr, L"DesignCapacity");
        std::wstring fullCap   = RowGet(pr, L"FullChargeCapacity");
        std::wstring mfr       = RowGet(pr, L"Manufacturer");
        std::wstring location  = RowGet(pr, L"Location");

        if (!mfr.empty())
            sec.items.push_back(HwItem{L"制造商", mfr});
        if (!location.empty())
            sec.items.push_back(HwItem{L"位置", location});

        double dc = _wtof(designCap.c_str());
        double fc = _wtof(fullCap.c_str());

        if (dc > 0) {
            sec.items.push_back(HwItem{L"设计容量", Fmt2(dc) + L" mWh"});
            if (fc > 0) {
                sec.items.push_back(HwItem{L"当前满充", Fmt2(fc) + L" mWh"});
                double wearPct = (1.0 - fc / dc) * 100.0;
                sec.items.push_back(HwItem{L"电池损耗", Fmt2(wearPct) + L"%"});
                if (wearPct > 40.0) {
                    sec.status = HwStatus::Warning;
                    sec.statusText = L"电池损耗超过 40%";
                    result.warnCount++;
                }
            }
        }
    }

    // Power
    if (bs == 1) {
        sec.items.push_back(HwItem{L"当前功率", L"放电中"});
    } else if (bs == 2 || bs == 6 || bs == 7) {
        sec.items.push_back(HwItem{L"当前功率", L"充电中 (AC 供电)"});
    }

    if (bs == 4 || bs == 5) {
        sec.status = HwStatus::Warning;
        sec.statusText = L"电池电量过低";
        result.warnCount++;
    }

    if (status != L"OK" && !status.empty() && status != L"正常") {
        sec.status = HwStatus::Error;
        sec.statusText = L"电池状态异常: " + status;
        result.errorCount++;
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    if (!sec.statusText.empty())
        sec.items.push_back(HwItem{L"故障说明", sec.statusText});

    result.sections.push_back(sec);
}

// ============================================================
// Network
// ============================================================
void HardwareDetector::DetectNetwork() {
    HwSection sec;
    sec.name = L"网络适配器";
    sec.status = HwStatus::Normal;

    auto rows = wmi.Query(L"SELECT * FROM Win32_NetworkAdapter WHERE PhysicalAdapter=TRUE");

    if (rows.empty()) {
        sec.model = L"未检测到网络适配器";
        sec.status = HwStatus::Unknown;
    } else {
        sec.model = L"网络适配器";
        bool hasIssue = false;
        for (auto& r : rows) {
            std::wstring name        = RowGet(r, L"Name");
            std::wstring mac         = RowGet(r, L"MACAddress");
            std::wstring netStatus   = RowGet(r, L"NetConnectionStatus");
            std::wstring speed       = RowGet(r, L"Speed");
            std::wstring adapterType = RowGet(r, L"AdapterType");

            sec.items.push_back(HwItem{name, adapterType});

            if (!mac.empty())
                sec.items.push_back(HwItem{L"  MAC 地址", mac});

            std::wstring connStr;
            int cs = _wtoi(netStatus.c_str());
            switch (cs) {
                case 0: connStr = L"已断开"; break;
                case 1: connStr = L"正在连接"; break;
                case 2: connStr = L"已连接"; break;
                case 3: connStr = L"断开中"; break;
                case 4: connStr = L"无信号"; break;
                case 5: connStr = L"受限"; break;
                default: connStr = L"未知 (" + netStatus + L")"; break;
            }
            sec.items.push_back(HwItem{L"  连接状态", connStr});

            if (!speed.empty()) {
                double bps = _wtof(speed.c_str());
                if (bps > 0)
                    sec.items.push_back(HwItem{L"  链路速度", Fmt2(bps / 1000000.0) + L" Mbps"});
            }

            if (cs == 0 && adapterType.find(L"Virtual") == std::wstring::npos)
                hasIssue = true;
        }

        if (hasIssue) {
            sec.status = HwStatus::Warning;
            sec.statusText = L"部分网络适配器已断开";
            result.warnCount++;
        }
    }

    sec.items.push_back(HwItem{L"状态", GetStatusStr(sec.status)});
    if (!sec.statusText.empty())
        sec.items.push_back(HwItem{L"故障说明", sec.statusText});

    result.sections.push_back(sec);
}

// ============================================================
// RunAll
// ============================================================
DetectResult HardwareDetector::RunAll() {
    result = DetectResult();
    result.errorCount = 0;
    result.warnCount = 0;

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[64];
    swprintf_s(buf, L"%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    result.time = buf;

    DetectCPU();
    DetectMemory();
    DetectDisk();
    DetectGPU();
    DetectMotherboard();
    DetectBattery();
    DetectNetwork();

    return result;
}
