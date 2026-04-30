#ifndef SC_TOUCHMAP_STATE_H
#define SC_TOUCHMAP_STATE_H

#include "common.h"

#include <stdbool.h>

#include "touchmap/touchmap.h"
#include "touchmap/touchmap_editor.h"
#include "touchmap/touchmap_loader.h"
#include "util/file.h"

struct sc_screen;

struct sc_touchmap_state {
    struct sc_screen *screen;
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
                     struct sc_screen *screen,
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

void
sc_touchmap_state_mark_dirty(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_request_refresh(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_free_up(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_set_manual_override(struct sc_touchmap_state *touchmap,
                                     bool enabled);

bool
sc_touchmap_state_set_deferred_switch(struct sc_touchmap_state *touchmap,
                                      const char *package_name,
                                      const char *touchmap_file);

bool
sc_touchmap_state_reload_file(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_maybe_apply_deferred_switch(struct sc_touchmap_state *touchmap);

bool
sc_touchmap_state_apply_auto_target(struct sc_touchmap_state *touchmap,
                            const char *package_name,
                            const char *touchmap_file);

void
sc_touchmap_state_on_foreground_app_changed(
    struct sc_touchmap_state *touchmap,
    const char *package_name);

char *
sc_touchmap_state_build_save_dialog_default_path(
    const struct sc_touchmap_state *touchmap);

bool
sc_touchmap_state_load_manual_file(struct sc_touchmap_state *touchmap,
                                    const char *touchmap_file);

bool
sc_touchmap_state_load_auto_file(struct sc_touchmap_state *touchmap,
                                     const char *touchmap_file);

bool
sc_touchmap_state_edit_mode_active(const struct sc_touchmap_state *touchmap);

bool
sc_touchmap_state_selection_is_button(const struct sc_touchmap_state *touchmap);

bool
sc_touchmap_state_selection_is_walk(const struct sc_touchmap_state *touchmap);

bool
sc_touchmap_state_apply_loaded_map(struct sc_touchmap_state *touchmap,
                                  struct sc_gptm_gamepad_touchmap *map,
                                  const char *touchmap_file);

bool
sc_touchmap_state_load_file(struct sc_touchmap_state *touchmap,
                           const char *touchmap_file);

bool
sc_touchmap_state_replace_string(char **dst, const char *src);

void
sc_touchmap_state_create_empty(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_enter_edit_mode(struct sc_touchmap_state *touchmap);

bool
sc_touchmap_state_add_button_at_center(struct sc_touchmap_state *touchmap,
                                       bool skill);

bool
sc_touchmap_state_add_walk_at_center(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_quit_edit_mode(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_save(struct sc_touchmap_state *touchmap, bool save_as);

void
sc_touchmap_state_open_file_dialog(struct sc_touchmap_state *touchmap);

void
sc_touchmap_state_handle_open_dialog_result(struct sc_touchmap_state *touchmap,
                                            const char *filename);

void
sc_touchmap_state_save_to_file(struct sc_touchmap_state *touchmap,
                               const char *filename);

void
sc_touchmap_state_handle_save_dialog_result(struct sc_touchmap_state *touchmap,
                                            bool cancelled,
                                            const char *filename);

bool
sc_touchmap_has_ctrl_modifier(void);

void
sc_touchmap_state_resume_auto_mode(struct sc_touchmap_state *touchmap);

#endif
