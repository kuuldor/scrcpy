#ifndef SC_UI_RENDER_H
#define SC_UI_RENDER_H

#include "common.h"

#include <SDL2/SDL_render.h>

#include "ui_geom.h"
#include "ui_metrics.h"

struct sc_ui_context;

struct sc_ui_render_ctx {
    SDL_Renderer *renderer;
    const struct sc_ui_context *ui;
    const struct sc_ui_geometry *geometry;
    const struct sc_ui_metrics *metrics;
};

#endif
