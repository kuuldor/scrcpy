# UI Module Design

This document describes a generic host-side UI module for the scrcpy client.
It is intentionally independent from any specific feature. The immediate goal is
to integrate a reusable UI layer into scrcpy's rendering and event flow without
disrupting the existing window, video, and device-input architecture.

## Goals

- provide a small generic UI module inside the scrcpy client;
- integrate with the existing SDL render loop and event routing;
- support layered overlay UI on top of the video content;
- support pointer hover, press, drag, capture, and keyboard focus;
- support both screen-space and frame-space widgets;
- keep device input forwarding separate from UI event handling;
- stay lightweight and idiomatic to the current scrcpy client codebase.

## Non-Goals

- no general-purpose desktop widget toolkit;
- no text shaping or font system in the first version;
- no automatic layout engine such as flexbox or grids in the first version;
- no animation framework in the first version;
- no worker-thread rendering or UI mutation.

## Design Constraints

- The UI runs on the SDL main thread only.
- `sc_screen` remains the composition root for window geometry and event
  routing.
- `sc_display` remains a render-only module.
- The UI module does not inject Android input events directly.
- Feature-specific state stays outside the UI core.
- Event consumption must be explicit so unconsumed events can continue through
  the existing input pipeline.

## Existing Integration Points

The current client architecture already provides the right boundaries for a UI
module.

- `app/src/screen.c` owns the SDL window, the visible content rectangle,
  orientation, and the main event routing.
- `app/src/display.c` already renders the video texture and optional overlay
  content before `SDL_RenderPresent()`.
- `app/src/input_manager.c` currently handles device input forwarding and some
  overlay-specific behavior.

The new UI module should fit between these responsibilities instead of replacing
them.

## Recommended Ownership Model

The UI context should be owned by `struct sc_screen`.

- `sc_screen` owns one `struct sc_ui_context` per SDL window.
- `sc_display` borrows the UI context during rendering.
- feature modules register UI layers into the UI context.
- feature modules own their domain state and expose it through UI callbacks.

This follows the existing pattern where parent structs embed child module state
and pass borrowed collaborators through init parameters.

## Core Model

The UI module should use a small retained model.

Retained mode is preferred because scrcpy overlays need stable interaction
state:

- hovered item;
- pressed item;
- pointer capture during drag;
- keyboard focus;
- cancellation and cleanup when a layer is hidden or removed.

The core does not need a full scene graph. A flat retained registry per layer is
enough for the first version.

## Coordinate Spaces

The UI module must support two coordinate spaces.

- drawable space: coordinates in the SDL drawable/content rectangle;
- frame space: coordinates relative to the device frame before orientation
  transform.

This is necessary because some UI elements are anchored to the window while
others are anchored to the device content.

`sc_screen` already owns the geometry needed to convert between these spaces:

- `frame_size`;
- `content_rect`;
- `orientation`.

The UI core should expose conversion helpers but should not query window state
by itself.

## Event Flow

The UI event pipeline should be:

1. `scrcpy.c` polls an SDL event.
2. `sc_screen_handle_event()` handles screen-local events first.
3. `sc_screen_handle_event()` normalizes relevant SDL events into UI events.
4. `sc_ui_context_handle_event()` processes the UI event.
5. If the UI consumed the event, stop there.
6. Otherwise continue with the existing `mouse_capture` and
   `input_manager` path.

This preserves the current centralized event routing model while allowing UI to
preempt input forwarding when necessary.

## Render Flow

The render pipeline should be:

1. `sc_screen_render()` updates the current UI geometry snapshot.
2. `sc_display_render()` draws the video texture.
3. `sc_display_render()` asks the UI context to render registered visible
   layers.
4. `SDL_RenderPresent()` presents the composed frame.

The UI module should not own the renderer lifetime and should not present the
frame itself.

## Layer Model

The core abstraction should be a UI layer.

A layer is a feature-owned object registered into the UI context. It may render
widgets, handle input, or both.

Each layer should have:

- visibility/enabled state;
- z-order;
- retained interaction state owned by the core;
- feature callbacks for sync, input handling, and rendering;
- `void *userdata` for feature state.

Layers should be ordered by z-index so the top-most visible layer receives hit
testing first.

## Widget Model

The first version should keep widgets simple and explicit.

Recommended first-pass widget kinds:

- button;
- toggle button;
- menu item;
- panel or toolbar item;
- drag handle;
- circular hit target;
- purely visual item.

The core should provide generic interaction state and hit-testing dispatch. It
should not hard-code feature behavior.

