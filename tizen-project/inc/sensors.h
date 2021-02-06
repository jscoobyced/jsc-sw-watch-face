#include <dlog.h>
#include <privacy_privilege_manager.h>
#include <sensor.h>
#include "jsc_sw_watch_face.h"

#define PEDOMETER_PERMISSION "http://tizen.org/privilege/healthinfo"
#define PEDOMETER_SENSOR_INTERVAL 1000

void init_pedometer(void *user_data);
int get_today_count();
void destroy_pedometer();
