#ifndef TOUCHMAP_H
#define TOUCHMAP_H

#include <stdbool.h>

#include "coords.h"

#define SC_GPTM_BASE_FINGER_ID   UINT64_C(100)

#define SC_GPTM_WALK_CONTROL_DEADZONE   25
#define SC_TOUCHMAP_MIN_RADIUS          32

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
    struct sc_gptm_walk_control walk;
    struct cJSON *json_root;
    int button_cnt;
    struct sc_gptm_touch_button buttons[0];
};

struct sc_gptm_gamepad_touchmap * parse_touchmap_config(const char * filename);
void sc_gptm_gamepad_touchmap_destroy(struct sc_gptm_gamepad_touchmap *map);
bool save_touchmap_config(const char * filename,
                          struct sc_gptm_gamepad_touchmap *map);
int sc_gptm_compare_btn(const void *a, const void *b);

#endif
