#include "touchmap/touchmap_state.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

#include "events.h"
#include "screen.h"
#include "touchmap/touchmap_utils.h"
#include "third_party/tfd/tinyfiledialogs.h"
#include "util/log.h"
#include "util/thread.h"

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

void
sc_touchmap_state_mark_dirty(struct sc_touchmap_state *touchmap) {
    touchmap->dirty = true;
    touchmap->exit_after_save = false;
}

void
sc_touchmap_state_request_refresh(struct sc_touchmap_state *touchmap) {
    struct sc_screen *screen = touchmap->screen;
    if (screen && screen->video && screen->has_frame) {
        sc_push_event(SC_EVENT_SCREEN_REFRESH);
    }
}

char *
sc_touchmap_state_build_save_dialog_default_path(
    const struct sc_touchmap_state *touchmap) {
    if (touchmap->file) {
        return SDL_strdup(touchmap->file);
    }

    const char *package_name = touchmap->map
                             ? sc_gptm_gamepad_touchmap_get_package_name(
                                   touchmap->map)
                             : NULL;
    if (!package_name) {
        return NULL;
    }

    return sc_touchmap_build_default_filename(package_name);
}

bool
sc_touchmap_state_load_manual_file(struct sc_touchmap_state *touchmap,
                                      const char *touchmap_file) {
    if (!sc_touchmap_state_load_file(touchmap, touchmap_file)) {
        return false;
    }

    if (touchmap->auto_enabled) {
        LOGI("Manual touchmap override enabled: %s", touchmap_file);
        sc_touchmap_state_set_manual_override(touchmap, true);
    }

    return true;
}

bool
sc_touchmap_state_reload_file(struct sc_touchmap_state *touchmap) {
    if (!touchmap->file) {
        LOGW("No touchmap file to reload");
        return false;
    }

    char *absolute_path = sc_file_get_absolute_path(touchmap->file);
    if (!absolute_path) {
        LOGE("Could not resolve touchmap path: %s", touchmap->file);
        return false;
    }

    struct sc_gptm_gamepad_touchmap *map = parse_touchmap_config(absolute_path);
    if (!map) {
        LOGE("Fail to parse touchmap file %s", absolute_path);
        free(absolute_path);
        return false;
    }

    if (touchmap->map) {
        sc_gptm_gamepad_touchmap_destroy(touchmap->map);
    }
    SDL_free(touchmap->file);

    touchmap->map = map;
    touchmap->file = absolute_path;
    sc_touchmap_state_reset_runtime(touchmap);
    return true;
}

bool
sc_touchmap_state_edit_mode_active(const struct sc_touchmap_state *touchmap) {
    return touchmap->map && touchmap->edit_mode;
}

bool
sc_touchmap_state_selection_is_button(const struct sc_touchmap_state *touchmap) {
    if (!touchmap->map) {
        return false;
    }
    struct sc_touchmap_editor_selection selection = touchmap->editor.selection;
    return (selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER
            || selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS)
        && selection.button_index >= 0
        && selection.button_index < touchmap->map->button_cnt;
}

bool
sc_touchmap_state_selection_is_walk(const struct sc_touchmap_state *touchmap) {
    if (!touchmap->map) {
        return false;
    }
    struct sc_touchmap_editor_selection selection = touchmap->editor.selection;
    return (selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER
            || selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS)
        && touchmap->map->has_walk;
}

bool
sc_touchmap_state_apply_loaded_map(struct sc_touchmap_state *touchmap,
                                  struct sc_gptm_gamepad_touchmap *map,
                                  const char *touchmap_file) {
    char *file_dup = touchmap_file ? SDL_strdup(touchmap_file) : NULL;
    if (touchmap_file && !file_dup) {
        LOG_OOM();
        return false;
    }

    if (touchmap->map) {
        sc_gptm_gamepad_touchmap_destroy(touchmap->map);
    }
    SDL_free(touchmap->file);

    touchmap->map = map;
    touchmap->file = file_dup;
    sc_touchmap_state_reset_runtime(touchmap);
    sc_touchmap_state_request_refresh(touchmap);
    return true;
}

