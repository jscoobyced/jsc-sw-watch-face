#include <tizen.h>
#include "jsc-sw-watch-face.h"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *hours;
	Evas_Object *minutes;
	Evas_Object *seconds;
	char hours_path[PATH_MAX];
	char minutes_path[PATH_MAX];
	char seconds_path[PATH_MAX];
} appdata_s;

#define TEXT_BUF_SIZE 256

char *hours_name = "jsc-sw-watch-face-hours.png";
char *minutes_name = "jsc-sw-watch-face-minutes.png";
char *seconds_name = "jsc-sw-watch-face-seconds.png";

static int hour_to_angle(int hour, int minute) {
	return (hour * 360 / 12) + (floor(minute / 12) * 360 / 60);
}

static int minute_or_second_to_angle(int minute) {
	return minute * 360 / 60;
}

static void display_hand(appdata_s *ad, Evas_Object *image, int angle,
		char *filename, int width, int height) {
	evas_object_image_file_set(image, filename, NULL);
	evas_object_move(image, 0, 0);
	evas_object_resize(image, width, height);
	Evas_Map *map = evas_map_new(4);
	evas_map_util_points_populate_from_object(map, image);
	evas_map_util_rotate(map, angle, width / 2, height / 2);
	evas_object_map_set(image, map);
	evas_object_map_enable_set(image, EINA_TRUE);
	evas_map_free(map);
	evas_object_show(image);
}

static void load_hand(appdata_s *ad, int hour, int minute, int second) {

	int width, height;
	Evas *evas = evas_object_evas_get(ad->conform);
	evas_output_size_get(evas, &width, &height);

	ad->hours = evas_object_image_filled_add(evas);
	display_hand(ad, ad->hours, hour_to_angle(hour, minute), ad->hours_path,
			width, height);

	ad->minutes = evas_object_image_filled_add(evas);
	display_hand(ad, ad->minutes, minute_or_second_to_angle(minute),
			ad->minutes_path, width, height);

	ad->seconds = evas_object_image_filled_add(evas);
	display_hand(ad, ad->seconds, minute_or_second_to_angle(second),
			ad->seconds_path, width, height);
}

static void update_watch(appdata_s *ad, watch_time_h watch_time, int ambient) {
	int hour, minute, second;

	if (watch_time == NULL)
		return;

	watch_time_get_hour(watch_time, &hour);
	watch_time_get_minute(watch_time, &minute);
	watch_time_get_second(watch_time, &second);
	evas_object_del(ad->hours);
	evas_object_del(ad->minutes);
	evas_object_del(ad->seconds);
	load_hand(ad, hour, minute, second);
}

static void create_base_gui(appdata_s *ad, int width, int height) {
	int ret;
	watch_time_h watch_time = NULL;

	/* Window */
	ret = watch_app_get_elm_win(&ad->win);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get window. err = %d", ret);
		return;
	}

	evas_object_resize(ad->win, width, height);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Image */
	char *res_dir_path = app_get_resource_path();

	snprintf(ad->hours_path, PATH_MAX, "%s%s%s", res_dir_path, "images/",
			hours_name);
	dlog_print(DLOG_INFO, LOG_TAG, "load image: %s.", ad->hours_path);

	snprintf(ad->minutes_path, PATH_MAX, "%s%s%s", res_dir_path, "images/",
			minutes_name);
	dlog_print(DLOG_INFO, LOG_TAG, "load image: %s.", ad->minutes_path);

	snprintf(ad->seconds_path, PATH_MAX, "%s%s%s", res_dir_path, "images/",
			seconds_name);
	dlog_print(DLOG_INFO, LOG_TAG, "load image: %s.", ad->seconds_path);

	free(res_dir_path);
	load_hand(ad, 0, 0, 0);

	ret = watch_time_get_current_time(&watch_time);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d",
				ret);

	update_watch(ad, watch_time, 0);
	watch_time_delete(watch_time);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

static bool app_create(int width, int height, void *data) {
	appdata_s *ad = data;
	create_base_gui(ad, width, height);
	return true;
}

static void app_control(app_control_h app_control, void *data) {
}

static void app_pause(void *data) {
}

static void app_resume(void *data) {
}

static void app_terminate(void *data) {
}

static void app_time_tick(watch_time_h watch_time, void *data) {
	appdata_s *ad = data;
	update_watch(ad, watch_time, 0);
}

static void app_ambient_tick(watch_time_h watch_time, void *data) {
	appdata_s *ad = data;
	update_watch(ad, watch_time, 1);
}

static void app_ambient_changed(bool ambient_mode, void *data) {
}

static void watch_app_lang_changed(app_event_info_h event_info, void *user_data) {
	char *locale = NULL;
	app_event_get_language(event_info, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void watch_app_region_changed(app_event_info_h event_info,
		void *user_data) {
}

int main(int argc, char *argv[]) {
	appdata_s ad = { 0, };
	int ret = 0;

	watch_app_lifecycle_callback_s event_callback = { 0, };
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;
	event_callback.time_tick = app_time_tick;
	event_callback.ambient_tick = app_ambient_tick;
	event_callback.ambient_changed = app_ambient_changed;

	watch_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
			APP_EVENT_LANGUAGE_CHANGED, watch_app_lang_changed, &ad);
	watch_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
			APP_EVENT_REGION_FORMAT_CHANGED, watch_app_region_changed, &ad);

	ret = watch_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "watch_app_main() is failed. err = %d",
				ret);
	}

	return ret;
}

