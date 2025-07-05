# Enough - Advanced Image Editor

![alt text](image.png)

This is a sophisticated image editing application built with SDL2, featuring a comprehensive GUI, layer management, and professional-grade tools. The project was primarily developed through my own efforts, evolving from basic drawing concepts into a comprehensive image editor with modern features and robust architecture. The documentation was created with assistance from Claude AI and DeepSeek-R1 AI assistants.

## Key Improvements Over Previous Versions

- **Professional GUI**: Modern Dear ImGui interface with intuitive panels replacing basic TUI
- **Robust Layer System**: Layer locking, duplication, reordering, and blending modes
- **Advanced Tools**: Gradient tool, text tool with font options, and selection tools
- **Critical Bug Fixes**: Resolved memory leaks, coordinate system issues, and flood fill algorithm
- **Cross-Platform**: Tested and optimized for Linux (Fedora, Ubuntu, Arch), Windows, and macOS
- **Self-Contained Dependencies**: Automated setup for external libraries with setup.py script

## Features

### Professional GUI
- Modern interface with panels for tools, layers, and properties
- Intuitive layout with tool selection and property editing
- Real-time preview and immediate visual feedback
- Left drawing canvas with adjustable size
- Right panel for tools and properties

### Layer Management
- Create, duplicate, reorder, and blend layers with various modes
- Layer opacity and visibility controls
- Layer locking to prevent accidental modifications
- Merge layers functionality
- Professional layer compositing with proper alpha blending

### Drawing Tools
- **Pencil** with adjustable size and pressure sensitivity
- **Eraser** with size control
- **Shapes**: Line, Rectangle, Circle, Triangle
- **Gradient tool** (linear, radial, angular)
- **Text tool** with font customization and TTF/OTF file loading
- **Selection tools** with copy/paste functionality
- **Fill bucket** with optimized flood fill algorithm using scanline method

### Color System
- Primary/secondary color management
- Color swapping functionality (`X` key)
- Eyedropper tool for color picking
- Real-time color preview
- Color picker integration

### Advanced Features
- **Undo/Redo**: Comprehensive history system (Ctrl+Z / Ctrl+Y)
- **File Operations**: Open/save images (PNG, JPG, BMP)
- **Canvas Control**: Dynamic resizing with input fields
- **Font Support**: Custom font loading with TTF/OTF files
- **Keyboard Shortcuts**: Full hotkey system for efficient workflow
- **Selection Operations**: Copy, paste, delete with proper clipboard integration

## Dependencies

- **SDL2** (Simple DirectMedia Layer) - Core graphics and input
- **SDL2_ttf** (TrueType Font support) - Font rendering
- **SDL2_image** (Image loading support) - Image format support
- **Dear ImGui** (Immediate mode GUI) - User interface (auto-downloaded)
- **TinyFileDialogs** (Native file dialogs) - Cross-platform file dialogs (auto-downloaded)

## Installation

### Quick Setup

```bash
# Clone the repository
git clone https://github.com/Euclidae/Enough.git
cd Enough

# Run setup script to download dependencies
python3 setup.py

# Build the application
make

# Run the application
./bin/enough
```

### Platform-Specific Instructions

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev build-essential python3
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel gcc-c++ make python3
```

#### Linux (Arch)
```bash
sudo pacman -S sdl2 sdl2_ttf sdl2_image base-devel python3
```

#### macOS
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install sdl2 sdl2_ttf sdl2_image python3
```

#### Windows
1. Install MinGW-w64 or MSYS2
2. The setup script will automatically download SDL2 development libraries
3. Install Python 3
4. Run the setup script

## Building

The Makefile supports both system and local SDL2 installations:

```bash
# Build with system SDL2 (default)
make

# Build with local SDL2 (Windows)
make USE_LOCAL_SDL=1

# Clean build files
make clean

# Run the application
./bin/enough
```

## Usage

### Interface Overview
- **Left Area**: Drawing canvas (resizable)
- **Right Panel**: Tools, properties, and layer management
- **Top Menu**: File operations and editing commands

### Tool Selection (Keyboard Shortcuts)
- **P**: Pencil Tool
- **E**: Eraser Tool
- **L**: Line Tool
- **R**: Rectangle Tool
- **C**: Circle Tool
- **T**: Triangle Tool
- **G**: Gradient Tool
- **S**: Selection Tool
- **F**: Fill Bucket (Flood Fill)
- **ESC**: Clear selection or reset tool

### Advanced Operations
- **Layer Management**: Right panel for layer operations
- **Undo/Redo**: Ctrl+Z / Ctrl+Y
- **Color Swap**: X to swap primary/secondary colors
- **Brush Size**: Mouse wheel to adjust / Ctrl+Plus/Minus
- **Canvas Resizing**: Input fields in properties panel
- **Text Editing**: Click and drag to create text box, edit in popup window
- **Selection**: Copy (Ctrl+C), Paste (Ctrl+V), Delete selection
- **Layer Navigation**: Plus/Minus keys to navigate layers
- **Merge Layers**: Ctrl+M

## Technical Notes

### Key Fixes and Improvements

#### Memory Management
- Fixed texture leaks and improper cleanup
- Implemented proper RAII patterns
- Stack-based undo/redo system with memory bounds
- Proper SDL texture and font management

#### Flood Fill Algorithm
- Replaced recursive algorithm with queue-based scanline method
- Added iteration limits to prevent infinite loops
- Optimized for large canvas sizes
- Prevents stack overflow on complex shapes

#### Coordinate System
- Corrected canvas offset calculations
- Fixed mouse input mapping with proper boundary checking
- Resolved canvas panning and coordinate transformation issues

