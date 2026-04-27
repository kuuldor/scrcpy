#include "fg_app_detect.h"

#include <SDL2/SDL.h>
#include <string.h>

#ifndef SC_TEST
#include "adb/adb.h"
#include "util/process_intr.h"
#endif
#include "events.h"
#include "util/log.h"

#define SC_FG_APP_DETECT_POLL_INTERVAL SC_TICK_FROM_MS(1000)
#define SC_FG_APP_DETECT_QUERY_BUFSIZE 131072

static bool
sc_fg_app_detect_replace_string(char **dst, const char *src) {
    char *copy = src ? SDL_strdup(src) : NULL;
    if (src && !copy) {
        LOG_OOM();
        return false;
    }

    SDL_free(*dst);
    *dst = copy;
    return true;
}

#ifdef SC_TEST
bool
#else
static bool
#endif
sc_fg_app_detect_parse_package_from_dump(const char *dump,
                                         char **package_name) {
    static const char *markers[] = {
        "topResumedActivity=",
        "mResumedActivity:",
        "ResumedActivity:",
    };

    for (size_t i = 0; i < SDL_arraysize(markers); ++i) {
        const char *pos = SDL_strstr(dump, markers[i]);
        if (!pos) {
            continue;
        }

        const char *search = pos + SDL_strlen(markers[i]);
        const char *slash = SDL_strchr(search, '/');
        if (!slash || slash == search) {
            continue;
        }

        const char *pkg = slash;
        while (pkg > search && pkg[-1] != ' ' && pkg[-1] != '\t'
                && pkg[-1] != '{') {
            --pkg;
        }

        if (pkg == slash) {
            continue;
        }

        size_t len = (size_t) (slash - pkg);
        char *copy = SDL_malloc(len + 1);
        if (!copy) {
            LOG_OOM();
            return false;
        }

        memcpy(copy, pkg, len);
        copy[len] = '\0';
        *package_name = copy;
        return true;
    }

    *package_name = NULL;
    return true;
}

#ifndef SC_TEST
static bool
sc_fg_app_detect_query_package_default(struct sc_fg_app_detect *detect,
                                       void *userdata,
                                       char **package_name) {
    (void) userdata;

    if (!detect->device_serial || !*detect->device_serial) {
        return false;
    }

    const char *const argv[] = {
        sc_adb_get_executable(),
        "-s", detect->device_serial,
        "shell", "dumpsys", "activity", "activities",
        NULL,
    };

    sc_pipe pout;
    sc_pid pid;
    enum sc_process_result result =
        sc_process_execute_p(argv, &pid, SC_PROCESS_NO_STDERR, NULL, &pout,
                             NULL);
    if (result != SC_PROCESS_SUCCESS) {
        LOGD("Could not execute foreground package query");
        return false;
    }

    char *buf = SDL_malloc(SC_FG_APP_DETECT_QUERY_BUFSIZE);
    if (!buf) {
        LOG_OOM();
        sc_pipe_close(pout);
        sc_process_terminate(pid);
        sc_process_wait(pid, true);
        return false;
    }

    ssize_t r = sc_pipe_read_all(pout, buf,
                                 SC_FG_APP_DETECT_QUERY_BUFSIZE - 1);
    sc_pipe_close(pout);
    sc_exit_code exit_code = sc_process_wait(pid, true);
    if (r < 0 || exit_code != 0) {
        SDL_free(buf);
        return false;
    }

    buf[r] = '\0';
    bool ok = sc_fg_app_detect_parse_package_from_dump(buf, package_name);
    SDL_free(buf);
    return ok;
}
#endif

bool
sc_fg_app_detect_init(struct sc_fg_app_detect *detect,
                      const char *device_serial) {
    detect->device_serial = NULL;
    detect->current_package = NULL;
    detect->poll_interval = SC_FG_APP_DETECT_POLL_INTERVAL;
    detect->stop_requested = false;
    detect->thread_started = false;
    detect->query_package = NULL;
    detect->query_package_userdata = NULL;
    detect->on_changed = NULL;
    detect->on_changed_userdata = NULL;

    if (!sc_mutex_init(&detect->mutex)) {
        return false;
    }
    if (!sc_cond_init(&detect->event_cond)) {
        sc_mutex_destroy(&detect->mutex);
        return false;
    }

    if (!device_serial) {
        return true;
    }

    if (!sc_fg_app_detect_set_device_serial(detect, device_serial)) {
        sc_cond_destroy(&detect->event_cond);
        sc_mutex_destroy(&detect->mutex);
        return false;
    }

    return true;
}

