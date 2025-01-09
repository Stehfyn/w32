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
    //RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
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
