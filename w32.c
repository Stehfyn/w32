/*=============================================================================
** 1. REFERENCES
**===========================================================================*/
/*=============================================================================
** 2. INCLUDE FILES
**===========================================================================*/
#define _CRT_SECURE_NO_WARNINGS
//#define _CRT_DISABLE_PERFCRIT_LOCKS
#include "w32.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <intrin.h>
#include <windowsx.h>
#include <initguid.h>
#pragma warning(disable : 4255 4820)
#include <winternl.h>
#include <shellscalingapi.h>
#include <dwmapi.h>
#include <roapi.h>
#include <Windows.ui.notifications.h>
#pragma warning(default : 4255 4820)
#include <notificationactivationcallback.h>

#include <GL\GL.h>
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
#define CAPTURE_REGION (800)
#define GUID_Impl_INotificationActivationCallback_Textual "0F82E845-CB89-4039-BDBF-67CA33254C76"
DEFINE_GUID(
  GUID_Impl_INotificationActivationCallback,
  0xf82e845,
  0xcb89,
  0x4039,
  0xbd,
  0xbf,
  0x67,
  0xca,
  0x33,
  0x25,
  0x4c,
  0x76
);

//{ 0x53E31837, 0x6600, 0x4A81, 0x93, 0x95, 0x75, 0xCF, 0xFE, 0x74, 0x6F, 0x94 };

DEFINE_GUID(
  IID_IToastNotificationManagerStatics,
  0x50ac103f,
  0xd235,
  0x4598,
  0xbb,
  0xef,
  0x98,
  0xfe,
  0x4d,
  0x1a,
  0x3a,
  0xd4
);

DEFINE_GUID(
  IID_IToastNotificationFactory,
  0x04124b20,
  0x82c6,
  0x4229,
  0xb1,
  0x09,
  0xfd,
  0x9e,
  0xd4,
  0x66,
  0x2b,
  0x53
);

DEFINE_GUID(
  IID_IXmlDocument,
  0xf7f3a506,
  0x1e87,
  0x42d6,
  0xbc,
  0xfb,
  0xb8,
  0xc8,
  0x09,
  0xfa,
  0x54,
  0x94
);

DEFINE_GUID(
  IID_IXmlDocumentIO,
  0x6cd0e74e,
  0xee65,
  0x4489,
  0x9e,
  0xbf,
  0xca,
  0x43,
  0xe8,
  0x7b,
  0xa6,
  0x37
);

#define APP_ID L"w32_demo"
#define TOAST_ACTIVATED_LAUNCH_ARG "-ToastActivated"
/*=============================================================================
** 3.2 Types
**===========================================================================*/
typedef struct Impl_IGeneric {
  IUnknownVtbl* lpVtbl;
  //DWORD         unused;
  LONG64 dwRefCount;
} Impl_IGeneric;

/*=============================================================================
** 3.3 External global variables
**===========================================================================*/

/*=============================================================================
** 3.4 Static global variables
**===========================================================================*/
const wchar_t wszBannerText[] =
  L"<toast scenario=\"reminder\" "
  L"activationType=\"foreground\" launch=\"action=mainContent\" duration=\"short\">\r\n"
  L"	<visual>\r\n"
  L"		<binding template=\"ToastGeneric\">\r\n"
  L"			<text><![CDATA[This is a demo notification]]></text>\r\n"
  L"			<text><![CDATA[It contains 2 lines of text]]></text>\r\n"
  L"			<text placement=\"attribution\"><![CDATA[Created by Valentin-Gabriel Radu (github.com/valinet)]]></text>\r\n"
  L"		</binding>\r\n"
  L"	</visual>\r\n"
  L"  <actions>\r\n"
  L"	  <input id=\"tbReply\" type=\"text\" placeHolderContent=\"Send a message to the app\"/>\r\n"
  L"	  <action content=\"Send\" activationType=\"foreground\" arguments=\"action=reply\"/>\r\n"
  L"	  <action content=\"Visit GitHub\" activationType=\"protocol\" arguments=\"https://github.com/stehfyn\"/>\r\n"
  L"	  <action content=\"Close app\" activationType=\"foreground\" arguments=\"action=closeApp\"/>\r\n"
  L"  </actions>\r\n"
  L"	<audio src=\"ms-winsoundevent:Notification.Default\" loop=\"false\" silent=\"false\"/>\r\n"
  L"</toast>\r\n";

static DWORD dwMainThreadId = 0;

static HBITMAP hBitmap;
static HBITMAP hBitmapBg;
static HDC     g_hDC;
static HGLRC   g_hRC;
static GLuint  g_hTex = 0;
static BYTE    g_lpPixelBuf[CAPTURE_REGION*CAPTURE_REGION*4];
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
ULONG
STDMETHODCALLTYPE
Impl_IGeneric_AddRef(
  Impl_IGeneric* _this
);

static
CFORCEINLINE
ULONG
STDMETHODCALLTYPE
Impl_IGeneric_Release(
  Impl_IGeneric* _this
);

static
HRESULT
STDMETHODCALLTYPE
Impl_INotificationActivationCallback_QueryInterface(
  Impl_IGeneric* _this,
  REFIID         riid,
  void**         ppvObject
);

