#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#define MAX_ENUM_DISPLAYS                               (16U)
#define MILLISECONDS_TO_100NANOSECONDS(durationMs)      ((durationMs) * 1000 * 10)
#define MILLISECONDS_FROM_100NANOSECONDS(durationNanoS) ((durationNanoS) / (1000 * 10))

typedef BOOL(*WNDPROCHOOK)(HWND, UINT, WPARAM, LPARAM, LRESULT *, LPVOID);

typedef struct _w32_window {
  HWND        hWnd;
  WNDPROCHOOK lpfnWndProcHook;
  LPVOID      lpUserData;
} w32_window;

typedef struct _w32_monitor_info
{
  CHAR          deviceName[CCHDEVICENAME + 1];
  BYTE          unused;
  WCHAR         deviceNameW[CCHDEVICENAME + 1];
  DEVMODE       deviceMode;
  MONITORINFOEX monitorInfoEx;
} w32_monitor_info;

typedef struct _w32_display_info
{
  RECT                 boundingRect;
  UINT                 numDisplays;
  w32_monitor_info     displays[MAX_ENUM_DISPLAYS];
} w32_display_info;

LPCTSTR
w32_create_window_class(
  LPCTSTR    lpszClassName,
  UINT       style
);

BOOL 
w32_create_window(
  w32_window* wnd,
  LPCTSTR     lpszTitle,
  LPCTSTR     lpszClassName,
  INT         x,
  INT         y,
  INT         nWidth,
  INT         nHeight,
  DWORD       exStyle,
  DWORD       style,
  WNDPROCHOOK lpfnWndProcHook,
  LPVOID      lpUserData
);

BOOL
w32_pump_message_loop(
  w32_window* wnd, 
  BOOL        pumpThread
);

VOID
w32_run_message_loop(
  w32_window* wnd,
  BOOL        pumpThread
);

BOOL
w32_get_display_info(
  w32_display_info* displayInfo
);

BOOL
w32_set_process_dpiaware(
  VOID
);

BOOL
w32_set_alpha_composition(
  w32_window* wnd,
  BOOL        enabled
);

LONG 
w32_set_timer_resolution(
  ULONG  hnsDesiredResolution, 
  BOOL   setResolution,
  PULONG hnsCurrentResolution
);

HANDLE
w32_create_high_resolution_timer(
  LPSECURITY_ATTRIBUTES lpTimerAttributes,
  LPCTSTR               lpszTimerName,
  DWORD                 dwDesiredAccess
);

BOOL 
w32_yield_on_high_resolution_timer(
  HANDLE               hTimer, 
  CONST PLARGE_INTEGER dueTime
);

BOOL 
w32_hectonano_sleep(
  LONGLONG hns
);

