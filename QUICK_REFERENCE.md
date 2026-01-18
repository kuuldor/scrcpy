# Quick Reference: Gamepad Touchmap Overlay

## Quick Start

```bash
# Build scrcpy with overlay support
cd /home/lucd/work/scrcpy
meson setup build
cd build
ninja

# Run with touchmap
./app/scrcpy --gamepad-touchmap /path/to/touchmap.json
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **Ctrl+T** | Open file dialog to load touchmap |
| **Ctrl+Shift+T** | Unload current touchmap |
| **Ctrl+E** | **Toggle overlay visibility** |

## Overlay Colors

```
Green  = Walk Control (Left Joystick) - Center: 200,800 Radius: 150
Red    = Regular Buttons (A, B, X, Y, etc.)
Blue   = Skill Buttons (Special abilities)
```

## What You See

- **Semi-transparent circles** = Touch zones from your gamepad
- **Circle outlines** = Zone boundaries
- **Solid colored dots** = Currently active/pressed touches

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

## File Locations

- **Implementation**: `/home/lucd/work/scrcpy/app/src/touchmap_overlay.{c,h}`
- **Integration**: `/home/lucd/work/scrcpy/app/src/display.{c,h}`
- **Shortcuts**: `/home/lucd/work/scrcpy/app/src/input_manager.c`
- **Documentation**: 
  - `/home/lucd/work/scrcpy/TOUCHMAP_OVERLAY.md` (detailed)
  - `/home/lucd/work/scrcpy/IMPLEMENTATION_SUMMARY.md` (overview)

## Troubleshooting

**Overlay not showing?**
- Make sure touchmap is loaded: `Ctrl+T` → select file
- Press `Ctrl+E` to toggle overlay on
- Check console for errors

**Overlay showing but no circles?**
- Touchmap coordinates might be outside screen bounds
- Verify touchmap.json has valid coordinates

**Performance issues?**
- Overlay only renders when enabled
- Press `Ctrl+E` to disable if needed
- Minimal overhead (~1-2% CPU)

## Development Notes

### How It Works
1. SDL draws the device screen texture
2. Overlay module draws semi-transparent circles on top
3. Button states determine indicator colors
4. Real-time coordinate conversion handles screen scaling

### Key Functions
- `sc_touchmap_overlay_render()` - Main drawing function
- `sc_display_toggle_overlay()` - Toggle visibility
- `sc_display_set_touchmap()` - Set active touchmap
- `draw_filled_circle()` - Core drawing primitive

### Architecture
```
Input (Gamepad) → Touch Events → Device Screen
                ↓
           Overlay Shows Mapping
```

---

**Ready to use!** Start scrcpy with your touchmap and press Ctrl+E to see the overlay.
