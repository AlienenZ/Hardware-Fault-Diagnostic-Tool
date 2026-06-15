#include "report.h"
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>

static std::string WToA(const std::wstring& ws) {
    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], len, NULL, NULL);
    return s;
}

static std::string StatusColor(HwStatus s) {
    switch (s) {
        case HwStatus::Normal:  return "#16a34a";
        case HwStatus::Warning: return "#d97706";
        case HwStatus::Error:   return "#dc2626";
        default:                return "#6b7280";
    }
}

static std::string StatusText(HwStatus s) {
    switch (s) {
        case HwStatus::Normal:  return "正常";
        case HwStatus::Warning: return "警告";
        case HwStatus::Error:   return "故障";
        default:                return "未知";
    }
}

static std::string StatusIcon(HwStatus s) {
    switch (s) {
        case HwStatus::Normal:  return "&#x2705;";
        case HwStatus::Warning: return "&#x26A0;&#xFE0F;";
        case HwStatus::Error:   return "&#x274C;";
        default:                return "&#x2753;";
    }
}

bool ExportHtml(const std::wstring& path, const DetectResult& result) {
    std::ofstream f(WToA(path));
    if (!f.is_open()) return false;

    f << "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='utf-8'>\n";
    f << "<title>硬件检测报告</title>\n";
    f << "<style>\n";
    f << "* { margin:0; padding:0; box-sizing:border-box; }\n";
    f << "body { font-family: 'Microsoft YaHei','Segoe UI',sans-serif; background:#f0f2f5; color:#333; padding:20px; }\n";
    f << ".container { max-width:900px; margin:0 auto; }\n";
    f << ".header { background:linear-gradient(135deg,#1e3a5f,#2563eb); color:#fff; padding:30px; border-radius:12px 12px 0 0; }\n";
    f << ".header h1 { font-size:24px; margin-bottom:8px; }\n";
    f << ".header .time { font-size:13px; opacity:0.8; }\n";
    f << ".summary { background:#fff; padding:20px 30px; border-bottom:1px solid #e5e7eb; }\n";
    f << ".summary-grid { display:flex; gap:15px; flex-wrap:wrap; }\n";
    f << ".summary-item { flex:1; min-width:100px; text-align:center; padding:12px; border-radius:8px; }\n";
    f << ".summary-item.normal { background:#ecfdf5; color:#16a34a; }\n";
    f << ".summary-item.warning { background:#fffbeb; color:#d97706; }\n";
    f << ".summary-item.error { background:#fef2f2; color:#dc2626; }\n";
    f << ".summary-item .num { font-size:28px; font-weight:bold; }\n";
    f << ".summary-item .label { font-size:12px; margin-top:4px; }\n";
    f << ".card { background:#fff; margin:0; padding:25px 30px; border-bottom:1px solid #e5e7eb; }\n";
    f << ".card:last-child { border-radius:0 0 12px 12px; border-bottom:none; }\n";
    f << ".card-header { display:flex; justify-content:space-between; align-items:center; margin-bottom:15px; }\n";
    f << ".card-header h2 { font-size:18px; color:#1e3a5f; }\n";
    f << ".badge { display:inline-block; padding:3px 12px; border-radius:20px; font-size:12px; font-weight:bold; }\n";
    f << ".model { color:#6b7280; font-size:14px; margin-bottom:12px; }\n";
    f << "table { width:100%; border-collapse:collapse; }\n";
    f << "td { padding:6px 0; border-bottom:1px solid #f3f4f6; font-size:13px; }\n";
    f << "td:first-child { color:#6b7280; width:150px; }\n";
    f << ".fault { background:#fef2f2; color:#dc2626; padding:10px; border-radius:6px; margin-top:10px; font-size:13px; }\n";
    f << ".footer { text-align:center; color:#9ca3af; font-size:12px; margin-top:20px; padding:20px; }\n";
    f << "</style></head><body><div class='container'>\n";

    // Header
    f << "<div class='header'><h1>硬件检测报告</h1>";
    f << "<div class='time'>检测时间: " << WToA(result.time) << "</div></div>\n";

    // Summary
    int normalCount = 0;
    for (auto& s : result.sections)
        if (s.status == HwStatus::Normal) normalCount++;

    f << "<div class='summary'><div class='summary-grid'>\n";
    f << "<div class='summary-item normal'><div class='num'>" << normalCount << "</div><div class='label'>正常</div></div>\n";
    if (result.warnCount > 0)
        f << "<div class='summary-item warning'><div class='num'>" << result.warnCount << "</div><div class='label'>警告</div></div>\n";
    if (result.errorCount > 0)
        f << "<div class='summary-item error'><div class='num'>" << result.errorCount << "</div><div class='label'>故障</div></div>\n";
    f << "</div></div>\n";

    // Sections
    for (auto& sec : result.sections) {
        std::string color = StatusColor(sec.status);
        std::string icon = StatusIcon(sec.status);
        std::string statusLabel = StatusText(sec.status);

        f << "<div class='card'><div class='card-header'>\n";
        f << "<h2>" << WToA(sec.name) << "</h2>\n";
        f << "<span class='badge' style='background:" << color << ";color:#fff'>"
          << icon << " " << statusLabel << "</span>\n";
        f << "</div>\n";

        if (!sec.model.empty())
            f << "<div class='model'>型号: " << WToA(sec.model) << "</div>\n";

        f << "<table>\n";
        for (auto& item : sec.items) {
            if (item.label == L"状态" || item.label == L"故障说明") continue;
            std::string label = WToA(item.label);
            // Indent sub-items
            if (label.size() > 2 && label.substr(0, 2) == "  ") {
                label = "&nbsp;&nbsp;&nbsp;&nbsp;" + label.substr(2);
            }
            f << "<tr><td>" << label << "</td><td>" << WToA(item.value) << "</td></tr>\n";
        }
        f << "</table>\n";

        if (!sec.statusText.empty())
            f << "<div class='fault'>故障说明: " << WToA(sec.statusText) << "</div>\n";

        f << "</div>\n";
    }

    f << "<div class='footer'>硬件故障检测工具 &mdash; 报告生成时间: " << WToA(result.time) << "</div>\n";
    f << "</div></body></html>\n";
    f.close();
    return true;
}

