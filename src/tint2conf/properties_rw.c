#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "common.h"
#include "properties.h"
#include "properties_rw.h"

void finalize_bg();
void add_entry(char *key, char *value);
void hex2gdk(char *hex, GdkColor *color);
void set_action(char *event, GtkWidget *combo);
char *get_action(GtkWidget *combo);

int config_has_panel_items;
int config_has_battery;
int config_has_systray;
int config_battery_enabled;
int config_systray_enabled;
int no_items_clock_enabled;
int no_items_systray_enabled;
int no_items_battery_enabled;

static int num_bg;
static int read_bg_color_hover;
static int read_border_color_hover;
static int read_bg_color_press;
static int read_border_color_press;

void config_read_file(const char *path)
{
	num_bg = 0;
	background_create_new();

	FILE *fp;
	char line[512];
	char *key, *value;

	if ((fp = fopen(path, "r")) == NULL)
		return;

	config_has_panel_items = 0;
	config_has_battery = 0;
	config_battery_enabled = 0;
	config_has_systray = 0;
	config_systray_enabled = 0;
	no_items_clock_enabled = 0;
	no_items_systray_enabled = 0;
	no_items_battery_enabled = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (parse_line(line, &key, &value)) {
			add_entry(key, value);
			free(key);
			free(value);
		}
	}
	fclose(fp);

	finalize_bg();

	if (!config_has_panel_items) {
		char panel_items[256];
		panel_items[0] = 0;
		strcat(panel_items, "T");
		if (config_has_battery) {
			if (config_battery_enabled)
				strcat(panel_items, "B");
		} else {
			if (no_items_battery_enabled)
				strcat(panel_items, "B");
		}
		if (config_has_systray) {
			if (config_systray_enabled)
				strcat(panel_items, "S");
		} else {
			if (no_items_systray_enabled)
				strcat(panel_items, "S");
		}
		if (no_items_clock_enabled)
			strcat(panel_items, "C");
		set_panel_items(panel_items);
	}
}

void config_write_color(FILE *fp, const char *name, GdkColor color, int opacity)
{
	fprintf(fp,
			"%s = #%02x%02x%02x %d\n",
			name,
			color.red >> 8,
			color.green >> 8,
			color.blue >> 8,
			opacity);
}

void config_write_backgrounds(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Backgrounds\n");

	int index;
	for (index = 1; ; index++) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_indices(index, -1);
		gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
		gtk_tree_path_free(path);

		if (!found) {
			break;
		}

		int r;
		int b;
		GdkColor *fillColor;
		int fillOpacity;
		GdkColor *borderColor;
		int borderOpacity;
		GdkColor *fillColorOver;
		int fillOpacityOver;
		GdkColor *borderColorOver;
		int borderOpacityOver;
		GdkColor *fillColorPress;
		int fillOpacityPress;
		GdkColor *borderColorPress;
		int borderOpacityPress;
		gchar *text;

		gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
						   bgColFillColor, &fillColor,
						   bgColFillOpacity, &fillOpacity,
						   bgColBorderColor, &borderColor,
						   bgColBorderOpacity, &borderOpacity,
						   bgColFillColorOver, &fillColorOver,
						   bgColFillOpacityOver, &fillOpacityOver,
						   bgColBorderColorOver, &borderColorOver,
						   bgColBorderOpacityOver, &borderOpacityOver,
						   bgColFillColorPress, &fillColorPress,
						   bgColFillOpacityPress, &fillOpacityPress,
						   bgColBorderColorPress, &borderColorPress,
						   bgColBorderOpacityPress, &borderOpacityPress,
						   bgColBorderWidth, &b,
						   bgColCornerRadius, &r,
						   bgColText, &text,
						   -1);
		fprintf(fp, "# Background %d: %s\n", index, text ? text : "");
		fprintf(fp, "rounded = %d\n", r);
		fprintf(fp, "border_width = %d\n", b);
		config_write_color(fp, "background_color", *fillColor, fillOpacity);
		config_write_color(fp, "border_color", *borderColor, borderOpacity);
		config_write_color(fp, "background_color_hover", *fillColorOver, fillOpacityOver);
		config_write_color(fp, "border_color_hover", *borderColorOver, borderOpacityOver);
		config_write_color(fp, "background_color_pressed", *fillColorPress, fillOpacityPress);
		config_write_color(fp, "border_color_pressed", *borderColorPress, borderOpacityPress);
		fprintf(fp, "\n");
	}
}

