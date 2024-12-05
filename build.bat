@echo off
setlocal enabledelayedexpansion
set root="%~dp0"
pushd %root%

set sources=..\..\main.c ..\..\w32.c
set libs=user32.lib gdi32.lib shcore.lib dwmapi.lib ntdll.lib

if "%1" == "--release" or if 1 (
set target_dir=x64\Release
set flags=/nologo /GL /MT /O2 /Zc:wchar_t /std:c11 /Wall /WX 
set defines=/D UNICODE /D _UNICODE
)
if "%1" == "--debug" (
set target_dir=x64\Debug
set flags=/nologo /MTd /Od /Zo /Zi /Zc:wchar_t /FC /std:c11 /Wall /WX
set defines=/D UNICODE /D _UNICODE /D _CONSOLE
)

if not defined DevEnvDir (
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
echo vcvars64.bat not found
(goto fail)
)
rmdir /s /q %target_dir%
if errorlevel 1 (goto fail)
mkdir %target_dir%
if errorlevel 1 (goto fail)
pushd %target_dir%
)

cl %flags% %defines% /Tc %sources% %libs% /link /SUBSYSTEM:WINDOWS /CGTHREADS:8
if errorlevel 1 (
:fail
echo Failure generating code
)

popd
popd
exit /b errorlevel