#### Font Handling
- Added caching for different font styles/sizes
- Custom font file loading support (TTF/OTF)
- Cross-platform font path detection
- Automatic font cleanup and memory management

#### Layer Rendering
- Fixed blend mode implementation
- Optimized compositing pipeline with minimal render target switches
- Proper alpha channel handling
- Layer opacity and visibility controls

#### Selection Tools
- Added proper move/copy/paste functionality
- Visual feedback with selection rectangle
- Clipboard integration with proper memory management

### Development History and Challenges

During the development process, several critical issues were encountered and resolved with assistance from Claude AI and DeepSeek-R1:

#### Text Tool Refactoring Crisis
The original text tool implementation became overly complex with layer integration issues:
- **Problem**: Memory leaks, font caching conflicts, layer system integration
- **Solution**: Simplified TextBox structure with direct rendering
- **Result**: Clean, maintainable text tool with real-time editing

#### Namespace and Linking Conflicts
Refactoring into separate modules created linking errors:
- **Problem**: `UI::hasSelection` and `Tools::selectionOffset` undefined references
- **Solution**: Proper global scope resolution with `::` operator
- **Lesson**: Careful namespace management in modular C++ projects

#### Cross-Platform Build Issues
SDL2 dependency checking caused build failures on different distributions:
- **Problem**: Package manager differences (apt vs dnf vs pacman vs brew)
- **Solution**: Automated detection and installation via setup.py script
- **Result**: Reliable builds across Linux distributions, Windows, and macOS

### Performance Optimizations

#### Texture-Based Undo/Redo
- Stores complete texture states instead of command lists
- Faster undo operations with direct texture swapping
- Memory-bounded with configurable history depth

#### Font Caching System
- Caches fonts by size/style combination
- Reduces TTF_OpenFont calls during text rendering
- Automatic cleanup on application exit

#### Layer Compositing
- Optimized rendering pipeline with minimal target switches
- Proper blend mode application
- Alpha channel optimization
- Batch operations for similar rendering calls

#### Gradient Tool Optimization
- Fixed-step gradient calculation for consistent performance
- Optimized color interpolation algorithms
- Efficient radial and angular gradient rendering

## File Structure

```
Enough/
├── bin/                    # Compiled binary
├── imgui/                  # Dear ImGui files (auto-downloaded)
├── tinyfiledialogs/        # File dialog library (auto-downloaded)
├── fonts/                  # Downloaded fonts for text tool
├── docs/                   # Technical documentation
│   └── TECHNICAL_OVERVIEW.md
├── main.cpp                # Main application entry point
├── canvas.cpp/.hpp         # Canvas and layer management
├── tools.cpp/.hpp          # Drawing tools and input handling
├── editor.cpp/.hpp         # Edit operations and history
├── ui.cpp/.hpp             # User interface implementation
├── Makefile                # Build configuration
├── setup.py                # Dependency setup script
└── README.md               # This documentation
```

## Code Architecture

### Modular Design
- **Separation of Concerns**: Each module handles specific functionality
- **Clear Interfaces**: Well-defined APIs between components
- **Minimal Dependencies**: Reduced coupling between modules

### Modern C++ Features
- **RAII**: Automatic resource management
- **STL Containers**: Safe memory management
- **Smart Pointers**: Where appropriate for SDL compatibility
- **Range-Based Loops**: Modern iteration patterns
- **Auto Keyword**: Type deduction for cleaner code

### Error Handling
- **Graceful Degradation**: Fallback options for failed operations
- **Resource Cleanup**: Proper SDL texture and font management
- **User Feedback**: Clear error messages and status updates
- **Cross-Platform Compatibility**: Robust error handling for different systems

## Development Notes

This project represents primarily my own development efforts with AI assistance for specific challenges. The core architecture, creative decisions, algorithmic implementations, and majority of the codebase were developed through my own work. Claude AI and DeepSeek-R1 provided assistance during:

- **Debugging Sessions**: Helping resolve complex linking and memory issues
- **Code Refactoring**: Assistance with modularizing the monolithic codebase
- **Cross-Platform Testing**: Help identifying and fixing platform-specific bugs
- **Documentation**: Creating comprehensive technical documentation
- **Performance Optimization**: Suggestions for identifying bottlenecks and implementing solutions

The result is a robust, maintainable codebase that demonstrates my development skills with effective use of AI assistance for specific technical challenges.

## Acknowledgments

- **Primary Development**: This project was primarily developed through my own efforts (Euclidae)
- **Initial Inspiration**: Vikash Kumar's OpenGL tutorials on Udemy
- **Specific Code Credits**:
  - **John Purcell**: Surface creation techniques (referenced in canvas.cpp)
  - **StackExchange Graphics Community**: Gradient mathematics implementation (https://graphicdesign.stackexchange.com/questions/54880/what-is-the-math-behind-gradient-map-in-photoshop)
  - **Icons8 Medium**: Gradient types reference (https://icons8.medium.com/types-of-gradients-and-how-to-use-them-9508be01048d)
- **Library Authors**:
  - **Dear ImGui**: Omar Cornut and contributors for the excellent GUI library
  - **SDL2**: Sam Lantinga and contributors for the robust multimedia library
  - **TinyFileDialogs**: Guillaume Vareille for cross-platform file dialogs
- **AI Assistance**:
  - **Claude AI**: Assistance with debugging, refactoring, and documentation
  - **DeepSeek-R1**: Additional AI assistance for technical documentation and optimization
- **Resources**: Google Fonts for the bundled font collection

## License

MIT License

Copyright (c) 2025 Euclidae

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.