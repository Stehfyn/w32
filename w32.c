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
#include <time.h>
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
#define BGR(color) RGB(GetBValue(color), GetGValue(color), GetRValue(color))

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
          return wnd->lpfnWndProcOverride(hWnd, msg, wParam, lParam, wnd->lpfnWndProcHook, wnd->lpUserData);

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
      //static
      ////const MARGINS m = {0,0,1,0};
      //const MARGINS m = {1,1,1,1};
      ////const MARGINS m = {0,0,0,0};
      //(VOID) DwmExtendFrameIntoClientArea(hWnd, &m);
      // Inform the application of the frame change.
      (VOID) SetWindowPos(
        hWnd,
        NULL,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW
      ); 
      //INT backdropType = DWMSBT_TRANSIENTWINDOW;
      //DwmSetWindowAttribute(
      //  hWnd,
      //  DWMWA_SYSTEMBACKDROP_TYPE,
      //  &backdropType,
      //  sizeof(INT)
      //);
      //DWM_WINDOW_CORNER_PREFERENCE cornerPreference = 1;
      //DwmSetWindowAttribute(
      //  hWnd,
      //  DWMWA_WINDOW_CORNER_PREFERENCE,
      //  &cornerPreference,
      //  sizeof(cornerPreference)
      //);
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
    DwmFlush();

    return 1U;
}
void __forceinline ApplyGaussianSmoothing(BYTE* p, int w, int h, int s, float sigma) {
  int kernelRadius = (int)ceil(3 * sigma); // 3-sigma rule for the kernel size
  int kernelSize = 2 * kernelRadius + 1;

  // Generate the 1D Gaussian kernel
  float* kernel = (float*)malloc(kernelSize * sizeof(float));
  float sum = 0.0f;
  for (int i = -kernelRadius; i <= kernelRadius; ++i) {
    kernel[i + kernelRadius] = expf(-0.5f * (i * i) / (sigma * sigma));
    sum += kernel[i + kernelRadius];
  }
  // Normalize the kernel
  for (int i = 0; i < kernelSize; ++i) {
    kernel[i] /= sum;
  }

  // Allocate a temporary buffer
  BYTE* temp = (BYTE*)malloc(s * h);

  // Horizontal pass
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float r = 0, g = 0, b = 0, a = 0;

      for (int k = -kernelRadius; k <= kernelRadius; ++k) {
        int nx = x + k;
        if (nx >= 0 && nx < w) {
          int index = (y * w + nx) * 4;
          float weight = kernel[k + kernelRadius];
          b += weight * p[index];
          g += weight * p[index + 1];
          r += weight * p[index + 2];
          a += weight * p[index + 3];
        }
      }

      int index = (y * w + x) * 4;
      temp[index] = (BYTE)fminf(fmaxf(b, 0), 255);
      temp[index + 1] = (BYTE)fminf(fmaxf(g, 0), 255);
      temp[index + 2] = (BYTE)fminf(fmaxf(r, 0), 255);
      temp[index + 3] = (BYTE)fminf(fmaxf(a, 0), 255);
    }
  }

  // Vertical pass
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float r = 0, g = 0, b = 0, a = 0;

      for (int k = -kernelRadius; k <= kernelRadius; ++k) {
        int ny = y + k;
        if (ny >= 0 && ny < h) {
          int index = (ny * w + x) * 4;
          float weight = kernel[k + kernelRadius];
          b += weight * temp[index];
          g += weight * temp[index + 1];
          r += weight * temp[index + 2];
          a += weight * temp[index + 3];
        }
      }

      int index = (y * w + x) * 4;
      p[index] = (BYTE)fminf(fmaxf(b, 0), 255);
      p[index + 1] = (BYTE)fminf(fmaxf(g, 0), 255);
      p[index + 2] = (BYTE)fminf(fmaxf(r, 0), 255);
      //p[index + 3] = (BYTE)fminf(fmaxf(a, 0), 255);
      p[index + 3] = 255;
    }
  }

  // Free resources
  free(kernel);
  free(temp);
}
void __forceinline ApplyBlurWithGrayscaleNoise(BYTE* p, int w, int h, int s, int rr, int noiseLevel) {
  BYTE* temp = (BYTE*)malloc(s * h);
  memcpy(temp, p, s * h);

  // Seed the random number generator
  srand((unsigned int)time(NULL));

  // Horizontal blur with grayscale noise
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int r = 0, g = 0, b = 0, count = 0;

      for (int kx = -rr; kx <= rr; ++kx) {
        int nx = x + kx;
        if (nx >= 0 && nx < w) {
          int index = (y * w + nx) * 4;
          b += p[index];
          g += p[index + 1];
          r += p[index + 2];
          count++;
        }
      }

      int index = (y * w + x) * 4;

      // Generate grayscale noise (same value for R, G, B)
      int noise = (rand() % (2 * noiseLevel + 1)) - noiseLevel;

      // Apply the noise equally to all channels
      BYTE grayNoise =  (BYTE)noise;
      temp[index]     = (BYTE)(b / count) + (BYTE)noise;
      temp[index + 1] = (BYTE)(g / count) + (BYTE)noise;
      temp[index + 2] = (BYTE)(r / count) + (BYTE)noise;
      temp[index + 3] = grayNoise; // Preserve alpha
    }
  }

  // Vertical blur with grayscale noise
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int r = 0, g = 0, b = 0, count = 0;

      for (int ky = -rr; ky <= rr; ++ky) {
        int ny = y + ky;
        if (ny >= 0 && ny < h) {
          int index = (ny * w + x) * 4;
          b += temp[index];
          g += temp[index + 1];
          r += temp[index + 2];
          count++;
        }
      }

      int index = (y * w + x) * 4;

      // Generate grayscale noise (same value for R, G, B)
      //int noise = (rand() % (2 * noiseLevel + 1)) - noiseLevel;

      // Apply the noise equally to all channels
      //BYTE grayNoise = (BYTE)noise;
      //BYTE alpha = (temp[index + 3] ^ grayNoise);
      temp[index]     = (BYTE)(b / count);
      temp[index + 1] = (BYTE)(g / count);
      temp[index + 2] = (BYTE)(r / count);
      p[index + 3] = 255; 
    }
  }

  free(temp);
}

