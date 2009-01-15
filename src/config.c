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
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib/gstdio.h>
#include <pango/pangocairo.h>
#include <Imlib2.h>
#include "window.h"
#include "config.h"
#include "launcher.h"
#include "visual.h"
#include "task.h"
#include "server.h"


void cleanup_taskbar() 
{
   int i, nb;
   Task *tsk, *next;

   nb = panel.nb_desktop * panel.nb_monitor;
   for (i=0 ; i < nb ; i++)
      for (tsk = panel.taskbar[i].tasklist; tsk ; tsk = next) {
         next = tsk->next;
         remove_task (tsk);
      }

   free(panel.taskbar);
   panel.taskbar = 0;
}


void cleanup ()
{
   if (panel.old_task_font) free(panel.old_task_font);
   if (panel.task_font_desc) pango_font_description_free(panel.task_font_desc);
   if (panel.time1_font_desc) pango_font_description_free(panel.time1_font_desc);
   if (panel.time2_font_desc) pango_font_description_free(panel.time2_font_desc);
   if (panel.taskbar) cleanup_taskbar();
   if (panel.time1_format) g_free(panel.time1_format);
   if (panel.time2_format) g_free(panel.time2_format);
   if (server.monitor) free(server.monitor);
   XCloseDisplay(server.dsp);
}


void config_get_text_size(PangoFontDescription *font, int *height_ink, int *height)
{
   PangoRectangle rect_ink, rect;

   Pixmap pmap = server_create_pixmap (panel.height, panel.height);
   cairo_surface_t *cs = cairo_xlib_surface_create (server.dsp, pmap, server.visual, panel.task_maximum_width, panel.task_height);
   cairo_t *c = cairo_create (cs);
   
   PangoLayout *layout = pango_cairo_create_layout (c);
   pango_layout_set_font_description (layout, font);
   pango_layout_set_text (layout, "A", 1);
   
   pango_layout_get_pixel_extents(layout, &rect_ink, &rect);
   *height_ink = rect_ink.height;
   *height = rect.height;
   //printf("dimension : %d - %d\n", rect_ink.height, rect.height);

   g_object_unref (layout);   
   cairo_destroy (c);
   cairo_surface_destroy (cs);
   XFreePixmap (server.dsp, pmap);
}


void copy_file(const char *pathSrc, const char *pathDest)
{
   FILE *fileSrc, *fileDest;
   char line[100];
   int  nb;

   fileSrc = fopen(pathSrc, "rb");
   if (fileSrc == NULL) return;
   
   fileDest = fopen(pathDest, "wb");
   if (fileDest == NULL) return;
   
   while ((nb = fread(line, 1, 100, fileSrc)) > 0) fwrite(line, 1, nb, fileDest);
   
   fclose (fileDest);
   fclose (fileSrc);
}


void extract_values (const char *value, char **value1, char **value2)
{
   char *b;

   if (*value1) free (*value1);
   if (*value2) free (*value2);
   
   if ((b = strchr (value, ' '))) {
      b[0] = '\0';
      b++;
      *value2 = strdup (b);
      g_strstrip(*value2);
   }
   else *value2 = 0;
   
   *value1 = strdup (value);
   g_strstrip(*value1);
}


int hex_char_to_int (char c)
{
   int r;
   
   if (c >= '0' && c <= '9')  r = c - '0';
   else if (c >= 'a' && c <= 'f')  r = c - 'a' + 10;
   else if (c >= 'A' && c <= 'F')  r = c - 'A' + 10;
   else  r = 0;

   return r;
}


