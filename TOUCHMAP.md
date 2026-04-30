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
- If the overlay is shown with no touchmap loaded, click `NEW` to create an
  empty touchmap and enter edit mode.
- Click the `EDIT` button in the overlay to enter edit mode.
- Click the `QUIT` button in edit mode to leave edit mode.
- `Ctrl+S`: save directly to the current touchmap file when one is loaded.
- `Ctrl+Shift+S`: open Save As for the current touchmap.

The "shortcut modifier" is scrcpy's configured shortcut modifier, not
hard-coded Ctrl. The save shortcuts are currently hard-coded to Ctrl.

## Data Model

The runtime model is defined in `app/src/touchmap.h`.

`struct sc_gptm_gamepad_touchmap` owns:

- `joystick[2]`: current left and right stick values in SDL axis units.
- `has_walk`: whether the optional walk control exists.
- `walk`: the left-stick touch control when `has_walk` is true.
- `json_root`: retained parsed JSON tree used to preserve metadata on save.
- `button_cnt`: number of mapped buttons.
- `buttons[]`: flexible array of regular and skill buttons.

`struct sc_gptm_walk_control` stores:

- `center`: frame-space center of the movement joystick.
- `radius`: maximum touch movement distance.
- `current_pos`: current injected touch point.
- `touch_down`: whether the virtual walk finger is currently down.
- `finger_id`: stable virtual pointer id for this control.
- `json_entry`: non-owning pointer into `json_root` for the JSON object that
  stores this control.

`struct sc_gptm_touch_button` stores:

- `center`: frame-space touch point or skill center.
- `radius`: skill aiming radius. Regular buttons currently use `0`.
- `current_pos`: current injected touch point.
- `touch_down`: whether this button's virtual finger is down.
- `finger_id`: stable virtual pointer id for this mapping.
- `button`: SDL controller button id, with LT/RT represented as
  `SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERLEFT/RIGHT`.
- `is_skill`: true for skill-casting mappings.
- `json_entry`: non-owning pointer into `json_root` for the JSON object that
  stores this control.

`struct sc_touchmap_editor`, defined in `app/src/touchmap_editor.h`, owns edit
selection and drag state. It tracks the selected target and the active drag
target separately, so later editor actions can operate on the selected control
without depending on the input manager.

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

`walk_control` is optional. A touchmap may have zero or one walk control. Empty
or newly created maps may contain only empty button and skill arrays:

```json
{
  "mappings": {
    "button_mappings": [],
    "skill_casting": []
  }
}
```

The parser also tolerates extra fields such as `type`, `joystick`,
`packageName`, and `titles` because it only reads the fields it needs. The
saver preserves those unknown fields by retaining the parsed JSON tree,
duplicating it on save, updating known fields, and rebuilding the regular and
skill mapping arrays from the current in-memory controls. On successful save,
the map's retained `json_root` and per-control `json_entry` pointers are
relinked to the saved JSON model.

Supported button names include:

- Face buttons: `A`, `B`, `X`, `Y`
- Navigation: `BACK`, `SELECT`, `GUIDE`, `HOME`, `START`
- Sticks: `LTHUMB`, `L3`, `RTHUMB`, `R3`
- Shoulders/triggers: `LB`, `L1`, `RB`, `R1`, `LT`, `L2`, `RT`, `R2`
- D-pad: `UP`, `DOWN`, `LEFT`, `RIGHT`
- SDL extras: `MISC`, `PADDLE1`, `PADDLE2`, `PADDLE3`, `PADDLE4`,
  `TOUCHPAD`

## Runtime Input Flow

The main runtime logic is split between `app/src/input_manager.c` and
`app/src/touchmap_runtime.c`.

When no touchmap is loaded, gamepad input follows scrcpy's normal gamepad
processor path.

When a touchmap is loaded:

1. SDL gamepad events are handled by `sc_input_manager_handle_event()`.
2. Axis and button events delegate to `sc_touchmap_runtime_handle_event()`.
3. The runtime module updates `game_touchmap->joystick` with left and right stick values.
4. Walk and skill-cast are processed immediately by `sc_touchmap_runtime_handle_walk()`
   and `sc_touchmap_runtime_handle_skill_cast()`.
