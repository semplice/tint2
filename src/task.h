
#ifndef TASK_H
#define TASK_H

#include <X11/Xlib.h>


typedef struct Task {
   struct Task *next;
   Window win;
   long *icon_data;
   int icon_width;
   int icon_height;
   Pixmap pmap;
   Pixmap active_pmap;
   // need redraw Pixmap
   int redraw;
   char *title;
   int id_taskbar;
   int posx;
   int width;
} Task;


typedef struct Taskbar {
   Task *tasklist;
   int nbTask;
   int posx;
   int width;

   // task parameters
   int task_width;
   int task_modulo;
   int text_width;
} Taskbar;



void add_task (Window win);
void remove_task (Task *tsk);
Task *task_get_task (Window win);
void set_redraw_all ();
void set_redraw_taskbar (int id_taskbar);
void task_refresh_tasklist ();
// return 1 if task_width changed
int resize_tasks (int id_taskbar);

#endif
