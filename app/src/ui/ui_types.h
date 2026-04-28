#ifndef SC_UI_TYPES_H
#define SC_UI_TYPES_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t sc_ui_id;

#define SC_UI_ID_INVALID UINT64_C(0)

enum sc_ui_coord_space {
    SC_UI_COORD_SPACE_DRAWABLE,
    SC_UI_COORD_SPACE_FRAME,
};

enum sc_ui_pointer_button {
    SC_UI_POINTER_BUTTON_NONE = 0,
    SC_UI_POINTER_BUTTON_LEFT = 1,
    SC_UI_POINTER_BUTTON_MIDDLE = 2,
    SC_UI_POINTER_BUTTON_RIGHT = 3,
};

struct sc_ui_input_result {
    bool consumed;
    bool request_refresh;
};

#endif