void __forceinline ApplyBlurWithNoise(BYTE* p, int w, int h, int s, int rr, int noiseLevel) {
  BYTE* temp = (BYTE*)malloc(s * h);
  memcpy(temp, p, s * h);

  // Seed the random number generator
  srand((unsigned int)time(NULL));

  // Horizontal blur with noise
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int r = 0, g = 0, b = 0, count = 0;

      for (int kx = -rr; kx <= rr; ++kx) {
        int nx = x + kx;
        if (nx >= 0 && nx < w) {
          int index = (y * w + nx) * 4;
          b += p[index];
          g += p[index + 1];
          r += p[index + 2];
          count++;
        }
      }

      int index = (y * w + x) * 4;

      // Add random noise to the blurred result
      int noiseB = (rand() % (2 * noiseLevel + 1)) - noiseLevel;
      int noiseG = (rand() % (2 * noiseLevel + 1)) - noiseLevel;
      int noiseR = (rand() % (2 * noiseLevel + 1)) - noiseLevel;

      temp[index] = (BYTE)(b / count + noiseB);
      temp[index + 1] = (BYTE)(g / count + noiseG);
      temp[index + 2] = (BYTE)(r / count + noiseR);
      temp[index + 3] = (BYTE)(p[index + 3]); // Preserve alpha
    }
  }

  // Vertical blur with noise
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int r = 0, g = 0, b = 0, count = 0;

      for (int ky = -rr; ky <= rr; ++ky) {
        int ny = y + ky;
        if (ny >= 0 && ny < h) {
          int index = (ny * w + x) * 4;
          b += temp[index];
          g += temp[index + 1];
          r += temp[index + 2];
          count++;
        }
      }

      int index = (y * w + x) * 4;

      // Add random noise to the blurred result
      int noiseB = (rand() % (2 * noiseLevel + 1)) - noiseLevel;
      int noiseG = (rand() % (2 * noiseLevel + 1)) - noiseLevel;
      int noiseR = (rand() % (2 * noiseLevel + 1)) - noiseLevel;

      p[index] = (BYTE)(b / count + noiseB);
      p[index + 1] = (BYTE)(g / count + noiseG);
      p[index + 2] = (BYTE)(r / count + noiseR);
      p[index + 3] = (BYTE)(temp[index + 3]); // Preserve alpha
    }
  }

  free(temp);
}
void __forceinline ApplyBlur(BYTE* p, int w, int h, int s, int rr) {

  BYTE* temp = (BYTE*)malloc(s * h);
  memcpy(temp, p, s * h);
  // Horizontal blur
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int r = 0, g = 0, b = 0, count = 0;

      for (int kx = -rr; kx <= rr; ++kx) {
        int nx = x + kx;
        if (nx >= 0 && nx < w) {
          int index = (y * w + nx) * 4;
          b += p[index];
          g += p[index + 1];
          r += p[index + 2];
          count++;
        }
      }

      int index = (y * w + x) * 4;
      temp[index] = (BYTE)(b / count);
      temp[index + 1] = (BYTE)(g / count);
      temp[index + 2] = (BYTE)(r / count);
      temp[index + 3] = (BYTE)(p[index + 3]); // Preserve alpha
    }
  }

  // Vertical blur
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int r = 0, g = 0, b = 0, count = 0;

      for (int ky = -rr; ky <= rr; ++ky) {
        int ny = y + ky;
        if (ny >= 0 && ny < h) {
          int index = (ny * w + x) * 4;
          b += temp[index];
          g += temp[index + 1];
          r += temp[index + 2];
          count++;
        }
      }

      int index = (y * w + x) * 4;
      p[index] = (BYTE)(b / count);
      p[index + 1] = (BYTE)(g / count);
      p[index + 2] = (BYTE)(r / count);
      p[index + 3] = (BYTE)(temp[index + 3]); // Preserve alpha
    }
  }

  free(temp);
}