void
sc_fg_app_detect_destroy(struct sc_fg_app_detect *detect) {
    if (!detect) {
        return;
    }

    sc_fg_app_detect_stop(detect);

    SDL_free(detect->device_serial);
    SDL_free(detect->current_package);
    sc_cond_destroy(&detect->event_cond);
    sc_mutex_destroy(&detect->mutex);

    detect->device_serial = NULL;
    detect->current_package = NULL;
    detect->stop_requested = false;
    detect->thread_started = false;
}

bool
sc_fg_app_detect_set_device_serial(struct sc_fg_app_detect *detect,
                                   const char *serial) {
    return sc_fg_app_detect_replace_string(&detect->device_serial, serial);
}

bool
sc_fg_app_detect_set_current_package(struct sc_fg_app_detect *detect,
                                     const char *package_name) {
    return sc_fg_app_detect_replace_string(&detect->current_package,
                                           package_name);
}

void
sc_fg_app_detect_set_query_package(struct sc_fg_app_detect *detect,
                                   sc_fg_app_detect_query_package_fn fn,
                                   void *userdata) {
    detect->query_package = fn;
    detect->query_package_userdata = userdata;
}

void
sc_fg_app_detect_set_on_changed(struct sc_fg_app_detect *detect,
                                sc_fg_app_detect_on_changed_fn fn,
                                void *userdata) {
    detect->on_changed = fn;
    detect->on_changed_userdata = userdata;
}

static int
sc_fg_app_detect_run(void *data) {
    struct sc_fg_app_detect *detect = data;
    sc_fg_app_detect_query_package_fn query = detect->query_package;
#ifndef SC_TEST
    if (!query) {
        query = sc_fg_app_detect_query_package_default;
    }
#endif

    while (true) {
        sc_mutex_lock(&detect->mutex);
        if (detect->stop_requested) {
            sc_mutex_unlock(&detect->mutex);
            break;
        }
        sc_tick deadline = sc_tick_now() + detect->poll_interval;
        sc_mutex_unlock(&detect->mutex);

        if (query) {
            char *package_name = NULL;
            bool ok = query(detect, detect->query_package_userdata,
                            &package_name);
            if (!ok) {
                LOGD("Foreground package query failed");
            } else {
                bool changed = false;
                sc_mutex_lock(&detect->mutex);
                const char *current =
                    detect->current_package ? detect->current_package : "";
                const char *next = package_name ? package_name : "";
                if (SDL_strcmp(current, next)) {
                    changed = sc_fg_app_detect_set_current_package(detect,
                                                                   package_name);
                }
                sc_mutex_unlock(&detect->mutex);

                if (changed) {
                    LOGI("Foreground package changed: %s",
                         package_name ? package_name : "(none)");
#ifndef SC_TEST
                    sc_post_fg_app_changed(package_name);
#endif
                    if (detect->on_changed) {
                        detect->on_changed(detect, package_name,
                                           detect->on_changed_userdata);
                    }
                }
            }
            SDL_free(package_name);
        }

        sc_mutex_lock(&detect->mutex);
        if (!detect->stop_requested) {
            sc_cond_timedwait(&detect->event_cond, &detect->mutex, deadline);
        }
        sc_mutex_unlock(&detect->mutex);
    }

    return 0;
}

bool
sc_fg_app_detect_start(struct sc_fg_app_detect *detect) {
    if (detect->thread_started) {
        return true;
    }

    sc_fg_app_detect_query_package_fn query = detect->query_package;
#ifndef SC_TEST
    if (!query && detect->device_serial) {
        query = sc_fg_app_detect_query_package_default;
    }
#endif
    if (!query) {
        LOGW("Foreground app polling requires a package query function");
        return false;
    }

    detect->stop_requested = false;
    if (!sc_thread_create(&detect->thread, sc_fg_app_detect_run,
                          "fg-app", detect)) {
        return false;
    }

    detect->thread_started = true;
    return true;
}

void
sc_fg_app_detect_stop(struct sc_fg_app_detect *detect) {
    if (!detect->thread_started) {
        return;
    }

    sc_mutex_lock(&detect->mutex);
    detect->stop_requested = true;
    sc_cond_signal(&detect->event_cond);
    sc_mutex_unlock(&detect->mutex);

    sc_thread_join(&detect->thread, NULL);
    detect->thread_started = false;
}
