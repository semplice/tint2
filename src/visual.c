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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>
#include <Imlib2.h>
#include "visual.h"
#include "server.h"
#include "config.h"
#include "task.h"
#include "launcher.h"
#include "window.h"


void visual_refresh ()
{
   Pixmap *pmap;
   Task *tsk;
   int desktop, monitor;

   server_refresh_root_pixmap ();
   
   if (panel.time1_format) {
      if (panel.redraw_clock) redraw_clock ();
      
      XCopyArea (server.dsp, panel.pmap_clock, server.pmap, server.gc, 0, 0, panel.clock_width, panel.clock_height, panel.clock_posx, panel.clock_posy);
   }
      
   for (desktop=0 ; desktop < panel.nb_desktop ; desktop++) {
      if (panel.mode != MULTI_DESKTOP && desktop != server.desktop) continue;

      for (monitor=0 ; monitor < panel.nb_monitor ; monitor++) {
         for (tsk = panel.taskbar[index(desktop, monitor)].tasklist ; tsk ; tsk = tsk->next) {
            if (tsk->redraw) {
               redraw_task (tsk);
               //printf("  draw task %s (%d, %d)\n", tsk->title, tsk->posx, tsk->width);
            }
            
            //printf("copy screen %s\n", tsk->title);
            if (tsk == panel.task_active) pmap = &tsk->active_pmap;
            else pmap = &tsk->pmap;

            XCopyArea (server.dsp, *pmap, server.pmap, server.gc, 0, 0, tsk->width, panel.task_height, tsk->posx, panel.task_posy);
         }
      }
   }

   XCopyArea (server.dsp, server.pmap, window.main_win, server.gc, 0, 0, panel.width, panel.height, 0, 0);
   XFlush(server.dsp);
   panel.refresh = 0;
}


void draw_task_icon (Task *tsk, int text_width, int active)
{
   if (tsk->icon_data == 0) return;

   Pixmap *pmap;
   
   if (active) pmap = &tsk->active_pmap;
   else pmap = &tsk->pmap;
   
   /* Find pos */
   int pos_x;
   if (panel.task_text_centered) {
      pos_x = (tsk->width - text_width - panel.task_icon_size - (panel.task_icon_size/6.0)) / 2;
   }
   else pos_x = panel.task_padding + panel.task_border.width;

   //printf("render and draw icon\n");
   /* Render */
   Imlib_Image icon;
   Imlib_Color_Modifier cmod;
   DATA8 red[256], green[256], blue[256], alpha[256];

   // TODO: cpu improvement : compute only when icon changed
   DATA32 *data;
   /* do we have 64bit? => long = 8bit */
   if (sizeof(long) != 4) {
      int length = tsk->icon_width * tsk->icon_height;
      data = malloc(sizeof(DATA32) * length);
      int i;
      for (i = 0; i < length; ++i)
         data[i] = tsk->icon_data[i];
   }
   else data = (DATA32 *) tsk->icon_data;
            
   icon = imlib_create_image_using_data (tsk->icon_width, tsk->icon_height, data);
   imlib_context_set_image (icon);
   imlib_context_set_drawable (*pmap);

   cmod = imlib_create_color_modifier ();
   imlib_context_set_color_modifier (cmod);
   imlib_image_set_has_alpha (1);
   imlib_get_color_modifier_tables (red, green, blue, alpha);

   int i;
   if (!active) {
      /* Have no idea what I'm doing here, but it works */
      for(i = 127; i < 256; i++) alpha[i] = 100;
   }
       
   imlib_set_color_modifier_tables (red, green, blue, alpha);
   
   //imlib_render_image_on_drawable (pos_x, pos_y);
   imlib_render_image_on_drawable_at_size (pos_x, panel.task_icon_posy, panel.task_icon_size, panel.task_icon_size);
   
   imlib_free_color_modifier ();
   imlib_free_image ();
   if (sizeof(long) != 4) free(data);
}


void draw_task_title (cairo_t *c, Task *tsk, int active)
{
   PangoLayout *layout;
   config_color *config_text;
   int width, height;
   
   /* Layout */
   layout = pango_cairo_create_layout (c);
   pango_layout_set_font_description (layout, panel.task_font_desc);
   pango_layout_set_text (layout, tsk->title, -1);

   /* Drawing width and Cut text */
   pango_layout_set_width (layout, panel.taskbar[tsk->id_taskbar].text_width * PANGO_SCALE);
   pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

   /* Center text */
   if (panel.task_text_centered) pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
   else pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

   pango_layout_get_pixel_size (layout, &width, &height);

   if (active) config_text = &panel.task_font_active;
   else config_text = &panel.task_font;

   cairo_set_source_rgba (c, config_text->color[0], config_text->color[1], config_text->color[2], config_text->alpha);

   pango_cairo_update_layout (c, layout);
   cairo_move_to (c, panel.task_text_posx, panel.task_text_posy);
   pango_cairo_show_layout (c, layout);

   if (panel.font_shadow) {
      cairo_set_source_rgba (c, 0.0, 0.0, 0.0, 0.5);
      pango_cairo_update_layout (c, layout);
      cairo_move_to (c, panel.task_text_posx + 1, panel.task_text_posy + 1);
      pango_cairo_show_layout (c, layout);
   }
   
   if (panel.task_icon_size) draw_task_icon (tsk, width, active);
   
   g_object_unref (layout);
}


