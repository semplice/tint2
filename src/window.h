
#ifndef WINDOW_H
#define WINDOW_H

#include "task.h"

typedef struct window_global
{
   Window main_win;
} window_global;

window_global window;

void set_active (Window win);
void set_desktop (int desktop);
void set_close (Window win);
int server_get_current_desktop ();
int server_get_number_of_desktop ();
int window_is_iconified (Window win);
int window_is_hidden (Window win);
int window_is_active (Window win);
void window_action (Task *tsk, int action);
void window_toggle_shade (Window win);
void window_draw_panel ();
int window_get_desktop (Window win);
void windows_set_desktop (Window win, int desktop);
int window_get_monitor (Window win);
Window window_get_active ();
void window_get_icon (Task *tsk);
void window_get_title(Task *tsk);

#endif
