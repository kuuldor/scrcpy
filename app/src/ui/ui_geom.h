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

static inline struct sc_size
sc_ui_geom_get_logical_size(const struct sc_ui_geometry *geometry) {
    if (sc_orientation_is_swap(geometry->orientation)) {
        return (struct sc_size) {
            .width = geometry->frame_size.height,
            .height = geometry->frame_size.width,
        };
    }

    return geometry->frame_size;
}

static inline bool
sc_ui_geom_point_in_rect(int32_t x, int32_t y, const SDL_Rect *rect) {
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}

#endif
