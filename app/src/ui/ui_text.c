#include "ui_text.h"

#include <assert.h>

#include <SDL2/SDL.h>

#include "ui_context.h"

#define SC_UI_TEXT_GLYPH_WIDTH 5
#define SC_UI_TEXT_GLYPH_HEIGHT 7

static uint8_t
sc_ui_text_letter_row(unsigned index, int row) {
    static const uint8_t letters[26][SC_UI_TEXT_GLYPH_HEIGHT] = {
        {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
        {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
        {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
        {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
        {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
        {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
        {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
        {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C},
        {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
        {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
        {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
        {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
        {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
        {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
        {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A},
        {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
        {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
    };
    return letters[index][row];
}

static uint8_t
sc_ui_text_digit_row(unsigned index, int row) {
    static const uint8_t digits[10][SC_UI_TEXT_GLYPH_HEIGHT] = {
        {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
        {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
        {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
        {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E},
        {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E},
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
        {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E},
    };
    return digits[index][row];
}

static uint8_t
sc_ui_text_symbol_row(unsigned char c, int row) {
    switch (c) {
        case ' ': return 0x00;
        case '!': return (uint8_t[]) {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}[row];
        case '"': return (uint8_t[]) {0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00}[row];
        case '#': return (uint8_t[]) {0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A}[row];
        case '$': return (uint8_t[]) {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}[row];
        case '%': return (uint8_t[]) {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}[row];
        case '&': return (uint8_t[]) {0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D}[row];
        case '\'': return (uint8_t[]) {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}[row];
        case '(': return (uint8_t[]) {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}[row];
        case ')': return (uint8_t[]) {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}[row];
        case '*': return (uint8_t[]) {0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00}[row];
        case '+': return (uint8_t[]) {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}[row];
        case ',': return (uint8_t[]) {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08}[row];
        case '-': return (uint8_t[]) {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}[row];
        case '.': return (uint8_t[]) {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}[row];
        case '/': return (uint8_t[]) {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}[row];
        case ':': return (uint8_t[]) {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}[row];
        case ';': return (uint8_t[]) {0x00, 0x0C, 0x0C, 0x00, 0x04, 0x04, 0x08}[row];
        case '<': return (uint8_t[]) {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}[row];
        case '=': return (uint8_t[]) {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00}[row];
        case '>': return (uint8_t[]) {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}[row];
        case '?': return (uint8_t[]) {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}[row];
        case '@': return (uint8_t[]) {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E}[row];
        case '[': return (uint8_t[]) {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}[row];
        case '\\': return (uint8_t[]) {0x10, 0x08, 0x08, 0x04, 0x02, 0x02, 0x01}[row];
        case ']': return (uint8_t[]) {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}[row];
        case '^': return (uint8_t[]) {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00}[row];
        case '_': return (uint8_t[]) {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}[row];
        case '`': return (uint8_t[]) {0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}[row];
        case '{': return (uint8_t[]) {0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02}[row];
        case '|': return (uint8_t[]) {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}[row];
        case '}': return (uint8_t[]) {0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08}[row];
        case '~': return (uint8_t[]) {0x00, 0x00, 0x09, 0x16, 0x00, 0x00, 0x00}[row];
        default: return (uint8_t[]) {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}[row];
    }
}

static uint8_t
sc_ui_text_glyph_row(char c, int row) {
    unsigned char ch = (unsigned char) c;
    if (ch < 0x20 || ch > 0x7E) {
        ch = '?';
    }
    if (ch >= 'a' && ch <= 'z') {
        ch = (unsigned char) (ch - 'a' + 'A');
    }
    if (ch >= 'A' && ch <= 'Z') {
        return sc_ui_text_letter_row(ch - 'A', row);
    }
    if (ch >= '0' && ch <= '9') {
        return sc_ui_text_digit_row(ch - '0', row);
    }
    return sc_ui_text_symbol_row(ch, row);
}

static int
sc_ui_text_pixel_size(const struct sc_ui_text_style *style) {
    int scale = style->scale > 0 ? style->scale : 1;
    return scale;
}

struct sc_ui_text_metrics
sc_ui_text_measure(const char *text, const struct sc_ui_text_style *style) {
    struct sc_ui_text_metrics metrics = {0, 0};
    if (!text || !*text) {
        return metrics;
    }

    int pixel = sc_ui_text_pixel_size(style);
    int tracking = style->tracking;
    size_t len = SDL_strlen(text);
    metrics.width = (int) len * SC_UI_TEXT_GLYPH_WIDTH * pixel;
    if (len > 1) {
        metrics.width += ((int) len - 1) * tracking;
    }
    metrics.height = SC_UI_TEXT_GLYPH_HEIGHT * pixel;
    return metrics;
}

bool
sc_ui_text_draw(const struct sc_ui_render_ctx *render_ctx,
                int32_t x, int32_t y, const char *text,
                const struct sc_ui_text_style *style) {
    if (!text || !*text) {
        return true;
    }

    SDL_Renderer *renderer = render_ctx->renderer;
    struct sc_ui_text_style drawable_style = *style;
    struct sc_point drawable_origin = {.x = x, .y = y};
    if (render_ctx->ui) {
        sc_ui_context_logical_to_drawable_point(render_ctx->ui,
                                                (struct sc_point) {.x = x, .y = y},
                                                &drawable_origin);
        drawable_style.scale = sc_ui_context_logical_to_drawable_length(
            render_ctx->ui, style->scale > 0 ? style->scale : 1);
        drawable_style.tracking = sc_ui_context_logical_to_drawable_length(
            render_ctx->ui, style->tracking);
        if (!style->tracking) {
            drawable_style.tracking = 0;
        }
    }

    int pixel = sc_ui_text_pixel_size(&drawable_style);
    int tracking = drawable_style.tracking;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (SDL_SetRenderDrawColor(renderer, drawable_style.color.r,
                               drawable_style.color.g,
                               drawable_style.color.b,
                               drawable_style.color.a)) {
        return false;
    }

    size_t len = SDL_strlen(text);
    for (size_t i = 0; i < len; ++i) {
        int glyph_x = drawable_origin.x
                    + (int) i * (SC_UI_TEXT_GLYPH_WIDTH * pixel + tracking);
        for (int row = 0; row < SC_UI_TEXT_GLYPH_HEIGHT; ++row) {
            uint8_t bits = sc_ui_text_glyph_row(text[i], row);
            for (int col = 0; col < SC_UI_TEXT_GLYPH_WIDTH; ++col) {
                if (!(bits & (1u << (SC_UI_TEXT_GLYPH_WIDTH - 1 - col)))) {
                    continue;
                }

                SDL_Rect rect = {
                    .x = glyph_x + col * pixel,
                    .y = drawable_origin.y + row * pixel,
                    .w = pixel,
                    .h = pixel,
                };
                if (SDL_RenderFillRect(renderer, &rect)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool
sc_ui_text_draw_in_rect(const struct sc_ui_render_ctx *render_ctx,
                        const SDL_Rect *rect, const char *text,
                        const struct sc_ui_text_style *style,
                        enum sc_ui_text_align align) {
    SDL_Rect drawable_rect = *rect;
    struct sc_ui_text_style drawable_style = *style;
    if (render_ctx->ui) {
        sc_ui_context_logical_to_drawable_rect(render_ctx->ui, rect,
                                               &drawable_rect);
        drawable_style.scale = sc_ui_context_logical_to_drawable_length(
            render_ctx->ui, style->scale > 0 ? style->scale : 1);
        drawable_style.tracking = sc_ui_context_logical_to_drawable_length(
            render_ctx->ui, style->tracking);
        if (!style->tracking) {
            drawable_style.tracking = 0;
        }
    }

    struct sc_ui_text_metrics metrics = sc_ui_text_measure(text, &drawable_style);
    int x = drawable_rect.x;
    switch (align) {
        case SC_UI_TEXT_ALIGN_LEFT:
            break;
        case SC_UI_TEXT_ALIGN_CENTER:
            x += (drawable_rect.w - metrics.width) / 2;
            break;
        case SC_UI_TEXT_ALIGN_RIGHT:
            x += drawable_rect.w - metrics.width;
            break;
        default:
            assert(false);
            break;
    }

    int y = drawable_rect.y + (drawable_rect.h - metrics.height) / 2;

    if (!text || !*text) {
        return true;
    }

    SDL_Renderer *renderer = render_ctx->renderer;
    int pixel = sc_ui_text_pixel_size(&drawable_style);
    int tracking = drawable_style.tracking;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (SDL_SetRenderDrawColor(renderer, drawable_style.color.r,
                               drawable_style.color.g,
                               drawable_style.color.b,
                               drawable_style.color.a)) {
        return false;
    }

    size_t len = SDL_strlen(text);
    for (size_t i = 0; i < len; ++i) {
        int glyph_x = x + (int) i * (SC_UI_TEXT_GLYPH_WIDTH * pixel + tracking);
        for (int row = 0; row < SC_UI_TEXT_GLYPH_HEIGHT; ++row) {
            uint8_t bits = sc_ui_text_glyph_row(text[i], row);
            for (int col = 0; col < SC_UI_TEXT_GLYPH_WIDTH; ++col) {
                if (!(bits & (1u << (SC_UI_TEXT_GLYPH_WIDTH - 1 - col)))) {
                    continue;
                }

                SDL_Rect pixel_rect = {
                    .x = glyph_x + col * pixel,
                    .y = y + row * pixel,
                    .w = pixel,
                    .h = pixel,
                };
                if (SDL_RenderFillRect(renderer, &pixel_rect)) {
                    return false;
                }
            }
        }
    }

    return true;
}
