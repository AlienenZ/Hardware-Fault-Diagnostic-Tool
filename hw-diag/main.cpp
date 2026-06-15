#include <windows.h>
#include "gui.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    GuiApp app;
    if (!app.Create(hInst)) {
        MessageBoxW(NULL, L"创建窗口失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    return app.Run();
}
