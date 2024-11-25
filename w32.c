#include "w32.h"
#include <intrin.h>
#include <windowsx.h>

#pragma warning(disable : 4255 4820)
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <winternl.h>
#pragma warning(default : 4255 4820)

#define ASSERT_W32(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define STATUS_SUCCESS                  (0)
#define STATUS_TIMER_RESOLUTION_NOT_SET (0xC0000245)

NTSYSAPI NTSTATUS NTAPI 
NtSetTimerResolution(
  ULONG   DesiredResolution,
  BOOLEAN SetResolution,
  PULONG  CurrentResolution
);

static LRESULT
wndproc(
  HWND   hWnd,
  UINT   msg,
  WPARAM wParam,
  LPARAM lParam
);

static BOOL
monitorenumproc(
  HMONITOR hMonitor,
  HDC      hDC,
  LPRECT   lpRect,
  LPARAM   lParam
);

static LRESULT
wndproc(
  HWND   hWnd,
  UINT   msg,
  WPARAM wParam,
  LPARAM lParam)
{
  if (msg == WM_NCCREATE)
  {
    LONG_PTR offset;
    LPVOID   user_data = ((LPCREATESTRUCT) lParam)->lpCreateParams;

    SetLastError(0);
    offset = SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) user_data);
    ASSERT_W32((0 == offset) && (0 == GetLastError()));
  }
  else
  {
    w32_window* wnd = (w32_window*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (wnd)
    {
      if (wnd->lpfnWndProcHook)
      {
        LRESULT result = 0;
        BOOL    handled =
            wnd->lpfnWndProcHook(hWnd, msg, wParam, lParam, &result, wnd->lpUserData);
        if (handled)
        {
          return result;
        }
      }
    }

    switch (msg)
    {
    case WM_DESTROY:
    {
      PostQuitMessage(0);
      return 0;
    }
    default:
      break;
    }
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

static BOOL
monitorenumproc(
  HMONITOR hMonitor,
  HDC      hDC,
  LPRECT   lpRect,
  LPARAM   lParam)
{
  UNREFERENCED_PARAMETER(hDC);

  w32_display_info* display_info    = NULL;
  w32_monitor_info* monitor_info    = NULL;
  LPMONITORINFOEX   lpMonitorInfoEx = NULL;
  LPDEVMODE         lpDevMode       = NULL;

  display_info = (w32_display_info*) lParam;
  monitor_info = (w32_monitor_info*) &display_info->monitors[display_info->numMonitors];
  lpMonitorInfoEx         = (LPMONITORINFOEX) &monitor_info->monitorInfoEx;
  lpDevMode               = (LPDEVMODE) &monitor_info->deviceMode;
  lpMonitorInfoEx->cbSize = (DWORD) sizeof(MONITORINFOEX);
  lpDevMode->dmSize       = (WORD) sizeof(DEVMODE);
  
  (VOID)GetMonitorInfo(
    hMonitor, 
    (LPMONITORINFO)lpMonitorInfoEx
  );
  (VOID)UnionRect(
    &display_info->boundingRect,
    &display_info->boundingRect,
    lpRect
  );
#ifdef UNICODE
  (VOID)wcsncpy_s(
    monitor_info->deviceNameW, 
    CCHDEVICENAME + 1,
    lpMonitorInfoEx->szDevice,
    CCHDEVICENAME
  );
  (VOID)WideCharToMultiByte(
    CP_UTF8,
    0,
    (LPCWCH)lpMonitorInfoEx->szDevice,
    CCHDEVICENAME,
    (LPSTR)monitor_info->deviceName,
    CCHDEVICENAME + 1,
    0,
    NULL
  );
  (VOID)EnumDisplaySettings(
    monitor_info->deviceNameW, 
    ENUM_CURRENT_SETTINGS, 
    &monitor_info->deviceMode
  );
#else
  (VOID)strncpy_s(
    monitor_info->deviceName,
    CCHDEVICENAME + 1, 
    lpMonitorInfoEx->szDevice,
    CCHDEVICENAME
  );
  (VOID)MultiByteToWideChar(
    CP_UTF8,
    0,
    (LPCCH)lpMonitorInfoEx->szDevice,
    CCHDEVICENAME,
    (LPWSTR)monitor_info->deviceNameW,
    CCHDEVICENAME + 1
  );
  (VOID)EnumDisplaySettings(
    monitor_info->deviceName,
    ENUM_CURRENT_SETTINGS,
    &monitor_info->deviceMode
  );
#endif
    return (display_info->numMonitors++ < MAX_ENUM_MONITORS);
}

FORCEINLINE 
LPCTSTR
w32_create_window_class(
  LPCTSTR    lpszClassName,
  UINT       style)
{
  WNDCLASSEX wcex = {0};
  {
    ATOM _             = 0;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.lpszClassName = lpszClassName;
    wcex.style         = style;
    wcex.hInstance     = GetModuleHandle(NULL);
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wcex.lpfnWndProc   = (WNDPROC) wndproc;
    _ = RegisterClassEx(&wcex);
    ASSERT_W32(_);
  }
  return lpszClassName;
}

FORCEINLINE 
BOOL 
w32_create_window(
  w32_window* wnd,
  LPCTSTR     lpszTitle,
  LPCTSTR     lpszClassName,
  INT         x,
  INT         y,
  INT         nWidth,
  INT         nHeight,
  DWORD       dwExStyle,
  DWORD       dwStyle,
  WNDPROCHOOK lpfnWndProcHook,
  LPVOID      lpUserData)
{
  ASSERT_W32(wnd);
  wnd->lpfnWndProcHook = lpfnWndProcHook;
  wnd->lpUserData      = lpUserData;
  wnd->hWnd            = CreateWindowEx(
      dwExStyle,
      lpszClassName,
      lpszTitle,
      dwStyle,
      x,
      y,
      nWidth,
      nHeight,
      NULL,
      NULL,
      NULL,
      (LPVOID)wnd
  );
  ASSERT_W32(wnd->hWnd);
  ASSERT_W32(FALSE == ShowWindow(wnd->hWnd, SW_SHOWDEFAULT));
  ASSERT_W32(TRUE == UpdateWindow(wnd->hWnd));
  return TRUE;
}

FORCEINLINE 
BOOL
w32_pump_message_loop(
  w32_window *wnd, 
  BOOL       pumpThread)
{
  MSG  msg  = {0};
  BOOL quit = FALSE;
  while (PeekMessage(&msg, pumpThread ? NULL : wnd->hWnd, 0U, 0U, PM_REMOVE))
  {
    (VOID) TranslateMessage(&msg);
    (VOID) DispatchMessage(&msg);
    quit |= (msg.message == WM_QUIT);
  }
  return !quit;
}

FORCEINLINE 
VOID
w32_run_message_loop(
  w32_window* wnd, 
  BOOL        pumpThread)
{
  for (;;)
  {
    MSG  msg      = {0};
    BOOL received = GetMessage(&msg, pumpThread ? NULL : wnd->hWnd, 0U, 0U);
    if (received)
    {
      (VOID) TranslateMessage(&msg);
      (VOID) DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
      {
        break;
      }
    }
    else
    {
      break;
    }
  }
}

FORCEINLINE 
BOOL 
w32_get_display_info(
  w32_display_info* displayInfo)
{
  return EnumDisplayMonitors(
    NULL,
    NULL,
    (MONITORENUMPROC)monitorenumproc,
    (LPARAM)displayInfo
  );
}

FORCEINLINE 
BOOL 
w32_set_process_dpiaware(
  VOID)
{
  HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
  return hr == S_OK;
}

FORCEINLINE 
BOOL
w32_set_alpha_composition(
  w32_window* wnd,
  BOOL        enabled)
{
  ASSERT_W32(wnd);
  DWM_BLURBEHIND bb = {0};

  if (enabled)
  {
    HRGN region = CreateRectRgn(0, 0, -1, -1);
    bb.dwFlags  = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur = region;
    bb.fEnable  = TRUE;
  }
  else
  {
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = FALSE;
  }
  return S_OK == DwmEnableBlurBehindWindow(wnd->hWnd, &bb);
}

FORCEINLINE 
LONG
w32_set_timer_resolution(
  ULONG  hnsDesiredResolution, 
  BOOL   setResolution, 
  PULONG hnsCurrentResolution)
{
  ULONG    _;
  NTSTATUS status = NtSetTimerResolution(
    hnsDesiredResolution,
    (BOOLEAN) !!setResolution,
    hnsCurrentResolution? hnsCurrentResolution : &_
  );
  return (LONG) status;
}

FORCEINLINE 
HANDLE
w32_create_high_resolution_timer(
  LPSECURITY_ATTRIBUTES lpTimerAttributes,
  LPCTSTR               lpszTimerName, 
  DWORD                 dwDesiredAccess)
{
  return CreateWaitableTimerEx(
    lpTimerAttributes, 
    lpszTimerName,
    CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
    dwDesiredAccess
  );
}

FORCEINLINE 
BOOL
w32_yield_on_high_resolution_timer(
  HANDLE               hTimer, 
  const PLARGE_INTEGER dueTime)
{
  BOOL set = SetWaitableTimerEx(hTimer, dueTime, 0, 0, 0, NULL, 0);
  if (!set)
  {
    return FALSE;
  }
  else
  {
    DWORD result = WaitForSingleObjectEx(hTimer, INFINITE, TRUE);
    return (result == WAIT_OBJECT_0);
  }
}

FORCEINLINE 
BOOL
w32_hectonano_sleep(
  LONGLONG hns)
{
  BOOL          result   = FALSE;
  LARGE_INTEGER due_time = {0};
  HANDLE        timer = w32_create_high_resolution_timer(NULL, NULL, TIMER_MODIFY_STATE);
  if (NULL == timer)
  {
    return FALSE;
  }
  due_time.QuadPart = hns;
  result = w32_yield_on_high_resolution_timer(timer, &due_time);
  (VOID) CloseHandle(timer);
  return result;
}

FORCEINLINE 
BOOL
w32_adjust_window_start_point(
  LPPOINT point)
{
  MONITORINFO mi = {sizeof(MONITORINFO)};
  if (GetMonitorInfo(
      MonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST), 
      (LPMONITORINFO)&mi))
  {
    if (!PtInRect(&mi.rcWork, *point))
    {
      POINT x_check = {point->x, mi.rcWork.top};
      POINT y_check = {mi.rcWork.left, point->y};
    
      if (!PtInRect(&mi.rcWork, x_check))
      {
        point->x = mi.rcWork.left;
      }
      if (!PtInRect(&mi.rcWork, y_check))
      {
        point->y = mi.rcWork.top;
      }
    }
    return TRUE;
  }
  return FALSE;
}

