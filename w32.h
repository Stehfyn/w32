#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#define MAX_ENUM_MONITORS                               (16U)
#define MILLISECONDS_TO_100NANOSECONDS(durationMs)      ((durationMs) * 1000 * 10)
#define MILLISECONDS_FROM_100NANOSECONDS(durationNanoS) ((durationNanoS) / (1000 * 10))

typedef struct _w32_user_state {
  ULONG_PTR   piActiveProcessID;
  wchar_t     imageName[300];
  POINT       cursor;
  MONITORINFO monitorInfo;
} w32_user_state;

typedef struct _w32_timer {
  LARGE_INTEGER freq;
  LARGE_INTEGER start;
  LARGE_INTEGER stop;
  LARGE_INTEGER elapsed;
} w32_timer;

typedef BOOL (*WNDPROCHOOK)(
  HWND     hWnd,
  UINT     msg,
  WPARAM   wParam,
  LPARAM   lParam,
  LRESULT* lpResult,
  LPVOID   lpUserData
);

typedef LRESULT (*WNDPROCOVERRIDE)(
  HWND        hWnd,
  UINT        msg,
  WPARAM      wParam,
  LPARAM      lParam,
  WNDPROCHOOK lpfnWndProcHook,
  LPVOID      lpUserData
);

typedef struct _w32_window {
  HWND            hWnd;
  WNDPROCOVERRIDE lpfnWndProcOverride;
  WNDPROCHOOK     lpfnWndProcHook;
  LPVOID          lpUserData;
} w32_window;

typedef struct _w32_monitor_info {
  CHAR          deviceName[CCHDEVICENAME + 1];
  BYTE          unused;
  WCHAR         deviceNameW[CCHDEVICENAME + 1];
  DEVMODE       deviceMode;
  MONITORINFOEX monitorInfoEx;
} w32_monitor_info;

typedef struct _w32_display_info {
  RECT             boundingRect;
  UINT             numMonitors;
  w32_monitor_info monitors[MAX_ENUM_MONITORS];
} w32_display_info;

EXTERN_C
FORCEINLINE
LPCTSTR
w32_create_window_class(
  LPCTSTR lpszClassName,
  UINT    style
);

EXTERN_C
FORCEINLINE
BOOL
w32_create_window(
  w32_window*     wnd,
  LPCTSTR         lpszTitle,
  LPCTSTR         lpszClassName,
  INT             x,
  INT             y,
  INT             nWidth,
  INT             nHeight,
  DWORD           exStyle,
  DWORD           style,
  WNDPROCOVERRIDE lpfnWndProcOverride,
  WNDPROCHOOK     lpfnWndProcHook,
  LPVOID          lpUserData
);

EXTERN_C
FORCEINLINE
BOOL
w32_pump_message_loop(
  w32_window* wnd,
  HWND        hPump
);

EXTERN_C
FORCEINLINE
VOID
w32_run_message_loop(
  w32_window* wnd,
  HWND        hPump
);

EXTERN_C
FORCEINLINE
BOOL
w32_get_display_info(
  w32_display_info* displayInfo
);

EXTERN_C
FORCEINLINE
BOOL
  w32_set_process_dpiaware(
  VOID
  );

EXTERN_C
FORCEINLINE
BOOL
w32_set_alpha_composition(
  w32_window* wnd,
  BOOL        enabled
);

EXTERN_C
FORCEINLINE
LONG
w32_set_timer_resolution(
  ULONG  hnsDesiredResolution,
  BOOL   setResolution,
  PULONG hnsCurrentResolution
);

EXTERN_C
FORCEINLINE
HANDLE
w32_create_high_resolution_timer(
  LPSECURITY_ATTRIBUTES lpTimerAttributes,
  LPCTSTR               lpszTimerName,
  DWORD                 dwDesiredAccess
);

EXTERN_C
FORCEINLINE
BOOL
w32_yield_on_high_resolution_timer(
  HANDLE               hTimer,
  CONST PLARGE_INTEGER dueTime
);

EXTERN_C
FORCEINLINE
BOOL
w32_hectonano_sleep(
  LONGLONG hns
);

EXTERN_C
FORCEINLINE
BOOL
w32_adjust_window_start_point(
  LPPOINT point
);

EXTERN_C
FORCEINLINE
BOOL
w32_timer_init(
  w32_timer* tmr
);

EXTERN_C
FORCEINLINE
BOOL
w32_timer_start(
  w32_timer* tmr
);

EXTERN_C
FORCEINLINE
BOOL
w32_timer_stop(
  w32_timer* tmr
);

EXTERN_C
LRESULT
w32_borderless_wndproc(
  HWND        hWnd,
  UINT        msg,
  WPARAM      wParam,
  LPARAM      lParam,
  WNDPROCHOOK lpfnWndProcHook,
  LPVOID      lpUserData
);

EXTERN_C
FORCEINLINE
BOOL
w32_get_user_state(
  w32_user_state* us
);

EXTERN_C
CFORCEINLINE
BOOL
w32_get_centered_window_point(
  LPPOINT      p,
  CONST LPSIZE sz
);
