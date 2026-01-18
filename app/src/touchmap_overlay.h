#ifndef SC_TOUCHMAP_OVERLAY_H
#define SC_TOUCHMAP_OVERLAY_H

#include "common.h"

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "touchmap.h"
#include "options.h"

// Forward declaration to avoid circular includes
struct sc_touchmap_overlay;

// Color constants for the overlay (RGBA)
#define SC_OVERLAY_WALK_COLOR       0x00FF0080  // Green with transparency
#define SC_OVERLAY_BUTTON_COLOR     0xFF000080  // Red with transparency
#define SC_OVERLAY_SKILL_COLOR      0x0000FF80  // Blue with transparency
#define SC_OVERLAY_TEXT_COLOR       0xFFFFFF80  // White with transparency

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
 * @param geometry The geometry where the content is displayed
 * @param orientation The display orientation
 * @return true on success, false on error
 */
bool
sc_touchmap_overlay_render(struct sc_touchmap_overlay *overlay,
                           SDL_Renderer *renderer,
                           const struct sc_gptm_gamepad_touchmap *touchmap,
                           const SDL_Rect *geometry,
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

#endif