## Focus and Capture Model

The UI core should explicitly track:

- hover id;
- active id;
- focus id;
- pointer capture owner.

Minimum behavior:

- press on a hit target makes it active;
- drag may capture the pointer until release or cancel;
- key events go to the focused layer or focused widget;
- `Esc` may trigger a cancel path when a layer supports it;
- hiding or removing a layer clears any focus or capture it owns.

This should be part of the generic UI core, not re-implemented by each feature.

## Threading Rules

The UI is main-thread only.

- no direct UI state mutation from worker threads;
- no direct renderer access from worker threads;
- background tasks must post results back via SDL user events;
- ownership of any heap payload must stay explicit, following the existing
  pattern in `app/src/events.c`.

## Proposed Module Split

Recommended initial files:

- `app/src/ui/ui_context.h`
- `app/src/ui/ui_context.c`
- `app/src/ui/ui_layer.h`
- `app/src/ui/ui_event.h`
- `app/src/ui/ui_render.h`
- `app/src/ui/ui_geom.h`

Optional later files:

- `app/src/ui/ui_widgets.h`
- `app/src/ui/ui_widgets.c`
- `app/src/ui/ui_theme.h`
- `app/src/ui/ui_theme.c`

## Core Responsibilities

`ui_context` should own:

- the current geometry snapshot;
- the ordered list of registered layers;
- hover, active, focus, and capture state;
- refresh-needed state.

`ui_context` should not own:

- feature domain state;
- SDL renderer lifetime;
- window lifetime;
- device input injection.

`ui_layer` should describe:

- layer z-order and visibility;
- callbacks for sync, event handling, and render;
- a borrowed `userdata` pointer.

## API Shape

The UI module should match scrcpy's existing small-module C style.

Recommended context-level API:

- `sc_ui_context_init()`
- `sc_ui_context_destroy()`
- `sc_ui_context_set_geometry()`
- `sc_ui_context_add_layer()`
- `sc_ui_context_remove_layer()`
- `sc_ui_context_handle_event()`
- `sc_ui_context_render()`
- `sc_ui_context_request_refresh()`
- `sc_ui_context_cancel_interaction()`

Recommended layer callback shape:

- `sync(layer, ui)`
- `handle_event(layer, ui, event)`
- `render(layer, render_ctx)`
- `on_detach(layer, ui)`

The layer interface may use either a small ops table or a direct callback struct.
Using a small ops table would match the existing trait style used elsewhere in
the client.

## Event Normalization

The UI core should not consume raw SDL events directly. `sc_screen` should
normalize relevant SDL events into UI events.

The first version only needs:

- pointer move;
- pointer down;
- pointer up;
- pointer wheel;
- key down;
- key up;
- text input;
- cancel/focus-lost.

This keeps SDL-specific details out of the feature layers and makes unit tests
easier.

## Refresh Policy

UI-triggered redraw should stay event-driven.

- input that changes UI state may request a refresh;
- geometry changes trigger refresh through the existing screen flow;
- the UI context does not run its own render loop.

This matches scrcpy's current rendering model and avoids extra idle work.

## Migration Strategy

The UI module should be introduced in small steps.

1. Add `sc_ui_context` with no feature layers.
2. Integrate it into `sc_screen` lifecycle and `sc_display_render()`.
3. Add SDL-to-UI event normalization in `sc_screen_handle_event()`.
4. Add one trivial proof-of-integration layer.
5. Move future overlay features to the UI layer API incrementally.

The first implementation should focus on framework boundaries, event
consumption, and render integration, not on a large widget set.

## Testing Strategy

Focused unit tests should cover:

- layer ordering and dispatch;
- hover/active/focus transitions;
- pointer capture behavior during drag;
- cancel behavior;
- geometry conversions between drawable and frame space;
- event consumption and refresh requests.

Integration tests should verify:

- consumed UI events do not leak into device input forwarding;
- visible layers render after video content;
- layer removal clears focus and capture safely.

## Open Questions

- whether widget ids should be globally unique or only unique within a layer;
- whether the first version should include generic primitive draw helpers or let
  each layer render directly with SDL;
- whether keyboard focus should be managed per widget or only per layer in the
  first version.

## Recommended First Version

The first version should stay deliberately small:

- retained interaction state in `sc_ui_context`;
- layered input dispatch;
- layered rendering;
- geometry conversion helpers;
- explicit event consumption;
- no general layout engine.

This is enough to establish a reusable UI framework inside scrcpy without adding
unnecessary complexity up front.