int hex_to_rgb (char *hex, int *r, int *g, int *b)
{
   int len;

   if (hex == NULL || hex[0] != '#') return (0);

   len = strlen (hex);
   if (len == 3 + 1) {
      *r = hex_char_to_int (hex[1]);
      *g = hex_char_to_int (hex[2]);
      *b = hex_char_to_int (hex[3]);
   }
   else if (len == 6 + 1) {
      *r = hex_char_to_int (hex[1]) * 16 + hex_char_to_int (hex[2]);
      *g = hex_char_to_int (hex[3]) * 16 + hex_char_to_int (hex[4]);
      *b = hex_char_to_int (hex[5]) * 16 + hex_char_to_int (hex[6]);
   }
   else if (len == 12 + 1) {
      *r = hex_char_to_int (hex[1]) * 16 + hex_char_to_int (hex[2]);
      *g = hex_char_to_int (hex[5]) * 16 + hex_char_to_int (hex[6]);
      *b = hex_char_to_int (hex[9]) * 16 + hex_char_to_int (hex[10]);
   }
   else return 0;

   return 1;
}


void get_color (char *hex, double *rgb)
{
   int r, g, b;
   hex_to_rgb (hex, &r, &g, &b);

   rgb[0] = (r / 255.0);
   rgb[1] = (g / 255.0);
   rgb[2] = (b / 255.0);
}


void get_action (char *event, int *action)
{
   if (strcmp (event, "none") == 0)
      *action = NONE;
   else if (strcmp (event, "close") == 0)
      *action = CLOSE;
   else if (strcmp (event, "toggle") == 0)
      *action = TOGGLE;
   else if (strcmp (event, "iconify") == 0)
      *action = ICONIFY;
   else if (strcmp (event, "shade") == 0)
      *action = SHADE;
   else if (strcmp (event, "toggle_iconify") == 0)
      *action = TOGGLE_ICONIFY;
}


