#ifndef SC_TOUCHMAP_EDITOR_H
#define SC_TOUCHMAP_EDITOR_H

#include "common.h"

#include <stdbool.h>

#include "coords.h"
#include "touchmap.h"

enum sc_touchmap_editor_target {
    SC_TOUCHMAP_EDITOR_TARGET_NONE,
    SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER,
    SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS,
    SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER,
    SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS,
};

struct sc_touchmap_editor_selection {
    enum sc_touchmap_editor_target target;
    int button_index;
};

struct sc_touchmap_editor {
    bool dragging;
    struct sc_touchmap_editor_selection drag;
    struct sc_touchmap_editor_selection selection;
};

void
sc_touchmap_editor_init(struct sc_touchmap_editor *editor);

void
sc_touchmap_editor_reset(struct sc_touchmap_editor *editor);

void
sc_touchmap_editor_reset_drag(struct sc_touchmap_editor *editor);

bool
sc_touchmap_editor_is_dragging(const struct sc_touchmap_editor *editor);

bool
sc_touchmap_editor_try_start_drag(struct sc_touchmap_editor *editor,
                                  const struct sc_gptm_gamepad_touchmap *map,
                                  struct sc_point point);

bool
sc_touchmap_editor_apply_drag(struct sc_touchmap_editor *editor,
                              struct sc_gptm_gamepad_touchmap *map,
                              struct sc_point point);

bool
sc_touchmap_editor_nudge_selection(struct sc_touchmap_editor *editor,
                                   struct sc_gptm_gamepad_touchmap *map,
                                   int32_t dx, int32_t dy,
                                   int32_t radius_delta);

#endif