static
HRESULT
STDMETHODCALLTYPE
Impl_INotificationActivationCallback_Activate(
  INotificationActivationCallback*    _this,
  LPCWSTR                             appUserModelId,
  LPCWSTR                             invokedArgs,
  const NOTIFICATION_USER_INPUT_DATA* data,
  ULONG                               count
);

static
HRESULT
STDMETHODCALLTYPE
Impl_IClassFactory_QueryInterface(
  Impl_IGeneric* _this,
  REFIID         riid,
  void**         ppvObject
);

static
HRESULT
STDMETHODCALLTYPE
Impl_IClassFactory_LockServer(
  IClassFactory* _this,
  BOOL           flock
);

static
HRESULT
STDMETHODCALLTYPE
Impl_IClassFactory_CreateInstance(
  IClassFactory* _this,
  IUnknown*      punkOuter,
  REFIID         vTableGuid,
  void**         ppv
);

#pragma warning(disable : 4113)
static
CONST
INotificationActivationCallbackVtbl Impl_INotificationActivationCallback_Vtbl = {
  .QueryInterface = Impl_INotificationActivationCallback_QueryInterface,
  .AddRef         = Impl_IGeneric_AddRef,
  .Release        = Impl_IGeneric_Release,
  .Activate       = Impl_INotificationActivationCallback_Activate
};

static
CONST
IClassFactoryVtbl Impl_IClassFactory_Vtbl = {
  .QueryInterface = Impl_IClassFactory_QueryInterface,
  .AddRef         = Impl_IGeneric_AddRef,
  .Release        = Impl_IGeneric_Release,
  .LockServer     = Impl_IClassFactory_LockServer,
  .CreateInstance = Impl_IClassFactory_CreateInstance
};
#pragma warning(default : 4113)

static
LRESULT
CALLBACK
wndproc(
  HWND   hWnd,
  UINT   msg,
  WPARAM wParam,
  LPARAM lParam
);

static
BOOL
CALLBACK
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
ULONG
STDMETHODCALLTYPE
Impl_IGeneric_AddRef(
  Impl_IGeneric* _this)
{
  return (ULONG) InterlockedIncrement64(&(_this->dwRefCount));
}

static
ULONG
STDMETHODCALLTYPE
Impl_IGeneric_Release(
  Impl_IGeneric* _this)
{
  LONG64 dwNewRefCount = InterlockedDecrement64(&(_this->dwRefCount));
  if (!dwNewRefCount) free(_this);
  return (ULONG) dwNewRefCount;
}

