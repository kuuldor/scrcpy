#include "ui_widget_action_menu.h"

#include <assert.h>

static int32_t
sc_ui_widget_action_menu_max_width(struct sc_ui_widget_button **buttons,
                                   size_t count) {
    int32_t width = 0;
    for (size_t i = 0; i < count; ++i) {
        struct sc_ui_widget_button *button = buttons[i];
        int32_t item_width = sc_ui_widget_button_width_for_label(button->label,
                                                                 &button->style);
        if (item_width > width) {
            width = item_width;
        }
    }

    return width;
}

static int32_t
sc_ui_widget_action_menu_max_height(struct sc_ui_widget_button **buttons,
                                    size_t count) {
    int32_t height = 0;
    for (size_t i = 0; i < count; ++i) {
        struct sc_ui_widget_button *button = buttons[i];
        int32_t item_height = sc_ui_widget_button_height_for_style(&button->style);
        if (item_height > height) {
            height = item_height;
        }
    }

    return height;
}

void
sc_ui_widget_action_menu_init(struct sc_ui_widget_action_menu *action_menu,
                              struct sc_ui_widget_button **toolbar_buttons,
                              size_t toolbar_button_count,
                              const sc_ui_widget_action_handler *toolbar_actions,
                              struct sc_ui_widget_button **menu_buttons,
                              size_t menu_button_count,
                              const sc_ui_widget_action_handler *menu_actions,
                              void *userdata) {
    sc_ui_widget_panel_init_default(&action_menu->toolbar_panel);
    sc_ui_widget_menu_init_default(&action_menu->menu);
    assert(toolbar_button_count <= SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS);
    assert(menu_button_count <= SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS);
    action_menu->toolbar_button_count = toolbar_button_count;
    action_menu->menu_button_count = menu_button_count;
    for (size_t i = 0; i < toolbar_button_count; ++i) {
        action_menu->toolbar_buttons[i] = toolbar_buttons[i];
        action_menu->toolbar_actions[i] = toolbar_actions[i];
    }
    for (size_t i = 0; i < menu_button_count; ++i) {
        action_menu->menu_buttons[i] = menu_buttons[i];
        action_menu->menu_actions[i] = menu_actions[i];
        action_menu->menu_button_slots[i] = (int) i;
    }
    action_menu->userdata = userdata;
    action_menu->menu_layout_count = menu_button_count;
    action_menu->margin = 8;
    action_menu->padding = 6;
    action_menu->gap = 6;
    action_menu->menu_gap = 6;
}

void
sc_ui_widget_action_menu_layout_top_right(
    struct sc_ui_widget_action_menu *action_menu, struct sc_size bounds) {
    int32_t button_width = sc_ui_widget_action_menu_max_width(
        action_menu->toolbar_buttons, action_menu->toolbar_button_count);
    int32_t menu_width = sc_ui_widget_action_menu_max_width(
        action_menu->menu_buttons, action_menu->menu_button_count);
    if (menu_width > button_width) {
        button_width = menu_width;
    }

    int32_t button_height = sc_ui_widget_action_menu_max_height(
        action_menu->toolbar_buttons, action_menu->toolbar_button_count);
    int32_t menu_height = sc_ui_widget_action_menu_max_height(
        action_menu->menu_buttons, action_menu->menu_button_count);
    if (menu_height > button_height) {
        button_height = menu_height;
    }

    action_menu->toolbar_panel.style.padding = action_menu->padding;
    action_menu->toolbar_panel.rect.w =
        (int32_t) action_menu->toolbar_button_count * button_width
      + ((int32_t) action_menu->toolbar_button_count - 1) * action_menu->gap
      + 2 * action_menu->padding;
    action_menu->toolbar_panel.rect.h = button_height + 2 * action_menu->padding;
    sc_ui_widget_panel_place_top_right(&action_menu->toolbar_panel, bounds,
                                       action_menu->margin);

    for (size_t i = 0; i < action_menu->toolbar_button_count; ++i) {
        struct sc_ui_widget_button *button = action_menu->toolbar_buttons[i];
        sc_ui_widget_button_set_size(button, button_width, button_height);
        button->rect.x = action_menu->toolbar_panel.rect.x + action_menu->padding
                       + (int32_t) i * (button_width + action_menu->gap);
        button->rect.y = action_menu->toolbar_panel.rect.y + action_menu->padding;
    }

    action_menu->menu.panel.style.padding = action_menu->padding;
    action_menu->menu.style.panel.padding = action_menu->padding;
    action_menu->menu.style.item_width = button_width;
    action_menu->menu.style.item_height = button_height;
    action_menu->menu.style.item_gap = action_menu->menu_gap;
    sc_ui_widget_menu_fit_panel(&action_menu->menu,
                                (int) action_menu->menu_layout_count);
    sc_ui_widget_menu_place_below(&action_menu->menu,
                                  &action_menu->toolbar_panel.rect,
                                  action_menu->menu_gap);

    for (size_t i = 0; i < action_menu->menu_button_count; ++i) {
        struct sc_ui_widget_button *button = action_menu->menu_buttons[i];
        sc_ui_widget_button_set_size(button, button_width, button_height);
        button->rect = sc_ui_widget_menu_get_item_rect(&action_menu->menu,
                                                       action_menu->menu_button_slots[i]);
    }
}

