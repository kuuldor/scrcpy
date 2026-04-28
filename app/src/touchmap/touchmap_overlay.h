#ifndef SC_TOUCHMAP_OVERLAY_H
#define SC_TOUCHMAP_OVERLAY_H

#include "common.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "touchmap/touchmap.h"
#include "touchmap/touchmap_state.h"
#include "options.h"
#include "coords.h"

// Color constants for the overlay (RGBA)
#define SC_OVERLAY_WALK_COLOR       0xFFFFFF40  // White, low opacity
#define SC_OVERLAY_BUTTON_COLOR     0x6BFF6B55  // Stronger green tint
#define SC_OVERLAY_SKILL_COLOR      0x6B6BFF70  // Stronger blue tint
#define SC_OVERLAY_UNBOUND_COLOR    0xFF3434A0  // Red warning tint
#define SC_OVERLAY_TEXT_COLOR       0xFFFFFFB0  // White, high opacity

#define SC_OVERLAY_DASH_COLOR       0xFFFFFFD0
#define SC_OVERLAY_SELECTION_COLOR  0xFFD23FFF

/**
 * Render the touchmap overlay on top of the display
 *
 * This function should be called after the main texture is rendered
 *
 * @param overlay The overlay to render
 * @param renderer The SDL renderer
 * @param touchmap The touchmap configuration (can be NULL to disable)
 * @param content_rect The geometry where the content is displayed
 * @param orientation The display orientation
 * @return true on success, false on error
 */
bool
sc_touchmap_overlay_render(SDL_Renderer *renderer,
                           const struct sc_touchmap_state *touchmap_state,
                           const struct sc_size *frame_size,
                           const SDL_Rect *content_rect,
                           enum sc_orientation orientation);

#ifdef SC_TEST
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

#endif
