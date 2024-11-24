#include "w32.h"

int
main(
  void)
{
  (VOID) w32_set_process_dpiaware();

  w32_window wnd           = {0};
  LPCTSTR    lpszClassName = 
    w32_create_window_class(
      _T("w32_demo_class"),
      CS_VREDRAW |
      CS_HREDRAW |
      CS_OWNDC
    );

  BOOL result = 
    w32_create_window(
      &wnd,
      _T("w32_demo"), 
      lpszClassName,
      0,
      0,
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