5. Touch events are injected through `simulate_virtual_touch()` as
   `SC_CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT`.

Walk behavior:

- If no walk control exists, left-stick movement does not inject touch events.
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

Edit-mode safety:

- Entering edit mode releases any active touchmap virtual touches first.
- While edit mode is active, gamepad-to-touch output is suppressed.
- While edit mode is active, mouse/touch/scroll input is consumed by the editor
  and does not reach the underlying Android game, except for overlay editor
  interactions.

## Overlay Rendering

The overlay is initialized and owned by `struct sc_display` in
`app/src/display.h`.

The render flow is:

1. `sc_display_render()` draws the device frame.
2. If a touchmap is attached, it calls `sc_ui_touchmap_layer_render()`.
3. The overlay draws into the same SDL renderer before `SDL_RenderPresent()`.

Current visual elements:

- Walk zone: low-opacity white filled circle with white outline.
- Walk direction labels: arrow glyphs around the walk center.
- Regular buttons: green translucent circles.
- Skill buttons: blue translucent circles.
- Skill radius in edit mode: dashed white circle.
- Active touch positions: solid filled indicators.
- Button labels: built-in bitmap glyphs for common controller labels.
- Selection highlight: yellow multi-ring outline around the selected center or
  radius target in edit mode.
- Edit control: green `EDIT` or red `QUIT` button in the top-right of the
  content rect.

The overlay code applies display orientation transforms so rendered mapping
positions follow scrcpy orientation changes. Stored coordinates remain in
unrotated frame coordinates.

## Edit Mode

Edit mode is coordinated by `app/src/touchmap_state.c`, but editor hit testing,
selection state, and drag mutation live in `app/src/touchmap_editor.c`. Visual
feedback is rendered by `app/src/ui_touchmap_layer.c`.

Entering edit mode:

- The overlay must be enabled and a touchmap must be loaded.
- A left click on the overlay `EDIT` button switches to edit mode.

Editing gestures:

- Drag the walk center to move the walk control.
- Drag the walk radius edge to resize the walk control.
- Drag a button center to move a button.
- Drag a skill radius edge to resize a skill area.
- Use the arrow keys to nudge the selected control by one frame pixel.
- Hold `Shift` with an arrow key to nudge by ten frame pixels.
- For selected radius targets, `Right`/`Up` increase the radius and
  `Left`/`Down` decrease it.

Change tracking:

- Edits mutate the loaded `game_touchmap` directly.
- `touchmap_dirty` is set on movement or resize.
- Leaving edit mode prompts to save if there are unsaved changes.
- Choosing save starts a save-file dialog thread and exits edit mode after a
  successful save.
- Choosing no reloads the current touchmap file from disk, restoring saved
  positions and sizes before exiting edit mode.
- `Ctrl+S` saves directly to the current file when available. `Ctrl+Shift+S`
  opens Save As.

Minimum radius:

- Buttons and skills clamp to `SC_TOUCHMAP_BUTTON_RADIUS`, currently `24`.
- Walk controls clamp to `SC_TOUCHMAP_WALK_RADIUS`, currently `32`.

Planned add/remove workflow:

- Edit mode shows a floating toolbar on top of the overlay with `ADD`, `DEL`,
  and `QUIT` buttons. The old top-right `QUIT` button moved into this toolbar.
- `ADD` opens a dropdown menu with `Button`, `Skill`, and `Walk`.
- Choosing `Button`, `Skill`, or `Walk` adds that control type directly at the center of
  the screen.
- `Walk` is disabled in the `ADD` dropdown when a walk control already exists.
- The touchmap may have zero or one walk control. This supports building a
  mapping from an empty file.
- `DEL` removes the currently selected control, including Walk.
- Selecting a button or skill mapping in edit mode makes it listen for the next
  gamepad button or trigger press. Walk is not bindable while walk-stick
  behavior remains hard-coded.
- New button and skill controls may initially have no binding. No-binding
  controls are shown in red to warn the user.
- In edit mode, the next gamepad button or trigger press is assigned to the
  selected button or skill mapping.