static
HRESULT
STDMETHODCALLTYPE
Impl_INotificationActivationCallback_QueryInterface(
  Impl_IGeneric* _this,
  REFIID         riid,
  void**         ppvObject)
{
  UNREFERENCED_PARAMETER(_this);

  if (!IsEqualIID(riid, &IID_INotificationActivationCallback) && !IsEqualIID(riid, &IID_IUnknown))
  {
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }
  *ppvObject = _this;
  _this->lpVtbl->AddRef((IUnknown*) _this);
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Impl_INotificationActivationCallback_Activate(
  INotificationActivationCallback*    _this,
  LPCWSTR                             appUserModelId,
  LPCWSTR                             invokedArgs,
  const NOTIFICATION_USER_INPUT_DATA* data,
  ULONG                               count)
{
  UNREFERENCED_PARAMETER(_this);
  __debugbreak();
  (VOID) wprintf(
    L"Interacted with notification from AUMID \"%s\" with arguments: \"%s\". User input count: %lu.\n",
    appUserModelId,
    invokedArgs,
    count
  );
  if (!_wcsicmp(invokedArgs, L"action=closeApp"))
  {
    (VOID) PostThreadMessage(dwMainThreadId, WM_QUIT, 0, 0);
  }
  else if (!_wcsicmp(invokedArgs, L"action=reply"))
  {
    for (unsigned long i = 0; i != count; ++i)
    {
      if (!_wcsicmp(data[i].Key, L"tbReply"))
      {
        (VOID) wprintf(L"Reply was \"%s\".\n", data[i].Value);
      }
    }
  }
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Impl_IClassFactory_QueryInterface(
  Impl_IGeneric* _this,
  REFIID         riid,
  void**         ppvObject)
{
  if (!IsEqualIID(riid, &IID_IClassFactory) && !IsEqualIID(riid, &IID_IUnknown))
  {
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }
  *ppvObject = _this;
  _this->lpVtbl->AddRef((IUnknown*) _this);
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Impl_IClassFactory_LockServer(
  IClassFactory* _this,
  BOOL           flock)
{
  UNREFERENCED_PARAMETER(_this);
  UNREFERENCED_PARAMETER(flock);

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Impl_IClassFactory_CreateInstance(
  IClassFactory* _this,
  IUnknown*      punkOuter,
  REFIID         vTableGuid,
  void**         ppv)
{
  UNREFERENCED_PARAMETER(_this);

  HRESULT        hr      = E_NOINTERFACE;
  Impl_IGeneric* thisobj = NULL;
  *ppv = 0;

  if (punkOuter){
    hr = CLASS_E_NOAGGREGATION;
  }
  else
  {
    BOOL bOk = FALSE;
    if (!(thisobj = malloc(sizeof(Impl_IGeneric))))
    {
      hr = E_OUTOFMEMORY;
    }
    else
    {
      thisobj->lpVtbl = (IUnknownVtbl*) &Impl_INotificationActivationCallback_Vtbl;
      bOk             = TRUE;
    }
    if (bOk)
    {
      thisobj->dwRefCount = 1;
      hr                  = thisobj->lpVtbl->QueryInterface((IUnknown*) thisobj, vTableGuid, ppv);
      thisobj->lpVtbl->Release((IUnknown*) thisobj);
    }
    else
    {
      return hr;
    }
  }

  return hr;
}

static
LRESULT
CALLBACK
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
CALLBACK
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
  POINT window_center = { 0 };
  window_center.x = (LONG)(0.5f * (r.right + r.left));
  window_center.y = (LONG)(0.5f * (r.bottom + r.top));
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
      CAPTURE_REGION,
      CAPTURE_REGION
    );
  }
  if(hBitmapBg)
  {
    BITMAPINFOHEADER bi = {0};
    bi.biSize        =sizeof(BITMAPINFOHEADER);
    bi.biWidth       = CAPTURE_REGION;
    bi.biHeight      = CAPTURE_REGION; // Negative to flip vertically for OpenGL
    bi.biPlanes      =1;
    bi.biBitCount    =32; // RGBA
    bi.biCompression =BI_RGB;
    SelectObject(hCaptureDC,hBitmapBg);
    BitBlt(
      hCaptureDC,
      0,
      0,
      CAPTURE_REGION,
      CAPTURE_REGION,
      hDesktopDC,
      //0,
      //0,
      //max(min(cursor.x - (LONG)(0.5f * 256), 2560 - 256), 0),
      //max(min(cursor.y - (LONG)(0.5f * 256), 1600 - 256), 0),
      max(min(window_center.x - (LONG)(0.5f * CAPTURE_REGION), 2560 - CAPTURE_REGION), 0),
      max(min(window_center.y - (LONG)(0.5f * CAPTURE_REGION), 1600 - CAPTURE_REGION), 0),
      //SRCCOPY|CAPTUREBLT
      SRCCOPY
    );

    GetDIBits(hCaptureDC, hBitmapBg, 0, CAPTURE_REGION, buf, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
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
EXTERN_C
FORCEINLINE
BOOL
w32_init_thunk(
  BOOL fInvoked)
{
  HRESULT        hr                = S_OK;
  Impl_IGeneric* pClassFactory     = NULL;
  BOOL           bOk               = FALSE;
  BOOL           bInvokedFromToast = fInvoked;
  dwMainThreadId = GetCurrentThreadId();
  (VOID) bOk;

  /*
   * Initialize COM and Windows Runtime on this thread. Make sure that the threading models of the two match.
   */
  if (SUCCEEDED(hr))
  {
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  }

  if (SUCCEEDED(hr))
  {
    hr = RoInitialize(RO_INIT_MULTITHREADED);
  }

  /*
   * Allocate class factory. This factory produces our implementation of the INotificationActivationCallback interface.
   * This interface has an ::Activate member method that gets called when someone interacts with the toast notification.
   */
  if (SUCCEEDED(hr))
  {
    if (!(pClassFactory = malloc(sizeof(Impl_IGeneric)))) hr = E_OUTOFMEMORY;
    else
    {
      pClassFactory->lpVtbl     = (IUnknownVtbl*) &Impl_IClassFactory_Vtbl;
      pClassFactory->dwRefCount = 1;
    }
  }
  /*
   * Instead of having to register our COM class in the registry beforehand, we opt to registering it at runtime;
   * we associate our GUID with the class factory that provides our INotificationActivationCallback interface.
   */
  DWORD dwCookie = 0;
  if (SUCCEEDED(hr))
  {
    hr = CoRegisterClassObject(
      &GUID_Impl_INotificationActivationCallback,
      (LPUNKNOWN) pClassFactory,
      CLSCTX_LOCAL_SERVER,
      REGCLS_MULTIPLEUSE,
      &dwCookie
    );
  }

  /*
   * Construct the path to our EXE that will be used to launch it when something requests our interface.
   * As said above, registration is dynamic - as long as this app runs, COM knows about the fact that this
   * app implements our INotificationActivationCallback interface. The info here is used when this app has
   * closed and someone clicks the toast notification for example; in that case, since our app is not
   * running, thus CoRegisterClassObject was not called, COM needs info on what EXE contains the implementation
   * of the interface, and we specify that here; without setting this, clicking on notifications will do nothing
   */
  wchar_t wszExePath[MAX_PATH + 100];
  ZeroMemory(wszExePath, MAX_PATH + 100);
  if (SUCCEEDED(hr))
  {
    hr = (GetModuleFileNameW(NULL, wszExePath + 1, MAX_PATH) != 0 ? S_OK : E_FAIL);
  }

  if (SUCCEEDED(hr))
  {
    wszExePath[0] = L'"';
    wcscat_s(wszExePath, MAX_PATH + 100, L"\" " _T(TOAST_ACTIVATED_LAUNCH_ARG));
  }

  if (SUCCEEDED(hr))
  {
    hr = HRESULT_FROM_WIN32(
      RegSetValueW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Classes\\CLSID\\{" _T(
          GUID_Impl_INotificationActivationCallback_Textual
        ) L"}\\LocalServer32",
        REG_SZ,
        wszExePath,
        (DWORD) wcslen(wszExePath) + 1
      )
    );
  }

  /*
   * Here we set some info about our app and associate our AUMID with the GUID from above
   * (the one that is associated with our class factory which produces our INotificationActivationCallback interface)
   */
  if (SUCCEEDED(hr))
  {
    hr = HRESULT_FROM_WIN32(
      RegSetKeyValueW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Classes\\AppUserModelId\\" APP_ID,
        L"DisplayName",
        REG_SZ,
        L"Toast Activator Pure C Example",
        31 * sizeof(wchar_t)
      )
    );
  }

  if (SUCCEEDED(hr))
  {
    hr = HRESULT_FROM_WIN32(
      RegSetKeyValueW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Classes\\AppUserModelId\\" APP_ID,
        L"IconBackgroundColor",
        REG_SZ,
        L"FF00FF00",
        9 * sizeof(wchar_t)
      )
    );
  }

  if (SUCCEEDED(hr))
  {
    hr = HRESULT_FROM_WIN32(
      RegSetKeyValueW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Classes\\AppUserModelId\\" APP_ID,
        L"CustomActivator",
        REG_SZ,
        L"{" _T(GUID_Impl_INotificationActivationCallback_Textual) L"}",
        39 * sizeof(wchar_t)
      )
    );
  }
/*
 * We will display a notification only when this app is launched standalone (not by interacting with a notification)
 */
  HSTRING_HEADER hshAppId;
  HSTRING        hsAppId = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = WindowsCreateStringReference(
      APP_ID,
      (UINT32)(sizeof(APP_ID) / sizeof(TCHAR) - 1),
      &hshAppId,
      &hsAppId
    );
  }

  HSTRING_HEADER hshToastNotificationManager;
  HSTRING        hsToastNotificationManager = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr =
      WindowsCreateStringReference(
        RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
        (UINT32)(sizeof(
                   RuntimeClass_Windows_UI_Notifications_ToastNotificationManager)
                 / sizeof(wchar_t) - 1),
        &hshToastNotificationManager,
        &hsToastNotificationManager
      );
  }

  __x_ABI_CWindows_CUI_CNotifications_CIToastNotificationManagerStatics* pToastNotificationManager =
    NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = RoGetActivationFactory(
      hsToastNotificationManager,
      &IID_IToastNotificationManagerStatics,
      (LPVOID*)&pToastNotificationManager
    );
  }

  __x_ABI_CWindows_CUI_CNotifications_CIToastNotifier* pToastNotifier = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = pToastNotificationManager->lpVtbl->CreateToastNotifierWithId(
      pToastNotificationManager,
      hsAppId,
      &pToastNotifier
    );
  }

  HSTRING_HEADER hshToastNotification;
  HSTRING        hsToastNotification = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = WindowsCreateStringReference(
      RuntimeClass_Windows_UI_Notifications_ToastNotification,
      (UINT32)(sizeof(RuntimeClass_Windows_UI_Notifications_ToastNotification) / sizeof(wchar_t) -
               1),
      &hshToastNotification,
      &hsToastNotification
    );
  }

  __x_ABI_CWindows_CUI_CNotifications_CIToastNotificationFactory* pNotificationFactory = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = RoGetActivationFactory(
      hsToastNotification,
      &IID_IToastNotificationFactory,
      (LPVOID*)&pNotificationFactory
    );
  }

  HSTRING_HEADER hshXmlDocument;
  HSTRING        hsXmlDocument = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = WindowsCreateStringReference(
      RuntimeClass_Windows_Data_Xml_Dom_XmlDocument,
      (UINT32)(sizeof(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument) / sizeof(wchar_t) - 1),
      &hshXmlDocument,
      &hsXmlDocument
    );
  }

  HSTRING_HEADER hshBanner;
  HSTRING        hsBanner = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = WindowsCreateStringReference(
      wszBannerText,
      (UINT32)(sizeof(wszBannerText) / sizeof(wchar_t) - 1),
      &hshBanner,
      &hsBanner
    );
  }

  IInspectable* pInspectable = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = RoActivateInstance(hsXmlDocument, &pInspectable);
  }

  __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocument* pXmlDocument = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = pInspectable->lpVtbl->QueryInterface(pInspectable, &IID_IXmlDocument, &pXmlDocument);
  }

  __x_ABI_CWindows_CData_CXml_CDom_CIXmlDocumentIO* pXmlDocumentIO = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = pXmlDocument->lpVtbl->QueryInterface(pXmlDocument, &IID_IXmlDocumentIO, &pXmlDocumentIO);
  }

  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = pXmlDocumentIO->lpVtbl->LoadXml(pXmlDocumentIO, hsBanner);
  }

  __x_ABI_CWindows_CUI_CNotifications_CIToastNotification* pToastNotification = NULL;
  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = pNotificationFactory->lpVtbl->CreateToastNotification(
      pNotificationFactory,
      pXmlDocument,
      &pToastNotification
    );
  }

  if (SUCCEEDED(hr) && !bInvokedFromToast)
  {
    hr = pToastNotifier->lpVtbl->Show(pToastNotifier, pToastNotification);
  }

  return SUCCEEDED(hr);
}

