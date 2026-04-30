#include "touchmap/touchmap_runtime.h"

#include <SDL2/SDL.h>

#include "android/input.h"
#include "events.h"
#include "input_manager.h"
#include "screen.h"
#include "touchmap/touchmap_utils.h"
#include "util/log.h"

static bool
sc_touchmap_runtime_simulate_touch(struct sc_input_manager *im,
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

    LOGD("Simulate touch ID(%ld) Point(%d, %d) Action(%d)",
         touch_id, point.x, point.y, action);

    if (!sc_controller_push_msg(im->controller, &msg)) {
        LOGW("Could not request 'inject virtual finger event'");
        return false;
    }

    return true;
}

static void
sc_touchmap_runtime_skill_button_direction(struct sc_input_manager *im,
                                           struct sc_gptm_touch_button *touch_btn,
                                           struct sc_point pos) {
    if (sc_touchmap_state_edit_mode_active(&im->touchmap)) {
        return;
    }

    touch_btn->current_pos.x = touch_btn->center.x
                             + (pos.x * touch_btn->radius / SDL_MAX_SINT16);
    touch_btn->current_pos.y = touch_btn->center.y
                             + (pos.y * touch_btn->radius / SDL_MAX_SINT16);

    sc_touchmap_runtime_simulate_touch(im, touch_btn->finger_id,
                                       AMOTION_EVENT_ACTION_MOVE,
                                       touch_btn->current_pos);
}

struct sc_touchmap_runtime_delayed_skill_ctx {
    Uint32 delay;
    struct sc_input_manager *im;
    struct sc_gptm_touch_button *button;
    struct sc_point pos;
};

static int
sc_touchmap_runtime_delay_skill_event_thread(void *data) {
    struct sc_touchmap_runtime_delayed_skill_ctx *ctx = data;

    SDL_Delay(ctx->delay);
    sc_touchmap_runtime_skill_button_direction(ctx->im, ctx->button, ctx->pos);
    SDL_free(data);
    return 0;
}

static void
sc_touchmap_runtime_handle_walk(struct sc_input_manager *im,
                                struct sc_point pos) {
    if (sc_touchmap_state_edit_mode_active(&im->touchmap)) {
        return;
    }

    if (!im->touchmap.map->has_walk) {
        return;
    }

    struct sc_gptm_walk_control *walk = &im->touchmap.map->walk;

    int wctl_x = pos.x * walk->radius / SDL_MAX_SINT16;
    int wctl_y = pos.y * walk->radius / SDL_MAX_SINT16;

    walk->current_pos.x = walk->center.x + wctl_x;
    walk->current_pos.y = walk->center.y + wctl_y;

    int distance = wctl_x * wctl_x + wctl_y * wctl_y;
    if (distance < SC_GPTM_WALK_CONTROL_DEADZONE) {
        if (walk->touch_down) {
            walk->touch_down = false;
            sc_touchmap_runtime_simulate_touch(im, walk->finger_id,
                                               AMOTION_EVENT_ACTION_UP,
                                               walk->center);
        }
    } else {
        if (!walk->touch_down) {
            walk->touch_down = true;
            sc_touchmap_runtime_simulate_touch(im, walk->finger_id,
                                               AMOTION_EVENT_ACTION_DOWN,
                                               walk->center);
        }
        sc_touchmap_runtime_simulate_touch(im, walk->finger_id,
                                           AMOTION_EVENT_ACTION_MOVE,
                                           walk->current_pos);
    }
}

static void
sc_touchmap_runtime_handle_skill_cast(struct sc_input_manager *im,
                                      struct sc_point pos) {
    if (sc_touchmap_state_edit_mode_active(&im->touchmap)) {
        return;
    }

    struct sc_gptm_gamepad_touchmap *map = im->touchmap.map;
    for (int i = 0; i < map->button_cnt; ++i) {
        struct sc_gptm_touch_button *btn = &map->buttons[i];
        if (btn->is_skill && btn->touch_down) {
            sc_touchmap_runtime_skill_button_direction(im, btn, pos);
        }
    }
}