- Direct selected binding is shared by new controls and existing controls, so
  adding a new binding and rebinding use the same workflow.
- Binding to an input already used by another mapping is allowed. The currently
  selected mapping gets the binding, and the old mapping loses its binding.
- Saving is disallowed while any button or skill mapping has no binding. The
  UI shows a warning dialog and keeps edit mode open so the user can bind the
  red controls.
- `Esc` cancels the pending add operation.
- New regular button mappings use radius `SC_TOUCHMAP_BUTTON_RADIUS`, currently `24`.
- New skill mappings use radius `SC_TOUCHMAP_WALK_RADIUS`, currently `32`.
- Newly added controls are selected after addition. Removed controls clear
  selection or select a nearby remaining control.
- After a successful add, remove, or bind operation, the map is marked dirty and
  the overlay receives the updated map pointer if the map allocation changed.

## File and Module Responsibilities

- `app/src/touchmap.h`: runtime touchmap structs and public parser/saver API.
- `app/src/touchmap.c`: JSON parsing, JSON saving, button name conversion, and
  button sorting for binary search. Saving preserves unknown metadata and
  relinks the retained JSON model after successful writes.
- `app/src/touchmap_state.h`: touchmap load/unload/save state, overlay display/edit
  mode state, dirty tracking, auto-mode state, and screen refresh orchestration.
- `app/src/touchmap_state.c`: touchmap state management, dialog handling,
  foreground-app change handling, add-at-center actions, and screen refresh logic.
- `app/src/touchmap_runtime.h`: runtime gamepad-to-touch simulation API.
- `app/src/touchmap_runtime.c`: controller event handling, button press/release,
  walk and skill-cast simulation, and touch injection.
- `app/src/touchmap_editor.h`: editor selection and drag state API.
- `app/src/touchmap_editor.c`: editor hit testing, selection updates, and drag
  mutation of touchmap controls.
- `app/src/input_manager.h`: input manager state and shortut handling fields.
- `app/src/input_manager.c`: shortcut handling and gamepad event routing to the runtime
  module.
- `app/src/touchmap_utils.h`: shared utility functions for starting SDL threads.
- `app/src/touchmap/touchmap_overlay.h`: overlay coordinate transform API.
- `app/src/touchmap/touchmap_overlay.c`: SDL overlay coordinate transforms.
- `app/src/touchmap/ui_touchmap_layer.h`: UI layer rendering and hit testing API.
- `app/src/touchmap/ui_touchmap_layer.c`: SDL overlay drawing, widgets,
  coordinate transforms, glyph labels, selection highlighting, edit button
  layout, and overlay visibility/edit-mode state.
- `app/src/display.h`: display-owned overlay state and active touchmap pointer.
- `app/src/display.c`: overlay initialization, destruction, rendering, and
  display-level touchmap setters/toggles.
- `app/src/screen.c`: window/drawable/frame coordinate conversion and routing
  SDL events into the input manager.
- `app/tests/test_touchmap.c`: focused tests for touchmap JSON parse/save
  metadata preservation and overlay coordinate transforms.
- `app/meson.build`: includes touchmap modules and links libm for math functions.

## Known Limitations

- The in-memory map is edited directly. There is no separate draft copy or undo
  stack.
- File dialogs run on SDL threads and communicate back through custom SDL
  events. This is functional, but state ownership should be treated carefully.
- The right stick is hard-coded as the skill aiming stick.
- The left stick is hard-coded as the walk stick.
- Axis trigger thresholds are hard-coded.
- Overlay colors are hard-coded.

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
- Overlay orientation transforms are already implemented and covered by focused
  tests.
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
- Keyboard nudging is supported for selected controls in edit mode. Gamepad
  nudging is not planned.
- Add/remove should be driven by a floating toolbar with `ADD`, `DEL`, and
  `QUIT`. `ADD` uses a dropdown for `Button`, `Skill`, and `Walk`.
- A touchmap may contain zero or one walk control. Walk can be added or deleted,
  but only when no other Walk exists.
- Empty touchmaps can be created in memory and edited, then saved through Save
  As.