void __forceinline ApplyTint(BYTE* p, int w, int h,BYTE tintR, BYTE tintG, BYTE tintB, float tintStrength) {
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int index = (y * w + x) * 4;

      p[index] =     (BYTE)(p[index] *     (1.0f - tintStrength) + tintB * tintStrength);
      p[index + 1] = (BYTE)(p[index + 1] * (1.0f - tintStrength) + tintG * tintStrength);
      p[index + 2] = (BYTE)(p[index + 2] * (1.0f - tintStrength) + tintR * tintStrength);
      // Alpha remains unchanged
    }
  }
}
void __forceinline
capture_screen(HWND hWnd, HDC hdc, DWORD dwmColor)
{
  HBITMAP hBitmapBg = NULL;
  HWND hDesktopWnd = GetDesktopWindow();
  HDC  hDesktopDC = GetDC(hDesktopWnd);
  HDC  hCaptureDC = CreateCompatibleDC(hDesktopDC);
  RECT r;
  GetWindowRect(hWnd, &r);
  int w = labs(r.right - r.left);
  int h = labs(r.bottom - r.top);
  hBitmapBg = CreateCompatibleBitmap(
    hDesktopDC,
    labs(r.right - r.left),
    labs(r.bottom - r.top)
  );
  if (hBitmapBg)
  {

    BITMAPINFO bmi = { 0 };
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = w;
    bi.biHeight = -h; // Negative to flip vertically for OpenGL
    bi.biPlanes = 1;
    bi.biBitCount = 32; // RGBA
    bi.biCompression = BI_RGB;
    bmi.bmiHeader = bi;
    SelectObject(hCaptureDC, hBitmapBg);
    BitBlt(
      hCaptureDC,
      0,
      0,
      w,
      h,
      hDesktopDC,
      r.left,
      r.top,
      SRCCOPY | CAPTUREBLT
    );

    BYTE* pixels = malloc(4*w*h);
    GetDIBits(hCaptureDC, hBitmapBg, 0, h, pixels, &bmi, DIB_RGB_COLORS);
    ApplyGaussianSmoothing(pixels, w, h, 4 * w, 42.5f);
    FillRect(hCaptureDC, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, 0 };
    BLENDFUNCTION blend2 = { AC_SRC_OVER, 0, 255, 0};
    AlphaBlend(hCaptureDC, 0, 0, r.right, r.bottom, hCaptureDC, 0, 0, r.right, r.bottom, blend);
    ApplyTint(pixels, w, h, GetRValue(dwmColor), GetGValue(dwmColor), GetBValue(dwmColor),0.3f);
    FillRect(hCaptureDC, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
    AlphaBlend(hCaptureDC, 0, 0, r.right, r.bottom, hCaptureDC, 0, 0, r.right, r.bottom, blend2);
    ApplyBlurWithGrayscaleNoise(pixels, w, h, 4*w, 70, 5);
    SetDIBits(hCaptureDC, hBitmapBg, 0, h, pixels, &bmi, DIB_RGB_COLORS);
    free(pixels);
    SelectObject(hdc, hBitmapBg);
    BitBlt(
      hdc,
      0,
      0,
      w,
      h,
      hCaptureDC,
      0,
      0,
      SRCCOPY
    );
  }
  ReleaseDC(hDesktopWnd, hDesktopDC);
  DeleteDC(hCaptureDC);
  DeleteObject(hBitmapBg);
}
void __forceinline DeflateRectByPixels(RECT* rect, int nPixels) {
  rect->left   += nPixels;
  rect->top    += nPixels;
  rect->right  -= nPixels;
  rect->bottom -= nPixels;
}
VOID CFORCEINLINE
on_paint(
    HWND hWnd)
{
  SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    DWORD dwmColor = 0;
    BOOL isOpaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&dwmColor, &isOpaque))) {
      COLORREF color = BGR(dwmColor);

    capture_screen(hWnd, hdc, color);
      HDC memDC = CreateCompatibleDC(hdc);
      HBITMAP memBitmap = CreateCompatibleBitmap(hdc, ps.rcPaint.right, ps.rcPaint.bottom);
      HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);
      //BITMAPINFO bmi = { 0 };
      //bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      //bmi.bmiHeader.biWidth = labs(ps.rcPaint.right - ps.rcPaint.left);
      //bmi.bmiHeader.biHeight = -labs(ps.rcPaint.bottom - ps.rcPaint.top); // Negative for top-down DIB
      //bmi.bmiHeader.biPlanes = 1;
      //bmi.bmiHeader.biBitCount = 32;
      //bmi.bmiHeader.biCompression = BI_RGB;
      //RGBQUAD bitmapBits = { 0x00, 0, 0, 0xFF };
      //
      //StretchDIBits(memDC, 0, 0, labs(ps.rcPaint.right - ps.rcPaint.left), labs(ps.rcPaint.bottom - ps.rcPaint.top),
      //  0, 0, 1, 1, &bitmapBits, &bmi,
      //  DIB_RGB_COLORS, SRCPAINT);
      //
      ////SelectObject(hCaptureDC, hBitmapBg);
      //BitBlt(
      //  hdc,
      //  0,
      //  0,
      //  labs(ps.rcPaint.right - ps.rcPaint.left),
      //  labs(ps.rcPaint.bottom - ps.rcPaint.top),
      //  memDC,
      //  0,
      //  0,
      //  SRCCOPY
      //);
      //StretchDIBits(hdc, 0, 0, labs(ps.rcPaint.right - ps.rcPaint.left), labs(ps.rcPaint.bottom - ps.rcPaint.top),
      //  0, 0, 1, 1, &bitmapBits, &bmi,
      //  DIB_RGB_COLORS, SRCPAINT);
      // Fill the memory DC with the DWM color
      //HBRUSH brush = CreateSolidBrush(color);
      //FillRect(memDC, &ps.rcPaint, brush);
      //FillRect(memDC, &ps.rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH));
      ////DeleteObject(brush);
      //// Blend the memory DC with the main DC using AlphaBlend
      //BLENDFUNCTION blend = { AC_SRC_OVER, 0, 16, 0};
      //AlphaBlend(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom,
      //  memDC, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, blend);
      
      // Cleanup
      SelectObject(memDC, oldBitmap);
      DeleteObject(memBitmap);
      DeleteObject(oldBitmap);
      DeleteDC(memDC);
      SetWindowDisplayAffinity(hWnd, WDA_NONE);
      GdiFlush();

    }
