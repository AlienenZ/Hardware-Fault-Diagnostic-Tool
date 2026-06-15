#include "gui.h"
#include "report.h"
#include <commctrl.h>
#include <richedit.h>
#include <string>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

static HMODULE hRichEdit = NULL;
static GuiApp* g_app = nullptr;

static const int TREE_WIDTH = 200;
static const int TOOLBAR_H = 45;
static const int BTN_W = 110;
static const int BTN_H = 32;
static const int PADDING = 8;

bool GuiApp::Create(HINSTANCE h) {
    hInst = h;
    hRichEdit = LoadLibrary(L"Msftedit.dll");
    if (!hRichEdit) hRichEdit = LoadLibrary(L"Riched20.dll");

    INITCOMMONCONTROLSEX ic;
    ic.dwSize = sizeof(ic);
    ic.dwICC = ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&ic);

    RegisterClass();

    hWnd = CreateWindowExW(
        0, L"HwDiagWnd", L"硬件故障检测工具",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 640,
        NULL, NULL, hInst, this);

    if (!hWnd) return false;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return true;
}

ATOM GuiApp::RegisterClass() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"HwDiagWnd";
    wc.hIcon = LoadIcon(NULL, IDI_SHIELD);
    return RegisterClassExW(&wc);
}

int GuiApp::Run() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_TAB) {
            SetFocus(GetNextDlgTabItem(hWnd, GetFocus(), FALSE));
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void GuiApp::CreateControls() {
    // Buttons
    hBtnDetect = CreateWindowW(L"BUTTON", L"\xD83D\xDD0D 开始检测",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        PADDING, PADDING, BTN_W, BTN_H,
        hWnd, (HMENU)1001, hInst, NULL);

    hBtnRefresh = CreateWindowW(L"BUTTON", L"\xD83D\xDD04 刷新",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        PADDING + BTN_W + 8, PADDING, BTN_W - 30, BTN_H,
        hWnd, (HMENU)1002, hInst, NULL);

    hBtnExport = CreateWindowW(L"BUTTON", L"\xD83D\xDCC4 导出报告",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        PADDING + BTN_W * 2, PADDING, BTN_W, BTN_H,
        hWnd, (HMENU)1003, hInst, NULL);

    HFONT hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Microsoft YaHei UI");
    SendMessage(hBtnDetect, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnRefresh, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnExport, WM_SETFONT, (WPARAM)hFont, TRUE);

    // TreeView
    hTree = CreateWindowExW(0, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, 0, 0,
        hWnd, (HMENU)1010, hInst, NULL);
    TreeView_SetBkColor(hTree, RGB(248, 249, 250));

    // Detail panel
    if (hRichEdit) {
        hDetailEdit = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
            ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hWnd, (HMENU)1020, hInst, NULL);
        SendMessage(hDetailEdit, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
    } else {
        hDetailEdit = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
            ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hWnd, (HMENU)1020, hInst, NULL);
    }

    HFONT hMonoFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    SendMessage(hDetailEdit, WM_SETFONT, (WPARAM)hMonoFont, TRUE);

    // Status bar
    hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hWnd, (HMENU)1030, hInst, NULL);

    // Welcome text
    SetWindowTextW(hDetailEdit,
        L"  欢迎使用硬件故障检测工具\r\n\r\n"
        L"  点击 [开始检测] 按钮进行硬件检测\r\n\r\n"
        L"  检测项目:\r\n"
        L"    - CPU 处理器 (型号/温度/负载)\r\n"
        L"    - 内存 RAM (容量/使用率/错误)\r\n"
        L"    - 硬盘存储 (SMART/容量/分区)\r\n"
        L"    - 显卡 GPU (型号/显存/驱动)\r\n"
        L"    - 主板 BIOS (型号/版本)\r\n"
        L"    - 电池 (电量/充放电状态)\r\n"
        L"    - 网络适配器 (连接/状态)\r\n"
    );
}

