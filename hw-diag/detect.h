#pragma once
#include <string>
#include <vector>
#include "wmi.h"

enum class HwStatus { Normal, Warning, Error, Unknown };

struct HwItem {
    std::wstring label;
    std::wstring value;
};

struct HwSection {
    std::wstring name;
    std::wstring model;
    HwStatus status = HwStatus::Unknown;
    std::wstring statusText;
    std::vector<HwItem> items;
};

struct DetectResult {
    std::wstring time;
    std::vector<HwSection> sections;
    int errorCount = 0;
    int warnCount = 0;
};

class HardwareDetector {
public:
    bool Init();
    DetectResult RunAll();

private:
    WmiQuery wmi;
    DetectResult result;

    void DetectCPU();
    void DetectMemory();
    void DetectDisk();
    void DetectGPU();
    void DetectMotherboard();
    void DetectBattery();
    void DetectNetwork();

    std::wstring LookupWmi(const std::wstring& wql, const std::wstring& field);
    std::vector<std::wstring> GetWmiFieldValues(const std::wstring& wql, const std::wstring& field);
    std::wstring GetStatusStr(HwStatus s);
};
