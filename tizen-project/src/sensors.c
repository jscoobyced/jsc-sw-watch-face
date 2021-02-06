#include <time.h>
#include "sensors.h"

sensor_listener_h listener;
int total_steps = -1;
int step_count = 0;
sensor_recorder_option_h option;

void finalize_listener(void *user_data) {
	sensor_events_cb app_sensor_cb = (sensor_events_cb) user_data;
	int error = sensor_listener_set_events_cb(listener, app_sensor_cb, NULL);
	if (error != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG,
				"Cannot register event to listener. Error: %d", error);
		destroy_pedometer();
		return;
	}
	error = sensor_listener_set_interval(listener, PEDOMETER_SENSOR_INTERVAL);
	if (error != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG,
				"Cannot set listener interval, listener will not be enabled to preserve battery. Error: %d",
				error);
		destroy_pedometer();
		return;
	}
}

bool sensor_pedometer_data_cb(sensor_type_e type, sensor_recorder_data_h data,
		int remains, sensor_error_e error, void *user_data) {
	if (type != SENSOR_HUMAN_PEDOMETER) {
		dlog_print(DLOG_WARN, LOG_TAG,
				"Sensor record callback not for pedometer.");
		return true;
	}
	int step;
	time_t start;
	time_t end;

	if (error != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Sensor record callback error: %d",
				error);
		return true;
	}

	sensor_recorder_data_get_time(data, &start, &end);
	struct tm *tmStart = localtime(&start);
	dlog_print(DLOG_INFO, LOG_TAG, "Start time: %d %d:%d:%d", tmStart->tm_mday,
			tmStart->tm_hour, tmStart->tm_min, tmStart->tm_sec);
	struct tm *tmEnd = localtime(&end);
	dlog_print(DLOG_INFO, LOG_TAG, "End time: %d %d:%d:%d", tmEnd->tm_mday,
			tmEnd->tm_hour, tmEnd->tm_min, tmEnd->tm_sec);

	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_STEPS, &step);
	step_count += step;
	if (remains == 0) {
		total_steps = step_count;
		step_count = 0;
		int error = sensor_listener_start(listener);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_WARN, LOG_TAG, "Cannot start listener. Error: %d",
					error);
		}
	}
	dlog_print(DLOG_INFO, LOG_TAG, "Step count: %d", total_steps);
	finalize_listener(user_data);
	return true;
}

void get_initial_pedometer_data(void *user_data) {
	bool recorderSupported;
	sensor_recorder_is_supported(SENSOR_HUMAN_PEDOMETER, &recorderSupported);
	if (!recorderSupported) {
		dlog_print(DLOG_WARN, LOG_TAG, "Recorder not supported.");
		return;
	}

	sensor_recorder_create_option(&option);
	sensor_recorder_option_set_int(option,
			SENSOR_RECORDER_OPTION_RETENTION_PERIOD, 24);
	sensor_recorder_start(SENSOR_HUMAN_PEDOMETER, option);
	sensor_recorder_query_h query;
	if (sensor_recorder_create_query(&query) != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot create query");
		return;
	}

	time_t nowTime, startTime;
	nowTime = time(NULL);
	struct tm *tmNow = localtime(&nowTime);
	startTime = nowTime - (tmNow->tm_hour * 3600) - (tmNow->tm_min * 60)
			- (tmNow->tm_sec);

	if (sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_START_TIME,
			startTime) != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot set query START option: %ld.",
				startTime);
		return;
	}
	if (sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_END_TIME,
			nowTime) != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot set query END option: %ld.",
				nowTime);
		return;
	}
	if (sensor_recorder_query_set_int(query,
			SENSOR_RECORDER_QUERY_TIME_INTERVAL, 24 * 60)
			!= SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot set query INTERVAL option: %d.",
				24 * 60);
		return;
	}

	if (sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_ANCHOR_TIME,
			startTime) != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot set query ANCHOR option: %ld.",
				startTime);
		return;
	}

	int error = sensor_recorder_read_sync(SENSOR_HUMAN_PEDOMETER, query,
			sensor_pedometer_data_cb, user_data);
	if (error != SENSOR_ERROR_NONE) {
		char str_error[32];
		if (error == SENSOR_ERROR_INVALID_PARAMETER) {
			snprintf(str_error, 32, "%s", "Invalid parameter");
		} else if (error == SENSOR_ERROR_NOT_SUPPORTED) {
			snprintf(str_error, 32, "%s", "Not supported");
		} else if (error == SENSOR_ERROR_PERMISSION_DENIED) {
			snprintf(str_error, 32, "%s", "Permission denied");
		} else if (error == SENSOR_ERROR_OPERATION_FAILED) {
			snprintf(str_error, 32, "%s", "Operation failed");
		} else if (error == SENSOR_ERROR_NO_DATA) {
			snprintf(str_error, 32, "%s", "No data");
		}
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot read query: %s.", str_error);
	}
}

void create_sensor(void *user_data) {
	sensor_h sensor;
	int error = sensor_get_default_sensor(SENSOR_HUMAN_PEDOMETER, &sensor);
	if (error != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "No default sensor found error: %d",
				error);
		return;
	}
	error = sensor_create_listener(sensor, &listener);
	if (error != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot create listener. Error: %d",
				error);
		return;
	}
	get_initial_pedometer_data(user_data);
	dlog_print(DLOG_INFO, LOG_TAG, "Sensor created.");
}

void check_permission_cb(ppm_call_cause_e cause, ppm_request_result_e result,
		const char *privilege, void *user_data) {
	if (result != PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER) {
		dlog_print(DLOG_WARN, LOG_TAG, "Not allowed: %s", privilege);
		return;
	} else {
		create_sensor(user_data);
	}
}

void init_pedometer(void *user_data) {
	ppm_check_result_e permission_result;
	int error = ppm_check_permission("http://tizen.org/privilege/healthinfo",
			&permission_result);
	if (error < 0) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot ask for permission: %d", error);
		return;
	}
	if (permission_result == PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK) {
		dlog_print(DLOG_INFO, LOG_TAG, "Permission requested.");
		ppm_request_permission("http://tizen.org/privilege/healthinfo",
				check_permission_cb, user_data);
	} else if (permission_result
			== PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Permission denied.");
	} else if (permission_result
			== PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW) {
		check_permission_cb(PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ANSWER,
				PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER,
				"http://tizen.org/sensor/healthinfo/human_pedometer",
				user_data);
	}
}

int get_today_count() {
	return total_steps;
}

void destroy_pedometer() {
	sensor_listener_stop(listener);
	sensor_destroy_listener(listener);
}
