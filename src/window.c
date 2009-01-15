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
#include <Imlib2.h>
#include "task.h"
#include "window.h"
#include "server.h"
#include "config.h"

#define WM_CLASS_TINT   "tint"


void set_active (Window win)
{
   send_event32 (win, server.atom._NET_ACTIVE_WINDOW, 2, 0);
}


void set_desktop (int desktop)
{
   send_event32 (server.root_win, server.atom._NET_CURRENT_DESKTOP, desktop, 0);
}


void windows_set_desktop (Window win, int desktop)
{
   send_event32 (win, server.atom._NET_WM_DESKTOP, desktop, 2);
}


void set_close (Window win)
{
   send_event32 (win, server.atom._NET_CLOSE_WINDOW, 0, 2);
}


void window_toggle_shade (Window win)
{
   send_event32 (win, server.atom._NET_WM_STATE, 2, 0);
}


int window_is_hidden (Window win)
{
   Window window;
   Atom *at;
   int count, i;
   
   if (XGetTransientForHint(server.dsp, win, &window) != 0) {
      if (window) {
         return 1;
      }
   }

   at = server_get_property (win, server.atom._NET_WM_STATE, XA_ATOM, &count);
   for (i = 0; i < count; i++) {      
      if (at[i] == server.atom._NET_WM_STATE_SKIP_PAGER || at[i] == server.atom._NET_WM_STATE_SKIP_TASKBAR) {
         XFree(at);
         return 1;
      }
   }
   XFree(at);

   at = server_get_property (win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, &count);
   for (i = 0; i < count; i++) {
      if (at[i] == server.atom._NET_WM_WINDOW_TYPE_DOCK || at[i] == server.atom._NET_WM_WINDOW_TYPE_DESKTOP || at[i] == server.atom._NET_WM_WINDOW_TYPE_TOOLBAR || at[i] == server.atom._NET_WM_WINDOW_TYPE_MENU || at[i] == server.atom._NET_WM_WINDOW_TYPE_SPLASH) {
         XFree(at);
         return 1;
      }
   }

   // specification
   // Windows with neither _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set
   // MUST be taken as top-level window.
   XFree(at);
   return 0;
}


int window_get_desktop (Window win)
{
   return get_property32(win, server.atom._NET_WM_DESKTOP, XA_CARDINAL);
}  


int window_get_monitor (Window win)
{
   int i, x, y;
   Window src;

   XTranslateCoordinates(server.dsp, win, server.root_win, 0, 0, &x, &y, &src);   
   for (i = 0; i < server.nb_monitor; i++) {
      if (x >= server.monitor[i].x && x <= (server.monitor[i].x + server.monitor[i].width))
         if (y >= server.monitor[i].y && y <= (server.monitor[i].y + server.monitor[i].height))
            break;
   }
   
   //printf("window %lx : ecran %d, (%d, %d)\n", win, i, x, y);
   if (i == server.nb_monitor) return 0;
   else return i;
}


int window_is_iconified (Window win)
{
   return (IconicState == get_property32(win, server.atom.WM_STATE, server.atom.WM_STATE));
}


int server_get_number_of_desktop ()
{
   return get_property32(server.root_win, server.atom._NET_NUMBER_OF_DESKTOPS, XA_CARDINAL);
}


int server_get_current_desktop ()
{
   return get_property32(server.root_win, server.atom._NET_CURRENT_DESKTOP, XA_CARDINAL);
}


Window window_get_active ()
{
   return get_property32(server.root_win, server.atom._NET_ACTIVE_WINDOW, XA_WINDOW);
}


int window_is_active (Window win)
{
   return (win == get_property32(server.root_win, server.atom._NET_ACTIVE_WINDOW, XA_WINDOW));
}