void add_entry (char *key, char *value)
{
   char *value1=0, *value2=0;

   /* Font */
   if (strcmp (key, "task_font") == 0) {
      if (panel.task_font_desc) pango_font_description_free(panel.task_font_desc);
      panel.task_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "task_font_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.task_font.color);
      if (value2) panel.task_font.alpha = (atoi (value2) / 100.0);
      else panel.task_font.alpha = 0.1;
   }
   else if (strcmp (key, "task_active_font_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.task_font_active.color);
      if (value2) panel.task_font_active.alpha = (atoi (value2) / 100.0);
      else panel.task_font_active.alpha = 0.1;
   }
   else if (strcmp (key, "font_shadow") == 0)
      panel.font_shadow = atoi (value);

   /* Panel */
   else if (strcmp (key, "panel_mode") == 0) {
      if (strcmp (value, "multi_desktop") == 0) panel.mode = MULTI_DESKTOP;
      else if (strcmp (value, "multi_monitor") == 0) panel.mode = MULTI_MONITOR;
      else panel.mode = SINGLE_DESKTOP;
   }
   else if (strcmp (key, "panel_monitor") == 0) {
      panel.monitor = atoi (value);
      if (panel.monitor > 0) panel.monitor -= 1;
   }
   else if (strcmp (key, "panel_size") == 0) {
      extract_values(value, &value1, &value2);
      panel.width = atoi (value1);
      if (value2) panel.height = atoi (value2);
   }
   else if (strcmp (key, "panel_margin") == 0) {
      extract_values(value, &value1, &value2);
      panel.marginx = atoi (value1);
      if (value2) panel.marginy = atoi (value2);
   }
   else if (strcmp (key, "panel_padding") == 0) {
      extract_values(value, &value1, &value2);
      panel.paddingx = atoi (value1);
      if (value2) panel.paddingy = atoi (value2);
   }
   else if (strcmp (key, "panel_position") == 0) {
      extract_values(value, &value1, &value2);
      if (strcmp (value1, "top") == 0) panel.position = TOP;
      else panel.position = BOTTOM;

      if (!value2) panel.position = CENTER;
      else {
         if (strcmp (value2, "left") == 0) panel.position |= LEFT;
         else {
            if (strcmp (value2, "right") == 0) panel.position |= RIGHT;
            else panel.position |= CENTER;
         }
      }
   }

   /* Clock */
   else if (strcmp (key, "time1_format") == 0) {
      if (panel.time1_format) g_free(panel.time1_format);
      if (strlen(value) > 0) panel.time1_format = strdup (value);
      else panel.time1_format = 0;
   }
   else if (strcmp (key, "time2_format") == 0) {
      if (panel.time2_format) g_free(panel.time2_format);
      if (strlen(value) > 0) panel.time2_format = strdup (value);
      else panel.time2_format = 0;
   }
   else if (strcmp (key, "time1_font") == 0) {
      if (panel.time1_font_desc) pango_font_description_free(panel.time1_font_desc);
      panel.time1_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "time2_font") == 0) {
      if (panel.time2_font_desc) pango_font_description_free(panel.time2_font_desc);
      panel.time2_font_desc = pango_font_description_from_string (value);
   }
   else if (strcmp (key, "clock_font_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.clock_font.color);
      if (value2) panel.clock_font.alpha = (atoi (value2) / 100.0);
      else panel.clock_font.alpha = 0.1;
   }
   
   /* Panel Background */
   else if (strcmp (key, "panel_background_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.background.color);
      if (value2) panel.background.alpha = (atoi (value2) / 100.0);
      else panel.background.alpha = 0.5;
   }
   else if (strcmp (key, "panel_border_width") == 0)
      panel.border.width = atoi (value);
   else if (strcmp (key, "panel_border_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.border.color);
      if (value2) panel.border.alpha = (atoi (value2) / 100.0);
      else panel.border.alpha = 0.5;
   }
   else if (strcmp (key, "panel_rounded") == 0)
      panel.border.rounded = atoi (value);

   /* Task */
   else if (strcmp (key, "task_text_centered") == 0)
      panel.task_text_centered = atoi (value);
   else if (strcmp (key, "task_width") == 0)
      panel.task_maximum_width = atoi (value);
   else if (strcmp (key, "task_margin") == 0)
      panel.task_margin = atoi (value);
   else if (strcmp (key, "task_padding") == 0)
      panel.task_padding = atoi (value);
   else if (strcmp (key, "task_icon_size") == 0)
      panel.task_icon_size = atoi (value);

   /* Task Background and border */
   else if (strcmp (key, "task_background_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.task_back.color);
      if (value2) panel.task_back.alpha = (atoi (value2) / 100.0);
      else panel.task_back.alpha = 0.5;
   }
   else if (strcmp (key, "task_active_background_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.task_active_back.color);
      if (value2) panel.task_active_back.alpha = (atoi (value2) / 100.0);
      else panel.task_active_back.alpha = 0.5;
   }

   else if (strcmp (key, "task_border_width") == 0) {
      panel.task_border.width = atoi (value);
      panel.task_active_border.width = atoi (value);
   }
   else if (strcmp (key, "task_rounded") == 0) {
      panel.task_border.rounded = atoi (value);
      panel.task_active_border.rounded = atoi (value);
   }
   else if (strcmp (key, "task_border_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.task_border.color);
      if (value2) panel.task_border.alpha = (atoi (value2) / 100.0);
      else panel.task_border.alpha = 0.5;
   }
   else if (strcmp (key, "task_active_border_color") == 0) {
      extract_values(value, &value1, &value2);
      get_color (value1, panel.task_active_border.color);
      if (value2) panel.task_active_border.alpha = (atoi (value2) / 100.0);
      else panel.task_active_border.alpha = 0.5;
   }

   /* Mouse actions */
   else if (strcmp (key, "mouse_middle") == 0)
      get_action (value, &panel.mouse_middle);
   else if (strcmp (key, "mouse_right") == 0)
      get_action (value, &panel.mouse_right);
   else if (strcmp (key, "mouse_scroll_up") == 0)
      get_action (value, &panel.mouse_scroll_up);
   else if (strcmp (key, "mouse_scroll_down") == 0)
      get_action (value, &panel.mouse_scroll_down);

   /* Launcher */
   else if (strcmp (key, "launcher_exec") == 0)
      launcher_add_exec (value);
   else if (strcmp (key, "launcher_icon") == 0)
      launcher_add_icon (value);
   else if (strcmp (key, "launcher_text") == 0)
      launcher_add_text (value);

   /* Read old config for backward compatibility */
   else if (strcmp (key, "font") == 0) {
      panel.old_config_file = 1;
      if (panel.task_font_desc) pango_font_description_free(panel.task_font_desc);
      panel.task_font_desc = pango_font_description_from_string (value);
      if (panel.old_task_font) free(panel.old_task_font);
      panel.old_task_font = strdup (value);
   }
   else if (strcmp (key, "font_color") == 0)
      get_color (value, panel.task_font.color);
   else if (strcmp (key, "font_alpha") == 0)
      panel.task_font.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "font_active_color") == 0)
      get_color (value, panel.task_font_active.color);
   else if (strcmp (key, "font_active_alpha") == 0)
      panel.task_font_active.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "panel_show_all_desktop") == 0) {
      if (atoi (value) == 0) panel.mode = SINGLE_DESKTOP;
      else panel.mode = MULTI_DESKTOP;
   }
   else if (strcmp (key, "panel_width") == 0)
      panel.width = atoi (value);
   else if (strcmp (key, "panel_height") == 0)
      panel.height = atoi (value);
   else if (strcmp (key, "panel_background") == 0)
      panel.old_panel_background = atoi (value);
   else if (strcmp (key, "panel_background_alpha") == 0)
      panel.background.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "panel_border_alpha") == 0)
      panel.border.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_icon") == 0)
      panel.old_task_icon = atoi (value);
   else if (strcmp (key, "task_background") == 0)
      panel.old_task_background = atoi (value);
   else if (strcmp (key, "task_background_alpha") == 0)
      panel.task_back.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_active_background_alpha") == 0)
      panel.task_active_back.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_border_alpha") == 0)
      panel.task_border.alpha = (atoi (value) / 100.0);
   else if (strcmp (key, "task_active_border_alpha") == 0)
      panel.task_active_border.alpha = (atoi (value) / 100.0);
   // disabled parameters
   else if (strcmp (key, "task_active_border_width") == 0) ;
   else if (strcmp (key, "task_active_rounded") == 0) ;
   
   else
      fprintf(stderr, "Invalid option: \"%s\", correct your config file\n", key);

   if (value1) free (value1);
   if (value2) free (value2);
}


