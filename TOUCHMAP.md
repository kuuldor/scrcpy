# Touchmap Design and Implementation

This document is the maintained reference for the local gamepad-to-touch
mapping feature. It consolidates the useful parts of `TOUCHMAP_OVERLAY.md`,
`VISUAL_GUIDE.md`, and `IMPLEMENTATION_SUMMARY.md`, and corrects details that
have changed in the current code.

## Goal

The touchmap feature lets scrcpy translate local gamepad input into Android
touch events. This is intended for games where the device UI expects virtual
touch controls, for example MOBA movement, skill aiming, and fixed action
buttons.

When a touchmap is loaded, the user can display an overlay on top of the scrcpy
video. The overlay shows where each gamepad input will touch the device screen.
It also has an edit mode for moving and resizing mapped controls.

## User Workflow

Start scrcpy with a touchmap:

```bash
scrcpy --gamepad-touchmap /path/to/touchmap.json
```

Load or unload a touchmap at runtime:

- Shortcut modifier + `T`: open a file dialog and load a touchmap.
- Shortcut modifier + `Shift+T`: unload the current touchmap.

Display and edit the overlay:

- Shortcut modifier + `E`: toggle the overlay, unless edit mode is active.
- Click the `EDIT` button in the overlay to enter edit mode.
- Click the `CLOSE` button in edit mode to leave edit mode.
- `Ctrl+S`: open a save dialog for the current touchmap.

The "shortcut modifier" is scrcpy's configured shortcut modifier, not
hard-coded Ctrl. The edit save shortcut is currently hard-coded to Ctrl.

## Data Model

The runtime model is defined in `app/src/touchmap.h`.

`struct sc_gptm_gamepad_touchmap` owns:

- `joystick[2]`: current left and right stick values in SDL axis units.
- `walk`: the left-stick touch control.
- `button_cnt`: number of mapped buttons.
- `buttons[]`: flexible array of regular and skill buttons.

`struct sc_gptm_walk_control` stores:

- `center`: frame-space center of the movement joystick.
- `radius`: maximum touch movement distance.
- `current_pos`: current injected touch point.
- `touch_down`: whether the virtual walk finger is currently down.
- `finger_id`: stable virtual pointer id for this control.

`struct sc_gptm_touch_button` stores:

- `center`: frame-space touch point or skill center.
- `radius`: skill aiming radius. Regular buttons currently use `0`.
- `current_pos`: current injected touch point.
- `touch_down`: whether this button's virtual finger is down.
- `finger_id`: stable virtual pointer id for this mapping.
- `button`: SDL controller button id, with LT/RT represented as
  `SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERLEFT/RIGHT`.
- `is_skill`: true for skill-casting mappings.

The source of truth for persisted positions is device frame coordinates, not
window coordinates. Overlay rendering converts frame coordinates to the current
content rectangle, and mouse editing converts window coordinates back to frame
coordinates.

## JSON Format

Touchmap files are parsed by `parse_touchmap_config()` in `app/src/touchmap.c`
and saved by `save_touchmap_config()`.

Supported mapping sections:

```json
{
  "mappings": {
    "walk_control": {
      "center": { "x": 310, "y": 845 },
      "radius": 150
    },
    "button_mappings": [
      {
        "button": "A",
        "touch": { "x": 1765, "y": 920 }
      }
    ],
    "skill_casting": [
      {
        "button": "LB",
        "center": { "x": 1427, "y": 944 },
        "radius": 180
      }
    ]
  }
}
```

The parser also tolerates extra fields such as `type`, `joystick`,
`packageName`, and `titles` because it only reads the fields it needs. The
saver does not preserve unknown fields; it writes a normalized file containing
the known mapping data only.

Supported button names include:

- Face buttons: `A`, `B`, `X`, `Y`
- Navigation: `BACK`, `SELECT`, `GUIDE`, `HOME`, `START`
- Sticks: `LTHUMB`, `L3`, `RTHUMB`, `R3`
- Shoulders/triggers: `LB`, `L1`, `RB`, `R1`, `LT`, `L2`, `RT`, `R2`
- D-pad: `UP`, `DOWN`, `LEFT`, `RIGHT`
- SDL extras: `MISC`, `PADDLE1`, `PADDLE2`, `PADDLE3`, `PADDLE4`,
  `TOUCHPAD`

