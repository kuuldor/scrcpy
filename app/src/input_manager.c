#include "input_manager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>

#include "android/input.h"
#include "android/keycodes.h"
#include "input_events.h"
#include "screen.h"
#include "shortcut_mod.h"
#include "util/log.h"

#include "touchmap.h"
#include "touchmap_overlay.h"
#include "third_party/tfd/tinyfiledialogs.h"
#include "events.h"

static int save_touchmap_dialog_thread(void *data);
static void sc_start_thread(const char *name, SDL_ThreadFunction fn, void *data);
static void create_empty_touchmap(struct sc_input_manager *im);

void
sc_input_manager_init(struct sc_input_manager *im,
                      const struct sc_input_manager_params *params) {
    // A key/mouse processor may not be present if there is no controller
    assert((!params->kp && !params->mp && !params->gp) || params->controller);
    // A processor must have ops initialized
    assert(!params->kp || params->kp->ops);
    assert(!params->mp || params->mp->ops);
    assert(!params->gp || params->gp->ops);

    im->controller = params->controller;
    im->fp = params->fp;
    im->screen = params->screen;
    im->kp = params->kp;
    im->mp = params->mp;
    im->gp = params->gp;

    im->mouse_bindings = params->mouse_bindings;
    im->touchmap_file = params->touchmap_file ? SDL_strdup(params->touchmap_file)
                                               : NULL;
    im->gamepad_input_mode = params->gamepad_input_mode;
    sc_touchmap_editor_init(&im->touchmap_editor);
    im->touchmap_dirty = false;
    im->touchmap_exit_after_save = false;
    im->touchmap_consume_left_button_up = false;
    im->legacy_paste = params->legacy_paste;
    im->clipboard_autosync = params->clipboard_autosync;

    im->sdl_shortcut_mods = sc_shortcut_mods_to_sdl(params->shortcut_mods);

    im->vfinger_down = false;
    im->vfinger_invert_x = false;
    im->vfinger_invert_y = false;

    im->mouse_buttons_state = 0;

    im->last_keycode = SDLK_UNKNOWN;
    im->last_mod = 0;
    im->key_repeat = 0;

    im->next_sequence = 1; // 0 is reserved for SC_SEQUENCE_INVALID

    if (im->touchmap_file != NULL) {
        im->game_touchmap = parse_touchmap_config(im->touchmap_file);
        if (im->game_touchmap == NULL) {
            LOGE("Fail to parse touchmap file %s", im->touchmap_file);
        } else {
            // Set the touchmap to the display for overlay rendering
            sc_display_set_touchmap(&im->screen->display, im->game_touchmap);
        }
    }
}

static void
send_keycode(struct sc_input_manager *im, enum android_keycode keycode,
             enum sc_action action, const char *name) {
    assert(im->controller && im->kp);

    // send DOWN event
    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_INJECT_KEYCODE;
    msg.inject_keycode.action = action == SC_ACTION_DOWN
                              ? AKEY_EVENT_ACTION_DOWN
                              : AKEY_EVENT_ACTION_UP;
    msg.inject_keycode.keycode = keycode;
    msg.inject_keycode.metastate = 0;
    msg.inject_keycode.repeat = 0;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'inject %s'", name);
    }
}

static inline void
action_home(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_HOME, action, "HOME");
}

static inline void
action_back(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_BACK, action, "BACK");
}

static inline void
action_app_switch(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_APP_SWITCH, action, "APP_SWITCH");
}

static inline void
action_power(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_POWER, action, "POWER");
}

static inline void
action_volume_up(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_VOLUME_UP, action, "VOLUME_UP");
}

static inline void
action_volume_down(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_VOLUME_DOWN, action, "VOLUME_DOWN");
}

static inline void
action_menu(struct sc_input_manager *im, enum sc_action action) {
    send_keycode(im, AKEYCODE_MENU, action, "MENU");
}

// turn the screen on if it was off, press BACK otherwise
// If the screen is off, it is turned on only on ACTION_DOWN
static void
press_back_or_turn_screen_on(struct sc_input_manager *im,
                             enum sc_action action) {
    assert(im->controller && im->kp);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON;
    msg.back_or_screen_on.action = action == SC_ACTION_DOWN
                                 ? AKEY_EVENT_ACTION_DOWN
                                 : AKEY_EVENT_ACTION_UP;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'press back or turn screen on'");
    }
}

static void
expand_notification_panel(struct sc_input_manager *im) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'expand notification panel'");
    }
}

static void
expand_settings_panel(struct sc_input_manager *im) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_EXPAND_SETTINGS_PANEL;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'expand settings panel'");
    }
}

static void
collapse_panels(struct sc_input_manager *im) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_COLLAPSE_PANELS;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'collapse notification panel'");
    }
}

static bool
get_device_clipboard(struct sc_input_manager *im, enum sc_copy_key copy_key) {
    assert(im->controller && im->kp);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_GET_CLIPBOARD;
    msg.get_clipboard.copy_key = copy_key;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'get device clipboard'");
        return false;
    }

    return true;
}

static bool
set_device_clipboard(struct sc_input_manager *im, bool paste,
                     uint64_t sequence) {
    assert(im->controller && im->kp);

    char *text = SDL_GetClipboardText();
    if (!text) {
        LOGW("Could not get clipboard text: %s", SDL_GetError());
        return false;
    }

    char *text_dup = strdup(text);
    SDL_free(text);
    if (!text_dup) {
        LOGW("Could not strdup input text");
        return false;
    }

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_SET_CLIPBOARD;
    msg.set_clipboard.sequence = sequence;
    msg.set_clipboard.text = text_dup;
    msg.set_clipboard.paste = paste;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        free(text_dup);
        LOGW("Could not request 'set device clipboard'");
        return false;
    }

    return true;
}

static void
set_display_power(struct sc_input_manager *im, bool on) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_SET_DISPLAY_POWER;
    msg.set_display_power.on = on;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'set screen power mode'");
    }
}

static void
switch_fps_counter_state(struct sc_input_manager *im) {
    struct sc_fps_counter *fps_counter = &im->screen->fps_counter;

    // the started state can only be written from the current thread, so there
    // is no ToCToU issue
    if (sc_fps_counter_is_started(fps_counter)) {
        sc_fps_counter_stop(fps_counter);
    } else {
        sc_fps_counter_start(fps_counter);
        // Any error is already logged
    }
}

static void
clipboard_paste(struct sc_input_manager *im) {
    assert(im->controller && im->kp);

    char *text = SDL_GetClipboardText();
    if (!text) {
        LOGW("Could not get clipboard text: %s", SDL_GetError());
        return;
    }
    if (!*text) {
        // empty text
        SDL_free(text);
        return;
    }

    char *text_dup = strdup(text);
    SDL_free(text);
    if (!text_dup) {
        LOGW("Could not strdup input text");
        return;
    }

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_INJECT_TEXT;
    msg.inject_text.text = text_dup;
    if (!sc_controller_push_msg(im->controller, &msg)) {
        free(text_dup);
        LOGW("Could not request 'paste clipboard'");
    }
}

