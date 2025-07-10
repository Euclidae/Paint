# Technical Overview - Enough Image Editor

> [!info] About This Document
> Technical breakdown of the core architecture and systems. Written primarily for my own reference but might be useful if you're trying to understand how this thing works.

## Core Architecture

### High-Level Design

The project follows a **singleton-heavy OOP architecture** with four main components:

- **Canvas** - Manages the drawing surface and layers
- **ToolManager** - Handles all drawing tools and their interactions
- **Editor** - Manages undo/redo, selections, and file operations
- **UI** - Handles the Dear ImGui interface

**Why singletons everywhere?** Because I needed global access and didn't want to pass references around everywhere. Probably not the best design decision, but it works.

### Class Hierarchy

```
Tool (base class)
├── PencilTool
├── EraserTool
├── LineTool
├── RectangleTool
├── CircleTool
├── TriangleTool
├── FillTool
├── SelectionTool
├── FloodSelectionTool
├── TextTool
├── GradientTool
├── HealingTool
└── CloneStampTool
```

**Layer System:**
- `Layer` class encapsulates texture, properties, and metadata
- Canvas manages a vector of `unique_ptr<Layer>`
- Each layer has its own SDL texture for rendering

**Related:** [[OOP_DESIGN]] - Details on the refactoring process

## SDL2 + Dear ImGui Integration

### Rendering Pipeline

1. **SDL2** handles the main window and renderer
2. **Dear ImGui** renders the UI panels on top
3. **Canvas** renders to its own texture buffer
4. **Tools** draw directly to layer textures
5. **Final composite** combines everything

### Event Handling

```cpp
// Main event loop pattern
while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    
    if (!ImGui::GetIO().WantCaptureMouse) {
        // Only handle canvas events if ImGui isn't using the mouse
        canvas.handleEvents(event);
        toolManager.handleEvents(event);
    }
}
```

**Key Issue:** Event handling priority was a nightmare to get right. ImGui needs to capture events first, then canvas, then tools.

### Memory Management

**SDL Textures:**
- Each layer owns its texture
- Canvas has a main buffer texture
- Filter operations use temporary buffer textures
- **Problem:** Texture leaks were common, especially in filter operations

**RAII Pattern:**
```cpp
class Layer {
    SDL_Texture* m_texture = nullptr;
    
public:
    ~Layer() {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
        }
    }
};
```