EXTERN_C
FORCEINLINE
BOOL
w32_release_thunk(
  VOID)
{
  return TRUE;
}

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
    (MONITORENUMPROC) monitorenumproc,
    (LPARAM) displayInfo
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
  BOOL     success = QueryPerformanceCounter(&tmr->stop);
  LONGLONG e       = tmr->stop.QuadPart - tmr->start.QuadPart;
  if(!isinf(e))
  {
    tmr->elapsed.QuadPart       = e;
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
  tmr->elapsed.QuadPart      = 0;
  tmr->elapsedAccum.QuadPart = 0;
  tmr->start.QuadPart        = 0;
  tmr->stop.QuadPart         = 0;
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

PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB    = NULL;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT         = NULL;
PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT      = NULL;
// OpenGL 2.0 and above function pointers
typedef void (APIENTRY* PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY* PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRY* PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRY* PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY* PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint* buffers);
typedef void (APIENTRY* PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint* arrays);
typedef void (APIENTRY* PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRY* PFNGLBUFFERDATAPROC) (
  GLenum target,
  GLsizeiptr size,
  const void* data,
  GLenum usage
);
typedef void (APIENTRY* PFNGLSHADERSOURCEPROC) (
  GLuint shader,
  GLsizei count,
  const GLchar* const* string,
  const GLint* length
);
typedef void (APIENTRY* PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC) (
  GLuint index,
  GLint size,
  GLenum type,
  GLboolean normalized,
  GLsizei stride,
  const void* pointer
);
typedef void (APIENTRY* PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC) (
  GLuint shader,
  GLsizei bufSize,
  GLsizei* length,
  GLchar* infoLog
);
typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC) (
  GLuint program,
  GLsizei bufSize,
  GLsizei* length,
  GLchar* infoLog
);
typedef GLint (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar* name);
typedef void (APIENTRY* PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);

