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

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <Imlib2.h>
#include <signal.h>
#include "server.h"
#include "visual.h"
#include "config.h"
#include "task.h"
#include "window.h"


void signal_handler(int sig)
{
	// signal handler is light as it should be
	panel.signal_pending = sig;
}


void init ()
{
	// Set signal handler
   signal(SIGUSR1, signal_handler);
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);

   memset(&panel, 0, sizeof(Panel));
   memset(&server, 0, sizeof(server_global));
   window.main_win = 0;
   
   server.dsp = XOpenDisplay (NULL);
   if (!server.dsp) {
      fprintf(stderr, "Could not open display.\n");
      exit(0);
   }
   server_init_atoms ();
   server.screen = DefaultScreen (server.dsp);
   server.root_win = RootWindow (server.dsp, server.screen);   
   server.depth = DefaultDepth (server.dsp, server.screen);
   server.visual = DefaultVisual (server.dsp, server.screen);
   server.desktop = server_get_current_desktop ();

   XSetErrorHandler ((XErrorHandler) server_catch_error);

   imlib_context_set_display (server.dsp);
   imlib_context_set_visual (server.visual);
   imlib_context_set_colormap (DefaultColormap (server.dsp, server.screen));

   /* Catch events */
   XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);

   setlocale(LC_ALL, "");
}


void event_button_press (int x, int y)
{
   Task *tsk;
   int i, nb;
   
   if (panel.mode == SINGLE_DESKTOP) {
      // drag and drop disabled
      XLowerWindow (server.dsp, window.main_win);
      return;
   }
   
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      if (x >= panel.taskbar[i].posx && x <= (panel.taskbar[i].posx + panel.taskbar[i].width))
         break;
   }
   
   if (i != nb) {
      for (tsk = panel.taskbar[i].tasklist ; tsk ; tsk = tsk->next) {
         if (x >= tsk->posx && x <= tsk->posx + tsk->width) {
            panel.task_drag = tsk;
            break;
         }
      }
   }
   XLowerWindow (server.dsp, window.main_win);
}


void event_button_release (int button, int x, int y)
{
   Task *tsk;
   int desktop, monitor, id, action = TOGGLE_ICONIFY;

   switch (button) {
      case 2:
         action = panel.mouse_middle;
         break;
      case 3:
         action = panel.mouse_right;
         break;
      case 4:
         action = panel.mouse_scroll_up;
         break;
      case 5:
         action = panel.mouse_scroll_down;
         break;
   }

   // search taskbar
   for (desktop=0 ; desktop < panel.nb_desktop ; desktop++) {
      if (panel.mode != MULTI_DESKTOP && desktop != server.desktop) continue;
      
      for (monitor=0 ; monitor < panel.nb_monitor ; monitor++) {
         id = index(desktop, monitor);
         if (x >= panel.taskbar[id].posx && x <= (panel.taskbar[id].posx + panel.taskbar[id].width))
            goto suite;
      }
   }

   // TODO: check better solution to keep window below
   XLowerWindow (server.dsp, window.main_win);
   panel.task_drag = 0;
   return;

suite:
   // drag and drop task
   if (panel.task_drag) {
      if (id != panel.task_drag->id_taskbar && action == TOGGLE_ICONIFY) {
         windows_set_desktop(panel.task_drag->win, desktop);
         if (desktop == server.desktop) 
            set_active(panel.task_drag->win);
         panel.task_drag = 0;
         return;
      }
      else panel.task_drag = 0;
   }
   
   // switch desktop
   if (panel.mode == MULTI_DESKTOP)
      if (desktop != server.desktop && action != CLOSE)
         set_desktop (desktop);

   // action on task
   for (tsk = panel.taskbar[id].tasklist ; tsk ; tsk = tsk->next) {
      if (x >= tsk->posx && x <= tsk->posx + tsk->width) {
         window_action (tsk, action);
         break;
      }
   }
   
   // to keep window below
   XLowerWindow (server.dsp, window.main_win);
}