## Runtime Input Flow

The main runtime logic is in `app/src/input_manager.c`.

When no touchmap is loaded, gamepad input follows scrcpy's normal gamepad
processor path.

When a touchmap is loaded:

1. SDL gamepad events are handled by `sc_input_manager_handle_event()`.
2. Left and right stick axis values update `game_touchmap->joystick`.
3. Left stick updates the walk touch through `sc_handle_touchmap_walk()`.
4. Right stick updates any pressed skill buttons through
   `sc_handle_touchmap_skill_cast()`.
5. Button and trigger events call `sc_handle_touchmap_button()`.
6. Touch events are injected through `simulate_virtual_touch()` as
   `SC_CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT`.

Walk behavior:

- The left stick maps to `walk.center + stick * walk.radius`.
- Below `SC_GPTM_WALK_CONTROL_DEADZONE`, the walk finger is released.
- Above the deadzone, the walk finger is pressed at the center and moved to the
  current position.

Regular button behavior:

- On press, inject DOWN at the mapped center.
- On release, inject UP at the last current position.

Skill button behavior:

- On press, inject DOWN at the skill center.
- If the right stick is already outside the deadzone, a short delayed move is
  scheduled so the skill direction can be applied after the DOWN event.
- While a skill button is held, right stick movement updates the skill touch
  position within the skill radius.
- On release, inject UP at the current skill position.

## Overlay Rendering

The overlay is initialized and owned by `struct sc_display` in
`app/src/display.h`.

The render flow is:

1. `sc_display_render()` draws the device frame.
2. If a touchmap is attached, it calls `sc_touchmap_overlay_render()`.
3. The overlay draws into the same SDL renderer before `SDL_RenderPresent()`.

Current visual elements:

- Walk zone: low-opacity white filled circle with white outline.
- Walk direction labels: arrow glyphs around the walk center.
- Regular buttons: green translucent circles.
- Skill buttons: blue translucent circles.
- Skill radius in edit mode: dashed white circle.
- Active touch positions: solid filled indicators.
- Button labels: built-in bitmap glyphs for common controller labels.
- Edit control: `EDIT` or `CLOSE` button in the top-right of the content rect.

The overlay code applies display orientation transforms so rendered mapping
positions follow scrcpy orientation changes. Stored coordinates remain in
unrotated frame coordinates.

## Edit Mode

Edit mode is implemented in `app/src/input_manager.c` with visual support from
`app/src/touchmap_overlay.c`.

Entering edit mode:

- The overlay must be enabled and a touchmap must be loaded.
- A left click on the overlay `EDIT` button switches to edit mode.

Editing gestures:

- Drag the walk center to move the walk control.
- Drag the walk radius edge to resize the walk control.
- Drag a button center to move a button.
- Drag a skill radius edge to resize a skill area.

Change tracking:

- Edits mutate the loaded `game_touchmap` directly.
- `touchmap_dirty` is set on movement or resize.
- Leaving edit mode prompts to save if there are unsaved changes.
- Choosing save starts a save-file dialog thread and exits edit mode after a
  successful save.
- Choosing no discards only the dirty flag; it does not reload the original
  file. The in-memory edited map remains active.

Minimum radius:

- Radius edits clamp to `SC_TOUCHMAP_MIN_RADIUS`, currently `32`.

## File and Module Responsibilities

- `app/src/touchmap.h`: runtime touchmap structs and public parser/saver API.
- `app/src/touchmap.c`: JSON parsing, JSON saving, button name conversion, and
  button sorting for binary search.
- `app/src/input_manager.h`: input manager state, edit drag state, and touchmap
  fields.
- `app/src/input_manager.c`: shortcut handling, file dialogs, runtime
  gamepad-to-touch translation, edit hit testing, drag mutation, and save flow.
- `app/src/touchmap_overlay.h`: overlay rendering API and color constants.
- `app/src/touchmap_overlay.c`: SDL overlay drawing, coordinate transforms,
  glyph labels, edit button layout, and overlay visibility/edit-mode state.
- `app/src/display.h`: display-owned overlay state and active touchmap pointer.
- `app/src/display.c`: overlay initialization, destruction, rendering, and
  display-level touchmap setters/toggles.
- `app/src/screen.c`: window/drawable/frame coordinate conversion and routing
  SDL events into the input manager.
