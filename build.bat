@echo off
setlocal enabledelayedexpansion

set target_dir=x64
set flags=/nologo /Od /Zo /Zi /FC /std:c11 /Wall /WX /D UNICODE /D _UNICODE
set sources=..\main.c ..\w32.c
set libs=user32.lib gdi32.lib shcore.lib dwmapi.lib ntdll.lib

set root="%~dp0"
pushd %root%

if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    if errorlevel 1 (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
    if errorlevel 1 (
       echo vcvars64.bat not found
       (goto fail)
    )
)

rmdir /s /q %target_dir%
mkdir %target_dir%
pushd %target_dir%

cl %flags% /Tc %sources% %libs%

if errorlevel 1 (goto fail)

:success
popd
popd
exit /b 0

:fail
popd
popd
exit /b 1