int parse_line (const char *line)
{
   char *a, *b, *key, *value;

   /* Skip useless lines */
   if ((line[0] == '#') || (line[0] == '\n')) return 0;
   if (!(a = strchr (line, '='))) return 0;

   /* overwrite '=' with '\0' */
   a[0] = '\0';
   key = strdup (line);
   a++;

   /* overwrite '\n' with '\0' if '\n' present */
   if ((b = strchr (a, '\n'))) b[0] = '\0';

   value = strdup (a);

   g_strstrip(key);
   g_strstrip(value);

   add_entry (key, value);

   free (key);
   free (value);
   return 1;
}


void config_taskbar()
{
   int i;

   if (panel.task_border.rounded > panel.task_height/2)
      panel.task_border.rounded = panel.task_height/2;
   
   for (i=0 ; i < 15 ; i++) {
      server.nb_desktop = server_get_number_of_desktop ();
      if (server.nb_desktop > 0) break;
      sleep(1);
   }
   if (server.nb_desktop == 0) {
      cleanup ();
      fprintf(stderr, "tint error : cannot find your desktop.\n");
      exit(0);
   }

   if (panel.taskbar) cleanup_taskbar();
   
   panel.nb_desktop = server.nb_desktop;
   if (panel.mode == MULTI_MONITOR) panel.nb_monitor = server.nb_monitor;
   else panel.nb_monitor = 1;
   panel.taskbar = calloc(panel.nb_desktop * panel.nb_monitor, sizeof(Taskbar));

   //printf("tasbar (desktop x monitor) : (%d x %d)\n", panel.nb_desktop, panel.nb_monitor);
   resize_taskbar();
   task_refresh_tasklist ();
   panel.refresh = 1;
}


