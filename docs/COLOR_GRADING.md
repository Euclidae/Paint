# Color Grading Features

## Overview

The Enough Image Editor now includes a comprehensive set of color grading tools designed to give you professional-level color control without the complexity of enterprise software. These tools are accessible through the **Filter > Color Grading** menu and provide intuitive controls for common color correction tasks.

## Available Tools

### Directional Blur
**Access**: Filter > Directional Blur

Creates motion blur effects in a specific direction - perfect for speed effects or artistic stylization.

**Controls**:
- **Angle**: Direction of blur (0-359 degrees)
- **Distance**: Strength of blur effect (1-20 pixels)

**Usage Tips**:
- Use low angles (0-45°) for horizontal motion effects
- Higher distances work better on high-resolution images
- Combine with other filters for cinematic effects

### Shadows/Highlights
**Access**: Filter > Color Grading > Shadows/Highlights

Separately control dark and bright areas of your image - more natural than global brightness adjustments.

**Controls**:
- **Shadows**: Adjust dark areas (-1.0 to +1.0)
- **Highlights**: Adjust bright areas (-1.0 to +1.0)

**How it Works**:
- Automatically detects shadow vs highlight areas using luminance
- Applies stronger effects to appropriate pixel ranges
- Preserves midtones better than global adjustments

### Color Balance
**Access**: Filter > Color Grading > Color Balance

Adjust individual RGB channels - like the classic Photoshop color balance tool.

**Controls**:
- **Red**: Add/remove red tint (-1.0 to +1.0)
- **Green**: Add/remove green tint (-1.0 to +1.0)
- **Blue**: Add/remove blue tint (-1.0 to +1.0)

**Common Uses**:
- Fix white balance issues
- Create warm/cool color casts
- Correct color shifts from scanning

### Curves
**Access**: Filter > Color Grading > Curves

Simplified curve adjustment for contrast and tone mapping.

**Controls**:
- **Input**: Input luminance level (0.0 to 1.0)
- **Output**: Output luminance level (0.0 to 1.0)

**Understanding Curves**:
- Input 0.5, Output 0.5 = no change
- Input 0.5, Output 0.7 = brighten midtones
- Input 0.5, Output 0.3 = darken midtones

### Vibrance
**Access**: Filter > Color Grading > Vibrance

Smart saturation that protects skin tones and prevents over-saturation.

**Controls**:
- **Vibrance**: Enhancement strength (-1.0 to +1.0)

**Why Vibrance vs Saturation**:
- Less effect on already-saturated colors
- Protects skin tones from becoming unnatural
- More pleasing results for portraits

## Workflow Tips

### Basic Color Correction Workflow
1. **Start with Shadows/Highlights** - Fix exposure issues
2. **Use Color Balance** - Correct white balance
3. **Apply Curves** - Adjust contrast
4. **Finish with Vibrance** - Enhance colors subtly

### Creative Effects
- **Directional Blur** + **Color Balance** = Dynamic motion with color cast
- **Shadows/Highlights** + **Vibrance** = HDR-style look
- **Curves** + **Color Balance** = Film emulation effects

### Performance Notes
- Color grading operations are CPU-intensive
- Work on smaller images first to test settings
- Apply effects incrementally rather than extreme adjustments
- Save your work frequently when stacking multiple effects

## Technical Implementation

### Memory Management
All color grading functions include proper SDL surface cleanup to prevent memory leaks. The implementation follows the same pattern as other filters:

```cpp
// Create surface for pixel manipulation
SDL_Surface* surface = SDL_CreateRGBSurface(...)
// Process pixels
// Clean up regardless of success/failure
SDL_FreeSurface(surface);
```

### Algorithm Details

**Directional Blur**: Samples pixels along a direction vector based on angle and distance parameters.

**Shadows/Highlights**: Uses luminance weighting to determine shadow vs highlight masks, then applies separate adjustments.

**Color Balance**: Direct RGB channel manipulation with clamping to prevent overflow.

**Curves**: Creates lookup table based on input/output points, applies to all channels.

**Vibrance**: Calculates saturation level and applies adjustment with inverse weighting (less effect on already-saturated areas).

## Troubleshooting

### Common Issues

**Color Banding**: Reduce adjustment strength or work with higher bit-depth images.

**Slow Performance**: Use smaller images for testing, apply effects one at a time.

**Unnatural Results**: Try lower adjustment values - subtle changes often look better.

**Memory Issues**: Close other applications, restart editor between heavy operations.

### Limitations

- No real-time preview (by design - keeps UI responsive)
- Limited to 8-bit per channel processing
- No batch processing support
- Single curve point (not full curve editor)

## Future Enhancements

Potential improvements that might be added:

- Multiple curve points for more complex tone mapping
- Real-time preview for smaller images
- Color wheels for more intuitive color balance
- Batch processing for multiple images
- 16-bit processing for professional work

## Integration Notes

These tools integrate seamlessly with the existing layer system:
- All effects apply to the active layer only
- Locked layers are protected from modifications
- Effects can be combined with layer masks for selective application
- Undo/redo system tracks all color grading operations

---

*Remember: Color grading is subjective - trust your eyes and adjust to taste. These tools provide the foundation, but your artistic vision makes the final call.*