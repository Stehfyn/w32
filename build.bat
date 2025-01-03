@echo off
setlocal enabledelayedexpansion
cd "%~dp0"

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   exit /b 0
)

set out=out
set sources=..\..\demo.c ..\..\w32.c
set cflags=/nologo /O2 /Oi /MT /std:c11 /Wall /WX /wd4710 /wd4191 /D _NDEBUG /D UNICODE /D _UNICODE
set libs=ntdll.lib gdi32.lib dwmapi.lib Shcore.lib Kernel32.lib libcmt.lib user32.lib comctl32.lib  Shlwapi.lib shell32.lib runtimeobject.lib ole32.lib advapi32.lib glu32.lib opengl32.lib
set lflags=/CGTHREADS:8

set "build_demo=/?"

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
	rmdir /s /q %out% 2>nul
	mkdir %out%
	pushd %out%
	call :%%build_demo%% demo x86 3>&1 >nul
	call :%%build_demo%% demo x64 3>&1 >nul
	popd
)

if "%0" == ":%build_demo%" (
	echo Build %1 %2 @call:
	call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" %2
	taskkill /f /im %1%2.exe >NUL 2>&1
	rmdir /s /q %2 2>nul
	mkdir %2
	pushd %2
	echo. && echo %2 %1 exe
	cl %cflags% /Tc %sources% %libs% /link /MACHINE:%2 /OUT:%1.exe /SUBSYSTEM:Windows
	move %1.exe ..\%1%2.exe
	popd
	rmdir /s /q %2 2>nul
)>&3

exit /b 0