static void
rotate_device(struct sc_input_manager *im) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_ROTATE_DEVICE;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request device rotation");
    }
}

static void
open_hard_keyboard_settings(struct sc_input_manager *im) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_OPEN_HARD_KEYBOARD_SETTINGS;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request opening hard keyboard settings");
    }
}

static void
reset_video(struct sc_input_manager *im) {
    assert(im->controller);

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_RESET_VIDEO;

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request reset video");
    }
}

static void
apply_orientation_transform(struct sc_input_manager *im,
                            enum sc_orientation transform) {
    struct sc_screen *screen = im->screen;
    enum sc_orientation new_orientation =
        sc_orientation_apply(screen->orientation, transform);
    sc_screen_set_orientation(screen, new_orientation);
}

static void
sc_input_manager_process_text_input(struct sc_input_manager *im,
                                    const SDL_TextInputEvent *event) {
    if (!im->kp->ops->process_text) {
        // The key processor does not support text input
        return;
    }

    if (sc_shortcut_mods_is_shortcut_mod(im->sdl_shortcut_mods,
                                         SDL_GetModState())) {
        // A shortcut must never generate text events
        return;
    }

    struct sc_text_event evt = {
        .text = event->text,
    };

    im->kp->ops->process_text(im->kp, &evt);
}

static bool
simulate_virtual_touch(struct sc_input_manager *im,
                        uint64_t touch_id,
                        enum android_motionevent_action action,
                        struct sc_point point) {
    bool up = action == AMOTION_EVENT_ACTION_UP;

    struct sc_control_msg msg;
    msg.type = SC_CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT;
    msg.inject_touch_event.action = action;
    msg.inject_touch_event.position.screen_size = im->screen->frame_size;
    msg.inject_touch_event.position.point = point;
    msg.inject_touch_event.pointer_id = touch_id;
    msg.inject_touch_event.pressure = up ? 0.0f : 1.0f;
    msg.inject_touch_event.action_button = 0;
    msg.inject_touch_event.buttons = 0;

    LOGD("Simulate touch ID(%ld) Point(%d, %d) Action(%d)", touch_id, point.x, point.y, action);

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'inject virtual finger event'");
        return false;
    }

    return true;
}

static bool
simulate_virtual_finger(struct sc_input_manager *im,
                        enum android_motionevent_action action,
                        struct sc_point point) {
    return simulate_virtual_touch(im, SC_POINTER_ID_VIRTUAL_FINGER, action, point);
}

static struct sc_point
inverse_point(struct sc_point point, struct sc_size size,
              bool invert_x, bool invert_y) {
    if (invert_x) {
        point.x = size.width - point.x;
    }
    if (invert_y) {
        point.y = size.height - point.y;
    }
    return point;
}

static inline void
free_up_touchmap(struct sc_input_manager *im) {
    if (im->game_touchmap != NULL) {
        sc_gptm_gamepad_touchmap_destroy(im->game_touchmap);
        im->game_touchmap = NULL;
    }
    if (im->touchmap_file) {
        SDL_free((void *) im->touchmap_file);
        im->touchmap_file = NULL;
    }
    sc_touchmap_editor_reset(&im->touchmap_editor);
    sc_display_set_touchmap(&im->screen->display, NULL);
}

static bool
sc_touchmap_reload_current_file(struct sc_input_manager *im) {
    if (!im->touchmap_file) {
        LOGW("No touchmap file to reload");
        return false;
    }

    struct sc_gptm_gamepad_touchmap *reloaded =
        parse_touchmap_config(im->touchmap_file);
    if (!reloaded) {
        LOGE("Fail to reload touchmap file %s", im->touchmap_file);
        return false;
    }

    sc_gptm_gamepad_touchmap_destroy(im->game_touchmap);
    im->game_touchmap = reloaded;
    sc_display_set_touchmap(&im->screen->display, im->game_touchmap);
    sc_touchmap_editor_reset(&im->touchmap_editor);
    im->touchmap_dirty = false;
    im->touchmap_exit_after_save = false;
    return true;
}

bool
sc_touchmap_drag_is_active(const struct sc_input_manager *im) {
    return sc_touchmap_editor_is_dragging(&im->touchmap_editor);
}

bool
sc_touchmap_has_ctrl_modifier(void) {
    return SDL_GetModState() & KMOD_CTRL;
}

static bool
sc_touchmap_edit_mode_active(const struct sc_input_manager *im) {
    return im->game_touchmap
        && sc_touchmap_overlay_is_edit_mode(&im->screen->display.overlay);
}

static void
sc_touchmap_release_active_touches(struct sc_input_manager *im) {
    struct sc_gptm_gamepad_touchmap *map = im->game_touchmap;
    if (!map) {
        return;
    }

    if (map->has_walk && map->walk.touch_down) {
        map->walk.touch_down = false;
        simulate_virtual_touch(im, map->walk.finger_id,
                               AMOTION_EVENT_ACTION_UP,
                               map->walk.current_pos);
    }
    if (map->has_walk) {
        map->walk.current_pos = map->walk.center;
    }

    for (int i = 0; i < map->button_cnt; ++i) {
        struct sc_gptm_touch_button *btn = &map->buttons[i];
        if (btn->touch_down) {
            btn->touch_down = false;
            simulate_virtual_touch(im, btn->finger_id,
                                   AMOTION_EVENT_ACTION_UP,
                                   btn->current_pos);
        }
        btn->current_pos = btn->center;
    }

    map->joystick[0] = (struct sc_point) {0, 0};
    map->joystick[1] = (struct sc_point) {0, 0};
}

static bool
sc_touchmap_point_in_rect(int32_t x, int32_t y, const SDL_Rect *rect) {
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}

