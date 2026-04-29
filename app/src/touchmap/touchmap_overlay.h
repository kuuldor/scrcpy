#ifndef SC_TOUCHMAP_OVERLAY_H
#define SC_TOUCHMAP_OVERLAY_H

#include "common.h"

#include <stdint.h>

#include <SDL2/SDL_rect.h>

#include "coords.h"
#include "options.h"

struct sc_point
sc_touchmap_overlay_transform_point(const struct sc_point *point,
                                    const struct sc_size *frame_size,
                                    const SDL_Rect *content_rect,
                                    enum sc_orientation orientation);

int32_t
sc_touchmap_overlay_transform_radius(int32_t radius,
                                     const struct sc_size *frame_size,
                                     const SDL_Rect *content_rect,
                                     enum sc_orientation orientation);

#endif
