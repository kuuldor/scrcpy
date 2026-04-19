#include "touchmap_editor.h"

#include <math.h>
#include <stdlib.h>

static void
sc_touchmap_editor_selection_reset(struct sc_touchmap_editor_selection *selection) {
    selection->target = SC_TOUCHMAP_EDITOR_TARGET_NONE;
    selection->button_index = -1;
}

void
sc_touchmap_editor_init(struct sc_touchmap_editor *editor) {
    editor->dragging = false;
    sc_touchmap_editor_selection_reset(&editor->drag);
    sc_touchmap_editor_selection_reset(&editor->selection);
}

void
sc_touchmap_editor_reset(struct sc_touchmap_editor *editor) {
    sc_touchmap_editor_init(editor);
}

void
sc_touchmap_editor_reset_drag(struct sc_touchmap_editor *editor) {
    editor->dragging = false;
    sc_touchmap_editor_selection_reset(&editor->drag);
}

bool
sc_touchmap_editor_is_dragging(const struct sc_touchmap_editor *editor) {
    return editor->dragging;
}

static void
sc_touchmap_editor_start_drag(struct sc_touchmap_editor *editor,
                              enum sc_touchmap_editor_target target,
                              int button_index) {
    editor->dragging = true;
    editor->drag.target = target;
    editor->drag.button_index = button_index;
    editor->selection = editor->drag;
}

static bool
sc_touchmap_editor_is_skill_button(const struct sc_gptm_touch_button *btn) {
    return btn->is_skill;
}

static bool
sc_touchmap_editor_hit_test_radius(const struct sc_point *center,
                                   int32_t radius, struct sc_point point,
                                   int32_t threshold) {
    int32_t dx = point.x - center->x;
    int32_t dy = point.y - center->y;
    int32_t dist = (int32_t) sqrt((double) dx * dx + (double) dy * dy);
    return abs(dist - radius) <= threshold;
}

static bool
sc_touchmap_editor_hit_test_center(const struct sc_point *center,
                                   int32_t radius, struct sc_point point) {
    int32_t dx = point.x - center->x;
    int32_t dy = point.y - center->y;
    int32_t dist2 = dx * dx + dy * dy;
    return dist2 <= radius * radius;
}

bool
sc_touchmap_editor_try_start_drag(struct sc_touchmap_editor *editor,
                                  const struct sc_gptm_gamepad_touchmap *map,
                                  struct sc_point point) {
    if (!map) {
        return false;
    }

    int32_t min_radius = SC_TOUCHMAP_MIN_RADIUS;
    int32_t radius_threshold = min_radius / 4;

    if (sc_touchmap_editor_hit_test_radius(&map->walk.center,
                                           map->walk.radius, point,
                                           radius_threshold)) {
        sc_touchmap_editor_start_drag(editor,
                                      SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS,
                                      -1);
        return true;
    }

    if (sc_touchmap_editor_hit_test_center(&map->walk.center, min_radius,
                                           point)) {
        sc_touchmap_editor_start_drag(editor,
                                      SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER,
                                      -1);
        return true;
    }

    for (int i = 0; i < map->button_cnt; ++i) {
        const struct sc_gptm_touch_button *btn = &map->buttons[i];
        int32_t radius = btn->radius > 0 ? btn->radius : min_radius;

        if (sc_touchmap_editor_is_skill_button(btn)
                && sc_touchmap_editor_hit_test_radius(&btn->center, radius,
                                                      point,
                                                      radius_threshold)) {
            sc_touchmap_editor_start_drag(editor,
                                          SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS,
                                          i);
            return true;
        }

        if (sc_touchmap_editor_hit_test_center(&btn->center, min_radius,
                                               point)) {
            sc_touchmap_editor_start_drag(editor,
                                          SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER,
                                          i);
            return true;
        }

        if (sc_touchmap_editor_hit_test_radius(&btn->center, radius, point,
                                               radius_threshold)) {
            sc_touchmap_editor_start_drag(editor,
                                          SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS,
                                          i);
            return true;
        }
    }

    return false;
}

