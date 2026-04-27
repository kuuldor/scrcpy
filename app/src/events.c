#include "events.h"

#include <assert.h>

#include "util/log.h"
#include "util/thread.h"

bool
sc_push_event_impl(uint32_t type, const char *name) {
    SDL_Event event;
    event.type = type;
    int ret = SDL_PushEvent(&event);
    // ret < 0: error (queue full)
    // ret == 0: event was filtered
    // ret == 1: success
    if (ret != 1) {
        LOGE("Could not post %s event: %s", name, SDL_GetError());
        return false;
    }

    return true;
}

bool
sc_post_to_main_thread(sc_runnable_fn run, void *userdata) {
    SDL_Event event = {
        .user = {
            .type = SC_EVENT_RUN_ON_MAIN_THREAD,
            .data1 = run,
            .data2 = userdata,
        },
    };
    int ret = SDL_PushEvent(&event);
    // ret < 0: error (queue full)
    // ret == 0: event was filtered
    // ret == 1: success
    if (ret != 1) {
        if (ret == 0) {
            // if ret == 0, this is expected on exit, log in debug mode
            LOGD("Could not post runnable to main thread (filtered)");
        } else {
            assert(ret < 0);
            LOGW("Could not post runnable to main thread: %s", SDL_GetError());
        }
        return false;
    }

    return true;
}

struct sc_fg_app_changed_event *
sc_fg_app_changed_event_new(const char *package_name) {
    struct sc_fg_app_changed_event *event = SDL_malloc(sizeof(*event));
    if (!event) {
        LOG_OOM();
        return NULL;
    }

    event->package_name = package_name ? SDL_strdup(package_name) : NULL;
    if (package_name && !event->package_name) {
        LOG_OOM();
        SDL_free(event);
        return NULL;
    }

    return event;
}

void
sc_fg_app_changed_event_destroy(
    struct sc_fg_app_changed_event *event) {
    if (!event) {
        return;
    }

    SDL_free(event->package_name);
    SDL_free(event);
}

bool
sc_post_fg_app_changed(const char *package_name) {
    struct sc_fg_app_changed_event *payload =
        sc_fg_app_changed_event_new(package_name);
    if (!payload) {
        return false;
    }

    SDL_Event event = {
        .user = {
            .type = SC_EVENT_FG_APP_CHANGED,
            // Ownership is transferred to the SDL main thread handler.
            .data1 = payload,
        },
    };
    int ret = SDL_PushEvent(&event);
    if (ret != 1) {
        if (ret == 0) {
            LOGD("Could not post foreground app changed event (filtered)");
        } else {
            assert(ret < 0);
            LOGW("Could not post foreground app changed event: %s",
                 SDL_GetError());
        }
        sc_fg_app_changed_event_destroy(payload);
        return false;
    }

    return true;
}

void
sc_cleanup_event(SDL_Event *event) {
    switch (event->type) {
        case SC_EVENT_FG_APP_CHANGED:
            sc_fg_app_changed_event_destroy(event->user.data1);
            event->user.data1 = NULL;
            break;
        default:
            break;
    }
}

static int SDLCALL
task_event_filter(void *userdata, SDL_Event *event) {
    (void) userdata;

    if (event->type == SC_EVENT_RUN_ON_MAIN_THREAD) {
        // Reject this event type from now on
        return 0;
    }

    return 1;
}

void
sc_reject_new_runnables(void) {
    assert(sc_thread_get_id() == SC_MAIN_THREAD_ID);

    SDL_SetEventFilter(task_event_filter, NULL);
}
