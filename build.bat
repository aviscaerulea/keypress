@echo off
REM ビルドスクリプト for keypress

if not exist out mkdir out

cl /EHsc /W4 /O2 /utf-8 /Fo"out/" /Fe"out/" keypress.cpp /link /SUBSYSTEM:WINDOWS user32.lib

if %errorlevel% equ 0 (
    echo ビルド成功: out\keypress.exe
) else (
    echo ビルド失敗
    exit /b 1
)
