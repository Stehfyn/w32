/*=============================================================================
** 1. REFERENCES
**===========================================================================*/
/*=============================================================================
** 2. INCLUDE FILES
**===========================================================================*/

#include "w32.h"
#include <intrin.h>
#include <windowsx.h>

#pragma warning(disable : 4255 4820)
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <winternl.h>
#pragma warning(default : 4255 4820)

/*=============================================================================
** 3. DECLARATIONS
**===========================================================================*/
/*=============================================================================
** 3.1 Macros
**===========================================================================*/
#define ASSERT_W32(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define STATUS_SUCCESS                  (0)
#define STATUS_TIMER_RESOLUTION_NOT_SET (0xC0000245)

/*=============================================================================
** 3.2 Types
**===========================================================================*/

/*=============================================================================
** 3.3 External global variables
**===========================================================================*/

/*=============================================================================
** 3.4 Static global variables
**===========================================================================*/

/*=============================================================================
** 3.5 External function prototypes
**===========================================================================*/
NTSYSAPI
NTSTATUS
NTAPI
NtSetTimerResolution(
  ULONG   DesiredResolution,
  BOOLEAN SetResolution,
  PULONG  CurrentResolution
);

/*=============================================================================
** 3.5 Static function prototypes
**===========================================================================*/
static
LRESULT
wndproc(
  HWND   hWnd,
  UINT   msg,
  WPARAM wParam,
  LPARAM lParam
);

static
BOOL
monitorenumproc(
  HMONITOR hMonitor,
  HDC      hDC,
  LPRECT   lpRect,
  LPARAM   lParam
);

CFORCEINLINE
VOID
on_wm_destroy(
  HWND hWnd
);

CFORCEINLINE
VOID
borderless_on_wm_keyup(
  HWND hWnd,
  UINT vk,
  BOOL fDown,
  int  cRepeat,
  UINT flags
);

CFORCEINLINE
VOID
borderless_on_wm_activate(
  HWND hWnd,
  UINT state,
  HWND hwndActDeact,
  BOOL fMinimized
);

CFORCEINLINE
UINT
borderless_on_wm_nchittest(
  HWND hWnd,
  int  x,
  int  y
);

CFORCEINLINE
UINT
borderless_on_wm_nccalcsize(
  HWND               hWnd,
  BOOL               fCalcValidRects,
  NCCALCSIZE_PARAMS* lpcsp
);

CFORCEINLINE
UINT
borderless_on_wm_erasebkgnd(
  HWND hWnd,
  HDC  hDC
);

/*=============================================================================
** 4. Private functions
**===========================================================================*/
static
LRESULT
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
      if(wnd->lpfnWndProcOverride)
      {
        return wnd->lpfnWndProcOverride(
          hWnd,
          msg,
          wParam,
          lParam,
          wnd->lpfnWndProcHook,
          wnd->lpUserData
        );
      }
      else if (wnd->lpfnWndProcHook)
      {
        LRESULT result  = 0;
        BOOL    handled = wnd->lpfnWndProcHook(
          hWnd,
          msg,
          wParam,
          lParam,
          &result,
          wnd->lpUserData
        );

        if (handled)
        {
          return result;
        }
      }
    }

    switch (msg) {
    HANDLE_MSG(hWnd, WM_DESTROY, on_wm_destroy);
    default:
      break;
    }
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

static
BOOL
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

  display_info            = (w32_display_info*) lParam;
  monitor_info            = (w32_monitor_info*) &display_info->monitors[display_info->numMonitors];
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

CFORCEINLINE
VOID
on_wm_destroy
(
  HWND hWnd)
{
  UNREFERENCED_PARAMETER(hWnd);
  PostQuitMessage(0);
}

CFORCEINLINE
VOID
borderless_on_wm_keyup(
  HWND hWnd,
  UINT vk,
  BOOL fDown,
  int  cRepeat,
  UINT flags)
{
  UNREFERENCED_PARAMETER(fDown);
  UNREFERENCED_PARAMETER(cRepeat);
  UNREFERENCED_PARAMETER(flags);

  switch (vk) {
  case VK_SPACE:
  {
    RECT  r    = {0};
    POINT p    = {0};
    INPUT i[2] = {0};
    (VOID) SecureZeroMemory(i, sizeof(i));
    (VOID) GetWindowRect(hWnd, &r);
    p.x = (LONG)(r.left + (0.5f * (labs(r.right - r.left))));
    p.y = (LONG)(r.top + (0.5f * (labs(r.bottom - r.top))));
    (VOID) SendMessage(hWnd, WM_NCLBUTTONDBLCLK, HTCAPTION, (LPARAM)&p);

    break;
  }
  case VK_ESCAPE:
  {
    if(hWnd == GetActiveWindow())
    {
      (VOID) PostMessage(hWnd, WM_DESTROY, 0, 0);
    }

    break;
  }
  default: break;
  }
}

