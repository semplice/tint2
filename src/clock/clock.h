/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
*
* Clock with fonctionnal data (timeval, precision) and drawing data (area, font, ...).
* Each panel use his own drawing data.
*
**************************************************************************/

#ifndef CLOCK_H
#define CLOCK_H

#include <sys/time.h>
#include "common.h"
#include "area.h"

typedef struct Clock {
	// always start with area
	Area area;

	Color font;
	int time1_posy;
	int time2_posy;
} Clock;

extern char *time1_format;
extern char *time1_timezone;
extern char *time2_format;
extern char *time2_timezone;
extern char *time_tooltip_format;
extern char *time_tooltip_timezone;
extern gboolean time1_has_font;
extern PangoFontDescription *time1_font_desc;
extern gboolean time2_has_font;
extern PangoFontDescription *time2_font_desc;
extern char *clock_lclick_command;
extern char *clock_mclick_command;
extern char *clock_rclick_command;
extern char *clock_uwheel_command;
extern char *clock_dwheel_command;
extern gboolean clock_enabled;

// default global data
void default_clock();

// freed memory
void cleanup_clock();

// initialize clock : y position, precision, ...
void init_clock();
void init_clock_panel(void *panel);
void clock_default_font_changed();

void draw_clock(void *obj, cairo_t *c);

gboolean resize_clock(void *obj);

void clock_action(int button);

#endif