void config_write_panel(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Panel\n");
	char *items = get_panel_items();
	fprintf(fp, "panel_items = %s\n", items);
	free(items);
	fprintf(fp, "panel_size = %d%s %d%s\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_width)),
			gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_width_type)) == 0 ? "%" : "",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_height)),
			gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_height_type)) == 0 ? "%" : "");
	fprintf(fp, "panel_margin = %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_margin_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_margin_y)));
	fprintf(fp, "panel_padding = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_padding_y)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_spacing)));
	fprintf(fp, "panel_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(panel_background)));
	fprintf(fp, "wm_menu = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_wm_menu)) ? 1 : 0);
	fprintf(fp, "panel_dock = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_dock)) ? 1 : 0);

	fprintf(fp, "panel_position = ");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLH]))) {
		fprintf(fp, "bottom left horizontal");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BCH]))) {
		fprintf(fp, "bottom center horizontal");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRH]))) {
		fprintf(fp, "bottom right horizontal");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLH]))) {
		fprintf(fp, "top left horizontal");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TCH]))) {
		fprintf(fp, "top center horizontal");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRH]))) {
		fprintf(fp, "top right horizontal");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLV]))) {
		fprintf(fp, "top left vertical");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_CLV]))) {
		fprintf(fp, "center left vertical");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLV]))) {
		fprintf(fp, "bottom left vertical");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRV]))) {
		fprintf(fp, "top right vertical");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_CRV]))) {
		fprintf(fp, "center right vertical");
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRV]))) {
		fprintf(fp, "bottom right vertical");
	}
	fprintf(fp, "\n");

	fprintf(fp, "panel_layer = ");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_layer)) == 0) {
		fprintf(fp, "top");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_layer)) == 1) {
		fprintf(fp, "normal");
	} else {
		fprintf(fp, "bottom");
	}
	fprintf(fp, "\n");

	fprintf(fp, "panel_monitor = ");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_monitor)) == 0) {
		fprintf(fp, "all");
	} else {
		fprintf(fp, "%d", gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_monitor)));
	}
	fprintf(fp, "\n");

	fprintf(fp, "autohide = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_autohide)) ? 1 : 0);
	fprintf(fp, "autohide_show_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_autohide_show_time)));
	fprintf(fp, "autohide_hide_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_autohide_hide_time)));
	fprintf(fp, "autohide_height = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_autohide_size)));

	fprintf(fp, "strut_policy = ");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_strut_policy)) == 0) {
		fprintf(fp, "follow_size");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_strut_policy)) == 1) {
		fprintf(fp, "minimum");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_strut_policy)) == 2) {
		fprintf(fp, "none");
	}
	fprintf(fp, "\n");

	fprintf(fp, "panel_window_name = %s\n", gtk_entry_get_text(GTK_ENTRY(panel_window_name)));
	fprintf(fp, "disable_transparency = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(disable_transparency)) ? 1 : 0);
	fprintf(fp, "mouse_effects = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_mouse_effects)) ? 1 : 0);
	fprintf(fp, "font_shadow = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(font_shadow)) ? 1 : 0);
	fprintf(fp,
			"mouse_hover_icon_asb = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_hover_icon_opacity)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_hover_icon_saturation)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_hover_icon_brightness)));
	fprintf(fp,
			"mouse_pressed_icon_asb = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_pressed_icon_opacity)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_pressed_icon_saturation)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_pressed_icon_brightness)));


	fprintf(fp, "\n");
}

void config_write_taskbar(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Taskbar\n");

	fprintf(fp,
			"taskbar_mode = %s\n",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_show_desktop)) ?
				"multi_desktop" :
				"single_desktop");
	fprintf(fp,
			"taskbar_padding = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_padding_y)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_spacing)));
	fprintf(fp, "taskbar_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_inactive_background)));
	fprintf(fp, "taskbar_active_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_active_background)));
	fprintf(fp, "taskbar_name = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_show_name)) ? 1 : 0);
	fprintf(fp, "taskbar_hide_inactive_tasks = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_hide_inactive_tasks)) ? 1 : 0);
	fprintf(fp, "taskbar_hide_different_monitor = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_hide_diff_monitor)) ? 1 : 0);
	fprintf(fp,
			"taskbar_name_padding = %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_name_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_name_padding_y)));
	fprintf(fp, "taskbar_name_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_name_inactive_background)));
	fprintf(fp, "taskbar_name_active_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_name_active_background)));
	fprintf(fp, "taskbar_name_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(taskbar_name_font)));

	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(taskbar_name_inactive_color), &color);
	config_write_color(fp,
					   "taskbar_name_font_color",
					   color,
					   gtk_color_button_get_alpha(GTK_COLOR_BUTTON(taskbar_name_inactive_color)) * 100 / 0xffff);

	gtk_color_button_get_color(GTK_COLOR_BUTTON(taskbar_name_active_color), &color);
	config_write_color(fp,
					   "taskbar_name_active_font_color",
					   color,
					   gtk_color_button_get_alpha(GTK_COLOR_BUTTON(taskbar_name_active_color)) * 100 / 0xffff);

	fprintf(fp, "taskbar_distribute_size = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_distribute_size)) ? 1 : 0);

	fprintf(fp, "taskbar_sort_order = ");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) <= 0) {
		fprintf(fp, "none");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) == 1) {
		fprintf(fp, "title");
	} else {
		fprintf(fp, "center");
	}
	fprintf(fp, "\n");

	fprintf(fp, "task_align = ");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_alignment)) <= 0) {
		fprintf(fp, "left");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_alignment)) == 1) {
		fprintf(fp, "center");
	} else {
		fprintf(fp, "right");
	}
	fprintf(fp, "\n");

	fprintf(fp, "\n");
}

void config_write_task_font_color(FILE *fp, char *name, GtkWidget *task_color)
{
	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(task_color), &color);
	char full_name[128];
	sprintf(full_name, "task%s_font_color", name);
	config_write_color(fp,
					   full_name,
					   color,
					   gtk_color_button_get_alpha(GTK_COLOR_BUTTON(task_color)) * 100 / 0xffff);
}

void config_write_task_icon_osb(FILE *fp,
								char *name,
								GtkWidget *widget_opacity,
								GtkWidget *widget_saturation,
								GtkWidget *widget_brightness)
{
	char full_name[128];
	sprintf(full_name, "task%s_icon_asb", name);
	fprintf(fp,
			"%s = %d %d %d\n",
			full_name,
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget_opacity)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget_saturation)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget_brightness)));
}

void config_write_task_background(FILE *fp, char *name, GtkWidget *task_background)
{
	char full_name[128];
	sprintf(full_name, "task%s_background_id", name);
	fprintf(fp, "%s = %d\n", full_name, gtk_combo_box_get_active(GTK_COMBO_BOX(task_background)));
}

