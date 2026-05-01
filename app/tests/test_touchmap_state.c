#include "common.h"

#include <assert.h>

#include "events.h"
#include "third_party/tfd/tinyfiledialogs.h"
#include "touchmap/touchmap_state.h"

bool
sc_push_event_impl(uint32_t type, const char *name) {
    (void) type;
    (void) name;
    return true;
}

int
tinyfd_messageBox(char const *title, char const *message,
                  char const *dialog_type, char const *icon_type,
                  int default_button) {
    (void) title;
    (void) message;
    (void) dialog_type;
    (void) icon_type;
    return default_button;
}

char *
tinyfd_saveFileDialog(char const *title, char const *default_path_and_or_file,
                      int num_filter_patterns,
                      char const *const *filter_patterns,
                      char const *single_filter_description) {
    (void) title;
    (void) default_path_and_or_file;
    (void) num_filter_patterns;
    (void) filter_patterns;
    (void) single_filter_description;
    return NULL;
}

char *
tinyfd_openFileDialog(char const *title, char const *default_path_and_or_file,
                      int num_filter_patterns,
                      char const *const *filter_patterns,
                      char const *single_filter_description,
                      int allow_multiple_selects) {
    (void) title;
    (void) default_path_and_or_file;
    (void) num_filter_patterns;
    (void) filter_patterns;
    (void) single_filter_description;
    (void) allow_multiple_selects;
    return NULL;
}

static void
test_touchmap_state_fg_change_load(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(
            &state, "com.example.game", "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_LOAD);
    assert(decision.package_name == (const char *) "com.example.game");
    assert(decision.touchmap_file == (const char *) "/maps/game.json");
}

static void
test_touchmap_state_fg_change_unload_on_no_match(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .map = (struct sc_gptm_gamepad_touchmap *) 1,
        .file = "/maps/old.json",
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(
            &state, "com.example.game", NULL);
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_UNLOAD);
    assert(decision.package_name == (const char *) "com.example.game");
    assert(!decision.touchmap_file);
}

static void
test_touchmap_state_fg_change_defer_when_dirty(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .dirty = true,
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(
            &state, "com.example.game", "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_DEFER);
}

static void
test_touchmap_state_fg_change_defer_when_editing(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .edit_mode = true,
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(
            &state, "com.example.game", "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_DEFER);
}

static void
test_touchmap_state_fg_change_ignored_for_manual_override(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .manual_override = true,
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(
            &state, "com.example.game", "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_NONE);
}

static void
test_touchmap_state_fg_change_keep_current(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .map = (struct sc_gptm_gamepad_touchmap *) 1,
        .file = "/maps/game.json",
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(
            &state, "com.example.game", "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_KEEP);
}

static void
test_touchmap_state_apply_deferred_load(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .deferred_package = "com.example.game",
        .deferred_file = "/maps/game.json",
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_apply_deferred(&state, "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_LOAD);
    assert(decision.package_name == (const char *) "com.example.game");
    assert(decision.touchmap_file == (const char *) "/maps/game.json");
}

static void
test_touchmap_state_apply_deferred_drop_when_target_changes(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .deferred_package = "com.example.game",
        .deferred_file = "/maps/game.json",
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_apply_deferred(&state, "/maps/other.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_DROP_DEFERRED);
}

static void
test_touchmap_state_apply_deferred_unload_on_no_match(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .map = (struct sc_gptm_gamepad_touchmap *) 1,
        .file = "/maps/current.json",
        .deferred_package = "com.example.game",
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_apply_deferred(&state, NULL);
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_UNLOAD);
}

static void
test_touchmap_state_apply_deferred_keep_current(void) {
    struct sc_touchmap_state state = {
        .auto_enabled = true,
        .map = (struct sc_gptm_gamepad_touchmap *) 1,
        .file = "/maps/game.json",
        .deferred_package = "com.example.game",
        .deferred_file = "/maps/game.json",
    };

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_apply_deferred(&state, "/maps/game.json");
    assert(decision.action == SC_TOUCHMAP_SWITCH_ACTION_KEEP);
}

int
main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test_touchmap_state_fg_change_load();
    test_touchmap_state_fg_change_unload_on_no_match();
    test_touchmap_state_fg_change_defer_when_dirty();
    test_touchmap_state_fg_change_defer_when_editing();
    test_touchmap_state_fg_change_ignored_for_manual_override();
    test_touchmap_state_fg_change_keep_current();
    test_touchmap_state_apply_deferred_load();
    test_touchmap_state_apply_deferred_drop_when_target_changes();
    test_touchmap_state_apply_deferred_unload_on_no_match();
    test_touchmap_state_apply_deferred_keep_current();
    return 0;
}