void
sc_ui_widget_action_menu_set_spacing(struct sc_ui_widget_action_menu *action_menu,
                                     int margin, int padding, int gap,
                                     int menu_gap) {
    action_menu->margin = margin;
    action_menu->padding = padding;
    action_menu->gap = gap;
    action_menu->menu_gap = menu_gap;
}

void
sc_ui_widget_action_menu_apply_variants(
    struct sc_ui_widget_action_menu *action_menu,
    const enum sc_ui_widget_button_variant *toolbar_variants,
    const enum sc_ui_widget_button_variant *menu_variants) {
    for (size_t i = 0; i < action_menu->toolbar_button_count; ++i) {
        sc_ui_widget_button_apply_variant(action_menu->toolbar_buttons[i],
                                          toolbar_variants[i]);
    }
    for (size_t i = 0; i < action_menu->menu_button_count; ++i) {
        sc_ui_widget_button_apply_variant(action_menu->menu_buttons[i],
                                          menu_variants[i]);
    }
}

void
sc_ui_widget_action_menu_set_menu_button_enabled(
    struct sc_ui_widget_action_menu *action_menu, size_t index, bool enabled) {
    assert(index < action_menu->menu_button_count);
    action_menu->menu_buttons[index]->enabled = enabled;
}

void
sc_ui_widget_action_menu_set_menu_layout_count(
    struct sc_ui_widget_action_menu *action_menu, size_t count) {
    action_menu->menu_layout_count = count;
}

void
sc_ui_widget_action_menu_set_menu_button_slot(
    struct sc_ui_widget_action_menu *action_menu, size_t index, int slot) {
    assert(index < action_menu->menu_button_count);
    action_menu->menu_button_slots[index] = slot;
}

bool
sc_ui_widget_action_menu_render(
    const struct sc_ui_widget_action_menu *action_menu,
    const struct sc_ui_render_ctx *render_ctx, bool menu_open) {
    bool ok = sc_ui_widget_panel_render(&action_menu->toolbar_panel, render_ctx);
    for (size_t i = 0; i < action_menu->toolbar_button_count; ++i) {
        ok &= sc_ui_widget_button_render(action_menu->toolbar_buttons[i],
                                         render_ctx);
    }

    if (!menu_open) {
        return ok;
    }

    ok &= sc_ui_widget_menu_render(&action_menu->menu, render_ctx);
    for (size_t i = 0; i < action_menu->menu_button_count; ++i) {
        ok &= sc_ui_widget_button_render(action_menu->menu_buttons[i],
                                         render_ctx);
    }

    return ok;
}

struct sc_ui_widget_action_menu_result
sc_ui_widget_action_menu_handle_event(
    struct sc_ui_widget_action_menu *action_menu,
    struct sc_ui_context *ui, struct sc_ui_layer *layer,
    const struct sc_ui_event *event, bool menu_open) {
    struct sc_ui_widget_action_menu_result result = {
        .input = {false, false},
        .clicked_outside = false,
    };

    for (size_t i = 0; i < action_menu->toolbar_button_count; ++i) {
        struct sc_ui_button_result button_result =
            sc_ui_widget_button_handle_event(action_menu->toolbar_buttons[i],
                                             ui, layer, event);
        result.input = button_result.input;
        if (button_result.action == SC_UI_BUTTON_ACTION_CLICK) {
            if (action_menu->toolbar_actions[i]) {
                action_menu->toolbar_actions[i](action_menu->userdata,
                                                &result.input);
            }
            return result;
        }
        if (result.input.consumed) {
            return result;
        }
    }

    if (!menu_open) {
        return result;
    }

    for (size_t i = 0; i < action_menu->menu_button_count; ++i) {
        struct sc_ui_widget_button *button = action_menu->menu_buttons[i];
        if (!button->enabled) {
            continue;
        }

        struct sc_ui_button_result button_result =
            sc_ui_widget_button_handle_event(button, ui, layer, event);
        result.input = button_result.input;
        if (button_result.action == SC_UI_BUTTON_ACTION_CLICK) {
            if (action_menu->menu_actions[i]) {
                action_menu->menu_actions[i](action_menu->userdata,
                                             &result.input);
            }
            return result;
        }
        if (result.input.consumed) {
            return result;
        }
    }

    if (event->type == SC_UI_EVENT_POINTER_DOWN) {
        bool in_menu = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                event->data.pointer.y,
                                                &action_menu->menu.panel.rect);
        bool in_toolbar = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                   event->data.pointer.y,
                                                   &action_menu->toolbar_panel.rect);
        if (!in_menu && !in_toolbar) {
            result.clicked_outside = true;
        }
    }

    return result;
}
