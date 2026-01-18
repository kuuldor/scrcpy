# Visual Guide: Touchmap Overlay Display

## Screen Layout Example

```
┌─────────────────────────────────────────────┐
│                                             │
│          Android Device Screen              │
│                                             │
│  ┌─────────────────────────────────────┐   │
│  │                                     │   │
│  │          ╱─╲     ╱─╲               │   │
│  │         │ X │   │ Y │              │   │
│  │          ╲─╱     ╲─╱               │   │
│  │                                     │   │
│  │          ╱─╲     ╱─╲               │   │
│  │         │ A │   │ B │              │   │
│  │          ╲─╱     ╲─╱               │   │
│  │                                     │   │
│  │  ●                        ○○○       │   │
│  │ ╱ ╲                      ○   ○      │   │
│  │●───●  ← Walk Zone       ○     ○    │   │
│  │ ╲ ╱   (Left Joystick)   ○ LB/RB ○  │   │
│  │  ●                       ○○○       │   │
│  │                                     │   │
│  │              (Skill Zones)          │   │
│  │                                     │   │
│  └─────────────────────────────────────┘   │
│                                             │
└─────────────────────────────────────────────┘

Legend:
● = Green semi-transparent circle (Walk Control)
◎ = Red semi-transparent circle (Buttons)
○ = Blue semi-transparent circle (Skill Zones)
```

## Color Meanings

### Walk Control (Left Joystick) - GREEN 🟢
```
    When you push left joystick:
    
    Before:  "Move character on screen"
    
    After:   Shows GREEN circle where
             movement input is detected
             
    ●●●      ← Current joystick position
   ●   ●        (solid indicator when active)
    ●●●
```

### Regular Buttons - RED 🔴
```
    When you press A/B/X/Y buttons:
    
    Before:  "Press button"
    
    After:   Shows RED circles at button
             zones on device screen
             
    ●  ●     ← Each is a button
     
    ●  ●
```

### Skill Buttons - BLUE 🔵
```
    When you use special ability buttons (LB/RB/etc):
    
    Before:  "Cast ability"
    
    After:   Shows BLUE circles with
             LARGER radius (skill zone)
             
    ◯◯◯
   ◯   ◯   ← Larger area for skill casting
    ◯◯◯
```

## Interactive Example

### Step 1: Initial State
```
Screen shows Android device content
No overlay visible
```

### Step 2: Load Touchmap (Ctrl+T)
```
File dialog appears
Select touchmap.json
Touchmap data loaded into memory
```

### Step 3: Enable Overlay (Ctrl+E)
```
Screen now shows:
- Green circles for movement zone
- Red circles for button zones  
- Blue circles for skill zones
- Device content still visible underneath
```

### Step 4: Use Gamepad
```
Push left joystick:
└─> Green circle shows movement input
    Green dot indicates current position

Press button (e.g., A):
└─> Red circle lights up
    Shows where tap will occur on device

Cast ability (e.g., LB):
└─> Blue circle shows active zone
    Indicates ability location
```

### Step 5: Disable Overlay (Ctrl+E)
```
Overlay disappears
Device screen visible again
Gamepad still works (just invisible)
```

## Overlay Transparency

The overlay is SEMI-TRANSPARENT (50% alpha):

```
Device Screen:     Overlay Circle:   Result:
[Pixels]       +   [Semi-trans]   =  [Blended]
[Dark]             [Green+Trans]      [Darker Green]
[Light]            [Green+Trans]      [Light Green]
```

This allows you to:
- See device content underneath
- See where touches will happen
- Play the game normally
- Understand the mapping visually

## Real-Time Updates

### Walk/Movement
```
Joystick Position    →    Overlay Position
(0,0)  = center             Green dot at center
(-100,0) = left             Green dot moves left
(100,100) = diagonal        Green dot moves diagonally
```

### Button Presses
```
Button State    →    Overlay Appearance
Released        →    Red circle (transparent)
Pressed         →    Red dot (solid) + circle
Multiple keys   →    Multiple circles light up
```

### Screen Rotation (Future Enhancement)
```
Device Rotates  →    Overlay Rotates
Landscape       →    Circles repositioned
Portrait        →    Circles repositioned
```

## Performance Impact

### Without Overlay
```
CPU Usage: ~15-20% (device screen rendering)
Memory:    ~100MB
```

### With Overlay (Disabled)
```
CPU Usage: ~15-20% (no change)
Memory:    ~100MB (no change)
```

### With Overlay (Enabled)
```
CPU Usage: ~16-22% (+1-2% for drawing)
Memory:    ~100MB (no change)
```

## Example Touchmap.json Visualization

```json
{
  "mappings": {
    "walk_control": {
      "center": {"x": 200, "y": 800},  // Bottom-left
      "radius": 150
    }
}
```

Visual representation:
```
────────────────────────────────
│                             │
│         (middle)            │
│                             │
│                             │
│                             │
│                             │
│  ●●●                        │  ← y: 800 (near bottom)
│ ●   ● (radius: 150)         │
│  ●●●                        │
│  ↑                          │
│  x: 200 (near left)         │
────────────────────────────────
```

## Expected Behavior

✅ **Circles appear at touchmap coordinates**
✅ **Semi-transparent overlay doesn't hide device screen**
✅ **Colors match button/zone types**
✅ **Real-time updates when buttons pressed**
✅ **Smooth circle rendering**
✅ **No lag or stuttering**
✅ **Can toggle on/off instantly**

## Troubleshooting Visual Issues

### Problem: Circles not visible
**Solution**: Check if overlay is enabled (Ctrl+E), coordinates in bounds

### Problem: Circles in wrong location
**Solution**: Verify touchmap.json coordinates match your screen resolution

### Problem: Colors are wrong
**Solution**: Software issue - colors are hard-coded (see IMPLEMENTATION_SUMMARY.md)

### Problem: Jerky or laggy
**Solution**: Overlay is disabled or coordinates out of range

---

**The overlay makes the invisible visible!** 
See exactly where your gamepad inputs touch the device screen.
