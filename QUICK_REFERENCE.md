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
3. Click `EDIT` to enter edit mode.
4. Use the toolbar:
   - `ADD`: choose `BUTTON`, `SKILL`, or `WALK` to add the control at the
     center of the screen.
   - `DEL`: delete the selected control.
   - `QUIT`: leave edit mode, with a save prompt if there are unsaved changes.
5. Select a button or skill, then press a gamepad button or trigger to bind it.
   There is no separate Bind button.
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

## File Locations

- **Runtime model and JSON**: `/home/lucd/work/scrcpy/app/src/touchmap/touchmap.{c,h}`
- **State and dialog handling**: `/home/lucd/work/scrcpy/app/src/touchmap/touchmap_state.{c,h}`
- **Runtime gamepad-to-touch**: `/home/lucd/work/scrcpy/app/src/touchmap/touchmap_runtime.{c,h}`
- **Editor logic**: `/home/lucd/work/scrcpy/app/src/touchmap/touchmap_editor.{c,h}`
- **Overlay rendering**: `/home/lucd/work/scrcpy/app/src/touchmap/ui_touchmap_layer.{c,h}`
- **Input integration and shortcuts**: `/home/lucd/work/scrcpy/app/src/input_manager.c`
- **Display integration**: `/home/lucd/work/scrcpy/app/src/display.{c,h}`
- **Maintained design doc**: `/home/lucd/work/scrcpy/TOUCHMAP.md`

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
1. SDL draws the device screen texture
2. Overlay module draws semi-transparent circles on top
3. Input manager translates gamepad state into virtual Android touch events
4. Editor mode mutates the in-memory touchmap and marks it dirty
5. Save rebuilds known JSON sections while preserving unknown metadata

### Key Functions
- `parse_touchmap_config()` - Load touchmap JSON
- `save_touchmap_config()` - Save touchmap JSON
- `sc_touchmap_state_load_file()` - Load touchmap file into state
- `sc_touchmap_state_save()` - Save touchmap to file
- `sc_touchmap_runtime_handle_event()` - Handle gamepad axis/button event
- `sc_ui_touchmap_layer_render()` - Main overlay drawing function
- `sc_ui_touchmap_layer_handle_event()` - Toolbar and menu event handling
- `sc_touchmap_editor_try_start_drag()` - Edit-mode hit testing
- `sc_gptm_gamepad_touchmap_add_button()` - Append button/skill mapping
- `sc_gptm_gamepad_touchmap_bind_button()` - Bind or rebind selected mapping

### Architecture
```
Gamepad input → input_manager → touchmap lookup → virtual Android touch events
                         ↓
                  overlay/editor UI
```

---

Start scrcpy with a touchmap, toggle the overlay, and use `EDIT` to adjust or
build mappings.