extern
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern
PFNGLUNIFORM1FPROC glUniform1f;
extern
PFNGLATTACHSHADERPROC glAttachShader;
extern
PFNGLCOMPILESHADERPROC glCompileShader;
extern
PFNGLCREATEPROGRAMPROC glCreateProgram;
extern
PFNGLCREATESHADERPROC glCreateShader;
extern
PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern
PFNGLDELETESHADERPROC glDeleteShader;
extern
PFNGLDETACHSHADERPROC glDetachShader;
extern
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern
PFNGLGENBUFFERSARBPROC glGenBuffers;
extern
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern
PFNGLBUFFERDATAPROC glBufferData;
extern
PFNGLSHADERSOURCEPROC glShaderSource;
extern
PFNGLUSEPROGRAMPROC glUseProgram;
extern
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern
PFNGLLINKPROGRAMPROC glLinkProgram;
extern
PFNGLGETSHADERIVPROC glGetShaderiv;
extern
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern
PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern
PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORM1FPROC          glUniform1f          = NULL;

PFNGLATTACHSHADERPROC             glAttachShader             = NULL;
PFNGLCOMPILESHADERPROC            glCompileShader            = NULL;
PFNGLCREATEPROGRAMPROC            glCreateProgram            = NULL;
PFNGLCREATESHADERPROC             glCreateShader             = NULL;
PFNGLDELETEPROGRAMPROC            glDeleteProgram            = NULL;
PFNGLDELETESHADERPROC             glDeleteShader             = NULL;
PFNGLDETACHSHADERPROC             glDetachShader             = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray  = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = NULL;
PFNGLGENBUFFERSARBPROC            glGenBuffers               = NULL;
PFNGLGENVERTEXARRAYSPROC          glGenVertexArrays          = NULL;
PFNGLBINDVERTEXARRAYPROC          glBindVertexArray          = NULL;
PFNGLBUFFERDATAPROC               glBufferData               = NULL;
PFNGLSHADERSOURCEPROC             glShaderSource             = NULL;
PFNGLUSEPROGRAMPROC               glUseProgram               = NULL;
PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer      = NULL;
PFNGLLINKPROGRAMPROC              glLinkProgram              = NULL;
PFNGLGETSHADERIVPROC              glGetShaderiv              = NULL;
PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog         = NULL;
PFNGLGETPROGRAMIVPROC             glGetProgramiv             = NULL;
PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog        = NULL;
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
  // Load OpenGL 2.0 and above function pointers
  glAttachShader = (PFNGLATTACHSHADERPROC)(LPVOID)wgl_load_proc("glAttachShader");
  ASSERT_W32(glAttachShader);
  glCompileShader = (PFNGLCOMPILESHADERPROC)(LPVOID)wgl_load_proc("glCompileShader");
  ASSERT_W32(glCompileShader);
  glCreateProgram = (PFNGLCREATEPROGRAMPROC)(LPVOID)wgl_load_proc("glCreateProgram");
  ASSERT_W32(glCreateProgram);
  glCreateShader = (PFNGLCREATESHADERPROC)(LPVOID)wgl_load_proc("glCreateShader");
  ASSERT_W32(glCreateShader);
  glDeleteProgram = (PFNGLDELETEPROGRAMPROC)(LPVOID)wgl_load_proc("glDeleteProgram");
  ASSERT_W32(glDeleteProgram);
  glDeleteShader = (PFNGLDELETESHADERPROC)(LPVOID)wgl_load_proc("glDeleteShader");
  ASSERT_W32(glDeleteShader);
  glDetachShader = (PFNGLDETACHSHADERPROC)(LPVOID)wgl_load_proc("glDetachShader");
  ASSERT_W32(glDetachShader);
  glEnableVertexAttribArray =
    (PFNGLENABLEVERTEXATTRIBARRAYPROC)(LPVOID)wgl_load_proc("glEnableVertexAttribArray");
  ASSERT_W32(glEnableVertexAttribArray);
  glDisableVertexAttribArray =
    (PFNGLDISABLEVERTEXATTRIBARRAYPROC)(LPVOID)wgl_load_proc("glDisableVertexAttribArray");
  ASSERT_W32(glDisableVertexAttribArray);
  glGenBuffers = (PFNGLGENBUFFERSARBPROC)(LPVOID)wgl_load_proc("glGenBuffers");
  ASSERT_W32(glGenBuffers);
  glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)(LPVOID)wgl_load_proc("glGenVertexArrays");
  ASSERT_W32(glGenVertexArrays);
  glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)(LPVOID)wgl_load_proc("glBindVertexArray");
  ASSERT_W32(glBindVertexArray);
  glBufferData = (PFNGLBUFFERDATAPROC)(LPVOID)wgl_load_proc("glBufferData");
  ASSERT_W32(glBufferData);
  glShaderSource = (PFNGLSHADERSOURCEPROC)(LPVOID)wgl_load_proc("glShaderSource");
  ASSERT_W32(glShaderSource);
  glUseProgram = (PFNGLUSEPROGRAMPROC)(LPVOID)wgl_load_proc("glUseProgram");
  ASSERT_W32(glUseProgram);
  glVertexAttribPointer =
    (PFNGLVERTEXATTRIBPOINTERPROC)(LPVOID)wgl_load_proc("glVertexAttribPointer");
  ASSERT_W32(glVertexAttribPointer);
  glLinkProgram = (PFNGLLINKPROGRAMPROC)(LPVOID)wgl_load_proc("glLinkProgram");
  ASSERT_W32(glLinkProgram);
  glGetShaderiv = (PFNGLGETSHADERIVPROC)(LPVOID)wgl_load_proc("glGetShaderiv");
  ASSERT_W32(glGetShaderiv);
  glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)(LPVOID)wgl_load_proc("glGetShaderInfoLog");
  ASSERT_W32(glGetShaderInfoLog);
  glGetProgramiv = (PFNGLGETPROGRAMIVPROC)(LPVOID)wgl_load_proc("glGetProgramiv");
  ASSERT_W32(glGetProgramiv);
  glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)(LPVOID)wgl_load_proc("glGetProgramInfoLog");
  ASSERT_W32(glGetProgramInfoLog);
  glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)(LPVOID)wgl_load_proc("glGetUniformLocation");
  ASSERT_W32(glGetUniformLocation);
  glUniform1f = (PFNGLUNIFORM1FPROC)(LPVOID)wgl_load_proc("glUniform1f");
  ASSERT_W32(glUniform1f);
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
                 PFD_SUPPORT_OPENGL |     // Format Must Support OpenGL
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
  PIXELFORMATDESCRIPTOR pfd = {0};
  //INT                   msaa = w32_wgl_get_pixel_format(16U);
  //INT msaa = w32_wgl_get_pixel_format(1U);
  INT msaa = w32_wgl_get_pixel_format(4U);
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

  (void) memset(g_lpPixelBuf, 0, (size_t)(CAPTURE_REGION*CAPTURE_REGION* 4));
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
    CAPTURE_REGION,
    CAPTURE_REGION,
    0,
    GL_BGRA,
    GL_UNSIGNED_BYTE,
    g_lpPixelBuf
  );
}

