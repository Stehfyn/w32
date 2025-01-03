#include "w32.h"

int
APIENTRY
WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd)
{
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nShowCmd);

  BOOL          result = FALSE;
  w32_window    wnd = { 0 };
  POINT         start = { 0 };
  SIZE          sz = { 600, 340 };
  LARGE_INTEGER dueTime = { 0 };
  //__int64       TIMEOUT = 16'7;
  __int64       TIMEOUT = 30'0;
  //__int64       TIMEOUT = 8'0;
  __int64       COEFF = -100'0;
  (void)w32_set_process_dpiaware();
  (void)GetCursorPos(&start);
  (void)w32_get_centered_window_point(&start, &sz);
  dueTime.QuadPart = TIMEOUT * COEFF;
  //(void) w32_wgl_get_pixel_format(4U);
  result = w32_create_window(
    &wnd,
    _T("main"),
    w32_create_window_class(
      _T("w32_demo_class"),
      _T("res\\checkerboard.ico"),
      CS_VREDRAW | CS_HREDRAW | CS_OWNDC
    ),
    start.x,
    start.y,
    sz.cx,
    sz.cy,
    WS_EX_APPWINDOW,
    WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
    w32_borderless_wndproc,
    NULL,
    NULL
  );
  ShowWindow(wnd.hWnd, SW_SHOWDEFAULT);
  UpdateWindow(wnd.hWnd);
  w32_wgl_attach_device(&wnd);

  (void)w32_set_alpha_composition(&wnd, TRUE);
  if (result)
  {
    //#ifdef _DEBUG
    //w32_run_message_loop(&wnd, NULL);
    //#else
    result = w32_set_timer_resolution((ULONG)(MILLISECONDS_TO_100NANOSECONDS(0.5)), TRUE, NULL);
    if (result) {
      HANDLE hTimer = w32_create_high_resolution_timer(NULL, NULL, TIMER_ALL_ACCESS);
      if (hTimer)
        for (;;) {
          (void) SetWaitableTimerEx(hTimer, &dueTime, 0, 0, 0, NULL, 0);
          if (!w32_pump_message_loop(NULL))
            break;
          //Sleep(15);
          (void) WaitForSingleObjectEx(hTimer, INFINITE, TRUE);
        }
    }
    //#endif
  }
  return 0;
}