void config_write_task(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Task\n");

	fprintf(fp, "task_text = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_show_text)) ? 1 : 0);
	fprintf(fp, "task_icon = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_show_icon)) ? 1 : 0);
	fprintf(fp, "task_centered = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_align_center)) ? 1 : 0);
	fprintf(fp, "urgent_nb_of_blink = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_urgent_blinks)));
	fprintf(fp,
			"task_maximum_size = %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_maximum_width)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_maximum_height)));
	fprintf(fp,
			"task_padding = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_padding_y)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_spacing)));
	fprintf(fp, "task_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(task_font)));
	fprintf(fp, "task_tooltip = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tooltip_task_show)) ? 1 : 0);

	// same for: "" _normal _active _urgent _iconified
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_default_color_set))) {
		config_write_task_font_color(fp, "", task_default_color);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_normal_color_set))) {
		config_write_task_font_color(fp, "_normal", task_normal_color);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_active_color_set))) {
		config_write_task_font_color(fp, "_active", task_active_color);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_urgent_color_set))) {
		config_write_task_font_color(fp, "_urgent", task_urgent_color);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_iconified_color_set))) {
		config_write_task_font_color(fp, "_iconified", task_iconified_color);
	}

	// same for: "" _normal _active _urgent _iconified
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_default_icon_osb_set))) {
		config_write_task_icon_osb(fp, "",
								   task_default_icon_opacity,
								   task_default_icon_saturation,
								   task_default_icon_brightness);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_normal_icon_osb_set))) {
		config_write_task_icon_osb(fp, "_normal",
								   task_normal_icon_opacity,
								   task_normal_icon_saturation,
								   task_normal_icon_brightness);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_active_icon_osb_set))) {
		config_write_task_icon_osb(fp, "_active",
								   task_active_icon_opacity,
								   task_active_icon_saturation,
								   task_active_icon_brightness);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_urgent_icon_osb_set))) {
		config_write_task_icon_osb(fp, "_urgent",
								   task_urgent_icon_opacity,
								   task_urgent_icon_saturation,
								   task_urgent_icon_brightness);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_iconified_icon_osb_set))) {
		config_write_task_icon_osb(fp, "_iconified",
								   task_iconified_icon_opacity,
								   task_iconified_icon_saturation,
								   task_iconified_icon_brightness);
	}

	// same for: "" _normal _active _urgent _iconified
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_default_background_set))) {
		config_write_task_background(fp, "", task_default_background);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_normal_background_set))) {
		config_write_task_background(fp, "_normal", task_normal_background);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_active_background_set))) {
		config_write_task_background(fp, "_active", task_active_background);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_urgent_background_set))) {
		config_write_task_background(fp, "_urgent", task_urgent_background);
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_iconified_background_set))) {
		config_write_task_background(fp, "_iconified", task_iconified_background);
	}


	fprintf(fp, "mouse_left = %s\n", get_action(task_mouse_left));
	fprintf(fp, "mouse_middle = %s\n", get_action(task_mouse_middle));
	fprintf(fp, "mouse_right = %s\n", get_action(task_mouse_right));
	fprintf(fp, "mouse_scroll_up = %s\n", get_action(task_mouse_scroll_up));
	fprintf(fp, "mouse_scroll_down = %s\n", get_action(task_mouse_scroll_down));

	fprintf(fp, "\n");
}