bool
sc_touchmap_state_load_file(struct sc_touchmap_state *touchmap,
                           const char *touchmap_file) {
    char *absolute_path = sc_file_get_absolute_path(touchmap_file);
    if (!absolute_path) {
        return false;
    }

    struct sc_gptm_gamepad_touchmap *map = parse_touchmap_config(absolute_path);
    if (!map) {
        LOGE("Fail to parse touchmap file %s", absolute_path);
        free(absolute_path);
        return false;
    }

    if (!sc_touchmap_state_apply_loaded_map(touchmap, map, absolute_path)) {
        sc_gptm_gamepad_touchmap_destroy(map);
        free(absolute_path);
        return false;
    }

    free(absolute_path);
    return true;
}

bool
sc_touchmap_state_replace_string(char **dst, const char *src) {
    char *new_str = SDL_strdup(src);
    if (!new_str) {
        LOG_OOM();
        return false;
    }
    SDL_free(*dst);
    *dst = new_str;
return true;
}

bool
sc_touchmap_state_load_auto_file(struct sc_touchmap_state *touchmap,
                                        const char *touchmap_file) {
    if (!sc_touchmap_state_load_file(touchmap, touchmap_file)) {
        return false;
    }

    sc_touchmap_state_set_manual_override(touchmap, false);
    sc_touchmap_state_clear_deferred(touchmap);
    return true;
}

void
sc_touchmap_state_create_empty(struct sc_touchmap_state *touchmap) {
    if (touchmap->map) {
        LOGW("Touchmap already loaded");
        return;
    }

    struct sc_gptm_gamepad_touchmap *map =
        sc_gptm_gamepad_touchmap_new_empty();
    if (!map) {
        LOGE("Failed to create empty touchmap");
        return;
    }

    if (touchmap->auto_enabled) {
        if (touchmap->current_package) {
            const char *indexed_path =
                sc_touchmap_loader_find_path(&touchmap->loader,
                                            touchmap->current_package);
            if (indexed_path) {
                LOGI("Auto-loading touchmap for %s: %s",
                     touchmap->current_package, indexed_path);
                if (!sc_touchmap_state_load_file(touchmap, indexed_path)) {
                    LOGE("Failed to auto-load touchmap for %s",
                         touchmap->current_package);
                    sc_touchmap_state_set_manual_override(touchmap, true);
                    touchmap->map = map;
                    touchmap->file = NULL;
                    sc_touchmap_state_mark_dirty(touchmap);
                    return;
                }
                sc_gptm_gamepad_touchmap_destroy(map);
                return;
            }
        }
        sc_touchmap_state_set_manual_override(touchmap, true);
    }

    touchmap->map = map;
    touchmap->file = NULL;
    sc_touchmap_state_mark_dirty(touchmap);
    if (touchmap->auto_enabled) {
        LOGI("Enabled manual touchmap override");
    }
    sc_touchmap_state_request_refresh(touchmap);
}

void
sc_touchmap_state_enter_edit_mode(struct sc_touchmap_state *touchmap) {
    if (!touchmap->map) {
        return;
    }

    struct sc_gptm_gamepad_touchmap *map = touchmap->map;
    if (map->has_walk && map->walk.touch_down) {
        map->walk.touch_down = false;
    }
    if (map->has_walk) {
        map->walk.current_pos = map->walk.center;
    }

    for (int i = 0; i < map->button_cnt; ++i) {
        struct sc_gptm_touch_button *btn = &map->buttons[i];
        if (btn->touch_down) {
            btn->touch_down = false;
        }
        btn->current_pos = btn->center;
    }

    map->joystick[0] = (struct sc_point) {0, 0};
    map->joystick[1] = (struct sc_point) {0, 0};

    touchmap->edit_mode = true;
    sc_touchmap_editor_set_mode(&touchmap->editor,
                                 SC_TOUCHMAP_EDITOR_MODE_SELECT);
}