static bool
sc_touchmap_toggle_edit_mode(struct sc_input_manager *im, int32_t x, int32_t y) {
    if (!sc_touchmap_overlay_is_enabled(&im->screen->display.overlay)) {
        return false;
    }

    SDL_Rect rect = sc_touchmap_overlay_get_edit_button_rect(
        &im->screen->rect,
        sc_touchmap_overlay_is_edit_mode(&im->screen->display.overlay));
    if (!sc_touchmap_point_in_rect(x, y, &rect)) {
        return false;
    }

    if (!im->game_touchmap) {
        create_empty_touchmap(im);
        return true;
    }

    bool edit_mode = sc_touchmap_overlay_is_edit_mode(&im->screen->display.overlay);
    if (!edit_mode) {
        sc_touchmap_release_active_touches(im);
        sc_touchmap_overlay_set_edit_mode(&im->screen->display.overlay, true);
        return true;
    }

    if (!im->touchmap_dirty) {
        im->touchmap_exit_after_save = false;
        sc_touchmap_overlay_set_edit_mode(&im->screen->display.overlay, false);
        return true;
    }

    int choice = tinyfd_messageBox(
        "Save Touch Map?",
        "Save changes before exiting edit mode?",
        "yesnocancel",
        "question",
        1);

    if (choice == 1) {
        im->touchmap_exit_after_save = true;
        sc_start_thread("SaveTouchMap", save_touchmap_dialog_thread, im);
    } else if (choice == 2) {
        if (!im->touchmap_file) {
            free_up_touchmap(im);
            sc_touchmap_overlay_set_edit_mode(&im->screen->display.overlay,
                                              false);
        } else if (sc_touchmap_reload_current_file(im)) {
            sc_touchmap_overlay_set_edit_mode(&im->screen->display.overlay,
                                              false);
        }
    } else {
        im->touchmap_exit_after_save = false;
    }

    return true;
}

static int open_file_dialog_thread(void *data) {
    struct sc_input_manager *im = (struct sc_input_manager *)data;
    (void)im;

    char const * lFilterPatterns[2] = {"*.json", "*.*"};
    char * file_name = tinyfd_openFileDialog(
        "Open Touch Map File",
        "",
        2,
        lFilterPatterns,
        "JSON file",
        0
    );

    if (file_name == NULL) {
        LOGI("Open File cancelled");
        return 1;
    }

    LOGI("Selected file: %s", file_name);

    // Send custom event to notify the main thread
    SDL_Event event;
    event.type = SC_EVENT_FILE_DIALOG;
    event.user.code = 0;

    int len = SDL_strlen(file_name) + 1;
    event.user.data1 = SDL_malloc(len);
    SDL_strlcpy(event.user.data1, file_name, len);
    SDL_PushEvent(&event);

    return 0;
}