**Related:** [[TROUBLESHOOTING#Memory Issues]]

## Tool System

### Base Tool Class

```cpp
class Tool {
public:
    virtual void handleMouseDown(const SDL_Event& event) = 0;
    virtual void handleMouseMove(const SDL_Event& event) = 0;
    virtual void handleMouseUp(const SDL_Event& event) = 0;
    virtual void render(SDL_Renderer* renderer) {}
    virtual const char* getName() const = 0;
};
```

### Tool Registration

Tools are registered in `ToolManager::init()`:
```cpp
void ToolManager::init() {
    m_tools.push_back(std::make_unique<PencilTool>());
    m_tools.push_back(std::make_unique<EraserTool>());
    // ... etc
}
```

**Tool Selection:**
- UI calls `setCurrentTool(index)`
- Current tool receives all mouse events
- Only one tool active at a time

### Tool State Management

**Color System:**
- Primary/secondary colors stored in ToolManager
- Tools inherit colors from manager
- Color swapping with X key

**Brush Settings:**
- Size, opacity, hardness stored per-tool-type
- Settings persist between tool switches
- Some tools ignore certain settings

**Related:** [[TEXT_TOOL_GUIDE]], [[SDL_IMGUI_FUNCTIONS_REFERENCE#Tool System]]

## Layer System

### Layer Properties

```cpp
class Layer {
    SDL_Texture* m_texture;
    std::string m_name;
    float m_opacity = 1.0f;
    bool m_visible = true;
    bool m_locked = false;
    BlendMode m_blendMode = BlendMode::Normal;
    SDL_Rect m_bounds;
};
```

### Layer Operations

**Creation:**
- `Canvas::addLayer()` creates new layer
- Default size matches canvas dimensions
- Each layer gets its own SDL texture

**Rendering:**
- Layers rendered bottom-to-top
- Blend modes applied during composite
- Opacity and visibility checked per layer

**Transform:**
- Move, scale, rotate operations
- Transform box with handles
- **Issue:** Transform system is fundamentally broken

### Blend Modes

Currently supported:
- Normal
- Multiply
- Screen
- Overlay
- **Reality:** Most blend modes don't work correctly

**Related:** [[TROUBLESHOOTING#Layer Issues]]

## Filter System

### Filter Architecture

**Buffer System:**
```cpp
void Canvas::createFilterBuffer() {
    m_filterBuffer = SDL_CreateTexture(
        m_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        m_width, m_height
    );
}
```

**Filter Pattern:**
1. Create buffer texture
2. Render layer to buffer
3. Apply filter to buffer
4. Replace layer texture with buffer
5. Clean up

### Available Filters

**Basic Filters:**
- Grayscale - Simple RGB to luminance conversion
- Blur - Box blur with adjustable radius
- Sharpen - Unsharp mask convolution
- Edge Detection - Sobel operator implementation

**Advanced Filters:**
- Color grading suite
- Directional blur
- Shadows/highlights adjustment

**Filter Stacking Issue:**
Applying multiple filters in sequence often causes segfaults. The buffer system was supposed to fix this but didn't work completely.

**Related:** [[COLOR_GRADING]], [[TROUBLESHOOTING#Filter Stacking Issues]]

## File System

### Supported Formats

**Import:**
- PNG, JPG, BMP via SDL2_image
- Automatic format detection
- Proper alpha channel handling

**Export:**
- PNG with transparency
- JPG with quality settings
- BMP for compatibility

### File Operations

**Import Process:**
1. Load image with `IMG_Load()`
2. Convert to RGBA format
3. Create new layer
4. Copy image data to layer texture
5. Add to recent files list

**Export Process:**
1. Create composite surface
2. Render all visible layers
3. Save with appropriate format
4. Add to recent files list

**Recent Files:**
- Stored in `.enough_recent_files` config
- Simple text format, one path per line
- Maximum 10 files tracked

**Related:** [[new_features#Recent Files System]]

## UI System

### Dear ImGui Integration

**Window Layout:**
- Main menu bar
- Tool palette (left)
- Layer panel (right)
- Properties panel (bottom right)
- Status bar (bottom)

**UI State Management:**
- UI class manages all ImGui windows
- State stored in UI singleton
- Updates synchronized with main loop

### Tool-Specific UI

**Dynamic Properties:**
- Different tools show different properties
- Color picker for paint tools
- Size slider for brushes
- Font selector for text tool

**Text Tool Modal:**
- Separate window for text editing
- Font selection dropdown
- Size and style controls
- **Issue:** Font loading is unreliable

**Related:** [[TEXT_TOOL_GUIDE#UI Integration]]

## Memory Management

### SDL Resource Management

**Texture Lifecycle:**
```cpp
// Creation
SDL_Texture* texture = SDL_CreateTexture(...);

// Usage
SDL_RenderCopy(renderer, texture, src, dst);

// Cleanup (in destructor)
SDL_DestroyTexture(texture);
```

**Common Leak Sources:**
- Forgot to destroy temporary textures
- Filter operations creating orphaned textures
- Undo system accumulating textures

### RAII Implementation

**Smart Pointers:**
- `unique_ptr` for layers
- `unique_ptr` for tools
- Raw pointers for SDL resources (had to)

**Stack-Based Cleanup:**
- Most objects use stack allocation
- Destructors handle cleanup
- **Issue:** SDL resources still need manual cleanup

**Related:** [[TROUBLESHOOTING#Memory Issues]]

## Threading Model

**Single-Threaded:**
- Main thread handles everything
- UI, rendering, and processing all on main thread
- **Why:** SDL2 and ImGui are not thread-safe

**Performance Implications:**
- Large image operations block UI
- Filter processing can cause frame drops
- **Workaround:** None implemented

## Build System

### CMake Configuration

**C++20 Standard:**
```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

**Dependencies:**
- SDL2, SDL2_ttf, SDL2_image
- Dear ImGui (included)
- TinyFileDialogs (included)

**Platform Differences:**
- Windows: MSYS2 toolchain
- Linux: System package manager
- macOS: Homebrew

### Makefile Alternative

**Simple Build:**
```makefile
CXXFLAGS = -Wall -Wextra -g -std=c++20
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image -lpthread -lGL
```

**Dependency Checking:**
- Automatic detection of SDL2 libraries
- Fallback to pkg-config if needed
- Cross-platform compatibility

## Known Limitations

### Architecture Issues

**Singleton Overuse:**
- Global state makes testing difficult
- Tight coupling between components
- **Fix:** Would require major refactoring

**Event System:**
- No proper event queue
- Direct coupling between input and actions
- **Fix:** Implement command pattern

### Performance Problems

**Large Images:**
- Memory usage scales poorly
- No tile-based rendering
- **Workaround:** Limit image size

**Filter Operations:**
- Synchronous processing blocks UI
- No progress indication
- **Fix:** Would need threading

### Compatibility Issues

**Font Loading:**
- Platform-specific font paths
- Inconsistent font rendering
- **Status:** Partially resolved

**File Paths:**
- Windows vs Unix path separators
- Unicode filename support
- **Status:** Mostly works

**Related:** [[TROUBLESHOOTING]] - Complete list of known issues

## Development History

### Original Architecture (April 2025)

**Procedural Design:**
- Single main.cpp file
- Global variables for state
- Functions for each operation
- **Problem:** Became unmaintainable

### OOP Refactor (May-July 2025)

**Class Extraction:**
- Canvas functionality → Canvas class
- Tool functions → Tool hierarchy
- UI code → UI class
- **Result:** More organized but more complex

### Final Updates (July 11, 2025)

**Feature Additions:**
- Clone stamp tool
- Sharpen filter
- Recent files system
- **Status:** Last changes before abandoning project on July 11, 2025

**Related:** [[REFACTORING_REPORT]] - Detailed refactoring process

## Future Improvements (Never Happening)

### Architecture

**Better Design Patterns:**
- Replace singletons with dependency injection
- Implement proper observer pattern
- Add command pattern for undo/redo

**Threading:**
- Background processing for filters
- Async file operations
- UI responsiveness improvements

### Performance

**Memory Optimization:**
- Tile-based rendering for large images
- Texture streaming
- Better memory pooling

**Rendering:**
- GPU-accelerated filters
- Shader-based effects
- Real-time preview

### Features

**Advanced Tools:**
- Vector layer support
- Non-destructive editing
- Batch processing

**File Format Support:**
- PSD import/export
- RAW image support
- Animation support

## Implementation Notes

### Code Style

**Naming Conventions:**
- Classes: PascalCase
- Members: m_camelCase
- Functions: camelCase
- Constants: UPPER_SNAKE_CASE

**Error Handling:**
- Return bool for success/failure
- Print errors to stderr
- Graceful degradation where possible

**Memory Safety:**
- RAII for resource management
- Prefer stack allocation
- Check pointers before use

### Documentation

**Comment Style:**
- Brief explanations for complex logic
- TODO/FIXME/HACK markers
- Avoid obvious comments

**API Documentation:**
- Function signatures are self-documenting
- Class headers explain purpose
- **Reality:** Documentation is incomplete

## Related Documentation

- [[OOP_DESIGN]] - Object-oriented refactoring details
- [[REFACTORING_REPORT]] - Development journey
- [[TROUBLESHOOTING]] - Known issues and workarounds
- [[new_features]] - Latest feature implementations
- [[COLOR_GRADING]] - Image processing algorithms
- [[TEXT_TOOL_GUIDE]] - Font system implementation
- [[SDL_IMGUI_FUNCTIONS_REFERENCE]] - API reference
- [[CHANGES]] - Complete changelog

## Final Thoughts

This project taught me a lot about graphics programming, C++, and software architecture. The codebase is far from perfect, but it's a decent example of how to structure a medium-sized C++ application with SDL2 and Dear ImGui.

**Key Takeaways:**
- Start with simpler architecture
- Don't over-engineer early
- Testing is crucial for complex systems
- Graphics programming is harder than it looks

**For Future Projects:**
- Limit scope more aggressively
- Write tests from the beginning
- Use established patterns, not custom solutions
- Plan for performance from the start

**Archive Status:** This project is complete and no longer maintained. Use it as a learning resource or starting point for your own image editor.