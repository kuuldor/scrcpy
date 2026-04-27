#ifndef SC_EVENTS_H
#define SC_EVENTS_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL_events.h>

enum {
    SC_EVENT_NEW_FRAME = SDL_USEREVENT,
    SC_EVENT_RUN_ON_MAIN_THREAD,
    SC_EVENT_DEVICE_DISCONNECTED,
    SC_EVENT_SERVER_CONNECTION_FAILED,
    SC_EVENT_SERVER_CONNECTED,
    SC_EVENT_USB_DEVICE_DISCONNECTED,
    SC_EVENT_DEMUXER_ERROR,
    SC_EVENT_RECORDER_ERROR,
    SC_EVENT_SCREEN_INIT_SIZE,
    SC_EVENT_SCREEN_REFRESH,
    SC_EVENT_TIME_LIMIT_REACHED,
    SC_EVENT_CONTROLLER_ERROR,
    SC_EVENT_AOA_OPEN_ERROR,
    SC_EVENT_FILE_DIALOG,
    SC_EVENT_TOUCHMAP_SAVE,
    SC_EVENT_FG_APP_CHANGED,
};

bool
sc_push_event_impl(uint32_t type, const char *name);

#define sc_push_event(TYPE) sc_push_event_impl(TYPE, # TYPE)

typedef void (*sc_runnable_fn)(void *userdata);

struct sc_fg_app_changed_event {
    char *package_name;
};

bool
sc_post_to_main_thread(sc_runnable_fn run, void *userdata);

struct sc_fg_app_changed_event *
sc_fg_app_changed_event_new(const char *package_name);

void
sc_fg_app_changed_event_destroy(
    struct sc_fg_app_changed_event *event);

bool
sc_post_fg_app_changed(const char *package_name);

void
sc_cleanup_event(SDL_Event *event);

void
sc_reject_new_runnables(void);

#endif