void config_clock()
{
   int time_height, time_height_ink, date_height, date_height_ink;

   if (!panel.time1_format) return;
   
   if (strchr(panel.time1_format, 'S') == NULL) panel.time_precision = 60;
   else panel.time_precision = 1;
   
   gettimeofday(&panel.clock, 0);
   panel.clock.tv_sec -= panel.clock.tv_sec % panel.time_precision;

   panel.clock_posy = panel.border.width;
   panel.clock_height = panel.height - 2 * panel.border.width;
   
   config_get_text_size(panel.time1_font_desc, &time_height_ink, &time_height);
   if (panel.time2_format) {
      config_get_text_size(panel.time2_font_desc, &date_height_ink, &date_height);
      
      panel.time1_posy = (panel.clock_height - (date_height + time_height)) / 2;
      panel.time1_posy -= (time_height-time_height_ink) / 4;
      
      panel.time2_posy = panel.time1_posy + time_height;
   }
   else {
      panel.time1_posy = (panel.clock_height - time_height) / 2;
   }
   panel.redraw_clock = 1;
}


void config_finish ()
{
   int height_ink, height;
   
   if (panel.old_config_file) save_config();

   // get monitor's configuration
   get_monitors();

   if (panel.monitor > (server.nb_monitor-1)) {
      panel.sleep_mode = 1;
      printf("tint sleep and wait monitor %d.\n", panel.monitor+1);
   }
   else {
      panel.sleep_mode = 0;
      //printf("tint wake up on monitor %d\n", panel.monitor+1);
      if (!server.monitor[panel.monitor].width || !server.monitor[panel.monitor].height) 
         fprintf(stderr, "tint error : invalid monitor size.\n");
   }
      
   if (!panel.width) panel.width = server.monitor[panel.monitor].width;
   panel.task_posy = panel.border.width + panel.paddingy;
   panel.task_height = panel.height - (2 * panel.paddingy) - (2 * panel.border.width);
   //panel.launcher_width = ((panel.paddingx + panel.task_height) * launcher.launcher_count);
   panel.launcher_width = 0;
   
   if (!panel.task_maximum_width)
      panel.task_maximum_width = server.monitor[panel.monitor].width;
         
   if (panel.border.rounded > panel.height/2)
      panel.border.rounded = panel.height/2;
   
   config_clock();

   // compute vertical position : text and icon
   config_get_text_size(panel.task_font_desc, &height_ink, &height);
   panel.task_text_posy = (panel.task_height - height) / 2.0;

   // add 1/6 of task_icon_size
   panel.task_text_posx = panel.task_padding + panel.task_border.width;
   if (panel.task_icon_size)
      panel.task_text_posx += panel.task_icon_size + (panel.task_icon_size/6.0);
   panel.task_icon_posy = (panel.task_height - panel.task_icon_size) / 2;
   
   config_taskbar();   
}


int config_read ()
{
   const gchar * const * system_dirs;
   char *path1, *path2, *dir;
   gint i;

   // check tintrc file according to XDG specification
   path1 = g_build_filename (g_get_user_config_dir(), "tint", "tintrc", NULL);
   if (!g_file_test (path1, G_FILE_TEST_EXISTS)) {
      
      path2 = 0;
      system_dirs = g_get_system_config_dirs();
      for (i = 0; system_dirs[i]; i++) {
         path2 = g_build_filename(system_dirs[i], "tint", "tintrc", NULL);
         
         if (g_file_test(path2, G_FILE_TEST_EXISTS)) break;
         g_free (path2);
         path2 = 0;
      }
      
      if (path2) {
         // copy file in user directory (path1)
         dir = g_build_filename (g_get_user_config_dir(), "tint", NULL);
         if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) g_mkdir(dir, 0777);
         g_free(dir);
   
         copy_file(path2, path1);
         g_free(path2);
      }
   }

   i = config_read_file (path1);
   g_free(path1);
   return i;
}


int config_read_file (const char *path)
{
   FILE *fp;
   char line[80];
   
   if ((fp = fopen(path, "r")) == NULL) return 0;
   
   while (fgets(line, sizeof(line), fp) != NULL)
      parse_line (line);

   fclose (fp);
   return 1;
}


