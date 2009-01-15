
#ifndef VISUAL_H
#define VISUAL_H

#include "task.h"

void visual_refresh ();
void redraw_task (Task *tsk);
void redraw_clock ();
void visual_draw_launcher ();
void draw_panel_background ();
void resize_clock(int width);
void resize_taskbar();
      
#endif