const char* vertexShaderSource =
  "#version 330 core\n"
  "layout(location = 0) in vec2 aPos;\n"
  "layout(location = 1) in vec2 aTexCoord;\n"
  "out vec2 TexCoord;\n"
  "void main()\n"
  "{\n"
  "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
  "    TexCoord = aTexCoord;\n"
  "}\n";

const char* fragmentShaderSource =
  "#version 330 core\n"
  "out vec4 FragColor;\n"
  "in vec2 TexCoord;\n"
  "uniform sampler2D texture1;\n"
  "uniform float offset;\n"
  "uniform float brightness;\n"
  "uniform float time;\n"
  "float rand(vec2 co) {\n"
  "    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);\n"
  "}\n"
  "void main()\n"
  "{\n"
  "    float kernel[169] = float[](\n"
  "        1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0,\n"
  "        4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0,\n"
  "        6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0,\n"
  "        4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0,\n"
  "        1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0,\n"
  "        4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0,\n"
  "        6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0,\n"
  "        4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0,\n"
  "        1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0,\n"
  "        4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0,\n"
  "        6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0, 24.0 / 8192.0, 36.0 / 8192.0, 24.0 / 8192.0, 6.0 / 8192.0,\n"
  "        4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0, 16.0 / 8192.0, 24.0 / 8192.0, 16.0 / 8192.0, 4.0 / 8192.0,\n"
  "        1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0, 4.0 / 8192.0, 6.0 / 8192.0, 4.0 / 8192.0, 1.0 / 8192.0\n"
  "    );\n"
  "    vec2 offsets[169] = vec2[](\n"
  "        vec2(-6,  6) * offset, vec2(-5,  6) * offset, vec2(-4,  6) * offset, vec2(-3,  6) * offset, vec2(-2,  6) * offset, vec2(-1,  6) * offset, vec2(0,  6) * offset, vec2(1,  6) * offset, vec2(2,  6) * offset, vec2(3,  6) * offset, vec2(4,  6) * offset, vec2(5,  6) * offset, vec2(6,  6) * offset,\n"
  "        vec2(-6,  5) * offset, vec2(-5,  5) * offset, vec2(-4,  5) * offset, vec2(-3,  5) * offset, vec2(-2,  5) * offset, vec2(-1,  5) * offset, vec2(0,  5) * offset, vec2(1,  5) * offset, vec2(2,  5) * offset, vec2(3,  5) * offset, vec2(4,  5) * offset, vec2(5,  5) * offset, vec2(6,  5) * offset,\n"
  "        vec2(-6,  4) * offset, vec2(-5,  4) * offset, vec2(-4,  4) * offset, vec2(-3,  4) * offset, vec2(-2,  4) * offset, vec2(-1,  4) * offset, vec2(0,  4) * offset, vec2(1,  4) * offset, vec2(2,  4) * offset, vec2(3,  4) * offset, vec2(4,  4) * offset, vec2(5,  4) * offset, vec2(6,  4) * offset,\n"
  "        vec2(-6,  3) * offset, vec2(-5,  3) * offset, vec2(-4,  3) * offset, vec2(-3,  3) * offset, vec2(-2,  3) * offset, vec2(-1,  3) * offset, vec2(0,  3) * offset, vec2(1,  3) * offset, vec2(2,  3) * offset, vec2(3,  3) * offset, vec2(4,  3) * offset, vec2(5,  3) * offset, vec2(6,  3) * offset,\n"
  "        vec2(-6,  2) * offset, vec2(-5,  2) * offset, vec2(-4,  2) * offset, vec2(-3,  2) * offset, vec2(-2,  2) * offset, vec2(-1,  2) * offset, vec2(0,  2) * offset, vec2(1,  2) * offset, vec2(2,  2) * offset, vec2(3,  2) * offset, vec2(4,  2) * offset, vec2(5,  2) * offset, vec2(6,  2) * offset,\n"
  "        vec2(-6,  1) * offset, vec2(-5,  1) * offset, vec2(-4,  1) * offset, vec2(-3,  1) * offset, vec2(-2,  1) * offset, vec2(-1,  1) * offset, vec2(0,  1) * offset, vec2(1,  1) * offset, vec2(2,  1) * offset, vec2(3,  1) * offset, vec2(4,  1) * offset, vec2(5,  1) * offset, vec2(6,  1) * offset,\n"
  "        vec2(-6,  0) * offset, vec2(-5,  0) * offset, vec2(-4,  0) * offset, vec2(-3,  0) * offset, vec2(-2,  0) * offset, vec2(-1,  0) * offset, vec2(0,  0) * offset, vec2(1,  0) * offset, vec2(2,  0) * offset, vec2(3,  0) * offset, vec2(4,  0) * offset, vec2(5,  0) * offset, vec2(6,  0) * offset,\n"
  "        vec2(-6, -1) * offset, vec2(-5, -1) * offset, vec2(-4, -1) * offset, vec2(-3, -1) * offset, vec2(-2, -1) * offset, vec2(-1, -1) * offset, vec2(0, -1) * offset, vec2(1, -1) * offset, vec2(2, -1) * offset, vec2(3, -1) * offset, vec2(4, -1) * offset, vec2(5, -1) * offset, vec2(6, -1) * offset,\n"
  "        vec2(-6, -2) * offset, vec2(-5, -2) * offset, vec2(-4, -2) * offset, vec2(-3, -2) * offset, vec2(-2, -2) * offset, vec2(-1, -2) * offset, vec2(0, -2) * offset, vec2(1, -2) * offset, vec2(2, -2) * offset, vec2(3, -2) * offset, vec2(4, -2) * offset, vec2(5, -2) * offset, vec2(6, -2) * offset,\n"
  "        vec2(-6, -3) * offset, vec2(-5, -3) * offset, vec2(-4, -3) * offset, vec2(-3, -3) * offset, vec2(-2, -3) * offset, vec2(-1, -3) * offset, vec2(0, -3) * offset, vec2(1, -3) * offset, vec2(2, -3) * offset, vec2(3, -3) * offset, vec2(4, -3) * offset, vec2(5, -3) * offset, vec2(6, -3) * offset,\n"
  "        vec2(-6, -4) * offset, vec2(-5, -4) * offset, vec2(-4, -4) * offset, vec2(-3, -4) * offset, vec2(-2, -4) * offset, vec2(-1, -4) * offset, vec2(0, -4) * offset, vec2(1, -4) * offset, vec2(2, -4) * offset, vec2(3, -4) * offset, vec2(4, -4) * offset, vec2(5, -4) * offset, vec2(6, -4) * offset,\n"
  "        vec2(-6, -5) * offset, vec2(-5, -5) * offset, vec2(-4, -5) * offset, vec2(-3, -5) * offset, vec2(-2, -5) * offset, vec2(-1, -5) * offset, vec2(0, -5) * offset, vec2(1, -5) * offset, vec2(2, -5) * offset, vec2(3, -5) * offset, vec2(4, -5) * offset, vec2(5, -5) * offset, vec2(6, -5) * offset,\n"
  "        vec2(-6, -6) * offset, vec2(-5, -6) * offset, vec2(-4, -6) * offset, vec2(-3, -6) * offset, vec2(-2, -6) * offset, vec2(-1, -6) * offset, vec2(0, -6) * offset, vec2(1, -6) * offset, vec2(2, -6) * offset, vec2(3, -6) * offset, vec2(4, -6) * offset, vec2(5, -6) * offset, vec2(6, -6) * offset\n"
  "    );\n"
  "    vec4 color = vec4(0.0);\n"
  "    for (int i = 0; i < 169; i++)\n"
  "    {\n"
  "        color += texture(texture1, TexCoord + offsets[i]) * kernel[i];\n"
  "    }\n"
  "    color *= brightness;\n"
  "    color.a = 1.0; // Set alpha to maximum\n"
  "    // Add noise\n"
  "    //float noise = rand(TexCoord + time) * 0.1;\n"
  "    //color.rgb += noise;\n"
  "    FragColor = color;\n"
  "}\n";