static bool
sc_touchmap_state_get_screen_center(const struct sc_touchmap_state *touchmap,
                                    struct sc_point *point) {
    if (!touchmap->screen) {
        return false;
    }

    struct sc_size frame_size = touchmap->screen->frame_size;
    if (!frame_size.width || !frame_size.height) {
        return false;
    }

    *point = (struct sc_point) {
        .x = frame_size.width / 2,
        .y = frame_size.height / 2,
    };
    return true;
}

bool
sc_touchmap_state_add_button_at_center(struct sc_touchmap_state *touchmap,
                                       bool skill) {
    if (!touchmap->map) {
        return false;
    }

    struct sc_point center;
    if (!sc_touchmap_state_get_screen_center(touchmap, &center)) {
        return false;
    }

    struct sc_gptm_touch_button button = {
        .center = center,
        .radius = skill ? SC_TOUCHMAP_MIN_RADIUS : 0,
        .button = SC_GPTM_BUTTON_UNBOUND,
        .is_skill = skill,
    };

    int index = -1;
    struct sc_gptm_gamepad_touchmap *map =
        sc_gptm_gamepad_touchmap_add_button(touchmap->map, &button, &index);
    if (!map) {
        return false;
    }

    touchmap->map = map;
    sc_touchmap_editor_select_button(&touchmap->editor, index);
    sc_touchmap_state_mark_dirty(touchmap);
    sc_touchmap_state_request_refresh(touchmap);
    return true;
}

bool
sc_touchmap_state_add_walk_at_center(struct sc_touchmap_state *touchmap) {
    if (!touchmap->map) {
        return false;
    }

    if (touchmap->map->has_walk) {
        sc_touchmap_editor_select_walk(&touchmap->editor);
        sc_touchmap_state_request_refresh(touchmap);
        return true;
    }

    struct sc_point center;
    if (!sc_touchmap_state_get_screen_center(touchmap, &center)) {
        return false;
    }

    if (!sc_gptm_gamepad_touchmap_set_walk(touchmap->map, center,
                                           SC_TOUCHMAP_MIN_RADIUS)) {
        return false;
    }

    sc_touchmap_editor_select_walk(&touchmap->editor);
    sc_touchmap_state_mark_dirty(touchmap);
    sc_touchmap_state_request_refresh(touchmap);
    return true;
}

void
sc_touchmap_state_free_up(struct sc_touchmap_state *touchmap) {
    if (touchmap->map) {
        sc_gptm_gamepad_touchmap_destroy(touchmap->map);
        touchmap->map = NULL;
    }
    if (touchmap->file) {
        SDL_free(touchmap->file);
        touchmap->file = NULL;
    }
    sc_touchmap_state_reset_runtime(touchmap);
    sc_touchmap_state_request_refresh(touchmap);
}

void
sc_touchmap_state_set_manual_override(struct sc_touchmap_state *touchmap,
                                      bool enabled) {
    if (touchmap->manual_override == enabled) {
        return;
    }
    touchmap->manual_override = enabled;
    if (enabled) {
        sc_touchmap_state_clear_deferred(touchmap);
    }
}

bool
sc_touchmap_state_set_deferred_switch(struct sc_touchmap_state *touchmap,
                                       const char *package_name,
                                       const char *touchmap_file) {
    if (!sc_touchmap_state_replace_string(&touchmap->deferred_package,
                                           package_name)) {
        return false;
    }

    if (!sc_touchmap_state_replace_string(&touchmap->deferred_file,
                                           touchmap_file)) {
        sc_touchmap_state_replace_string(&touchmap->deferred_package, NULL);
        return false;
    }

    return true;
}

