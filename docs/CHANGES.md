# Project Changes & Development Log

> [!info] About This Document
> This tracks the major changes made to the project before I decided to stop working on it. Written for my own reference and anyone else who might poke around the codebase.

## July 11, 2025 - Final Update

### What Got Done

**Selection Tool Rewrite**
- Completely rewrote (how many times is irrelevant. Do not mention it in my presence) the selection system because it was broken
- Left click = move objects, Shift+left click = scale
- Made scaling less sensitive (was stupidly jumpy before)
- Increased minimum transform size from 10px to 20px
- **Result:** Still doesn't work properly, but at least it's different

**Clone Stamp Tool**
- Added clone stamp functionality (attempted)
- Alt+click to set source, drag to clone
- Supposed to work like real image editors
- **Result:** Completely broken - doesn't actually clone anything

**Sharpen Filter**
- Basic unsharp mask implementation
- Uses standard convolution kernel
- Adjustable strength parameter
- **Result:** Works fine, but filter stacking still causes crashes

**Recent Files System**
- Tracks last 10 opened/saved files (attempted)
- Saves to `.enough_recent_files` config
- Auto-deduplication
- **Result:** Broken - file I/O operations fail silently

**C++20 Upgrade**
- Updated build system to require C++20
- Modified CMakeLists.txt and Makefile
- Updated compiler requirements in docs
- **Result:** Compiles fine on modern systems

**Keyboard Shortcuts**
- Attempted to implement consistent shortcuts
- Ctrl+Z/Y for undo/redo, tool switching with numbers
- Copy/paste functionality
- **Result:** Inconsistent and unreliable - event handling conflicts between SDL2 and ImGui

### Files Modified

**Core Implementation:**
- `tools/ToolManager.cpp` - Selection tool rewrite, clone stamp
- `canvas/Canvas.cpp` - Transform handling, sharpen filter
- `editor/Editor.cpp` - Recent files system
- `tools/Tool.hpp` - Clone stamp class declaration
- `canvas/Canvas.hpp` - Sharpen filter declaration
- `editor/Editor.hpp` - Recent files interface

**Build System:**
- `CMakeLists.txt` - C++20 standard
- `Makefile` - C++20 compiler flags
- `build_and_test.sh` - Updated comments

**Documentation:**
- `README.md` - Project status, C++20 requirements
- All files in `docs/` folder

### Technical Details

**Selection Tool:**
- Commented out old implementation instead of deleting
- Added proper keyboard state detection
- Separated move/scale logic into different code paths
- Better bounds checking in transform operations
- **Issue:** Flood select still changes colors on deletion

**Clone Stamp:**
- Inherits from base `Tool` class
- Uses offset-based cloning algorithm (attempted)
- Circular brush with opacity blending
- Visual feedback with crosshair and circle
- **Broken:** Pixel copying doesn't work, cloning operation fails

**Sharpen Filter:**
- Standard `[0,-1,0],[-1,5,-1],[0,-1,0]` kernel
- Follows existing filter pattern with buffer system
- Pixel-level manipulation with bounds checking
- **Issue:** Still segfaults when stacking filters

**Recent Files:**
- Simple file-based storage system
- Cross-platform compatible file handling (attempted)
- Integrated with import/export operations
- **Broken:** File I/O fails, config file not created properly

### Code Quality