- Direct selected binding is shared by newly added controls and existing
  controls.
- Button and skill mappings may temporarily have no binding, must render red,
  and must block saving until resolved.
- Rebinding to an input used by another mapping transfers the binding to the
  currently selected mapping and leaves the old mapping unbound.
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
- [x] Change `Ctrl+S` to save directly to the current loaded file when one is
  available.
- [x] Add `Ctrl+Shift+S` as Save As.
- [x] Preserve source JSON metadata and unknown fields when saving.

### Editor Architecture

- [x] Move editor hit testing, selection state, and mutation logic out of
  `input_manager.c` into a dedicated editor module, likely
  `touchmap_editor.{c,h}`.
- [x] Add selection state for edited controls.
- [x] Render visible selection state in the overlay.
- [x] Render the edit button with a green background, and the quit/close button
  with a red background.

### Later Editor Actions

- [x] Add focused tests for JSON parse/save behavior and coordinate transforms
  where the local test harness supports it.
- [x] Decide whether to add keyboard nudging for precise movement and radius
  changes.
- [x] Create new empty touchmap workflow.
  - [x] Add a `NEW` overlay control that creates an empty in-memory touchmap
    when no touchmap is loaded, so users can build a mapping from zero.
  - [x] Ensure an empty touchmap can be displayed and edited by the overlay.
  - [x] Save empty or partially built touchmaps through Save As.
  - [x] Parse and save maps with no walk control and empty button/skill arrays.
- [x] Add GUI add/remove/bind workflows for mapped controls.
  - [x] Add map mutation helpers in `touchmap.c`/`touchmap.h` for appending and
    removing controls. Button/skill helpers should return the possibly new map
    pointer because `struct sc_gptm_gamepad_touchmap` uses a flexible array.
  - [x] Add optional Walk support with `has_walk`, allowing zero or one walk
    control.
  - [x] Preserve retained JSON metadata when adding/removing by relying on the
    existing save-time JSON rebuild path. New controls should have
    `json_entry == NULL` until the next successful save relinks entries.
  - [x] Support no-binding button/skill mappings as temporary editor state.
  - [x] Render no-binding button/skill mappings in red.
  - [x] Disallow saving while any button/skill mapping has no binding.
  - [x] When binding to an already used gamepad input, transfer the binding to
    the selected mapping and clear the old mapping's binding.
  - [x] Assign stable virtual finger ids for newly added controls without
    colliding with existing walk/button ids.
  - [x] Re-sort bound entries after add/remove/bind because runtime button
    lookup depends on sorted buttons.
  - [x] Ensure runtime lookup ignores no-binding mappings.
  - [x] Update editor selection after add/remove. Added controls should become
    selected. Removed controls should clear selection or select a nearby
    remaining control.
  - [x] Add editor toolbar rendering with `ADD`, `DEL`, and `QUIT`.
  - [x] Move the existing `QUIT` edit control into the toolbar.
  - [x] Add `ADD` dropdown rendering and hit-testing for `Button`, `Skill`, and
    `Walk`.
  - [x] Gray out `Walk` in the `ADD` dropdown when a walk control already
    exists.
  - [x] Add editor mode state for select, add menu, place button, place skill,
    and place walk.
  - [x] Place new controls at the next overlay click coordinate after choosing
    a type from the `ADD` dropdown.
  - [x] Bind the next gamepad button or trigger press to the selected button or
    skill.
  - [x] Make `Esc` cancel pending add placement.
  - [x] Make `DEL` remove the currently selected control, including Walk.
  - [x] Refresh the display touchmap pointer after any add/remove operation that
    changes the map allocation.
  - [x] Mark the touchmap dirty only after successful add/remove/bind mutation.
  - [x] Add focused tests for empty maps, optional Walk, append/remove, binding
    transfer, no-binding save rejection, sorting, selection updates, and JSON
    output after saving added controls. Empty maps, optional Walk,
    append/remove helpers, no-binding representation, no-binding save
    rejection, binding transfer, sorting, metadata preservation, and JSON
    output, and editor selection updates are covered.

## Automatic Touchmap Selection

