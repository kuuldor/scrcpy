#ifndef SC_TOUCHMAP_OVERLAY_H
#define SC_TOUCHMAP_OVERLAY_H

#include "common.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "touchmap.h"
#include "options.h"
#include "coords.h"

// Forward declaration to avoid circular includes
struct sc_touchmap_overlay;

// Color constants for the overlay (RGBA)
#define SC_OVERLAY_WALK_COLOR       0xFFFFFF40  // White, low opacity
#define SC_OVERLAY_BUTTON_COLOR     0x6BFF6B55  // Stronger green tint
#define SC_OVERLAY_SKILL_COLOR      0x6B6BFF70  // Stronger blue tint
#define SC_OVERLAY_TEXT_COLOR       0xFFFFFFB0  // White, high opacity

#define SC_OVERLAY_DASH_COLOR       0xFFFFFFD0
#define SC_OVERLAY_EDIT_BG_COLOR    0x00000080
#define SC_OVERLAY_EDIT_BG_ACTIVE   0x000000B0
#define SC_OVERLAY_EDIT_BORDER      0xFFFFFFB0

/**
 * Initialize the touchmap overlay
 *
 * @param overlay The overlay structure to initialize
 * @param renderer The SDL renderer to use for drawing
 * @return true on success, false on error
 */
bool
sc_touchmap_overlay_init(struct sc_touchmap_overlay *overlay,
                         SDL_Renderer *renderer);

/**
 * Destroy the touchmap overlay and free resources
 *
 * @param overlay The overlay to destroy
 */
void
sc_touchmap_overlay_destroy(struct sc_touchmap_overlay *overlay);

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
sc_touchmap_overlay_render(struct sc_touchmap_overlay *overlay,
                           SDL_Renderer *renderer,
                           const struct sc_gptm_gamepad_touchmap *touchmap,
                           const struct sc_size *frame_size,
                           const SDL_Rect *content_rect,
                           enum sc_orientation orientation);

/**
 * Toggle overlay visibility
 *
 * @param overlay The overlay to toggle
 */
void
sc_touchmap_overlay_toggle(struct sc_touchmap_overlay *overlay);

/**
 * Set overlay visibility
 *
 * @param overlay The overlay
 * @param enabled Whether to show the overlay
 */
void
sc_touchmap_overlay_set_enabled(struct sc_touchmap_overlay *overlay,
                                bool enabled);

/**
 * Check if overlay is currently enabled
 *
 * @param overlay The overlay
 * @return true if enabled, false otherwise
 */
bool
sc_touchmap_overlay_is_enabled(const struct sc_touchmap_overlay *overlay);

void
sc_touchmap_overlay_set_edit_mode(struct sc_touchmap_overlay *overlay,
                                  bool edit_mode);

bool
sc_touchmap_overlay_is_edit_mode(const struct sc_touchmap_overlay *overlay);

SDL_Rect
sc_touchmap_overlay_get_edit_button_rect(const SDL_Rect *content_rect,
                                         bool edit_mode);

#endif
