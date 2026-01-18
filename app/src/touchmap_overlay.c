#include "touchmap_overlay.h"

#include <assert.h>
#include <math.h>

#include "display.h"
#include "util/log.h"
#include "coords.h"

#define OVERLAY_CIRCLE_POINTS 32

/**
 * Helper function to extract RGBA components from a color value
 */
static void
color_to_rgba(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b,
              uint8_t *a) {
    *r = (color >> 24) & 0xFF;
    *g = (color >> 16) & 0xFF;
    *b = (color >> 8) & 0xFF;
    *a = color & 0xFF;
}

/**
 * Draw a filled circle using SDL drawing primitives
 */
static void
draw_filled_circle(SDL_Renderer *renderer, int center_x, int center_y,
                   int radius, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    // Draw filled circle using Bresenham-like algorithm
    for (int y = -radius; y <= radius; ++y) {
        int x = (int)sqrt(radius * radius - y * y);
        SDL_RenderDrawLine(renderer, center_x - x, center_y + y,
                          center_x + x, center_y + y);
    }
}

/**
 * Draw a circle outline (not filled)
 */
static void
draw_circle_outline(SDL_Renderer *renderer, int center_x, int center_y,
                    int radius, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    double angle_step = 2.0 * M_PI / OVERLAY_CIRCLE_POINTS;
    for (int i = 0; i < OVERLAY_CIRCLE_POINTS; ++i) {
        double angle1 = i * angle_step;
        double angle2 = (i + 1) * angle_step;

        int x1 = center_x + (int)(radius * cos(angle1));
        int y1 = center_y + (int)(radius * sin(angle1));
        int x2 = center_x + (int)(radius * cos(angle2));
        int y2 = center_y + (int)(radius * sin(angle2));

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

/**
 * Get button name from SDL button constant
 */
static const char *
button_value_to_name(uint8_t button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return "A";
        case SDL_CONTROLLER_BUTTON_B: return "B";
        case SDL_CONTROLLER_BUTTON_X: return "X";
        case SDL_CONTROLLER_BUTTON_Y: return "Y";
        case SDL_CONTROLLER_BUTTON_BACK: return "BACK";
        case SDL_CONTROLLER_BUTTON_START: return "START";
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "L3";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "R3";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
        case SDL_CONTROLLER_BUTTON_GUIDE: return "GUIDE";
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return "UP";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "DOWN";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "LEFT";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "RIGHT";
        default:
            // Handle triggers
            if (button == SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                return "LT";
            }
            if (button == SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                return "RT";
            }
            return "?";
    }
}

bool
sc_touchmap_overlay_init(struct sc_touchmap_overlay *overlay,
                         SDL_Renderer *renderer) {
    (void)renderer; // Not used in this simple implementation
    overlay->overlay_texture = NULL;
    overlay->last_size.width = 0;
    overlay->last_size.height = 0;
    overlay->enabled = false;
    return true;
}

void
sc_touchmap_overlay_destroy(struct sc_touchmap_overlay *overlay) {
    if (overlay->overlay_texture) {
        SDL_DestroyTexture(overlay->overlay_texture);
        overlay->overlay_texture = NULL;
    }
}

bool
sc_touchmap_overlay_render(struct sc_touchmap_overlay *overlay,
                           SDL_Renderer *renderer,
                           const struct sc_gptm_gamepad_touchmap *touchmap,
                           const SDL_Rect *geometry,
                           enum sc_orientation orientation) {
    (void)orientation; // Simplified implementation doesn't handle rotation yet
    
    if (!overlay->enabled || !touchmap || !geometry) {
        return true;
    }

    // Draw walk control (outer circle)
    if (touchmap->walk.radius > 0) {
        draw_filled_circle(renderer, 
                          touchmap->walk.center.x,
                          touchmap->walk.center.y,
                          touchmap->walk.radius,
                          SC_OVERLAY_WALK_COLOR);
        
        // Draw walk control outline
        draw_circle_outline(renderer,
                           touchmap->walk.center.x,
                           touchmap->walk.center.y,
                           touchmap->walk.radius,
                           0x00FF00FF); // Solid green outline
    }

    // Draw current position for walk control
    if (touchmap->walk.touch_down) {
        draw_filled_circle(renderer,
                          touchmap->walk.current_pos.x,
                          touchmap->walk.current_pos.y,
                          5,
                          0x00FF00FF); // Solid green for current pos
    }

    // Draw button mappings
    for (int i = 0; i < touchmap->button_cnt; ++i) {
        const struct sc_gptm_touch_button *btn = &touchmap->buttons[i];
        uint32_t color = btn->is_skill ? SC_OVERLAY_SKILL_COLOR 
                                       : SC_OVERLAY_BUTTON_COLOR;

        // Draw button area as filled circle with transparency
        draw_filled_circle(renderer, btn->center.x, btn->center.y,
                          btn->radius > 0 ? btn->radius : 20,
                          color);

        // Draw outline circle (solid)
        uint32_t outline_color = btn->is_skill ? 0x0000FFFF : 0xFF0000FF;
        draw_circle_outline(renderer, btn->center.x, btn->center.y,
                           btn->radius > 0 ? btn->radius : 20,
                           outline_color);

        // Draw button indicator if touched
        if (btn->touch_down) {
            draw_filled_circle(renderer, btn->center.x, btn->center.y,
                              10, outline_color);
        }

        // Draw button label (small text indicator at the center)
        // Note: Full text rendering requires additional font handling
        // For now, we just draw a small indicator
        const char *label = button_value_to_name(btn->button);
        LOGV("Button %s at (%d, %d) %s", label, btn->center.x, btn->center.y,
             btn->is_skill ? "[skill]" : "");
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    return true;
}

void
sc_touchmap_overlay_toggle(struct sc_touchmap_overlay *overlay) {
    overlay->enabled = !overlay->enabled;
    LOGI("Touchmap overlay %s", overlay->enabled ? "enabled" : "disabled");
}

void
sc_touchmap_overlay_set_enabled(struct sc_touchmap_overlay *overlay,
                                bool enabled) {
    overlay->enabled = enabled;
}

bool
sc_touchmap_overlay_is_enabled(const struct sc_touchmap_overlay *overlay) {
    return overlay->enabled;
}
