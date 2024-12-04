@echo off
setlocal enabledelayedexpansion

set target_dir=Debug-x64
set flags=/nologo /MTd /Od /Zo /Zi /EHsc /Zc:wchar_t /FC /std:c11 /Wall /WX /fsanitize=address /D UNICODE /D _UNICODE
set sources=..\main.c ..\w32.c
set libs=user32.lib gdi32.lib shcore.lib dwmapi.lib ntdll.lib

set root="%~dp0"
pushd %root%

if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    
    if errorlevel 1 (
       echo vcvars64.bat not found
       (goto fail)
    )
)

rmdir /s /q %target_dir%
mkdir %target_dir%
pushd %target_dir%

cl %flags% /Tc %sources% %libs% /link /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup

if errorlevel 1 (goto fail)

:success
popd
popd
exit /b 0

:fail
popd
popd
exit /b 1