void draw_rect(cairo_t *c, double x, double y, double w, double h, double r)
{   
   if (r > 0.0) {
      double c1 = 0.55228475 * r;

      cairo_move_to(c, x+r, y);
      cairo_rel_line_to(c, w-2*r, 0);
      cairo_rel_curve_to(c, c1, 0.0, r, c1, r, r);
      cairo_rel_line_to(c, 0, h-2*r);
      cairo_rel_curve_to(c, 0.0, c1, c1-r, r, -r, r);
      cairo_rel_line_to (c, -w +2*r, 0);
      cairo_rel_curve_to (c, -c1, 0, -r, -c1, -r, -r);
      cairo_rel_line_to (c, 0, -h + 2 * r);
      cairo_rel_curve_to (c, 0, -c1, r - c1, -r, r, -r);
   }
   else
      cairo_rectangle(c, x, y, w, h);
}


void draw_task_background (cairo_t *c, Task *tsk, int active)
{
   config_border *border;
   config_color  *back;
   
   if (active) {
      border = &panel.task_active_border;
      back = &panel.task_active_back;
   }
   else {
      border = &panel.task_border;
      back = &panel.task_back;
   }
   
   if (back->alpha > 0.0) {
      draw_rect(c, border->width, border->width, tsk->width-(2.0*border->width), panel.task_height-(2.0*border->width), border->rounded - border->width/1.571);
      
      cairo_set_source_rgba (c, back->color[0], back->color[1], back->color[2], back->alpha);
      
      cairo_fill (c);
   }

   if (border->width > 0 || border->alpha > 0.0) {
      cairo_set_line_width (c, border->width);
      
      // draw border inside (x, y, w, h)
      draw_rect(c, border->width/2.0, border->width/2.0, tsk->width-border->width, panel.task_height-border->width, border->rounded);
      
      cairo_set_source_rgba (c, border->color[0], border->color[1], border->color[2], border->alpha);
      
      cairo_stroke (c);
   }
}


void draw_panel_background ()
{
   cairo_surface_t *cs;
   cairo_t *c;
   double alpha;

   cs = cairo_xlib_surface_create (server.dsp, server.root_pmap, server.visual, panel.width, panel.height);
   c = cairo_create (cs);

   if (!server.got_root_pmap) alpha = 1.0; // No wallpaper, no alpha
   else alpha = panel.background.alpha;
   
   if (panel.background.alpha > 0.0) {
      draw_rect(c, panel.border.width, panel.border.width, panel.width-(2.0*panel.border.width), panel.height-(2.0*panel.border.width), panel.border.rounded - panel.border.width/1.571);
      
      cairo_set_source_rgba (c, panel.background.color[0], panel.background.color[1], panel.background.color[2], alpha);
      
      cairo_fill (c);
   }
   if (panel.border.width > 0) {
      cairo_set_line_width (c, panel.border.width);
      
      // draw border inside (x, y, w, h)
      draw_rect(c, panel.border.width/2.0, panel.border.width/2.0, panel.width-panel.border.width, panel.height-panel.border.width, panel.border.rounded);
      
      cairo_set_source_rgba (c, panel.border.color[0], panel.border.color[1], panel.border.color[2], panel.border.alpha);
      
      cairo_stroke (c);
   }
   
   cairo_destroy (c);
   cairo_surface_destroy (cs);
}


void redraw_task (Task *tsk)
{
   cairo_surface_t *cs;
   cairo_t *c;

   if (tsk->icon_data == 0) window_get_icon (tsk);
   
   if (tsk->pmap != 0) XFreePixmap (server.dsp, tsk->pmap);
   if (tsk->active_pmap != 0) XFreePixmap (server.dsp, tsk->active_pmap);

   // draw inactive pixmap
   tsk->pmap = server_create_pixmap (tsk->width, panel.task_height);
   tsk->active_pmap = server_create_pixmap (tsk->width, panel.task_height);
   
   /* Copy over root pixmap */
   XCopyArea (server.dsp, server.pmap, tsk->pmap, server.gc, tsk->posx, panel.task_posy, tsk->width, panel.task_height, 0, 0);
   XCopyArea (server.dsp, server.pmap, tsk->active_pmap, server.gc, tsk->posx, panel.task_posy, tsk->width, panel.task_height, 0, 0);

   cs = cairo_xlib_surface_create (server.dsp, tsk->pmap, server.visual, tsk->width, panel.task_height);
   c = cairo_create (cs);

   /* Refresh task visuals */
   draw_task_background (c, tsk, 0);
   draw_task_title (c, tsk, 0);

   cairo_destroy (c);
   cairo_surface_destroy (cs);

   // draw active pixmap
   cs = cairo_xlib_surface_create (server.dsp, tsk->active_pmap, server.visual, tsk->width, panel.task_height);
   c = cairo_create (cs);

   /* Refresh task visuals */
   draw_task_background (c, tsk, 1);
   draw_task_title (c, tsk, 1);

   cairo_destroy (c);
   cairo_surface_destroy (cs);

   tsk->redraw = 0;
}


