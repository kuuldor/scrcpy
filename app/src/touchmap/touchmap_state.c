#include "touchmap/touchmap_state.h"

#include <SDL2/SDL.h>

#include "touchmap/touchmap_overlay.h"
#include "util/log.h"

static struct sc_touchmap_switch_decision
sc_touchmap_switch_make_decision(enum sc_touchmap_switch_action action,
                                 const char *package_name,
                                 const char *touchmap_file) {
    struct sc_touchmap_switch_decision decision = {
        .action = action,
        .package_name = package_name,
        .touchmap_file = touchmap_file,
    };
    return decision;
}

bool
sc_touchmap_state_init(struct sc_touchmap_state *touchmap,
                       const char *dir, bool auto_enabled) {
    touchmap->map = NULL;
    touchmap->file = NULL;
    if (!sc_touchmap_loader_init(&touchmap->loader, dir)) {
        return false;
    }
    touchmap->dir = touchmap->loader.touchmap_dir;
    sc_touchmap_editor_init(&touchmap->editor);

    touchmap->overlay_enabled = false;
    touchmap->edit_mode = false;
    touchmap->add_menu_open = false;
    touchmap->pending_control = SC_TOUCHMAP_OVERLAY_CONTROL_NONE;

    touchmap->dirty = false;
    touchmap->exit_after_save = false;
    touchmap->consume_left_button_up = false;

    touchmap->auto_enabled = auto_enabled && touchmap->loader.enabled;
    touchmap->manual_override = false;
    touchmap->current_package = NULL;
    touchmap->deferred_package = NULL;
    touchmap->deferred_file = NULL;
    return true;
}

void
sc_touchmap_state_destroy(struct sc_touchmap_state *touchmap) {
    SDL_free(touchmap->current_package);
    SDL_free(touchmap->deferred_package);
    SDL_free(touchmap->deferred_file);
    touchmap->current_package = NULL;
    touchmap->deferred_package = NULL;
    touchmap->deferred_file = NULL;
    sc_touchmap_loader_destroy(&touchmap->loader);
    touchmap->dir = NULL;
}

void
sc_touchmap_state_reset_runtime(struct sc_touchmap_state *touchmap) {
    sc_touchmap_editor_reset(&touchmap->editor);
    touchmap->dirty = false;
    touchmap->exit_after_save = false;
}

void
sc_touchmap_state_clear_deferred(struct sc_touchmap_state *touchmap) {
    SDL_free(touchmap->deferred_package);
    SDL_free(touchmap->deferred_file);
    touchmap->deferred_package = NULL;
    touchmap->deferred_file = NULL;
}

void
sc_touchmap_state_set_overlay_enabled(struct sc_touchmap_state *touchmap,
                                      bool enabled) {
    touchmap->overlay_enabled = enabled;
    if (!enabled) {
        touchmap->edit_mode = false;
        touchmap->add_menu_open = false;
    }
}

void
sc_touchmap_state_toggle_overlay(struct sc_touchmap_state *touchmap) {
    sc_touchmap_state_set_overlay_enabled(touchmap,
                                          !touchmap->overlay_enabled);
}

void
sc_touchmap_state_set_edit_mode(struct sc_touchmap_state *touchmap,
                                bool edit_mode) {
    touchmap->edit_mode = edit_mode;
    if (!edit_mode) {
        touchmap->add_menu_open = false;
    }
}

struct sc_touchmap_switch_decision
sc_touchmap_switch_decide_foreground_change(
        const struct sc_touchmap_state *state,
        const char *package_name, const char *resolved_file) {
    if (!state->auto_enabled || state->manual_override) {
        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_NONE, package_name, resolved_file);
    }

    if (state->dirty || state->edit_mode) {
        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_DEFER, package_name, resolved_file);
    }

    if (resolved_file) {
        if (state->map && state->file
                && !SDL_strcmp(state->file, resolved_file)) {
            return sc_touchmap_switch_make_decision(
                SC_TOUCHMAP_SWITCH_ACTION_KEEP, package_name, resolved_file);
        }

        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_LOAD, package_name, resolved_file);
    }

    if (state->map) {
        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_UNLOAD, package_name, NULL);
    }

    return sc_touchmap_switch_make_decision(
        SC_TOUCHMAP_SWITCH_ACTION_NONE, package_name, NULL);
}

struct sc_touchmap_switch_decision
sc_touchmap_switch_decide_apply_deferred(
        const struct sc_touchmap_state *state,
        const char *resolved_file) {
    if (!state->auto_enabled || state->manual_override || state->dirty
            || state->edit_mode
            || (!state->deferred_package && !state->deferred_file)) {
        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_NONE, NULL, NULL);
    }

    if (state->deferred_file
            && (!resolved_file
                || SDL_strcmp(resolved_file, state->deferred_file))) {
        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_DROP_DEFERRED,
            state->deferred_package, state->deferred_file);
    }

    if (resolved_file) {
        if (state->map && state->file
                && !SDL_strcmp(state->file, resolved_file)) {
            return sc_touchmap_switch_make_decision(
                SC_TOUCHMAP_SWITCH_ACTION_KEEP,
                state->deferred_package, resolved_file);
        }

        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_LOAD,
            state->deferred_package, resolved_file);
    }

    if (state->map) {
        return sc_touchmap_switch_make_decision(
            SC_TOUCHMAP_SWITCH_ACTION_UNLOAD,
            state->deferred_package, NULL);
    }

    return sc_touchmap_switch_make_decision(
        SC_TOUCHMAP_SWITCH_ACTION_NONE,
        state->deferred_package, NULL);
}