This section describes the current implementation for automatically selecting a
touchmap based on the Android package currently running in the foreground.

### Goal

The goal is to remove the need to:

- pass a touchmap file explicitly on the scrcpy command line; or
- load and unload touchmaps manually for each game.

Instead, scrcpy should detect the foreground Android package, resolve a
touchmap matching that package, and load it automatically.

### Scope

Current scope is intentionally narrow:

- match by Android package name only;
- load at most one touchmap for a package;
- support creating a new empty touchmap for the current foreground package;
- defer or block auto-switch while the editor has unsaved work;
- keep foreground-app detection host-side via polling.

Out of scope for the first version:

- matching by activity name or window title;
- multiple touchmap profiles per package;
- per-device variants;
- automatic save or overwrite on package switch;
- server-to-host protocol changes in the first version.
- device-side package monitoring inside the scrcpy server in the first version.

### User-Facing Behavior

Current startup and runtime behavior:

- `--gamepad-touchmap-dir` enables package-based auto-loading.
- If this option is not set, the manual workflow remains unchanged.
- When it is set, scrcpy scans that directory for touchmap files.
- If `--gamepad-touchmap` is also set, that explicit file is a hard manual
  selection and automatic selection is disabled.
- Each file participating in auto-load must include top-level `packageName`.
- When the foreground package changes, scrcpy tries to find a touchmap with a
  matching `packageName`.
- If found, scrcpy loads it automatically.
- If not found, scrcpy unloads the current touchmap.
- If the overlay is visible and no touchmap exists for the package, clicking
  `NEW` creates an empty in-memory touchmap pre-seeded with that current
  package name.

### Package Matching Rules

The current matching contract is:

- source of truth: JSON field `packageName`;
- key type: exact string match against detected foreground package;
- one touchmap file per package in auto mode.

If multiple files claim the same package:

- log a warning;
- pick one deterministically, currently the lexicographically first path after
  sorting by package and path;
- revisit multi-profile selection only if real usage requires it.

This keeps auto-loading explicit and avoids accidental filename-based matches.

### High-Level Architecture

The current host-side implementation is split across these modules.

#### `app/src/options.{c,h}`

Current relevant option field:

- `const char *touchmap_dir;`
This keeps auto-touchmap configuration next to the existing single-file
`touchmap_file` option.

#### `app/src/cli.c`

The CLI parser already supports the touchmap directory option.

Current behavior:

- `--gamepad-touchmap` still loads a specific file manually;
- `--gamepad-touchmap-dir` enables automatic selection from a directory;
- if both are present, the explicit `--gamepad-touchmap` file wins and
  auto-selection must not load other map files on app switches.

#### Host-Side Auto-Load Modules

- `app/src/touchmap_loader.h`
- `app/src/touchmap_loader.c`
- `app/src/fg_app_detect.h`
- `app/src/fg_app_detect.c`

`touchmap_loader` owns:

- scanning the touchmap directory;
- parsing candidate JSON files just far enough to read `packageName`;
- building and refreshing the package-to-path index.

`fg_app_detect` owns:

- host-side foreground-package polling;
- adb command execution;
- `dumpsys` parsing;
- debounce of unchanged package names;
- posting `SC_EVENT_FG_APP_CHANGED`.

This keeps the automatic selection logic out of `input_manager.c` and keeps
foreground-app detection replaceable by a future server-side source.

#### `app/src/input_manager.{c,h}`

`input_manager` should remain the main-thread owner of:

- the currently loaded touchmap;
- dirty/edit-mode safety rules;
- actual load/unload/reload/save behavior;
- user-visible switching policy.

It should not be responsible for directory scanning or package polling logic.

Instead, `input_manager` should receive package-change notifications and decide:

- load new matching touchmap;
- unload touchmap if no match exists;
- defer switching while dirty or editing;
- resume a deferred switch later if appropriate.

#### `app/src/events.h`

One custom SDL event is used for auto-touchmap package changes.

Current event:

- `SC_EVENT_FG_APP_CHANGED`

The payload is a small allocated struct, not a raw C string:

```c
struct sc_fg_app_changed_event {
    char *package_name;
};
```

Ownership rule:

