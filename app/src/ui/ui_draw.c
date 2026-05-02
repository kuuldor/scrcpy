#include "ui_draw.h"

#include <math.h>

#include <SDL2/SDL.h>

#include "ui_context.h"

#define SC_UI_DRAW_CIRCLE_POINTS 32

static bool
sc_ui_draw_set_color(SDL_Renderer *renderer, struct sc_ui_color color) {
    return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b,
                                  color.a) == 0;
}

bool
sc_ui_draw_fill_rect(const struct sc_ui_render_ctx *render_ctx,
                     const SDL_Rect *rect, struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    SDL_Rect drawable_rect = *rect;
    if (render_ctx->ui) {
        if (!sc_ui_context_logical_to_drawable_rect(render_ctx->ui, rect,
                                                    &drawable_rect)) {
            drawable_rect = *rect;
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    return SDL_RenderFillRect(renderer, &drawable_rect) == 0;
}

bool
sc_ui_draw_rect_border(const struct sc_ui_render_ctx *render_ctx,
                       const SDL_Rect *rect, struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    SDL_Rect drawable_rect = *rect;
    if (render_ctx->ui) {
        if (!sc_ui_context_logical_to_drawable_rect(render_ctx->ui, rect,
                                                    &drawable_rect)) {
            drawable_rect = *rect;
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    return SDL_RenderDrawRect(renderer, &drawable_rect) == 0;
}

bool
sc_ui_draw_fill_circle(const struct sc_ui_render_ctx *render_ctx,
                       struct sc_point center, int32_t radius,
                       struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    struct sc_point drawable_center = center;
    if (render_ctx->ui) {
        if (!sc_ui_context_logical_to_drawable_point(render_ctx->ui, center,
                                                     &drawable_center)) {
            drawable_center = center;
        }
        radius = sc_ui_context_logical_to_drawable_length(render_ctx->ui,
                                                          radius);
    }

    if (radius < 1) {
        radius = 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    for (int32_t y = -radius; y <= radius; ++y) {
        int32_t x = (int32_t) sqrt((double) radius * radius
                                   - (double) y * y);
        if (SDL_RenderDrawLine(renderer, drawable_center.x - x,
                               drawable_center.y + y,
                               drawable_center.x + x,
                               drawable_center.y + y)) {
            return false;
        }
    }

    return true;
}

bool
sc_ui_draw_circle_outline(const struct sc_ui_render_ctx *render_ctx,
                          struct sc_point center, int32_t radius,
                          struct sc_ui_color color, bool dashed) {
    SDL_Renderer *renderer = render_ctx->renderer;
    struct sc_point drawable_center = center;
    if (render_ctx->ui) {
        if (!sc_ui_context_logical_to_drawable_point(render_ctx->ui, center,
                                                     &drawable_center)) {
            drawable_center = center;
        }
        radius = sc_ui_context_logical_to_drawable_length(render_ctx->ui,
                                                          radius);
    }

    if (radius < 1) {
        radius = 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    double angle_step = 2.0 * acos(-1.0) / SC_UI_DRAW_CIRCLE_POINTS;
    for (int i = 0; i < SC_UI_DRAW_CIRCLE_POINTS; ++i) {
        if (dashed && i % 2) {
            continue;
        }

        double angle1 = i * angle_step;
        double angle2 = (i + 1) * angle_step;
        int x1 = drawable_center.x + (int) (radius * cos(angle1));
        int y1 = drawable_center.y + (int) (radius * sin(angle1));
        int x2 = drawable_center.x + (int) (radius * cos(angle2));
        int y2 = drawable_center.y + (int) (radius * sin(angle2));
        if (SDL_RenderDrawLine(renderer, x1, y1, x2, y2)) {
            return false;
        }
    }

    return true;
}

bool
sc_ui_draw_bitmap_icon(const struct sc_ui_render_ctx *render_ctx,
                       int32_t center_x, int32_t center_y,
                       const uint64_t *rows, int width, int height, int scale,
                       struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    struct sc_point drawable_center = {.x = center_x, .y = center_y};
    if (render_ctx->ui) {
        sc_ui_context_logical_to_drawable_point(render_ctx->ui,
                                                (struct sc_point) {
                                                    .x = center_x,
                                                    .y = center_y,
                                                },
                                                &drawable_center);
        scale = sc_ui_context_logical_to_drawable_length(render_ctx->ui,
                                                         scale > 0 ? scale : 1);
    }

    if (scale < 1) {
        scale = 1;
    }

    int drawable_width = width * scale;
    int drawable_height = height * scale;
    int start_x = drawable_center.x - drawable_width / 2;
    int start_y = drawable_center.y - drawable_height / 2;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    for (int row = 0; row < height; ++row) {
        uint64_t row_bits = rows[row];
        for (int col = 0; col < width; ++col) {
            if (!(row_bits & (UINT64_C(1) << (width - 1 - col)))) {
                continue;
            }

            SDL_Rect pixel_rect = {
                .x = start_x + col * scale,
                .y = start_y + row * scale,
                .w = scale,
                .h = scale,
            };
            if (SDL_RenderFillRect(renderer, &pixel_rect)) {
                return false;
            }
        }
    }

    return true;
}

bool
sc_ui_draw_bitmap_icon_in_rect(const struct sc_ui_render_ctx *render_ctx,
                               const SDL_Rect *rect, const uint64_t *rows,
                               int width, int height, struct sc_ui_color color) {
    SDL_Renderer *renderer = render_ctx->renderer;
    SDL_Rect drawable_rect = *rect;
    if (render_ctx->ui) {
        if (!sc_ui_context_logical_to_drawable_rect(render_ctx->ui, rect,
                                                    &drawable_rect)) {
            drawable_rect = *rect;
        }
    }

    if (drawable_rect.w < 1 || drawable_rect.h < 1) {
        return true;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_draw_set_color(renderer, color)) {
        return false;
    }

    for (int row = 0; row < height; ++row) {
        uint64_t row_bits = rows[row];
        int y0 = drawable_rect.y + (int64_t) row * drawable_rect.h / height;
        int y1 = drawable_rect.y + (int64_t) (row + 1) * drawable_rect.h / height;
        if (y1 <= y0) {
            y1 = y0 + 1;
        }

        for (int col = 0; col < width; ++col) {
            if (!(row_bits & (UINT64_C(1) << (width - 1 - col)))) {
                continue;
            }

            int x0 = drawable_rect.x + (int64_t) col * drawable_rect.w / width;
            int x1 = drawable_rect.x + (int64_t) (col + 1) * drawable_rect.w / width;
            if (x1 <= x0) {
                x1 = x0 + 1;
            }

            SDL_Rect pixel_rect = {
                .x = x0,
                .y = y0,
                .w = x1 - x0,
                .h = y1 - y0,
            };
            if (SDL_RenderFillRect(renderer, &pixel_rect)) {
                return false;
            }
        }
    }

    return true;
}
