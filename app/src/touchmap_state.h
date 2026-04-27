#ifndef SC_TOUCHMAP_STATE_H
#define SC_TOUCHMAP_STATE_H

#include "common.h"

#include <stdbool.h>

#include "touchmap.h"
#include "touchmap_editor.h"
#include "touchmap_loader.h"

struct sc_touchmap_state {
    struct sc_gptm_gamepad_touchmap *map;
    char *file;
    const char *dir;
    struct sc_touchmap_loader loader;
    struct sc_touchmap_editor editor;

    bool overlay_enabled;
    bool edit_mode;
    bool add_menu_open;

    bool dirty;
    bool exit_after_save;
    bool consume_left_button_up;

    bool auto_enabled;
    bool manual_override;
    char *current_package;
    char *deferred_package;
    char *deferred_file;
};

enum sc_touchmap_switch_action {
    SC_TOUCHMAP_SWITCH_ACTION_NONE,
    SC_TOUCHMAP_SWITCH_ACTION_KEEP,
    SC_TOUCHMAP_SWITCH_ACTION_DEFER,
    SC_TOUCHMAP_SWITCH_ACTION_DROP_DEFERRED,
    SC_TOUCHMAP_SWITCH_ACTION_LOAD,
    SC_TOUCHMAP_SWITCH_ACTION_UNLOAD,
};

struct sc_touchmap_switch_decision {
    enum sc_touchmap_switch_action action;
    const char *package_name;
    const char *touchmap_file;
};

bool
sc_touchmap_state_init(struct sc_touchmap_state *touchmap,
                       const char *dir, bool auto_enabled);

void
sc_touchmap_state_destroy(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_reset_runtime(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_clear_deferred(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_set_overlay_enabled(struct sc_touchmap_state *touchmap,
                                      bool enabled);

void
sc_touchmap_state_toggle_overlay(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_set_edit_mode(struct sc_touchmap_state *touchmap,
                                bool edit_mode);

struct sc_touchmap_switch_decision
sc_touchmap_switch_decide_foreground_change(
        const struct sc_touchmap_state *state,
        const char *package_name, const char *resolved_file);

struct sc_touchmap_switch_decision
sc_touchmap_switch_decide_apply_deferred(
        const struct sc_touchmap_state *state,
        const char *resolved_file);

#endif