- producer allocates payload;
- main-thread consumer frees payload after handling.

This follows the SDL user-event ownership model and avoids implicit `data1`
string contracts.

### Runtime State

The loader tracks:

- configured touchmap directory path;
- whether auto mode is enabled;
- indexed mapping from package name to touchmap file path;

The foreground-app detector tracks:

- configured device serial;
- current detected foreground package;
- polling thread state.

`input_manager` additionally tracks:

- whether the current loaded touchmap came from auto mode or manual override;
- whether an auto-switch is pending because dirty/editing state blocked an
  immediate switch;
- the pending target package and/or file path if such a switch is deferred.

Current touchmap auto-selection state lives in `struct sc_touchmap_state`:

- `bool auto_enabled;`
- `bool manual_override;`
- `char *current_package;`
- `char *deferred_package;`
- `char *deferred_file;`

Display/editor runtime state remains centralized there as well.

### Event and Data Flow

The current flow is:

1. Loader initializes from `touchmap_dir`.
2. Loader scans all touchmap JSON files and builds an index keyed by
   `packageName`.
3. A host-side foreground-app detector periodically queries the Android
   foreground
   package name.
4. When the detected package changes, the detector posts
   `SC_EVENT_FG_APP_CHANGED` to the SDL main thread.
5. `input_manager` receives the event.
6. `input_manager` decides among:
   - load matching touchmap;
   - unload current touchmap if no match exists;
   - defer switching because of dirty/edit-mode state.
7. If loading or unloading occurs, `input_manager` refreshes:
   - `im->touchmap.map`
   - `im->touchmap.file`
   - display touchmap pointer
   - editor state
   - dirty flags

### Switching Policy

Current policy for package changes:

#### When not editing and not dirty

- switch immediately to the resolved auto-touchmap;
- or unload immediately if there is no match.

#### When editing or dirty

- do not switch immediately;
- keep the current touchmap active;
- record the pending package and pending resolved file path;
- log the deferred switch;
- attempt the switch later when edit mode exits cleanly or dirty state is
  cleared.

This avoids surprising data loss or silent map changes while the user is
modifying a touchmap.

### Manual Override Policy

Current behavior:

- startup `--gamepad-touchmap` loads that file as a manual override, even if
  `--gamepad-touchmap-dir` is also provided;
- manual `Ctrl+T` load creates a temporary manual override;
- while manual override is active, auto-detected package changes do not
  immediately replace the manually loaded touchmap;
- `Ctrl+Shift+T` unload clears manual override and immediately reapplies the
  current foreground package mapping, if one exists;
- Save and Save As during manual override keep the manual file path current and
  only rejoin auto mode automatically if the saved file becomes the indexed
  match for the current foreground package;
- auto mode resumes only when the manual override is explicitly cleared.

### Empty-Map Creation in Auto Mode

When no auto-touchmap exists for the current foreground package:

- overlay may still be shown;
- `NEW` should create an empty map in memory;
- new map immediately stamps top-level `packageName` with the current detected
  package when one is known;
- Save As should prefer a package-based default filename, for example
  `<package>.json`.

This allows a build-from-zero workflow that fits naturally with auto-loading.

### Directory Indexing Strategy

Current behavior:

- scan directory on startup;
- include only `.json` files;
- parse minimally to extract `packageName`;
- ignore files that fail to parse or do not contain `packageName`;
- log duplicates and deterministic winner selection;
- rescan on startup and after successful saves into the auto directory.

Filesystem watching is not implemented.

### Error Handling

Auto-load failures are non-fatal. Current behavior:

- if package detection fails temporarily, keep the current touchmap and retry;
- if a matching touchmap file exists but fails to parse, log it and treat it as
  no match;
- if a deferred auto-switch target disappears before it is applied, drop the
  pending switch and log it;
- if multiple files match the same package, log a warning every time the index
  is rebuilt, not on every package switch.

### First-Version Implementation Todo

Complete these items in order. When all are complete, the host-side polling
version should work as designed.

#### 1. CLI and Option Plumbing

