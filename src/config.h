
#ifndef CONFIG_H
#define CONFIG_H

#include <pango/pangocairo.h>
#include <sys/time.h>
#include "task.h"


typedef struct config_border
{
   double color[3];
   double alpha;
   int width;
   int rounded;
} config_border;


typedef struct config_color
{
   double color[3];
   double alpha;
} config_color;


// mouse actions
enum { NONE=0, CLOSE, TOGGLE, ICONIFY, SHADE, TOGGLE_ICONIFY };

//panel mode
enum { SINGLE_DESKTOP=0, MULTI_DESKTOP, MULTI_MONITOR };

//panel alignment
enum { LEFT=0x01, RIGHT=0x02, CENTER=0X04, TOP=0X08, BOTTOM=0x10 };

#define index(i, j) ((i * panel.nb_monitor) + j)


typedef struct Panel {
   // --------------------------------------------------
   // backward compatibility
   int old_config_file;
   int old_task_icon;
   int old_panel_background;
   int old_task_background;
   char *old_task_font;

   // --------------------------------------------------
   // panel
   int signal_pending;
   int sleep_mode;
   int refresh;
   int mode;
   int monitor;
   int position;
   int marginx, marginy;
   int paddingx, paddingy;
   int posx, posy;
   int width, height;
   config_color background;
   config_border border;
   int font_shadow;

   // --------------------------------------------------
   // taskbar parameters. number of tasbar == nb_desktop x nb_monitor.
   Taskbar *taskbar;
   int    nb_desktop;
   int    nb_monitor;
   Task   *task_active;
   Task   *task_drag;
   
   // --------------------------------------------------
   // tasks parameters
   int task_posy;
   int task_height;
   int task_text_centered;
   int task_maximum_width;
   int task_margin;
   int task_padding;
   int task_icon_size;
   // icon position
   int task_icon_posy;
   // starting position for text ~ task_padding + task_border + icon_size
   double task_text_posx, task_text_posy;
   PangoFontDescription *task_font_desc;
   config_color task_font;
   config_color task_font_active;

   config_color task_back;
   config_color task_active_back;
   config_border task_border;
   config_border task_active_border;
   
   // --------------------------------------------------
   // clock
   int redraw_clock;
   int clock_posx;
   int clock_posy;
   int clock_width;
   int clock_height;
   Pixmap pmap_clock;
   PangoFontDescription *time1_font_desc;
   PangoFontDescription *time2_font_desc;
   config_color clock_font;
   int time1_posy;
   int time2_posy;
   char *time1_format;
   char *time2_format;
   struct timeval clock;
   int  time_precision;

   // --------------------------------------------------
   // launcher parameters
   int launcher_width;

   // --------------------------------------------------
   // mouse events
   int mouse_middle;
   int mouse_right;
   int mouse_scroll_up;
   int mouse_scroll_down;
} Panel;

Panel panel;


int  config_read_file (const char *path);
int  config_read ();
void config_taskbar();
void config_clock();
void config_finish ();
void cleanup_taskbar();
void cleanup ();
void save_config ();

#endif