void GuiApp::OnSize() {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right;
    int h = rc.bottom;

    int treeX = PADDING;
    int treeY = TOOLBAR_H;
    int treeW = TREE_WIDTH;
    int treeH = h - TOOLBAR_H - 25;

    int detailX = treeX + treeW + PADDING;
    int detailY = TOOLBAR_H;
    int detailW = w - detailX - PADDING;
    int detailH = treeH;

    MoveWindow(hTree, treeX, treeY, treeW, treeH, TRUE);
    MoveWindow(hDetailEdit, detailX, detailY, detailW, detailH, TRUE);
    MoveWindow(hStatusBar, 0, h - 22, w, 22, TRUE);
}

void GuiApp::PopulateTree() {
    TreeView_DeleteAllItems(hTree);

    TVINSERTSTRUCTW tvi{};
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;

    tvi.item.pszText = (LPWSTR)L"检测总览";
    tvi.item.lParam = -1;
    TreeView_InsertItem(hTree, &tvi);

    for (int i = 0; i < (int)result.sections.size(); i++) {
        auto& sec = result.sections[i];
        std::wstring label;
        switch (sec.status) {
            case HwStatus::Normal:  label = L"[OK] "; break;
            case HwStatus::Warning: label = L"[!] "; break;
            case HwStatus::Error:   label = L"[X] "; break;
            default:                label = L"[?] "; break;
        }
        label += sec.name;

        tvi.item.pszText = (LPWSTR)label.c_str();
        tvi.item.lParam = i;
        TreeView_InsertItem(hTree, &tvi);
    }

    HTREEITEM hFirst = TreeView_GetRoot(hTree);
    if (hFirst) {
        TreeView_SelectItem(hTree, hFirst);
    }
}

static void AppendRichText(HWND hEdit, const std::wstring& text, COLORREF color, bool bold = false, int fontSize = 0) {
    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, len, len);

    CHARFORMAT2W cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE;
    cf.crTextColor = color;
    cf.dwEffects = bold ? CFE_BOLD : 0;
    cf.yHeight = fontSize > 0 ? fontSize * 20 : 240;
    SendMessageW(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
}

void GuiApp::ShowOverview() {
    SetWindowTextW(hDetailEdit, L"");

    if (!detected) {
        AppendRichText(hDetailEdit, L"  尚未进行检测\r\n\r\n", RGB(128, 128, 128), false, 14);
        AppendRichText(hDetailEdit, L"  请点击 [开始检测] 按钮", RGB(128, 128, 128), false, 12);
        return;
    }

    AppendRichText(hDetailEdit, L"  === 检测总览 ===\r\n\r\n", RGB(0, 120, 212), true, 18);
    AppendRichText(hDetailEdit, L"  检测时间: ", RGB(80, 80, 80), true, 12);
    AppendRichText(hDetailEdit, result.time + L"\r\n\r\n", RGB(40, 40, 40), false, 12);

    for (auto& sec : result.sections) {
        COLORREF sc;
        std::wstring statusStr;
        switch (sec.status) {
            case HwStatus::Normal:  sc = RGB(34, 139, 34);  statusStr = L"[OK] 正常"; break;
            case HwStatus::Warning: sc = RGB(200, 150, 0);  statusStr = L"[!] 警告"; break;
            case HwStatus::Error:   sc = RGB(200, 30, 30);  statusStr = L"[X] 故障"; break;
            default:                sc = RGB(128, 128, 128); statusStr = L"[?] 未知"; break;
        }

        std::wstring line = sec.name;
        while (line.size() < 16) line += L" ";
        AppendRichText(hDetailEdit, L"  " + line, RGB(40, 40, 40), true, 13);

        std::wstring model = sec.model;
        if (model.size() > 24) model = model.substr(0, 24) + L"...";
        while (model.size() < 24) model += L" ";
        AppendRichText(hDetailEdit, model, RGB(80, 80, 80), false, 12);
        AppendRichText(hDetailEdit, L"  " + statusStr + L"\r\n", sc, true, 13);
    }

    AppendRichText(hDetailEdit, L"\r\n", RGB(40, 40, 40));

    if (result.errorCount > 0) {
        AppendRichText(hDetailEdit, L"  [X] 发现 " + std::to_wstring(result.errorCount) + L" 项故障\r\n",
            RGB(200, 30, 30), true, 14);
    }
    if (result.warnCount > 0) {
        AppendRichText(hDetailEdit, L"  [!] 发现 " + std::to_wstring(result.warnCount) + L" 项警告\r\n",
            RGB(200, 150, 0), true, 14);
    }
    if (result.errorCount == 0 && result.warnCount == 0) {
        AppendRichText(hDetailEdit, L"  [OK] 所有检测项目均正常，未发现硬件故障\r\n",
            RGB(34, 139, 34), true, 14);
    }

    AppendRichText(hDetailEdit, L"\r\n  点击左侧各硬件项查看详细信息\r\n", RGB(150, 150, 150), false, 11);
}