**Memory Management:**
- Maintained RAII principles
- Proper SDL cleanup patterns
- No new memory leaks introduced (that I'm aware of)

**Performance:**
- Optimized filters for real-time use
- Efficient pixel manipulation
- Reduced UI sensitivity to prevent lag

**Documentation:**
- Added comprehensive implementation notes
- Maintained existing comment style
- Used TODO/FIXME/HACK conventions consistently

## Development Cycle

### Initial Architecture (April 2025)

**Procedural to OOP Refactor:**
- Started with procedural approach (Copied Vikash Kumar's opengl tutorial but used SDL2 instead of GLUT and OpenGL)
- Refactored to namespaces to initially split up monolith of code
- Refactored to object-oriented design 
- Implemented singleton pattern for core components
- Created proper class hierarchies for tools

**Core Systems:**
- Canvas and Layer management
- Tool system with inheritance
- Undo/redo functionality
- File I/O operations

**GUI Implementation:**
- SDL2 + Dear ImGui integration (Originally SDL_TTF TUI integration)
- Semi-Professional interface layout
- Tool panels and property editors
- Real-time preview system

### Major Features Added

**Drawing Tools:**
- Pencil, eraser, shapes (line, rectangle, circle, triangle)
- Gradient tool (linear, radial, angular)
- Text tool with font support
- Fill bucket with flood fill algorithm

**Layer System:**
- Create, duplicate, reorder layers
- Blend modes and opacity controls
- Layer locking and visibility
- Layer renaming and masking

**Image Processing:**
- Grayscale, blur, edge detection filters
- Color grading suite
- Directional blur effects
- Contrast and brightness adjustments

**Advanced Features:**
- Smart object selection
- Transform operations (move, scale, rotate)
- Copy/paste functionality
- Keyboard shortcuts
- Cross-platform file dialogs

### Known Issues Throughout Development

**Memory Management:**
- Texture leaks in filter operations
- Improper cleanup in some edge cases
- **Status:** Partially fixed with buffer system

**Filter System:**
- Stacking certain filters causes segfaults
- Grayscale + blur combination is particularly problematic
- **Status:** Workaround implemented, not fully resolved

**Selection System:**
- Fundamentally broken from the start
- Multiple rewrite attempts
- **Status:** Still broken, gave up fixing it

**Cross-Platform Issues:**
- Font loading differences between platforms
- File path handling inconsistencies
- **Status:** Mostly resolved with proper abstractions

## Development Statistics

**Timeline:**
- Started: April 2025
- Final Update: July 11, 2025
- Active Development: ~3.5 months

**Development Timeline:** ~3.5 months
**Code Metrics:**
- Total Lines: ~7000+
- Files: 20+ source files
- Documentation: 15+ files
- Features: 12+ major tools/features (some don't actually work)

**Repository History:**
- Multiple private repositories deleted/recreated
- Numerous feature branches abandoned
- Several complete rewrites of core systems

## Why I Stopped

**Complexity Overwhelm:**
- Codebase grew beyond my ability to maintain
- Too many interconnected systems
- Bug fixes created new bugs

**Technical Debt:**
- Selection system fundamentally broken
- Filter stacking causes crashes
- Memory management issues persist
- Clone stamp tool doesn't function
- Recent files system fails silently
- Keyboard shortcuts unreliable

**Time Investment:**
- Features taking much longer than expected
- Debugging eating up development time
- Feature gating more advanced implementations to keep GitHub version stable
- Other projects became more interesting

**Skill Limitations:**
- My coding level insufficient for proper maintenance
- Advanced graphics programming beyond my expertise
- Complex UI interactions too difficult to debug

## Lessons Learned

**Architecture:**
- Should have stuck with simpler design
- OOP refactor added complexity without enough benefit
- Singleton pattern overused

**Feature Scope:**
- Tried to implement too much too quickly
- Should have focused on core functionality
- Feature creep killed the project

**Documentation:**
- Good documentation helped but wasn't enough
- Should have written tests
- Better version control practices needed

**Project Management:**
- Feature gating helped maintain stable public version while developing in background
- Occasional timeboxing helped but wasn't followed consistently
- Should have cut features earlier
- Multiple projects simultaneously was too much

## Related Documentation

- [[TECHNICAL_OVERVIEW]] - Core architecture details
- [[OOP_DESIGN]] - Object-oriented refactoring process
- [[TROUBLESHOOTING]] - Known issues and workarounds
- [[REFACTORING_REPORT]] - Detailed development journey
- [[new_features]] - Latest features implementation
- [[COLOR_GRADING]] - Image processing algorithms
- [[TEXT_TOOL_GUIDE]] - Font system implementation
- [[FLOOD_FILL_AND_ADJUSTMENTS]] - Algorithm implementations

## Final Notes

This project taught me a lot about C++, SDL2, and graphics programming. While I couldn't maintain it to completion, the learning experience was valuable. The codebase might be useful for others learning similar technologies.

**Next Project:** Moving on to "Alcides" - expected around September 2025. Will apply lessons learned from this project to keep scope manageable.

**Archive Status:** This project is complete and archived. No further development planned.