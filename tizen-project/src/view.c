#include <jsc_sensors.h>
#include <watch_app_efl.h>
#include <system_settings.h>
#include "jsc_sw_watch_face.h"
#include "view.h"
#include "view_defines.h"

#define MAIN_EDJ "edje/main.edj"

static struct view_info {
	Evas_Object *win;
	Evas_Object *layout;
	int w;
	int h;
	int lang;
	int day;
	int month;
	int battery;
} s_info = { .win = NULL, .layout = NULL, .w = 0, .h = 0, .lang = 0, .day = 0,
		.month = 0 };

char months[2][12][8] = { { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		"Aug", "Sep", "Oct", "Nov", "Dec" }, { "Janv", "Fevr", "Mars", "Avri",
		"Mai", "Juin", "Juil", "Aout", "Sep", "Oct", "Nov", "Dec" } };

/*
 * @brief Creates the application's UI with window's width and height preset.
 */
void view_create_with_size(int width, int height) {
	s_info.w = width;
	s_info.h = height;

	view_create();
}

/*
 * @brief Create Essential Object window and layout
 */
void view_create(void) {
	evas_font_path_global_append(SW_OUT_ITALIC);
	s_info.win = view_create_win(PACKAGE);
	if (!s_info.win) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a window.");
		return;
	}

	s_info.layout = _create_layout();
	if (!s_info.layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create main layout.");
		return;
	}

	evas_object_show(s_info.win);
}

/*
 * @brief Makes a basic window named with package name.
 * @param[pkg_name]: Name of the window.
 */
Evas_Object *view_create_win(const char *pkg_name) {
	Evas_Object *win = NULL;
	int ret;

	/*
	 * Window
	 * Create and initialize elm_win.
	 * elm_win is mandatory to manipulate window
	 */
	ret = watch_app_get_elm_win(&win);
	if (ret != APP_ERROR_NONE)
		return NULL;

	elm_win_title_set(win, pkg_name);
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_HIDE);
	elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_BG_TRANSPARENT);
	elm_win_prop_focus_skip_set(win, EINA_TRUE);
	elm_win_role_set(win, "no-effect");

	evas_object_resize(win, s_info.w, s_info.h);

	return win;
}

/*
 * @brief Makes and sets a layout to the part
 * @param[parent]: Object to which you want to set this layout
 * @param[file_path]: File path of EDJ will be used
 * @param[group_name]: Group name in EDJ that you want to set to layout
 * @param[part_name]: Part name to which you want to set this layout
 */
Evas_Object *view_create_layout_for_part(Evas_Object *parent, char *file_path,
		char *group_name, char *part_name) {
	Evas_Object *layout = NULL;

	if (parent == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "parent is NULL.");
		return NULL;
	}

	layout = elm_layout_add(parent);
	elm_layout_file_set(layout, file_path, group_name);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);

	evas_object_show(layout);

	elm_object_part_content_set(parent, part_name, layout);

	return layout;
}

void view_set_language(char *locale) {
	if (strcmp("en_US", locale) == 0) {
		s_info.lang = 0;
	} else if (strcmp("fr_FR", locale) == 0) {
		s_info.lang = 1;
	}
}

void view_update_battery(int battery) {
	char battery_percent[8] = { 0, };
	snprintf(battery_percent, 8, "%d %%", battery);
	Evas_Object *layout = elm_layout_edje_get(s_info.layout);
	edje_object_part_text_set(layout, PART_BATTERY, battery_percent);
}

void update_pedometer(int steps) {
	char step_message[64];
	snprintf(step_message, 64, "%d", steps);
	Evas_Object *layout = elm_layout_edje_get(s_info.layout);
	edje_object_part_text_set(layout, PART_STEPS, step_message);
}

void view_update_pedometer() {
	get_today_count(update_pedometer);
}

/*
 * @brief Draws the clock's hands.
 * @param[current_time]: the structure of time components.
 */
void view_set_display_time(current_time_t current_time) {
	Evas_Object *layout = elm_layout_edje_get(s_info.layout);
	if (s_info.day != current_time.day) {
		char date_day[8] = { 0, };
		char date_month[8] = { 0, };

		snprintf(date_day, 4, "%s%d", current_time.day >= 10 ? "" : "0",
				current_time.day);
		if (current_time.month > 0) {
			snprintf(date_month, 6, "%s",
					months[s_info.lang][current_time.month - 1]);
		} else {
			snprintf(date_month, 6, "---");
		}

		edje_object_part_text_set(layout, PART_DATE_DAY, date_day);
		edje_object_part_text_set(layout, PART_DATE_MONTH, date_month);
		s_info.month = current_time.month;
		s_info.day = current_time.day;
	}

	view_update_pedometer();

	Edje_Message_Int_Set *msg = malloc(
			sizeof(Edje_Message_Int_Set) + 2 * sizeof(int));

	msg->count = 3;
	msg->val[0] = current_time.hour;
	msg->val[1] = current_time.minute;
	msg->val[2] = current_time.second;

	edje_object_message_send(elm_layout_edje_get(s_info.layout),
			EDJE_MESSAGE_INT_SET, MSG_ID_SET_TIME, msg);

	free(msg);
}

/*
 * @brief Toggles the ambient mode on (draws a second hand) and off (hides a second hand).
 * @param[current_time]: the structure of time components.
 */
void view_toggle_ambient_mode(bool ambient_mode) {
	Edje_Message_Int msg = { 0, };

	msg.val = ambient_mode ? 1 : 0;
	edje_object_message_send(elm_layout_edje_get(s_info.layout),
			EDJE_MESSAGE_INT,
			MSG_ID_AMBIENT_MODE, &msg);
}

/*
 * @brief Destroys the main window.
 */
void view_destroy(void) {
	if (s_info.win == NULL)
		return;
	destroy_pedometer();
	evas_object_del(s_info.win);
}

/*
 * @brief Creates path to the given resource file by concatenation of the basic resource path and the given file_name.
 * @param[file_name]: File name or path relative to the resource directory.
 * @return: The absolute path to the resource with given file_name is returned.
 */
static char *_create_resource_path(const char *file_name) {
	static char res_path_buff[PATH_MAX] = { 0, };
	char *res_path = NULL;

	res_path = app_get_resource_path();
	if (res_path == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get resource path.");
		return NULL;
	}

	snprintf(res_path_buff, PATH_MAX, "%s%s", res_path, file_name);
	free(res_path);

	return &res_path_buff[0];
}

/*
 * @brief Creates the application's layout.
 * @return: The Evas_Object of the layout created.
 */
static Evas_Object *_create_layout(void) {
	char *edj_path = NULL;

	edj_path = _create_resource_path(MAIN_EDJ);

	Evas_Object *layout = view_create_layout_for_part(s_info.win, edj_path,
			"main", "default");
	if (!layout)
		return NULL;

	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL,
	EVAS_HINT_FILL);
	evas_object_size_hint_min_set(layout, s_info.w, s_info.h);
	evas_object_resize(layout, s_info.w, s_info.h);
	evas_object_show(layout);
	init_pedometer();
	view_update_pedometer();

	return layout;
}