static int
save_touchmap_dialog_thread(void *data) {
    struct sc_touchmap_state *touchmap =
        (struct sc_touchmap_state *) data;

    char *default_path =
        sc_touchmap_state_build_save_dialog_default_path(touchmap);

    char const *lFilterPatterns[2] = {"*.json", "*.*"};
    char *file_name = tinyfd_saveFileDialog(
        "Save Touch Map File",
        default_path ? default_path : "",
        2,
        lFilterPatterns,
        "JSON file");
    SDL_free(default_path);

    if (file_name == NULL) {
        LOGI("Save File cancelled");
        SDL_Event event;
        event.type = SC_EVENT_TOUCHMAP_SAVE;
        event.user.code = 1;
        event.user.data1 = NULL;
        SDL_PushEvent(&event);
        return 1;
    }

    LOGI("Selected save file: %s", file_name);

    SDL_Event event;
    event.type = SC_EVENT_TOUCHMAP_SAVE;
    event.user.code = 0;

    int len = SDL_strlen(file_name) + 1;
    event.user.data1 = SDL_malloc(len);
    SDL_strlcpy(event.user.data1, file_name, len);
    SDL_PushEvent(&event);

    return 0;
}

static int
open_file_dialog_thread(void *data) {
    struct sc_touchmap_state *touchmap = data;
    (void) touchmap;

    char const *lFilterPatterns[2] = {"*.json", "*.*"};
    char *file_name = tinyfd_openFileDialog(
        "Open Touch Map File",
        "",
        2,
        lFilterPatterns,
        "JSON file",
        0);

    if (file_name == NULL) {
        LOGI("Open File cancelled");
        return 1;
    }

    LOGI("Selected file: %s", file_name);

    SDL_Event event;
    event.type = SC_EVENT_FILE_DIALOG;
    event.user.code = 0;

    int len = SDL_strlen(file_name) + 1;
    event.user.data1 = SDL_malloc(len);
    SDL_strlcpy(event.user.data1, file_name, len);
    SDL_PushEvent(&event);

    return 0;
}

void
sc_touchmap_state_save_to_file(struct sc_touchmap_state *touchmap,
                               const char *filename) {
    if (!filename || !touchmap->map) {
        return;
    }

    if (sc_gptm_gamepad_touchmap_has_unbound_buttons(touchmap->map)) {
        tinyfd_messageBox(
            "Cannot Save Touch Map",
            "Some touchmap controls are not bound to a gamepad input.\n\n"
            "Bind every red control before saving.",
            "ok",
            "warning",
            1);
        touchmap->exit_after_save = false;
        return;
    }

    if (!save_touchmap_config(filename, touchmap->map)) {
        LOGE("Fail to save touchmap file %s", filename);
        touchmap->exit_after_save = false;
        return;
    }

    char *absolute_path = sc_file_get_absolute_path(filename);
    if (!absolute_path) {
        touchmap->exit_after_save = false;
        return;
    }

    if (!sc_touchmap_state_replace_string(&touchmap->file, absolute_path)) {
        free(absolute_path);
        touchmap->exit_after_save = false;
        return;
    }

    if (touchmap->auto_enabled) {
        bool in_auto_dir = touchmap->dir
            && !SDL_strncmp(absolute_path, touchmap->dir,
                SDL_strlen(touchmap->dir));
        if (in_auto_dir) {
            if (sc_touchmap_loader_rebuild_index(&touchmap->loader)) {
                const char *package_name =
                    sc_gptm_gamepad_touchmap_get_package_name(
                        touchmap->map);
                const char *indexed_path = package_name
                    ? sc_touchmap_loader_find_path(&touchmap->loader,
                                                   package_name)
                    : NULL;
                if (package_name && touchmap->current_package
                        && !SDL_strcmp(package_name,
                            touchmap->current_package)
                        && indexed_path
                        && !SDL_strcmp(absolute_path, indexed_path)) {
                    LOGI("Saved touchmap is now the active "
                         "auto-loaded map for %s", package_name);
                    sc_touchmap_state_set_manual_override(
                        touchmap, false);
                } else {
                    LOGI("Saved touchmap stays as manual override");
                    sc_touchmap_state_set_manual_override(
                        touchmap, true);
                }
            } else {
                LOGW("Could not refresh touchmap index after save");
                sc_touchmap_state_set_manual_override(touchmap, true);
            }
        } else {
            LOGI("Saved touchmap outside auto directory, keeping "
                 "manual override");
            sc_touchmap_state_set_manual_override(touchmap, true);
        }
    }

    free(absolute_path);

    touchmap->dirty = false;
    if (touchmap->exit_after_save) {
        sc_touchmap_state_set_edit_mode(touchmap, false);
        touchmap->exit_after_save = false;
        sc_touchmap_state_request_refresh(touchmap);
    }
}

