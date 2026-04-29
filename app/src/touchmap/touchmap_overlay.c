#include "touchmap_overlay.h"

struct sc_point
sc_touchmap_overlay_transform_point(const struct sc_point *point,
                                    const struct sc_size *frame_size,
                                    const SDL_Rect *content_rect,
                                    enum sc_orientation orientation) {
    struct sc_point oriented = *point;
    int32_t fw = frame_size->width;
    int32_t fh = frame_size->height;

    switch (orientation) {
        case SC_ORIENTATION_0:
            break;
        case SC_ORIENTATION_90:
            oriented.x = fh - point->y;
            oriented.y = point->x;
            break;
        case SC_ORIENTATION_180:
            oriented.x = fw - point->x;
            oriented.y = fh - point->y;
            break;
        case SC_ORIENTATION_270:
            oriented.x = point->y;
            oriented.y = fw - point->x;
            break;
        case SC_ORIENTATION_FLIP_0:
            oriented.x = fw - point->x;
            oriented.y = point->y;
            break;
        case SC_ORIENTATION_FLIP_90:
            oriented.x = fh - point->y;
            oriented.y = fw - point->x;
            break;
        case SC_ORIENTATION_FLIP_180:
            oriented.x = point->x;
            oriented.y = fh - point->y;
            break;
        default:
            oriented.x = point->y;
            oriented.y = point->x;
            break;
    }

    if (sc_orientation_is_swap(orientation)) {
        int32_t tmp = fw;
        fw = fh;
        fh = tmp;
    }

    return (struct sc_point) {
        .x = content_rect->x + (int64_t) oriented.x * content_rect->w / fw,
        .y = content_rect->y + (int64_t) oriented.y * content_rect->h / fh,
    };
}

int32_t
sc_touchmap_overlay_transform_radius(int32_t radius,
                                     const struct sc_size *frame_size,
                                     const SDL_Rect *content_rect,
                                     enum sc_orientation orientation) {
    int32_t fw = frame_size->width;
    int32_t fh = frame_size->height;

    if (sc_orientation_is_swap(orientation)) {
        int32_t tmp = fw;
        fw = fh;
        fh = tmp;
    }

    int32_t sx = (int64_t) radius * content_rect->w / fw;
    int32_t sy = (int64_t) radius * content_rect->h / fh;
    return sx < sy ? sx : sy;
}
