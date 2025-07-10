# Text Tool & Layer Management Guide

## Overview

This guide covers the enhanced text tool and layer management system in Enough Image Editor. The text tool has been completely overhauled with professional font management, live preview, and comprehensive styling controls. The layer system now includes intuitive drag-and-drop reordering and enhanced visual controls.

## Enhanced Text Tool

### Getting Started

1. **Select the Text Tool**: Click the "Text" button in the tool panel or press `T`
2. **Create Text Box**: Click and drag on the canvas to create a text bounding box
3. **Edit Text**: The text editor modal will automatically open for new text boxes

### Text Editor Modal Features

#### Main Text Area
- **Multi-line Input**: Full support for paragraphs and line breaks
- **Live Preview**: See your text rendered in real-time as you type
- **Word Wrapping**: Automatic text wrapping within the bounding box

#### Font & Style Controls
- **Font Family Selection**: Choose from available system fonts
- **Font Size**: Adjustable from 8pt to 72pt with slider control
- **Bold & Italic**: Toggle styling options independently
- **Color Picker**: Full RGBA color selection with alpha transparency

#### Position & Size Controls
- **X/Y Position**: Precise positioning with slider controls
- **Width/Height**: Adjustable bounding box dimensions
- **Real-time Updates**: Changes apply immediately to the canvas

#### Action Buttons
- **Done**: Finalizes text and renders to layer
- **Delete**: Removes the text box entirely
- **Live Preview**: Toggle real-time preview on/off

### Font Management

#### Supported Font Formats
- **TTF (TrueType)**: Primary font format support
- **OTF (OpenType)**: Extended font format support
- **System Fonts**: Automatic detection of installed fonts

#### Font Loading Process
1. **Automatic Discovery**: Scans common font directories on startup
2. **Custom Fonts**: Place font files in the `fonts/` directory
3. **Fallback System**: Gracefully falls back to default fonts if custom fonts fail

#### Font Directories Scanned
- `fonts/` (project directory)
- `/usr/share/fonts/` (Linux)
- `/System/Library/Fonts/` (macOS)
- `C:/Windows/Fonts/` (Windows)

### Text Rendering Technical Details

#### Layer Integration
- **Dedicated Layers**: Each text box creates its own layer
- **Layer Naming**: Automatically named "Text 1", "Text 2", etc.
- **Layer Targeting**: Text renders to the correct layer, not just the active one

#### Rendering Pipeline
1. **Font Loading**: Loads font with specified size and style
2. **Surface Creation**: Creates SDL surface with word wrapping
3. **Texture Generation**: Converts surface to GPU texture
4. **Layer Rendering**: Renders texture to specific layer
5. **Cleanup**: Proper resource cleanup to prevent memory leaks

#### Performance Optimizations
- **Font Caching**: Fonts cached by size/style combination
- **Texture Reuse**: Efficient texture creation and destruction
- **Memory Management**: Proper cleanup of SDL resources

## Enhanced Layer Management

### Drag-and-Drop Layer Reordering

#### How to Reorder Layers
1. **Click and Drag**: Click on any layer name and drag it up or down
2. **Visual Feedback**: Drag indicator shows where the layer will be placed
3. **Drop Target**: Release over another layer to complete the move
4. **Active Layer Tracking**: Active layer index updates automatically

#### Visual Indicators
- **Drag Handle**: ">>" symbol indicates draggable layers
- **Drop Zones**: Highlighted areas show valid drop targets
- **Active Layer**: Currently selected layer highlighted in blue

### Layer Controls

#### Visibility Control
- **Eye Icon**: Layer is visible and will render
- **Hidden Icon**: Layer is hidden and won't render
- **Click to Toggle**: Single click switches between visible/hidden

#### Lock Control
- **Unlocked**: Layer can be edited and modified
- **Locked**: Layer is protected from modifications
- **Visual Feedback**: Locked layers have different styling

#### Opacity Control
- **Slider Range**: 0.0 (transparent) to 1.0 (opaque)
- **Real-time Updates**: Changes apply immediately to canvas
- **Precision**: Two decimal place precision for fine control

#### Blend Mode Selection
- **Comprehensive Modes**: Normal, Multiply, Screen, Overlay, and more
- **Dropdown Selection**: Easy access to all blend modes
- **Real-time Preview**: See blend mode effects immediately