CFORCEINLINE
VOID
borderless_on_wm_activate(
  HWND hWnd,
  UINT state,
  HWND hwndActDeact,
  BOOL fMinimized)
{
  UNREFERENCED_PARAMETER(state);
  UNREFERENCED_PARAMETER(hwndActDeact);

  if (!fMinimized)
  {
    static
    //const MARGINS m = {0,0,1,0};
    const MARGINS m = {1,1,1,1};
    //const MARGINS m = {0,0,0,0};
    (VOID) DwmExtendFrameIntoClientArea(hWnd, &m);
    // Inform the application of the frame change.
    (VOID) SetWindowPos(
      hWnd,
      NULL,
      0,
      0,
      0,
      0,
      SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE
    );
  }
}

CFORCEINLINE
UINT
borderless_on_wm_nchittest(
  HWND hWnd,
  int  x,
  int  y)
{
  RECT        r      = {0};
  const POINT cursor = {(LONG) x, (LONG) y};
  const SIZE  border =
  {
    (LONG) (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)),
    (LONG) (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER))  // Padded border is symmetric for both x, y
  };
  (VOID) GetWindowRect(hWnd, &r);

  enum region_mask
  {
    client = 0b0000,
    left   = 0b0001,
    right  = 0b0010,
    top    = 0b0100,
    bottom = 0b1000,
  };

  const INT result =
    left   * (cursor.x <  (r.left   + border.cx)) |
    right  * (cursor.x >= (r.right  - border.cx)) |
    top    * (cursor.y <  (r.top    + border.cy)) |
    bottom * (cursor.y >= (r.bottom - border.cy));

  switch (result) {
  case left:           return HTLEFT;
  case right:          return HTRIGHT;
  case top:            return HTTOP;
  case bottom:         return HTBOTTOM;
  case top | left:     return HTTOPLEFT;
  case top | right:    return HTTOPRIGHT;
  case bottom | left:  return HTBOTTOMLEFT;
  case bottom | right: return HTBOTTOMRIGHT;
  case client:         return HTCAPTION;
  default:             return FORWARD_WM_NCHITTEST(hWnd, x, y, DefWindowProc);
  }
}

CFORCEINLINE
UINT
borderless_on_wm_nccalcsize(
  HWND               hWnd,
  BOOL               fCalcValidRects,
  NCCALCSIZE_PARAMS* lpcsp)
{
  if (fCalcValidRects)
  {
    UINT dpi     = GetDpiForWindow(hWnd);
    int  frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
    int  frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int  padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    if (IsMaximized(hWnd))
    {
      MONITORINFO monitor_info = {sizeof(MONITORINFO)};
      if (GetMonitorInfo(
            MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),
            (LPMONITORINFO)&monitor_info
      ))
      {
        lpcsp->rgrc[0]         = monitor_info.rcWork;
        lpcsp->rgrc[0].right  += frame_x + padding;
        lpcsp->rgrc[0].left   -= frame_x + padding;
        lpcsp->rgrc[0].bottom += frame_y + padding - 2;
        return 0u;
      }
    }

    //lpcsp->rgrc[0].right  -= frame_x + padding;
    //lpcsp->rgrc[0].left   += frame_x + padding;
    //lpcsp->rgrc[0].bottom -= (frame_y + padding);

    lpcsp->rgrc[0].bottom += 1;
    //lpcsp->rgrc[0].bottom -= 1;

    //lpcsp->rgrc[0].top   += 1;
    //lpcsp->rgrc[0].right -= 1;
    //lpcsp->rgrc[0].left  += 1;

    //return WVR_VALIDRECTS;
    return WVR_VALIDRECTS;
    //return 0;
  }
  else
  {
    return FORWARD_WM_NCCALCSIZE(hWnd, fCalcValidRects, lpcsp, DefWindowProc);
  }
}

CFORCEINLINE
UINT
borderless_on_wm_erasebkgnd(
  HWND hWnd,
  HDC  hDC)
{
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(hDC);

  return 1U;
}