- `app/meson.build`: includes `touchmap.c`, `touchmap_overlay.c`, and links
  libm for math functions.

## Known Limitations

- Edit mode does not currently suppress gamepad-to-touch injection. Editing
  while actively using the gamepad can interact with the device.
- Edit mode does not currently suppress normal mouse touch injection unless a
  drag/edit hit test consumes the event. This means ordinary clicks can still
  reach the underlying game.
- The in-memory map is edited directly. There is no separate draft copy or undo
  stack.
- Choosing "No" when leaving edit mode clears the dirty flag but does not
  revert the in-memory edits.
- Save rewrites only known fields and drops unknown metadata.
- File dialogs run on SDL threads and communicate back through custom SDL
  events. This is functional, but state ownership should be treated carefully.
- Hit testing and editor logic live in `input_manager.c`, which is convenient
  but will become hard to maintain if the editor grows.
- The right stick is hard-coded as the skill aiming stick.
- The left stick is hard-coded as the walk stick.
- Axis trigger thresholds are hard-coded.
- The overlay colors are hard-coded.

## Reconciled Notes From Earlier Docs

The previous docs were correct that:

- The overlay is drawn on top of the scrcpy video with translucent shapes.
- Runtime loading through a file dialog is supported.
- The overlay updates live from current touch state.
- The implementation is integrated through `display`, `input_manager`,
  `touchmap_overlay`, and Meson.

The previous docs are stale or incomplete in these areas:

- Regular buttons are green in the current code, not red.
- Walk control is low-opacity white in the current code, not green.
- Button labels are already implemented with built-in glyphs; SDL_ttf is not
  required for current labels.
- Overlay orientation transforms are already implemented.
- Edit mode, dirty tracking, and save prompts are now part of the feature.
- Shortcuts should be described as scrcpy shortcut modifier combinations where
  applicable, not always Ctrl combinations.

## Design Decisions

These decisions were agreed for the next implementation passes:

- Edit mode must suppress touch output. Gamepad-to-touch output must be
  disabled while editing, and normal mouse clicks used for overlay editing
  should not accidentally touch the underlying game.
- Discarding changes must reload the original touchmap file and reposition all
  controls from disk.
- `Ctrl+S` should save directly to the current loaded file when possible.
  `Ctrl+Shift+S` should open Save As.
- Editor behavior should be split out of `input_manager.c` because more edit
  actions will be added later.
- Saving should preserve metadata and unknown fields from the source JSON.
- Walk-stick and skill-stick configurability is deferred.
- Trigger thresholds should remain hard-coded for now unless controller
  reliability problems appear.
- The editor should show selection state.
- Keyboard/gamepad nudging is undecided. Keep it recorded for later.
- Add/remove/rebind controls are required later, after earlier editor
  foundations are in place.
- Controller-driven editing is not planned.

## Implementation Todo

Use this list to track future work. Implement one item at a time.

### Editor Safety

- [x] Suppress all gamepad-to-touch output while edit mode is active.
- [x] Suppress underlying mouse touch/click injection while edit mode is active,
  except for overlay editor interactions.
- [x] Ensure any active virtual touches are released or otherwise made safe
  when entering edit mode.

### Save, Discard, and JSON

- [x] Implement true discard by reloading the current touchmap file from disk
  and refreshing overlay positions.
- [ ] Change `Ctrl+S` to save directly to the current loaded file when one is
  available.
- [ ] Add `Ctrl+Shift+S` as Save As.
- [ ] Preserve source JSON metadata and unknown fields when saving.

### Editor Architecture

- [ ] Move editor hit testing, selection state, and mutation logic out of
  `input_manager.c` into a dedicated editor module, likely
  `touchmap_editor.{c,h}`.
- [ ] Add selection state for edited controls.
- [ ] Render visible selection state in the overlay.

### Later Editor Actions

- [ ] Add focused tests for JSON parse/save behavior and coordinate transforms
  where the local test harness supports it.
- [ ] Decide whether to add keyboard nudging for precise movement and radius
  changes.
- [ ] Add add/remove workflows for mapped controls.
- [ ] Add rebinding workflows for mapped controls.

### Deferred Runtime Configuration

- [ ] Make walk stick and skill stick configurable instead of hard-coded.
- [ ] Revisit configurable trigger thresholds only if hard-coded thresholds
  cause unreliable behavior across controllers.
