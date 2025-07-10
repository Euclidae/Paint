# New Features - July 11, 2025 Update

> [!warning] Project Status
> This project is complete and no longer maintained. These were the last features added before I gave up on it on July 11, 2025.

## Recent Changes

### Selection Tool Overhaul
**Status:** 🟡 Mostly working but still broken  
**Files:** `tools/ToolManager.cpp`, `canvas/Canvas.cpp`  
**Related:** [[TROUBLESHOOTING#Selection Tool Issues]]

The selection tool was a nightmare. I tried to fix it to work like actual image editors:
- **Left click** = move selected stuff around
- **Shift + left click** = scale (supposed to have higher precedence)
- Made scaling less jumpy by reducing sensitivity

**What I actually did:**
- Commented out the old broken code instead of deleting it
- Added proper keyboard state checking for shift
- Separated move/scale into different code paths
- Increased minimum transform size from 10px to 20px

**Reality check:** It still doesn't work properly. The flood select changes colors when you delete selections, and I can't figure out why.

### Clone Stamp Tool
**Status:** 🔴 Broken  
**Files:** `tools/Tool.hpp`, `tools/ToolManager.cpp`  
**Related:** [[SDL_IMGUI_FUNCTIONS_REFERENCE#Tool System]]

This one looked promising but doesn't actually work:
- **Alt + left click** = set source point (shows red crosshair)
- **Left click + drag** = clone from source to destination
- Source point moves with your brush (traditional behavior)
- Circular brush with size control

**Implementation notes:**
- New `CloneStampTool` class inheriting from `Tool`
- Uses offset-based cloning algorithm
- Has visual feedback with crosshair and circle
- **Problem:** Cloning operation doesn't actually copy pixels correctly

**What doesn't work:**
1. Source point detection is unreliable
2. Cloning doesn't transfer pixel data properly
3. Crosshair appears but painting does nothing
4. Brush blending is broken

### Sharpen Filter
**Status:** ✅ Actually works  
**Files:** `canvas/Canvas.hpp`, `canvas/Canvas.cpp`  
**Related:** [[COLOR_GRADING#Filter Implementation]]

Added basic sharpening using unsharp mask:
- Classic convolution kernel: `[0,-1,0],[-1,5,-1],[0,-1,0]`
- Adjustable strength (default: 2)
- Proper edge handling
- Integrated with existing filter system

**Access:** `applyFilter(3)` or direct `applySharpen(strength)` call

**Warning:** Still causes segfaults when stacking with other filters. See [[TROUBLESHOOTING#Filter Stacking Issues]].

### Recent Files System
**Status:** 🔴 Broken  
**Files:** `editor/Editor.hpp`, `editor/Editor.cpp`, `canvas/Canvas.cpp`  
**Related:** [[TECHNICAL_OVERVIEW#File System]]

Supposed to track recent files but doesn't work:
- Tracks up to 10 files
- Saves to `.enough_recent_files` config file
- Auto-deduplication (moves existing to top)
- Integrates with import/export

**Implementation:**
- Added to `Editor` class
- Simple file-based storage  
- Cross-platform file handling
- **Problem:** File I/O operations fail silently

**What's broken:**
- Config file creation fails
- File paths not properly saved
- Loading recent files doesn't work
- Integration with import/export incomplete

### Keyboard Shortcuts
**Status:** 🟡 Inconsistent  
**Files:** Various event handling code  
**Related:** [[TROUBLESHOOTING#Input Issues]]

**What works sometimes:**
- Ctrl+Z (undo) - works most of the time
- Ctrl+Y (redo) - occasionally
- Basic tool switching (1-9 keys)

**What doesn't work:**
- Ctrl+C/V (copy/paste) - completely broken
- Ctrl+S (save) - inconsistent
- Tool shortcuts conflict with text input
- Modifier key detection unreliable across platforms

**Problem:** Event handling priority is messed up between ImGui and SDL2.

## C++20 Upgrade

**Why:** Figured I might as well use modern C++ since I was already changing things.

**Compiler Requirements:**
- GCC 10+ (recommended: GCC 11+)
- Clang 10+ (recommended: Clang 12+)
- MSVC 2019+ (VS 2019 16.11+)
- Apple Clang 12+ (Xcode 12+)

**Files Updated:**
- `CMakeLists.txt` - `CMAKE_CXX_STANDARD 20`
- `Makefile` - `-std=c++20`
- Build scripts updated

**Platform Notes:**
- Ubuntu 20.04+: Need `gcc-10` or newer
- Fedora 32+: Default GCC works
- Arch: Default GCC works
- macOS: Need Xcode 12+
- Windows: MSYS2 toolchain handles it

## Code Quality

**Memory Management:**
- Maintained RAII patterns
- Proper SDL cleanup
- No new memory leaks (that I know of)

**Performance:**
- Optimized filters for real-time use
- Efficient pixel manipulation in sharpen
- Reduced UI sensitivity to prevent lag

**Documentation:**
- Added implementation notes
- Kept existing comment style
- Used TODO/FIXME/HACK conventions

## Testing Notes

**What actually works:**
- Sharpen filter integrates with existing UI
- Undo/redo works sometimes
- C++20 compilation works fine
- No obvious crashes in basic use

**Edge cases handled:**
- Empty/locked layers
- Out-of-bounds coordinates
- Keyboard state across platforms
- Filter buffer edge cases
- Transform bounds validation

## Known Issues

> [!bug] Major Problems
> These are the issues that made me give up on the project:

1. **Selection Tool**: Still fundamentally broken despite the rewrite
2. **Flood Select**: Changes colors when deleting selections
3. **Filter Stacking**: Causes segfaults, especially grayscale + blur
4. **Clone Stamp**: Doesn't actually clone anything
5. **Recent Files**: File I/O operations fail
6. **Keyboard Shortcuts**: Inconsistent and unreliable
7. **Memory Leaks**: Definitely still there despite attempts to fix

See [[TROUBLESHOOTING]] for more details and attempted workarounds.

## Future Ideas (Never Happening)

**Clone Stamp:**
- Make it actually work first
- Fix pixel copying algorithm
- Proper source point detection

**Selection Tool:**
- Visual feedback for modes
- Proportional scaling
- Actually make it work

**Filters:**
- Filter preview
- Strength UI controls
- More convolution filters

**UI:**
- Recent files menu (when file system works)
- Customizable workspace
- Preferences dialog
- Fix keyboard shortcuts properly

## Integration Notes

**Backward Compatibility:**
- No breaking changes to existing API
- Old projects still work
- Existing shortcuts preserved

**UI Integration:**
- Clone Stamp appears in tool list automatically
- Sharpen filter in existing filter system
- Selection changes are transparent
- Features respect existing patterns

## Development Timeline

**Total Time:** ~8 hours focused development  
**Code Added:** ~400 lines  
**Files Modified:** 8 files  
**Features Added:** 2 working (sharpen filter, C++20) + 2 broken (clone stamp, recent files)  
**Bugs Fixed:** 0 (definitely added more)

**Reality:** This took way longer than it should have because I kept getting distracted by other broken things in the codebase.

---

**Links:**
- [[TECHNICAL_OVERVIEW]] - Core architecture
- [[OOP_DESIGN]] - Object-oriented refactoring
- [[TROUBLESHOOTING]] - Known issues and workarounds
- [[CHANGES]] - Full changelog
- [[REFACTORING_REPORT]] - Development journey