void redraw_clock ()
{
   int time_width, date_width;
   cairo_surface_t *cs;
   cairo_t *c;
   PangoLayout *layout;
   char buf_time[40];
   char buf_date[40];

   time_width = date_width = 0;
   strftime(buf_time, sizeof(buf_time), panel.time1_format, localtime(&panel.clock.tv_sec));
   if (panel.time2_format)
      strftime(buf_date, sizeof(buf_date), panel.time2_format, localtime(&panel.clock.tv_sec));

redraw:
   if (panel.pmap_clock) XFreePixmap (server.dsp, panel.pmap_clock);
   panel.pmap_clock = server_create_pixmap (panel.clock_width, panel.clock_height);
   
   XCopyArea (server.dsp, server.pmap, panel.pmap_clock, server.gc, panel.clock_posx, panel.clock_posy, panel.clock_width, panel.clock_height, 0, 0);

   cs = cairo_xlib_surface_create (server.dsp, panel.pmap_clock, server.visual, panel.clock_width, panel.clock_height);
   c = cairo_create (cs);
   layout = pango_cairo_create_layout (c);

   // check width
   pango_layout_set_font_description (layout, panel.time1_font_desc);
   pango_layout_set_indent(layout, 0);
   pango_layout_set_text (layout, buf_time, strlen(buf_time));
   pango_layout_get_pixel_size (layout, &time_width, NULL);
   if (panel.time2_format) {
      pango_layout_set_font_description (layout, panel.time2_font_desc);
      pango_layout_set_indent(layout, 0);
      pango_layout_set_text (layout, buf_date, strlen(buf_date));
      pango_layout_get_pixel_size (layout, &date_width, NULL);
   }
   if (time_width > panel.clock_width || (date_width != panel.clock_width && date_width > time_width)) {
      //printf("clock_width %d, new_width %d\n", panel.clock_width, time_width);
      if (time_width > date_width) resize_clock(time_width);
      else resize_clock(date_width);
      
      g_object_unref (layout);
      cairo_destroy (c);
      cairo_surface_destroy (cs);
      resize_taskbar();
      set_redraw_all();
      goto redraw;
   }

   // draw layout
   pango_layout_set_font_description (layout, panel.time1_font_desc);
   pango_layout_set_width (layout, panel.clock_width * PANGO_SCALE);
   pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
   pango_layout_set_text (layout, buf_time, strlen(buf_time));

   cairo_set_source_rgba (c, panel.clock_font.color[0], panel.clock_font.color[1], panel.clock_font.color[2], panel.clock_font.alpha);

   pango_cairo_update_layout (c, layout);
   cairo_move_to (c, 0, panel.time1_posy);
   pango_cairo_show_layout (c, layout);

   if (panel.time2_format) {
      pango_layout_set_font_description (layout, panel.time2_font_desc);
      pango_layout_set_indent(layout, 0);
      pango_layout_set_text (layout, buf_date, strlen(buf_date));
      pango_layout_set_width (layout, panel.clock_width * PANGO_SCALE);
      
      pango_cairo_update_layout (c, layout);
      cairo_move_to (c, 0, panel.time2_posy);
      pango_cairo_show_layout (c, layout);
   }
   
   g_object_unref (layout);
   cairo_destroy (c);
   cairo_surface_destroy (cs);
   panel.redraw_clock = 0; 
   panel.refresh = 1;
}


void resize_clock(int width)
{
   panel.clock_width = width;
   panel.clock_posx = panel.width - width - panel.paddingx - panel.border.width;
}


// initialise taskbar posx and width
void resize_taskbar()
{
   int taskbar_width, modulo_width, taskbar_on_screen;

   if (panel.mode == MULTI_DESKTOP) taskbar_on_screen = panel.nb_desktop;
   else taskbar_on_screen = panel.nb_monitor;
   
   taskbar_width = panel.width - panel.launcher_width - (2 * panel.paddingx) - (2 * panel.border.width);
   if (panel.time1_format) 
      taskbar_width -= (panel.clock_width + panel.paddingx);
   taskbar_width = (taskbar_width - ((taskbar_on_screen-1) * panel.paddingx)) / taskbar_on_screen;

   if (taskbar_on_screen > 1)
      modulo_width = (taskbar_width - ((taskbar_on_screen-1) * panel.paddingx)) % taskbar_on_screen;
   else 
      modulo_width = 0;
   
   int posx, modulo, i, nb;
   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++) {
      if ((i % taskbar_on_screen) == 0) {
         posx = panel.border.width + panel.paddingx;
         modulo = modulo_width;
      }
      else posx += taskbar_width + panel.paddingx;

      panel.taskbar[i].posx = posx;
      panel.taskbar[i].width = taskbar_width;
      if (modulo) {
         panel.taskbar[i].width++;
         modulo--;
      }
      
      resize_tasks(i);
   }
}


void visual_draw_launcher ()
{
   
}