static int save_touchmap_dialog_thread(void *data) {
    struct sc_input_manager *im = (struct sc_input_manager *)data;

    char const * lFilterPatterns[2] = {"*.json", "*.*"};
    char * file_name = tinyfd_saveFileDialog(
        "Save Touch Map File",
        im->touchmap_file ? im->touchmap_file : "",
        2,
        lFilterPatterns,
        "JSON file"
    );

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

static void sc_start_thread(const char * name, SDL_ThreadFunction fn, void *data) {
    SDL_Thread *thread = SDL_CreateThread(fn, name, data);
    if (!thread) {
        LOGE("Failed to create thread: %s", SDL_GetError());
    } else {
        SDL_DetachThread(thread);
    }
}

static void
open_touchmap_file(struct sc_input_manager *im) {
    assert(im->controller);

    sc_start_thread("FileDialogThread", open_file_dialog_thread, im);
}

static void
create_empty_touchmap(struct sc_input_manager *im) {
    if (im->game_touchmap) {
        LOGW("Touchmap already loaded");
        return;
    }

    struct sc_gptm_gamepad_touchmap *map =
        sc_gptm_gamepad_touchmap_new_empty();
    if (!map) {
        LOGE("Failed to create empty touchmap");
        return;
    }

    im->game_touchmap = map;
    SDL_free((void *) im->touchmap_file);
    im->touchmap_file = NULL;
    im->touchmap_dirty = true;
    im->touchmap_exit_after_save = false;
    sc_touchmap_editor_reset(&im->touchmap_editor);
    sc_display_set_touchmap(&im->screen->display, im->game_touchmap);
    sc_touchmap_overlay_set_enabled(&im->screen->display.overlay, true);
    sc_touchmap_overlay_set_edit_mode(&im->screen->display.overlay, true);
}

static void
save_touchmap_file(struct sc_input_manager *im, const char *filename) {
    if (!filename || !im->game_touchmap) {
        return;
    }

    char *filename_dup = SDL_strdup(filename);
    if (!filename_dup) {
        LOG_OOM();
        im->touchmap_exit_after_save = false;
        return;
    }

    if (!save_touchmap_config(filename, im->game_touchmap)) {
        LOGE("Fail to save touchmap file %s", filename);
        SDL_free(filename_dup);
        im->touchmap_exit_after_save = false;
        return;
    }

    im->touchmap_dirty = false;
    SDL_free((void *) im->touchmap_file);
    im->touchmap_file = filename_dup;
    if (im->touchmap_exit_after_save) {
        sc_touchmap_overlay_set_edit_mode(&im->screen->display.overlay, false);
        im->touchmap_exit_after_save = false;
    }
}



static bool
sc_input_manager_process_touchmap_edit_key(struct sc_input_manager *im,
                                           const SDL_KeyboardEvent *event) {
    if (!sc_touchmap_edit_mode_active(im)) {
        return false;
    }

    SDL_Keycode sdl_keycode = event->keysym.sym;
    if (sdl_keycode != SDLK_LEFT && sdl_keycode != SDLK_RIGHT
            && sdl_keycode != SDLK_UP && sdl_keycode != SDLK_DOWN) {
        return false;
    }

    uint16_t mod = event->keysym.mod;
    if (mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) {
        return false;
    }

    if (event->type != SDL_KEYDOWN) {
        return true;
    }

    int32_t step = mod & KMOD_SHIFT ? 10 : 1;
    int32_t dx = 0;
    int32_t dy = 0;
    int32_t radius_delta = 0;

    switch (sdl_keycode) {
        case SDLK_LEFT:
            dx = -step;
            radius_delta = -step;
            break;
        case SDLK_RIGHT:
            dx = step;
            radius_delta = step;
            break;
        case SDLK_UP:
            dy = -step;
            radius_delta = step;
            break;
        case SDLK_DOWN:
            dy = step;
            radius_delta = -step;
            break;
        default:
            assert(false);
            return true;
    }

    if (sc_touchmap_editor_nudge_selection(&im->touchmap_editor,
                                           im->game_touchmap, dx, dy,
                                           radius_delta)) {
        im->touchmap_dirty = true;
    }

    return true;
}

static void
sc_input_manager_process_key(struct sc_input_manager *im,
                             const SDL_KeyboardEvent *event) {
    // controller is NULL if --no-control is requested
    bool control = im->controller;
    bool paused = im->screen->paused;
    bool video = im->screen->video;

    SDL_Keycode sdl_keycode = event->keysym.sym;
    uint16_t mod = event->keysym.mod;
    bool down = event->type == SDL_KEYDOWN;
    bool ctrl = sc_touchmap_has_ctrl_modifier();
    bool shift = event->keysym.mod & KMOD_SHIFT;
    bool repeat = event->repeat;

    if (sdl_keycode == SDLK_s && ctrl && down && !repeat
            && im->game_touchmap) {
        if (shift || !im->touchmap_file) {
            sc_start_thread("SaveTouchMap", save_touchmap_dialog_thread, im);
        } else {
            save_touchmap_file(im, im->touchmap_file);
        }
        return;
    }

    if (sc_input_manager_process_touchmap_edit_key(im, event)) {
        return;
    }

    // Either the modifier includes a shortcut modifier, or the key
    // press/release is a modifier key.
    // The second condition is necessary to ignore the release of the modifier
    // key (because in this case mod is 0).
    uint16_t mods = im->sdl_shortcut_mods;
    bool is_shortcut = sc_shortcut_mods_is_shortcut_mod(mods, mod)
                    || sc_shortcut_mods_is_shortcut_key(mods, sdl_keycode);

    if (down && !repeat) {
        if (sdl_keycode == im->last_keycode && mod == im->last_mod) {
            ++im->key_repeat;
        } else {
            im->key_repeat = 0;
            im->last_keycode = sdl_keycode;
            im->last_mod = mod;
        }
    }

    if (is_shortcut) {
        enum sc_action action = down ? SC_ACTION_DOWN : SC_ACTION_UP;
        switch (sdl_keycode) {
            case SDLK_h:
                if (im->kp && !shift && !repeat && !paused) {
                    action_home(im, action);
                }
                return;
            case SDLK_b: // fall-through
            case SDLK_BACKSPACE:
                if (im->kp && !shift && !repeat && !paused) {
                    action_back(im, action);
                }
                return;
            case SDLK_s:
                if (im->kp && !shift && !repeat && !paused) {
                    action_app_switch(im, action);
                }
                return;
            case SDLK_m:
                if (im->kp && !shift && !repeat && !paused) {
                    action_menu(im, action);
                }
                return;
            case SDLK_p:
                if (im->kp && !shift && !repeat && !paused) {
                    action_power(im, action);
                }
                return;
            case SDLK_o:
                if (control && !repeat && down && !paused) {
                    bool on = shift;
                    set_display_power(im, on);
                }
                return;
            case SDLK_z:
                if (video && down && !repeat) {
                    sc_screen_set_paused(im->screen, !shift);
                }
                return;
            case SDLK_DOWN:
                if (shift) {
                    if (video && !repeat && down) {
                        apply_orientation_transform(im,
                                                    SC_ORIENTATION_FLIP_180);
                    }
                } else if (im->kp && !paused) {
                    // forward repeated events
                    action_volume_down(im, action);
                }
                return;
            case SDLK_UP:
                if (shift) {
                    if (video && !repeat && down) {
                        apply_orientation_transform(im,
                                                    SC_ORIENTATION_FLIP_180);
                    }
                } else if (im->kp && !paused) {
                    // forward repeated events
                    action_volume_up(im, action);
                }
                return;
            case SDLK_LEFT:
                if (video && !repeat && down) {
                    if (shift) {
                        apply_orientation_transform(im,
                                                    SC_ORIENTATION_FLIP_0);
                    } else {
                        apply_orientation_transform(im,
                                                    SC_ORIENTATION_270);
                    }
                }
                return;
            case SDLK_RIGHT:
                if (video && !repeat && down) {
                    if (shift) {
                        apply_orientation_transform(im,
                                                    SC_ORIENTATION_FLIP_0);
                    } else {
                        apply_orientation_transform(im,
                                                    SC_ORIENTATION_90);
                    }
                }
                return;
            case SDLK_c:
                if (im->kp && !shift && !repeat && down && !paused) {
                    get_device_clipboard(im, SC_COPY_KEY_COPY);
                }
                return;
            case SDLK_x:
                if (im->kp && !shift && !repeat && down && !paused) {
                    get_device_clipboard(im, SC_COPY_KEY_CUT);
                }
                return;
            case SDLK_v:
                if (im->kp && !repeat && down && !paused) {
                    if (shift || im->legacy_paste) {
                        // inject the text as input events
                        clipboard_paste(im);
                    } else {
                        // store the text in the device clipboard and paste,
                        // without requesting an acknowledgment
                        set_device_clipboard(im, true, SC_SEQUENCE_INVALID);
                    }
                }
                return;
            case SDLK_f:
                if (video && !shift && !repeat && down) {
                    sc_screen_toggle_fullscreen(im->screen);
                }
                return;
            case SDLK_w:
                if (video && !shift && !repeat && down) {
                    sc_screen_resize_to_fit(im->screen);
                }
                return;
            case SDLK_g:
                if (video && !shift && !repeat && down) {
                    sc_screen_resize_to_pixel_perfect(im->screen);
                }
                return;
            case SDLK_i:
                if (video && !shift && !repeat && down) {
                    switch_fps_counter_state(im);
                }
                return;
            case SDLK_n:
                if (control && !repeat && down && !paused) {
                    if (shift) {
                        collapse_panels(im);
                    } else if (im->key_repeat == 0) {
                        expand_notification_panel(im);
                    } else {
                        expand_settings_panel(im);
                    }
                }
                return;
            case SDLK_r:
                if (control && !repeat && down && !paused) {
                    if (shift) {
                        reset_video(im);
                    } else {
                        rotate_device(im);
                    }
                }
                return;
            case SDLK_k:
                if (control && !shift && !repeat && down && !paused
                        && im->kp && im->kp->hid) {
                    // Only if the current keyboard is hid
                    open_hard_keyboard_settings(im);
                }
                return;
            case SDLK_t:
                if (control && !repeat && down && !paused
                        && im->kp) {
                    if (shift) {
                        free_up_touchmap(im);
                    } else {
                        // Show OpenFileDialog to select TouchMap file
                        open_touchmap_file(im);
                    }
                }
                return;
            case SDLK_e:
                if (control && !repeat && down && im->screen) {
                    // Toggle touchmap overlay with Ctrl+E
                    if (!sc_touchmap_overlay_is_edit_mode(&im->screen->display.overlay)) {
                        sc_display_toggle_overlay(&im->screen->display);
                    }
                }
                return;                
        }

        return;
    }

    if (!im->kp || paused) {
        return;
    }

    uint64_t ack_to_wait = SC_SEQUENCE_INVALID;
    bool is_ctrl_v = ctrl && !shift && sdl_keycode == SDLK_v && down && !repeat;
    if (im->clipboard_autosync && is_ctrl_v) {
        if (im->legacy_paste) {
            // inject the text as input events
            clipboard_paste(im);
            return;
        }

        // Request an acknowledgement only if necessary
        uint64_t sequence = im->kp->async_paste ? im->next_sequence
                                                : SC_SEQUENCE_INVALID;

        // Synchronize the computer clipboard to the device clipboard before
        // sending Ctrl+v, to allow seamless copy-paste.
        bool ok = set_device_clipboard(im, false, sequence);
        if (!ok) {
            LOGW("Clipboard could not be synchronized, Ctrl+v not injected");
            return;
        }

        if (im->kp->async_paste) {
            // The key processor must wait for this ack before injecting Ctrl+v
            ack_to_wait = sequence;
            // Increment only when the request succeeded
            ++im->next_sequence;
        }
    }

    enum sc_keycode keycode = sc_keycode_from_sdl(sdl_keycode);
    if (keycode == SC_KEYCODE_UNKNOWN) {
        return;
    }

    enum sc_scancode scancode = sc_scancode_from_sdl(event->keysym.scancode);
    if (scancode == SC_SCANCODE_UNKNOWN) {
        return;
    }

    struct sc_key_event evt = {
        .action = sc_action_from_sdl_keyboard_type(event->type),
        .keycode = keycode,
        .scancode = scancode,
        .repeat = event->repeat,
        .mods_state = sc_mods_state_from_sdl(event->keysym.mod),
    };

    assert(im->kp->ops->process_key);
    im->kp->ops->process_key(im->kp, &evt, ack_to_wait);
}

static struct sc_position
sc_input_manager_get_position(struct sc_input_manager *im, int32_t x,
                                                           int32_t y) {
    if (im->mp->relative_mode) {
        // No absolute position
        return (struct sc_position) {
            .screen_size = {0, 0},
            .point = {0, 0},
        };
    }

    return (struct sc_position) {
        .screen_size = im->screen->frame_size,
        .point = sc_screen_convert_window_to_frame_coords(im->screen, x, y),
    };
}

static void
sc_input_manager_process_mouse_motion(struct sc_input_manager *im,
                                      const SDL_MouseMotionEvent *event) {
    if (event->which == SDL_TOUCH_MOUSEID) {
        // simulated from touch events, so it's a duplicate
        return;
    }

    if (sc_touchmap_drag_is_active(im)) {
        struct sc_point point =
            sc_screen_convert_window_to_frame_coords(im->screen, event->x,
                                                     event->y);
        if (sc_touchmap_editor_apply_drag(&im->touchmap_editor,
                                          im->game_touchmap, point)) {
            im->touchmap_dirty = true;
        }
        return;
    }

    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    struct sc_mouse_motion_event evt = {
        .position = sc_input_manager_get_position(im, event->x, event->y),
        .pointer_id = im->vfinger_down ? SC_POINTER_ID_GENERIC_FINGER
                                       : SC_POINTER_ID_MOUSE,
        .xrel = event->xrel,
        .yrel = event->yrel,
        .buttons_state = im->mouse_buttons_state,
    };

    assert(im->mp->ops->process_mouse_motion);
    im->mp->ops->process_mouse_motion(im->mp, &evt);

    // vfinger must never be used in relative mode
    assert(!im->mp->relative_mode || !im->vfinger_down);

    if (im->vfinger_down) {
        assert(!im->mp->relative_mode); // assert one more time
        struct sc_point mouse =
           sc_screen_convert_window_to_frame_coords(im->screen, event->x,
                                                    event->y);
        struct sc_point vfinger = inverse_point(mouse, im->screen->frame_size,
                                                im->vfinger_invert_x,
                                                im->vfinger_invert_y);
        simulate_virtual_finger(im, AMOTION_EVENT_ACTION_MOVE, vfinger);
    }
}

static void
sc_input_manager_process_touch(struct sc_input_manager *im,
                               const SDL_TouchFingerEvent *event) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    if (!im->mp->ops->process_touch) {
        // The mouse processor does not support touch events
        return;
    }

    int dw;
    int dh;
    SDL_GL_GetDrawableSize(im->screen->window, &dw, &dh);

    // SDL touch event coordinates are normalized in the range [0; 1]
    int32_t x = event->x * dw;
    int32_t y = event->y * dh;

    struct sc_touch_event evt = {
        .position = {
            .screen_size = im->screen->frame_size,
            .point =
                sc_screen_convert_drawable_to_frame_coords(im->screen, x, y),
        },
        .action = sc_touch_action_from_sdl(event->type),
        .pointer_id = event->fingerId,
        .pressure = event->pressure,
    };

    im->mp->ops->process_touch(im->mp, &evt);
}

