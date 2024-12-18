@echo off
setlocal enabledelayedexpansion
set root="%~dp0"
pushd %root%

set exe=main.exe
set sources=..\..\main.c ..\..\w32.c
set libs=user32.lib gdi32.lib shcore.lib dwmapi.lib ntdll.lib opengl32.lib runtimeobject.lib Ole32.lib Advapi32.lib
set lflags=/CGTHREADS:8
set defines=/D UNICODE /D _UNICODE

if "%1" == "--release" or if 1 (
set target_dir=x64\Release
set cflags=/nologo /GL /MT /O2 /Zc:wchar_t /std:c11 /Wall /WX /wd4710
set lflags=%lflags% /SUBSYSTEM:WINDOWS
)
if "%1" == "--debug" (
set target_dir=x64\Debug
set cflags=/nologo /MTd /Od /Zo /Zi /Zc:wchar_t /FC /std:c11 /Wall /WX /wd4710
set defines=%defines% /D _CONSOLE
set lflags=%lflags% /SUBSYSTEM:CONSOLE
)
if not defined DevEnvDir (
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
echo vcvars64.bat not found
(goto fail)
)
)

taskkill /f /im %exe% >NUL 2>&1
taskkill /f /im %exe% >NUL 2>&1
ping -n 1 -w 1000 127.0.0.1 > nul
rmdir /s /q %target_dir%
if errorlevel 1 (goto fail)
mkdir %target_dir%
if errorlevel 1 (goto fail)
pushd %target_dir%

cl %cflags% %defines% /Tc %sources% %libs% /link %lflags% /out:%exe%

if errorlevel 1 (
:fail
echo Failure generating code
)

popd
popd
endlocal

exit /b errorlevel
