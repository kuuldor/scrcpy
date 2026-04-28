#ifndef SC_UI_WIDGET_BUTTON_H
#define SC_UI_WIDGET_BUTTON_H

#include "common.h"

#include <SDL2/SDL_rect.h>

#include "ui_button.h"
#include "ui_draw.h"
#include "ui_text.h"

struct sc_ui_widget_button_style {
    struct sc_ui_color fill_color;
    struct sc_ui_color fill_hover_color;
    struct sc_ui_color fill_pressed_color;
    struct sc_ui_color fill_disabled_color;
    struct sc_ui_color border_color;
    struct sc_ui_color border_pressed_color;
    struct sc_ui_color border_disabled_color;
    struct sc_ui_text_style text_style;
    struct sc_ui_color text_disabled_color;
    int padding_h;
    int padding_v;
};

enum sc_ui_widget_button_variant {
    SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
    SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS,
    SC_UI_WIDGET_BUTTON_VARIANT_DANGER,
};

struct sc_ui_widget_button {
    struct sc_ui_button_state state;
    SDL_Rect rect;
    const char *label;
    bool enabled;
    struct sc_ui_widget_button_style style;
};

struct sc_ui_widget_button_style
sc_ui_widget_button_style_default(void);

struct sc_ui_widget_button_style
sc_ui_widget_button_style_variant(enum sc_ui_widget_button_variant variant);

void
sc_ui_widget_button_init(struct sc_ui_widget_button *button, sc_ui_id id,
                         const SDL_Rect *rect, const char *label,
                         const struct sc_ui_widget_button_style *style);

void
sc_ui_widget_button_init_default(struct sc_ui_widget_button *button,
                                 sc_ui_id id, const char *label);

void
sc_ui_widget_button_fit_to_content(struct sc_ui_widget_button *button);

void
sc_ui_widget_button_set_label(struct sc_ui_widget_button *button,
                              const char *label);

void
sc_ui_widget_button_set_label_and_fit(struct sc_ui_widget_button *button,
                                      const char *label);

void
sc_ui_widget_button_apply_variant(struct sc_ui_widget_button *button,
                                  enum sc_ui_widget_button_variant variant);

int32_t
sc_ui_widget_button_max_width(const struct sc_ui_widget_button *buttons,
                              size_t count);

int32_t
sc_ui_widget_button_max_height(const struct sc_ui_widget_button *buttons,
                               size_t count);

void
sc_ui_widget_button_set_size(struct sc_ui_widget_button *button,
                             int32_t width, int32_t height);

void
sc_ui_widget_button_place_top_right(struct sc_ui_widget_button *button,
                                    struct sc_size bounds, int margin);

void
sc_ui_widget_button_reset(struct sc_ui_widget_button *button,
                          struct sc_ui_context *ui,
                          struct sc_ui_layer *layer);

SDL_Rect
sc_ui_widget_button_get_content_rect(const struct sc_ui_widget_button *button);

int32_t
sc_ui_widget_button_width_for_label(const char *label,
                                    const struct sc_ui_widget_button_style *style);

int32_t
sc_ui_widget_button_height_for_style(
    const struct sc_ui_widget_button_style *style);

struct sc_ui_button_result
sc_ui_widget_button_handle_event(struct sc_ui_widget_button *button,
                                 struct sc_ui_context *ui,
                                 struct sc_ui_layer *layer,
                                 const struct sc_ui_event *event);

bool
sc_ui_widget_button_render(const struct sc_ui_widget_button *button,
                           const struct sc_ui_render_ctx *render_ctx);

#endif
