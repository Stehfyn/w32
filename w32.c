/*=============================================================================
** 1. REFERENCES
**===========================================================================*/
/*=============================================================================
** 2. INCLUDE FILES
**===========================================================================*/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "w32.h"
#include <intrin.h>
#include <windowsx.h>
#pragma warning(disable : 4255 4820)
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <winternl.h>
#pragma warning(default : 4255 4820)


#include <GL\GL.h>
//#include <GL\GLU.h>
//#include <GL\glext.h>
#include <GL\wglext.h>
#define RETURN_FALSE_IF_NULL(p) if (!p) return FALSE;
/*=============================================================================
** 3. DECLARATIONS
**===========================================================================*/
/*=============================================================================
** 3.1 Macros
**===========================================================================*/
#define STATUS_SUCCESS (0x00000000)
#define ASSERT_W32(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define GL_BGRA        (0x80E1)
/*=============================================================================
** 3.2 Types
**===========================================================================*/

/*=============================================================================
** 3.3 External global variables
**===========================================================================*/

/*=============================================================================
** 3.4 Static global variables
**===========================================================================*/
static HDC     g_hDC;
static HGLRC   g_hRC;
static INT     g_nWidth  = 0;
static INT     g_nHeight = 0;

/*=============================================================================
** 3.5 External function prototypes
**===========================================================================*/
NTSYSAPI NTSTATUS NTAPI
NtSetTimerResolution(
    ULONG   DesiredResolution,
    BOOLEAN SetResolution,
    PULONG  CurrentResolution
    );

/*=============================================================================
** 3.5 Static function prototypes
**===========================================================================*/
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

VOID CFORCEINLINE
on_destroy(
    HWND hWnd
    );

VOID CFORCEINLINE
on_size(
    HWND hWnd,
    UINT state,
    INT  cx,
    INT  cy
    );

VOID CFORCEINLINE
on_keyup(
    HWND hWnd,
    UINT vk,
    BOOL fDown,
    INT  cRepeat,
    UINT flags
    );

VOID CFORCEINLINE
on_activate(
    HWND hWnd,
    UINT state,
    HWND hwndActDeact,
    BOOL fMinimized
    );

UINT CFORCEINLINE
on_nchittest(
    HWND hWnd,
    INT  x,
    INT  y
    );

UINT CFORCEINLINE
on_nccalcsize(
    HWND               hWnd,
    BOOL               fCalcValidRects,
    NCCALCSIZE_PARAMS* lpcsp
    );

UINT CFORCEINLINE
on_erasebkgnd(
    HWND hWnd,
    HDC  hDC
    );

VOID CFORCEINLINE
on_paint(
    HWND hWnd
    );

/*=============================================================================
** 4. Private functions
**===========================================================================*/
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

      SetLastError(NO_ERROR);
      offset = SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) user_data);
      ASSERT_W32((0 == offset) && (NO_ERROR == GetLastError()));
    }
    else
    {
      w32_window* wnd = (w32_window*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

      if (wnd)
      {
        if(wnd->lpfnWndProcOverride)
        {
          return wnd->lpfnWndProcOverride(hWnd, msg, wParam, lParam, wnd->lpfnWndProcHook, wnd->lpUserData);
        }
        else if (wnd->lpfnWndProcHook)
        {
          LRESULT result = 0;
          if (wnd->lpfnWndProcHook(hWnd, msg, wParam, lParam, &result, wnd->lpUserData))
            return result;
        }
      }

      switch (msg) {
      HANDLE_MSG(hWnd, WM_DESTROY, on_destroy);
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

VOID CFORCEINLINE
on_destroy(
    HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);

    PostQuitMessage(0);
}

BOOL CFORCEINLINE
on_ncactivate(
    HWND hWnd,
    BOOL fActive,
    HWND hwndActDeact,
    BOOL fMinimized)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(fActive);
    UNREFERENCED_PARAMETER(hwndActDeact);
    UNREFERENCED_PARAMETER(fMinimized);
    return TRUE;
}

VOID CFORCEINLINE
on_size(
    HWND hWnd,
    UINT state,
    INT  cx,
    INT  cy)
{
    UNREFERENCED_PARAMETER(hWnd);

    if(SIZE_MINIMIZED != state)
    {
      g_nWidth  = cx;
      g_nHeight = cy;
    }
}

VOID CFORCEINLINE
on_keyup(
    HWND hWnd,
    UINT vk,
    BOOL fDown,
    INT  cRepeat,
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
      if (hWnd == GetForegroundWindow())
        //CloseWindow(hWnd);
        DestroyWindow(hWnd);
      break;
    }
    default: break;
    }
}

