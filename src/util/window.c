/**************************************************************************
*
* Tint2 : common windows function
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Imlib2.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "common.h"
#include "window.h"
#include "server.h"
#include "panel.h"
#include "taskbar.h"



void set_active (Window win)
{
	send_event32 (win, server.atom._NET_ACTIVE_WINDOW, 2, CurrentTime, 0);
}


void set_desktop (int desktop)
{
	send_event32 (server.root_win, server.atom._NET_CURRENT_DESKTOP, desktop, 0, 0);
}


void windows_set_desktop (Window win, int desktop)
{
	send_event32 (win, server.atom._NET_WM_DESKTOP, desktop, 2, 0);
}


void set_close (Window win)
{
	send_event32 (win, server.atom._NET_CLOSE_WINDOW, 0, 2, 0);
}


void window_toggle_shade (Window win)
{
	send_event32 (win, server.atom._NET_WM_STATE, 2, server.atom._NET_WM_STATE_SHADED, 0);
}


void window_maximize_restore (Window win)
{
	send_event32 (win, server.atom._NET_WM_STATE, 2, server.atom._NET_WM_STATE_MAXIMIZED_VERT, 0);
	send_event32 (win, server.atom._NET_WM_STATE, 2, server.atom._NET_WM_STATE_MAXIMIZED_HORZ, 0);
}


int window_is_hidden (Window win)
{
	Window window;
	Atom *at;
	int count, i;

	at = server_get_property (win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_SKIP_TASKBAR) {
			XFree(at);
			return 1;
		}
		// do not add transient_for windows if the transient window is already in the taskbar
		window=win;
		while ( XGetTransientForHint(server.dsp, window, &window) ) {
			if ( task_get_tasks(window) ) {
				XFree(at);
				return 1;
			}
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
	XFree(at);

	for (i=0 ; i < nb_panel ; i++) {
		if (panel1[i].main_win == win) {
			return 1;
		}
	}

	// specification
	// Windows with neither _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set
	// MUST be taken as top-level window.
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
	int best_match = -1;
	int match_right = 0;
	int match_bottom = 0;
	// There is an ambiguity when a window is right on the edge between screens.
	// In that case, prefer the monitor which is on the right and bottom of the window's top-left corner.
	for (i = 0; i < server.nb_monitor; i++) {
		if (x >= server.monitor[i].x && x <= (server.monitor[i].x + server.monitor[i].width))
			if (y >= server.monitor[i].y && y <= (server.monitor[i].y + server.monitor[i].height)) {
				int current_right = x < (server.monitor[i].x + server.monitor[i].width);
				int current_bottom = y < (server.monitor[i].y + server.monitor[i].height);
				if (best_match < 0 ||
					(!match_right && current_right) ||
					(!match_bottom && current_bottom)) {
					best_match = i;
				}
			}
	}

	if (best_match < 0)
		best_match = 0;
	// printf("window %lx : ecran %d, (%d, %d)\n", win, best_match+1, x, y);
	return best_match;
}

void window_get_coordinates (Window win, int *x, int *y, int *w, int *h)
{
	int dummy_int;
	unsigned ww, wh, bw, bh;
 	Window src;
 	XTranslateCoordinates(server.dsp, win, server.root_win, 0, 0, x, y, &src);
	XGetGeometry(server.dsp, win, &src, &dummy_int, &dummy_int, &ww, &wh, &bw, &bh);
	*w = ww + bw;
	*h = wh + bh;
}

int window_is_iconified (Window win)
{
	// EWMH specification : minimization of windows use _NET_WM_STATE_HIDDEN.
	// WM_STATE is not accurate for shaded window and in multi_desktop mode.
	Atom *at;
	int count, i;
	at = server_get_property (win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_HIDDEN) {
			XFree(at);
			return 1;
		}
	}
	XFree(at);
	return 0;
}


int window_is_urgent (Window win)
{
	Atom *at;
	int count, i;

	at = server_get_property (win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (i = 0; i < count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_DEMANDS_ATTENTION) {
			XFree(at);
			return 1;
		}
	}
	XFree(at);
	return 0;
}


int window_is_skip_taskbar (Window win)
{
	Atom *at;
	int count, i;

	at = server_get_property(win, server.atom._NET_WM_STATE, XA_ATOM, &count);
	for (i=0; i<count; i++) {
		if (at[i] == server.atom._NET_WM_STATE_SKIP_TASKBAR) {
			XFree(at);
			return 1;
		}
	}
	XFree(at);
	return 0;
}



GSList *server_get_name_of_desktop ()
{
	int count, j;
	GSList *list = NULL;
	gchar *data_ptr, *ptr;
	data_ptr = server_get_property (server.root_win, server.atom._NET_DESKTOP_NAMES, server.atom.UTF8_STRING, &count);
	if (data_ptr) {
		list = g_slist_append(list, g_strdup(data_ptr));
		for (j = 0; j < count-1; j++) {
			if (*(data_ptr + j)	== '\0') {
				ptr = (gchar*)data_ptr + j + 1;
				list = g_slist_append(list, g_strdup(ptr));
			}
		}
		XFree(data_ptr);
	}
	return list;
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


int get_icon_count (gulong *data, int num)
{
	int count, pos, w, h;

	count = 0;
	pos = 0;
	while (pos+2 < num) {
		w = data[pos++];
		h = data[pos++];
		pos += w * h;
		if (pos > num || w <= 0 || h <= 0) break;
		count++;
	}

	return count;
}


gulong *get_best_icon (gulong *data, int icon_count, int num, int *iw, int *ih, int best_icon_size)
{
	int width[icon_count], height[icon_count], pos, i, w, h;
	gulong *icon_data[icon_count];

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
		if (width[i] == best_icon_size) {
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


void get_text_size2(PangoFontDescription *font,
					int *height_ink,
					int *height,
					int *width,
					int panel_height,
					int panel_width,
					char *text,
					int len,
					PangoWrapMode wrap,
					PangoEllipsizeMode ellipsis)
{
	PangoRectangle rect_ink, rect;

	Pixmap pmap = XCreatePixmap (server.dsp, server.root_win, panel_height, panel_width, server.depth);

	cairo_surface_t *cs = cairo_xlib_surface_create (server.dsp, pmap, server.visual, panel_height, panel_width);
	cairo_t *c = cairo_create (cs);

	PangoLayout *layout = pango_cairo_create_layout (c);
	pango_layout_set_width(layout, panel_width * PANGO_SCALE);
	pango_layout_set_height(layout, panel_height * PANGO_SCALE);
	pango_layout_set_wrap(layout, wrap);
	pango_layout_set_ellipsize(layout, ellipsis);
	pango_layout_set_font_description (layout, font);
	pango_layout_set_text (layout, text, len);

	pango_layout_get_pixel_extents(layout, &rect_ink, &rect);
	*height_ink = rect_ink.height;
	*height = rect.height;
	*width = rect.width;
	//printf("dimension : %d - %d\n", rect_ink.height, rect.height);

	g_object_unref (layout);
	cairo_destroy (c);
	cairo_surface_destroy (cs);
	XFreePixmap (server.dsp, pmap);
}