void
sc_touchmap_state_quit_edit_mode(struct sc_touchmap_state *touchmap) {
    if (!touchmap->map) {
        return;
    }

    sc_touchmap_editor_set_mode(&touchmap->editor,
                                SC_TOUCHMAP_EDITOR_MODE_SELECT);

    if (!touchmap->dirty) {
        touchmap->exit_after_save = false;
        touchmap->edit_mode = false;
        return;
    }

    int choice = tinyfd_messageBox(
        "Save Touch Map?",
        "Save changes before exiting edit mode?",
        "yesnocancel",
        "question",
        1);

    if (choice == 1) {
        touchmap->exit_after_save = true;
        sc_touchmap_start_thread("SaveTouchMap",
                                 save_touchmap_dialog_thread, touchmap);
    } else if (choice == 2) {
        if (!touchmap->file) {
            sc_touchmap_state_free_up(touchmap);
        } else {
            sc_touchmap_state_reload_file(touchmap);
        }
        touchmap->edit_mode = false;
    } else {
        touchmap->exit_after_save = false;
    }
}

void
sc_touchmap_state_save(struct sc_touchmap_state *touchmap, bool save_as) {
    if (!touchmap->map) {
        return;
    }

    if (save_as || !touchmap->file) {
        sc_touchmap_start_thread("SaveTouchMap",
                                 save_touchmap_dialog_thread, touchmap);
    } else {
        sc_touchmap_state_save_to_file(touchmap, touchmap->file);
    }
}

void
sc_touchmap_state_open_file_dialog(struct sc_touchmap_state *touchmap) {
    sc_touchmap_start_thread("FileDialogThread", open_file_dialog_thread,
                             touchmap);
}

void
sc_touchmap_state_handle_open_dialog_result(struct sc_touchmap_state *touchmap,
                                            const char *filename) {
    if (!filename) {
        return;
    }

    LOGI("Got FILE OPEN Event with file name: %s", filename);
    sc_touchmap_state_load_manual_file(touchmap, filename);
}

void
sc_touchmap_state_handle_save_dialog_result(struct sc_touchmap_state *touchmap,
                                            bool cancelled,
                                            const char *filename) {
    if (cancelled) {
        touchmap->exit_after_save = false;
        return;
    }

    if (!filename) {
        return;
    }

    LOGI("Got TOUCHMAP SAVE Event with file name: %s", filename);
    sc_touchmap_state_save_to_file(touchmap, filename);
}