void window_action (Task *tsk, int action)
{
   switch (action) {
      case CLOSE:
         set_close (tsk->win);
         break;
      case TOGGLE:
         set_active(tsk->win);
         break;
      case ICONIFY:
         XIconifyWindow (server.dsp, tsk->win, server.screen);
         break;
      case TOGGLE_ICONIFY:
         if (tsk == panel.task_active) XIconifyWindow (server.dsp, tsk->win, server.screen);
         else set_active (tsk->win);
         break;
      case SHADE:
         window_toggle_shade (tsk->win);
         break;
   }
}


void window_get_title(Task *tsk)
{
   char *title;
   
   XFree (tsk->title);
   
   title = server_get_property (tsk->win, server.atom._NET_WM_VISIBLE_NAME, server.atom.UTF8_STRING, 0);
   if (!title || !strlen(title)) {
      if (title) XFree (title);
      
      title = server_get_property (tsk->win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 0);
      if (!title || !strlen(title)) {
         if (title) XFree (title);

         title = malloc(10);
         strcpy(title, "Untitled");
      }
   }
   tsk->title = title;
}


int get_icon_count (long *data, int num)
{
   int count, pos, w, h;

   count = 0;
   pos = 0;
   while (pos < num) {
      w = data[pos++];
      h = data[pos++];
      pos += w * h;
      if (pos > num || w * h == 0) break;
      count++;
   }

   return count;
}


long *get_best_icon (long *data, int icon_count, int num, int *iw, int *ih)
{
   int width[icon_count], height[icon_count], pos, i, w, h;
   long *icon_data[icon_count];

   /* List up icons */
   pos = 0;
   i = icon_count;
   while (i--) {
      w = data[pos++];
      h = data[pos++];
      if (pos + w * h > num) break;

      width[i] = w;
      height[i] = h;
      icon_data[i] = &data[pos];

      pos += w * h;
   }

   /* Try to find exact size */
   int icon_num = -1;
   for (i = 0; i < icon_count; i++) {
      if (width[i] == panel.task_icon_size) {
         icon_num = i;
         break;
      }
   }

   /* Take the biggest or whatever */
   if (icon_num < 0) {
      int highest = 0;
      for (i = 0; i < icon_count; i++) {
         if (width[i] > highest) {
               icon_num = i;
               highest = width[i];
         }
      }
   }
       
   *iw = width[icon_num];
   *ih = height[icon_num];
   return icon_data[icon_num];
}


void window_get_icon (Task *tsk)
{
   long *data;
   int num;

   if (tsk->icon_data != 0) return;

   data = server_get_property (tsk->win, server.atom._NET_WM_ICON, XA_CARDINAL, &num);

   if (!data) return;

   int w, h;
   long *tmp_data;
   tmp_data = get_best_icon (data, get_icon_count (data, num), num, &w, &h);

   tsk->icon_width = w;
   tsk->icon_height = h;
   tsk->icon_data = malloc (w * h * sizeof (long));
   memcpy (tsk->icon_data, tmp_data, w * h * sizeof (long));

   /* TODO: resize icon => check the impact on memory
   Imlib_Image iconSrc, iconDest;
   iconSrc = imlib_create_image_using_data (w, h, (DATA32 *)tmp_data);
   iconDest = imlib_create_image(panel.task_icon_size, panel.task_icon_size);
   imlib_context_set_image (iconDest);
   //imlib_image_set_has_alpha(1);
   imlib_blend_image_onto_image(iconSrc, 1, 0, 0, w, h, 0, 0, panel.task_icon_size, panel.task_icon_size);
   
   tsk->icon_width = panel.task_icon_size;
   tsk->icon_height = panel.task_icon_size;
   tsk->icon_data = malloc (panel.task_icon_size * 2 * sizeof (long));
   memcpy (tsk->icon_data, imlib_image_get_data(), panel.task_icon_size * 2 * sizeof (long));
   imlib_free_image ();
*/   
      
   XFree (data);
}


