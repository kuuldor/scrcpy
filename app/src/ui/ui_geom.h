#ifndef SC_UI_GEOM_H
#define SC_UI_GEOM_H

#include "common.h"

#include <stdbool.h>

#include <SDL2/SDL_rect.h>

#include "coords.h"
#include "options.h"

struct sc_ui_geometry {
    struct sc_size frame_size;
    SDL_Rect content_rect;
    enum sc_orientation orientation;
    bool has_frame;
};

#endif