void event_property_notify (Window win, Atom at)
{   
   
   if (win == server.root_win) {
      if (!server.got_root_win) {
         XSelectInput (server.dsp, server.root_win, PropertyChangeMask|StructureNotifyMask);
         server.got_root_win = 1;
      }

      /* Change number of desktops */
      else if (at == server.atom._NET_NUMBER_OF_DESKTOPS) {
         config_taskbar();
         set_redraw_all ();
         panel.refresh = 1;
      }
      /* Change desktop */
      else if (at == server.atom._NET_CURRENT_DESKTOP) {
         server.desktop = server_get_current_desktop ();
         if (panel.mode != MULTI_DESKTOP) panel.refresh = 1;
      }
      /* Window list */
      else if (at == server.atom._NET_CLIENT_LIST) {
         task_refresh_tasklist ();
         panel.refresh = 1;
      }
      /* Active */
      else if (at == server.atom._NET_ACTIVE_WINDOW) {
         Window w1 = window_get_active ();
         Task *t = task_get_task(w1);
         if (t) panel.task_active = t;
         else {
            Window w2;
            if (XGetTransientForHint(server.dsp, w1, &w2) != 0)
               if (w2) panel.task_active = task_get_task(w2);
         }
         panel.refresh = 1;
      }
      /* Wallpaper changed */
      else if (at == server.atom._XROOTPMAP_ID) {
         XFreePixmap (server.dsp, server.pmap);
         server.got_root_pmap = 0;
         set_redraw_all ();
         panel.redraw_clock = 1;
         panel.refresh = 1;
      }
   }
   else {
      Task *tsk;
      tsk = task_get_task (win);
      if (!tsk) return;
      //printf("atom root_win = %s, %s\n", XGetAtomName(server.dsp, at), tsk->title);

      /* Window title changed */
      if (at == server.atom._NET_WM_VISIBLE_NAME) {
         window_get_title(tsk);
         tsk->redraw = 1;
         panel.refresh = 1;
      }
      /* Iconic state */
      else if (at == server.atom.WM_STATE) {
         if (window_is_iconified (win))
            if (panel.task_active == tsk) panel.task_active = 0;
      }
      /* Window icon changed */
      else if (at == server.atom._NET_WM_ICON) {
         if (tsk->icon_data != 0) XFree (tsk->icon_data);
         tsk->redraw = 1;
         tsk->icon_data = 0;
         panel.refresh = 1;
      }
      /* Window desktop changed */
      else if (at == server.atom._NET_WM_DESKTOP) {
         add_task (tsk->win);
         remove_task (tsk);         
         panel.refresh = 1;
      }

      if (!server.got_root_win) server.root_win = RootWindow (server.dsp, server.screen);
   }
}


void event_configure_notify (Window win)
{   
   Task *tsk;

   tsk = task_get_task (win);
   if (!tsk) return;

   int new_monitor = window_get_monitor (win);
   int desktop = tsk->id_taskbar / server.nb_monitor;
   
   // task on the same monitor
   if (tsk->id_taskbar == index(desktop, new_monitor)) return;
   
   add_task (tsk->win);
   remove_task (tsk);
   panel.refresh = 1;
}


void event_timer()
{
   struct timeval stv;

   if (!panel.time1_format) return;
   
   if (gettimeofday(&stv, 0)) return;
   
   if (abs(stv.tv_sec - panel.clock.tv_sec) < panel.time_precision) return;
      
   // update clock
   panel.clock.tv_sec = stv.tv_sec;
   panel.clock.tv_sec -= panel.clock.tv_sec % panel.time_precision;
   panel.redraw_clock = 1;
   panel.refresh = 1;
}


int main (int argc, char *argv[])
{
   XEvent e;
   fd_set fd;
   int x11_fd, i, c;
   struct timeval tv;

   c = getopt (argc, argv, "c:");
   init ();

load_config:
   // read tintrc config
   i = 0;
   if (c != -1)
      i = config_read_file (optarg);
   if (!i)
      i = config_read ();
   if (!i) {
      fprintf(stderr, "exit tint : no config file\n");
      cleanup();
      exit(0);
   }
   config_finish ();
   
   window_draw_panel ();

   x11_fd = ConnectionNumber (server.dsp);
   XSync (server.dsp, False);
   
   while (1) {
      // thanks to AngryLlama for the timer
      // Create a File Description Set containing x11_fd
      FD_ZERO (&fd);
      FD_SET (x11_fd, &fd);

      tv.tv_usec = 500000;
      tv.tv_sec = 0;
      
      // Wait for X Event or a Timer
      if (select(x11_fd+1, &fd, 0, 0, &tv)) {
         while (XPending (server.dsp)) {
            XNextEvent(server.dsp, &e);
            
            switch (e.type) {
               case ButtonPress:
                  if (e.xbutton.button == 1) event_button_press (e.xbutton.x, e.xbutton.y);
                  break;

               case ButtonRelease:
                  event_button_release (e.xbutton.button, e.xbutton.x, e.xbutton.y);
                  break;
            
               case Expose:
                  XCopyArea (server.dsp, server.pmap, window.main_win, server.gc, 0, 0, panel.width, panel.height, 0, 0);
                  break;

               case PropertyNotify:
                  event_property_notify (e.xproperty.window, e.xproperty.atom);
                  break;

               case ConfigureNotify:
                  if (e.xconfigure.window == server.root_win)
                     goto load_config;
                  else
                     if (panel.mode == MULTI_MONITOR) 
                        event_configure_notify (e.xconfigure.window);                        
                  break;
            }
            //if (panel.refresh) printf("  refresh\n");
         }
      }
      else event_timer();

		switch (panel.signal_pending) {
			case SIGUSR1:
            goto load_config;
			case SIGINT:
			case SIGTERM:
			   cleanup ();
			   return 0;
      }

      if (panel.refresh && !panel.sleep_mode) {
         visual_refresh ();
         //printf("   *** visual_refresh\n");
      }      
   }
}


