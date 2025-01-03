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
#include <GL\glext.h>
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

/*=============================================================================
** 3.2 Types
**===========================================================================*/

/*=============================================================================
** 3.3 External global variables
**===========================================================================*/

/*=============================================================================
** 3.4 Static global variables
**===========================================================================*/
static HBITMAP hBitmap;
static HBITMAP hBitmapBg;
static HDC     g_hDC;
static HGLRC   g_hRC;
static GLuint  g_hTex = 0;
static BYTE    g_lpPixelBuf[256*256*4];
static INT     g_nWidth  = 0;
static INT     g_nHeight = 0;

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
BOOL
borderless_on_wmcreate(
  HWND           hWnd,
  LPCREATESTRUCT lpCreateStruct
);

CFORCEINLINE
VOID
borderless_on_wm_size(
  HWND hWnd,
  UINT state,
  int  cx,
  int  cy
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
borderless_on_wm_ncrbuttondown(
  HWND hwnd,
  BOOL fDoubleClick,
  int  x,
  int  y,
  UINT codeHitTest
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

CFORCEINLINE
VOID
borderless_on_wm_paint(
  HWND hWnd
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
    hBitmap = (HBITMAP)LoadImage(
      GetModuleHandle(NULL),
      L"res\\winpe.bmp",
      IMAGE_BITMAP,
      0,
      0,
      LR_LOADFROMFILE | LR_CREATEDIBSECTION
    );
    ASSERT_W32(hBitmap);
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
BOOL
borderless_on_wm_ncactivate(
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

CFORCEINLINE
BOOL
borderless_on_wmcreate(
  HWND           hWnd,
  LPCREATESTRUCT lpCreateStruct)
{
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(lpCreateStruct);

  //FORWARD_WM_CREATE(hWnd, lpCreateStruct, DefWindowProc);
  //return TRUE;
  return FALSE;
}

CFORCEINLINE
VOID
borderless_on_wm_size(
  HWND hWnd,
  UINT state,
  int  cx,
  int  cy)
{
  UNREFERENCED_PARAMETER(hWnd);

  if(SIZE_MINIMIZED != state)
  {
    g_nWidth  = cx;
    g_nHeight = cy;
  }
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
    if(hWnd == GetForegroundWindow())
    {
      (VOID) PostMessage(hWnd, WM_DESTROY, 0, 0);
    }

    break;
  }
  default: break;
  }
}

static BOOL fAlwaysOnTop = FALSE;

CFORCEINLINE
VOID
borderless_on_wm_ncrbuttondown(
  HWND hWnd,
  BOOL fDoubleClick,
  int  x,
  int  y,
  UINT codeHitTest)
{
  RECT r = { 0 };
  (void) GetWindowRect(hWnd, &r);
  POINT pt = {0};
  (void) GetCursorPos(&pt);
  pt.x -= r.left;
  pt.y -= r.top;
  UNREFERENCED_PARAMETER(fDoubleClick);
  UNREFERENCED_PARAMETER(x);
  UNREFERENCED_PARAMETER(y);
  UNREFERENCED_PARAMETER(codeHitTest);
  HMENU hPopupMenu = CreatePopupMenu();

  AppendMenu(hPopupMenu, MF_BYPOSITION | MF_STRING, 1001, _T("Exit"));
  AppendMenu(
    hPopupMenu,
    (fAlwaysOnTop? MF_CHECKED:0) | MF_BYPOSITION | MF_STRING,
    1002,
    _T("Always On Top")
  );
  ClientToScreen(hWnd, &pt);
  if (!TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL)) {
    MessageBox(hWnd, _T("Failed to display context menu"), _T("Error"), MB_ICONERROR);
  }
  DestroyMenu(hPopupMenu);
}

CFORCEINLINE
VOID
borderless_on_wm_command(
  HWND hWnd,
  INT  id,
  HWND hwndChild,
  int  codeNotify)
{
  UNREFERENCED_PARAMETER(hwndChild);
  UNREFERENCED_PARAMETER(codeNotify);
  switch (id) {
  case 1001:
    PostMessage(hWnd, WM_CLOSE,0,0);
    break;
  case 1002:
  {
    fAlwaysOnTop = !fAlwaysOnTop;
    (VOID) SetWindowPos(
      hWnd,
      fAlwaysOnTop? HWND_TOPMOST : HWND_NOTOPMOST,
      0,
      0,
      0,
      0,
      SWP_NOMOVE | SWP_NOSIZE
    );
    break;
  }
  }
  FORWARD_WM_COMMAND(hWnd, id, hwndChild, codeNotify, DefWindowProc);
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

CFORCEINLINE
UINT
borderless_on_wm_nccalcsize(
  HWND               hWnd,
  BOOL               fCalcValidRects,
  NCCALCSIZE_PARAMS* lpcsp)
{
  if (fCalcValidRects)
  {
    //UINT dpi     = GetDpiForWindow(hWnd);
    //int  frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
    //int  frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    //int  padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    if (IsMaximized(hWnd))
    {
      MONITORINFO monitor_info = {sizeof(MONITORINFO)};
      if (GetMonitorInfo(
            MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),
            (LPMONITORINFO)&monitor_info
      ))
      {
        lpcsp->rgrc[0] = monitor_info.rcWork;
        //lpcsp->rgrc[0].right  += frame_x + padding;
        //lpcsp->rgrc[0].left   -= frame_x + padding;
        //lpcsp->rgrc[0].bottom += frame_y + padding - 2;
        return 0u;
      }
    }
    lpcsp->rgrc[0].bottom += 1;
    return WVR_VALIDRECTS;

    //lpcsp->rgrc[0].right  -= frame_x + padding;
    //lpcsp->rgrc[0].left   += frame_x + padding;
    //lpcsp->rgrc[0].bottom -= (frame_y + padding);

    //lpcsp->rgrc[0].bottom -= 1;

    //lpcsp->rgrc[0].top   += 1;
    //lpcsp->rgrc[0].right -= 1;
    //lpcsp->rgrc[0].left  += 1;

    //return WVR_VALIDRECTS;
    //return WVR_VALIDRECTS;
    //return 0;
  }
  return 0U;
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

EXTERN_C
CFORCEINLINE
void
CaptureScreen(
  HWND   hWnd,
  LPBYTE buf)
{
  POINT cursor = {0};
  (void) GetCursorPos(&cursor);
  RECT r = {0};
  (void) GetWindowRect(hWnd, &r);
  //int  nWindowWidth  = labs(r.right - r.left);
  //int  nWindowHeight = labs(r.bottom - r.top);
  //int  nScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
  //int  nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
  HWND hDesktopWnd = GetDesktopWindow();
  HDC  hDesktopDC  = GetDC(hDesktopWnd);
  HDC  hCaptureDC  = CreateCompatibleDC(hDesktopDC);

  if(!hBitmapBg)
  {
    hBitmapBg = CreateCompatibleBitmap(
      hDesktopDC,
      256,
      256
    );
  }
  if(hBitmapBg)
  {
    BITMAPINFOHEADER bi = {0};
    bi.biSize        =sizeof(BITMAPINFOHEADER);
    bi.biWidth       = 256;
    bi.biHeight      = 256; // Negative to flip vertically for OpenGL
    bi.biPlanes      =1;
    bi.biBitCount    =32; // RGBA
    bi.biCompression =BI_RGB;
    SelectObject(hCaptureDC,hBitmapBg);
    BitBlt(
      hCaptureDC,
      0,
      0,
      256,
      256,
      hDesktopDC,
      //0,
      //0,
      max(min(cursor.x - (LONG)(0.5f * 256), 2560 - 256), 0),
      max(min(cursor.y - (LONG)(0.5f * 256), 1600 - 256), 0),
      SRCCOPY|CAPTUREBLT
    );

    GetDIBits(hCaptureDC, hBitmapBg, 0, 256, buf, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
  }
  //BitBlt(hDC, 0, 0, nScreenWidth, nScreenHeight, hCaptureDC, 0, 0, SRCCOPY);
  //here to save the captured image to disk

  ReleaseDC(hDesktopWnd,hDesktopDC);
  DeleteDC(hCaptureDC);
}

CFORCEINLINE
VOID
borderless_on_wm_paint(
  HWND hWnd)
{
  RECT        r = {0};
  PAINTSTRUCT ps;
  HDC         hDC = BeginPaint(hWnd, &ps);
  (void) GetWindowRect(hWnd, &r);
  (void)hDC;
  #if 0
  //CaptureScreen(hDC, &r);
  if (hBitmapBg)
  {

    BITMAP  bitmap;
    HDC     hdcMem;
    HGDIOBJ oldBitmap;
    hdcMem    = CreateCompatibleDC(hDC);
    oldBitmap = SelectObject(hdcMem, hBitmapBg);
    GetObject(hBitmap, sizeof(bitmap), &bitmap);
    BitBlt(hDC, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);
  }
  #endif
  (void) EndPaint(hWnd, &ps);
  DwmFlush();
  FORWARD_WM_PAINT(hWnd, DefWindowProc);
}

FORCEINLINE
HICON
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
FORCEINLINE
LPCTSTR
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
  HWND        hPump)
{
  UNREFERENCED_PARAMETER(wnd);

  MSG  msg  = {0};
  BOOL quit = FALSE;
  while (PeekMessage(&msg, hPump, 0U, 0U, PM_REMOVE))
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
  HWND        hPump)
{
  UNREFERENCED_PARAMETER(wnd);

  for (;;)
  {
    MSG  msg      = {0};
    BOOL received = GetMessage(&msg, hPump, 0U, 0U);
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
  return S_OK == SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
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
  ULONG _ = {0};
  return STATUS_SUCCESS == NtSetTimerResolution(
    hnsDesiredResolution,
    (BOOLEAN) !!setResolution,
    hnsCurrentResolution? hnsCurrentResolution : &_
  );
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
  LONGLONG e = tmr->stop.QuadPart - tmr->start.QuadPart;
  if(!isinf(e))
  {
  tmr->elapsed.QuadPart = e;
  tmr->elapsedAccum.QuadPart +=e;
  }
  return success;
}

EXTERN_C
FORCEINLINE
DOUBLE
w32_timer_elapsed(
  w32_timer* tmr)
  {
    DOUBLE e = (DOUBLE)tmr->elapsedAccum.QuadPart / (DOUBLE)tmr->freq.QuadPart;
    return isinf(e) ? 0.0 : e;
  }

EXTERN_C
FORCEINLINE
BOOL
w32_timer_reset(
  w32_timer* tmr)
{
  tmr->elapsed.QuadPart = 0;
  tmr->elapsedAccum.QuadPart = 0;
  tmr->start.QuadPart = 0;
  tmr->stop.QuadPart = 0;
  return TRUE;
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
  static UINT tID = 0;
  switch (msg) {
  HANDLE_MSG(hWnd, WM_SIZE, borderless_on_wm_size);
  //HANDLE_MSG(hWnd, WM_CREATE,        borderless_on_wmcreate);
  HANDLE_MSG(hWnd, WM_KEYUP,         borderless_on_wm_keyup);
  HANDLE_MSG(hWnd, WM_NCRBUTTONDOWN, borderless_on_wm_ncrbuttondown);
  HANDLE_MSG(hWnd, WM_COMMAND,       borderless_on_wm_command);
  HANDLE_MSG(hWnd, WM_ACTIVATE,      borderless_on_wm_activate);
  HANDLE_MSG(hWnd, WM_NCHITTEST,     borderless_on_wm_nchittest);
  HANDLE_MSG(hWnd, WM_NCCALCSIZE,    borderless_on_wm_nccalcsize);
  //HANDLE_MSG(hWnd, WM_PAINT,         borderless_on_wm_paint);
  HANDLE_MSG(hWnd, WM_ERASEBKGND,    borderless_on_wm_erasebkgnd);
  HANDLE_MSG(hWnd, WM_DESTROY,       on_wm_destroy);
  case (WM_ENTERSIZEMOVE): {
    (void) SetTimer(hWnd, (UINT_PTR)tID, USER_TIMER_MINIMUM, NULL);
    return 0;

  }
  case (WM_TIMER): {
    static w32_window* wnd = NULL;
    if(!wnd){

      wnd = (w32_window*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }
    wender(wnd);
    //CaptureScreen(hWnd, TRUE);
    //borderless_on_wm_paint(hWnd);
    DwmFlush();
    return 0;
  }
  case (WM_EXITSIZEMOVE): {
    (void) KillTimer(hWnd, (UINT_PTR)tID);
    return 0;
  }
  //HANDLE_MSG(hWnd, WM_ERASEBKGND, borderless_on_wm_erasebkgnd);
  default: return DefWindowProc(hWnd, msg, wParam, lParam);
  }
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

CFORCEINLINE
PROC
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
  pfd.dwFlags  = PFD_DRAW_TO_WINDOW |      // Format Must Support Window
                 PFD_SUPPORT_OPENGL |      // Format Must Support OpenGL
                 PFD_SUPPORT_COMPOSITION | // Format Must Support Composition
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
  //}

  if (NULL != hRC) wglMakeCurrent(NULL, NULL);
  if (NULL != hRC) wglDeleteContext(hRC);
  if (NULL != hWnd && NULL != hDC) ReleaseDC(hWnd, hDC);
  if (NULL != hWnd) DestroyWindow(hWnd);
  UnregisterClass(_T("GLEW"), GetModuleHandle(NULL));
  return pfMSAA;
}

EXTERN_C
CFORCEINLINE
void
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
                 PFD_SWAP_EXCHANGE  |
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
  g_hRC = hRC;
  g_hDC = hDC;

  (void) memset(g_lpPixelBuf, 0, (size_t)(256*256* 4));
  CaptureScreen(wnd->hWnd, g_lpPixelBuf);

  glGenTextures(1, &g_hTex);
  glBindTexture(GL_TEXTURE_2D, g_hTex);
  ASSERT_W32(g_hTex);
  // Upload texture data
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  //glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_BGRA,
    256,
    256,
    0,
    GL_BGRA,
    GL_UNSIGNED_BYTE,
    g_lpPixelBuf
  );
}

EXTERN_C
CFORCEINLINE
void
wender(
  w32_window* wnd)
{
  static w32_timer* rt = NULL;
  static w32_timer* ft = NULL;
  static w32_timer* cam = NULL;
  static w32_timer* gdi = NULL;
  static w32_timer* upload = NULL;
  static w32_timer* quad = NULL;
  static w32_timer* swap = NULL;
  static int samples = 0;
  if(!rt)
  {
    rt = malloc(sizeof(w32_timer));
    ft = malloc(sizeof(w32_timer));
    cam = malloc(sizeof(w32_timer));
    gdi = malloc(sizeof(w32_timer));
    upload = malloc(sizeof(w32_timer));
    quad = malloc(sizeof(w32_timer));
    swap = malloc(sizeof(w32_timer));
    ASSERT_W32(rt);
    ASSERT_W32(ft);
    ASSERT_W32(cam);
    ASSERT_W32(gdi);
    ASSERT_W32(upload);
    ASSERT_W32(quad);
    ASSERT_W32(swap);
    w32_timer_init(rt);
    w32_timer_init(ft);
    w32_timer_init(cam);
    w32_timer_init(gdi);
    w32_timer_init(upload);
    w32_timer_init(quad);
    w32_timer_init(swap);
    w32_timer_start(rt);
  }
  
  w32_timer_start(ft);

  w32_timer_start(cam);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(0,0,g_nWidth, g_nHeight);
  w32_timer_stop(cam);

  //(void) memset(g_lpPixelBuf, 125, (size_t)(256*256*4));
  w32_timer_start(gdi);
  CaptureScreen(wnd->hWnd, g_lpPixelBuf);
  for (int i = 0; i < 256; ++i)
    for (int j = 0; j < 256; ++j)
      g_lpPixelBuf[((j + (i * 256)) * 4) + 3] = 225;   // Alpha is at offset 3
  w32_timer_stop(gdi);

  w32_timer_start(upload);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_hTex);
  #if 1
  glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0,
    0,
    256,
    256,
    GL_BGRA,
    GL_UNSIGNED_BYTE,
    g_lpPixelBuf
  );
  #endif
  w32_timer_stop(upload);
  // Set color (white ensures no tint)
  //glColor3f(1.0f, 1.0f, 1.0f);

  // Draw a textured quad
  w32_timer_start(quad);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(-1.0f, -1.0f);           // Bottom-left
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(1.0f, -1.0f);           // Bottom-right
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(1.0f,1.0f);           // Top-right
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(-1.0f,1.0f);           // Top-left
  glEnd();

  // Disable texturing
  glDisable(GL_TEXTURE_2D);
  w32_timer_stop(quad);
  w32_timer_start(swap);
  SwapBuffers(g_hDC);
  w32_timer_stop(swap);
  w32_timer_stop(ft);

  ++samples;
  w32_timer_stop(rt);
  if(w32_timer_elapsed(rt) < 0.0 || (w32_timer_elapsed(rt) >= 1.0))
  //if(samples >= 60)
  {
    //double _rt = w32_timer_elapsed(rt);
    //double _ft = w32_timer_elapsed(ft);
    //double _cam = w32_timer_elapsed(cam);
    //double _gdi = w32_timer_elapsed(gdi);
    //double _upload = w32_timer_elapsed(upload);
    //double _quad = w32_timer_elapsed(quad);
    //double _swap = w32_timer_elapsed(swap);
    //char buf[256] = {0};
    //sprintf(buf, 
    //"    rt: %lf\n    ft: %lf\n   cam: %lf\n   gdi: %lf\nupload: %lf\n  quad: %lf\n  swap: %lf\n",
    //_rt,
    //_ft / samples,
    //_cam / samples,
    //_gdi / samples,
    //_upload / samples,
    //_quad / samples,
    //_swap / samples
    //);
    //samples = 0;
    ////fwrite(buf, 1, strlen(buf), stdout);
    //printf("%s", buf);
    w32_timer_reset(rt);
    w32_timer_reset(ft);
    w32_timer_reset(cam);
    w32_timer_reset(gdi);
    w32_timer_reset(upload);
    w32_timer_reset(quad);
    w32_timer_reset(swap);
  }
  w32_timer_start(rt);
}
