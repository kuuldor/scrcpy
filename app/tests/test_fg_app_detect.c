#include "common.h"

#include <assert.h>

#include <SDL2/SDL.h>

#include "fg_app_detect.h"
#include "util/thread.h"

struct query_sequence {
    const char *values[8];
    bool results[8];
    size_t count;
    size_t index;
};

struct package_events {
    sc_mutex mutex;
    sc_cond cond;
    unsigned count;
    char *last_package;
};

static bool
query_sequence_next(struct sc_fg_app_detect *detect, void *userdata,
                    char **package_name) {
    (void) detect;
    struct query_sequence *seq = userdata;
    if (seq->index >= seq->count) {
        *package_name = NULL;
        return true;
    }

    size_t i = seq->index++;
    if (!seq->results[i]) {
        *package_name = NULL;
        return false;
    }

    *package_name = seq->values[i] ? SDL_strdup(seq->values[i]) : NULL;
    return true;
}

static void
record_package_event(struct sc_fg_app_detect *detect, const char *package_name,
                     void *userdata) {
    (void) detect;
    struct package_events *events = userdata;
    sc_mutex_lock(&events->mutex);
    ++events->count;
    SDL_free(events->last_package);
    events->last_package = package_name ? SDL_strdup(package_name) : NULL;
    sc_cond_signal(&events->cond);
    sc_mutex_unlock(&events->mutex);
}

static void
test_fg_app_detect_init(void) {
    struct sc_fg_app_detect detect;
    bool ok = sc_fg_app_detect_init(&detect, NULL);
    assert(ok);
    assert(!detect.device_serial);
    assert(!detect.current_package);
    assert(!detect.thread_started);
    sc_fg_app_detect_destroy(&detect);
}

static void
test_fg_app_detect_parse_dump(void) {
    char *package_name = NULL;
    bool ok = sc_fg_app_detect_parse_package_from_dump(
        "topResumedActivity=ActivityRecord{123456 u0 "
        "com.levelinfinite.sgameGlobal/com.tencent.SGameActivity t12}",
        &package_name);
    assert(ok);
    assert(package_name);
    assert(!SDL_strcmp(package_name, "com.levelinfinite.sgameGlobal"));
    SDL_free(package_name);
}

static void
test_fg_app_detect_polling(void) {
    struct sc_fg_app_detect detect;
    bool ok = sc_fg_app_detect_init(&detect, NULL);
    assert(ok);
    detect.poll_interval = SC_TICK_FROM_MS(10);

    struct query_sequence seq = {
        .values = {
            "com.example.one",
            "com.example.one",
            NULL,
            "com.example.two",
        },
        .results = {true, true, false, true},
        .count = 4,
        .index = 0,
    };
    struct package_events events;
    assert(sc_mutex_init(&events.mutex));
    assert(sc_cond_init(&events.cond));
    events.count = 0;
    events.last_package = NULL;

    sc_fg_app_detect_set_query_package(&detect, query_sequence_next, &seq);
    sc_fg_app_detect_set_on_changed(&detect, record_package_event, &events);
    ok = sc_fg_app_detect_start(&detect);
    assert(ok);

    sc_mutex_lock(&events.mutex);
    sc_tick deadline = sc_tick_now() + SC_TICK_FROM_MS(500);
    while (events.count < 2) {
        bool signaled = sc_cond_timedwait(&events.cond, &events.mutex,
                                          deadline);
        assert(signaled);
    }
    sc_mutex_unlock(&events.mutex);

    sc_fg_app_detect_stop(&detect);
    assert(!SDL_strcmp(detect.current_package, "com.example.two"));
    assert(events.count == 2);
    assert(events.last_package);
    assert(!SDL_strcmp(events.last_package, "com.example.two"));

    SDL_free(events.last_package);
    sc_cond_destroy(&events.cond);
    sc_mutex_destroy(&events.mutex);
    sc_fg_app_detect_destroy(&detect);
}

int
main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test_fg_app_detect_init();
    test_fg_app_detect_parse_dump();
    test_fg_app_detect_polling();
    return 0;
}