static enum sc_mouse_binding
sc_input_manager_get_binding(const struct sc_mouse_binding_set *bindings,
                             uint8_t sdl_button) {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT:
            return SC_MOUSE_BINDING_CLICK;
        case SDL_BUTTON_RIGHT:
            return bindings->right_click;
        case SDL_BUTTON_MIDDLE:
            return bindings->middle_click;
        case SDL_BUTTON_X1:
            return bindings->click4;
        case SDL_BUTTON_X2:
            return bindings->click5;
        default:
            return SC_MOUSE_BINDING_DISABLED;
    }
}

static void
sc_input_manager_process_mouse_button(struct sc_input_manager *im,
                                      const SDL_MouseButtonEvent *event) {
    if (event->which == SDL_TOUCH_MOUSEID) {
        // simulated from touch events, so it's a duplicate
        return;
    }

    bool control = im->controller;
    bool paused = im->screen->paused;
    bool down = event->type == SDL_MOUSEBUTTONDOWN;

    enum sc_mouse_button button = sc_mouse_button_from_sdl(event->button);
    if (button == SC_MOUSE_BUTTON_UNKNOWN) {
        return;
    }

    if (!down) {
        // Mark the button as released
        im->mouse_buttons_state &= ~button;
    }

    SDL_Keymod keymod = SDL_GetModState();
    bool ctrl_pressed = sc_touchmap_has_ctrl_modifier();
    bool shift_pressed = keymod & KMOD_SHIFT;

    if (!down && event->button == SDL_BUTTON_LEFT
            && im->touchmap_consume_left_button_up) {
        im->touchmap_consume_left_button_up = false;
        return;
    }

    if (down && event->button == SDL_BUTTON_LEFT) {
        int32_t x = event->x;
        int32_t y = event->y;
        sc_screen_hidpi_scale_coords(im->screen, &x, &y);
        if (sc_touchmap_toggle_edit_mode(im, x, y)) {
            im->touchmap_consume_left_button_up = true;
            return;
        }
    }

    if (sc_touchmap_edit_mode_active(im)) {
        if (!down && event->button == SDL_BUTTON_LEFT
                && sc_touchmap_drag_is_active(im)) {
            sc_touchmap_editor_reset_drag(&im->touchmap_editor);
            return;
        }

        if (sc_touchmap_drag_is_active(im)) {
            return;
        }

        if (down && event->button == SDL_BUTTON_LEFT) {
            struct sc_point point = sc_screen_convert_window_to_frame_coords(
                im->screen, event->x, event->y);
            if (!sc_touchmap_editor_try_start_drag(&im->touchmap_editor,
                                                   im->game_touchmap, point)) {
                im->touchmap_consume_left_button_up = true;
            }
        }

        return;
    }

    if (control && !paused) {
        enum sc_action action = down ? SC_ACTION_DOWN : SC_ACTION_UP;

        struct sc_mouse_binding_set *bindings = !shift_pressed
                                              ? &im->mouse_bindings.pri
                                              : &im->mouse_bindings.sec;
        enum sc_mouse_binding binding =
            sc_input_manager_get_binding(bindings, event->button);
        assert(binding != SC_MOUSE_BINDING_AUTO);
        switch (binding) {
            case SC_MOUSE_BINDING_DISABLED:
                // ignore click
                return;
            case SC_MOUSE_BINDING_BACK:
                if (im->kp) {
                    press_back_or_turn_screen_on(im, action);
                }
                return;
            case SC_MOUSE_BINDING_HOME:
                if (im->kp) {
                    action_home(im, action);
                }
                return;
            case SC_MOUSE_BINDING_APP_SWITCH:
                if (im->kp) {
                    action_app_switch(im, action);
                }
                return;
            case SC_MOUSE_BINDING_EXPAND_NOTIFICATION_PANEL:
                if (down) {
                    if (event->clicks < 2) {
                        expand_notification_panel(im);
                    } else {
                        expand_settings_panel(im);
                    }
                }
                return;
            default:
                assert(binding == SC_MOUSE_BINDING_CLICK);
                break;
        }
    }

    // double-click on black borders resizes to fit the device screen
    bool video = im->screen->video;
    bool mouse_relative_mode = im->mp && im->mp->relative_mode;
    if (video && !mouse_relative_mode && event->button == SDL_BUTTON_LEFT
            && event->clicks == 2) {
        int32_t x = event->x;
        int32_t y = event->y;
        sc_screen_hidpi_scale_coords(im->screen, &x, &y);
        SDL_Rect *r = &im->screen->rect;
        bool outside = x < r->x || x >= r->x + r->w
                    || y < r->y || y >= r->y + r->h;
        if (outside) {
            if (down) {
                sc_screen_resize_to_fit(im->screen);
            }
            return;
        }
    }

    if (!im->mp || paused) {
        return;
    }

    if (down) {
        // Mark the button as pressed
        im->mouse_buttons_state |= button;
    }

    if (!down && event->button == SDL_BUTTON_LEFT && sc_touchmap_drag_is_active(im)) {
        sc_touchmap_editor_reset_drag(&im->touchmap_editor);
        return;
    }

    if (sc_touchmap_drag_is_active(im)) {
        return;
    }

    bool change_vfinger = event->button == SDL_BUTTON_LEFT &&
            ((down && !im->vfinger_down && (ctrl_pressed || shift_pressed)) ||
             (!down && im->vfinger_down));
    bool use_finger = im->vfinger_down || change_vfinger;

    struct sc_mouse_click_event evt = {
        .position = sc_input_manager_get_position(im, event->x, event->y),
        .action = sc_action_from_sdl_mousebutton_type(event->type),
        .button = button,
        .pointer_id = use_finger ? SC_POINTER_ID_GENERIC_FINGER
                                 : SC_POINTER_ID_MOUSE,
        .buttons_state = im->mouse_buttons_state,
    };

    LOGD("Mouse Click (%d, %d) -> Touch %ld (%d, %d)", event->x, event->y, evt.pointer_id, evt.position.point.x, evt.position.point.y);
    assert(im->mp->ops->process_mouse_click);
    im->mp->ops->process_mouse_click(im->mp, &evt);

    if (im->mp->relative_mode) {
        assert(!im->vfinger_down); // vfinger must not be used in relative mode
        // No pinch-to-zoom simulation
        return;
    }

    // Pinch-to-zoom, rotate and tilt simulation.
    //
    // If Ctrl is hold when the left-click button is pressed, then
    // pinch-to-zoom mode is enabled: on every mouse event until the left-click
    // button is released, an additional "virtual finger" event is generated,
    // having a position inverted through the center of the screen.
    //
    // In other words, the center of the rotation/scaling is the center of the
    // screen.
    //
    // To simulate a vertical tilt gesture (a vertical slide with two fingers),
    // Shift can be used instead of Ctrl. The "virtual finger" has a position
    // inverted with respect to the vertical axis of symmetry in the middle of
    // the screen.
    //
    // To simulate a horizontal tilt gesture (a horizontal slide with two
    // fingers), Ctrl+Shift can be used. The "virtual finger" has a position
    // inverted with respect to the horizontal axis of symmetry in the middle
    // of the screen. It is expected to be less frequently used, that's why the
    // one-mod shortcuts are assigned to rotation and vertical tilt.
    if (change_vfinger) {
        struct sc_point mouse =
            sc_screen_convert_window_to_frame_coords(im->screen, event->x,
                                                                 event->y);
        if (down) {
            // Ctrl  Shift     invert_x  invert_y
            // ----  ----- ==> --------  --------
            //   0     0           0         0      -
            //   0     1           1         0      vertical tilt
            //   1     0           1         1      rotate
            //   1     1           0         1      horizontal tilt
            im->vfinger_invert_x = ctrl_pressed ^ shift_pressed;
            im->vfinger_invert_y = ctrl_pressed;
        }
        struct sc_point vfinger = inverse_point(mouse, im->screen->frame_size,
                                                im->vfinger_invert_x,
                                                im->vfinger_invert_y);
        enum android_motionevent_action action = down
                                               ? AMOTION_EVENT_ACTION_DOWN
                                               : AMOTION_EVENT_ACTION_UP;
        if (!simulate_virtual_finger(im, action, vfinger)) {
            return;
        }
        im->vfinger_down = down;
    }
}

