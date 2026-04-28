#ifndef SC_UI_WIDGET_ACTION_MENU_H
#define SC_UI_WIDGET_ACTION_MENU_H

#include "common.h"

#include <SDL2/SDL_rect.h>
#include <stddef.h>

#include "coords.h"
#include "ui_widget_button.h"
#include "ui_widget_menu.h"
#include "ui_widget_panel.h"

#define SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS 8
#define SC_UI_WIDGET_ACTION_MENU_ACTION_NONE (-1)

struct sc_ui_widget_action_menu_result {
    struct sc_ui_input_result input;
    int action_id;
    bool from_menu;
    bool clicked_outside;
};

struct sc_ui_widget_action_menu {
    struct sc_ui_widget_panel toolbar_panel;
    struct sc_ui_widget_menu menu;
    struct sc_ui_widget_button *toolbar_buttons[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    int toolbar_action_ids[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    size_t toolbar_button_count;
    struct sc_ui_widget_button *menu_buttons[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    int menu_action_ids[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    size_t menu_button_count;
    int margin;
    int padding;
    int gap;
    int menu_gap;
};

void
sc_ui_widget_action_menu_init(struct sc_ui_widget_action_menu *action_menu,
                              struct sc_ui_widget_button **toolbar_buttons,
                              size_t toolbar_button_count,
                              const int *toolbar_action_ids,
                              struct sc_ui_widget_button **menu_buttons,
                              size_t menu_button_count,
                              const int *menu_action_ids);

void
sc_ui_widget_action_menu_layout_top_right(
    struct sc_ui_widget_action_menu *action_menu, struct sc_size bounds);

bool
sc_ui_widget_action_menu_render(
    const struct sc_ui_widget_action_menu *action_menu,
    const struct sc_ui_render_ctx *render_ctx, bool menu_open);

struct sc_ui_widget_action_menu_result
sc_ui_widget_action_menu_handle_event(
    struct sc_ui_widget_action_menu *action_menu,
    struct sc_ui_context *ui, struct sc_ui_layer *layer,
    const struct sc_ui_event *event, bool menu_open);

#endif
