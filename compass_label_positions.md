# Compass Label Positioning Calculations

## Screen and Compass Dimensions
- Screen size: 240x320 pixels
- Compass center: (120, 160)
- Compass circle radius: 90 pixels
- Label radius: 105 pixels (15px outside circle edge)
- Font size: 10px

## Formula for Upright Text Positioned Radially
For angle θ (measured clockwise from north/0°):
- x = center_x + label_radius × sin(θ)
- y = center_y - label_radius × cos(θ) + (font_size / 2)

Where:
- center_x = 120
- center_y = 160
- label_radius = 105
- font_size / 2 = 5 (for vertical centering)

## Calculated Label Positions
| Angle | Side      | x     | y     |
|-------|-----------|-------|-------|
| 0°    | North     | 120.0 | 60.0  |
| 30°   | Starboard | 172.5 | 74.1  |
| 60°   | Starboard | 210.9 | 112.5 |
| 90°   | Starboard | 225.0 | 165.0 |
| 120°  | Starboard | 210.9 | 217.5 |
| 150°  | Starboard | 172.5 | 255.9 |
| 180°  | South     | 120.0 | 270.0 |
| 210°  | Port      | 67.5  | 255.9 |
| 240°  | Port      | 29.1  | 217.5 |
| 270°  | Port      | 15.0  | 165.0 |
| 300°  | Port      | 29.1  | 112.5 |
| 330°  | Port      | 67.5  | 74.1  |

## Notes
- Text uses text-anchor="middle" for horizontal centering
- The +5px adjustment to y centers text vertically on the radial line
- Labels are symmetric: port and starboard sides mirror each other
- Yacht notation: angles shown as 0-180° on both port and starboard sides

## Python Code for Calculation

```python
import math

# Compass dimensions
center_x = 120
center_y = 160
circle_radius = 90
label_radius = 105  # 15px outside circle edge
font_size = 10
half_font = font_size / 2

# Calculate label positions for all major tick angles
angles = [0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330]

print("Compass Label Positions:")
print("=" * 40)

for angle in angles:
    rad = math.radians(angle)
    x = center_x + label_radius * math.sin(rad)
    y = center_y - label_radius * math.cos(rad) + half_font
    print(f"{angle:3d}°: x={x:5.1f}, y={y:5.1f}")
```