static void
sc_input_manager_process_mouse_wheel(struct sc_input_manager *im,
                                     const SDL_MouseWheelEvent *event) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    if (!im->mp->ops->process_mouse_scroll) {
        // The mouse processor does not support scroll events
        return;
    }

    // mouse_x and mouse_y are expressed in pixels relative to the window
    int mouse_x;
    int mouse_y;
    uint32_t buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    (void) buttons; // Actual buttons are tracked manually to ignore shortcuts

    struct sc_mouse_scroll_event evt = {
        .position = sc_input_manager_get_position(im, mouse_x, mouse_y),
#if SDL_VERSION_ATLEAST(2, 0, 18)
        .hscroll = event->preciseX,
        .vscroll = event->preciseY,
#else
        .hscroll = event->x,
        .vscroll = event->y,
#endif
        .hscroll_int = event->x,
        .vscroll_int = event->y,
        .buttons_state = im->mouse_buttons_state,
    };

    im->mp->ops->process_mouse_scroll(im->mp, &evt);
}

static void
sc_input_manager_process_gamepad_device(struct sc_input_manager *im,
                                       const SDL_ControllerDeviceEvent *event) {

    if (event->type == SDL_CONTROLLERDEVICEADDED) {
        SDL_GameController *gc = SDL_GameControllerOpen(event->which);
        if (!gc) {
            LOGW("Could not open game controller");
            return;
        }

        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gc);
        if (!joystick) {
            LOGW("Could not get controller joystick");
            SDL_GameControllerClose(gc);
            return;
        }

        if (im->gamepad_input_mode != SC_GAMEPAD_INPUT_MODE_LOCAL) {
            struct sc_gamepad_device_event evt = {
                .gamepad_id = SDL_JoystickInstanceID(joystick),
            };
            im->gp->ops->process_gamepad_added(im->gp, &evt);
        } else {
            LOGD("Local gamepad. Do not send event to server");
        }
    } else if (event->type == SDL_CONTROLLERDEVICEREMOVED) {
        SDL_JoystickID id = event->which;

        SDL_GameController *gc = SDL_GameControllerFromInstanceID(id);
        if (gc) {
            SDL_GameControllerClose(gc);
        } else {
            LOGW("Unknown gamepad device removed");
        }

        if (im->gamepad_input_mode != SC_GAMEPAD_INPUT_MODE_LOCAL) {
            struct sc_gamepad_device_event evt = {
                .gamepad_id = id,
            };
            im->gp->ops->process_gamepad_removed(im->gp, &evt);
        } else {
            LOGD("Local gamepad. Do not send event to server");
        }
    } else {
        // Nothing to do
        return;
    }
}

