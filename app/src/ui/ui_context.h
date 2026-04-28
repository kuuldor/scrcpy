#ifndef SC_UI_CONTEXT_H
#define SC_UI_CONTEXT_H

#include "common.h"

#include <stdbool.h>
#include <stddef.h>

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

#include "coords.h"
#include "ui_event.h"
#include "ui_geom.h"
#include "ui_layer.h"
#include "ui_metrics.h"
#include "ui_render.h"
#include "ui_types.h"

struct sc_ui_context_params {
    SDL_Window *window;
};

struct sc_ui_registered_layer {
    struct sc_ui_layer *layer;
    int z_index;
};

struct sc_ui_context {
    SDL_Window *window;

    bool enabled;
    bool needs_refresh;

    struct sc_ui_geometry geometry;
    struct sc_ui_metrics metrics;

    sc_ui_id hover_id;
    sc_ui_id active_id;
    sc_ui_id focus_id;
    struct sc_ui_layer *capture_layer;

    struct sc_ui_registered_layer *layers;
    size_t layer_count;
};

bool
sc_ui_context_init(struct sc_ui_context *ui,
                   const struct sc_ui_context_params *params);

void
sc_ui_context_destroy(struct sc_ui_context *ui);

void
sc_ui_context_set_geometry(struct sc_ui_context *ui,
                           const struct sc_ui_geometry *geometry);

const struct sc_ui_geometry *
sc_ui_context_get_geometry(const struct sc_ui_context *ui);

const struct sc_ui_metrics *
sc_ui_context_get_metrics(const struct sc_ui_context *ui);

void
sc_ui_context_set_enabled(struct sc_ui_context *ui, bool enabled);

bool
sc_ui_context_is_enabled(const struct sc_ui_context *ui);

bool
sc_ui_context_add_layer(struct sc_ui_context *ui, struct sc_ui_layer *layer,
                        int z_index);

void
sc_ui_context_remove_layer(struct sc_ui_context *ui,
                           struct sc_ui_layer *layer);

void
sc_ui_context_clear_layers(struct sc_ui_context *ui);

struct sc_ui_input_result
sc_ui_context_handle_event(struct sc_ui_context *ui,
                           const struct sc_ui_event *event);

bool
sc_ui_context_render(struct sc_ui_context *ui, SDL_Renderer *renderer);

void
sc_ui_context_request_refresh(struct sc_ui_context *ui);

bool
sc_ui_context_needs_refresh(const struct sc_ui_context *ui);

void
sc_ui_context_clear_refresh(struct sc_ui_context *ui);

void
sc_ui_context_cancel_interaction(struct sc_ui_context *ui);

void
sc_ui_context_release_pointer_capture(struct sc_ui_context *ui);

void
sc_ui_context_capture_pointer(struct sc_ui_context *ui,
                              struct sc_ui_layer *layer);

void
sc_ui_context_release_focus(struct sc_ui_context *ui);

sc_ui_id
sc_ui_context_get_hover_id(const struct sc_ui_context *ui);

sc_ui_id
sc_ui_context_get_active_id(const struct sc_ui_context *ui);

sc_ui_id
sc_ui_context_get_focus_id(const struct sc_ui_context *ui);

void
sc_ui_context_set_hover_id(struct sc_ui_context *ui, sc_ui_id id);

void
sc_ui_context_set_active_id(struct sc_ui_context *ui, sc_ui_id id);

struct sc_ui_layer *
sc_ui_context_get_capture_layer(const struct sc_ui_context *ui);

bool
sc_ui_context_drawable_to_frame_point(const struct sc_ui_context *ui,
                                      struct sc_point drawable,
                                      struct sc_point *out);

bool
sc_ui_context_drawable_to_logical_point(const struct sc_ui_context *ui,
                                        struct sc_point drawable,
                                        struct sc_point *out);

bool
sc_ui_context_logical_to_drawable_point(const struct sc_ui_context *ui,
                                        struct sc_point logical,
                                        struct sc_point *out);

bool
sc_ui_context_logical_to_drawable_rect(const struct sc_ui_context *ui,
                                       const SDL_Rect *logical,
                                       SDL_Rect *out);

int32_t
sc_ui_context_logical_to_drawable_length(const struct sc_ui_context *ui,
                                         int32_t value);

bool
sc_ui_context_frame_to_drawable_point(const struct sc_ui_context *ui,
                                      struct sc_point frame,
                                      struct sc_point *out);

int32_t
sc_ui_context_frame_to_drawable_radius(const struct sc_ui_context *ui,
                                       int32_t radius);

#endif
