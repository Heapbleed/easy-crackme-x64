@echo off
setlocal
cd /d "%~dp0"

where cl >nul 2>nul || call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul

if not exist build mkdir build

cl /nologo /O2 /EHsc /MT /DUNICODE /D_UNICODE /Iimgui /Iimgui\backends ^
   crackme.cpp ^
   imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp ^
   imgui\backends\imgui_impl_win32.cpp imgui\backends\imgui_impl_dx11.cpp ^
   /Fobuild\ /Fe:CrackMe.exe ^
   /link /SUBSYSTEM:WINDOWS d3d11.lib dxgi.lib user32.lib gdi32.lib shell32.lib dwmapi.lib
if errorlevel 1 (echo CRACKME BUILD FAILED & exit /b 1)

cl /nologo /O2 /EHsc keygen.cpp /Fobuild\keygen.obj /Fe:keygen.exe
if errorlevel 1 (echo KEYGEN BUILD FAILED & exit /b 1)

echo Built CrackMe.exe and keygen.exe
