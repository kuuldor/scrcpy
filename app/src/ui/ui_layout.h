#ifndef SC_UI_LAYOUT_H
#define SC_UI_LAYOUT_H

#include "common.h"

#include <stdbool.h>

#include <SDL2/SDL_rect.h>

struct sc_ui_row_layout {
    int32_t x;
    int32_t y;
    int32_t item_width;
    int32_t item_height;
    int32_t gap;
};

static inline struct sc_ui_row_layout
sc_ui_row_layout_make(int32_t x, int32_t y, int32_t item_width,
                      int32_t item_height, int32_t gap) {
    return (struct sc_ui_row_layout) {
        .x = x,
        .y = y,
        .item_width = item_width,
        .item_height = item_height,
        .gap = gap,
    };
}

static inline SDL_Rect
sc_ui_row_layout_get_item_rect(const struct sc_ui_row_layout *layout,
                               int index) {
    return (SDL_Rect) {
        .x = layout->x + index * (layout->item_width + layout->gap),
        .y = layout->y,
        .w = layout->item_width,
        .h = layout->item_height,
    };
}

static inline int32_t
sc_ui_row_layout_total_width(const struct sc_ui_row_layout *layout,
                             int item_count) {
    if (item_count <= 0) {
        return 0;
    }
    return item_count * layout->item_width + (item_count - 1) * layout->gap;
}

#endif
