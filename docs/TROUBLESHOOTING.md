# Troubleshooting Guide

> [!warning] Project Status
> This project is no longer maintained. These are the known issues I encountered before giving up on it. Some have workarounds, others are fundamental problems I couldn't solve.

## Critical Issues

### Selection Tool Broken

> [!bug] Severity: Critical
> **Status:** Unfixable (tried multiple times)  
> **Affects:** All selection operations  
> **Related:** [[new_features#Selection Tool Overhaul]]

**Problem:** The selection tool is fundamentally broken despite multiple rewrite attempts.

**Symptoms:**
- Left click doesn't properly select objects
- Shift+left click scaling is unpredictable
- Transform box appears in wrong positions
- Selection rect calculations are off

**What I tried:**
- Complete rewrite of selection logic
- Separated move/scale into different code paths
- Added proper keyboard state detection
- Improved bounds checking

**Result:** Still doesn't work properly. The flood select has a related issue where it changes colors when you delete selections.

**Workaround:** Use the flood selection tool instead, but be aware of the color change bug.

**Code Location:** `tools/ToolManager.cpp` lines 1051-1178

### Filter Stacking Segfaults

> [!error] Severity: Critical
> **Status:** Partially fixed with buffer system  
> **Affects:** Grayscale + Blur combination especially

**Problem:** Applying multiple filters in sequence causes segmentation faults.

**Root Cause:** Texture ownership issues and improper cleanup between filter operations.

**Specific Combinations That Crash:**
- Grayscale → Blur
- Any filter → Edge Detection
- Multiple blur applications

**Attempted Fix:** Implemented buffer system to prevent direct texture manipulation:
```cpp
// HACK: Buffer image system to prevent filter stacking segfaults
void Canvas::createFilterBuffer();
void Canvas::applyFilterBuffer();
void Canvas::cleanupFilterBuffer();
```

**Current Status:** Reduced crashes but didn't eliminate them completely.

**Workaround:** 
1. Save your work before applying filters
2. Apply filters one at a time
3. Test on a duplicate layer first

**External Resources:**
- [SDL2 Texture Management Best Practices](https://wiki.libsdl.org/SDL2/CategoryRender)
- [Memory Management in Graphics Programming](https://www.khronos.org/opengl/wiki/Common_Mistakes#Resource_Management)

### Memory Leaks in Filter Operations

> [!warning] Severity: High
> **Status:** Partially resolved  
> **Affects:** Long editing sessions

**Problem:** Filter operations don't properly clean up temporary textures.

**Symptoms:**
- Memory usage increases over time
- Application becomes sluggish
- Eventually crashes on large images

**Code Issues:**
```cpp
// This pattern was problematic:
SDL_Texture* temp = SDL_CreateTexture(...);
// Filter processing...
// Missing: SDL_DestroyTexture(temp);
```

**Fix Implemented:** Added proper RAII patterns and cleanup calls.

**Monitoring:** Use system monitor to watch memory usage during filter operations.

**Related:** [[TECHNICAL_OVERVIEW#Memory Management]]

### Clone Stamp Tool Failure

> [!bug] Severity: High
> **Status:** Completely broken  
> **Affects:** Clone stamp functionality
> **Related:** [[new_features#Clone Stamp Tool]]

**Problem:** Clone stamp tool was implemented but doesn't actually work.

**What's Broken:**
- Source point detection is unreliable
- Pixel copying algorithm doesn't transfer data properly
- Visual feedback (crosshair) appears but no cloning occurs
- Brush blending implementation is fundamentally flawed

**Code Issues:**
```cpp
// This was supposed to work but doesn't:
void CloneStampTool::cloneAt(int x, int y) {
    // Complex offset calculations that don't work
    // SDL_RenderCopy calls that fail silently
    // Texture manipulation that corrupts data
}
```

**Attempted Fixes:**
- Tried different pixel copying approaches
- Attempted SDL texture manipulation
- Added debug output (showed wrong coordinates)

**Result:** Spent hours debugging, never got it working properly.

**Workaround:** None - feature is completely non-functional.

### Recent Files System Broken

> [!error] Severity: Medium
> **Status:** File I/O fails silently  
> **Affects:** File management convenience

**Problem:** Recent files tracking doesn't work despite implementation.

**Symptoms:**
- `.enough_recent_files` config file not created
- File paths not properly saved to config
- Loading recent files on startup fails
- No error messages or feedback

**Code Problems:**
```cpp
// This fails silently:
FILE* file = fopen(configFile.c_str(), "w");
if (!file) return;  // Fails here but no indication to user
```

**Platform Issues:**
- File permissions problems on some systems
- Path handling inconsistent across platforms
- Unicode filenames not supported properly

**Status:** Implemented but completely non-functional.

### Keyboard Shortcuts Inconsistent

> [!warning] Severity: Medium  
> **Status:** Hit-or-miss functionality  
> **Affects:** User workflow efficiency

**Problem:** Keyboard shortcuts work sporadically and conflict with each other.

**What Works Sometimes:**
- Ctrl+Z (undo) - works about 70% of the time
- Ctrl+Y (redo) - occasionally responds
- Number keys (1-9) for tool switching

**What's Completely Broken:**
- Ctrl+C/V (copy/paste) - never works
- Ctrl+S (save) - inconsistent, sometimes triggers twice
- Ctrl+O (open) - conflicts with ImGui input
- Tool shortcuts interfere with text input fields

**Root Cause:**
Event handling priority between ImGui and SDL2 is messed up:
```cpp
// This logic is flawed:
if (!ImGui::GetIO().WantCaptureMouse) {
    // Handle canvas events
} 
// But keyboard events get mixed up
```

**Platform Differences:**
- Windows: Modifier keys sometimes stuck
- Linux: Works better but still inconsistent  
- macOS: Different key combinations expected

**Workaround:** Use menu items instead of shortcuts.

## Major Issues

### Font Loading Inconsistencies

> [!warning] Severity: Medium
> **Status:** Workaround available  
> **Affects:** Text tool functionality

**Problem:** Font loading behaves differently across platforms.

**Platform-Specific Issues:**

**Windows:**
- Font paths use backslashes
- System fonts in `C:\Windows\Fonts\`
- TTF files sometimes not recognized

**Linux:**
- Font paths vary by distribution
- Fontconfig integration inconsistent
- Permission issues with system fonts

**macOS:**
- Different font directory structure
- Font registration required
- Bundle vs system fonts confusion

**Workaround:** 
1. Use bundled fonts in `fonts/` directory
2. Provide full paths to font files
3. Test on target platform before release

**Code Location:** `tools/ToolManager.cpp` TextTool implementation

**External Resources:**
- [SDL_ttf Documentation](https://wiki.libsdl.org/SDL2_ttf/FrontPage)
- [Cross-Platform Font Handling](https://stackoverflow.com/questions/10542832/how-to-use-fontconfig-to-get-font-list-c)

### Transform System Issues

> [!bug] Severity: Medium
> **Status:** Broken by design  
> **Affects:** Object manipulation

**Problem:** The transform box system has fundamental design flaws.

**Issues:**
- Handle detection is unreliable
- Scale operations don't respect aspect ratio
- Move operations have offset errors
- Coordinate system mismatches

**Code Problems:**
```cpp
// This calculation is wrong:
int deltaX = mousePos.x - m_transformStartMouse.x;
// Should account for canvas offset and zoom
```

**What Should Be Fixed:**
1. Proper coordinate system handling
2. Aspect ratio constraints for scaling
3. Better handle hit detection
4. Visual feedback for current operation

**Status:** Too complex to fix properly, would need complete rewrite.

**Related:** [[new_features#Selection Tool Overhaul]]

## Minor Issues

### Cross-Platform Build Problems

> [!info] Severity: Low
> **Status:** Mostly resolved  
> **Affects:** Initial setup

**Common Issues:**

**SDL2 Not Found:**
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev

# Fedora/RHEL  
sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel

# Arch
sudo pacman -S sdl2 sdl2_ttf sdl2_image

# macOS
brew install sdl2 sdl2_ttf sdl2_image
```

**C++20 Compiler Issues:**
- Ubuntu 18.04: Need to install gcc-10 manually
- CentOS 7: Default GCC too old
- macOS: Need Xcode 12+

**CMake vs Makefile:**
- CMake: Better cross-platform support
- Makefile: Simpler but Linux-focused

**External Resources:**
- [SDL2 Installation Guide](https://wiki.libsdl.org/SDL2/Installation)
- [CMake Best Practices](https://cliutils.gitlab.io/modern-cmake/)

### File Path Handling

> [!tip] Severity: Low
> **Status:** Workaround exists

**Problem:** Inconsistent path handling across platforms.

**Issues:**
- Windows backslash vs Unix forward slash
- Unicode filenames not fully supported
- Relative vs absolute path confusion

**Workaround:** Use SDL2's path functions where possible:
```cpp
char* basePath = SDL_GetBasePath();
char* prefPath = SDL_GetPrefPath("Organization", "AppName");
```

### UI Responsiveness

> [!note] Severity: Low
> **Status:** Design limitation

**Problem:** UI blocks during long operations (large image filters).

**Cause:** Single-threaded architecture - SDL2 and ImGui are not thread-safe.

**Symptoms:**
- UI freezes during blur operations
- No progress indication
- Can't cancel long operations

**Potential Solutions:**
1. Break operations into smaller chunks
2. Implement async processing with callbacks
3. Add progress bars and cancel buttons

**Not Implemented:** Would require major architecture changes.

## Input Issues

### Event Handling Priority Problems

> [!warning] Critical Design Flaw
> **Affects:** All user input

**Problem:** SDL2 and ImGui event handling conflicts create unpredictable behavior.

**Symptoms:**
- Keyboard shortcuts sometimes trigger multiple times
- Mouse events occasionally ignored
- Text input interferes with canvas operations
- Modifier keys get "stuck" in wrong state

**Code Issue:**
```cpp
// This pattern is used but doesn't work reliably:
ImGui_ImplSDL2_ProcessEvent(&event);
if (!ImGui::GetIO().WantCaptureMouse) {
    canvas.handleEvents(event);
    toolManager.handleEvents(event);
}
// Problem: keyboard and mouse handled differently
```

**Platform Variations:**
- Windows: Worst behavior, especially with Alt key
- Linux: More stable but still has issues
- macOS: Different modifier key expectations

### File Dialog Integration Issues

> [!info] Tinyfiledialogs Problems
> **Status:** Partially working

**Problem:** Native file dialogs don't always integrate properly.

**Issues:**
- File dialog sometimes appears behind main window
- Cancel operation doesn't always register
- File path encoding problems with special characters
- Multiple file selection broken

**Workaround:** Use simple file path input instead of dialogs.

## Platform-Specific Issues

### Windows (MSYS2)

> [!warning] Windows Users
> MSYS2 setup can be tricky. Follow the exact steps in the README.

**Common Problems:**
- Wrong terminal (use UCRT64)
- Missing toolchain packages
- DLL not found errors

**Solutions:**
```bash
# Make sure you're in UCRT64 terminal
pacman -S mingw-w64-ucrt-x86_64-toolchain
pacman -S mingw-w64-ucrt-x86_64-SDL2
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
pacman -S mingw-w64-ucrt-x86_64-SDL2_image
```

**DLL Issues:** CMake tries to copy DLLs automatically, but paths might be wrong.

### Linux

> [!success] Linux Generally Works
> Linux is the most tested platform, fewest issues.

**Distribution Differences:**
- Package names vary slightly
- Some distros use older SDL2 versions
- Font paths different

**Wayland vs X11:** Should work on both, but X11 more tested.

### macOS

> [!info] macOS Specific
> Requires Xcode Command Line Tools and Homebrew.

**Issues:**
- Framework vs library linking
- Code signing for distribution
- Different keyboard shortcuts expected

**Retina Display:** UI scaling might look wrong on high-DPI displays.

## Performance Issues

### Large Image Handling

> [!warning] Memory Usage
> No optimization for large images (>4k resolution).

**Problems:**
- Memory usage scales O(width × height × layers)
- No tile-based rendering
- Full image loaded into memory

**Symptoms:**
- Slow performance on 4K+ images
- Memory exhaustion
- Swap thrashing

**Workaround:** Resize images before editing, or add more RAM.

### Filter Performance

**Slow Filters:**
- Blur (especially large radius)
- Edge detection
- Color grading

**Why Slow:**
- CPU-only processing
- No SIMD optimization
- Synchronous execution

**Potential Optimizations:**
- GPU shaders for filters
- Multi-threading
- SIMD instructions
- Tile-based processing

## Debug Information

### Useful Debug Tools

**Memory Leaks:**
```bash
# Linux
valgrind --leak-check=full ./bin/EnoughImageEditor

# macOS  
leaks -atExit -- ./bin/EnoughImageEditor
```

**Performance Profiling:**
```bash
# Linux
perf record ./bin/EnoughImageEditor
perf report

# General
gprof ./bin/EnoughImageEditor gmon.out
```

### Debug Build

```bash
# CMake debug build
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Makefile debug
make CXXFLAGS="-g -O0 -DDEBUG"
```

**Debug Flags Added:**
- `-g`: Debug symbols
- `-O0`: No optimization
- `-DDEBUG`: Enable debug prints

### Common Error Messages

**"SDL_Init failed":**
- Missing SDL2 libraries
- Check installation

**"Failed to create window":**
- Graphics driver issues
- Try different SDL video driver

**"Texture creation failed":**
- Out of GPU memory
- Image too large

**"Font loading failed":**
- Font file not found
- Check font path
- Font file corrupted

## Getting Help

### Before Reporting Issues

> [!note] Project Abandoned
> Remember: this project is no longer maintained. These are just notes for anyone trying to understand or fix the code.

**Check These First:**
1. Is your compiler C++20 compatible?
2. Are all SDL2 libraries installed?
3. Did you run `python3 setup.py`?
4. Does a minimal example work?

### Useful Resources

**SDL2 Resources:**
- [SDL2 Documentation](https://wiki.libsdl.org/SDL2/FrontPage)
- [LazyFoo SDL2 Tutorials](https://lazyfoo.net/tutorials/SDL/) - These tutorials were invaluable
- [SDL2 GitHub Issues](https://github.com/libsdl-org/SDL/issues)

**Dear ImGui Resources:**
- [Dear ImGui GitHub](https://github.com/ocornut/imgui)
- [ImGui Demo Window](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp)
- [ImGui FAQ](https://github.com/ocornut/imgui/blob/master/docs/FAQ.md)

**C++ Resources:**
- [cppreference.com](https://en.cppreference.com/) - C++20 documentation
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

**Graphics Programming:**
- [Real-Time Rendering](http://www.realtimerendering.com/) - Comprehensive graphics reference
- [Computer Graphics: Principles and Practice](https://www.amazon.com/Computer-Graphics-Principles-Practice-3rd/dp/0321399528)

### Learning Resources

**Courses That Helped:**
- **Vikash Kumar's Udemy Course** - Provided the foundation for this project
- **John Purcell's SDL2 Course** - Essential for understanding SDL2 architecture

**YouTube Channels:**
- Various image processing tutorials (search for "convolution filters", "image processing algorithms")
- SDL2 tutorials for graphics programming basics

### Related Documentation

- [[TECHNICAL_OVERVIEW]] - Core architecture details
- [[OOP_DESIGN]] - Why the refactoring made things worse
- [[new_features]] - Latest changes before abandoning
- [[CHANGES]] - Complete development history
- [[REFACTORING_REPORT]] - Painful refactoring process
- [[COLOR_GRADING]] - Filter implementation details

## Final Notes

> [!quote] Developer's Note
> I spent way too much time trying to fix these issues. Some are fundamental design problems that would require starting over. If you're thinking about fixing them, seriously consider whether your time would be better spent on a new project.

**Lessons Learned:**
1. Test on multiple platforms early
2. Keep architecture simple initially
3. Memory management is harder than it looks
4. Graphics programming has lots of edge cases

**For Future Projects:**
- Test features immediately after implementing them
- Don't assume code works just because it compiles
- Use established libraries (like GIMP's GEGL) instead of rolling your own
- Start with smaller scope and actually finish features
- Write tests from the beginning
- Profile early and often

**Alternative Approaches:**
- Use a game engine (Unity, Godot) for tool development
- Build as a web app with Canvas API
- Use existing frameworks (Qt, GTK) instead of ImGui
- Stick with procedural code until you actually need OOP complexity

**Hard Truth:** Most of the "features" I implemented in the final update don't actually work. I got so caught up in writing code that I forgot to test if it was functional. 

Remember: this project was a learning experience. The bugs and failed implementations taught me more than if everything had worked perfectly - mainly that I need to test my code more thoroughly.