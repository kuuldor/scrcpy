#ifndef SC_UI_WIDGET_LABEL_H
#define SC_UI_WIDGET_LABEL_H

#include "common.h"

#include <SDL2/SDL_rect.h>

#include "ui_draw.h"
#include "ui_text.h"

struct sc_ui_widget_label_style {
    struct sc_ui_text_style text_style;
};

struct sc_ui_widget_label {
    SDL_Rect rect;
    const char *text;
    struct sc_ui_widget_label_style style;
};

void
sc_ui_widget_label_init(struct sc_ui_widget_label *label,
                        const SDL_Rect *rect, const char *text,
                        const struct sc_ui_widget_label_style *style);

int32_t
sc_ui_widget_label_width_for_text(const char *text,
                                  const struct sc_ui_widget_label_style *style);

int32_t
sc_ui_widget_label_height_for_style(
    const struct sc_ui_widget_label_style *style);

bool
sc_ui_widget_label_render(const struct sc_ui_widget_label *label,
                         const struct sc_ui_render_ctx *render_ctx);

#endif