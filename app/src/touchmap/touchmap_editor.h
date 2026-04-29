#ifndef SC_TOUCHMAP_EDITOR_H
#define SC_TOUCHMAP_EDITOR_H

#include "common.h"

#include <stdbool.h>

#include "coords.h"
#include "touchmap/touchmap.h"

enum sc_touchmap_editor_target {
    SC_TOUCHMAP_EDITOR_TARGET_NONE,
    SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER,
    SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS,
    SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER,
    SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS,
};

enum sc_touchmap_editor_mode {
    SC_TOUCHMAP_EDITOR_MODE_SELECT,
    SC_TOUCHMAP_EDITOR_MODE_ADD_MENU,
    SC_TOUCHMAP_EDITOR_MODE_PLACE_BUTTON,
    SC_TOUCHMAP_EDITOR_MODE_PLACE_SKILL,
    SC_TOUCHMAP_EDITOR_MODE_PLACE_WALK,
};

struct sc_touchmap_editor_selection {
    enum sc_touchmap_editor_target target;
    int button_index;
};

struct sc_touchmap_editor {
    enum sc_touchmap_editor_mode mode;
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

void
sc_touchmap_editor_set_mode(struct sc_touchmap_editor *editor,
                            enum sc_touchmap_editor_mode mode);

enum sc_touchmap_editor_mode
sc_touchmap_editor_get_mode(const struct sc_touchmap_editor *editor);

void
sc_touchmap_editor_clear_selection(struct sc_touchmap_editor *editor);

void
sc_touchmap_editor_select_walk(struct sc_touchmap_editor *editor);

void
sc_touchmap_editor_select_button(struct sc_touchmap_editor *editor,
                                 int button_index);

void
sc_touchmap_editor_select_after_button_remove(
    struct sc_touchmap_editor *editor,
    const struct sc_gptm_gamepad_touchmap *map,
    int button_index);

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

bool
sc_touchmap_editor_delete_selected(struct sc_touchmap_editor *editor,
                                 struct sc_gptm_gamepad_touchmap **map);

#endif