bool
sc_touchmap_state_init(struct sc_touchmap_state *touchmap,
                      struct sc_screen *screen,
                      const char *dir, bool auto_enabled) {
    touchmap->screen = screen;
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

    touchmap->dirty = false;
    touchmap->exit_after_save = false;

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

void
sc_touchmap_state_maybe_apply_deferred_switch(struct sc_touchmap_state *touchmap) {
    const char *resolved = NULL;
    if (touchmap->deferred_package) {
        resolved = sc_touchmap_loader_find_path(&touchmap->loader,
                                              touchmap->deferred_package);
    }

    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_apply_deferred(touchmap, resolved);

    if (decision.action == SC_TOUCHMAP_SWITCH_ACTION_NONE) {
        return;
    }

    if (decision.action == SC_TOUCHMAP_SWITCH_ACTION_DROP_DEFERRED) {
        LOGI("Dropping deferred touchmap switch for %s: target changed",
             touchmap->deferred_package
                ? touchmap->deferred_package : "(none)");
        sc_touchmap_state_clear_deferred(touchmap);
        return;
    }

    if (decision.action == SC_TOUCHMAP_SWITCH_ACTION_KEEP) {
        sc_touchmap_state_clear_deferred(touchmap);
        return;
    }

    sc_touchmap_state_apply_auto_target(touchmap, decision.package_name,
                                      decision.touchmap_file);
}

bool
sc_touchmap_state_apply_auto_target(struct sc_touchmap_state *touchmap,
                                    const char *package_name,
                                    const char *touchmap_file) {
    if (touchmap_file) {
        if (touchmap->map && touchmap->file
                && !SDL_strcmp(touchmap->file, touchmap_file)) {
            LOGI("Foreground package %s already uses %s",
                 package_name ? package_name : "(none)", touchmap_file);
            sc_touchmap_state_clear_deferred(touchmap);
            return true;
        }

        LOGI("Auto-loading touchmap for %s: %s",
             package_name ? package_name : "(none)", touchmap_file);
        if (!sc_touchmap_state_load_file(touchmap, touchmap_file)) {
            return false;
        }
    } else {
        if (touchmap->map) {
            LOGI("No auto touchmap for %s, unloading current touchmap",
                 package_name ? package_name : "(none)");
            sc_touchmap_state_free_up(touchmap);
        } else {
            LOGD("No auto touchmap for %s",
                 package_name ? package_name : "(none)");
        }
    }

    sc_touchmap_state_set_manual_override(touchmap, false);
    return true;
}

void
sc_touchmap_state_on_foreground_app_changed(
        struct sc_touchmap_state *touchmap,
        const char *package_name) {
    sc_touchmap_state_replace_string(&touchmap->current_package,
                                     package_name);

    if (!touchmap->auto_enabled || touchmap->manual_override) {
        return;
    }

    const char *touchmap_file =
        sc_touchmap_loader_find_path(&touchmap->loader, package_name);
    struct sc_touchmap_switch_decision decision =
        sc_touchmap_switch_decide_foreground_change(touchmap, package_name,
                                                    touchmap_file);
    LOGI("Foreground package %s resolved to %s",
         package_name ? package_name : "(none)",
         touchmap_file ? touchmap_file : "(no touchmap)");
    if (decision.action == SC_TOUCHMAP_SWITCH_ACTION_DEFER) {
        if (sc_touchmap_state_set_deferred_switch(touchmap,
                                                  decision.package_name,
                                                  decision.touchmap_file)) {
            LOGI("Deferring auto touchmap switch for %s",
                 package_name ? package_name : "(none)");
        }
    } else if (decision.action != SC_TOUCHMAP_SWITCH_ACTION_NONE
            && decision.action != SC_TOUCHMAP_SWITCH_ACTION_KEEP) {
        sc_touchmap_state_apply_auto_target(touchmap, decision.package_name,
                                            decision.touchmap_file);
    }
}

void
sc_touchmap_state_resume_auto_mode(struct sc_touchmap_state *touchmap) {
    if (!touchmap->auto_enabled) {
        sc_touchmap_state_set_manual_override(touchmap, false);
        return;
    }

    LOGI("Manual touchmap override cleared");
    sc_touchmap_state_set_manual_override(touchmap, false);
    if (touchmap->current_package) {
        sc_touchmap_state_apply_auto_target(touchmap,
            touchmap->current_package,
            sc_touchmap_loader_find_path(&touchmap->loader,
                touchmap->current_package));
    } else {
        sc_touchmap_state_set_manual_override(touchmap, false);
    }
}

bool
sc_touchmap_has_ctrl_modifier(void) {
    return SDL_GetModState() & KMOD_CTRL;
}
