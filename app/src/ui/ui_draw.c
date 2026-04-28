#include "ui_draw.h"

#include <SDL2/SDL.h>

static bool
sc_ui_draw_set_color(SDL_Renderer *renderer, struct sc_ui_color color) {
    return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b,
                                  color.a) == 0;
}

bool
sc_ui_draw_fill_rect(const struct sc_ui_render_ctx *render_ctx,
                     const SDL_Rect *rect, struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    return SDL_RenderFillRect(renderer, rect) == 0;
}

bool
sc_ui_draw_rect_border(const struct sc_ui_render_ctx *render_ctx,
                       const SDL_Rect *rect, struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    return SDL_RenderDrawRect(renderer, rect) == 0;
}
