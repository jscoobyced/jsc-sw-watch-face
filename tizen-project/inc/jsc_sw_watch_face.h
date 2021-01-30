#ifndef __jsc_sw_watch_face_H__
#define __jsc_sw_watch_face_H__

#include <app.h>
#include <dlog.h>

struct _current_time {
	int hour;
	int minute;
	int second;
};

typedef struct _current_time current_time_t;

#include <watch_app.h>
#include <watch_app_efl.h>
#include <Elementary.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "jsc-sw-watch-face"

#if !defined(PACKAGE)
#define PACKAGE "io.narok.jsc_sw_watch_face"
#endif

#endif /* __jsc_sw_watch_face_H__ */
