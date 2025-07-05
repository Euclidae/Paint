# Paint1 Technical Overview

## Architecture

Enough is a modular image editing application built with SDL2 and Dear ImGui. The codebase is organized into several key components that handle different aspects of the application.

### Core Components

#### Canvas System (`canvas.cpp`, `canvas.hpp`)
The canvas system manages the drawing surface and layer operations. It provides:

- **Layer Management**: Create, duplicate, remove, and reorder layers
- **Rendering Pipeline**: Composites all layers with proper blending modes
- **Texture Management**: Handles SDL texture creation and cleanup
- **Font System**: Caches fonts for efficient text rendering

Key data structures:
```cpp
struct Layer {
    SDL_Texture* texture;
    std::string name;
    float opacity = 1.0f;
    bool visible = true;
    bool locked = false;
    int blendMode = 0;
    // ... other properties
};
```

#### Tools System (`tools.cpp`, `tools.hpp`)
Implements all drawing tools and handles mouse/keyboard input:

- **Drawing Tools**: Pencil, eraser, shapes (rectangle, circle, triangle)
- **Advanced Tools**: Gradient, fill bucket, selection tools
- **Text Tool**: Custom text boxes with font selection
- **Input Handling**: Mouse events, keyboard shortcuts

The text tool uses a separate TextBox structure:
```cpp
struct TextBox {
    std::string content;
    SDL_Rect rect;
    int fontSize;
    bool bold, italic;
    ImVec4 color;
    std::string fontPath;  // Custom font support
};
```

#### Editor System (`editor.cpp`, `editor.hpp`)
Manages editing operations and history:

- **Undo/Redo**: Stack-based history system
- **Selection Operations**: Copy, paste, delete
- **Layer Merging**: Combines multiple layers into one

The undo system uses a simple state structure:
```cpp
struct HistoryState {
    SDL_Texture* texture;
    int layerIndex;
};
```

#### User Interface (`ui.cpp`, `ui.hpp`)
Dear ImGui-based interface providing:

- **Tool Panels**: Tool selection and properties
- **Layer Panel**: Layer management interface
- **Text Editor**: Font selection and text formatting
- **File Operations**: Save/load dialogs

## Key Algorithms

### Flood Fill Algorithm
The flood fill tool uses a queue-based approach instead of recursion to prevent stack overflow:

```cpp
std::queue<std::pair<int, int>> pixels;
std::vector<std::vector<bool>> visited(CANVAS_WIDTH, std::vector<bool>(CANVAS_HEIGHT, false));

while (!pixels.empty() && iterations < maxIterations) {
    std::pair<int, int> p = pixels.front();
    pixels.pop();
    // Process pixel and add neighbors
}
```

**Algorithm Details**: [Flood Fill - Wikipedia](https://en.wikipedia.org/wiki/Flood_fill)

### Font Rendering System
Text rendering combines SDL_ttf with a caching system:

1. **Font Cache**: Stores fonts by size/style combination
2. **Custom Fonts**: Supports TTF/OTF file loading
3. **Style Application**: Bold/italic through TTF_SetFontStyle

### Layer Compositing
Layers are rendered bottom-to-top with blend modes:

```cpp
for (const auto& layer : layers) {
    if (!layer.visible) continue;
    SDL_SetTextureAlphaMod(layer.texture, (Uint8)(layer.opacity * 255));
    // Apply blend mode
    SDL_RenderCopy(renderer, layer.texture, nullptr, nullptr);
}
```

**Blend Modes**: [SDL Blend Modes](https://wiki.libsdl.org/SDL2/SDL_BlendMode)

## Code Organization

### Naming Conventions
The codebase uses mixed naming conventions (intentionally inconsistent):

- **CamelCase**: `applyUndo()`, `saveUndoState()`
- **snake_case**: `save_undo_state()`, `get_text_boxes()`
- **Mixed**: Some methods have both variants for compatibility

### Modern C++ Features
Selective use of C++11/17 features:

- **Auto keyword**: Type deduction where beneficial
- **Range-based loops**: For container iteration
- **Move semantics**: In performance-critical paths
- **Structured bindings**: Limited use in newer code

### Memory Management
- **RAII**: SDL textures wrapped in proper cleanup
- **Smart Pointers**: Minimal use, mostly raw pointers for SDL compatibility
- **Stack Containers**: STL containers for automatic cleanup

## Performance Considerations

### Texture Management
- **Lazy Loading**: Textures created only when needed
- **Caching**: Font textures cached by size/style
- **Cleanup**: Proper SDL_DestroyTexture calls

### Rendering Optimization
- **Target Switching**: Minimal render target changes
- **Batch Operations**: Group similar rendering calls
- **Alpha Blending**: Optimized blend mode selection

### Memory Usage
- **Undo Stack**: Limited to prevent excessive memory usage
- **Layer Limits**: Practical limits on layer count
- **Texture Reuse**: Reuse textures where possible

## Build System

### Dependencies
- **SDL2**: Core graphics and input
- **SDL2_ttf**: Font rendering
- **SDL2_image**: Image loading/saving
- **Dear ImGui**: User interface
- **TinyFileDialogs**: Native file dialogs

### Cross-Platform Support
- **Linux**: Primary development platform
- **Windows**: MinGW compatibility
- **macOS**: Clang support with Homebrew

## Error Handling

### Texture Operations
```cpp
if (!texture) {
    std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    return false;
}
```

### Font Loading
- **Fallback System**: System fonts when custom fonts fail
- **Graceful Degradation**: Skip rendering instead of crashing
- **Error Logging**: SDL/TTF error messages to console

## Thread Safety

The application is single-threaded with all operations on the main thread:
- **SDL Requirement**: SDL rendering must be on main thread
- **ImGui Constraint**: UI operations are not thread-safe
- **Simplicity**: Avoids complex synchronization

## Testing Strategy

### Manual Testing
- **Tool Validation**: Each tool tested with various inputs
- **Memory Leaks**: Valgrind testing on Linux
- **Cross-Platform**: Testing on multiple operating systems

### Error Scenarios
- **Invalid Input**: Out-of-bounds coordinates
- **Memory Pressure**: Large canvas sizes
- **File Operations**: Invalid file paths and formats

## Future Improvements

### Performance
- **GPU Acceleration**: Consider OpenGL/Vulkan backend
- **Multi-threading**: Background operations for file I/O
- **Memory Pooling**: Reduce allocation overhead

### Features
- **Plugin System**: Extensible tool architecture
- **Script Support**: Lua/Python integration
- **Advanced Filters**: Blur, sharpen, color correction

### Code Quality
- **Unit Tests**: Automated testing framework
- **Documentation**: API documentation with Doxygen
- **Static Analysis**: Clang-tidy integration

---

*This documentation covers the core technical aspects of Enough. For specific implementation details, refer to the source code and inline comments.*