void GuiApp::ShowSection(int idx) {
    if (idx < 0 || idx >= (int)result.sections.size()) {
        ShowOverview();
        return;
    }

    SetWindowTextW(hDetailEdit, L"");

    auto& sec = result.sections[idx];

    COLORREF sc;
    std::wstring statusStr;
    switch (sec.status) {
        case HwStatus::Normal:  sc = RGB(34, 139, 34);  statusStr = L"[OK] 正常"; break;
        case HwStatus::Warning: sc = RGB(200, 150, 0);  statusStr = L"[!] 警告"; break;
        case HwStatus::Error:   sc = RGB(200, 30, 30);  statusStr = L"[X] 故障"; break;
        default:                sc = RGB(128, 128, 128); statusStr = L"[?] 未知"; break;
    }

    // Header
    AppendRichText(hDetailEdit, L"  === " + sec.name + L" ===\r\n\r\n", RGB(0, 80, 160), true, 18);

    if (!sec.model.empty()) {
        AppendRichText(hDetailEdit, L"  设备型号:  ", RGB(80, 80, 80), true, 13);
        AppendRichText(hDetailEdit, sec.model + L"\r\n", RGB(40, 40, 40), false, 13);
    }

    AppendRichText(hDetailEdit, L"  当前状态:  ", RGB(80, 80, 80), true, 13);
    AppendRichText(hDetailEdit, statusStr + L"\r\n\r\n", sc, true, 14);
    AppendRichText(hDetailEdit, L"  ----------------------------------------\r\n\r\n", RGB(210, 210, 210));

    for (auto& item : sec.items) {
        if (item.label == L"状态" || item.label == L"故障说明") continue;

        bool isSub = (item.label.size() > 2 && item.label[0] == L' ' && item.label[1] == L' ');

        if (isSub) {
            std::wstring pad(item.label.size() + 4, L' ');
            AppendRichText(hDetailEdit, L"    " + item.label, RGB(120, 120, 120), false, 11);
            AppendRichText(hDetailEdit, L"  ", RGB(40, 40, 40));
            AppendRichText(hDetailEdit, item.value + L"\r\n", RGB(60, 60, 60), false, 11);
        } else {
            AppendRichText(hDetailEdit, L"  " + item.label, RGB(80, 80, 80), true, 12);
            int padLen = 14 - (int)item.label.size();
            if (padLen < 2) padLen = 2;
            AppendRichText(hDetailEdit, std::wstring(padLen, L' '), RGB(255, 255, 255));
            AppendRichText(hDetailEdit, item.value + L"\r\n", RGB(40, 40, 40), false, 12);
        }
    }

    if (!sec.statusText.empty()) {
        AppendRichText(hDetailEdit, L"\r\n  ----------------------------------------\r\n\r\n", RGB(210, 210, 210));
        AppendRichText(hDetailEdit, L"  [!] 故障说明: ", RGB(200, 30, 30), true, 13);
        AppendRichText(hDetailEdit, sec.statusText + L"\r\n", RGB(200, 30, 30), false, 13);
    }
}

