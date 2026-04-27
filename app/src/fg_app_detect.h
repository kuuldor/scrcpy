#ifndef SC_FG_APP_DETECT_H
#define SC_FG_APP_DETECT_H

#include "common.h"

#include <stdbool.h>

#include "util/thread.h"

struct sc_fg_app_detect;

typedef bool
(*sc_fg_app_detect_query_package_fn)(struct sc_fg_app_detect *detect,
                                     void *userdata,
                                     char **package_name);

typedef void
(*sc_fg_app_detect_on_changed_fn)(struct sc_fg_app_detect *detect,
                                  const char *package_name,
                                  void *userdata);

struct sc_fg_app_detect {
    char *device_serial;
    char *current_package;

    sc_tick poll_interval;
    sc_mutex mutex;
    sc_cond event_cond;
    sc_thread thread;
    bool stop_requested;
    bool thread_started;

    sc_fg_app_detect_query_package_fn query_package;
    void *query_package_userdata;
    sc_fg_app_detect_on_changed_fn on_changed;
    void *on_changed_userdata;
};

bool
sc_fg_app_detect_init(struct sc_fg_app_detect *detect,
                      const char *device_serial);

void
sc_fg_app_detect_destroy(struct sc_fg_app_detect *detect);

bool
sc_fg_app_detect_set_device_serial(struct sc_fg_app_detect *detect,
                                   const char *serial);

bool
sc_fg_app_detect_set_current_package(struct sc_fg_app_detect *detect,
                                     const char *package_name);

void
sc_fg_app_detect_set_query_package(struct sc_fg_app_detect *detect,
                                   sc_fg_app_detect_query_package_fn fn,
                                   void *userdata);

void
sc_fg_app_detect_set_on_changed(struct sc_fg_app_detect *detect,
                                sc_fg_app_detect_on_changed_fn fn,
                                void *userdata);

bool
sc_fg_app_detect_start(struct sc_fg_app_detect *detect);

void
sc_fg_app_detect_stop(struct sc_fg_app_detect *detect);

#ifdef SC_TEST
bool
sc_fg_app_detect_parse_package_from_dump(const char *dump,
                                         char **package_name);
#endif

#endif