static void
sc_input_manager_process_gamepad_axis(struct sc_input_manager *im,
                                      const SDL_ControllerAxisEvent *event) {
    enum sc_gamepad_axis axis = sc_gamepad_axis_from_sdl(event->axis);
    if (axis == SC_GAMEPAD_AXIS_UNKNOWN) {
        return;
    }

    struct sc_gamepad_axis_event evt = {
        .gamepad_id = event->which,
        .axis = axis,
        .value = event->value,
    };
    im->gp->ops->process_gamepad_axis(im->gp, &evt);
}

static void
sc_input_manager_process_gamepad_button(struct sc_input_manager *im,
                                       const SDL_ControllerButtonEvent *event) {
    enum sc_gamepad_button button = sc_gamepad_button_from_sdl(event->button);
    if (button == SC_GAMEPAD_BUTTON_UNKNOWN) {
        return;
    }

    struct sc_gamepad_button_event evt = {
        .gamepad_id = event->which,
        .action = sc_action_from_sdl_controllerbutton_type(event->type),
        .button = button,
    };
    im->gp->ops->process_gamepad_button(im->gp, &evt);
}

static bool
is_apk(const char *file) {
    const char *ext = strrchr(file, '.');
    return ext && !strcmp(ext, ".apk");
}

static void
sc_input_manager_process_file(struct sc_input_manager *im,
                              const SDL_DropEvent *event) {
    char *file = strdup(event->file);
    SDL_free(event->file);
    if (!file) {
        LOG_OOM();
        return;
    }

    enum sc_file_pusher_action action;
    if (is_apk(file)) {
        action = SC_FILE_PUSHER_ACTION_INSTALL_APK;
    } else {
        action = SC_FILE_PUSHER_ACTION_PUSH_FILE;
    }
    bool ok = sc_file_pusher_request(im->fp, action, file);
    if (!ok) {
        free(file);
    }
}


static void 
sc_handle_skill_button_direction(struct sc_input_manager *im, struct sc_gptm_touch_button *touch_btn, struct sc_point pos) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    touch_btn->current_pos.x = touch_btn->center.x + (pos.x * touch_btn->radius / SDL_MAX_SINT16);
    touch_btn->current_pos.y = touch_btn->center.y + (pos.y * touch_btn->radius / SDL_MAX_SINT16);

    simulate_virtual_touch(im, touch_btn->finger_id, AMOTION_EVENT_ACTION_MOVE, touch_btn->current_pos);
}

struct delayed_skill_ctx {
    Uint32 delay;
    struct sc_input_manager *im;
    struct sc_gptm_touch_button * button;
    struct sc_point pos;
} ;

static int delay_skill_event_thread(void *data) {
    struct delayed_skill_ctx *ctx = (struct delayed_skill_ctx *)data;

    SDL_Delay(ctx->delay);
    
    sc_handle_skill_button_direction(ctx->im, ctx->button, ctx->pos);

    SDL_free(data);

    return 0;
}

static void 
sc_handle_touchmap_button(struct sc_input_manager *im, uint8_t button, uint8_t state) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    struct sc_gptm_gamepad_touchmap * map = im->game_touchmap;
    struct sc_gptm_touch_button key = {.button = button};
    struct sc_gptm_touch_button * touch_btn = bsearch(&key, map->buttons, map->button_cnt, 
                    sizeof(struct sc_gptm_touch_button), sc_gptm_compare_btn);
    if (touch_btn == NULL) {
        LOGE("Button %d not found in touch map", button);
        return;
    }
    if (state) {
        if (!touch_btn->touch_down) {
            touch_btn->touch_down = true;
            touch_btn->current_pos = touch_btn->center;
            simulate_virtual_touch(im, touch_btn->finger_id, AMOTION_EVENT_ACTION_DOWN, touch_btn->center);

            if (touch_btn->is_skill) {
                // Hard code to use the rigth joystick to control skill 
                struct sc_point joystick = im->game_touchmap->joystick[1];

                int delta_x, delta_y, distance;
                delta_x = joystick.x * touch_btn->radius / SDL_MAX_SINT16;
                delta_y = joystick.y * touch_btn->radius / SDL_MAX_SINT16;
                distance = delta_x * delta_x + delta_y * delta_y;

                if (distance >= SC_GPTM_WALK_CONTROL_DEADZONE) {
                    struct delayed_skill_ctx * ctx = SDL_malloc(sizeof(struct delayed_skill_ctx));
                    if (ctx != NULL) {
                        ctx->delay = 5;
                        ctx->im = im;
                        ctx->button = touch_btn;
                        ctx->pos = joystick;

                        sc_start_thread("DelaySkill", delay_skill_event_thread, ctx);                    
                    }
                }
            }
        }
    } else {
        if (touch_btn->touch_down) {
            touch_btn->touch_down = false;
            simulate_virtual_touch(im, touch_btn->finger_id, AMOTION_EVENT_ACTION_UP, touch_btn->current_pos);
        }
    }
}

