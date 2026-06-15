#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include "detect.h"

#pragma comment(lib, "comctl32.lib")

#define WM_DETECT_DONE (WM_USER + 100)

class GuiApp {
public:
    bool Create(HINSTANCE hInst);
    int Run();

private:
    HINSTANCE hInst = NULL;
    HWND hWnd = NULL;
    HWND hTree = NULL;
    HWND hList = NULL;
    HWND hStatusBar = NULL;
    HWND hBtnDetect = NULL;
    HWND hBtnRefresh = NULL;
    HWND hBtnExport = NULL;
    HWND hDetailEdit = NULL;

    DetectResult result;
    bool detected = false;

    ATOM RegisterClass();
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

    void CreateControls();
    void OnSize();
    void OnDetect();
    void OnExport();
    void PopulateTree();
    void ShowSection(int idx);
    void ShowOverview();
    void UpdateStatusBar();

    static DWORD WINAPI DetectThread(LPVOID lpParam);
};