- [x] Add `touchmap_dir` to `struct scrcpy_options`.
- [x] Initialize `touchmap_dir` to `NULL` in default options.
- [x] Add CLI parsing for `--gamepad-touchmap-dir`.
- [x] Add help/manpage/completion text for the new option.
- [x] Pass `touchmap_dir` from `scrcpy_options` into the app runtime context.
- [x] Pass `touchmap_dir` into `sc_input_manager_params`.
- [x] Add a CLI test covering the new option.
- [x] Decide and document startup precedence when both `--gamepad-touchmap`
  and `--gamepad-touchmap-dir` are provided.

#### 2. Touchmap Package Metadata Helpers

- [x] Add a helper to read top-level `packageName` from a JSON file without
  constructing a full runtime touchmap.
- [x] Add a helper to set or replace top-level `packageName` in an in-memory
  touchmap.
- [x] Add a helper to build a default auto-save filename from a package name,
  for example `<package>.json`.
- [x] Add tests for reading, preserving, and setting `packageName`.

#### 3. Auto-Loader Module Skeleton

- [x] Add `app/src/touchmap_loader.h`.
- [x] Add `app/src/touchmap_loader.c`.
- [x] Add `app/src/fg_app_detect.h`.
- [x] Add `app/src/fg_app_detect.c`.
- [x] Register the new source files in `app/meson.build`.
- [x] Define a loader state struct owning the configured directory path,
  current resolved file, and package index.
- [x] Define a foreground-app detector state struct owning the device serial,
  current detected package, and polling state.
- [x] Add init/destroy functions with clear ownership rules.
- [x] Add focused tests for init/destroy if the local test harness can cover
  them cleanly.

#### 4. Directory Indexing

- [x] Implement startup scan of `touchmap_dir`.
- [x] Include only `.json` files.
- [x] Parse each candidate only far enough to read `packageName`.
- [x] Ignore invalid JSON and files without `packageName`, with logs.
- [x] Build an index from exact package name to file path.
- [x] Resolve duplicate package names deterministically and log the duplicate.
- [x] Add tests for valid files, invalid files, missing `packageName`,
  duplicate package names, and exact match lookup.

#### 5. Foreground Package Polling

- [x] Choose the first Android command or host API used to query the foreground
  package.
- [x] Implement a polling thread owned by the foreground-app detector.
- [x] Make the polling interval explicit and conservative.
- [x] Debounce unchanged package names so repeated identical polls do not post
  events.
- [x] Treat temporary command failures as non-fatal and retry later.
- [x] Stop and join/clean up the polling thread during shutdown.
- [x] Add logs for package changes and detection failures at appropriate levels.

#### 6. SDL Event Payload

- [x] Add `SC_EVENT_FG_APP_CHANGED` to `app/src/events.h`.
- [x] Define a typed payload struct for package-change events.
- [x] Ensure producer-side allocation and main-thread freeing are documented in
  code.
- [x] Post package-change events from the foreground-app detector to the SDL
  main thread.
- [x] Add receiver-side cleanup for every early-return path.

#### 7. Input Manager Auto State

- [x] Add input-manager fields for auto mode state, manual override state, and
  deferred package/file target.
- [x] Initialize and destroy those fields correctly.
- [x] Add small helpers for loading a touchmap file into `im->game_touchmap`
  so manual load and auto-load share behavior.
- [x] Add small helpers for unloading a touchmap and refreshing the display
  pointer.
- [x] Keep dirty/edit-mode state transitions in one place.

#### 8. Auto-Switch Policy

- [x] Handle `SC_EVENT_FG_APP_CHANGED` in the main SDL event switch.
- [x] Resolve package-to-file match through the auto-loader index.
- [x] If a match exists and the current map is clean and not editing, load it.
- [x] If no match exists and the current map is clean and not editing, unload
  the current touchmap.
- [x] If dirty or editing, defer the switch and keep the current touchmap.
- [x] Apply a deferred switch after dirty state clears or edit mode exits
  cleanly.
- [x] Drop deferred switches if the target file is no longer valid.
- [x] Log switch, unload, defer, and deferred-apply decisions.

#### 9. Manual Override Policy

- [x] Make `Ctrl+T` manual file loading mark the current map as a manual
  override when auto mode is enabled.