void config_write_systray(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# System tray (notification area)\n");

	fprintf(fp,
			"systray_padding = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_padding_y)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_spacing)));
	fprintf(fp, "systray_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(systray_background)));

	fprintf(fp, "systray_sort = ");
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 0) {
		fprintf(fp, "ascending");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 1) {
		fprintf(fp, "descending");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 2) {
		fprintf(fp, "left2right");
	} else if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 3) {
		fprintf(fp, "right2left");
	}
	fprintf(fp, "\n");

	fprintf(fp, "systray_icon_size = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_size)));
	fprintf(fp,
			"systray_icon_asb = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_opacity)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_saturation)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_brightness)));

	fprintf(fp, "systray_monitor = ");
	fprintf(fp, "%d", MAX(1, 1 + gtk_combo_box_get_active(GTK_COMBO_BOX(systray_monitor))));
	fprintf(fp, "\n");

	fprintf(fp, "\n");
}

void config_write_launcher(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Launcher\n");

	fprintf(fp,
			"launcher_padding = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_padding_y)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_spacing)));
	fprintf(fp, "launcher_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(launcher_background)));
	fprintf(fp, "launcher_icon_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(launcher_icon_background)));
	fprintf(fp, "launcher_icon_size = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_size)));
	fprintf(fp,
			"launcher_icon_asb = %d %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_opacity)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_saturation)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_brightness)));
	gchar *icon_theme = get_current_icon_theme();
	if (icon_theme && !g_str_equal(icon_theme, "")) {
		fprintf(fp, "launcher_icon_theme = %s\n", icon_theme);
		g_free(icon_theme);
		icon_theme = NULL;
	}
	fprintf(fp, "launcher_icon_theme_override = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(launcher_icon_theme_override)) ? 1 : 0);
	fprintf(fp, "startup_notifications = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(startup_notifications)) ? 1 : 0);
	fprintf(fp, "launcher_tooltip = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(launcher_tooltip)) ? 1 : 0);

	int index;
	for (index = 0; ; index++) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_indices(index, -1);
		gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(launcher_apps), &iter, path);
		gtk_tree_path_free(path);

		if (!found) {
			break;
		}

		gchar *app_path;
		gtk_tree_model_get(GTK_TREE_MODEL(launcher_apps), &iter,
						   appsColPath, &app_path,
						   -1);
		char *contracted = contract_tilde(app_path);
		fprintf(fp, "launcher_item_app = %s\n", contracted);
		free(contracted);
		g_free(app_path);
	}

	gchar **app_dirs = g_strsplit(gtk_entry_get_text(GTK_ENTRY(launcher_apps_dirs)), ",", 0);
	for (index = 0; app_dirs[index]; index++) {
		gchar *dir = app_dirs[index];
		g_strstrip(dir);
		if (strlen(dir) > 0) {
			char *contracted = contract_tilde(dir);
			fprintf(fp, "launcher_apps_dir = %s\n", contracted);
			free(contracted);
		}
	}
	g_strfreev(app_dirs);

	fprintf(fp, "\n");
}

void config_write_clock(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Clock\n");

	fprintf(fp, "time1_format = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_format_line1)));
	fprintf(fp, "time2_format = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_format_line2)));
	fprintf(fp, "time1_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(clock_font_line1)));
	fprintf(fp, "time1_timezone = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_tmz_line1)));
	fprintf(fp, "time2_timezone = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_tmz_line2)));
	fprintf(fp, "time2_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(clock_font_line2)));

	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(clock_font_color), &color);
	config_write_color(fp,
					   "clock_font_color",
					   color,
					   gtk_color_button_get_alpha(GTK_COLOR_BUTTON(clock_font_color)) * 100 / 0xffff);

	fprintf(fp,
			"clock_padding = %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock_padding_y)));
	fprintf(fp, "clock_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(clock_background)));
	fprintf(fp, "clock_tooltip = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_format_tooltip)));
	fprintf(fp, "clock_tooltip_timezone = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_tmz_tooltip)));
	fprintf(fp, "clock_lclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_left_command)));
	fprintf(fp, "clock_rclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_right_command)));
	fprintf(fp, "clock_mclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_mclick_command)));
	fprintf(fp, "clock_uwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_uwheel_command)));
	fprintf(fp, "clock_dwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_dwheel_command)));

	fprintf(fp, "\n");
}

void config_write_battery(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Battery\n");

	fprintf(fp, "battery_tooltip = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(battery_tooltip)) ? 1 : 0);
	fprintf(fp, "battery_low_status = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_alert_if_lower)));
	fprintf(fp, "battery_low_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_alert_cmd)));
	fprintf(fp, "bat1_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(battery_font_line1)));
	fprintf(fp, "bat2_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(battery_font_line2)));
	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(battery_font_color), &color);
	config_write_color(fp,
					   "battery_font_color",
					   color,
					   gtk_color_button_get_alpha(GTK_COLOR_BUTTON(battery_font_color)) * 100 / 0xffff);
	fprintf(fp,
			"battery_padding = %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_padding_y)));
	fprintf(fp, "battery_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(battery_background)));
	fprintf(fp, "battery_hide = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_hide_if_higher)));
	fprintf(fp, "battery_lclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_left_command)));
	fprintf(fp, "battery_rclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_right_command)));
	fprintf(fp, "battery_mclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_mclick_command)));
	fprintf(fp, "battery_uwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_uwheel_command)));
	fprintf(fp, "battery_dwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_dwheel_command)));

	fprintf(fp, "ac_connected_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(ac_connected_cmd)));
	fprintf(fp, "ac_disconnected_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(ac_disconnected_cmd)));

	fprintf(fp, "\n");
}

void config_write_tooltip(FILE *fp)
{
	fprintf(fp, "#-------------------------------------\n");
	fprintf(fp, "# Tooltip\n");

	fprintf(fp, "tooltip_show_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_show_after)));
	fprintf(fp, "tooltip_hide_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_hide_after)));
	fprintf(fp,
			"tooltip_padding = %d %d\n",
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_padding_x)),
			(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_padding_y)));
	fprintf(fp, "tooltip_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(tooltip_background)));

	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(tooltip_font_color), &color);
	config_write_color(fp,
					   "tooltip_font_color",
					   color,
					   gtk_color_button_get_alpha(GTK_COLOR_BUTTON(tooltip_font_color)) * 100 / 0xffff);

	fprintf(fp, "tooltip_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(tooltip_font)));

	fprintf(fp, "\n");
}

// Similar to BSD checksum, except we skip the first line (metadata)
unsigned short checksum_txt(FILE *f)
{
    unsigned int checksum = 0;
	fseek(f, 0, SEEK_SET);

	// Skip the first line
	int c;
	do {
	  c = getc(f);
	} while (c != EOF && c != '\n');

	while ((c = getc(f)) != EOF) {
		// Rotate right
        checksum = (checksum >> 1) + ((checksum & 1) << 15);
        // Update checksum
		checksum += c;
		// Truncate to 16 bits
        checksum &= 0xffff;
    }
    return checksum;
}

void config_save_file(const char *path) {
	printf("config_save_file : %s\n", path);

	FILE *fp;
	if ((fp = fopen(path, "w+t")) == NULL)
		return;

	unsigned short checksum = 0;
	fprintf(fp, "#---- Generated by tint2conf %04x ----\n", checksum);
	fprintf(fp, "# See https://gitlab.com/o9000/tint2/wikis/Configure for \n");
	fprintf(fp, "# full documentation of the configuration options.\n");

	config_write_backgrounds(fp);
	config_write_panel(fp);
	config_write_taskbar(fp);
	config_write_task(fp);
	config_write_systray(fp);
	config_write_launcher(fp);
	config_write_clock(fp);
	config_write_battery(fp);
	config_write_tooltip(fp);

	checksum = checksum_txt(fp);
	fseek(fp, 0, SEEK_SET);
	fflush(fp);
	fprintf(fp, "#---- Generated by tint2conf %04x ----\n", checksum);

	fclose(fp);
}

gboolean config_is_manual(const char *path)
{
	FILE *fp;
	char line[512];
	gboolean result;

	if ((fp = fopen(path, "r")) == NULL)
		return FALSE;

	result = TRUE;
	if (fgets(line, sizeof(line), fp) != NULL) {
		if (!g_regex_match_simple("^#---- Generated by tint2conf [0-9a-f][0-9a-f][0-9a-f][0-9a-f] ----\n$", line, 0, 0)) {
			result = TRUE;
		} else {
			unsigned short checksum1 = checksum_txt(fp);
			unsigned short checksum2 = 0;
			if (sscanf(line, "#---- Generated by tint2conf %hxu", &checksum2) == 1) {
				result = checksum1 != checksum2;
			} else {
				result = TRUE;
			}
		}
	}
	fclose(fp);
	return result;
}

void finalize_bg()
{
	if (num_bg > 0) {
		if (!read_bg_color_hover) {
			GdkColor fillColor;
			gtk_color_button_get_color(GTK_COLOR_BUTTON(background_fill_color), &fillColor);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color_over), &fillColor);
			int fillOpacity = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_fill_color));
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color_over), fillOpacity);
			background_force_update();
		}
		if (!read_border_color_hover) {
			GdkColor fillColor;
			gtk_color_button_get_color(GTK_COLOR_BUTTON(background_border_color), &fillColor);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color_over), &fillColor);
			int fillOpacity = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_border_color));
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color_over), fillOpacity);
			background_force_update();
		}
		if (!read_bg_color_press) {
			GdkColor fillColor;
			gtk_color_button_get_color(GTK_COLOR_BUTTON(background_fill_color), &fillColor);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color_press), &fillColor);
			int fillOpacity = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_fill_color));
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color_press), fillOpacity);
			background_force_update();
		}
		if (!read_border_color_press) {
			GdkColor fillColor;
			gtk_color_button_get_color(GTK_COLOR_BUTTON(background_border_color_over), &fillColor);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color_press), &fillColor);
			int fillOpacity = gtk_color_button_get_alpha(GTK_COLOR_BUTTON(background_border_color_over));
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color_press), fillOpacity);
			background_force_update();
		}
	}
}

void add_entry(char *key, char *value)
{
	char *value1=0, *value2=0, *value3=0;

	/* Background and border */
	if (strcmp(key, "rounded") == 0) {
		// 'rounded' is the first parameter => alloc a new background
		finalize_bg();
		background_create_new();
		num_bg++;
		read_bg_color_hover = 0;
		read_border_color_hover = 0;
		read_bg_color_press = 0;
		read_border_color_press = 0;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_corner_radius), atoi(value));
		background_force_update();
	}
	else if (strcmp(key, "border_width") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_width), atoi(value));
		background_force_update();
	}
	else if (strcmp(key, "background_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color), &col);
		int alpha = value2 ? atoi(value2) : 50;
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color), (alpha*65535)/100);
		background_force_update();
	}
	else if (strcmp(key, "border_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color), &col);
		int alpha = value2 ? atoi(value2) : 50;
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color), (alpha*65535)/100);
		background_force_update();
	}
	else if (strcmp(key, "background_color_hover") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color_over), &col);
		int alpha = value2 ? atoi(value2) : 50;
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color_over), (alpha*65535)/100);
		background_force_update();
		read_bg_color_hover = 1;
	}
	else if (strcmp(key, "border_color_hover") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color_over), &col);
		int alpha = value2 ? atoi(value2) : 50;
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color_over), (alpha*65535)/100);
		background_force_update();
		read_border_color_hover = 1;
	}
	else if (strcmp(key, "background_color_pressed") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(background_fill_color_press), &col);
		int alpha = value2 ? atoi(value2) : 50;
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_fill_color_press), (alpha*65535)/100);
		background_force_update();
		read_bg_color_press = 1;
	}
	else if (strcmp(key, "border_color_pressed") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(background_border_color_press), &col);
		int alpha = value2 ? atoi(value2) : 50;
		gtk_color_button_set_alpha(GTK_COLOR_BUTTON(background_border_color_press), (alpha*65535)/100);
		background_force_update();
		read_border_color_press = 1;
	}

	/* Panel */
	else if (strcmp(key, "panel_size") == 0) {
		extract_values(value, &value1, &value2, &value3);
		char *b;
		if ((b = strchr(value1, '%'))) {
			b[0] = '\0';
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 0);
		}
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_width), atoi(value1));
		if (atoi(value1) == 0) {
			// full width mode
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_width), 100);
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 0);
		}
		if ((b = strchr(value2, '%'))) {
			b[0] = '\0';
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_height_type), 0);
		}
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_height_type), 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_height), atoi(value2));
	}
	else if (strcmp(key, "panel_items") == 0) {
		config_has_panel_items = 1;
		set_panel_items(value);
	}
	else if (strcmp(key, "panel_margin") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_margin_x), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_margin_y), atoi(value2));
	}
	else if (strcmp(key, "panel_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_padding_x), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_spacing), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_padding_y), atoi(value2));
		if (value3) gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_spacing), atoi(value3));
	}
	else if (strcmp(key, "panel_position") == 0) {
		extract_values(value, &value1, &value2, &value3);

		char vpos, hpos, orientation;

		vpos = 'B';
		hpos = 'C';
		orientation = 'H';

		if (value1) {
			if (strcmp(value1, "top") == 0)
				vpos = 'T';
			if (strcmp(value1, "bottom") == 0)
				vpos = 'B';
			if (strcmp(value1, "center") == 0)
				vpos = 'C';
		}

		if (value2) {
			if (strcmp(value2, "left") == 0)
				hpos = 'L';
			if (strcmp(value2, "right") == 0)
				hpos = 'R';
			if (strcmp(value2, "center") == 0)
				hpos = 'C';
		}

		if (value3) {
			if (strcmp(value3, "horizontal") == 0)
				orientation = 'H';
			if (strcmp(value3, "vertical") == 0)
				orientation = 'V';
		}

		if (vpos == 'T' && hpos == 'L' && orientation == 'H')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLH]), 1);
		if (vpos == 'T' && hpos == 'C' && orientation == 'H')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TCH]), 1);
		if (vpos == 'T' && hpos == 'R' && orientation == 'H')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRH]), 1);

		if (vpos == 'B' && hpos == 'L' && orientation == 'H')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLH]), 1);
		if (vpos == 'B' && hpos == 'C' && orientation == 'H')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BCH]), 1);
		if (vpos == 'B' && hpos == 'R' && orientation == 'H')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRH]), 1);

		if (vpos == 'T' && hpos == 'L' && orientation == 'V')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLV]), 1);
		if (vpos == 'C' && hpos == 'L' && orientation == 'V')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_CLV]), 1);
		if (vpos == 'B' && hpos == 'L' && orientation == 'V')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLV]), 1);

		if (vpos == 'T' && hpos == 'R' && orientation == 'V')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRV]), 1);
		if (vpos == 'C' && hpos == 'R' && orientation == 'V')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_CRV]), 1);
		if (vpos == 'B' && hpos == 'R' && orientation == 'V')
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRV]), 1);
	}
	else if (strcmp(key, "panel_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(panel_background), id);
	}
	else if (strcmp(key, "panel_window_name") == 0) {
		gtk_entry_set_text(GTK_ENTRY(panel_window_name), value);
	}
	else if (strcmp(key, "disable_transparency") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable_transparency), atoi(value));
	}
	else if (strcmp(key, "mouse_effects") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_mouse_effects), atoi(value));
	}
	else if (strcmp(key, "mouse_hover_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_opacity), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_saturation), atoi(value2));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_brightness), atoi(value3));
	}
	else if (strcmp(key, "mouse_pressed_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_opacity), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_saturation), atoi(value2));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_brightness), atoi(value3));
	}
	else if (strcmp(key, "font_shadow") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(font_shadow), atoi(value));
	}
	else if (strcmp(key, "wm_menu") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_wm_menu), atoi(value));
	}
	else if (strcmp(key, "panel_dock") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_dock), atoi(value));
	}
	else if (strcmp(key, "panel_layer") == 0) {
		if (strcmp(value, "bottom") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 2);
		else if (strcmp(value, "top") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 0);
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 1);
	}
	else if (strcmp(key, "panel_monitor") == 0) {
		if (strcmp(value, "all") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 0);
		else if (strcmp(value, "1") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 1);
		else if (strcmp(value, "2") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 2);
		else if (strcmp(value, "3") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 3);
		else if (strcmp(value, "4") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 4);
		else if (strcmp(value, "5") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 5);
		else if (strcmp(value, "6") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 6);
	}
	
	/* autohide options */
	else if (strcmp(key, "autohide") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_autohide), atoi(value));
	}
	else if (strcmp(key, "autohide_show_timeout") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_show_time), atof(value));
	}
	else if (strcmp(key, "autohide_hide_timeout") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_hide_time), atof(value));
	}
	else if (strcmp(key, "strut_policy") == 0) {
		if (strcmp(value, "follow_size") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 0);
		else if (strcmp(value, "none") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 2);
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 1);
	}
	else if (strcmp(key, "autohide_height") == 0) {
		if (atoi(value) <= 0) {
			// autohide need height > 0
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_size), 1);
		} else {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_size), atoi(value));
		}
	}

	/* Battery */
	else if (strcmp(key, "battery") == 0) {
		// Obsolete option
		config_has_battery = 1;
		config_battery_enabled = atoi(value);
	} else if (strcmp(key, "battery_tooltip") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(battery_tooltip), atoi(value));
	} else if (strcmp(key, "battery_low_status") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_alert_if_lower), atof(value));
	}
	else if (strcmp(key, "battery_low_cmd") == 0) {
		gtk_entry_set_text(GTK_ENTRY(battery_alert_cmd), value);
	}
	else if (strcmp(key, "bat1_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(battery_font_line1), value);
	}
	else if (strcmp(key, "bat2_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(battery_font_line2), value);
	}
	else if (strcmp(key, "battery_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(battery_font_color), &col);
		if (value2) {
			int alpha = atoi(value2);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(battery_font_color), (alpha*65535)/100);
		}
	}
	else if (strcmp(key, "battery_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_padding_x), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_padding_y), atoi(value2));
	}
	else if (strcmp(key, "battery_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(battery_background), id);
	}
	else if (strcmp(key, "battery_hide") == 0) {
		int percentage_hide = atoi(value);
		if (percentage_hide == 0)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_hide_if_higher), 101.0);
		else
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_hide_if_higher), atoi(value));
	}
	else if (strcmp(key, "battery_lclick_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(battery_left_command), value);
	}
	else if (strcmp(key, "battery_rclick_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(battery_right_command), value);
	}
	else if (strcmp(key, "battery_mclick_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(battery_mclick_command), value);
	}
	else if (strcmp(key, "battery_uwheel_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(battery_uwheel_command), value);
	}
	else if (strcmp(key, "battery_dwheel_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(battery_dwheel_command), value);
	}
	else if (strcmp(key, "ac_connected_cmd") == 0) {
		gtk_entry_set_text(GTK_ENTRY(ac_connected_cmd), value);
	}
	else if (strcmp(key, "ac_disconnected_cmd") == 0) {
		gtk_entry_set_text(GTK_ENTRY(ac_disconnected_cmd), value);
	}

	/* Clock */
	else if (strcmp(key, "time1_format") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_format_line1), value);
		no_items_clock_enabled = strlen(value) > 0;
	}
	else if (strcmp(key, "time2_format") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_format_line2), value);
	}
	else if (strcmp(key, "time1_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(clock_font_line1), value);
	}
	else if (strcmp(key, "time1_timezone") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_tmz_line1), value);
	}
	else if (strcmp(key, "time2_timezone") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_tmz_line2), value);
	}
	else if (strcmp(key, "time2_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(clock_font_line2), value);
	}
	else if (strcmp(key, "clock_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(clock_font_color), &col);
		if (value2) {
			int alpha = atoi(value2);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(clock_font_color), (alpha*65535)/100);
		}
	}
	else if (strcmp(key, "clock_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock_padding_x), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock_padding_y), atoi(value2));
	}
	else if (strcmp(key, "clock_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(clock_background), id);
	}
	else if (strcmp(key, "clock_tooltip") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_format_tooltip), value);
	}
	else if (strcmp(key, "clock_tooltip_timezone") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_tmz_tooltip), value);
	}
	else if (strcmp(key, "clock_lclick_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_left_command), value);
	}
	else if (strcmp(key, "clock_rclick_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_right_command), value);
	}
	else if (strcmp(key, "clock_mclick_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_mclick_command), value);
	}
	else if (strcmp(key, "clock_uwheel_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_uwheel_command), value);
	}
	else if (strcmp(key, "clock_dwheel_command") == 0) {
		gtk_entry_set_text(GTK_ENTRY(clock_dwheel_command), value);
	}

	/* Taskbar */
	else if (strcmp(key, "taskbar_mode") == 0) {
		if (strcmp(value, "multi_desktop") == 0)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_show_desktop), 1);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_show_desktop), 0);
	}
	else if (strcmp(key, "taskbar_distribute_size") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_distribute_size), atoi(value));
	}
	else if (strcmp(key, "taskbar_sort_order") == 0) {
		if (strcmp(value, "none") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 0);
		else if (strcmp(value, "title") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 1);
		else if (strcmp(value, "center") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 2);
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 0);
	}
	else if (strcmp(key, "task_align") == 0) {
		if (strcmp(value, "left") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 0);
		else if (strcmp(value, "center") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 1);
		else if (strcmp(value, "right") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 2);
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 0);
	}
	else if (strcmp(key, "taskbar_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_padding_x), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_spacing), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_padding_y), atoi(value2));
		if (value3) gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_spacing), atoi(value3));
	}
	else if (strcmp(key, "taskbar_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_inactive_background), id);
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_active_background)) < 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_active_background), id);
	}
	else if (strcmp(key, "taskbar_active_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_active_background), id);
	}
	else if (strcmp(key, "taskbar_name") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_show_name), atoi(value));
	}
	else if (strcmp(key, "taskbar_hide_inactive_tasks") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_hide_inactive_tasks), atoi(value));
	}
	else if (strcmp(key, "taskbar_hide_different_monitor") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_hide_diff_monitor), atoi(value));
	}
	else if (strcmp(key, "taskbar_name_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_name_padding_x), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_name_padding_y), atoi(value2));
	}
	else if (strcmp(key, "taskbar_name_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_name_inactive_background), id);
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_name_active_background)) < 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_name_active_background), id);
	}
	else if (strcmp(key, "taskbar_name_active_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_name_active_background), id);
	}
	else if (strcmp(key, "taskbar_name_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(taskbar_name_font), value);
	}
	else if (strcmp(key, "taskbar_name_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(taskbar_name_inactive_color), &col);
		if (value2) {
			int alpha = atoi(value2);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(taskbar_name_inactive_color), (alpha*65535)/100);
		}
	}
	else if (strcmp(key, "taskbar_name_active_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(taskbar_name_active_color), &col);
		if (value2) {
			int alpha = atoi(value2);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(taskbar_name_active_color), (alpha*65535)/100);
		}
	}

	/* Task */
	else if (strcmp(key, "task_text") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_show_text), atoi(value));
	}
	else if (strcmp(key, "task_icon") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_show_icon), atoi(value));
	}
	else if (strcmp(key, "task_centered") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_align_center), atoi(value));
	}
	else if (strcmp(key, "urgent_nb_of_blink") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_urgent_blinks), atoi(value));
	}
	else if (strcmp(key, "task_width") == 0) {
		// old parameter : just for backward compatibility
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_width), atoi(value));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_height), 30.0);
	}
	else if (strcmp(key, "task_maximum_size") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_width), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_height), 30.0);
		if (value2)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_height), atoi(value2));
	}
	else if (strcmp(key, "task_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_padding_x), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_spacing), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_padding_y), atoi(value2));
		if (value3) gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_spacing), atoi(value3));
	}
	else if (strcmp(key, "task_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(task_font), value);
	}
	else if (g_regex_match_simple("task.*_font_color", key, 0, 0)) {
		gchar** split = g_regex_split_simple("_", key, 0, 0);
		GtkWidget *widget = NULL;
		if (strcmp(split[1], "normal") == 0) {
			widget = task_normal_color;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_normal_color_set), 1);
		} else if (strcmp(split[1], "active") == 0) {
			widget = task_active_color;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_active_color_set), 1);
		} else if (strcmp(split[1], "urgent") == 0) {
			widget = task_urgent_color;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_urgent_color_set), 1);
		} else if (strcmp(split[1], "iconified") == 0) {
			widget = task_iconified_color;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_iconified_color_set), 1);
		} else if (strcmp(split[1], "font") == 0) {
			widget = task_default_color;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_default_color_set), 1);
		}
		g_strfreev(split);
		if (widget) {
			extract_values(value, &value1, &value2, &value3);
			GdkColor col;
			hex2gdk(value1, &col);
			gtk_color_button_set_color(GTK_COLOR_BUTTON(widget), &col);
			if (value2) {
				int alpha = atoi(value2);
				gtk_color_button_set_alpha(GTK_COLOR_BUTTON(widget), (alpha*65535)/100);
			}
		}
	}
	else if (g_regex_match_simple("task.*_icon_asb", key, 0, 0)) {
		gchar** split = g_regex_split_simple("_", key, 0, 0);
		GtkWidget *widget_opacity = NULL;
		GtkWidget *widget_saturation = NULL;
		GtkWidget *widget_brightness = NULL;
		if (strcmp(split[1], "normal") == 0) {
			widget_opacity = task_normal_icon_opacity;
			widget_saturation = task_normal_icon_saturation;
			widget_brightness = task_normal_icon_brightness;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_normal_icon_osb_set), 1);
		} else if (strcmp(split[1], "active") == 0) {
			widget_opacity = task_active_icon_opacity;
			widget_saturation = task_active_icon_saturation;
			widget_brightness = task_active_icon_brightness;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_active_icon_osb_set), 1);
		} else if (strcmp(split[1], "urgent") == 0) {
			widget_opacity = task_urgent_icon_opacity;
			widget_saturation = task_urgent_icon_saturation;
			widget_brightness = task_urgent_icon_brightness;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_urgent_icon_osb_set), 1);
		} else if (strcmp(split[1], "iconified") == 0) {
			widget_opacity = task_iconified_icon_opacity;
			widget_saturation = task_iconified_icon_saturation;
			widget_brightness = task_iconified_icon_brightness;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_iconified_icon_osb_set), 1);
		} else if (strcmp(split[1], "icon") == 0) {
			widget_opacity = task_default_icon_opacity;
			widget_saturation = task_default_icon_saturation;
			widget_brightness = task_default_icon_brightness;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_default_icon_osb_set), 1);
		}
		g_strfreev(split);
		if (widget_opacity && widget_saturation && widget_brightness) {
			extract_values(value, &value1, &value2, &value3);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_opacity), atoi(value1));
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_saturation), atoi(value2));
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_brightness), atoi(value3));
		}
	}
	else if (g_regex_match_simple("task.*_background_id", key, 0, 0)) {
		gchar** split = g_regex_split_simple("_", key, 0, 0);
		GtkWidget *widget = NULL;
		if (strcmp(split[1], "normal") == 0) {
			widget = task_normal_background;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_normal_background_set), 1);
		} else if (strcmp(split[1], "active") == 0) {
			widget = task_active_background;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_active_background_set), 1);
		} else if (strcmp(split[1], "urgent") == 0) {
			widget = task_urgent_background;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_urgent_background_set), 1);
		} else if (strcmp(split[1], "iconified") == 0) {
			widget = task_iconified_background;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_iconified_background_set), 1);
		} else if (strcmp(split[1], "background") == 0) {
			widget = task_default_background;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_default_background_set), 1);
		}
		g_strfreev(split);
		if (widget) {
			int id = background_index_safe(atoi(value));
			gtk_combo_box_set_active(GTK_COMBO_BOX(widget), id);
		}
	}
	// "tooltip" is deprecated but here for backwards compatibility
	else if (strcmp(key, "task_tooltip") == 0 || strcmp(key, "tooltip") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooltip_task_show), atoi(value));
	}

	/* Systray */
	else if (strcmp(key, "systray") == 0) {
		// Obsolete option
		config_has_systray = 1;
		config_systray_enabled = atoi(value);
	}
	else if (strcmp(key, "systray_padding") == 0) {
		no_items_systray_enabled = 1;

		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_padding_x), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_spacing), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_padding_y), atoi(value2));
		if (value3) gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_spacing), atoi(value3));
	}
	else if (strcmp(key, "systray_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(systray_background), id);
	}
	else if (strcmp(key, "systray_sort") == 0) {
		if (strcmp(value, "descending") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_icon_order), 1);
		else if (strcmp(value, "ascending") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_icon_order), 0);
		else if (strcmp(value, "right2left") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_icon_order), 3);
		else  // default to left2right
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_icon_order), 2);
	}
	else if (strcmp(key, "systray_icon_size") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_size), atoi(value));
	}
	else if (strcmp(key, "systray_monitor") == 0) {
		if (strcmp(value, "1") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 0);
		else if (strcmp(value, "2") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 1);
		else if (strcmp(value, "3") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 2);
		else if (strcmp(value, "4") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 3);
		else if (strcmp(value, "5") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 4);
		else if (strcmp(value, "6") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 5);
	}
	else if (strcmp(key, "systray_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_opacity), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_saturation), atoi(value2));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_brightness), atoi(value3));
	}

	/* Launcher */
	else if (strcmp(key, "launcher_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_padding_x), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_spacing), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_padding_y), atoi(value2));
		if (value3) gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_spacing), atoi(value3));
	}
	else if (strcmp(key, "launcher_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(launcher_background), id);
	}
	else if (strcmp(key, "launcher_icon_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(launcher_icon_background), id);
	}
	else if (strcmp(key, "launcher_icon_size") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_size), atoi(value));
	}
	else if (strcmp(key, "launcher_item_app") == 0) {
		char *path = expand_tilde(value);
		load_desktop_file(path, TRUE);
		load_desktop_file(path, FALSE);
		free(path);
	}
	else if (strcmp(key, "launcher_apps_dir") == 0) {
		char *path = expand_tilde(value);

		if (gtk_entry_get_text_length(GTK_ENTRY(launcher_apps_dirs)) > 0) {
			gtk_entry_append_text(GTK_ENTRY(launcher_apps_dirs), ",");
		}
		gtk_entry_append_text(GTK_ENTRY(launcher_apps_dirs), path);

		free(path);
	}
	else if (strcmp(key, "launcher_icon_theme") == 0) {
		set_current_icon_theme(value);
	}
	else if (strcmp(key, "launcher_icon_theme_override") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(launcher_icon_theme_override), atoi(value));
	}
	else if (strcmp(key, "launcher_tooltip") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(launcher_tooltip), atoi(value));
	}
	else if (strcmp(key, "startup_notifications") == 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(startup_notifications), atoi(value));
	}
	else if (strcmp(key, "launcher_icon_asb") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_opacity), atoi(value1));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_saturation), atoi(value2));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_brightness), atoi(value3));
	}

	/* Tooltip */
	else if (strcmp(key, "tooltip_show_timeout") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_show_after), atof(value));
	}
	else if (strcmp(key, "tooltip_hide_timeout") == 0) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_hide_after), atof(value));
	}
	else if (strcmp(key, "tooltip_padding") == 0) {
		extract_values(value, &value1, &value2, &value3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_padding_x), atoi(value1));
		if (value2) gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_padding_y), atoi(value2));
	}
	else if (strcmp(key, "tooltip_background_id") == 0) {
		int id = background_index_safe(atoi(value));
		gtk_combo_box_set_active(GTK_COMBO_BOX(tooltip_background), id);
	}
	else if (strcmp(key, "tooltip_font_color") == 0) {
		extract_values(value, &value1, &value2, &value3);
		GdkColor col;
		hex2gdk(value1, &col);
		gtk_color_button_set_color(GTK_COLOR_BUTTON(tooltip_font_color), &col);
		if (value2) {
			int alpha = atoi(value2);
			gtk_color_button_set_alpha(GTK_COLOR_BUTTON(tooltip_font_color), (alpha*65535)/100);
		}
	}
	else if (strcmp(key, "tooltip_font") == 0) {
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(tooltip_font), value);
	}

	/* Mouse actions */
	else if (strcmp(key, "mouse_left") == 0) {
		set_action(value, task_mouse_left);
	}
	else if (strcmp(key, "mouse_middle") == 0) {
		set_action(value, task_mouse_middle);
	}
	else if (strcmp(key, "mouse_right") == 0) {
		set_action(value, task_mouse_right);
	}
	else if (strcmp(key, "mouse_scroll_up") == 0) {
		set_action(value, task_mouse_scroll_up);
	}
	else if (strcmp(key, "mouse_scroll_down") == 0) {
		set_action(value, task_mouse_scroll_down);
	}

	if (value1) free(value1);
	if (value2) free(value2);
	if (value3) free(value3);
}

