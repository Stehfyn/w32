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
#pragma warning(disable : 4255 4820 4431 4218 4820)
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <winternl.h>
#define  NTSTATUS LONG
#include <D3dkmthk.h>
#pragma warning(default : 4255 4820 4431 4218 4820)

/*=============================================================================
** 3. DECLARATIONS
**===========================================================================*/
/*=============================================================================
** 3.1 Macros
**===========================================================================*/
#define STATUS_SUCCESS (0x00000000)
#define ASSERT_W32(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define BGR(color) RGB(GetBValue(color), GetGValue(color), GetRValue(color))

#define HANDLE_WM_ENTERSIZEMOVE(hWnd, wParam, lParam, fn) \
        ((fn)((hWnd)), 0L)
#define HANDLE_WM_EXITSIZEMOVE(hWnd, wParam, lParam, fn) \
        ((fn)((hWnd)), 0L)

/*=============================================================================
** 3.2 Types
**===========================================================================*/

/*=============================================================================
** 3.3 External global variables
**===========================================================================*/

/*=============================================================================
** 3.4 Static global variables
**===========================================================================*/

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
MonitorEnumProc(
    HMONITOR hMonitor,
    HDC      hDC,
    LPRECT   lpRect,
    LPARAM   lParam
    );

VOID CFORCEINLINE
OnDestroy(
    HWND hWnd
    );

VOID CFORCEINLINE
OnSize(
    HWND hWnd,
    UINT state,
    INT  cx,
    INT  cy
    );

VOID CFORCEINLINE
OnKeyUp(
    HWND hWnd,
    UINT vk,
    BOOL fDown,
    INT  cRepeat,
    UINT flags
    );

VOID CFORCEINLINE
OnActivate(
    HWND hWnd,
    UINT state,
    HWND hwndActDeact,
    BOOL fMinimized
    );

UINT CFORCEINLINE
OnNcHittest(
    HWND hWnd,
    INT  x,
    INT  y
    );

UINT CFORCEINLINE
OnNcCalcSize(
    HWND               hWnd,
    BOOL               fCalcValidRects,
    NCCALCSIZE_PARAMS* lpcsp
    );

UINT CFORCEINLINE
OnEraseBkgnd(
    HWND hWnd,
    HDC  hDC
    );

VOID CFORCEINLINE
OnPaint(
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
      HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
      default:
        break;
      }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static BOOL
MonitorEnumProc(
    HMONITOR hMonitor,
    HDC      hDC,
    LPRECT   lpRect,
    LPARAM   lParam)
{
    UNREFERENCED_PARAMETER(hDC);

    PDISPLAYCONFIG   lpDisplayConfig = NULL;
    PDISPLAYINFO     lpDisplayInfo   = NULL;
    LPMONITORINFOEX   lpMonitorInfoEx = NULL;
    LPDEVMODE         lpDevMode       = NULL;

    lpDisplayConfig         = (PDISPLAYCONFIG)lParam;
    lpDisplayInfo           = (PDISPLAYINFO)&lpDisplayConfig->lpDisplays[lpDisplayConfig->nDisplays];
    lpMonitorInfoEx         = (LPMONITORINFOEX)&lpDisplayInfo->monitorInfoEx;
    lpDevMode               = (LPDEVMODE)&lpDisplayInfo->deviceMode;
    lpMonitorInfoEx->cbSize = (DWORD)sizeof(MONITORINFOEX);
    lpDevMode->dmSize       = (WORD)sizeof(DEVMODE);

    (VOID)GetMonitorInfo(
      hMonitor,
      (LPMONITORINFO)lpMonitorInfoEx
    );
    (VOID)UnionRect(
      &lpDisplayConfig->rcBound,
      &lpDisplayConfig->rcBound,
      lpRect
    );
#ifdef UNICODE
    (VOID)wcsncpy_s(
      lpDisplayInfo->deviceNameW,
      CCHDEVICENAME + 1,
      lpMonitorInfoEx->szDevice,
      CCHDEVICENAME
    );
    (VOID)WideCharToMultiByte(
      CP_UTF8,
      0,
      (LPCWCH)lpMonitorInfoEx->szDevice,
      CCHDEVICENAME,
      (LPSTR)lpDisplayInfo->deviceName,
      CCHDEVICENAME + 1,
      0,
      NULL
    );
    (VOID)EnumDisplaySettings(
      lpDisplayInfo->deviceNameW,
      ENUM_CURRENT_SETTINGS,
      &lpDisplayInfo->deviceMode
    );
#else
    (VOID)strncpy_s(
      lpDisplayInfo->deviceName,
      CCHDEVICENAME + 1,
      lpMonitorInfoEx->szDevice,
      CCHDEVICENAME
    );
    (VOID)MultiByteToWideChar(
      CP_UTF8,
      0,
      (LPCCH)lpMonitorInfoEx->szDevice,
      CCHDEVICENAME,
      (LPWSTR)lpDisplayInfo->deviceNameW,
      CCHDEVICENAME + 1
    );
    (VOID)EnumDisplaySettings(
      lpDisplayInfo->deviceName,
      ENUM_CURRENT_SETTINGS,
      &lpDisplayInfo->deviceMode
    );
#endif
    return (lpDisplayConfig->nDisplays++ < MAX_ENUM_MONITORS);
}

VOID CFORCEINLINE
OnDestroy(
    HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);

    PostQuitMessage(0);
}

BOOL CFORCEINLINE
OnNcActivate(
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
OnSize(
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
OnKeyUp(
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
        DestroyWindow(hWnd);
      break;
    }
    default: break;
    }
}

VOID CFORCEINLINE
OnActivate(
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
OnNcHittest(
    HWND hWnd,
    INT  x,
    INT  y)
{
    RECT        r      = {0};
    const POINT cursor = {(LONG) x, (LONG) y};
    const SIZE  border =
    {
      (LONG)(GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)),
      (LONG)(GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER))  // Padded border is symmetric for both x, y
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
//https://stackoverflow.com/a/53017156/22468901
UINT CFORCEINLINE
OnNcCalcSize(
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
      //lpcsp->rgrc[2] = lpcsp->rgrc[1];
      lpcsp->rgrc[0].bottom += 1;
      return WVR_VALIDRECTS;
      //return 0;
//WVR_
    }
    return 0;
}

UINT CFORCEINLINE
OnEraseBkgnd(
    HWND hWnd,
    HDC  hDC)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(hDC);

    return 1U;
}

VOID CFORCEINLINE
OnPaint(
    HWND hWnd)
{
  static DWORD dwmColor = 0;
  BOOL isOpaque = FALSE;
  DwmGetColorizationColor(&dwmColor, &isOpaque);
  D3DKMT_WAITFORVERTICALBLANKEVENT we;
  D3DKMT_OPENADAPTERFROMHDC oa;
  oa.hDc = GetDC(hWnd);
  D3DKMTOpenAdapterFromHdc(&oa);
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);
    we.hDevice = 0;
    we.hAdapter = oa.hAdapter;
we.VidPnSourceId = oa.VidPnSourceId;
  if(D3DKMTWaitForVerticalBlankEvent(&we))
ExitProcess(0);
    COLORREF color = BGR(dwmColor);
    HBRUSH brush = CreateSolidBrush(color);
    
    FillRect(hDC, &ps.rcPaint, brush);
    DeleteObject(brush);
    EndPaint(hWnd, &ps);
    //DwmFlush();
}

VOID CFORCEINLINE
OnEnterSizeMove(
    HWND hWnd)
{
    SetTimer(hWnd, (UINT_PTR)0, USER_TIMER_MINIMUM, 0);
    RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
}

VOID CFORCEINLINE
OnTimer(
    HWND hWnd,
    UINT id)
{
    UNREFERENCED_PARAMETER(id);
    InvalidateRect(hWnd, 0, 0);
    OnPaint(hWnd);
}

