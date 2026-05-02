# Quick Reference: Gamepad Touchmap

## Quick Start

```bash
# Build scrcpy with overlay support
cd /home/lucd/work/scrcpy
meson setup build
cd build
ninja

# Run with touchmap
./app/scrcpy -G --gamepad-touchmap /path/to/touchmap.json

# Or enable automatic touchmap selection by foreground app package
./app/scrcpy -G --gamepad-touchmap-dir /path/to/touchmaps
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Shortcut modifier + `T` | Open file dialog to load a touchmap |
| Shortcut modifier + `Shift+T` | Unload the current touchmap |
| Shortcut modifier + `E` | Toggle overlay visibility |
| `Ctrl+S` | Save to the current touchmap file |
| `Ctrl+Shift+S` | Save As |

The shortcut modifier is scrcpy's configured shortcut modifier. The save
shortcuts are currently hard-coded to Ctrl.

## Auto-Load Workflow

- `--gamepad-touchmap-dir` enables automatic package-based touchmap selection.
- Each participating JSON file must include top-level `packageName`.
- When the foreground app changes, scrcpy resolves `packageName -> touchmap`
  from that directory and loads the matching file.
- If no file matches the current foreground app, the current auto-loaded
  touchmap is unloaded.
- If `--gamepad-touchmap` is also specified, that explicit file wins and
  automatic switching is disabled for the session.
- `Ctrl+T` during auto mode loads a manual override.
- `Ctrl+Shift+T` clears the manual override and reapplies the current
  foreground-app mapping.
- `NEW` in auto mode seeds the new in-memory map with the current foreground
  app `packageName` when known.
- Save As inside the auto-load directory rebuilds the loader index. If the
  saved file becomes the indexed match for the current foreground app, it
  becomes the active auto-loaded map.

## Overlay Colors

| Color | Meaning |
|-------|---------|
| White translucent circle | Walk control, driven by the left stick |
| Green circle | Regular button mapping |
| Blue circle | Skill mapping |
| Red circle | Button/skill exists but has no gamepad binding |
| Yellow rings | Currently selected edit target |

## What You See

- Semi-transparent circles are touch zones from your gamepad.
- Circle outlines are zone boundaries.
- Dashed outlines show skill aiming radii in edit mode.
- Solid filled dots show currently active/pressed touches.
- Labels show gamepad inputs such as `A`, `RB`, `LT`, arrows, Guide, and
  Touchpad.

## Overlay Editing

1. Toggle the overlay with shortcut modifier + `E`.
2. If no touchmap is loaded, click `NEW` to create an empty in-memory map.
3. Click `EDIT` (top-right button) to enter edit mode.
4. In edit mode, a toolbar appears with these buttons:
   - `ADD`: opens a dropdown with `BUTTON`, `SKILL`, or `WALK` items.
     Select an item to add that control at the center of the screen.
   - `DEL`: delete the currently selected control (Walk, Button, or Skill).
   - `QUIT`: leave edit mode, with a save prompt if there are unsaved changes.
5. Select a button or skill control, then press a gamepad button or trigger
   to bind it. There is no separate Bind button.
6. Save with `Ctrl+S` or `Ctrl+Shift+S`.

Editing gestures:

- Drag a control center to move it.
- Drag a Walk or skill radius edge to resize it.
- Arrow keys nudge the selected control by one frame pixel.
- `Shift` + arrow keys nudge by ten frame pixels.
- `Esc` cancels pending add placement.

Important save rule:

- Red button/skill controls are unbound.
- Saving is blocked while any button/skill is unbound.

## Example Touchmap.json

```json
{
  "mappings": {
    "walk_control": {
      "center": {"x": 200, "y": 800},
      "radius": 150
    },
    "button_mappings": [
      {"touch": {"x": 900, "y": 400}, "button": "A"},
      {"touch": {"x": 1000, "y": 400}, "button": "B"},
      {"touch": {"x": 900, "y": 500}, "button": "X"},
      {"touch": {"x": 1000, "y": 500}, "button": "Y"}
    ],
    "skill_casting": [
      {"center": {"x": 500, "y": 500}, "radius": 100, "button": "LB"},
      {"center": {"x": 1300, "y": 500}, "radius": 100, "button": "RB"}
    ]
  }
}
```

Supported button names include:

- `A`, `B`, `X`, `Y`
- `BACK`, `SELECT`, `GUIDE`, `HOME`, `START`
- `LTHUMB`, `L3`, `RTHUMB`, `R3`
- `LB`, `L1`, `RB`, `R1`, `LT`, `L2`, `RT`, `R2`
- `UP`, `DOWN`, `LEFT`, `RIGHT`
- `MISC`, `PADDLE1`, `PADDLE2`, `PADDLE3`, `PADDLE4`, `TOUCHPAD`

Note: `SELECT`, `HOME`, `L3`, `R3`, `L1`, `L2`, `R1`, `R2` are accepted as
input aliases but are normalized on save to `BACK`, `GUIDE`, `LTHUMB`,
`RTHUMB`, `LB`, `LT`, `RB`, `RT` respectively.

## File Locations

- **Runtime model and JSON**: `app/src/touchmap/touchmap.{c,h}`
- **State and dialog handling**: `app/src/touchmap/touchmap_state.{c,h}`
- **Runtime gamepad-to-touch**: `app/src/touchmap/touchmap_runtime.{c,h}`
- **Editor logic**: `app/src/touchmap/touchmap_editor.{c,h}`
- **Auto-loader (package-indexed)**: `app/src/touchmap/touchmap_loader.{c,h}`
- **Foreground-app detection**: `app/src/fg_app_detect.{c,h}`
- **Overlay UI (rendering, toolbar, menus)**: `app/src/touchmap/ui_touchmap_layer.{c,h}`
- **UI layer registration and coordinate transforms**: `app/src/ui/ui_context.{c,h}`
- **Shared circle drawing**: `app/src/ui/ui_draw.{c,h}`
- **Circle widget**: `app/src/ui/ui_widget_circle_button.{c,h}`
- **Screen integration**: `app/src/screen.{c,h}`
- **Input integration and shortcuts**: `app/src/input_manager.{c,h}`
- **Display integration**: `app/src/display.{c,h}`
- **Maintained design doc**: `TOUCHMAP.md`

## Troubleshooting

**Overlay not showing?**
- Make sure a touchmap is loaded with shortcut modifier + `T`, or toggle the
  overlay and click `NEW`
- Press shortcut modifier + `E` to toggle overlay on
- Check console for errors

**Overlay showing but no circles?**
- Touchmap coordinates might be outside screen bounds
- The map may be empty; click `EDIT`, then use `ADD`

**Save fails?**
- Bind every red button/skill control first
- Walk does not need a binding

**Gamepad input touches the game while editing?**
- Edit mode suppresses gamepad-to-touch output
- Mouse input is consumed by the editor while edit mode is active

## Development Notes

### How It Works
1. SDL draws the device screen texture via `sc_display_render()`
2. `sc_ui_context_render()` iterates registered UI layers, including the
   touchmap overlay which draws semi-transparent circles on top
3. Input manager routes gamepad events to `sc_touchmap_runtime_handle_event()`,
   which translates state into virtual Android touch events
4. Editor mode mutates the in-memory touchmap and marks it dirty; it disables
   both gamepad-to-touch output and mouse injection to the device
5. Save rebuilds known JSON sections while preserving unknown metadata

### Key Functions
- `parse_touchmap_config()` - Load touchmap JSON from file
- `save_touchmap_config()` - Save touchmap JSON to file, preserving metadata
- `sc_touchmap_runtime_simulate_touch()` - Inject a virtual touch event
- `sc_touchmap_runtime_handle_event()` - Handle gamepad axis/button event
- `sc_touchmap_state_load_file()` - Load touchmap file into state
- `sc_touchmap_state_load_auto_file()` - Load touchmap via auto-loader
- `sc_touchmap_state_load_manual_file()` - Load touchmap as manual override
- `sc_touchmap_state_save()` - Save touchmap to file (normal or Save As)
- `sc_touchmap_state_enter_edit_mode()` - Enter edit mode (releases touches)
- `sc_touchmap_state_add_button_at_center()` - Add button/skill at screen center
- `sc_touchmap_state_add_walk_at_center()` - Add walk control at screen center
- `sc_touchmap_state_on_foreground_app_changed()` - Handle FG app auto-switch
- `sc_touchmap_state_maybe_apply_deferred_switch()` - Apply deferred auto-switch
- `sc_touchmap_state_reload_file()` - Discard edits, reload from disk
- `sc_touchmap_state_create_empty()` - Create empty in-memory touchmap
- `sc_ui_touchmap_layer_render()` - Main overlay drawing function
- `sc_ui_touchmap_layer_handle_event()` - Toolbar and menu event handling
- `sc_touchmap_editor_try_start_drag()` - Edit-mode hit testing
- `sc_touchmap_editor_apply_drag()` - Apply drag movement in edit mode
- `sc_touchmap_editor_nudge_selection()` - Nudge selected control (keyboard)
- `sc_gptm_gamepad_touchmap_add_button()` - Append button/skill mapping
- `sc_gptm_gamepad_touchmap_remove_button()` - Remove button/skill mapping
- `sc_gptm_gamepad_touchmap_bind_button()` - Bind or rebind selected mapping
- `sc_gptm_gamepad_touchmap_set_walk()` - Set walk control
- `sc_gptm_gamepad_touchmap_remove_walk()` - Remove walk control
- `sc_ui_widget_circle_button_render()` - Render a circle widget
- `sc_ui_draw_fill_circle()` - Draw a filled circle
- `sc_ui_draw_circle_outline()` - Draw a circle outline (solid or dashed)
- `sc_touchmap_loader_find_path()` - Find touchmap path for package name
- `sc_touchmap_loader_rebuild_index()` - Rebuild package-to-path index
- `sc_touchmap_read_package_name()` - Read packageName from JSON file
- `sc_touchmap_build_default_filename()` - Build `<package>.json` filename

### Architecture
```
Gamepad input → input_manager → touchmap_runtime → virtual Android touch events
          ↓
    touchmap_state
          ↓
    ui_touchmap_layer (registered in ui_context) → overlay rendering
          ↓
    touchmap_editor → edit-mode drag/select/nudge
          ↓
    touchmap_loader → auto package-based selection
```

---

Start scrcpy with a touchmap, toggle the overlay, and use `EDIT` to adjust or
build mappings.
