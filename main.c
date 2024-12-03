#include "w32.h"
#include <stdio.h>

int
wmain(
  void
);

int
main(
  void)
{
  return wmain();
}

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
  return wmain();
}

int
wmain(
  void)
{
  //w32_user_state us     = {0};
  BOOL       result = FALSE;
  w32_window wnd    = {0};
  POINT      start  = {0};
  SIZE       sz     = {600, 340};
  //(VOID) w32_get_user_state(&us);
  //start = us.cursor;
  GetCursorPos(&start);
  //printf("%ls :%zu\n", (const wchar_t* const)&us.imageName[0], us.piActiveProcessID);
  (VOID) w32_set_process_dpiaware();
  //(VOID) w32_adjust_window_start_point(&start);
  (VOID) w32_get_centered_window_point(&start, &sz);

  result =
    w32_create_window(
      &wnd,
      _T("w32_demo"),
      w32_create_window_class(
        _T("w32_demo_class"),
        CS_VREDRAW |
        CS_HREDRAW |
        CS_OWNDC
      ),
      start.x,
      start.y,
      sz.cx,
      sz.cy,
      //WS_EX_APPWINDOW,
      0,
      WS_POPUP|WS_SYSMENU|WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
      w32_borderless_wndproc,
      NULL,
      NULL
    );

  //(VOID) w32_set_alpha_composition(&wnd, TRUE);

  if (result)
  {
    for(;;)
      if(!w32_pump_message_loop(&wnd, TRUE))
        break;
    //w32_run_message_loop(&wnd, TRUE);
  }

  return 0;
}