RECT r;
if(GetClientRect(hWnd, &r))
{

  RECT rcPaint;
  rcPaint.left = r.left;
  rcPaint.top = r.top;
  rcPaint.right = r.left + 300;
  rcPaint.bottom = r.top + 100;
  //int width = rcPaint.right - rcPaint.left;
  //int height = rcPaint.bottom - rcPaint.top;
  CHAR text[] = "w32_demo";
  // Dimensions for the temporary bitmap
  const int width = 300;
  const int height = 100;

  // Create a memory buffer for the DIB
  RGBQUAD pixels[300 * 100] = { 0 };

  // Create a BITMAPINFO structure for the DIB
  BITMAPINFO bmi = { 0 };
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height; // Negative height for top-down DIB
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32; // 32 bits per pixel (supports alpha)
  bmi.bmiHeader.biCompression = BI_RGB;

  // Create a compatible DC for text rendering
  HDC memDC = CreateCompatibleDC(hdc);
  HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
  HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

  // Fill the memory DC with black
  SetBkColor(memDC, RGB(0, 0, 0));
  PatBlt(memDC, 0, 0, width, height, BLACKNESS);

  // Set text rendering attributes
  SetTextColor(memDC, RGB(255, 255, 255)); // White text
  SetBkMode(memDC, TRANSPARENT);

  // Draw the text into the memory DC
  TextOutA(memDC, 4, 4, text, sizeof(text));

  // Copy the rendered text into the DIB buffer
  GetDIBits(memDC, hBitmap, 0, height, pixels, &bmi, DIB_RGB_COLORS);

  // Apply alpha blending to the text color
  //for (int i = 0; i < width * height; i++) {
  //  if (pixels[i].rgbRed == 255 && pixels[i].rgbGreen == 255 && pixels[i].rgbBlue == 255) {
  //    pixels[i].rgbRed = 0;
  //    pixels[i].rgbGreen = 0;
  //    pixels[i].rgbBlue = 0;
  //    pixels[i].rgbReserved = 255; // Transparent background
  //  }
  //  else {
  //    //pixels[i].rgbRed =255;
  //    //pixels[i].rgbGreen = 255;
  //    //pixels[i].rgbBlue = 255;
  //    //pixels[i].rgbReserved = 255; // Set the alpha
  //    pixels[i].rgbRed = 0;
  //    pixels[i].rgbGreen = 0;
  //    pixels[i].rgbBlue = 0;
  //    pixels[i].rgbReserved = 0; // Set the alpha
  //  }
  //}

  // Use StretchDIBits to draw the DIB with alpha blending
  StretchDIBits(
    hdc,
    0, 0, width, height,
    0, 0, width, height,
    pixels,
    &bmi,
    DIB_RGB_COLORS,
    SRCPAINT
    //SRCCOPY
  );
  StretchDIBits(
    hdc,
    0, 0, width, height,
    0, 0, width, height,
    pixels,
    &bmi,
    DIB_RGB_COLORS,
    SRCINVERT
    //SRCCOPY
  );
  //SRCINVERT
  // Cleanup
  SelectObject(memDC, oldBitmap);
  DeleteObject(hBitmap);
  DeleteDC(memDC);
}
    EndPaint(hWnd, &ps);
    //return 0;
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
    HANDLE_MSG(hWnd, WM_PAINT,      on_paint);
    //HANDLE_MSG(hWnd, WM_ERASEBKGND, on_erasebkgnd);
    HANDLE_MSG(hWnd, WM_DESTROY,    on_destroy);
    //case (WM_ENTERSIZEMOVE): {
    //  (void) SetTimer(hWnd, (UINT_PTR)0, USER_TIMER_MINIMUM, NULL);
    //  return 0;
    //}
    case (WM_CREATE): {
      (void)SetTimer(hWnd, (UINT_PTR)0, 15*USER_TIMER_MINIMUM, NULL);
    }
    case (WM_TIMER): {
      DwmFlush();
      RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
      return 0;
    }
    //case (WM_EXITSIZEMOVE): {
    //  (void) KillTimer(hWnd, (UINT_PTR)0);
    //  return 0;
    //}
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
