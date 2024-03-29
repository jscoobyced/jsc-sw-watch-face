#if !defined(_VIEW_H)
#define _VIEW_H

#include <Elementary.h>
#include <efl_extension.h>
#include "jsc_sw_watch_face.h"

void view_create_with_size(int width, int height);
void view_create(void);
Evas_Object *view_create_win(const char *pkg_name);
Evas_Object *view_create_layout_for_part(Evas_Object *parent, char *file_path, char *group_name, char *part_name);
static char *_create_resource_path(const char *file_name);
static Evas_Object *_create_layout(void);
void view_set_display_time(current_time_t current_time);
void view_set_language(char *locale);
void view_toggle_ambient_mode(bool ambient_mode);
void view_destroy(void);
void view_update_battery(int battery_percent);
#endif