void save_config ()
{
   fprintf(stderr, "tint warning : convert user's config file\n");
   panel.paddingx = panel.paddingy = panel.marginx;
   panel.marginx = panel.marginy = 0;
   
   if (panel.old_task_icon == 0) panel.task_icon_size = 0;
   if (panel.old_panel_background == 0) panel.background.alpha = 0;
   if (panel.old_task_background == 0) {
      panel.task_back.alpha = 0;
      panel.task_active_back.alpha = 0;
   }
   panel.task_border.rounded = panel.task_border.rounded / 2;
   panel.task_active_border.rounded = panel.task_border.rounded;
   panel.border.rounded = panel.border.rounded / 2;
   
   char *path;
   FILE *fp;

   path = g_build_filename (g_get_user_config_dir(), "tint", "tintrc", NULL);
   fp = fopen(path, "w");
   g_free(path);
   if (fp == NULL) return;
   
   fputs("#---------------------------------------------\n", fp);
   fputs("# TINT CONFIG FILE\n", fp);
   fputs("#---------------------------------------------\n\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("# PANEL\n", fp);
   fputs("#---------------------------------------------\n", fp);
   if (panel.mode == SINGLE_DESKTOP) fputs("panel_mode = single_desktop\n", fp);
   else fputs("panel_mode = multi_desktop\n", fp);
   fputs("panel_monitor = 1\n", fp);
   if (panel.position & BOTTOM) fputs("panel_position = bottom", fp);
   else fputs("panel_position = top", fp);
   if (panel.position & LEFT) fputs(" left\n", fp);
   else if (panel.position & RIGHT) fputs(" right\n", fp);
   else fputs(" center\n", fp);
   fprintf(fp, "panel_size = %d %d\n", panel.width, panel.height);
   fprintf(fp, "panel_margin = %d %d\n", panel.marginx, panel.marginy);
   fprintf(fp, "panel_padding = %d %d\n", panel.paddingx, panel.paddingy);
   fprintf(fp, "font_shadow = %d\n", panel.font_shadow);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# PANEL BACKGROUND AND BORDER\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fprintf(fp, "panel_rounded = %d\n", panel.border.rounded);
   fprintf(fp, "panel_border_width = %d\n", panel.border.width);
   fprintf(fp, "panel_background_color = #%02x%02x%02x %d\n", (int)(panel.background.color[0]*255), (int)(panel.background.color[1]*255), (int)(panel.background.color[2]*255), (int)(panel.background.alpha*100));
   fprintf(fp, "panel_border_color = #%02x%02x%02x %d\n", (int)(panel.border.color[0]*255), (int)(panel.border.color[1]*255), (int)(panel.border.color[2]*255), (int)(panel.border.alpha*100));
   
   fputs("\n#---------------------------------------------\n", fp);
   fputs("# TASKS\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fprintf(fp, "task_text_centered = %d\n", panel.task_text_centered);
   fprintf(fp, "task_width = %d\n", panel.task_maximum_width);
   fprintf(fp, "task_margin = %d\n", panel.task_margin);
   fprintf(fp, "task_padding = %d\n", panel.task_padding);
   fprintf(fp, "task_icon_size = %d\n", panel.task_icon_size);
   fprintf(fp, "task_font = %s\n", panel.old_task_font);
   fprintf(fp, "task_font_color = #%02x%02x%02x %d\n", (int)(panel.task_font.color[0]*255), (int)(panel.task_font.color[1]*255), (int)(panel.task_font.color[2]*255), (int)(panel.task_font.alpha*100));
   fprintf(fp, "task_active_font_color = #%02x%02x%02x %d\n", (int)(panel.task_font_active.color[0]*255), (int)(panel.task_font_active.color[1]*255), (int)(panel.task_font_active.color[2]*255), (int)(panel.task_font_active.alpha*100));

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# TASK BACKGROUND AND BORDER\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fprintf(fp, "task_rounded = %d\n", panel.task_border.rounded);
   fprintf(fp, "task_background_color = #%02x%02x%02x %d\n", (int)(panel.task_back.color[0]*255), (int)(panel.task_back.color[1]*255), (int)(panel.task_back.color[2]*255), (int)(panel.task_back.alpha*100));
   fprintf(fp, "task_active_background_color = #%02x%02x%02x %d\n", (int)(panel.task_active_back.color[0]*255), (int)(panel.task_active_back.color[1]*255), (int)(panel.task_active_back.color[2]*255), (int)(panel.task_active_back.alpha*100));
   fprintf(fp, "task_border_width = %d\n", panel.task_border.width);
   fprintf(fp, "task_border_color = #%02x%02x%02x %d\n", (int)(panel.task_border.color[0]*255), (int)(panel.task_border.color[1]*255), (int)(panel.task_border.color[2]*255), (int)(panel.task_border.alpha*100));
   fprintf(fp, "task_active_border_color = #%02x%02x%02x %d\n", (int)(panel.task_active_border.color[0]*255), (int)(panel.task_active_border.color[1]*255), (int)(panel.task_active_border.color[2]*255), (int)(panel.task_active_border.alpha*100));

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# CLOCK\n", fp);
   fputs("#---------------------------------------------\n", fp);
   fputs("#time1_format = %H:%M\n", fp);
   fputs("time1_font = sans bold 8\n", fp);
   fputs("#time2_format = %A %d %B\n", fp);
   fputs("time2_font = sans 6\n", fp);
   fputs("clock_font_color = #ffffff 75\n", fp);

   fputs("\n#---------------------------------------------\n", fp);
   fputs("# MOUSE ACTION ON TASK\n", fp);
   fputs("#---------------------------------------------\n", fp);
   if (panel.mouse_middle == NONE) fputs("mouse_middle = none\n", fp);
   else if (panel.mouse_middle == CLOSE) fputs("mouse_middle = close\n", fp);
   else if (panel.mouse_middle == TOGGLE) fputs("mouse_middle = toggle\n", fp);
   else if (panel.mouse_middle == ICONIFY) fputs("mouse_middle = iconify\n", fp);
   else if (panel.mouse_middle == SHADE) fputs("mouse_middle = shade\n", fp);
   else fputs("mouse_middle = toggle_iconify\n", fp);

   if (panel.mouse_right == NONE) fputs("mouse_right = none\n", fp);
   else if (panel.mouse_right == CLOSE) fputs("mouse_right = close\n", fp);
   else if (panel.mouse_right == TOGGLE) fputs("mouse_right = toggle\n", fp);
   else if (panel.mouse_right == ICONIFY) fputs("mouse_right = iconify\n", fp);
   else if (panel.mouse_right == SHADE) fputs("mouse_right = shade\n", fp);
   else fputs("mouse_right = toggle_iconify\n", fp);
   
   if (panel.mouse_scroll_up == NONE) fputs("mouse_scroll_up = none\n", fp);
   else if (panel.mouse_scroll_up == CLOSE) fputs("mouse_scroll_up = close\n", fp);
   else if (panel.mouse_scroll_up == TOGGLE) fputs("mouse_scroll_up = toggle\n", fp);
   else if (panel.mouse_scroll_up == ICONIFY) fputs("mouse_scroll_up = iconify\n", fp);
   else if (panel.mouse_scroll_up == SHADE) fputs("mouse_scroll_up = shade\n", fp);
   else fputs("mouse_scroll_up = toggle_iconify\n", fp);
   
   if (panel.mouse_scroll_down == NONE) fputs("mouse_scroll_down = none\n", fp);
   else if (panel.mouse_scroll_down == CLOSE) fputs("mouse_scroll_down = close\n", fp);
   else if (panel.mouse_scroll_down == TOGGLE) fputs("mouse_scroll_down = toggle\n", fp);
   else if (panel.mouse_scroll_down == ICONIFY) fputs("mouse_scroll_down = iconify\n", fp);
   else if (panel.mouse_scroll_down == SHADE) fputs("mouse_scroll_down = shade\n", fp);
   else fputs("mouse_scroll_down = toggle_iconify\n", fp);

   fputs("\n\n", fp);
   fclose (fp);

   panel.old_config_file = 0;
}