static
CFORCEINLINE
GLuint
w32_create_shader(
  const char* source,
  GLenum      type)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
  }

  return shader;
}

static
CFORCEINLINE
GLuint
w32_create_program(
  const char* vertexSource,
  const char* fragmentSource)
{
  GLuint vertexShader   = w32_create_shader(vertexSource, GL_VERTEX_SHADER);
  GLuint fragmentShader = w32_create_shader(fragmentSource, GL_FRAGMENT_SHADER);
  GLuint shaderProgram  = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  GLint success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return shaderProgram;
}

EXTERN_C
CFORCEINLINE
void
wender(
  w32_window* wnd)
{
  static w32_timer* rt      = NULL;
  static w32_timer* ft      = NULL;
  static w32_timer* cam     = NULL;
  static w32_timer* gdi     = NULL;
  static w32_timer* upload  = NULL;
  static w32_timer* quad    = NULL;
  static w32_timer* swap    = NULL;
  static int        samples = 0;
  static GLuint     shader;
  if(!rt)
  {
    SetWindowDisplayAffinity(wnd->hWnd, WDA_EXCLUDEFROMCAPTURE);
    shader = w32_create_program(vertexShaderSource, fragmentShaderSource);
    rt     = malloc(sizeof(w32_timer));
    ft     = malloc(sizeof(w32_timer));
    cam    = malloc(sizeof(w32_timer));
    gdi    = malloc(sizeof(w32_timer));
    upload = malloc(sizeof(w32_timer));
    quad   = malloc(sizeof(w32_timer));
    swap   = malloc(sizeof(w32_timer));
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
  //for (int i = 0; i < 256; ++i)
  //  for (int j = 0; j < 256; ++j)
  //    g_lpPixelBuf[((j + (i * 256)) * 4) + 3] = 225;   // Alpha is at offset 3
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
    CAPTURE_REGION,
    CAPTURE_REGION,
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
  glUseProgram(shader);
  float offset = 1.0f / 256.0f; // Adjust based on your texture size
  glUniform1f(glGetUniformLocation(shader, "offset"), offset);
  float brightness         = 3.0f; // Adjust to control brightness
  GLint brightnessLocation = glGetUniformLocation(shader, "brightness");
  glUniform1f(brightnessLocation, brightness);
  // Set the time uniform (update this every frame)
  //float time         = (float)255; // Assuming you are using GLFW for timing
  float time         = (float)42536287; // Assuming you are using GLFW for timing
  GLint timeLocation = glGetUniformLocation(shader, "time");
  glUniform1f(timeLocation, time);
  // Set up vertex data (and buffer(s)) and configure vertex attributes
  GLfloat vertices[] = {
    // positions    // texture coords
    -1.0f, -1.0f,  0.0f, 0.0f,
    1.0f, -1.0f,  1.0f, 0.0f,
    1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
  };

  GLuint indices[] = {
    0, 1, 2,
    2, 3, 0
  };
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
  glEnableVertexAttribArray(1);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
  //glBegin(GL_QUADS);
  //glTexCoord2f(0.0f, 0.0f);
  //glVertex2f(-1.0f, -1.0f);           // Bottom-left
  //glTexCoord2f(1.0f, 0.0f);
  //glVertex2f(1.0f, -1.0f);           // Bottom-right
  //glTexCoord2f(1.0f, 1.0f);
  //glVertex2f(1.0f,1.0f);           // Top-right
  //glTexCoord2f(0.0f, 1.0f);
  //glVertex2f(-1.0f,1.0f);           // Top-left
  //glEnd();

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
    double _rt      = w32_timer_elapsed(rt);
    double _ft      = w32_timer_elapsed(ft);
    double _cam     = w32_timer_elapsed(cam);
    double _gdi     = w32_timer_elapsed(gdi);
    double _upload  = w32_timer_elapsed(upload);
    double _quad    = w32_timer_elapsed(quad);
    double _swap    = w32_timer_elapsed(swap);
    char   buf[256] = {0};
    sprintf(
      buf,
      "    rt: %lf\n    ft: %lf\n   cam: %lf\n   gdi: %lf\nupload: %lf\n  quad: %lf\n  swap: %lf\n",
      _rt,
      _ft / samples,
      _cam / samples,
      _gdi / samples,
      _upload / samples,
      _quad / samples,
      _swap / samples
    );
    samples = 0;
    fwrite(buf, 1, strlen(buf), stdout);
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