void set_panel_properties (Window win)
{
   XStoreName (server.dsp, win, "tint");

   // TODO: check if the name is realy needed for a panel/taskbar ?
   gsize len;
   gchar *name = g_locale_to_utf8("tint", -1, NULL, &len, NULL);
   if (name != NULL) {
      XChangeProperty(server.dsp, win, server.atom._NET_WM_NAME, server.atom.UTF8_STRING, 8, PropModeReplace, (unsigned char *) name, (int) len);
      g_free(name);
   }
  
   // Dock
   long val = server.atom._NET_WM_WINDOW_TYPE_DOCK;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char *) &val, 1);

   // Reserved space
   long   struts [12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   if (panel.position & TOP) {
      struts[2] = panel.height + panel.marginy;
      struts[8] = panel.posx;
      struts[9] = panel.posx + panel.width;
   }
   else {
      struts[3] = panel.height + panel.marginy;
      struts[10] = panel.posx;
      struts[11] = panel.posx + panel.width;
   }
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STRUT_PARTIAL, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 12);
   // Old specification
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STRUT, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &struts, 4);
   
   // Sticky and below other window
   val = 0xFFFFFFFF;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_DESKTOP, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
   Atom state[4];
   state[0] = server.atom._NET_WM_STATE_SKIP_PAGER;
   state[1] = server.atom._NET_WM_STATE_SKIP_TASKBAR;
   state[2] = server.atom._NET_WM_STATE_STICKY;
   state[3] = server.atom._NET_WM_STATE_BELOW;
   XChangeProperty (server.dsp, win, server.atom._NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char *) state, 4);   
   
   // Fixed position 
   XSizeHints size_hints;
   size_hints.flags = PPosition;
   XChangeProperty (server.dsp, win, XA_WM_NORMAL_HINTS, XA_WM_SIZE_HINTS, 32, PropModeReplace, (unsigned char *) &size_hints, sizeof (XSizeHints) / 4);
   
   // Unfocusable
   XWMHints wmhints;
   wmhints.flags = InputHint;
   wmhints.input = False;
   XChangeProperty (server.dsp, win, XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace, (unsigned char *) &wmhints, sizeof (XWMHints) / 4);
}


void window_draw_panel ()
{
   Window win;

   /* panel position determined here */
   if (panel.position & LEFT) panel.posx = server.monitor[panel.monitor].x + panel.marginx;
   else {
      if (panel.position & RIGHT) panel.posx = server.monitor[panel.monitor].x + server.monitor[panel.monitor].width - panel.width - panel.marginx;
      else panel.posx = server.monitor[panel.monitor].x + ((server.monitor[panel.monitor].width - panel.width) / 2);
   }
   if (panel.position & TOP) panel.posy = server.monitor[panel.monitor].y + panel.marginy;
   else panel.posy = server.monitor[panel.monitor].y + server.monitor[panel.monitor].height - panel.height - panel.marginy;

   /* Catch some events */
   XSetWindowAttributes att = { ParentRelative, 0L, 0, 0L, 0, 0, Always, 0L, 0L, False, ExposureMask|ButtonPressMask|ButtonReleaseMask, 0L, False, 0, 0 };
               
   /* XCreateWindow(display, parent, x, y, w, h, border, depth, class, visual, mask, attrib) */
   if (window.main_win) XDestroyWindow(server.dsp, window.main_win);
   win = XCreateWindow (server.dsp, server.root_win, panel.posx, panel.posy, panel.width, panel.height, 0, server.depth, InputOutput, CopyFromParent, CWEventMask, &att);

   set_panel_properties (win);
   window.main_win = win;

   // replaced : server.gc = DefaultGC (server.dsp, 0);
   if (server.gc) XFree(server.gc);
   XGCValues gcValues;
   server.gc = XCreateGC(server.dsp, win, (unsigned long) 0, &gcValues);
   
   XMapWindow (server.dsp, win);
   XFlush (server.dsp);
}