void hex2gdk(char *hex, GdkColor *color)
{
	if (hex == NULL || hex[0] != '#') return;

	color->red   = 257 * (hex_char_to_int(hex[1]) * 16 + hex_char_to_int(hex[2]));
	color->green = 257 * (hex_char_to_int(hex[3]) * 16 + hex_char_to_int(hex[4]));
	color->blue  = 257 * (hex_char_to_int(hex[5]) * 16 + hex_char_to_int(hex[6]));
	color->pixel = 0;
}

void set_action(char *event, GtkWidget *combo)
{
	if (strcmp(event, "none") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	else if (strcmp(event, "close") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);
	else if (strcmp(event, "toggle") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 2);
	else if (strcmp(event, "iconify") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 3);
	else if (strcmp(event, "shade") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 4);
	else if (strcmp(event, "toggle_iconify") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 5);
	else if (strcmp(event, "maximize_restore") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 6);
	else if (strcmp(event, "desktop_left") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 7);
	else if (strcmp(event, "desktop_right") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 8);
	else if (strcmp(event, "next_task") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 9);
	else if (strcmp(event, "prev_task") == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 10);
}

char *get_action(GtkWidget *combo)
{
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 0)
		return "none";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 1)
		return "close";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 2)
		return "toggle";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 3)
		return "iconify";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 4)
		return "shade";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 5)
		return "toggle_iconify";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 6)
		return "maximize_restore";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 7)
		return "desktop_left";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 8)
		return "desktop_right";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 9)
		return "next_task";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 10)
		return "prev_task";
	return "none";
}