/*=============================================================================
** 5. Public functions
**===========================================================================*/
FORCEINLINE
LPCTSTR
w32_create_window_class(
  LPCTSTR lpszClassName,
  UINT    style)
{
  WNDCLASSEX wcex = {0};
  {
    ATOM _ = 0;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.lpszClassName = lpszClassName;
    wcex.style         = style;
    wcex.hInstance     = GetModuleHandle(NULL);
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    //wcex.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wcex.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wcex.lpfnWndProc   = (WNDPROC) wndproc;
    _                  = RegisterClassEx(&wcex);
    ASSERT_W32(_);
  }
  return lpszClassName;
}

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
  DWORD           dwExStyle,
  DWORD           dwStyle,
  WNDPROCOVERRIDE lpfnWndProcOverride,
  WNDPROCHOOK     lpfnWndProcHook,
  LPVOID          lpUserData)
{
  ASSERT_W32(wnd);
  wnd->lpfnWndProcOverride = lpfnWndProcOverride;
  wnd->lpfnWndProcHook     = lpfnWndProcHook;
  wnd->lpUserData          = lpUserData;
  wnd->hWnd                = CreateWindowEx(
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
  w32_window* wnd,
  BOOL        pumpThread)
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
  ULONG _;
  NTSTATUS
    status = NtSetTimerResolution(
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
  HANDLE        timer    = w32_create_high_resolution_timer(NULL, NULL, TIMER_MODIFY_STATE);
  if (NULL == timer)
  {
    return FALSE;
  }
  due_time.QuadPart = hns;
  result            = w32_yield_on_high_resolution_timer(timer, &due_time);
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
        (LPMONITORINFO)&mi
  ))
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

EXTERN_C
FORCEINLINE
BOOL
w32_timer_init(
  w32_timer* tmr)
{
  return QueryPerformanceFrequency(&tmr->freq);
}

EXTERN_C
FORCEINLINE
BOOL
w32_timer_start(
  w32_timer* tmr)
{
  return QueryPerformanceCounter(&tmr->start);
}

EXTERN_C
FORCEINLINE
BOOL
w32_timer_stop(
  w32_timer* tmr)
{
  BOOL success = QueryPerformanceCounter(&tmr->stop);
  tmr->elapsed.QuadPart = tmr->stop.QuadPart - tmr->start.QuadPart;
  return success;
}

EXTERN_C
LRESULT
w32_borderless_wndproc(
  HWND        hWnd,
  UINT        msg,
  WPARAM      wParam,
  LPARAM      lParam,
  WNDPROCHOOK lpfnWndProcHook,
  LPVOID      lpUserData)
{
  if (lpfnWndProcHook)
  {
    LRESULT result  = 0;
    BOOL    handled = lpfnWndProcHook(hWnd, msg, wParam, lParam, &result, lpUserData);
    if (handled)
    {
      return result;
    }
  }

  switch (msg) {
  case WM_PAINT: {
    //PAINTSTRUCT ps = {0};
    //(VOID) BeginPaint(hWnd, &ps);
    //(VOID) EndPaint(hWnd, &ps);
    DwmFlush();

    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
    HANDLE_MSG(hWnd, WM_KEYUP,      borderless_on_wm_keyup);
    HANDLE_MSG(hWnd, WM_ACTIVATE,   borderless_on_wm_activate);
    HANDLE_MSG(hWnd, WM_NCHITTEST,  borderless_on_wm_nchittest);
    HANDLE_MSG(hWnd, WM_NCCALCSIZE, borderless_on_wm_nccalcsize);
    //HANDLE_MSG(hWnd, WM_ERASEBKGND, borderless_on_wm_erasebkgnd);
    HANDLE_MSG(hWnd, WM_DESTROY,    on_wm_destroy);
  default: return DefWindowProc(hWnd, msg, wParam, lParam);
  }
}

EXTERN_C
FORCEINLINE
BOOL
w32_get_user_state(
  w32_user_state* us)
{
  ULONG_PTR pbi[6];
  ULONG     ulSize = 0;
  (VOID) GetCursorPos(&us->cursor);
  us->monitorInfo.cbSize = sizeof(MONITORINFO);
  (VOID) GetMonitorInfo(MonitorFromPoint(us->cursor, MONITOR_DEFAULTTONEAREST), &us->monitorInfo);
  if(NtQueryInformationProcess(
       GetCurrentProcess(),
       0,
       &pbi,
       sizeof(pbi),
       &ulSize
     ) >= 0 && ulSize == sizeof(pbi))
  {
    DWORD size = 300;
    us->piActiveProcessID = pbi[5];
    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD) us->piActiveProcessID);
    (VOID) QueryFullProcessImageNameW(p, 0, us->imageName, &size);
    return TRUE;
  }
  us->piActiveProcessID = (ULONG_PTR)-1;
  return FALSE;
}

EXTERN_C
CFORCEINLINE
BOOL
w32_get_centered_window_point(
  LPPOINT      p,
  CONST LPSIZE sz)
{
  MONITORINFO mi = {sizeof(MONITORINFO)};
  if (GetMonitorInfo(
        MonitorFromPoint(*p, MONITOR_DEFAULTTONEAREST),
        (LPMONITORINFO)&mi
  ))
  {
    p->x = (LONG)(mi.rcWork.left + (0.5 * (labs(mi.rcWork.right - mi.rcWork.left) - sz->cx)));
    p->y = (LONG)(mi.rcWork.top  + (0.5 * (labs(mi.rcWork.bottom - mi.rcWork.top) - sz->cy)));
    return TRUE;
  }
  return FALSE;
}