static bool
sc_touchmap_editor_apply_center_drag(struct sc_touchmap_editor *editor,
                                     struct sc_gptm_gamepad_touchmap *map,
                                     struct sc_point point) {
    switch (editor->drag.target) {
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER:
            map->walk.center = point;
            map->walk.current_pos = point;
            return true;
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER:
            if (editor->drag.button_index >= 0) {
                struct sc_gptm_touch_button *btn =
                    &map->buttons[editor->drag.button_index];
                btn->center = point;
                btn->current_pos = point;
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

static bool
sc_touchmap_editor_apply_radius_drag(struct sc_touchmap_editor *editor,
                                     struct sc_gptm_gamepad_touchmap *map,
                                     struct sc_point point) {
    int32_t min_radius = SC_TOUCHMAP_MIN_RADIUS;
    switch (editor->drag.target) {
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS: {
            struct sc_point center = map->walk.center;
            int32_t dx = point.x - center.x;
            int32_t dy = point.y - center.y;
            int32_t radius = (int32_t) sqrt((double) dx * dx
                                            + (double) dy * dy);
            map->walk.radius = radius > min_radius ? radius : min_radius;
            return true;
        }
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS:
            if (editor->drag.button_index >= 0) {
                struct sc_gptm_touch_button *btn =
                    &map->buttons[editor->drag.button_index];
                struct sc_point center = btn->center;
                int32_t dx = point.x - center.x;
                int32_t dy = point.y - center.y;
                int32_t radius = (int32_t) sqrt((double) dx * dx
                                                + (double) dy * dy);
                btn->radius = radius > min_radius ? radius : min_radius;
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

bool
sc_touchmap_editor_apply_drag(struct sc_touchmap_editor *editor,
                              struct sc_gptm_gamepad_touchmap *map,
                              struct sc_point point) {
    if (!map || !editor->dragging) {
        return false;
    }

    switch (editor->drag.target) {
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER:
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER:
            return sc_touchmap_editor_apply_center_drag(editor, map, point);
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS:
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS:
            return sc_touchmap_editor_apply_radius_drag(editor, map, point);
        default:
            return false;
    }
}

bool
sc_touchmap_editor_nudge_selection(struct sc_touchmap_editor *editor,
                                   struct sc_gptm_gamepad_touchmap *map,
                                   int32_t dx, int32_t dy,
                                   int32_t radius_delta) {
    if (!map || !editor) {
        return false;
    }

    struct sc_touchmap_editor_selection selection = editor->selection;
    switch (selection.target) {
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER:
            if (!dx && !dy) {
                return false;
            }
            map->walk.center.x += dx;
            map->walk.center.y += dy;
            map->walk.current_pos = map->walk.center;
            return true;
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER: {
            if ((!dx && !dy) || selection.button_index < 0
                    || selection.button_index >= map->button_cnt) {
                return false;
            }
            struct sc_gptm_touch_button *btn =
                &map->buttons[selection.button_index];
            btn->center.x += dx;
            btn->center.y += dy;
            btn->current_pos = btn->center;
            return true;
        }
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS: {
            if (!radius_delta) {
                return false;
            }
            int32_t radius = map->walk.radius + radius_delta;
            if (radius < SC_TOUCHMAP_MIN_RADIUS) {
                radius = SC_TOUCHMAP_MIN_RADIUS;
            }
            if (radius == map->walk.radius) {
                return false;
            }
            map->walk.radius = radius;
            return true;
        }
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS: {
            if (!radius_delta || selection.button_index < 0
                    || selection.button_index >= map->button_cnt) {
                return false;
            }
            struct sc_gptm_touch_button *btn =
                &map->buttons[selection.button_index];
            if (!btn->is_skill) {
                return false;
            }
            int32_t radius = btn->radius + radius_delta;
            if (radius < SC_TOUCHMAP_MIN_RADIUS) {
                radius = SC_TOUCHMAP_MIN_RADIUS;
            }
            if (radius == btn->radius) {
                return false;
            }
            btn->radius = radius;
            return true;
        }
        default:
            return false;
    }
}
