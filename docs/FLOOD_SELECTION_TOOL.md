# Flood Selection Tool Documentation

## Overview

The Flood Selection Tool is a powerful selection feature directly inspired by the flood fill algorithm. It allows users to select areas of similar colors with a single click, making it ideal for selecting backgrounds, objects with uniform colors, or any contiguous area of similar pixels.

## How It Works

The tool uses a stack-based flood selection algorithm that:
1. Starts from the clicked pixel
2. Finds all connected pixels with similar colors within the tolerance range
3. Creates a selection area that can be manipulated or deleted
4. Provides visual feedback showing the selected region

## Usage

### Basic Selection
1. Select the "FloodSel" tool from the tool panel
2. Click on any area of the image you want to select
3. The tool will automatically select all connected pixels with similar colors
4. Selected pixels are highlighted with a blue overlay

### Color Tolerance
- Use the tolerance slider in the tool properties panel
- **Low tolerance (0-10)**: Very precise - selects only very similar colors
- **Medium tolerance (10-30)**: Moderate - selects nearby colors
- **High tolerance (30+)**: Loose - selects a wide range of colors

### Deleting Selected Pixels
- After making a flood selection, press the **Delete** key
- This will make all selected pixels transparent
- The operation is undoable (Ctrl+Z)

## Technical Implementation

### Algorithm Inspiration
The flood selection is directly inspired by the flood fill algorithm used in the Fill tool. Both use:
- Stack-based traversal for memory efficiency
- Pixel-by-pixel color comparison
- Proper boundary checking to prevent crashes

### Key Features
- **Memory efficient**: Uses stack-based algorithm instead of recursive calls
- **Safe boundaries**: Prevents out-of-bounds access
- **Undo support**: Saves state before destructive operations
- **Layer aware**: Works with the currently active layer
- **Tolerance based**: Flexible color matching system

### Color Comparison
```cpp
bool isColorSimilar(ImVec4 color1, ImVec4 color2) const {
    float tolerance = m_tolerance / 255.0f;
    
    return (std::abs(color1.x - color2.x) <= tolerance &&
            std::abs(color1.y - color2.y) <= tolerance &&
            std::abs(color1.z - color2.z) <= tolerance &&
            std::abs(color1.w - color2.w) <= tolerance);
}
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Delete | Delete selected pixels (make transparent) |
| Ctrl+D | Deselect all |
| Escape | Cancel current selection |

## Use Cases

### Background Removal
1. Set tolerance to medium (15-25)
2. Click on background area
3. Press Delete to remove background
4. Result: Clean cutout with transparent background

### Object Isolation
1. Use low tolerance (5-10) for precise selection
2. Click on object with uniform color
3. Use selection for further editing or copying

### Color Replacement
1. Select area with flood selection
2. Use other tools to paint over selection
3. Creates clean color replacement

## Limitations

- Works best with areas of uniform or similar colors
- May not work well with heavily textured or noisy images
- Performance depends on the size of the selected area
- Cannot select across different layers

## Tips for Best Results

1. **Start with low tolerance** and increase if needed
2. **Use on clean images** without excessive noise
3. **Combine with other tools** for complex selections
4. **Save your work** before using Delete function
5. **Use undo** (Ctrl+Z) if selection is not what you expected

## Integration Notes

- Tool index: 8 (FloodSel button)
- Inherits from base Tool class
- Uses Canvas and Layer system
- Integrates with Editor for undo/redo
- Follows same patterns as other selection tools

## Future Enhancements (TODO)

- [ ] Add feathering option for softer edges
- [ ] Support for additive selections (hold Shift)
- [ ] Support for subtractive selections (hold Alt)
- [ ] Preview mode before committing selection
- [ ] Magic wand style selection with multiple click points

## Troubleshooting

**Selection too large/small**: Adjust tolerance slider
**Nothing selected**: Check if layer is locked or empty
**Performance issues**: Reduce tolerance or work on smaller areas
**Unexpected results**: Ensure you're on the correct layer

---

*"Enough is cute - but flood selection is serious business"*