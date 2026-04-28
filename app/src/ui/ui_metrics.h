#ifndef SC_UI_METRICS_H
#define SC_UI_METRICS_H

#include "common.h"

#include "ui_geom.h"

struct sc_ui_metrics {
    int32_t scale_milli;
};

static inline struct sc_ui_metrics
sc_ui_metrics_make(const struct sc_ui_geometry *geometry) {
    if (!geometry->has_frame || !geometry->frame_size.width
            || !geometry->frame_size.height || !geometry->content_rect.w
            || !geometry->content_rect.h) {
        return (struct sc_ui_metrics) {
            .scale_milli = 1000,
        };
    }

    int32_t fw = geometry->frame_size.width;
    int32_t fh = geometry->frame_size.height;
    if (sc_orientation_is_swap(geometry->orientation)) {
        int32_t tmp = fw;
        fw = fh;
        fh = tmp;
    }

    int32_t sx = (int64_t) geometry->content_rect.w * 1000 / fw;
    int32_t sy = (int64_t) geometry->content_rect.h * 1000 / fh;
    return (struct sc_ui_metrics) {
        .scale_milli = sx < sy ? sx : sy,
    };
}

static inline int32_t
sc_ui_metrics_scale(const struct sc_ui_metrics *metrics, int32_t value) {
    int32_t scaled = (int64_t) value * metrics->scale_milli / 1000;
    return scaled > 0 ? scaled : 1;
}

static inline int
sc_ui_metrics_text_scale(const struct sc_ui_metrics *metrics, int base_scale) {
    int scale = ((int64_t) base_scale * metrics->scale_milli + 500) / 1000;
    return scale > 0 ? scale : 1;
}

#endif
