#include <dlog.h>
#include <sensor.h>

#include <privacy_privilege_manager.h>
#include "jsc_sw_watch_face.h"

#define PEDOMETER_PERMISSION "http://tizen.org/privilege/healthinfo"
#define PEDOMETER_SENSOR_INTERVAL 10

void init_pedometer();
void get_today_count(void (*update_steps)(int steps));
void destroy_pedometer();