VOID CFORCEINLINE
OnExitSizeMove(
    HWND hWnd)
{
    KillTimer(hWnd, (UINT_PTR)0);
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
      //wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wcex.hbrBackground = NULL;
      wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
      wcex.lpfnWndProc   = (WNDPROC) wndproc;
      wcex.hIcon         = (HICON) LoadImage(
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
PumpMessageQueue(
    HWND hwndPump)
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

VOID FORCEINLINE
RunMessageQueue(
    HWND hwndRun)
{
    MSG  msg;
    RtlSecureZeroMemory(&msg, sizeof(MSG));

    for (;;) {
      BOOL received = GetMessage(&msg, hwndRun, 0, 0) != 0;
      if (received != -1) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
          break;
      }
    }
}

BOOL FORCEINLINE
GetDisplayConfig(
    PDISPLAYCONFIG lpDisplayConfig)
{
    return EnumDisplayMonitors(
      NULL,
      NULL,
      (MONITORENUMPROC)MonitorEnumProc,
      (LPARAM)lpDisplayConfig
    );
}

BOOL FORCEINLINE
RequestSystemDpiAutonomy(
    VOID)
{
    return S_OK == SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

BOOL FORCEINLINE
SetAlphaComposition(
    w32_window* wnd,
    BOOL        fEnabled)
{
    ASSERT_W32(wnd);
    DWM_BLURBEHIND bb;
    RtlSecureZeroMemory(&bb, sizeof(DWM_BLURBEHIND));
    if (fEnabled)
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
SetSystemTimerResolution(
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
CreateHighResolutionTimer(
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
YieldOnTimer(
    HANDLE               hTimer,
    const PLARGE_INTEGER dueTime)
{
    if (!SetWaitableTimerEx(hTimer, dueTime, 0, 0, 0, 0, 0))
      return FALSE;
    return (WAIT_OBJECT_0 == WaitForSingleObjectEx(hTimer, INFINITE, TRUE));
}

BOOL FORCEINLINE
HectonanoSleep(
    LONGLONG hns)
{
    BOOL          result   = FALSE;
    LARGE_INTEGER due_time = {0};
    HANDLE        timer    = CreateHighResolutionTimer(0, 0, TIMER_MODIFY_STATE);
    if (!timer)
    {
      return FALSE;
    }
    due_time.QuadPart = hns;
    result            = YieldOnTimer(timer, &due_time);
    (VOID) CloseHandle(timer);
    return result;
}

BOOL FORCEINLINE
AdjustWindowPoint(
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
WndProc(
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
    HANDLE_MSG(hWnd, WM_SIZE,          OnSize);
    HANDLE_MSG(hWnd, WM_KEYUP,         OnKeyUp);
    HANDLE_MSG(hWnd, WM_NCACTIVATE,    OnNcActivate);
    HANDLE_MSG(hWnd, WM_ACTIVATE,      OnActivate);
    HANDLE_MSG(hWnd, WM_NCHITTEST,     OnNcHittest);
    HANDLE_MSG(hWnd, WM_NCCALCSIZE,    OnNcCalcSize);
    HANDLE_MSG(hWnd, WM_PAINT,         OnPaint);
    HANDLE_MSG(hWnd, WM_TIMER,         OnTimer);
    HANDLE_MSG(hWnd, WM_ENTERSIZEMOVE, OnEnterSizeMove);
    HANDLE_MSG(hWnd, WM_EXITSIZEMOVE,  OnExitSizeMove);
    HANDLE_MSG(hWnd, WM_ERASEBKGND,    OnEraseBkgnd);
    HANDLE_MSG(hWnd, WM_DESTROY,       OnDestroy);
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

EXTERN_C BOOL CFORCEINLINE
GetWindowCenteredPoint(
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

EXTERN_C LPCTSTR FORCEINLINE
DecipherMessage(
    UINT uMsg)
{
#define MSG2STR(x)      \
    case x:             \
        return _T(#x);
  switch (uMsg) {
  MSG2STR(WM_NULL)
  MSG2STR(WM_CREATE)
  MSG2STR(WM_DESTROY)
  MSG2STR(WM_MOVE)
  MSG2STR(WM_SIZE)
  MSG2STR(WM_ACTIVATE)
  MSG2STR(WM_SETFOCUS)
  MSG2STR(WM_KILLFOCUS)
  MSG2STR(WM_ENABLE)
  MSG2STR(WM_SETREDRAW)
  MSG2STR(WM_SETTEXT)
  MSG2STR(WM_GETTEXT)
  MSG2STR(WM_GETTEXTLENGTH)
  MSG2STR(WM_PAINT)
  MSG2STR(WM_CLOSE)
  MSG2STR(WM_QUERYENDSESSION)
  MSG2STR(WM_QUIT)
  MSG2STR(WM_QUERYOPEN)
  MSG2STR(WM_ERASEBKGND)
  MSG2STR(WM_SYSCOLORCHANGE)
  MSG2STR(WM_ENDSESSION)
  MSG2STR(WM_SHOWWINDOW)
  MSG2STR(WM_CTLCOLOR)
  MSG2STR(WM_WININICHANGE)
  MSG2STR(WM_DEVMODECHANGE)
  MSG2STR(WM_ACTIVATEAPP)
  MSG2STR(WM_FONTCHANGE)
  MSG2STR(WM_TIMECHANGE)
  MSG2STR(WM_CANCELMODE)
  MSG2STR(WM_SETCURSOR)
  MSG2STR(WM_MOUSEACTIVATE)
  MSG2STR(WM_CHILDACTIVATE)
  MSG2STR(WM_QUEUESYNC)
  MSG2STR(WM_GETMINMAXINFO)
  MSG2STR(WM_PAINTICON)
  MSG2STR(WM_ICONERASEBKGND)
  MSG2STR(WM_NEXTDLGCTL)
  MSG2STR(WM_SPOOLERSTATUS)
  MSG2STR(WM_DRAWITEM)
  MSG2STR(WM_MEASUREITEM)
  MSG2STR(WM_DELETEITEM)
  MSG2STR(WM_VKEYTOITEM)
  MSG2STR(WM_CHARTOITEM)
  MSG2STR(WM_SETFONT)
  MSG2STR(WM_GETFONT)
  MSG2STR(WM_SETHOTKEY)
  MSG2STR(WM_GETHOTKEY)
  MSG2STR(WM_QUERYDRAGICON)
  MSG2STR(WM_COMPAREITEM)
  MSG2STR(WM_GETOBJECT)
  MSG2STR(WM_COMPACTING)
  MSG2STR(WM_COMMNOTIFY)
  MSG2STR(WM_WINDOWPOSCHANGING)
  MSG2STR(WM_WINDOWPOSCHANGED)
  MSG2STR(WM_POWER)
  //MSG2STR(WM_COPYGLOBALDATA)
  MSG2STR(WM_COPYDATA)
  MSG2STR(WM_CANCELJOURNAL)
  MSG2STR(WM_NOTIFY)
  MSG2STR(WM_INPUTLANGCHANGEREQUEST)
  MSG2STR(WM_INPUTLANGCHANGE)
  MSG2STR(WM_TCARD)
  MSG2STR(WM_HELP)
  MSG2STR(WM_USERCHANGED)
  MSG2STR(WM_NOTIFYFORMAT)
  MSG2STR(WM_CONTEXTMENU)
  MSG2STR(WM_STYLECHANGING)
  MSG2STR(WM_STYLECHANGED)
  MSG2STR(WM_DISPLAYCHANGE)
  MSG2STR(WM_GETICON)
  MSG2STR(WM_SETICON)
  MSG2STR(WM_NCCREATE)
  MSG2STR(WM_NCDESTROY)
  MSG2STR(WM_NCCALCSIZE)
  MSG2STR(WM_NCHITTEST)
  MSG2STR(WM_NCPAINT)
  MSG2STR(WM_NCACTIVATE)
  MSG2STR(WM_GETDLGCODE)
  MSG2STR(WM_SYNCPAINT)
  MSG2STR(WM_NCMOUSEMOVE)
  MSG2STR(WM_NCLBUTTONDOWN)
  MSG2STR(WM_NCLBUTTONUP)
  MSG2STR(WM_NCLBUTTONDBLCLK)
  MSG2STR(WM_NCRBUTTONDOWN)
  MSG2STR(WM_NCRBUTTONUP)
  MSG2STR(WM_NCRBUTTONDBLCLK)
  MSG2STR(WM_NCMBUTTONDOWN)
  MSG2STR(WM_NCMBUTTONUP)
  MSG2STR(WM_NCMBUTTONDBLCLK)
  MSG2STR(WM_NCXBUTTONDOWN)
  MSG2STR(WM_NCXBUTTONUP)
  MSG2STR(WM_NCXBUTTONDBLCLK)
  MSG2STR(EM_GETSEL)
  MSG2STR(EM_SETSEL)
  MSG2STR(EM_GETRECT)
  MSG2STR(EM_SETRECT)
  MSG2STR(EM_SETRECTNP)
  MSG2STR(EM_SCROLL)
  MSG2STR(EM_LINESCROLL)
  MSG2STR(EM_SCROLLCARET)
  MSG2STR(EM_GETMODIFY)
  MSG2STR(EM_SETMODIFY)
  MSG2STR(EM_GETLINECOUNT)
  MSG2STR(EM_LINEINDEX)
  MSG2STR(EM_SETHANDLE)
  MSG2STR(EM_GETHANDLE)
  MSG2STR(EM_GETTHUMB)
  MSG2STR(EM_LINELENGTH)
  MSG2STR(EM_REPLACESEL)
  //MSG2STR(EM_SETFONT)
  MSG2STR(EM_GETLINE)
  MSG2STR(EM_LIMITTEXT)
  //MSG2STR(EM_SETLIMITTEXT)
  MSG2STR(EM_CANUNDO)
  MSG2STR(EM_UNDO)
  MSG2STR(EM_FMTLINES)
  MSG2STR(EM_LINEFROMCHAR)
  //MSG2STR(EM_SETWORDBREAK)
  MSG2STR(EM_SETTABSTOPS)
  MSG2STR(EM_SETPASSWORDCHAR)
  MSG2STR(EM_EMPTYUNDOBUFFER)
  MSG2STR(EM_GETFIRSTVISIBLELINE)
  MSG2STR(EM_SETREADONLY)
  MSG2STR(EM_SETWORDBREAKPROC)
  MSG2STR(EM_GETWORDBREAKPROC)
  MSG2STR(EM_GETPASSWORDCHAR)
  MSG2STR(EM_SETMARGINS)
  MSG2STR(EM_GETMARGINS)
  MSG2STR(EM_GETLIMITTEXT)
  MSG2STR(EM_POSFROMCHAR)
  MSG2STR(EM_CHARFROMPOS)
  MSG2STR(EM_SETIMESTATUS)
  MSG2STR(EM_GETIMESTATUS)
  MSG2STR(SBM_SETPOS)
  MSG2STR(SBM_GETPOS)
  MSG2STR(SBM_SETRANGE)
  MSG2STR(SBM_GETRANGE)
  MSG2STR(SBM_ENABLE_ARROWS)
  MSG2STR(SBM_SETRANGEREDRAW)
  MSG2STR(SBM_SETSCROLLINFO)
  MSG2STR(SBM_GETSCROLLINFO)
  MSG2STR(SBM_GETSCROLLBARINFO)
  MSG2STR(BM_GETCHECK)
  MSG2STR(BM_SETCHECK)
  MSG2STR(BM_GETSTATE)
  MSG2STR(BM_SETSTATE)
  MSG2STR(BM_SETSTYLE)
  MSG2STR(BM_CLICK)
  MSG2STR(BM_GETIMAGE)
  MSG2STR(BM_SETIMAGE)
  MSG2STR(BM_SETDONTCLICK)
  MSG2STR(WM_INPUT)
  MSG2STR(WM_KEYDOWN)
  //MSG2STR(WM_KEYFIRST)
  MSG2STR(WM_KEYUP)
  MSG2STR(WM_CHAR)
  MSG2STR(WM_DEADCHAR)
  MSG2STR(WM_SYSKEYDOWN)
  MSG2STR(WM_SYSKEYUP)
  MSG2STR(WM_SYSCHAR)
  MSG2STR(WM_SYSDEADCHAR)
  //MSG2STR(WM_UNICHAR / WM_KEYLAST)
  //MSG2STR(WM_WNT_CONVERTREQUESTEX)
  //MSG2STR(WM_CONVERTREQUEST)
  //MSG2STR(WM_CONVERTRESULT)
  //MSG2STR(WM_INTERIM)
  MSG2STR(WM_IME_STARTCOMPOSITION)
  MSG2STR(WM_IME_ENDCOMPOSITION)
  MSG2STR(WM_IME_COMPOSITION)
  //MSG2STR(WM_IME_KEYLAST)
  MSG2STR(WM_INITDIALOG)
  MSG2STR(WM_COMMAND)
  MSG2STR(WM_SYSCOMMAND)
  MSG2STR(WM_TIMER)
  MSG2STR(WM_HSCROLL)
  MSG2STR(WM_VSCROLL)
  MSG2STR(WM_INITMENU)
  MSG2STR(WM_INITMENUPOPUP)
  //MSG2STR(WM_SYSTIMER)
  MSG2STR(WM_MENUSELECT)
  MSG2STR(WM_MENUCHAR)
  MSG2STR(WM_ENTERIDLE)
  MSG2STR(WM_MENURBUTTONUP)
  MSG2STR(WM_MENUDRAG)
  MSG2STR(WM_MENUGETOBJECT)
  MSG2STR(WM_UNINITMENUPOPUP)
  MSG2STR(WM_MENUCOMMAND)
  MSG2STR(WM_CHANGEUISTATE)
  MSG2STR(WM_UPDATEUISTATE)
  MSG2STR(WM_QUERYUISTATE)
  //MSG2STR(WM_LBTRACKPOINT)
  MSG2STR(WM_CTLCOLORMSGBOX)
  MSG2STR(WM_CTLCOLOREDIT)
  MSG2STR(WM_CTLCOLORLISTBOX)
  MSG2STR(WM_CTLCOLORBTN)
  MSG2STR(WM_CTLCOLORDLG)
  MSG2STR(WM_CTLCOLORSCROLLBAR)
  MSG2STR(WM_CTLCOLORSTATIC)
  MSG2STR(CB_GETEDITSEL)
  MSG2STR(CB_LIMITTEXT)
  MSG2STR(CB_SETEDITSEL)
  MSG2STR(CB_ADDSTRING)
  MSG2STR(CB_DELETESTRING)
  MSG2STR(CB_DIR)
  MSG2STR(CB_GETCOUNT)
  MSG2STR(CB_GETCURSEL)
  MSG2STR(CB_GETLBTEXT)
  MSG2STR(CB_GETLBTEXTLEN)
  MSG2STR(CB_INSERTSTRING)
  MSG2STR(CB_RESETCONTENT)
  MSG2STR(CB_FINDSTRING)
  MSG2STR(CB_SELECTSTRING)
  MSG2STR(CB_SETCURSEL)
  MSG2STR(CB_SHOWDROPDOWN)
  MSG2STR(CB_GETITEMDATA)
  MSG2STR(CB_SETITEMDATA)
  MSG2STR(CB_GETDROPPEDCONTROLRECT)
  MSG2STR(CB_SETITEMHEIGHT)
  MSG2STR(CB_GETITEMHEIGHT)
  MSG2STR(CB_SETEXTENDEDUI)
  MSG2STR(CB_GETEXTENDEDUI)
  MSG2STR(CB_GETDROPPEDSTATE)
  MSG2STR(CB_FINDSTRINGEXACT)
  MSG2STR(CB_SETLOCALE)
  MSG2STR(CB_GETLOCALE)
  MSG2STR(CB_GETTOPINDEX)
  MSG2STR(CB_SETTOPINDEX)
  MSG2STR(CB_GETHORIZONTALEXTENT)
  MSG2STR(CB_SETHORIZONTALEXTENT)
  MSG2STR(CB_GETDROPPEDWIDTH)
  MSG2STR(CB_SETDROPPEDWIDTH)
  MSG2STR(CB_INITSTORAGE)
  //MSG2STR(CB_MULTIPLEADDSTRING)
  MSG2STR(CB_GETCOMBOBOXINFO)
  MSG2STR(CB_MSGMAX)
  MSG2STR(WM_MOUSEFIRST)
  //MSG2STR(WM_MOUSEMOVE)
  MSG2STR(WM_LBUTTONDOWN)
  MSG2STR(WM_LBUTTONUP)
  MSG2STR(WM_LBUTTONDBLCLK)
  MSG2STR(WM_RBUTTONDOWN)
  MSG2STR(WM_RBUTTONUP)
  MSG2STR(WM_RBUTTONDBLCLK)
  MSG2STR(WM_MBUTTONDOWN)
  MSG2STR(WM_MBUTTONUP)
  MSG2STR(WM_MBUTTONDBLCLK)
  MSG2STR(WM_MOUSELAST)
  MSG2STR(WM_MOUSEWHEEL)
  MSG2STR(WM_XBUTTONDOWN)
  MSG2STR(WM_XBUTTONUP)
  MSG2STR(WM_XBUTTONDBLCLK)
  //MSG2STR(WM_MOUSEHWHEEL)
  MSG2STR(WM_PARENTNOTIFY)
  MSG2STR(WM_ENTERMENULOOP)
  MSG2STR(WM_EXITMENULOOP)
  MSG2STR(WM_NEXTMENU)
  MSG2STR(WM_SIZING)
  MSG2STR(WM_CAPTURECHANGED)
  MSG2STR(WM_MOVING)
  MSG2STR(WM_POWERBROADCAST)
  MSG2STR(WM_DEVICECHANGE)
  MSG2STR(WM_MDICREATE)
  MSG2STR(WM_MDIDESTROY)
  MSG2STR(WM_MDIACTIVATE)
  MSG2STR(WM_MDIRESTORE)
  MSG2STR(WM_MDINEXT)
  MSG2STR(WM_MDIMAXIMIZE)
  MSG2STR(WM_MDITILE)
  MSG2STR(WM_MDICASCADE)
  MSG2STR(WM_MDIICONARRANGE)
  MSG2STR(WM_MDIGETACTIVE)
  MSG2STR(WM_MDISETMENU)
  MSG2STR(WM_ENTERSIZEMOVE)
  MSG2STR(WM_EXITSIZEMOVE)
  MSG2STR(WM_DROPFILES)
  MSG2STR(WM_MDIREFRESHMENU)
  //MSG2STR(WM_IME_REPORT)
  MSG2STR(WM_IME_SETCONTEXT)
  MSG2STR(WM_IME_NOTIFY)
  MSG2STR(WM_IME_CONTROL)
  MSG2STR(WM_IME_COMPOSITIONFULL)
  MSG2STR(WM_IME_SELECT)
  MSG2STR(WM_IME_CHAR)
  MSG2STR(WM_IME_REQUEST)
  //MSG2STR(WM_IMEKEYDOWN)
  MSG2STR(WM_IME_KEYDOWN)
  //MSG2STR(WM_IMEKEYUP)
  MSG2STR(WM_IME_KEYUP)
  MSG2STR(WM_NCMOUSEHOVER)
  MSG2STR(WM_MOUSEHOVER)
  MSG2STR(WM_NCMOUSELEAVE)
  MSG2STR(WM_MOUSELEAVE)
  //MSG2STR(WM_TABLET_DEFBASE)
  MSG2STR(WM_TABLET_FIRST)
  //MSG2STR(WM_TABLET_ADDED)
  //MSG2STR(WM_TABLET_DELETED)
  //MSG2STR(WM_TABLET_FLICK)
  //MSG2STR(WM_TABLET_QUERYSYSTEMGESTURESTATUS)
  MSG2STR(WM_TABLET_LAST)
  MSG2STR(WM_CUT)
  MSG2STR(WM_COPY)
  MSG2STR(WM_PASTE)
  MSG2STR(WM_CLEAR)
  MSG2STR(WM_UNDO)
  MSG2STR(WM_RENDERFORMAT)
  MSG2STR(WM_RENDERALLFORMATS)
  MSG2STR(WM_DESTROYCLIPBOARD)
  MSG2STR(WM_DRAWCLIPBOARD)
  MSG2STR(WM_PAINTCLIPBOARD)
  MSG2STR(WM_VSCROLLCLIPBOARD)
  MSG2STR(WM_SIZECLIPBOARD)
  MSG2STR(WM_ASKCBFORMATNAME)
  MSG2STR(WM_CHANGECBCHAIN)
  MSG2STR(WM_HSCROLLCLIPBOARD)
  MSG2STR(WM_QUERYNEWPALETTE)
  MSG2STR(WM_PALETTEISCHANGING)
  MSG2STR(WM_PALETTECHANGED)
  MSG2STR(WM_HOTKEY)
  MSG2STR(WM_PRINT)
  MSG2STR(WM_PRINTCLIENT)
  MSG2STR(WM_APPCOMMAND)
  MSG2STR(WM_HANDHELDFIRST)
  MSG2STR(WM_HANDHELDLAST)
  MSG2STR(WM_AFXFIRST)
  MSG2STR(WM_AFXLAST)
  MSG2STR(WM_PENWINFIRST)
  //MSG2STR(WM_RCRESULT)
  //MSG2STR(WM_HOOKRCRESULT)
  //MSG2STR(WM_GLOBALRCCHANGE)
  //MSG2STR(WM_PENMISCINFO)
  //MSG2STR(WM_SKB)
  //MSG2STR(WM_HEDITCTL)
  //MSG2STR(WM_PENCTL)
  //MSG2STR(WM_PENMISC)
  //MSG2STR(WM_CTLINIT)
  //MSG2STR(WM_PENEVENT)
  MSG2STR(WM_PENWINLAST)
  //MSG2STR(DDM_SETFMT)
  MSG2STR(DM_GETDEFID)
  //MSG2STR(NIN_SELECT)
  //MSG2STR(TBM_GETPOS)
  //MSG2STR(WM_PSD_PAGESETUPDLG)
  //MSG2STR(WM_USER)
  MSG2STR(CBEM_INSERTITEMA)
  //MSG2STR(DDM_DRAW)
  //MSG2STR(DM_SETDEFID)
  //MSG2STR(HKM_SETHOTKEY)
  //MSG2STR(PBM_SETRANGE)
  //MSG2STR(RB_INSERTBANDA)
  //MSG2STR(SB_SETTEXTA)
  //MSG2STR(TB_ENABLEBUTTON)
  //MSG2STR(TBM_GETRANGEMIN)
  //MSG2STR(TTM_ACTIVATE)
  ////MSG2STR(WM_CHOOSEFONT_GETLOGFONT)
  ////MSG2STR(WM_PSD_FULLPAGERECT)
  //MSG2STR(CBEM_SETIMAGELIST)
  ////MSG2STR(DDM_CLOSE)
  //MSG2STR(DM_REPOSITION)
  //MSG2STR(HKM_GETHOTKEY)
  //MSG2STR(PBM_SETPOS)
  //MSG2STR(RB_DELETEBAND)
  //MSG2STR(SB_GETTEXTA)
  //MSG2STR(TB_CHECKBUTTON)
  //MSG2STR(TBM_GETRANGEMAX)
  ////MSG2STR(WM_PSD_MINMARGINRECT)
  //MSG2STR(CBEM_GETIMAGELIST)
  ////MSG2STR(DDM_BEGIN)
  //MSG2STR(HKM_SETRULES)
  //MSG2STR(PBM_DELTAPOS)
  //MSG2STR(RB_GETBARINFO)
  //MSG2STR(SB_GETTEXTLENGTHA)
  //MSG2STR(TBM_GETTIC)
  //MSG2STR(TB_PRESSBUTTON)
  //MSG2STR(TTM_SETDELAYTIME)
  ////MSG2STR(WM_PSD_MARGINRECT)
  //MSG2STR(CBEM_GETITEMA)
  ////MSG2STR(DDM_END)
  //MSG2STR(PBM_SETSTEP)
  //MSG2STR(RB_SETBARINFO)
  //MSG2STR(SB_SETPARTS)
  //MSG2STR(TB_HIDEBUTTON)
  //MSG2STR(TBM_SETTIC)
  //MSG2STR(TTM_ADDTOOLA)
  ////MSG2STR(WM_PSD_GREEKTEXTRECT)
  //MSG2STR(CBEM_SETITEMA)
  //MSG2STR(PBM_STEPIT)
  //MSG2STR(TB_INDETERMINATE)
  //MSG2STR(TBM_SETPOS)
  //MSG2STR(TTM_DELTOOLA)
  ////MSG2STR(WM_PSD_ENVSTAMPRECT)
  //MSG2STR(CBEM_GETCOMBOCONTROL)
  //MSG2STR(PBM_SETRANGE32)
  //MSG2STR(RB_SETBANDINFOA)
  //MSG2STR(SB_GETPARTS)
  //MSG2STR(TB_MARKBUTTON)
  //MSG2STR(TBM_SETRANGE)
  //MSG2STR(TTM_NEWTOOLRECTA)
  ////MSG2STR(WM_PSD_YAFULLPAGERECT)
  //MSG2STR(CBEM_GETEDITCONTROL)
  //MSG2STR(PBM_GETRANGE)
  //MSG2STR(RB_SETPARENT)
  //MSG2STR(SB_GETBORDERS)
  //MSG2STR(TBM_SETRANGEMIN)
  //MSG2STR(TTM_RELAYEVENT)
  //MSG2STR(CBEM_SETEXSTYLE)
  //MSG2STR(PBM_GETPOS)
  //MSG2STR(RB_HITTEST)
  //MSG2STR(SB_SETMINHEIGHT)
  //MSG2STR(TBM_SETRANGEMAX)
  //MSG2STR(TTM_GETTOOLINFOA)
  //MSG2STR(CBEM_GETEXSTYLE)
  //MSG2STR(CBEM_GETEXTENDEDSTYLE)
  //MSG2STR(PBM_SETBARCOLOR)
  //MSG2STR(RB_GETRECT)
  //MSG2STR(SB_SIMPLE)
  //MSG2STR(TB_ISBUTTONENABLED)
  //MSG2STR(TBM_CLEARTICS)
  //MSG2STR(TTM_SETTOOLINFOA)
  //MSG2STR(CBEM_HASEDITCHANGED)
  //MSG2STR(RB_INSERTBANDW)
  //MSG2STR(SB_GETRECT)
  //MSG2STR(TB_ISBUTTONCHECKED)
  //MSG2STR(TBM_SETSEL)
  //MSG2STR(TTM_HITTESTA)
  ////MSG2STR(WIZ_QUERYNUMPAGES)
  //MSG2STR(CBEM_INSERTITEMW)
  //MSG2STR(RB_SETBANDINFOW)
  //MSG2STR(SB_SETTEXTW)
  //MSG2STR(TB_ISBUTTONPRESSED)
  //MSG2STR(TBM_SETSELSTART)
  //MSG2STR(TTM_GETTEXTA)
  ////MSG2STR(WIZ_NEXT)
  //MSG2STR(CBEM_SETITEMW)
  //MSG2STR(RB_GETBANDCOUNT)
  //MSG2STR(SB_GETTEXTLENGTHW)
  //MSG2STR(TB_ISBUTTONHIDDEN)
  //MSG2STR(TBM_SETSELEND)
  //MSG2STR(TTM_UPDATETIPTEXTA)
  ////MSG2STR(WIZ_PREV)
  //MSG2STR(CBEM_GETITEMW)
  //MSG2STR(RB_GETROWCOUNT)
  //MSG2STR(SB_GETTEXTW)
  //MSG2STR(TB_ISBUTTONINDETERMINATE)
  //MSG2STR(TTM_GETTOOLCOUNT)
  //MSG2STR(CBEM_SETEXTENDEDSTYLE)
  //MSG2STR(RB_GETROWHEIGHT)
  //MSG2STR(SB_ISSIMPLE)
  //MSG2STR(TB_ISBUTTONHIGHLIGHTED)
  //MSG2STR(TBM_GETPTICS)
  //MSG2STR(TTM_ENUMTOOLSA)
  //MSG2STR(SB_SETICON)
  //MSG2STR(TBM_GETTICPOS)
  //MSG2STR(TTM_GETCURRENTTOOLA)
  //MSG2STR(RB_IDTOINDEX)
  //MSG2STR(SB_SETTIPTEXTA)
  //MSG2STR(TBM_GETNUMTICS)
  //MSG2STR(TTM_WINDOWFROMPOINT)
  //MSG2STR(RB_GETTOOLTIPS)
  //MSG2STR(SB_SETTIPTEXTW)
  //MSG2STR(TBM_GETSELSTART)
  //MSG2STR(TB_SETSTATE)
  //MSG2STR(TTM_TRACKACTIVATE)
  //MSG2STR(RB_SETTOOLTIPS)
  //MSG2STR(SB_GETTIPTEXTA)
  MSG2STR(TB_GETSTATE)
  //MSG2STR(TBM_GETSELEND)
  //MSG2STR(TTM_TRACKPOSITION)
  MSG2STR(RB_SETBKCOLOR)
  //MSG2STR(SB_GETTIPTEXTW)
  //MSG2STR(TB_ADDBITMAP)
  //MSG2STR(TBM_CLEARSEL)
  //MSG2STR(TTM_SETTIPBKCOLOR)
  MSG2STR(RB_GETBKCOLOR)
  //MSG2STR(SB_GETICON)
  //MSG2STR(TB_ADDBUTTONSA)
  //MSG2STR(TBM_SETTICFREQ)
  //MSG2STR(TTM_SETTIPTEXTCOLOR)
  MSG2STR(RB_SETTEXTCOLOR)
  //MSG2STR(TB_INSERTBUTTONA)
  //MSG2STR(TBM_SETPAGESIZE)
  //MSG2STR(TTM_GETDELAYTIME)
  MSG2STR(RB_GETTEXTCOLOR)
  //MSG2STR(TB_DELETEBUTTON)
  //MSG2STR(TBM_GETPAGESIZE)
  //MSG2STR(TTM_GETTIPBKCOLOR)
  MSG2STR(RB_SIZETORECT)
  //MSG2STR(TB_GETBUTTON)
  //MSG2STR(TBM_SETLINESIZE)
  //MSG2STR(TTM_GETTIPTEXTCOLOR)
  MSG2STR(RB_BEGINDRAG)
  //MSG2STR(TB_BUTTONCOUNT)
  //MSG2STR(TBM_GETLINESIZE)
  //MSG2STR(TTM_SETMAXTIPWIDTH)
  MSG2STR(RB_ENDDRAG)
  //MSG2STR(TB_COMMANDTOINDEX)
  //MSG2STR(TBM_GETTHUMBRECT)
  //MSG2STR(TTM_GETMAXTIPWIDTH)
  MSG2STR(RB_DRAGMOVE)
  //MSG2STR(TBM_GETCHANNELRECT)
  //MSG2STR(TB_SAVERESTOREA)
  //MSG2STR(TTM_SETMARGIN)
  MSG2STR(RB_GETBARHEIGHT)
  //MSG2STR(TB_CUSTOMIZE)
  //MSG2STR(TBM_SETTHUMBLENGTH)
  //MSG2STR(TTM_GETMARGIN)
  MSG2STR(RB_GETBANDINFOW)
  //MSG2STR(TB_ADDSTRINGA)
  //MSG2STR(TBM_GETTHUMBLENGTH)
  //MSG2STR(TTM_POP)
  MSG2STR(RB_GETBANDINFOA)
  //MSG2STR(TB_GETITEMRECT)
  //MSG2STR(TBM_SETTOOLTIPS)
  //MSG2STR(TTM_UPDATE)
  MSG2STR(RB_MINIMIZEBAND)
  //MSG2STR(TB_BUTTONSTRUCTSIZE)
  //MSG2STR(TBM_GETTOOLTIPS)
  //MSG2STR(TTM_GETBUBBLESIZE)
  MSG2STR(RB_MAXIMIZEBAND)
  //MSG2STR(TBM_SETTIPSIDE)
  //MSG2STR(TB_SETBUTTONSIZE)
  //MSG2STR(TTM_ADJUSTRECT)
  MSG2STR(TBM_SETBUDDY)
  //MSG2STR(TB_SETBITMAPSIZE)
  //MSG2STR(TTM_SETTITLEA)
  //MSG2STR(MSG_FTS_JUMP_VA)
  MSG2STR(TB_AUTOSIZE)
  //MSG2STR(TBM_GETBUDDY)
  //MSG2STR(TTM_SETTITLEW)
  MSG2STR(RB_GETBANDBORDERS)
  //MSG2STR(MSG_FTS_JUMP_QWORD)
  MSG2STR(RB_SHOWBAND)
  //MSG2STR(TB_GETTOOLTIPS)
  //MSG2STR(MSG_REINDEX_REQUEST)
  MSG2STR(TB_SETTOOLTIPS)
  //MSG2STR(MSG_FTS_WHERE_IS_IT)
  MSG2STR(RB_SETPALETTE)
  //MSG2STR(TB_SETPARENT)
  MSG2STR(RB_GETPALETTE)
  MSG2STR(RB_MOVEBAND)
  //MSG2STR(TB_SETROWS)
  MSG2STR(TB_GETROWS)
  MSG2STR(TB_GETBITMAPFLAGS)
  MSG2STR(TB_SETCMDID)
  MSG2STR(RB_PUSHCHEVRON)
  //MSG2STR(TB_CHANGEBITMAP)
  MSG2STR(TB_GETBITMAP)
  //MSG2STR(MSG_GET_DEFFONT)
  MSG2STR(TB_GETBUTTONTEXTA)
  MSG2STR(TB_REPLACEBITMAP)
  MSG2STR(TB_SETINDENT)
  MSG2STR(TB_SETIMAGELIST)
  MSG2STR(TB_GETIMAGELIST)
  MSG2STR(TB_LOADIMAGES)
  //MSG2STR(EM_CANPASTE)
  //MSG2STR(TTM_ADDTOOLW)
  //MSG2STR(EM_DISPLAYBAND)
  MSG2STR(TB_GETRECT)
  //MSG2STR(TTM_DELTOOLW)
  //MSG2STR(EM_EXGETSEL)
  MSG2STR(TB_SETHOTIMAGELIST)
  //MSG2STR(TTM_NEWTOOLRECTW)
  //MSG2STR(EM_EXLIMITTEXT)
  MSG2STR(TB_GETHOTIMAGELIST)
  //MSG2STR(TTM_GETTOOLINFOW)
  //MSG2STR(EM_EXLINEFROMCHAR)
  MSG2STR(TB_SETDISABLEDIMAGELIST)
  //MSG2STR(TTM_SETTOOLINFOW)
  //MSG2STR(EM_EXSETSEL)
  MSG2STR(TB_GETDISABLEDIMAGELIST)
  //MSG2STR(TTM_HITTESTW)
  //MSG2STR(EM_FINDTEXT)
  MSG2STR(TB_SETSTYLE)
  //MSG2STR(TTM_GETTEXTW)
  //MSG2STR(EM_FORMATRANGE)
  MSG2STR(TB_GETSTYLE)
  //MSG2STR(TTM_UPDATETIPTEXTW)
  //MSG2STR(EM_GETCHARFORMAT)
  MSG2STR(TB_GETBUTTONSIZE)
  //MSG2STR(TTM_ENUMTOOLSW)
  //MSG2STR(EM_GETEVENTMASK)
  MSG2STR(TB_SETBUTTONWIDTH)
  //MSG2STR(TTM_GETCURRENTTOOLW)
  //MSG2STR(EM_GETOLEINTERFACE)
  MSG2STR(TB_SETMAXTEXTROWS)
  //MSG2STR(EM_GETPARAFORMAT)
  MSG2STR(TB_GETTEXTROWS)
  //MSG2STR(EM_GETSELTEXT)
  MSG2STR(TB_GETOBJECT)
  //MSG2STR(EM_HIDESELECTION)
  MSG2STR(TB_GETBUTTONINFOW)
  //MSG2STR(EM_PASTESPECIAL)
  MSG2STR(TB_SETBUTTONINFOW)
  //MSG2STR(EM_REQUESTRESIZE)
  MSG2STR(TB_GETBUTTONINFOA)
  //MSG2STR(EM_SELECTIONTYPE)
  MSG2STR(TB_SETBUTTONINFOA)
  //MSG2STR(EM_SETBKGNDCOLOR)
  MSG2STR(TB_INSERTBUTTONW)
  //MSG2STR(EM_SETCHARFORMAT)
  MSG2STR(TB_ADDBUTTONSW)
  //MSG2STR(EM_SETEVENTMASK)
  MSG2STR(TB_HITTEST)
  //MSG2STR(EM_SETOLECALLBACK)
  MSG2STR(TB_SETDRAWTEXTFLAGS)
  //MSG2STR(EM_SETPARAFORMAT)
  MSG2STR(TB_GETHOTITEM)
  //MSG2STR(EM_SETTARGETDEVICE)
  MSG2STR(TB_SETHOTITEM)
  //MSG2STR(EM_STREAMIN)
  MSG2STR(TB_SETANCHORHIGHLIGHT)
  //MSG2STR(EM_STREAMOUT)
  MSG2STR(TB_GETANCHORHIGHLIGHT)
  //MSG2STR(EM_GETTEXTRANGE)
  MSG2STR(TB_GETBUTTONTEXTW)
  //MSG2STR(EM_FINDWORDBREAK)
  MSG2STR(TB_SAVERESTOREW)
  //MSG2STR(EM_SETOPTIONS)
  MSG2STR(TB_ADDSTRINGW)
  //MSG2STR(EM_GETOPTIONS)
  MSG2STR(TB_MAPACCELERATORA)
  //MSG2STR(EM_FINDTEXTEX)
  MSG2STR(TB_GETINSERTMARK)
  //MSG2STR(EM_GETWORDBREAKPROCEX)
  MSG2STR(TB_SETINSERTMARK)
  //MSG2STR(EM_SETWORDBREAKPROCEX)
  MSG2STR(TB_INSERTMARKHITTEST)
  //MSG2STR(EM_SETUNDOLIMIT)
  MSG2STR(TB_MOVEBUTTON)
  MSG2STR(TB_GETMAXSIZE)
  //MSG2STR(EM_REDO)
  MSG2STR(TB_SETEXTENDEDSTYLE)
  //MSG2STR(EM_CANREDO)
  MSG2STR(TB_GETEXTENDEDSTYLE)
  //MSG2STR(EM_GETUNDONAME)
  MSG2STR(TB_GETPADDING)
  //MSG2STR(EM_GETREDONAME)
  MSG2STR(TB_SETPADDING)
  //MSG2STR(EM_STOPGROUPTYPING)
  MSG2STR(TB_SETINSERTMARKCOLOR)
  //MSG2STR(EM_SETTEXTMODE)
  MSG2STR(TB_GETINSERTMARKCOLOR)
  //MSG2STR(EM_GETTEXTMODE)
  MSG2STR(TB_MAPACCELERATORW)
  //MSG2STR(EM_AUTOURLDETECT)
  MSG2STR(TB_GETSTRINGW)
  //MSG2STR(EM_GETAUTOURLDETECT)
  MSG2STR(TB_GETSTRINGA)
  //MSG2STR(EM_SETPALETTE)
  //MSG2STR(EM_GETTEXTEX)
  //MSG2STR(EM_GETTEXTLENGTHEX)
  //MSG2STR(EM_SHOWSCROLLBAR)
  //MSG2STR(EM_SETTEXTEX)
  //MSG2STR(TAPI_REPLY)
  MSG2STR(ACM_OPENA)
  //MSG2STR(BFFM_SETSTATUSTEXTA)
  //MSG2STR(CDM_FIRST)
  //MSG2STR(CDM_GETSPEC)
  //MSG2STR(EM_SETPUNCTUATION)
  //MSG2STR(IPM_CLEARADDRESS)
  //MSG2STR(WM_CAP_UNICODE_START)
  MSG2STR(ACM_PLAY)
  //MSG2STR(BFFM_ENABLEOK)
  //MSG2STR(CDM_GETFILEPATH)
  //MSG2STR(EM_GETPUNCTUATION)
  //MSG2STR(IPM_SETADDRESS)
  //MSG2STR(PSM_SETCURSEL)
  //MSG2STR(UDM_SETRANGE)
  //MSG2STR(WM_CHOOSEFONT_SETLOGFONT)
  MSG2STR(ACM_STOP)
  //MSG2STR(BFFM_SETSELECTIONA)
  //MSG2STR(CDM_GETFOLDERPATH)
  //MSG2STR(EM_SETWORDWRAPMODE)
  //MSG2STR(IPM_GETADDRESS)
  //MSG2STR(PSM_REMOVEPAGE)
  //MSG2STR(UDM_GETRANGE)
  //MSG2STR(WM_CAP_SET_CALLBACK_ERRORW)
  //MSG2STR(WM_CHOOSEFONT_SETFLAGS)
  MSG2STR(ACM_OPENW)
  //MSG2STR(BFFM_SETSELECTIONW)
  //MSG2STR(CDM_GETFOLDERIDLIST)
  //MSG2STR(EM_GETWORDWRAPMODE)
  //MSG2STR(IPM_SETRANGE)
  //MSG2STR(PSM_ADDPAGE)
  //MSG2STR(UDM_SETPOS)
  //MSG2STR(WM_CAP_SET_CALLBACK_STATUSW)
  //MSG2STR(BFFM_SETSTATUSTEXTW)
  //MSG2STR(CDM_SETCONTROLTEXT)
  //MSG2STR(EM_SETIMECOLOR)
  MSG2STR(IPM_SETFOCUS)
  //MSG2STR(PSM_CHANGED)
  //MSG2STR(UDM_GETPOS)
  //MSG2STR(CDM_HIDECONTROL)
  //MSG2STR(EM_GETIMECOLOR)
  MSG2STR(IPM_ISBLANK)
  //MSG2STR(PSM_RESTARTWINDOWS)
  //MSG2STR(UDM_SETBUDDY)
  //MSG2STR(CDM_SETDEFEXT)
  //MSG2STR(EM_SETIMEOPTIONS)
  MSG2STR(PSM_REBOOTSYSTEM)
  //MSG2STR(UDM_GETBUDDY)
  //MSG2STR(EM_GETIMEOPTIONS)
  MSG2STR(PSM_CANCELTOCLOSE)
  //MSG2STR(UDM_SETACCEL)
  //MSG2STR(EM_CONVPOSITION)
  //MSG2STR(EM_CONVPOSITION)
  MSG2STR(PSM_QUERYSIBLINGS)
  //MSG2STR(UDM_GETACCEL)
  //MSG2STR(MCIWNDM_GETZOOM)
  MSG2STR(PSM_UNCHANGED)
  //MSG2STR(UDM_SETBASE)
  MSG2STR(PSM_APPLY)
  //MSG2STR(UDM_GETBASE)
  MSG2STR(PSM_SETTITLEA)
  //MSG2STR(UDM_SETRANGE32)
  MSG2STR(PSM_SETWIZBUTTONS)
  //MSG2STR(UDM_GETRANGE32)
  //MSG2STR(WM_CAP_DRIVER_GET_NAMEW)
  MSG2STR(PSM_PRESSBUTTON)
  //MSG2STR(UDM_SETPOS32)
  //MSG2STR(WM_CAP_DRIVER_GET_VERSIONW)
  MSG2STR(PSM_SETCURSELID)
  //MSG2STR(UDM_GETPOS32)
  MSG2STR(PSM_SETFINISHTEXTA)
  MSG2STR(PSM_GETTABCONTROL)
  MSG2STR(PSM_ISDIALOGMESSAGE)
  //MSG2STR(MCIWNDM_REALIZE)
  MSG2STR(PSM_GETCURRENTPAGEHWND)
  //MSG2STR(MCIWNDM_SETTIMEFORMATA)
  MSG2STR(PSM_INSERTPAGE)
  //MSG2STR(EM_SETLANGOPTIONS)
  //MSG2STR(MCIWNDM_GETTIMEFORMATA)
  MSG2STR(PSM_SETTITLEW)
  //MSG2STR(WM_CAP_FILE_SET_CAPTURE_FILEW)
  //MSG2STR(EM_GETLANGOPTIONS)
  //MSG2STR(MCIWNDM_VALIDATEMEDIA)
  MSG2STR(PSM_SETFINISHTEXTW)
  //MSG2STR(WM_CAP_FILE_GET_CAPTURE_FILEW)
  //MSG2STR(EM_GETIMECOMPMODE)
  //MSG2STR(EM_FINDTEXTW)
  //MSG2STR(MCIWNDM_PLAYTO)
  //MSG2STR(WM_CAP_FILE_SAVEASW)
  //MSG2STR(EM_FINDTEXTEXW)
  //MSG2STR(MCIWNDM_GETFILENAMEA)
  //MSG2STR(EM_RECONVERSION)
  //MSG2STR(MCIWNDM_GETDEVICEA)
  MSG2STR(PSM_SETHEADERTITLEA)
  //MSG2STR(WM_CAP_FILE_SAVEDIBW)
  //MSG2STR(EM_SETIMEMODEBIAS)
  //MSG2STR(MCIWNDM_GETPALETTE)
  MSG2STR(PSM_SETHEADERTITLEW)
  //MSG2STR(EM_GETIMEMODEBIAS)
  //MSG2STR(MCIWNDM_SETPALETTE)
  MSG2STR(PSM_SETHEADERSUBTITLEA)
  //MSG2STR(MCIWNDM_GETERRORA)
  MSG2STR(PSM_SETHEADERSUBTITLEW)
  MSG2STR(PSM_HWNDTOINDEX)
  MSG2STR(PSM_INDEXTOHWND)
  //MSG2STR(MCIWNDM_SETINACTIVETIMER)
  MSG2STR(PSM_PAGETOINDEX)
  MSG2STR(PSM_INDEXTOPAGE)
  MSG2STR(DL_BEGINDRAG)
  //MSG2STR(MCIWNDM_GETINACTIVETIMER)
  //MSG2STR(PSM_IDTOINDEX)
  MSG2STR(DL_DRAGGING)
  //MSG2STR(PSM_INDEXTOID)
  MSG2STR(DL_DROPPED)
  //MSG2STR(PSM_GETRESULT)
  MSG2STR(DL_CANCELDRAG)
  //MSG2STR(PSM_RECALCPAGESIZES)
  //MSG2STR(MCIWNDM_GET_SOURCE)
  //MSG2STR(MCIWNDM_PUT_SOURCE)
  //MSG2STR(MCIWNDM_GET_DEST)
  //MSG2STR(MCIWNDM_PUT_DEST)
  //MSG2STR(MCIWNDM_CAN_PLAY)
  //MSG2STR(MCIWNDM_CAN_WINDOW)
  //MSG2STR(MCIWNDM_CAN_RECORD)
  //MSG2STR(MCIWNDM_CAN_SAVE)
  //MSG2STR(MCIWNDM_CAN_EJECT)
  //MSG2STR(MCIWNDM_CAN_CONFIG)
  //MSG2STR(IE_GETINK)
  //MSG2STR(IE_MSGFIRST)
  //MSG2STR(MCIWNDM_PALETTEKICK)
  //MSG2STR(IE_SETINK)
  //MSG2STR(IE_GETPENTIP)
  //MSG2STR(IE_SETPENTIP)
  //MSG2STR(IE_GETERASERTIP)
  //MSG2STR(IE_SETERASERTIP)
  //MSG2STR(IE_GETBKGND)
  //MSG2STR(IE_SETBKGND)
  //MSG2STR(IE_GETGRIDORIGIN)
  //MSG2STR(IE_SETGRIDORIGIN)
  //MSG2STR(IE_GETGRIDPEN)
  //MSG2STR(IE_SETGRIDPEN)
  //MSG2STR(IE_GETGRIDSIZE)
  //MSG2STR(IE_SETGRIDSIZE)
  //MSG2STR(IE_GETMODE)
  //MSG2STR(IE_SETMODE)
  //MSG2STR(IE_GETINKRECT)
  //MSG2STR(WM_CAP_SET_MCI_DEVICEW)
  //MSG2STR(WM_CAP_GET_MCI_DEVICEW)
  //MSG2STR(WM_CAP_PAL_OPENW)
  //MSG2STR(WM_CAP_PAL_SAVEW)
  //MSG2STR(IE_GETAPPDATA)
  //MSG2STR(IE_SETAPPDATA)
  //MSG2STR(IE_GETDRAWOPTS)
  //MSG2STR(IE_SETDRAWOPTS)
  //MSG2STR(IE_GETFORMAT)
  //MSG2STR(IE_SETFORMAT)
  //MSG2STR(IE_GETINKINPUT)
  //MSG2STR(IE_SETINKINPUT)
  //MSG2STR(IE_GETNOTIFY)
  //MSG2STR(IE_SETNOTIFY)
  //MSG2STR(IE_GETRECOG)
  //MSG2STR(IE_SETRECOG)
  //MSG2STR(IE_GETSECURITY)
  //MSG2STR(IE_SETSECURITY)
  //MSG2STR(IE_GETSEL)
  //MSG2STR(IE_SETSEL)
  //MSG2STR(CDM_LAST)
  //MSG2STR(EM_SETBIDIOPTIONS)
  //MSG2STR(IE_DOCOMMAND)
  //MSG2STR(MCIWNDM_NOTIFYMODE)
  //MSG2STR(EM_GETBIDIOPTIONS)
  //MSG2STR(IE_GETCOMMAND)
  //MSG2STR(EM_SETTYPOGRAPHYOPTIONS)
  //MSG2STR(IE_GETCOUNT)
  //MSG2STR(EM_GETTYPOGRAPHYOPTIONS)
  //MSG2STR(IE_GETGESTURE)
  //MSG2STR(MCIWNDM_NOTIFYMEDIA)
  //MSG2STR(EM_SETEDITSTYLE)
  //MSG2STR(IE_GETMENU)
  //MSG2STR(EM_GETEDITSTYLE)
  //MSG2STR(IE_GETPAINTDC)
  //MSG2STR(MCIWNDM_NOTIFYERROR)
  //MSG2STR(IE_GETPDEVENT)
  //MSG2STR(IE_GETSELCOUNT)
  //MSG2STR(IE_GETSELITEMS)
  //MSG2STR(IE_GETSTYLE)
  //MSG2STR(MCIWNDM_SETTIMEFORMATW)
  //MSG2STR(EM_OUTLINE)
  //MSG2STR(MCIWNDM_GETTIMEFORMATW)
  //MSG2STR(EM_GETSCROLLPOS)
  //MSG2STR(EM_SETSCROLLPOS)
  //MSG2STR(EM_SETSCROLLPOS)
  //MSG2STR(EM_SETFONTSIZE)
  MSG2STR(EM_GETZOOM)
  //MSG2STR(MCIWNDM_GETFILENAMEW)
  MSG2STR(EM_SETZOOM)
  //MSG2STR(MCIWNDM_GETDEVICEW)
  //MSG2STR(EM_GETVIEWKIND)
  //MSG2STR(EM_SETVIEWKIND)
  //MSG2STR(EM_GETPAGE)
  //MSG2STR(MCIWNDM_GETERRORW)
  //MSG2STR(EM_SETPAGE)
  //MSG2STR(EM_GETHYPHENATEINFO)
  //MSG2STR(EM_SETHYPHENATEINFO)
  //MSG2STR(EM_GETPAGEROTATE)
  //MSG2STR(EM_SETPAGEROTATE)
  //MSG2STR(EM_GETCTFMODEBIAS)
  //MSG2STR(EM_SETCTFMODEBIAS)
  //MSG2STR(EM_GETCTFOPENSTATUS)
  //MSG2STR(EM_SETCTFOPENSTATUS)
  //MSG2STR(EM_GETIMECOMPTEXT)
  //MSG2STR(EM_ISIME)
  //MSG2STR(EM_GETIMEPROPERTY)
  //MSG2STR(EM_GETQUERYRTFOBJ)
  //MSG2STR(EM_SETQUERYRTFOBJ)
  //MSG2STR(FM_GETFOCUS)
  //MSG2STR(FM_GETDRIVEINFOA)
  //MSG2STR(FM_GETSELCOUNT)
  //MSG2STR(FM_GETSELCOUNTLFN)
  //MSG2STR(FM_GETFILESELA)
  //MSG2STR(FM_GETFILESELLFNA)
  //MSG2STR(FM_REFRESH_WINDOWS)
  //MSG2STR(FM_RELOAD_EXTENSIONS)
  //MSG2STR(FM_GETDRIVEINFOW)
  //MSG2STR(FM_GETFILESELW)
  //MSG2STR(FM_GETFILESELLFNW)
  //MSG2STR(WLX_WM_SAS)
  //MSG2STR(SM_GETSELCOUNT)
  //MSG2STR(UM_GETSELCOUNT)
  //MSG2STR(WM_CPL_LAUNCH)
  //MSG2STR(SM_GETSERVERSELA)
  //MSG2STR(UM_GETUSERSELA)
  //MSG2STR(WM_CPL_LAUNCHED)
  //MSG2STR(SM_GETSERVERSELW)
  //MSG2STR(UM_GETUSERSELW)
  //MSG2STR(SM_GETCURFOCUSA)
  //MSG2STR(UM_GETGROUPSELA)
  //MSG2STR(SM_GETCURFOCUSW)
  //MSG2STR(UM_GETGROUPSELW)
  //MSG2STR(SM_GETOPTIONS)
  //MSG2STR(UM_GETCURFOCUSA)
  //MSG2STR(UM_GETCURFOCUSW)
  //MSG2STR(UM_GETOPTIONS)
  //MSG2STR(UM_GETOPTIONS2)
  MSG2STR(LVM_FIRST)
  //MSG2STR(LVM_GETBKCOLOR)
  MSG2STR(LVM_SETBKCOLOR)
  MSG2STR(LVM_GETIMAGELIST)
  MSG2STR(LVM_SETIMAGELIST)
  MSG2STR(LVM_GETITEMCOUNT)
  MSG2STR(LVM_GETITEMA)
  MSG2STR(LVM_SETITEMA)
  MSG2STR(LVM_INSERTITEMA)
  MSG2STR(LVM_DELETEITEM)
  MSG2STR(LVM_DELETEALLITEMS)
  MSG2STR(LVM_GETCALLBACKMASK)
  MSG2STR(LVM_SETCALLBACKMASK)
  MSG2STR(LVM_GETNEXTITEM)
  MSG2STR(LVM_FINDITEMA)
  MSG2STR(LVM_GETITEMRECT)
  MSG2STR(LVM_SETITEMPOSITION)
  MSG2STR(LVM_GETITEMPOSITION)
  MSG2STR(LVM_GETSTRINGWIDTHA)
  MSG2STR(LVM_HITTEST)
  MSG2STR(LVM_ENSUREVISIBLE)
  MSG2STR(LVM_SCROLL)
  MSG2STR(LVM_REDRAWITEMS)
  MSG2STR(LVM_ARRANGE)
  MSG2STR(LVM_EDITLABELA)
  MSG2STR(LVM_GETEDITCONTROL)
  MSG2STR(LVM_GETCOLUMNA)
  MSG2STR(LVM_SETCOLUMNA)
  MSG2STR(LVM_INSERTCOLUMNA)
  MSG2STR(LVM_DELETECOLUMN)
  MSG2STR(LVM_GETCOLUMNWIDTH)
  MSG2STR(LVM_SETCOLUMNWIDTH)
  MSG2STR(LVM_GETHEADER)
  MSG2STR(LVM_CREATEDRAGIMAGE)
  MSG2STR(LVM_GETVIEWRECT)
  MSG2STR(LVM_GETTEXTCOLOR)
  MSG2STR(LVM_SETTEXTCOLOR)
  MSG2STR(LVM_GETTEXTBKCOLOR)
  MSG2STR(LVM_SETTEXTBKCOLOR)
  MSG2STR(LVM_GETTOPINDEX)
  MSG2STR(LVM_GETCOUNTPERPAGE)
  MSG2STR(LVM_GETORIGIN)
  MSG2STR(LVM_UPDATE)
  MSG2STR(LVM_SETITEMSTATE)
  MSG2STR(LVM_GETITEMSTATE)
  MSG2STR(LVM_GETITEMTEXTA)
  MSG2STR(LVM_SETITEMTEXTA)
  MSG2STR(LVM_SETITEMCOUNT)
  MSG2STR(LVM_SORTITEMS)
  MSG2STR(LVM_SETITEMPOSITION32)
  MSG2STR(LVM_GETSELECTEDCOUNT)
  MSG2STR(LVM_GETITEMSPACING)
  MSG2STR(LVM_GETISEARCHSTRINGA)
  MSG2STR(LVM_SETICONSPACING)
  MSG2STR(LVM_SETEXTENDEDLISTVIEWSTYLE)
  MSG2STR(LVM_GETEXTENDEDLISTVIEWSTYLE)
  MSG2STR(LVM_GETSUBITEMRECT)
  MSG2STR(LVM_SUBITEMHITTEST)
  MSG2STR(LVM_SETCOLUMNORDERARRAY)
  MSG2STR(LVM_GETCOLUMNORDERARRAY)
  MSG2STR(LVM_SETHOTITEM)
  MSG2STR(LVM_GETHOTITEM)
  MSG2STR(LVM_SETHOTCURSOR)
  MSG2STR(LVM_GETHOTCURSOR)
  MSG2STR(LVM_APPROXIMATEVIEWRECT)
  MSG2STR(LVM_SETWORKAREAS)
  MSG2STR(LVM_GETSELECTIONMARK)
  MSG2STR(LVM_SETSELECTIONMARK)
  MSG2STR(LVM_SETBKIMAGEA)
  MSG2STR(LVM_GETBKIMAGEA)
  MSG2STR(LVM_GETWORKAREAS)
  MSG2STR(LVM_SETHOVERTIME)
  MSG2STR(LVM_GETHOVERTIME)
  MSG2STR(LVM_GETNUMBEROFWORKAREAS)
  MSG2STR(LVM_SETTOOLTIPS)
  MSG2STR(LVM_GETITEMW)
  MSG2STR(LVM_SETITEMW)
  MSG2STR(LVM_INSERTITEMW)
  MSG2STR(LVM_GETTOOLTIPS)
  MSG2STR(LVM_FINDITEMW)
  MSG2STR(LVM_GETSTRINGWIDTHW)
  MSG2STR(LVM_GETCOLUMNW)
  MSG2STR(LVM_SETCOLUMNW)
  MSG2STR(LVM_INSERTCOLUMNW)
  MSG2STR(LVM_GETITEMTEXTW)
  MSG2STR(LVM_SETITEMTEXTW)
  MSG2STR(LVM_GETISEARCHSTRINGW)
  MSG2STR(LVM_EDITLABELW)
  MSG2STR(LVM_GETBKIMAGEW)
  MSG2STR(LVM_SETSELECTEDCOLUMN)
  //MSG2STR(LVM_SETTILEWIDTH)
  MSG2STR(LVM_SETVIEW)
  MSG2STR(LVM_GETVIEW)
  MSG2STR(LVM_INSERTGROUP)
  MSG2STR(LVM_SETGROUPINFO)
  MSG2STR(LVM_GETGROUPINFO)
  MSG2STR(LVM_REMOVEGROUP)
  MSG2STR(LVM_MOVEGROUP)
  MSG2STR(LVM_MOVEITEMTOGROUP)
  MSG2STR(LVM_SETGROUPMETRICS)
  MSG2STR(LVM_GETGROUPMETRICS)
  MSG2STR(LVM_ENABLEGROUPVIEW)
  MSG2STR(LVM_SORTGROUPS)
  MSG2STR(LVM_INSERTGROUPSORTED)
  MSG2STR(LVM_REMOVEALLGROUPS)
  MSG2STR(LVM_HASGROUP)
  MSG2STR(LVM_SETTILEVIEWINFO)
  MSG2STR(LVM_GETTILEVIEWINFO)
  MSG2STR(LVM_SETTILEINFO)
  MSG2STR(LVM_GETTILEINFO)
  MSG2STR(LVM_SETINSERTMARK)
  MSG2STR(LVM_GETINSERTMARK)
  MSG2STR(LVM_INSERTMARKHITTEST)
  MSG2STR(LVM_GETINSERTMARKRECT)
  MSG2STR(LVM_SETINSERTMARKCOLOR)
  MSG2STR(LVM_GETINSERTMARKCOLOR)
  MSG2STR(LVM_SETINFOTIP)
  MSG2STR(LVM_GETSELECTEDCOLUMN)
  MSG2STR(LVM_ISGROUPVIEWENABLED)
  MSG2STR(LVM_GETOUTLINECOLOR)
  MSG2STR(LVM_SETOUTLINECOLOR)
  MSG2STR(LVM_CANCELEDITLABEL)
  MSG2STR(LVM_MAPINDEXTOID)
  MSG2STR(LVM_MAPIDTOINDEX)
  MSG2STR(LVM_ISITEMVISIBLE)
  MSG2STR(LVM_GETEMPTYTEXT)
  MSG2STR(LVM_GETFOOTERRECT)
  MSG2STR(LVM_GETFOOTERINFO)
  MSG2STR(LVM_GETFOOTERITEMRECT)
  MSG2STR(LVM_GETFOOTERITEM)
  MSG2STR(LVM_GETITEMINDEXRECT)
  MSG2STR(LVM_SETITEMINDEXSTATE)
  MSG2STR(LVM_GETNEXTITEMINDEX)
  MSG2STR(CCM_FIRST)
  //MSG2STR(OCM__BASE)
  MSG2STR(CCM_SETBKCOLOR)
  MSG2STR(CCM_SETCOLORSCHEME)
  MSG2STR(CCM_GETCOLORSCHEME)
  MSG2STR(CCM_GETDROPTARGET)
  MSG2STR(CCM_SETUNICODEFORMAT)
  //MSG2STR(LVM_SETUNICODEFORMAT)
  MSG2STR(CCM_GETUNICODEFORMAT)
  //MSG2STR(LVM_GETUNICODEFORMAT)
  MSG2STR(CCM_SETVERSION)
  MSG2STR(CCM_GETVERSION)
  MSG2STR(CCM_SETNOTIFYWINDOW)
  MSG2STR(CCM_SETWINDOWTHEME)
  MSG2STR(CCM_DPISCALE)
  //MSG2STR(OCM_CTLCOLOR)
  //MSG2STR(OCM_DRAWITEM)
  //MSG2STR(OCM_MEASUREITEM)
  //MSG2STR(OCM_DELETEITEM)
  //MSG2STR(OCM_VKEYTOITEM)
  //MSG2STR(OCM_CHARTOITEM)
  //MSG2STR(OCM_COMPAREITEM)
  //MSG2STR(OCM_NOTIFY)
  //MSG2STR(OCM_COMMAND)
  //MSG2STR(OCM_HSCROLL)
  //MSG2STR(OCM_VSCROLL)
  //MSG2STR(OCM_CTLCOLORMSGBOX)
  //MSG2STR(OCM_CTLCOLOREDIT)
  //MSG2STR(OCM_CTLCOLORLISTBOX)
  //MSG2STR(OCM_CTLCOLORBTN)
  //MSG2STR(OCM_CTLCOLORDLG)
  //MSG2STR(OCM_CTLCOLORSCROLLBAR)
  //MSG2STR(OCM_CTLCOLORSTATIC)
  //MSG2STR(OCM_PARENTNOTIFY)
  //MSG2STR(OCM_PARENTNOTIFY)
  MSG2STR(WM_APP)
  //MSG2STR(WM_RASDIALEVENT)
  default:
    return _T("");
    }

#undef MSG2STR
}