VOID CFORCEINLINE
on_activate(
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

UINT CFORCEINLINE
on_nchittest(
    HWND hWnd,
    INT  x,
    INT  y)
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

    CONST INT result =
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

UINT CFORCEINLINE
on_nccalcsize(
    HWND               hWnd,
    BOOL               fCalcValidRects,
    NCCALCSIZE_PARAMS* lpcsp)
{
    if (fCalcValidRects)
    {
      if (IsMaximized(hWnd))
      {
        MONITORINFO monitor_info = {sizeof(MONITORINFO)};
        if (GetMonitorInfo(
              MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),
              (LPMONITORINFO)&monitor_info
        ))
        {
          lpcsp->rgrc[0] = monitor_info.rcWork;
          return 0;
        }
      }
      lpcsp->rgrc[0].bottom += 1;
      return WVR_VALIDRECTS;
    }
    return 0;
}

UINT CFORCEINLINE
on_erasebkgnd(
    HWND hWnd,
    HDC  hDC)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(hDC);

    return 1U;
}

VOID CFORCEINLINE
on_paint(
    HWND hWnd)
{
    PAINTSTRUCT ps;
    (void) BeginPaint(hWnd, &ps);
    (void) EndPaint(hWnd, &ps);
    DwmFlush();
    FORWARD_WM_PAINT(hWnd, DefWindowProc);
}

HICON FORCEINLINE
load_icon(
    LPCTSTR lpszIconFileName)
{
    if (lpszIconFileName)
    {
      return (HICON) LoadImage(
        GetModuleHandle(NULL),
        (LPCWSTR) lpszIconFileName,
        IMAGE_ICON,
        0,
        0,
        LR_LOADFROMFILE
      );
    }
    return NULL;
}

/*=============================================================================
** 5. Public functions
**===========================================================================*/
LPCTSTR FORCEINLINE
w32_create_window_class(
    LPCTSTR lpszClassName,
    LPCTSTR lpszIconFileName,
    UINT    style)
{
    WNDCLASSEX wcex = {0};
    {
      ATOM _ = 0;
      wcex.cbSize        = sizeof(WNDCLASSEX);
      wcex.lpszClassName = lpszClassName;
      wcex.style         = style;
      wcex.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
      //wcex.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
      wcex.hCursor     = LoadCursor(NULL, IDC_ARROW);
      wcex.lpfnWndProc = (WNDPROC) wndproc;
      wcex.hIcon       = (HICON) LoadImage(
        GetModuleHandle(NULL),
        lpszIconFileName,
        IMAGE_ICON,
        0,
        0,
        LR_LOADFROMFILE
      );
      wcex.hIconSm = wcex.hIcon;
      _            = RegisterClassEx(&wcex);
      ASSERT_W32(_);
    }
    return lpszClassName;
}

BOOL FORCEINLINE
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
    return !!wnd->hWnd;
}

BOOL FORCEINLINE
w32_pump_message_loop(
    HWND        hwndPump)
{
    MSG  msg;
    BOOL quit = FALSE;
    RtlSecureZeroMemory(&msg, sizeof(MSG));

    while (PeekMessage(&msg, hwndPump, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      quit |= (msg.message == WM_QUIT);
    }
    return !quit;
}

FORCEINLINE VOID
w32_run_message_loop(
    HWND        hwndPump)
{
    MSG  msg;
    RtlSecureZeroMemory(&msg, sizeof(MSG));

    for (;;) {
      BOOL received = GetMessage(&msg, hwndPump, 0, 0) != 0;
      if (received != -1) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
          break;
      }
    }
}

BOOL FORCEINLINE
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

