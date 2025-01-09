#include "w32.h"

int APIENTRY
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

    BOOL          bResult = FALSE;
    w32_window    wnd = { 0 };
    POINT         start = { 0 };
    SIZE          sz = { 600, 340 };
    LARGE_INTEGER dueTime = { 0 };
    __int64       TIMEOUT = 30'0;
    __int64       COEFF = -100'0;
    RequestSystemDpiAutonomy();
    GetCursorPos(&start);
    GetWindowCenteredPoint(&start, &sz);
    dueTime.QuadPart = TIMEOUT * COEFF;
    bResult = w32_create_window(
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
      WndProc,
      NULL,
      NULL
    );

    ShowWindow(wnd.hWnd, SW_SHOWDEFAULT);
    UpdateWindow(wnd.hWnd);

    SetAlphaComposition(&wnd, TRUE);
    
    if (bResult)
    {
      bResult = SetSystemTimerResolution((ULONG)(MILLISECONDS_TO_100NANOSECONDS(0.5)), TRUE, NULL);
      if (bResult) {
        HANDLE hTimer = CreateHighResolutionTimer(NULL, NULL, TIMER_ALL_ACCESS);
        if (hTimer)
          for (;;) {
            (void) SetWaitableTimerEx(hTimer, &dueTime, 0, 0, 0, NULL, 0);
            if (!PumpMessageQueue(NULL))
              break;
            //Sleep(15);
            (void) WaitForSingleObjectEx(hTimer, INFINITE, TRUE);
          }
      }
      //#endif
    }
    return 0;
}