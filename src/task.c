/**************************************************************************
*
* Tint Task Manager
* 
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
* 
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Soft-
* ware"), to deal in the Software without restriction, including without
* limitation the rights to use, copy, modify, merge, publish, distribute,
* sublicense, and/or sell copies of the Software, and to permit persons to
* whom the Software is furnished to do so, subject to the following condi-
* tions:
* 
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
* ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
* OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
* 
**************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "task.h"
#include "server.h"
#include "window.h"
#include "config.h"


void add_task (Window win)
{
   Task *new_tsk, *t;
   int desktop, monitor;

   if (!win || window_is_hidden (win) || win == window.main_win) return;

   new_tsk = calloc(1, sizeof(Task));
   new_tsk->win = win;
   window_get_icon (new_tsk);
   window_get_title(new_tsk);
   new_tsk->redraw = 1;
   
   desktop = window_get_desktop (new_tsk->win);
   if (panel.mode == MULTI_MONITOR) monitor = window_get_monitor (new_tsk->win);
   else monitor = 0;
   new_tsk->id_taskbar = index(desktop, monitor);
   //printf("task %s : desktop %d, monitor %d\n", new_tsk->title, desktop, monitor);
   
   XSelectInput (server.dsp, new_tsk->win, PropertyChangeMask|StructureNotifyMask);
   
   if (desktop == 0xFFFFFFFF) {
      XFree (new_tsk->title);
      free(new_tsk);
      fprintf(stderr, "task on all desktop : ignored\n");
      return;
   }
   
   /* Append */
   panel.taskbar[new_tsk->id_taskbar].nbTask++;
   for (t = panel.taskbar[new_tsk->id_taskbar].tasklist; t ; t = t->next)
      if (!t->next) break;
   
   if (!t) panel.taskbar[new_tsk->id_taskbar].tasklist = new_tsk;
   else t->next = new_tsk;

   if (resize_tasks (new_tsk->id_taskbar)) set_redraw_taskbar (new_tsk->id_taskbar);
}


void remove_task (Task *tsk)
{
   Task *t, *p;

   if (!tsk) return;
   
   for (p = 0, t = panel.taskbar[tsk->id_taskbar].tasklist; t ; p = t, t = t->next)
      if (t == tsk) break;
   if (!t) return;
   //printf("delete task %s, nbTask %d\n", tsk->title, panel.taskbar[tsk->id_taskbar].nbTask);
   
   panel.taskbar[tsk->id_taskbar].nbTask--;

   if (!p) panel.taskbar[tsk->id_taskbar].tasklist = t->next;
   else p->next = t->next;

   // if (size doesn't change) redraw only the following tasks
   if (resize_tasks (tsk->id_taskbar)) 
      set_redraw_taskbar (tsk->id_taskbar);
   else 
      for (p = t->next; p ; p = p->next) p->redraw = 1;

   XFree (t->title);
   if (t->icon_data != 0) XFree (t->icon_data);
   XFreePixmap (server.dsp, t->pmap);
   XFreePixmap (server.dsp, t->active_pmap);
   free(t);
}


Task *task_get_task (Window win)
{
   Task *tsk;
   int i, nb;
   
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      for (tsk = panel.taskbar[i].tasklist; tsk ; tsk = tsk->next) {
         if (win == tsk->win) return tsk;
      }
   }
   //printf("task_get_task return 0\n");
   return 0;
}


void set_redraw_all ()
{
   int i, nb;
   
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) set_redraw_taskbar (i);
}


void set_redraw_taskbar (int id_taskbar)
{
   Task *tsk;
   
   for (tsk = panel.taskbar[id_taskbar].tasklist; tsk ; tsk = tsk->next) tsk->redraw = 1;
}


void task_refresh_tasklist ()
{
   Task *tsk, *next;
   Window *win, active_win;
   int num_results, i, j, nb;

   win = server_get_property (server.root_win, server.atom._NET_CLIENT_LIST, XA_WINDOW, &num_results);

   if (!win) return;
   
   /* Remove any old and set active win */
   active_win = window_get_active ();

   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      tsk = panel.taskbar[i].tasklist;
      while (tsk) {
         if (tsk->win == active_win) panel.task_active = tsk;
         
         next = tsk->next;
         for (j = 0; j < num_results; j++) {
            if (tsk->win == win[j]) break;
         }
         if (tsk->win != win[j]) remove_task (tsk);

         tsk = next;
      }
   }
   
   /* Add any new */
   for (i = 0; i < num_results; i++) {
      if (!task_get_task (win[i])) add_task (win[i]);
   }

   XFree (win);
}


int resize_tasks (int id_taskbar)
{
   int ret, task_count, pixel_width, modulo_width=0;
   int x, taskbar_width;
   Task *t;

   // new task width for 'desktop'
   task_count = panel.taskbar[id_taskbar].nbTask;
   if (!task_count) pixel_width = panel.task_maximum_width;
   else {
      taskbar_width = panel.taskbar[id_taskbar].width - ((task_count-1) * panel.task_margin);

      pixel_width = taskbar_width / task_count;
      if (pixel_width > panel.task_maximum_width) pixel_width = panel.task_maximum_width;
      else modulo_width = taskbar_width % task_count;
   }

   if ((panel.taskbar[id_taskbar].task_width == pixel_width) && (panel.taskbar[id_taskbar].task_modulo == modulo_width)) {
      ret = 0;
   }
   else {
      ret = 1;
      panel.taskbar[id_taskbar].task_width = pixel_width;
      panel.taskbar[id_taskbar].task_modulo = modulo_width;
      panel.taskbar[id_taskbar].text_width = pixel_width - panel.task_text_posx - panel.task_border.width - panel.task_padding;
   }
      
   // change pos_x and width for all tasks
   x = panel.taskbar[id_taskbar].posx;
   for (t = panel.taskbar[id_taskbar].tasklist; t ; t = t->next) {
      t->posx = x;
      t->width = pixel_width;
      if (modulo_width) {
         t->width++;
         modulo_width--;
      }

      x += t->width + panel.task_margin;
   }
   return ret;
}


