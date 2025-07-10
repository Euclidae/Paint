# OOP Design - From Procedural Mess to Object-Oriented Mess

> [!info] About This Document
> This covers the painful journey from procedural spaghetti code to object-oriented complexity. Written mostly for my own reference to remember why certain design decisions were made and why some of them were terrible.

## Why Refactor to OOP?

### The Original Problem

**April 2025 - Initial Implementation:**
Started with a single `main.cpp` file that grew to ~2000 lines. Everything was global variables and functions. It worked, but...

> [!warning] Procedural Pain Points
> - **Global State Everywhere**: Every function needed access to the same 20+ global variables
> - **Function Parameter Hell**: Passing renderer, canvas, colors, tools to every function
> - **No Encapsulation**: Data and functions scattered everywhere
> - **Testing Impossible**: Couldn't test individual components
> - **Code Duplication**: Same logic repeated in multiple places

**The Breaking Point:**
Adding the text tool required passing 8 different parameters to every text-related function. That's when I decided enough was enough.

**External Inspiration:**
- [Game Programming Patterns](http://gameprogrammingpatterns.com/) - Especially the singleton and state patterns
- [Effective C++](https://www.aristeia.com/books.html) by Scott Meyers - RAII and object lifetime management
- [Clean Code](https://www.amazon.com/Clean-Code-Handbook-Software-Craftsmanship/dp/0132350882) - Though I probably didn't follow it well enough

## Design Patterns Used

### Singleton Pattern (Overused)

> [!example] Pattern Implementation
> **Why Used:** Needed global access without global variables  
> **Result:** Probably used it too much

**Classes That Are Singletons:**
```cpp
class Canvas {
public:
    static Canvas& getInstance() {
        static Canvas instance;
        return instance;
    }
private:
    Canvas() = default;  // Private constructor
};

// Similar pattern for:
// - ToolManager
// - Editor  
// - UI
```

**Why Singletons:**
- Needed single instance of each core system
- Global access without global variables
- Initialization order control

**Problems Created:**
- Hard to test (can't mock singletons easily)
- Tight coupling between classes
- Global state still exists, just hidden
- Threading issues (though this is single-threaded)

**Better Alternatives (That I Didn't Use):**
- Dependency injection
- Service locator pattern
- Simple static instances passed as parameters

**External Resources:**
- [Singleton Pattern - Design Patterns](https://refactoring.guru/design-patterns/singleton)
- [Why Singletons Are Controversial](https://stackoverflow.com/questions/137975/what-is-so-bad-about-singletons)

### Strategy Pattern (Tool System)

> [!success] Pattern Success
> **Status:** Actually worked well  
> **Use Case:** Different drawing tools with common interface

**Base Tool Class:**
```cpp
class Tool {
public:
    virtual ~Tool() = default;
    
    // Pure virtual interface
    virtual void handleMouseDown(const SDL_Event& event) = 0;
    virtual void handleMouseMove(const SDL_Event& event) = 0;
    virtual void handleMouseUp(const SDL_Event& event) = 0;
    virtual const char* getName() const = 0;
    
    // Common functionality
    virtual void setColor(const ImVec4& color) { m_color = color; }
    virtual void setSize(int size) { m_size = size; }
    
protected:
    ImVec4 m_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    int m_size = 1;
    bool m_isDrawing = false;
};
```

**Derived Tool Classes:**
```cpp
class PencilTool : public Tool {
public:
    void handleMouseDown(const SDL_Event& event) override {
        m_isDrawing = true;
        // Pencil-specific logic
    }
    const char* getName() const override { return "Pencil"; }
};

class EraserTool : public Tool {
    // Similar pattern...
};
```

**Why This Worked:**
- Clean separation of tool logic
- Easy to add new tools
- Polymorphism works naturally
- Tool switching is simple

**Tool Registration:**
```cpp
void ToolManager::init() {
    m_tools.push_back(std::make_unique<PencilTool>());
    m_tools.push_back(std::make_unique<EraserTool>());
    m_tools.push_back(std::make_unique<LineTool>());
    // etc...
}
```

**Related:** [[TECHNICAL_OVERVIEW#Tool System]]

### Command Pattern (Undo/Redo System)

> [!warning] Pattern Problems
> **Status:** Poorly implemented  
> **Issue:** Not a proper command pattern

**What I Actually Did:**
```cpp
class HistoryState {
private:
    SDL_Texture* m_texture;  // Full texture copy
    int m_layerIndex;
    
public:
    HistoryState(SDL_Texture* texture, int layerIndex);
    // ...
};

class Editor {
private:
    std::stack<HistoryState> m_undoStack;
    std::stack<HistoryState> m_redoStack;
};
```

**Problems:**
- Not actually command pattern - just state snapshots
- Memory intensive (full texture copies)
- No granular operations
- Can't optimize similar operations

**What Real Command Pattern Would Look Like:**
```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

class PaintCommand : public Command {
private:
    SDL_Point m_point;
    ImVec4 m_color;
    int m_brushSize;
    
public:
    void execute() override {
        // Apply paint stroke
    }
    void undo() override {
        // Remove paint stroke
    }
};
```

**Why I Didn't:** Too complex for the time I had, and texture snapshots were simpler to implement.

**External Resources:**
- [Command Pattern Explained](https://refactoring.guru/design-patterns/command)
- [Undo/Redo with Command Pattern](https://www.codeproject.com/Articles/33384/Multilevel-Undo-and-Redo-Implementation-in-C)

### Observer Pattern (Missing)

> [!note] Pattern Not Used
> **Should Have Implemented:** UI updates when canvas changes  
> **Reality:** Direct coupling everywhere

**What Should Have Been:**
```cpp
class Observable {
    std::vector<Observer*> observers;
public:
    void addObserver(Observer* obs) { observers.push_back(obs); }
    void notifyObservers() {
        for (auto obs : observers) {
            obs->update();
        }
    }
};

class Canvas : public Observable {
    void addLayer() {
        // Add layer logic
        notifyObservers();  // UI updates automatically
    }
};
```

**What I Actually Did:**
```cpp
// Direct coupling everywhere
canvas.addLayer();
ui.updateLayerPanel();  // Manual call
ui.updateCanvas();      // Another manual call
```

**Result:** UI updates scattered throughout the codebase, easy to forget calls.

## Class Architecture

### Core Classes Overview

```
Singletons:
├── Canvas          - Drawing surface and layers
├── ToolManager     - Tool selection and management  
├── Editor          - Undo/redo and editing operations
└── UI              - Dear ImGui interface

Supporting Classes:
├── Tool (abstract) - Base for all drawing tools
├── Layer           - Individual layer data
├── HistoryState    - Undo/redo state storage
└── TextBox         - Text tool data
```

### Canvas Class Design

> [!example] Core Component
> **Responsibility:** Manages drawing surface and all layers

**Key Design Decisions:**

**Layer Management:**
```cpp
class Canvas {
private:
    std::vector<std::unique_ptr<Layer>> m_layers;
    int m_activeLayerIndex = 0;
    
public:
    void addLayer(const std::string& name = "New Layer");
    void removeLayer(int index);
    Layer* getActiveLayer();
    void setActiveLayerIndex(int index);
};
```

**Why `unique_ptr`:**
- Automatic memory management
- Move semantics for efficiency
- Clear ownership semantics

**Filter System:**
```cpp
class Canvas {
private:
    SDL_Texture* m_filterBuffer = nullptr;
    bool m_filterInProgress = false;
    
    void createFilterBuffer();
    void applyFilterBuffer();
    void cleanupFilterBuffer();
};
```

**Buffer System Rationale:**
- Prevents filter stacking crashes
- Allows preview before applying
- Safer memory management

**Problems:**
- Too many responsibilities in one class
- Tight coupling with UI
- Hard to test filter operations

**Related:** [[TECHNICAL_OVERVIEW#Core Architecture]]

### Layer Class Design

> [!info] Data Encapsulation
> **Goal:** Encapsulate layer properties and operations

```cpp
class Layer {
private:
    SDL_Texture* m_texture = nullptr;
    std::string m_name;
    float m_opacity = 1.0f;
    bool m_visible = true;
    bool m_locked = false;
    BlendMode m_blendMode = BlendMode::Normal;
    
public:
    Layer(SDL_Renderer* renderer, int width, int height, const std::string& name);
    ~Layer();
    
    // Proper RAII
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;
    Layer(Layer&& other) noexcept;
    Layer& operator=(Layer&& other) noexcept;
    
    // Accessors
    SDL_Texture* getTexture() const { return m_texture; }
    const std::string& getName() const { return m_name; }
    bool isVisible() const { return m_visible; }
    bool isLocked() const { return m_locked; }
};
```

**RAII Implementation:**
- Constructor creates SDL texture
- Destructor cleans up texture
- Move semantics for efficiency
- Copy disabled (textures aren't copyable)

**Why This Worked:**
- Clear ownership of texture resource
- No memory leaks from layers
- Simple interface for layer operations

### ToolManager Class Design

> [!success] Pattern Success
> **Status:** One of the better-designed classes

```cpp
class ToolManager {
private:
    std::vector<std::unique_ptr<Tool>> m_tools;
    Tool* m_currentTool = nullptr;
    int m_currentToolIndex = 0;
    
    ImVec4 m_primaryColor = ImVec4(0, 0, 0, 1);
    ImVec4 m_secondaryColor = ImVec4(1, 1, 1, 1);
    
public:
    void setCurrentTool(int toolIndex);
    Tool* getCurrentTool() const { return m_currentTool; }
    
    void handleSDLEvent(const SDL_Event& event);
    void setPrimaryColor(const ImVec4& color);
};
```

**Design Benefits:**
- Clean tool switching
- Color management centralized
- Event routing to active tool
- Easy to add new tools

**Tool Ownership:**
- ToolManager owns all tool instances
- Tools created once and reused
- Current tool is just a pointer (not owned)

## Memory Management

### RAII Principles Applied

> [!tip] Resource Management
> **Goal:** Automatic resource cleanup  
> **Result:** Mostly successful

**Layer Texture Management:**
```cpp
Layer::Layer(SDL_Renderer* renderer, int width, int height, const std::string& name)
    : m_name(name) {
    m_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height
    );
    // Check for null...
}

Layer::~Layer() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
}
```

**Smart Pointer Usage:**
```cpp
// In Canvas class:
std::vector<std::unique_ptr<Layer>> m_layers;

// Adding layers:
void Canvas::addLayer(const std::string& name) {
    auto layer = std::make_unique<Layer>(m_renderer, m_width, m_height, name);
    m_layers.push_back(std::move(layer));
}
```

**Why `unique_ptr`:**
- Automatic cleanup when vector is destroyed
- Move semantics for efficiency
- Can't accidentally copy layers
- Clear ownership semantics

### Memory Leak Issues

> [!bug] Persistent Problems
> **Status:** Never fully resolved  
> **Area:** Filter operations and temporary textures

**Common Leak Sources:**
```cpp
// This pattern was problematic:
SDL_Texture* temp = SDL_CreateTexture(...);
// Do some processing...
if (error_condition) {
    return;  // LEAK: temp texture never destroyed
}
SDL_DestroyTexture(temp);
```

**Attempted Fix:**
```cpp
// Better pattern with RAII wrapper:
class TextureWrapper {
    SDL_Texture* m_texture;
public:
    TextureWrapper(SDL_Texture* tex) : m_texture(tex) {}
    ~TextureWrapper() { 
        if (m_texture) SDL_DestroyTexture(m_texture); 
    }
    SDL_Texture* get() const { return m_texture; }
};
```

**Reality:** Never implemented this consistently throughout the codebase.

**External Resources:**
- [RAII in C++](https://en.cppreference.com/w/cpp/language/raii)
- [Smart Pointers Best Practices](https://docs.microsoft.com/en-us/cpp/cpp/smart-pointers-modern-cpp)

## What Worked Well

### Tool System Success

> [!success] Design Win
> **Result:** Easy to add new tools, clean separation of concerns

**Adding New Tools:**
1. Create class inheriting from `Tool`
2. Implement virtual methods
3. Add to `ToolManager::init()`
4. UI automatically includes it

**Example - Clone Stamp Tool:**
```cpp
class CloneStampTool : public Tool {
private:
    bool m_hasSourcePoint = false;
    SDL_Point m_sourcePoint;
    
public:
    void handleMouseDown(const SDL_Event& event) override {
        if (/* Alt key held */) {
            setSourcePoint(event.button.x, event.button.y);
        } else {
            cloneAt(event.button.x, event.button.y);
        }
    }
    const char* getName() const override { return "Clone Stamp"; }
};
```

**Why It Worked:**
- Clear interface contract
- Each tool encapsulates its own logic
- No dependencies between tools
- Polymorphism handles tool switching

### Layer System Design

> [!success] Another Win
> **Result:** Clean layer management, good encapsulation

**Benefits:**
- Each layer owns its texture
- Clear lifetime management
- Easy to add layer properties
- Vector container handles ordering

**Layer Operations:**
```cpp
// Adding layers is simple:
canvas.addLayer("Background");

// Layer properties are encapsulated:
layer->setOpacity(0.5f);
layer->setVisible(false);
layer->setBlendMode(BlendMode::Multiply);
```

## What Didn't Work

### Singleton Overuse

> [!warning] Design Problem
> **Issue:** Too much global state, just hidden behind singletons

**Problems Created:**
- Testing became harder (can't mock singletons)
- Initialization order issues
- Hidden dependencies between classes
- Still have global state, just disguised

**Example of Tight Coupling:**
```cpp
// In Tool classes:
Canvas& canvas = GetCanvas();  // Hidden dependency
Editor& editor = GetEditor();  // Another hidden dependency

// Tools are tightly coupled to these singletons
```

**Better Approach Would Be:**
```cpp
class Tool {
protected:
    Canvas* m_canvas;
    Editor* m_editor;
    
public:
    Tool(Canvas* canvas, Editor* editor) 
        : m_canvas(canvas), m_editor(editor) {}
    // Now dependencies are explicit
};
```

### Undo/Redo System

> [!bug] Design Failure
> **Issue:** Not a proper command pattern, memory intensive

**Problems:**
- Stores full texture copies (memory expensive)
- No operation granularity
- Can't optimize sequential operations
- Limited history size due to memory

**Memory Usage:**
```cpp
// For 1920x1080 RGBA image:
// 1920 * 1080 * 4 bytes = ~8MB per undo state
// With 50 states = ~400MB just for undo history
```

**What Should Have Been Done:**
- Proper command pattern with incremental operations
- Delta compression for texture changes
- Tile-based undo for large images

### Filter System Architecture

> [!warning] Overcomplicated
> **Issue:** Buffer system was a hack to fix fundamental problems

**The Hack:**
```cpp
// This pattern everywhere:
createFilterBuffer();    // Create temp texture
// Apply filter to buffer
applyFilterBuffer();     // Replace original
cleanupFilterBuffer();   // Clean up temp
```

**Problems:**
- Double memory usage during filtering
- Complex state management
- Still didn't fix all crashes
- Made filter code harder to understand

**Root Issue:** Should have designed proper texture ownership from the start.

## Lessons Learned

### Design Patterns Are Tools, Not Goals

> [!note] Key Insight
> **Lesson:** Don't use patterns just because you can

**What I Did Wrong:**
- Used Singleton pattern everywhere because it was easy
- Over-engineered simple problems
- Added complexity without clear benefits

**What I Should Have Done:**
- Started with simple, direct solutions
- Added patterns only when complexity justified them
- Focused on solving actual problems, not applying patterns

### RAII Is Crucial for Graphics Programming

> [!success] Important Learning
> **Result:** Automatic resource management prevented many leaks

**What Worked:**
- Layer textures managed automatically
- Stack-based objects cleaned up properly
- Smart pointers for dynamic allocation

**What Didn't:**
- Temporary textures in filter operations
- Complex ownership scenarios
- Error handling paths

### Testing Should Drive Design

> [!warning] Missing Foundation
> **Problem:** No tests meant design flaws went unnoticed

**Issues Caused:**
- Tight coupling discovered too late
- Memory leaks found only in testing
- Hard to refactor safely

**If I Started Over:**
- Write tests first for core functionality
- Design for testability from the beginning
- Use dependency injection instead of singletons

### Incremental Refactoring Is Better

> [!info] Process Learning
> **Mistake:** Tried to refactor everything at once

**What Happened:**
- Massive rewrite took months
- Lost working features during transition
- Hard to track what broke when

**Better Approach:**
- Extract one class at a time
- Keep old code working alongside new
- Migrate functionality gradually

## External Resources That Helped

### Books
- [Game Programming Patterns](http://gameprogrammingpatterns.com/) by Robert Nystrom - Free online, excellent for understanding when to use patterns
- [Effective C++](https://www.aristeia.com/books.html) by Scott Meyers - RAII and resource management
- [Clean Code](https://www.amazon.com/Clean-Code-Handbook-Software-Craftsmanship/dp/0132350882) by Robert Martin - Code organization principles

### Online Resources
- [Refactoring Guru](https://refactoring.guru/design-patterns) - Best design pattern explanations I found
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - Modern C++ best practices
- [SDL2 Documentation](https://wiki.libsdl.org/SDL2/FrontPage) - Essential for understanding texture management

### Courses
- **Vikash Kumar's Udemy Course** - Provided the initial structure I built upon
- **John Purcell's SDL2 Tutorials** - Essential for understanding SDL2 architecture and patterns

## Related Documentation

- [[TECHNICAL_OVERVIEW]] - Current architecture overview
- [[TROUBLESHOOTING]] - Issues caused by design decisions
- [[REFACTORING_REPORT]] - Detailed refactoring process
- [[new_features]] - Latest changes to the architecture
- [[CHANGES]] - How the design evolved over time

## Final Thoughts

> [!quote] Honest Assessment
> The OOP refactor made the code more organized but also more complex. Some parts benefited (tool system, layer management), others became overengineered (singleton usage, filter system).

**What I'd Do Differently:**
1. Start with simpler architecture
2. Write tests from the beginning
3. Use dependency injection instead of singletons
4. Implement proper command pattern for undo/redo
5. Focus on solving real problems, not applying patterns

**What Worked:**
- Tool inheritance hierarchy
- Layer encapsulation
- RAII for resource management
- Clear separation of concerns

**What Didn't:**
- Singleton overuse
- Complex filter buffer system
- Tight coupling between components
- Over-engineered undo system

**For Future Projects:**
- Keep it simple initially
- Add complexity only when needed
- Design for testability
- Use established patterns correctly

The refactoring taught me a lot about both good and bad uses of OOP principles. While the final result wasn't perfect, it was a valuable learning experience in software architecture and design patterns.