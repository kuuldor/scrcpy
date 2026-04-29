#ifndef SC_UI_WIDGET_ACTION_MENU_H
#define SC_UI_WIDGET_ACTION_MENU_H

#include "common.h"

#include <SDL2/SDL_rect.h>
#include <stddef.h>

#include "coords.h"
#include "ui_widget_action_button.h"
#include "ui_widget_button.h"
#include "ui_widget_menu.h"
#include "ui_widget_panel.h"

#define SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS 8
struct sc_ui_widget_action_menu_result {
    struct sc_ui_input_result input;
    bool clicked_outside;
};

struct sc_ui_widget_action_menu {
    struct sc_ui_widget_panel toolbar_panel;
    struct sc_ui_widget_menu menu;
    struct sc_ui_widget_button *toolbar_buttons[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    sc_ui_widget_action_handler toolbar_actions[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    size_t toolbar_button_count;
    struct sc_ui_widget_button *menu_buttons[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    sc_ui_widget_action_handler menu_actions[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    int menu_button_slots[SC_UI_WIDGET_ACTION_MENU_MAX_BUTTONS];
    size_t menu_button_count;
    size_t menu_layout_count;
    void *userdata;
    int margin;
    int padding;
    int gap;
    int menu_gap;
};

void
sc_ui_widget_action_menu_init(struct sc_ui_widget_action_menu *action_menu,
                              struct sc_ui_widget_button **toolbar_buttons,
                              size_t toolbar_button_count,
                              const sc_ui_widget_action_handler *toolbar_actions,
                              struct sc_ui_widget_button **menu_buttons,
                              size_t menu_button_count,
                              const sc_ui_widget_action_handler *menu_actions,
                              void *userdata);

void
sc_ui_widget_action_menu_layout_top_right(
    struct sc_ui_widget_action_menu *action_menu, struct sc_size bounds);

void
sc_ui_widget_action_menu_set_spacing(struct sc_ui_widget_action_menu *action_menu,
                                     int margin, int padding, int gap,
                                     int menu_gap);

void
sc_ui_widget_action_menu_apply_variants(
    struct sc_ui_widget_action_menu *action_menu,
    const enum sc_ui_widget_button_variant *toolbar_variants,
    const enum sc_ui_widget_button_variant *menu_variants);

void
sc_ui_widget_action_menu_set_menu_button_enabled(
    struct sc_ui_widget_action_menu *action_menu, size_t index, bool enabled);

void
sc_ui_widget_action_menu_set_menu_layout_count(
    struct sc_ui_widget_action_menu *action_menu, size_t count);

void
sc_ui_widget_action_menu_set_menu_button_slot(
    struct sc_ui_widget_action_menu *action_menu, size_t index, int slot);

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