bool ExportTxt(const std::wstring& path, const DetectResult& result) {
    std::wofstream f(WToA(path));
    f.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    if (!f.is_open()) return false;

    f << L"═══════════════════════════════════════════════\n";
    f << L"              硬件检测报告\n";
    f << L"═══════════════════════════════════════════════\n";
    f << L"检测时间: " << result.time << L"\n\n";

    for (auto& sec : result.sections) {
        std::wstring statusStr;
        switch (sec.status) {
            case HwStatus::Normal:  statusStr = L"✅ 正常"; break;
            case HwStatus::Warning: statusStr = L"⚠️ 警告"; break;
            case HwStatus::Error:   statusStr = L"❌ 故障"; break;
            default:                statusStr = L"❓ 未知"; break;
        }

        f << L"──── " << sec.name << L" ────\n";
        if (!sec.model.empty())
            f << L"  型号: " << sec.model << L"\n";
        f << L"  状态: " << statusStr << L"\n";

        for (auto& item : sec.items) {
            if (item.label == L"状态" || item.label == L"故障说明") continue;
            f << L"  " << item.label << L": " << item.value << L"\n";
        }
        if (!sec.statusText.empty())
            f << L"  ⚠ 故障说明: " << sec.statusText << L"\n";
        f << L"\n";
    }

    f << L"═══════════════════════════════════════════════\n";
    if (result.errorCount > 0)
        f << L"❌ 故障: " << result.errorCount << L" 项\n";
    if (result.warnCount > 0)
        f << L"⚠️ 警告: " << result.warnCount << L" 项\n";
    if (result.errorCount == 0 && result.warnCount == 0)
        f << L"✅ 所有检测项目均正常\n";

    f.close();
    return true;
}
