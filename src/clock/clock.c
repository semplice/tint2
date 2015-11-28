/**************************************************************************
*
* Tint2 : clock
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <stdlib.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "clock.h"
#include "timer.h"
#include "common.h"


char *time1_format;
char *time1_timezone;
char *time2_format;
char *time2_timezone;
char *time_tooltip_format;
char *time_tooltip_timezone;
char *clock_lclick_command;
char *clock_mclick_command;
char *clock_rclick_command;
char *clock_uwheel_command;
char *clock_dwheel_command;
struct timeval time_clock;
PangoFontDescription *time1_font_desc;
PangoFontDescription *time2_font_desc;
static char buf_time[256];
static char buf_date[256];
static char buf_tooltip[512];
int clock_enabled;
static timeout* clock_timeout;


void default_clock()
{
	clock_enabled = 0;
	clock_timeout = NULL;
	time1_format = NULL;
	time1_timezone = NULL;
	time2_format = NULL;
	time2_timezone = NULL;
	time_tooltip_format = NULL;
	time_tooltip_timezone = NULL;
	clock_lclick_command = NULL;
	clock_mclick_command = NULL;
	clock_rclick_command = NULL;
	clock_uwheel_command = NULL;
	clock_dwheel_command = NULL;
	time1_font_desc = NULL;
	time2_font_desc = NULL;
}

void cleanup_clock()
{
	pango_font_description_free(time1_font_desc);
	time1_font_desc = NULL;
	pango_font_description_free(time2_font_desc);
	time2_font_desc = NULL;
	free(time1_format);
	time1_format = NULL;
	free(time2_format);
	time2_format = NULL;
	free(time_tooltip_format);
	time_tooltip_format = NULL;
	free(time1_timezone);
	time1_timezone = NULL;
	free(time2_timezone);
	time2_timezone = NULL;
	free(time_tooltip_timezone);
	time_tooltip_timezone = NULL;
	free(clock_lclick_command);
	clock_lclick_command = NULL;
	free(clock_mclick_command);
	clock_mclick_command = NULL;
	free(clock_rclick_command);
	clock_rclick_command = NULL;
	free(clock_uwheel_command);
	clock_uwheel_command = NULL;
	free(clock_dwheel_command);
	clock_dwheel_command = NULL;
	stop_timeout(clock_timeout);
	clock_timeout = NULL;
}


void update_clocks_sec(void* arg)
{
	gettimeofday(&time_clock, 0);
	int i;
	if (time1_format) {
		for (i=0 ; i < nb_panel ; i++)
			panel1[i].clock.area.resize = 1;
	}
	panel_refresh = 1;
}

void update_clocks_min(void* arg)
{
	// remember old_sec because after suspend/hibernate the clock should be updated directly, and not
	// on next minute change
	time_t old_sec = time_clock.tv_sec;
	gettimeofday(&time_clock, 0);
	if (time_clock.tv_sec % 60 == 0 || time_clock.tv_sec - old_sec > 60) {
		int i;
		if (time1_format) {
			for (i=0 ; i < nb_panel ; i++)
				panel1[i].clock.area.resize = 1;
		}
		panel_refresh = 1;
	}
}

struct tm* clock_gettime_for_tz(const char* timezone) {
	if (timezone) {
		const char* old_tz = getenv("TZ");
		setenv("TZ", timezone, 1);
		struct tm* result = localtime(&time_clock.tv_sec);
		if (old_tz) setenv("TZ", old_tz, 1);
		else unsetenv("TZ");
		return result;
	}
	else return localtime(&time_clock.tv_sec);
}

char* clock_get_tooltip(void* obj)
{
	strftime(buf_tooltip, sizeof(buf_tooltip), time_tooltip_format, clock_gettime_for_tz(time_tooltip_timezone));
	return strdup(buf_tooltip);
}

int time_format_needs_sec_ticks(char *time_format)
{
	if (!time_format)
		return 0;
	if (strchr(time_format, 'S') || strchr(time_format, 'T') || strchr(time_format, 'r'))
		return 1;
	return 0;
}

void init_clock()
{
	if (!clock_timeout) {
		if (time_format_needs_sec_ticks(time1_format) ||
			time_format_needs_sec_ticks(time2_format)) {
			clock_timeout = add_timeout(10, 1000, update_clocks_sec, 0, &clock_timeout);
		} else {
			clock_timeout = add_timeout(10, 1000, update_clocks_min, 0, &clock_timeout);
		}
	}
}


void init_clock_panel(void *p)
{
	Panel *panel =(Panel*)p;
	Clock *clock = &panel->clock;
	
	if (!time1_font_desc)
		time1_font_desc = pango_font_description_from_string(DEFAULT_FONT);
	if (!time2_font_desc)
		time2_font_desc = pango_font_description_from_string(DEFAULT_FONT);
	if (!clock->area.bg)
		clock->area.bg = &g_array_index(backgrounds, Background, 0);
	clock->area.parent = p;
	clock->area.panel = p;
	clock->area.mouse_press_effect = clock->area.mouse_over_effect = clock_lclick_command ||
																	 clock_mclick_command ||
																	 clock_rclick_command ||
																	 clock_uwheel_command ||
																	 clock_dwheel_command;
	clock->area._draw_foreground = draw_clock;
	clock->area.size_mode = SIZE_BY_CONTENT;
	clock->area._resize = resize_clock;
	// check consistency
	if (!time1_format)
		return;

	clock->area.resize = 1;
	clock->area.on_screen = 1;

	if (time_tooltip_format) {
		clock->area._get_tooltip_text = clock_get_tooltip;
		strftime(buf_tooltip, sizeof(buf_tooltip), time_tooltip_format, clock_gettime_for_tz(time_tooltip_timezone));
	}
}


void draw_clock (void *obj, cairo_t *c)
{
	Clock *clock = obj;
	PangoLayout *layout;

	layout = pango_cairo_create_layout (c);

	// draw layout
	pango_layout_set_font_description (layout, time1_font_desc);
	pango_layout_set_width (layout, clock->area.width * PANGO_SCALE);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
	pango_layout_set_text (layout, buf_time, strlen(buf_time));

	cairo_set_source_rgba (c, clock->font.color[0], clock->font.color[1], clock->font.color[2], clock->font.alpha);

	pango_cairo_update_layout (c, layout);
	draw_text(layout, c, 0, clock->time1_posy, &clock->font, ((Panel*)clock->area.panel)->font_shadow);

	if (time2_format) {
		pango_layout_set_font_description (layout, time2_font_desc);
		pango_layout_set_indent(layout, 0);
		pango_layout_set_text (layout, buf_date, strlen(buf_date));
		pango_layout_set_width (layout, clock->area.width * PANGO_SCALE);

		pango_cairo_update_layout (c, layout);
		draw_text(layout, c, 0, clock->time2_posy, &clock->font, ((Panel*)clock->area.panel)->font_shadow);
	}

	g_object_unref (layout);
}


int resize_clock (void *obj)
{
	Clock *clock = obj;
	Panel *panel = clock->area.panel;
	int time_height_ink, time_height, time_width, date_height_ink, date_height, date_width, ret = 0;

	clock->area.redraw = 1;
	
	date_height = date_width = 0;
	strftime(buf_time, sizeof(buf_time), time1_format, clock_gettime_for_tz(time1_timezone));
	get_text_size2(time1_font_desc, &time_height_ink, &time_height, &time_width, panel->area.height, panel->area.width, buf_time, strlen(buf_time),
				   PANGO_WRAP_WORD_CHAR,
				   PANGO_ELLIPSIZE_NONE);
	if (time2_format) {
		strftime(buf_date, sizeof(buf_date), time2_format, clock_gettime_for_tz(time2_timezone));
		get_text_size2(time2_font_desc, &date_height_ink, &date_height, &date_width, panel->area.height, panel->area.width, buf_date, strlen(buf_date),
					   PANGO_WRAP_WORD_CHAR,
					   PANGO_ELLIPSIZE_NONE);
	}

	if (panel_horizontal) {
		int new_size = (time_width > date_width) ? time_width : date_width;
		new_size += (2*clock->area.paddingxlr) + (2*clock->area.bg->border.width);
		if (new_size > clock->area.width || new_size < (clock->area.width-6)) {
			// we try to limit the number of resize
			clock->area.width = new_size + 1;
			clock->time1_posy = (clock->area.height - time_height) / 2;
			if (time2_format) {
				clock->time1_posy -= (date_height)/2;
				clock->time2_posy = clock->time1_posy + time_height;
			}
			ret = 1;
		}
	}
	else {
		int new_size = time_height + date_height + (2 * (clock->area.paddingxlr + clock->area.bg->border.width));
		if (new_size != clock->area.height) {
			// we try to limit the number of resize
			clock->area.height =  new_size;
			clock->time1_posy = (clock->area.height - time_height) / 2;
			if (time2_format) {
				clock->time1_posy -= (date_height)/2;
				clock->time2_posy = clock->time1_posy + time_height;
			}
			ret = 1;
		}
	}

	return ret;
}


void clock_action(int button)
{
	char *command = 0;
	switch (button) {
		case 1:
		command = clock_lclick_command;
		break;
		case 2:
		command = clock_mclick_command;
		break;
		case 3:
		command = clock_rclick_command;
		break;
		case 4:
		command = clock_uwheel_command;
		break;
		case 5:
		command = clock_dwheel_command;
		break;
	}
	tint_exec(command);
}