BOOL FORCEINLINE
w32_set_process_dpiaware(
    VOID)
{
    return S_OK == SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

BOOL FORCEINLINE
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

LONG FORCEINLINE
w32_set_timer_resolution(
    ULONG  hnsDesiredResolution,
    BOOL   setResolution,
    PULONG hnsCurrentResolution)
{
    ULONG _ = {0};
    return STATUS_SUCCESS == NtSetTimerResolution(
      hnsDesiredResolution,
      (BOOLEAN) !!setResolution,
      hnsCurrentResolution? hnsCurrentResolution : &_
    );
}

HANDLE FORCEINLINE
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

BOOL FORCEINLINE
w32_yield_on_high_resolution_timer(
    HANDLE               hTimer,
    const PLARGE_INTEGER dueTime)
{
    if (!SetWaitableTimerEx(hTimer, dueTime, 0, 0, 0, 0, 0))
      return FALSE;
    return (WAIT_OBJECT_0 == WaitForSingleObjectEx(hTimer, INFINITE, TRUE));
}

BOOL FORCEINLINE
w32_hectonano_sleep(
    LONGLONG hns)
{
    BOOL          result   = FALSE;
    LARGE_INTEGER due_time = {0};
    HANDLE        timer    = w32_create_high_resolution_timer(0, 0, TIMER_MODIFY_STATE);
    if (!timer)
    {
      return FALSE;
    }
    due_time.QuadPart = hns;
    result            = w32_yield_on_high_resolution_timer(timer, &due_time);
    (VOID) CloseHandle(timer);
    return result;
}

BOOL FORCEINLINE
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

EXTERN_C BOOL FORCEINLINE
w32_timer_init(
    w32_timer* tmr)
{
    return QueryPerformanceFrequency(&tmr->freq);
}

EXTERN_C BOOL FORCEINLINE
w32_timer_start(
    w32_timer* tmr)
{
    return QueryPerformanceCounter(&tmr->start);
}

EXTERN_C BOOL FORCEINLINE
w32_timer_stop(
    w32_timer* tmr)
{
    BOOL success = QueryPerformanceCounter(&tmr->stop);
    LONGLONG e = tmr->stop.QuadPart - tmr->start.QuadPart;
    if(!isinf(e))
    {
      tmr->elapsed.QuadPart = e;
      tmr->elapsedAccum.QuadPart +=e;
    }
    return success;
}

EXTERN_C DOUBLE FORCEINLINE
w32_timer_elapsed(
    w32_timer* tmr)
{
    DOUBLE e = (DOUBLE)tmr->elapsedAccum.QuadPart / (DOUBLE)tmr->freq.QuadPart;
    return isinf(e) ? 0.0 : e;
}

EXTERN_C BOOL FORCEINLINE
w32_timer_reset(
    w32_timer* tmr)
{
    tmr->elapsed.QuadPart = 0;
    tmr->elapsedAccum.QuadPart = 0;
    tmr->start.QuadPart = 0;
    tmr->stop.QuadPart = 0;
    return TRUE;
}

EXTERN_C LRESULT
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
    HANDLE_MSG(hWnd, WM_SIZE,       on_size);
    HANDLE_MSG(hWnd, WM_KEYUP,      on_keyup);
    HANDLE_MSG(hWnd, WM_ACTIVATE,   on_activate);
    HANDLE_MSG(hWnd, WM_NCHITTEST,  on_nchittest);
    HANDLE_MSG(hWnd, WM_NCCALCSIZE, on_nccalcsize);
    //HANDLE_MSG(hWnd, WM_PAINT,      on_paint);
    HANDLE_MSG(hWnd, WM_ERASEBKGND, on_erasebkgnd);
    HANDLE_MSG(hWnd, WM_DESTROY,    on_destroy);
    case (WM_ENTERSIZEMOVE): {
      (void) SetTimer(hWnd, (UINT_PTR)0, USER_TIMER_MINIMUM, NULL);
      return 0;
    }
    case (WM_TIMER): {
      static w32_window* wnd = NULL;
      if(!wnd){

        wnd = (w32_window*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
      }
      //on_paint(hWnd);
      return 0;
    }
    case (WM_EXITSIZEMOVE): {
      (void) KillTimer(hWnd, (UINT_PTR)0);
      return 0;
    }
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

EXTERN_C BOOL CFORCEINLINE
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

PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB    = NULL;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT         = NULL;
PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT      = NULL;
HMODULE                           wgl;
typedef PROC (__stdcall* _glw32_get_proc_addr)(LPCSTR);
static _glw32_get_proc_addr wgl_get_proc_address;
typedef VOID (* LPFNVOID)(
  VOID
);

PROC CFORCEINLINE
wgl_load_proc(
    LPCSTR proc)
{
    PROC res = NULL;
    res = wgl_get_proc_address(proc);
    if(!res)
    {
      res = (PROC) GetProcAddress(wgl, proc);
    }
    return res;
}

static BOOL
w32_wgl_init(
    VOID)
{
    wgl = LoadLibrary(_T("opengl32.dll"));
    ASSERT_W32(wgl);
    wgl_get_proc_address =(_glw32_get_proc_addr) (LPVOID) GetProcAddress(wgl, "wglGetProcAddress");
    ASSERT_W32(wgl_get_proc_address);
    wglChoosePixelFormatARB =
      (PFNWGLCHOOSEPIXELFORMATARBPROC) (LPVOID) wgl_load_proc("wglChoosePixelFormatARB");
    ASSERT_W32(wglChoosePixelFormatARB);
    wglCreateContextAttribsARB =
      (PFNWGLCREATECONTEXTATTRIBSARBPROC) (LPVOID) wgl_load_proc("wglCreateContextAttribsARB");
    ASSERT_W32(wglCreateContextAttribsARB);
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) (LPVOID) wgl_load_proc("wglSwapIntervalEXT");
    ASSERT_W32(wglSwapIntervalEXT);
    wglGetSwapIntervalEXT =
      (PFNWGLGETSWAPINTERVALEXTPROC) (LPVOID) wgl_load_proc("wglGetSwapIntervalEXT");
    ASSERT_W32(wglGetSwapIntervalEXT);

    return TRUE;
}

INT
w32_wgl_get_pixel_format(
    UINT msaa)
{
    HWND     hWnd;
    HDC      hDC;
    HGLRC    hRC;
    WNDCLASS wc;
    INT      pfMSAA;

    PIXELFORMATDESCRIPTOR pfd;
    SecureZeroMemory(&wc, sizeof(WNDCLASS));
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpfnWndProc   = DefWindowProc;
    wc.lpszClassName = _T("GLEW");
    if (0 == RegisterClass(&wc)) return GL_TRUE;
    hWnd = CreateWindow(
      _T("GLEW"),
      _T("GLEW"),
      WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      NULL,
      NULL,
      GetModuleHandle(NULL),
      NULL
    );
    if (NULL == hWnd) return GL_TRUE;
    hDC = GetDC(hWnd);
    if (NULL == hDC) return GL_TRUE;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize    = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags  = PFD_DRAW_TO_WINDOW |     
                   PFD_SUPPORT_OPENGL |      
                   PFD_SUPPORT_COMPOSITION | 
                   PFD_GENERIC_ACCELERATED |
                   PFD_SWAP_EXCHANGE |
                   PFD_DOUBLEBUFFER;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cColorBits = 32;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfMSAA         = ChoosePixelFormat(hDC, &pfd);
    if (pfMSAA == 0) return GL_TRUE;
    if(0 == SetPixelFormat(hDC, pfMSAA, &pfd)) return GL_TRUE;
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    w32_wgl_init();

    while (msaa > 0)
    {
      UINT num_formats = 0;
      int  pfAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_SWAP_METHOD_ARB,WGL_SWAP_EXCHANGE_ARB,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, (INT)msaa,
        0
      };
      if (wglChoosePixelFormatARB(hDC, pfAttribs, NULL, 1, &pfMSAA, &num_formats))
      {
        if (num_formats > 0)
        {
          break;
        }
      }
      msaa--;
    }

    if (NULL != hRC) wglMakeCurrent(NULL, NULL);
    if (NULL != hRC) wglDeleteContext(hRC);
    if (NULL != hWnd && NULL != hDC) ReleaseDC(hWnd, hDC);
    if (NULL != hWnd) DestroyWindow(hWnd);
    UnregisterClass(_T("GLEW"), GetModuleHandle(NULL));
    return pfMSAA;
}

EXTERN_C VOID CFORCEINLINE
w32_wgl_attach_device(
    w32_window* wnd)
{
    PIXELFORMATDESCRIPTOR pfd  = {0};
    INT                   msaa = w32_wgl_get_pixel_format(16U);
    pfd.nSize    = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags  = PFD_DRAW_TO_WINDOW |     // Format Must Support Window
                   PFD_SUPPORT_OPENGL |     // Format Must Support OpenGL
                   PFD_SUPPORT_COMPOSITION | // Format Must Support Composition
                   PFD_GENERIC_ACCELERATED |
                   PFD_SWAP_EXCHANGE      |
                   PFD_DOUBLEBUFFER;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cColorBits = 32;
    pfd.iPixelType = PFD_TYPE_RGBA;
    HDC hDC = GetDC(wnd->hWnd);
    ASSERT_W32(SetPixelFormat(hDC, msaa, &pfd));
    HGLRC hRC = wglCreateContextAttribsARB(hDC, NULL, NULL);
    ASSERT_W32(hRC);
    ASSERT_W32(wglMakeCurrent(hDC, hRC));
    ASSERT_W32(wglSwapIntervalEXT(0));
}