### Layer Management Best Practices

#### Organization Tips
- **Logical Naming**: Rename layers to reflect their content
- **Layer Grouping**: Keep related elements on adjacent layers
- **Text Layers**: Keep text on separate layers for easy editing
- **Background Layers**: Place background elements at the bottom

#### Performance Considerations
- **Layer Count**: While unlimited, fewer layers perform better
- **Layer Size**: Large layers consume more memory
- **Blend Modes**: Complex blend modes may slow rendering
- **Opacity**: Fully opaque layers render faster than transparent ones

## Advanced Usage

### Text Tool Workflows

#### Creating Styled Text
1. Create text box with appropriate dimensions
2. Enter your text content
3. Select desired font family
4. Adjust size and styling (bold/italic)
5. Choose color and opacity
6. Fine-tune position and size
7. Click "Done" to finalize

#### Editing Existing Text
1. Click on any text box on the canvas
2. Text editor modal opens automatically
3. Make your changes
4. Click "Done" to apply changes

#### Text Layer Management
- Text boxes create their own layers
- Layer names automatically generated
- Can be reordered like any other layer
- Opacity and blend modes apply to text

### Layer Reordering Strategies

#### Z-Order Management
- **Top Layers**: Render on top, use for foreground elements
- **Bottom Layers**: Render behind, use for backgrounds
- **Middle Layers**: Main content and design elements

#### Drag-and-Drop Tips
- **Smooth Dragging**: Click and hold on layer name, then drag
- **Visual Feedback**: Watch for drop zone indicators
- **Precise Placement**: Drop exactly where you want the layer
- **Undo Support**: Layer moves can be undone with Ctrl+Z

## Troubleshooting

### Common Issues

#### Text Not Rendering
- **Check Layer Visibility**: Ensure text layer is visible
- **Verify Font Loading**: Check if custom font loaded successfully
- **Layer Targeting**: Ensure text renders to correct layer
- **Color Visibility**: Check if text color contrasts with background

#### Font Loading Problems
- **Font Format**: Ensure font file is TTF or OTF format
- **Font Path**: Check if font file is in accessible directory
- **Permissions**: Verify read permissions on font files
- **Fallback**: Default font should load if custom fonts fail

#### Layer Dragging Issues
- **Click Target**: Ensure clicking on layer name, not controls
- **Drag Distance**: Move mouse while holding click button
- **Drop Zone**: Release over valid drop target
- **Layer Count**: Ensure multiple layers exist for reordering

### Performance Tips

#### Optimizing Text Rendering
- **Font Caching**: Reuse fonts with same size/style
- **Texture Management**: Finalize text boxes when done editing
- **Layer Cleanup**: Remove unused text layers
- **Preview Toggle**: Turn off live preview for better performance

#### Layer Management Performance
- **Minimize Layers**: Use fewer layers when possible
- **Optimize Opacity**: Use fully opaque layers when transparency not needed
- **Blend Mode Selection**: Use "Normal" blend mode for best performance
- **Layer Size**: Keep layer dimensions reasonable

## Technical Implementation

### Text Tool Architecture

#### Class Structure
- **TextTool**: Main tool class managing text boxes
- **TextBox**: Individual text element with styling
- **Font Management**: Caching and loading system
- **Layer Integration**: Proper layer targeting and rendering

#### Key Methods
- `createTextBox()`: Creates new text element
- `renderTextBoxToLayer()`: Renders text to specific layer
- `loadAvailableFonts()`: Discovers and loads fonts
- `setFontForTextBox()`: Applies font to specific text

### Layer System Architecture

#### Drag-and-Drop Implementation
- **ImGui Integration**: Uses ImGui drag-drop system
- **Index Management**: Proper layer index tracking
- **Active Layer Updates**: Maintains active layer reference
- **Visual Feedback**: Real-time drag indicators

#### Layer Reordering Logic
- **Move Operation**: Moves layer from source to target index
- **Index Adjustment**: Updates all affected layer indices
- **Active Layer Tracking**: Preserves active layer through moves
- **Bounds Checking**: Validates all layer operations

This enhanced text tool and layer management system provides professional-grade functionality while maintaining ease of use and performance.