- [x] Decide when manual override ends and encode that in state transitions.
- [x] Ensure `Ctrl+Shift+T` unload clears manual override and allows auto mode
  to resume.
- [x] Ensure Save As after manual override updates the current file path
  without corrupting auto-loader state.
- [x] Document the final manual override rules in this section after
  implementation.

#### 10. Empty Map Creation in Auto Mode

- [x] Store the last detected foreground package in a place reachable from the
  `NEW` workflow.
- [x] When `NEW` creates a map in auto mode, stamp top-level `packageName`.
- [x] If no package is known, create the empty map without `packageName` and
  log that auto association is unavailable.
- [x] Make Save As default to `<package>.json` when a package is known.
- [x] Add tests for package stamping and default filename selection where
  practical.

#### 11. Save and Directory Index Refresh

- [x] After a successful Save As in auto mode, refresh or update the directory
  index for the saved file.
- [x] If the saved file's `packageName` matches the current foreground package,
  mark it as the active auto-loaded map.
- [x] If Save As writes outside the auto directory, decide whether this becomes
  manual override or whether auto mode should still index it.
- [x] Document and test the chosen behavior.

Chosen behavior:

- Runtime touchmap file paths and the auto directory path are normalized to
  canonical absolute paths before comparison.
- Save As inside `--gamepad-touchmap-dir` rebuilds the loader index.
- If the saved file is the indexed match for the current foreground package, it
  becomes the active auto-loaded map and manual override clears.
- Otherwise, the saved file remains a manual override.
- Save As outside `--gamepad-touchmap-dir` always remains a manual override and
  is not indexed for auto-loading.

#### 12. Verification and User Documentation

- [x] Add or update focused tests for directory indexing and package resolution.
- [x] Add focused tests for auto-switch decision logic without requiring a real
  Android device.
- [x] Run `meson compile -C build app/scrcpy`.
- [x] Run `meson test -C /tmp/scrcpy-touchmap-debug`.
- [x] Update `TOUCHMAP.md` with final decisions made during implementation.
- [x] Update `QUICK_REFERENCE.md` with the user-facing auto-load workflow.
- [x] Manually test with at least two packages and two touchmap files on a
  connected device.

### Focused Test Plan

Focused tests cover:

- directory scan extracting `packageName` from valid files;
- ignoring invalid JSON and files without `packageName`;
- duplicate package resolution;
- exact package-name match behavior;
- no-match unload decision;
- deferred switch behavior when dirty or editing;
- resuming a deferred switch after editing ends;
- `NEW` in auto mode seeding `packageName`;
- package-based default Save As filename selection.

Current verification status:

- focused tests exist for package metadata helpers, directory indexing,
  duplicate resolution, and foreground-package polling;
- build and automated test runs pass locally;
- input-manager auto-switch decision logic is covered by `app/tests/test_touchmap_state.c`;
- connected-device package-switch testing remains valuable as integration coverage.

### Remaining Questions

Foreground-app detection for the first version is intentionally host-side and
polling-based.

The current first-version behavior is settled, but these future questions
remain open:

- whether the `dumpsys activity activities` query should later be replaced by a
  more stable Android signal;
- whether multiple profiles per package are worth supporting;
- whether package matching should later consider activity names or titles.

Server-side app-switch detection remains a later improvement. It would require:

- Android-side foreground-app monitoring in the scrcpy server;
- a new server-to-host message carrying the foreground package;
- host-side integration of that new message into the same auto-switch policy.

That server-side design is intentionally deferred until the host-side polling
version proves sufficient or real usage shows clear shortcomings.

### Later Todo: Server-Side App Detection

After the host-side polling version is implemented and validated, revisit a
server-side design that:

- listens for foreground app/task/window changes on the Android device;
- resolves the active package on the server;
- notifies the host only when the package changes;
- replaces or supplements host-side polling.

### Deferred Runtime Configuration

- [ ] Make walk stick and skill stick configurable instead of hard-coded.
- [ ] Revisit configurable trigger thresholds only if hard-coded thresholds
  cause unreliable behavior across controllers.
