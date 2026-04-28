#ifndef SC_UI_EVENT_H
#define SC_UI_EVENT_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>

#include "ui_types.h"

enum sc_ui_event_type {
    SC_UI_EVENT_NONE,

    SC_UI_EVENT_POINTER_MOVE,
    SC_UI_EVENT_POINTER_DOWN,
    SC_UI_EVENT_POINTER_UP,
    SC_UI_EVENT_POINTER_WHEEL,

    SC_UI_EVENT_KEY_DOWN,
    SC_UI_EVENT_KEY_UP,
    SC_UI_EVENT_TEXT_INPUT,

    SC_UI_EVENT_CANCEL,
    SC_UI_EVENT_FOCUS_LOST,
};

struct sc_ui_pointer_event {
    int32_t x;
    int32_t y;
    int32_t xrel;
    int32_t yrel;
    int32_t wheel_x;
    int32_t wheel_y;
    uint8_t button;
    uint8_t buttons_state;
};

struct sc_ui_key_event {
    SDL_Keycode keycode;
    uint16_t mod;
    bool repeat;
};

struct sc_ui_text_event {
    char text[SDL_TEXTINPUTEVENT_TEXT_SIZE];
};

struct sc_ui_event {
    enum sc_ui_event_type type;
    union {
        struct sc_ui_pointer_event pointer;
        struct sc_ui_key_event key;
        struct sc_ui_text_event text;
    } data;
};

#endif
