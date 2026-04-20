#ifndef TOUCHMAP_H
#define TOUCHMAP_H

#include <stdbool.h>

#include "coords.h"

#define SC_GPTM_BASE_FINGER_ID   UINT64_C(100)

#define SC_GPTM_WALK_CONTROL_DEADZONE   25
#define SC_TOUCHMAP_MIN_RADIUS          32
#define SC_GPTM_BUTTON_UNBOUND          UINT8_MAX

struct cJSON;

struct sc_gptm_walk_control {
    struct sc_point center;
    int32_t radius;

    struct sc_point current_pos;
    bool touch_down;

    uint64_t finger_id;

    struct cJSON *json_entry;
};

struct sc_gptm_touch_button {
    struct sc_point center;
    int32_t radius;
    
    struct sc_point current_pos;
    bool touch_down;

    uint64_t finger_id;

    uint8_t button;
    bool is_skill;

    struct cJSON *json_entry;
};

struct sc_gptm_gamepad_touchmap {
    struct sc_point joystick[2];
    bool has_walk;
    struct sc_gptm_walk_control walk;
    struct cJSON *json_root;
    int button_cnt;
    struct sc_gptm_touch_button buttons[0];
};

struct sc_gptm_gamepad_touchmap *sc_gptm_gamepad_touchmap_new_empty(void);
bool sc_gptm_touch_button_is_bound(const struct sc_gptm_touch_button *button);
bool sc_gptm_gamepad_touchmap_set_walk(struct sc_gptm_gamepad_touchmap *map,
                                       struct sc_point center,
                                       int32_t radius);
bool sc_gptm_gamepad_touchmap_remove_walk(
    struct sc_gptm_gamepad_touchmap *map);
struct sc_gptm_gamepad_touchmap *
sc_gptm_gamepad_touchmap_add_button(struct sc_gptm_gamepad_touchmap *map,
                                    const struct sc_gptm_touch_button *button,
                                    int *out_index);
struct sc_gptm_gamepad_touchmap *
sc_gptm_gamepad_touchmap_remove_button(struct sc_gptm_gamepad_touchmap *map,
                                       int index, int *out_index);
struct sc_gptm_gamepad_touchmap * parse_touchmap_config(const char * filename);
void sc_gptm_gamepad_touchmap_destroy(struct sc_gptm_gamepad_touchmap *map);
bool save_touchmap_config(const char * filename,
                          struct sc_gptm_gamepad_touchmap *map);
int sc_gptm_compare_btn(const void *a, const void *b);

#endif
