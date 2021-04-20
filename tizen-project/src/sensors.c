#include <time.h>
#include "jsc_sensors.h"

sensor_recorder_query_h query;

bool query_callback(sensor_type_e type, sensor_recorder_data_h data,
		int remains, sensor_error_e error, void (*update_steps)(int steps)) {
	if (error != SENSOR_ERROR_NONE)
		return false;
	int steps = 0;
	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_STEPS, &steps);
	update_steps(steps);
	return true;
}

void query_sensor(void (*update_steps)(int steps)) {
	/* Query until now */
	sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_END_TIME,
			time(NULL));
	sensor_recorder_read(SENSOR_HUMAN_PEDOMETER, query, query_callback,
			update_steps);
}

void init_query_sensor() {
	sensor_recorder_create_query(&query);
	/* Query from 24 hours ago */
	sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_START_TIME,
			(time_t) (time(NULL) - (24 * 3600)));
	/* Aggregate every 24 hours */
	sensor_recorder_query_set_int(query, SENSOR_RECORDER_QUERY_TIME_INTERVAL,
			24 * 60);
	/* Start the aggregation at 0 AM */
	sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_ANCHOR_TIME,
			(time_t) (0));
}

void create_sensor() {
	bool recorderSupported;
	sensor_recorder_is_supported(SENSOR_HUMAN_PEDOMETER, &recorderSupported);
	if (!recorderSupported) {
		dlog_print(DLOG_WARN, LOG_TAG, "Recorder not supported.");
		return;
	}

	sensor_recorder_option_h option;
	sensor_recorder_create_option(&option);
	int result = sensor_recorder_option_set_int(option,
			SENSOR_RECORDER_OPTION_RETENTION_PERIOD, 24);
	if (result != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Sensor cannot set option.");
		return;
	}
	result = sensor_recorder_start(SENSOR_HUMAN_PEDOMETER, option);
	if (result != SENSOR_ERROR_NONE) {
		dlog_print(DLOG_WARN, LOG_TAG, "Sensor cannot start recorder: %d.",
				result);
		return;
	}
	init_query_sensor();
	dlog_print(DLOG_INFO, LOG_TAG, "Sensor created.");
}

void check_permission_cb(ppm_call_cause_e cause, ppm_request_result_e result,
		const char *privilege, void *user_data) {
	if (result != PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER) {
		dlog_print(DLOG_WARN, LOG_TAG, "Not allowed: %s", privilege);
		return;
	} else {
		create_sensor();
	}
}

void init_pedometer() {
	ppm_check_result_e permission_result;
	int error = ppm_check_permission(PEDOMETER_PERMISSION, &permission_result);
	if (error < 0) {
		dlog_print(DLOG_WARN, LOG_TAG, "Cannot ask for permission: %d", error);
		return;
	}
	if (permission_result == PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK) {
		dlog_print(DLOG_INFO, LOG_TAG, "Permission requested.");
		ppm_request_permission(PEDOMETER_PERMISSION, check_permission_cb,
		NULL);
	} else if (permission_result
			== PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Permission denied.");
	} else if (permission_result
			== PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW) {
		check_permission_cb(PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ANSWER,
				PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER,
				"http://tizen.org/sensor/healthinfo/human_pedometer",
				NULL);
	}
}

void get_today_count(void (*update_steps)(int steps)) {
	query_sensor(update_steps);
}

void destroy_pedometer() {
	sensor_recorder_stop(SENSOR_HUMAN_PEDOMETER);
}
