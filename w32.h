#ifndef W32_H
#define W32_H
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wtypesbase.h>
#include <tchar.h>

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

EXTERN_C BOOL FORCEINLINE
RequestSystemDpiAutonomy(
    VOID
    );

EXTERN_C LPCTSTR FORCEINLINE
w32_create_window_class(
    LPCTSTR lpszClassName,
    LPCTSTR lpszIconFileName,
    UINT    style
    );

EXTERN_C BOOL FORCEINLINE
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

EXTERN_C BOOL FORCEINLINE
PumpMessageQueue(
    HWND hwndPump
    );

EXTERN_C VOID FORCEINLINE
RunMessageQueue(
    HWND hwndRun
    );

EXTERN_C LRESULT
WndProc(
    HWND        hWnd,
    UINT        msg,
    WPARAM      wParam,
    LPARAM      lParam,
    WNDPROCHOOK lpfnWndProcHook,
    LPVOID      lpUserData
    );

EXTERN_C BOOL FORCEINLINE
SetAlphaComposition(
    w32_window* wnd,
    BOOL        enabled
    );

#define MAX_ENUM_MONITORS                               (16U)

typedef struct _w32_display_info {
    CHAR          deviceName[CCHDEVICENAME + 1];
    BYTE          unused;
    WCHAR         deviceNameW[CCHDEVICENAME + 1];
    DEVMODE       deviceMode;
    MONITORINFOEX monitorInfoEx;
} DISPLAYINFO, * PDISPLAYINFO;

typedef struct _w32_display_configuration {
    RECT        rcBound;
    UINT        nDisplays;
    DISPLAYINFO lpDisplays[MAX_ENUM_MONITORS];
} DISPLAYCONFIG, *PDISPLAYCONFIG;

EXTERN_C BOOL FORCEINLINE
GetDisplayConfig(
    PDISPLAYCONFIG lpDisplayConfig
    );

EXTERN_C BOOL FORCEINLINE
AdjustWindowPoint(
    LPPOINT point
    );

EXTERN_C BOOL CFORCEINLINE
GetWindowCenteredPoint(
    LPPOINT      p,
    CONST LPSIZE sz
    );

typedef struct _w32_timer {
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;
    LARGE_INTEGER elapsed;
    LARGE_INTEGER elapsedAccum;
} w32_timer;

EXTERN_C BOOL FORCEINLINE
w32_timer_init(
    w32_timer* tmr
    );

EXTERN_C BOOL FORCEINLINE
w32_timer_start(
    w32_timer* tmr
    );

EXTERN_C BOOL FORCEINLINE
w32_timer_stop(
    w32_timer* tmr
    );

EXTERN_C DOUBLE FORCEINLINE
w32_timer_elapsed(
    w32_timer* tmr
    );

EXTERN_C BOOL FORCEINLINE
w32_timer_reset(
    w32_timer* tmr
    );

#define MILLISECONDS_TO_100NANOSECONDS(durationMs)      ((durationMs) * 1000 * 10)
#define MILLISECONDS_FROM_100NANOSECONDS(durationNanoS) ((durationNanoS) / (1000 * 10))

EXTERN_C LONG FORCEINLINE
SetSystemTimerResolution(
    ULONG  hnsDesiredResolution,
    BOOL   setResolution,
    PULONG hnsCurrentResolution
    );

EXTERN_C HANDLE FORCEINLINE
CreateHighResolutionTimer(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    LPCTSTR               lpszTimerName,
    DWORD                 dwDesiredAccess
    );

EXTERN_C BOOL FORCEINLINE
YieldOnTimer(
    HANDLE               hTimer,
    CONST PLARGE_INTEGER dueTime
    );

EXTERN_C BOOL FORCEINLINE
HectonanoSleep(
    LONGLONG hns
    );

EXTERN_C LPCTSTR FORCEINLINE
DecipherMessage(
    UINT uMsg);

#endif