static void
sc_touchmap_runtime_handle_button(struct sc_input_manager *im,
                                  uint8_t button, uint8_t state) {
    if (sc_touchmap_state_edit_mode_active(&im->touchmap)) {
        return;
    }

    struct sc_gptm_gamepad_touchmap *map = im->touchmap.map;
    struct sc_gptm_touch_button *touch_btn =
        sc_gptm_gamepad_touchmap_find_button(map, button);
    if (!touch_btn) {
        LOGE("Button %d not found in touch map", button);
        return;
    }

    if (state) {
        if (!touch_btn->touch_down) {
            touch_btn->touch_down = true;
            touch_btn->current_pos = touch_btn->center;
            sc_touchmap_runtime_simulate_touch(im, touch_btn->finger_id,
                                               AMOTION_EVENT_ACTION_DOWN,
                                               touch_btn->center);

            if (touch_btn->is_skill) {
                struct sc_point joystick = im->touchmap.map->joystick[1];
                int delta_x = joystick.x * touch_btn->radius / SDL_MAX_SINT16;
                int delta_y = joystick.y * touch_btn->radius / SDL_MAX_SINT16;
                int distance = delta_x * delta_x + delta_y * delta_y;

                if (distance >= SC_GPTM_WALK_CONTROL_DEADZONE) {
                    struct sc_touchmap_runtime_delayed_skill_ctx *ctx =
                        SDL_malloc(sizeof(*ctx));
                    if (ctx) {
                        ctx->delay = 5;
                        ctx->im = im;
                        ctx->button = touch_btn;
                        ctx->pos = joystick;
                        sc_touchmap_start_thread(
                            "DelaySkill",
                            sc_touchmap_runtime_delay_skill_event_thread,
                            ctx);
                    }
                }
            }
        }
    } else if (touch_btn->touch_down) {
        touch_btn->touch_down = false;
        sc_touchmap_runtime_simulate_touch(im, touch_btn->finger_id,
                                           AMOTION_EVENT_ACTION_UP,
                                           touch_btn->current_pos);
    }
}

static void
sc_touchmap_runtime_handle_axis(struct sc_input_manager *im, int idx,
                                int64_t value, bool is_x_axis) {
    if (sc_touchmap_state_edit_mode_active(&im->touchmap)) {
        return;
    }

    struct sc_point *joystick = &im->touchmap.map->joystick[idx];
    if (is_x_axis) {
        joystick->x = value;
    } else {
        joystick->y = value;
    }

    if (idx == 0) {
        sc_touchmap_runtime_handle_walk(im, *joystick);
    } else if (idx == 1) {
        sc_touchmap_runtime_handle_skill_cast(im, *joystick);
    }
}

static void
sc_touchmap_runtime_handle_controller_event(struct sc_input_manager *im,
                                            const SDL_Event *event) {
    switch (event->type) {
        case SDL_CONTROLLERAXISMOTION: {
            LOGD("Gamepad Axis: (%d, %d, %d)", event->caxis.which,
                 event->caxis.axis, event->caxis.value);

            const SDL_ControllerAxisEvent *axis_event = &event->caxis;
            int64_t value = axis_event->value;

            switch (axis_event->axis) {
                case SDL_CONTROLLER_AXIS_LEFTX:
                case SDL_CONTROLLER_AXIS_LEFTY:
                    sc_touchmap_runtime_handle_axis(
                        im, 0, value,
                        axis_event->axis == SDL_CONTROLLER_AXIS_LEFTX);
                    break;
                case SDL_CONTROLLER_AXIS_RIGHTX:
                case SDL_CONTROLLER_AXIS_RIGHTY:
                    sc_touchmap_runtime_handle_axis(
                        im, 1, value,
                        axis_event->axis == SDL_CONTROLLER_AXIS_RIGHTX);
                    break;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    if (value > SDL_MAX_SINT16 / 2) {
                        sc_touchmap_runtime_handle_button(
                            im, SDL_CONTROLLER_BUTTON_MAX + axis_event->axis,
                            1);
                    } else if (value < SDL_MAX_SINT16 / 3) {
                        sc_touchmap_runtime_handle_button(
                            im, SDL_CONTROLLER_BUTTON_MAX + axis_event->axis,
                            0);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            LOGD("Gamepad Button: (%d, %d, %d)", event->cbutton.which,
                 event->cbutton.button, event->cbutton.state);
            sc_touchmap_runtime_handle_button(im, event->cbutton.button,
                                              event->cbutton.state);
            break;
        default:
            break;
    }
}

bool
sc_touchmap_runtime_handle_event(struct sc_input_manager *im,
                                 const SDL_Event *event) {
    switch (event->type) {
        case SDL_CONTROLLERAXISMOTION:
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            sc_touchmap_runtime_handle_controller_event(im, event);
            return true;
        case SC_EVENT_FILE_DIALOG: {
            const char *file_name = event->user.data1;
            sc_touchmap_state_handle_open_dialog_result(&im->touchmap,
                                                        file_name);
            SDL_free(event->user.data1);
            return true;
        }
        case SC_EVENT_TOUCHMAP_SAVE: {
            const char *file_name = event->user.data1;
            sc_touchmap_state_handle_save_dialog_result(&im->touchmap,
                                                        event->user.code == 1,
                                                        file_name);
            SDL_free(event->user.data1);
            return true;
        }
        case SC_EVENT_FG_APP_CHANGED: {
            struct sc_fg_app_changed_event *payload = event->user.data1;
            const char *package_name = payload ? payload->package_name : NULL;
            if (payload) {
                LOGD("Got FG APP CHANGED Event: %s",
                     package_name ? package_name : "(none)");
            }

            sc_touchmap_state_on_foreground_app_changed(&im->touchmap,
                                                        package_name);
            sc_fg_app_changed_event_destroy(payload);
            return true;
        }
        default:
            return false;
    }
}
