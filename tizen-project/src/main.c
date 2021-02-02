#include <watch_app.h>
#include <system_settings.h>
#include <device/battery.h>
#include <callback.h>
#include "jsc_sw_watch_face.h"
#include "view.h"

static bool _get_time(watch_time_h watch_time, current_time_t *current_time);

/*
 * @brief The system language changed event callback function
 * @param[in] event_info The system event information
 * @param[in] user_data The user data passed from the add event handler function
 */
void lang_changed(app_event_info_h event_info, void* user_data) {
	char *locale = NULL;

	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE,
			&locale);
	if (locale == NULL)
		return;

	elm_language_set(locale);
	view_set_language(locale);
	free(locale);

	return;
}

/*
 * @brief The region format changed event callback function
 * @param[in] event_info The system event information
 * @param[in] user_data The user data passed from the add event handler function
 */
void region_changed(app_event_info_h event_info, void* user_data) {
	/*
	 * Takes necessary actions when region setting is changed
	 */
}

/*
 * @brief The low battery event callback function
 * @param[in] event_info The system event information
 * @param[in] user_data The user data passed from the add event handler function
 */
void low_battery(app_event_info_h event_info, void* user_data) {
	/*
	 * Takes necessary actions when system is running on low battery
	 */
	watch_app_exit();
}

/*
 * @brief The low memory event callback function
 * @param[in] event_info The system event information
 * @param[in] user_data The user data passed from the add event handler function
 */
void low_memory(app_event_info_h event_info, void* user_data) {
	/*
	 * Takes necessary actions when system is running on low memory
	 */
	watch_app_exit();
}

/*
 * @brief The device orientation changed event callback function
 * @param[in] event_info The system event information
 * @param[in] user_data The user data passed from the add event handler function
 */
void device_orientation(app_event_info_h event_info, void* user_data) {
	/*
	 * Takes necessary actions when device orientation is changed
	 */
}

/*
 * @brief Called when the application starts.
 * @param[in] width The width of the window of idle screen that will show the watch UI
 * @param[in] height The height of the window of idle screen that will show the watch UI
 * @param[in] user_data The user data passed from the callback registration function
 */
static bool app_create(int width, int height, void* user_data) {
	view_create_with_size(width, height);
	lang_changed(NULL, NULL);
	return true;
}

/*
 * @brief: This callback function is called when another application
 * sends the launch request to the application
 */
static void app_control(app_control_h app_control, void *user_data) {
	/*
	 * Handle the launch request.
	 */
}

/*
 * @brief: This callback function is called each time
 * the application is completely obscured by another application
 * and becomes invisible to the user
 */
static void app_pause(void *user_data) {
	/*
	 * Take necessary actions when application becomes invisible.
	 */
}

/*
 * @brief: This callback function is called each time
 * the application becomes visible to the user
 */
static void app_resume(void *user_data) {
	int battery;
	device_battery_get_percent(&battery);
	view_update_battery(battery);
}

/*
 * @brief: This callback function is called once after the main loop of the application exits
 */
static void app_terminate(void *user_data) {
	view_destroy();
}

/*
 * @brief Called at each second. This callback is not called while the app is paused or the device is in ambient mode.
 * @param[in] watch_time The watch time handle. watch_time will not be available after returning this callback. It will be freed by the framework.
 * @param[in] user_data The user data to be passed to the callback functions
 */
void app_time_tick(watch_time_h watch_time, void* user_data) {
	current_time_t current_time = { 0, };

	if (_get_time(watch_time, &current_time))
		view_set_display_time(current_time);
}

/*
 * @brief Called at each minute when the device in the ambient mode.
 * @param[in] watch_time The watch time handle. watch_time will not be available after returning this callback. It will be freed by the framework.
 * @param[in] user_data The user data to be passed to the callback functions
 */
void app_ambient_tick(watch_time_h watch_time, void* user_data) {
	app_time_tick(watch_time, user_data);
}

/*
 * @brief: This function will be called when the ambient mode is changed
 */
void app_ambient_changed(bool ambient_mode, void* user_data) {
	/*
	 * Take necessary actions when application goes to/from ambient state
	 */
	view_toggle_ambient_mode(ambient_mode);
}

void battery_capacity_cb(device_callback_e type, void *value, void *user_data) {
	view_update_battery((int) value);
}

/*
 * @brief: Main function of the application
 */
int main(int argc, char *argv[]) {
	int ret = 0;

	watch_app_lifecycle_callback_s event_callback = { 0, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;
	event_callback.time_tick = app_time_tick;
	event_callback.ambient_tick = app_ambient_tick;
	event_callback.ambient_changed = app_ambient_changed;

	if (device_add_callback(DEVICE_CALLBACK_BATTERY_CAPACITY,
			battery_capacity_cb,
			NULL) < 0) {
		dlog_print(DLOG_ERROR, LOG_TAG,
				"Cannot register battery capacity callback.");
	}

	ret = watch_app_main(argc, argv, &event_callback, NULL);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "watch_app_main() is failed. err = %d",
				ret);

	return ret;
}

/*
 * @brief: Obtains the current time from watch_time_h and converts to the current_time_t.
 * @param[watch_time]: The date and time structure acquired in time_tick callback function.
 * @param[current_time]: The structure of time components extracted from watch_time argument.
 * @return: The function returns 'true' if the time is successfully converted, otherwise 'false' is returned.
 */
static bool _get_time(watch_time_h watch_time, current_time_t *current_time) {
	int ret;

	if (!current_time) {
		dlog_print(DLOG_ERROR, LOG_TAG, "current_time is NULL");
		return false;
	}

	memset(current_time, 0, sizeof(current_time_t));

	if (!watch_time) {
		dlog_print(DLOG_ERROR, LOG_TAG, "watch_time is NULL");
		return false;
	}

	ret = watch_time_get_current_time(&watch_time);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get current time. err = %d",
				ret);
		return false;
	}

	watch_time_get_day(watch_time, &current_time->day);
	watch_time_get_month(watch_time, &current_time->month);
	watch_time_get_hour24(watch_time, &current_time->hour);
	watch_time_get_minute(watch_time, &current_time->minute);
	watch_time_get_second(watch_time, &current_time->second);

	return true;
}
