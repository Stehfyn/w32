#include "w32.h"

int
main(
  void)
{
  BOOL       result = FALSE;
  w32_window wnd    = {0};
  POINT      start  = {0L, 0L};
  LPCTSTR    lpszClassName = 
    w32_create_window_class(
      _T("w32_demo_class"),
      CS_VREDRAW |
      CS_HREDRAW |
      CS_OWNDC
    );

  (VOID) w32_set_process_dpiaware();
  (VOID) w32_adjust_window_start_point(&start);

  result = 
    w32_create_window(
      &wnd,
      _T("w32_demo"), 
      lpszClassName,
      start.x,
      start.y,
      1080,
      720,
      WS_EX_APPWINDOW,
      WS_OVERLAPPEDWINDOW,
      NULL,
      NULL
    );

  (VOID) w32_set_alpha_composition(&wnd, TRUE);

  if (result)
  {
    w32_run_message_loop(&wnd, TRUE);
  }

  return 0;
}