void GuiApp::UpdateStatusBar() {
    std::wstring text;
    if (detected) {
        text = L"  检测时间: " + result.time;
        text += L"    |    检测项目: " + std::to_wstring(result.sections.size()) + L" 项";
        if (result.errorCount > 0)
            text += L"    |    [X] 故障: " + std::to_wstring(result.errorCount) + L" 项";
        if (result.warnCount > 0)
            text += L"    |    [!] 警告: " + std::to_wstring(result.warnCount) + L" 项";
        if (result.errorCount == 0 && result.warnCount == 0)
            text += L"    |    [OK] 全部正常";
    } else {
        text = L"  就绪 - 请点击 [开始检测] 进行硬件检测";
    }
    SendMessageW(hStatusBar, SB_SETTEXT, 0, (LPARAM)text.c_str());
}

DWORD WINAPI GuiApp::DetectThread(LPVOID lpParam) {
    GuiApp* app = (GuiApp*)lpParam;
    HardwareDetector det;
    if (det.Init()) {
        app->result = det.RunAll();
        app->detected = true;
    }
    PostMessage(app->hWnd, WM_DETECT_DONE, 0, 0);
    return 0;
}

void GuiApp::OnDetect() {
    EnableWindow(hBtnDetect, FALSE);
    SetWindowTextW(hBtnDetect, L"检测中...");
    SetWindowTextW(hDetailEdit,
        L"\r\n\r\n\r\n\r\n"
        L"         +--------------------------+\r\n"
        L"         |                          |\r\n"
        L"         |     正在检测硬件...      |\r\n"
        L"         |                          |\r\n"
        L"         |     请稍候               |\r\n"
        L"         |                          |\r\n"
        L"         +--------------------------+\r\n");

    DWORD tid;
    CreateThread(NULL, 0, DetectThread, this, 0, &tid);
}

void GuiApp::OnExport() {
    if (!detected) {
        MessageBoxW(hWnd, L"请先进行硬件检测", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t file[MAX_PATH] = L"硬件检测报告.html";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"HTML 报告 (*.html)\0*.html\0文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"导出检测报告";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileNameW(&ofn)) {
        std::wstring path(file);
        bool isHtml = (path.find(L".html") != std::wstring::npos || path.find(L".htm") != std::wstring::npos);
        bool ok;
        if (isHtml)
            ok = ExportHtml(path, result);
        else
            ok = ExportTxt(path, result);

        if (ok) {
            if (MessageBoxW(hWnd, (L"报告已导出至:\n" + path + L"\n\n是否打开文件?").c_str(),
                    L"导出成功", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                ShellExecuteW(hWnd, L"open", path.c_str(), NULL, NULL, SW_SHOW);
            }
        } else {
            MessageBoxW(hWnd, L"导出失败，请检查文件路径和权限", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

LRESULT CALLBACK GuiApp::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)l;
        g_app = (GuiApp*)cs->lpCreateParams;
        g_app->hWnd = h;
        g_app->CreateControls();
        g_app->UpdateStatusBar();
        return 0;
    }
    if (g_app) return g_app->HandleMessage(m, w, l);
    return DefWindowProcW(h, m, w, l);
}

LRESULT GuiApp::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        OnSize();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
            case 1001: OnDetect(); break;
            case 1002: if (detected) OnDetect(); break;
            case 1003: OnExport(); break;
        }
        return 0;

    case WM_DETECT_DONE:
        EnableWindow(hBtnDetect, TRUE);
        SetWindowTextW(hBtnDetect, L"开始检测");
        PopulateTree();
        ShowOverview();
        UpdateStatusBar();
        return 0;

    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lp;
        if (nm->hwndFrom == hTree && nm->code == TVN_SELCHANGED) {
            NMTREEVIEW* ntv = (NMTREEVIEW*)lp;
            int idx = (int)ntv->itemNew.lParam;
            if (idx == -1)
                ShowOverview();
            else
                ShowSection(idx);
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, RGB(248, 249, 250));
        static HBRUSH hBr = CreateSolidBrush(RGB(248, 249, 250));
        return (LRESULT)hBr;
    }

    case WM_DESTROY:
        if (hRichEdit) FreeLibrary(hRichEdit);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}