static void 
sc_handle_touchmap_walk(struct sc_input_manager *im, struct sc_point pos) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    if (!im->game_touchmap->has_walk) {
        return;
    }

    struct sc_gptm_walk_control *walk = &im->game_touchmap->walk;

    int wctl_x, wctl_y, distance;
    wctl_x = pos.x * walk->radius / SDL_MAX_SINT16;
    wctl_y = pos.y * walk->radius / SDL_MAX_SINT16;

    walk->current_pos.x = walk->center.x + wctl_x;
    walk->current_pos.y = walk->center.y + wctl_y;


    distance = wctl_x * wctl_x + wctl_y * wctl_y;
    if (distance < SC_GPTM_WALK_CONTROL_DEADZONE) {
        if (walk->touch_down) {
            walk->touch_down = false;
            simulate_virtual_touch(im, walk->finger_id, AMOTION_EVENT_ACTION_UP, walk->center);
        }
    } else {
        if (!walk->touch_down) {
            walk->touch_down = true;
            simulate_virtual_touch(im, walk->finger_id, AMOTION_EVENT_ACTION_DOWN, walk->center);
        }
        simulate_virtual_touch(im, walk->finger_id, AMOTION_EVENT_ACTION_MOVE, walk->current_pos);
    }
}

static void 
sc_handle_touchmap_skill_cast(struct sc_input_manager *im, struct sc_point pos) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    struct sc_gptm_gamepad_touchmap *map = im->game_touchmap;

    for (int i = 0; i < map->button_cnt; i++) {
        struct sc_gptm_touch_button * btn = &map->buttons[i];
        if (btn->is_skill && btn->touch_down) {
            sc_handle_skill_button_direction(im, btn, pos);
        }
    }
}

static void 
sc_handle_touchmap_joystick(struct sc_input_manager *im, int idx, int64_t value, bool is_x_axis) {
    if (sc_touchmap_edit_mode_active(im)) {
        return;
    }

    struct sc_point *joystick = &im->game_touchmap->joystick[idx];

    if (is_x_axis) {
        joystick->x = value;
    } else {
        joystick->y = value;
    }

    if (idx == 0) {
        sc_handle_touchmap_walk(im, *joystick);
    } else if (idx == 1) {
        sc_handle_touchmap_skill_cast(im, *joystick);
    }
}


void
sc_input_manager_handle_event(struct sc_input_manager *im,
                              const SDL_Event *event) {
    bool control = im->controller;
    bool paused = im->screen->paused;
    switch (event->type) {
        case SDL_TEXTINPUT:
            if (!im->kp || paused) {
                break;
            }
            sc_input_manager_process_text_input(im, &event->text);
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            // some key events do not interact with the device, so process the
            // event even if control is disabled
            sc_input_manager_process_key(im, &event->key);
            break;
        case SDL_MOUSEMOTION:
            if (!im->mp || paused) {
                break;
            }
            sc_input_manager_process_mouse_motion(im, &event->motion);
            break;
        case SDL_MOUSEWHEEL:
            if (!im->mp || paused) {
                break;
            }
            sc_input_manager_process_mouse_wheel(im, &event->wheel);
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            // some mouse events do not interact with the device, so process
            // the event even if control is disabled
            sc_input_manager_process_mouse_button(im, &event->button);
            break;
        case SDL_FINGERMOTION:
        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
            if (!im->mp || paused) {
                break;
            }
            sc_input_manager_process_touch(im, &event->tfinger);
            break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
            // Handle device added or removed even if paused
            if (!im->gp) {
                break;
            }
            sc_input_manager_process_gamepad_device(im, &event->cdevice);
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (!im->gp || paused) {
                break;
            }

            if (sc_touchmap_edit_mode_active(im)) {
                break;
            }

            LOGD("Gamepad Axis: (%d, %d, %d)", event->caxis.which, event->caxis.axis, event->caxis.value);

            if (im->game_touchmap == NULL) {
                sc_input_manager_process_gamepad_axis(im, &event->caxis);
            } else {
                int64_t value = event->caxis.value;
                switch (event->caxis.axis) {
                case SDL_CONTROLLER_AXIS_LEFTX:
                case SDL_CONTROLLER_AXIS_LEFTY:
                    sc_handle_touchmap_joystick(im, 0, value, event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX);
                    break;
                case SDL_CONTROLLER_AXIS_RIGHTX:
                case SDL_CONTROLLER_AXIS_RIGHTY:
                    sc_handle_touchmap_joystick(im, 1, value, event->caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX);
                    break;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    if (value > SDL_MAX_SINT16 / 2) {
                        sc_handle_touchmap_button(im, SDL_CONTROLLER_BUTTON_MAX+event->caxis.axis, 1);

                    } else if (value < SDL_MAX_SINT16 / 3) {
                        sc_handle_touchmap_button(im, SDL_CONTROLLER_BUTTON_MAX+event->caxis.axis, 0);
                    }
                    break;
                }
            }            
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            if (!im->gp || paused) {
                break;
            }

            if (sc_touchmap_edit_mode_active(im)) {
                break;
            }

            LOGD("Gamepad Button: (%d, %d, %d)", event->cbutton.which, event->cbutton.button, event->cbutton.state);

            if (im->game_touchmap == NULL) {
                sc_input_manager_process_gamepad_button(im, &event->cbutton);
            } else if (im->game_touchmap != NULL) {
                sc_handle_touchmap_button(im, event->cbutton.button, event->cbutton.state);
            }
            break;
        case SDL_DROPFILE: {
            if (!control) {
                break;
            }
            sc_input_manager_process_file(im, &event->drop);
            break;
        }
        case SC_EVENT_FILE_DIALOG: {
            const char * file_name = event->user.data1;
            if (file_name == NULL) {
                break;
            }
            LOGI("Got FILE OPEN Event with file name: %s", file_name);

            free_up_touchmap(im);
            SDL_free((void *) im->touchmap_file);
            im->touchmap_file = SDL_strdup(file_name);
            im->game_touchmap = parse_touchmap_config(file_name);
            im->touchmap_dirty = false;
            im->touchmap_exit_after_save = false;
            if (im->game_touchmap == NULL) {
                LOGE("Fail to parse touchmap file %s", file_name);
                sc_display_set_touchmap(&im->screen->display, NULL);
            } else {
                // Set the touchmap to the display for overlay rendering
                sc_display_set_touchmap(&im->screen->display, im->game_touchmap);
            }
            SDL_free((void*)file_name);
            break;
        }
        case SC_EVENT_TOUCHMAP_SAVE: {
            const char * file_name = event->user.data1;
            if (event->user.code == 1) {
                im->touchmap_exit_after_save = false;
                break;
            }
            if (file_name == NULL) {
                break;
            }
            LOGI("Got TOUCHMAP SAVE Event with file name: %s", file_name);

            save_touchmap_file(im, file_name);
            SDL_free((void*)file_name);
            break;
        }
    }
}
