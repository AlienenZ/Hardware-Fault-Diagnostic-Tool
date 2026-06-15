@echo off
chcp 65001 >nul 2>&1
title 硬件故障检测工具 - 编译
echo.
echo  ========================================
echo    硬件故障检测工具 - 编译脚本
echo  ========================================
echo.

REM 添加 MSYS2 UCRT64 到 PATH
set "PATH=C:\msys64\ucrt64\bin;C:\msys64\mingw64\bin;%PATH%"

REM 检查 g++
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo  [错误] 未找到 g++ 编译器
    echo.
    echo  请安装 MSYS2:
    echo    1. 访问 https://www.msys2.org/ 下载安装
    echo    2. 打开 MSYS2 UCRT64 终端
    echo    3. 运行: pacman -S mingw-w64-ucrt-x86_64-gcc
    echo.
    pause
    exit /b 1
)

echo  编译器: g++ (静态链接，无需额外 DLL)
echo  正在编译...
echo.

g++ -std=c++17 -O2 -static -DUNICODE -D_UNICODE -mwindows -municode ^
    main.cpp gui.cpp wmi.cpp detect.cpp report.cpp ^
    -lole32 -loleaut32 -lwbemuuid -lcomctl32 -lshell32 -lcomdlg32 -lpdh ^
    -o hw_diag.exe

if %errorlevel% equ 0 (
    echo  ========================================
    echo    编译成功!
    echo    输出文件: hw_diag.exe
    echo    大小: 已静态链接，无需额外 DLL
    echo  ========================================
    echo.
    echo  双击 hw_diag.exe 即可运行
    echo.
    set /p run="  是否立即运行? (Y/N): "
    if /i "!run!"=="Y" start "" hw_diag.exe
) else (
    echo.
    echo  [错误] 编译失败，请检查上方错误信息
    echo.
)

pause
