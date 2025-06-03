# Simple Paint Application using SDL2

![image](https://github.com/user-attachments/assets/38e23863-3851-4b6d-b5bd-8b411fe60621)


This is a lightweight paint application built with SDL2, featuring essential drawing tools and color options. The project was inspired by Vikash Kumar's OpenGL tutorials on Udemy, and the documentation was created with assistance from DeepSeek-R1 AI assistant.

## Features

- 🖌️ **Drawing Tools**: Pencil, Eraser, Line, Rectangle, and Paint Bucket (flood fill)
- 🎨 **Color Selection**: Black, Red, Green, Yellow
- ↩️ **Undo Function**: Reverse your last action
- 🖱️ **Intuitive Interface**: Keyboard shortcuts + mouse drawing
- ⚡ **Real-time Preview**: See your drawing as you create it

## Dependencies

- [SDL2](https://www.libsdl.org/) (Simple DirectMedia Layer)

## Installation

### Windows
1. Install [vcpkg](https://vcpkg.io/en/getting-started.html)
2. Install SDL2:
   ```powershell
   vcpkg install sdl2:x64-windows
   ```
3. Clone repository:
   ```bash
   git clone https://github.com/yourusername/sdl-paint-app.git
   ```
4. Compile with:
   ```bash
   g++ src/main.cpp -Iinclude -Llib -lSDL2 -o sdl-paint.exe
   ```

### Linux
1. Install dependencies:
   ```bash
   # Debian/Ubuntu
   sudo apt install libsdl2-dev g++
   
   # Fedora
   sudo dnf install SDL2-devel gcc-c++
   ```
2. Clone repository:
   ```bash
   git clone https://github.com/yourusername/sdl-paint-app.git
   ```
3. Compile with:
   ```bash
   g++ src/main.cpp -lSDL2 -o sdl-paint
   ```

## Usage

1. Run the executable:
   ```bash
   # Windows
   sdl-paint.exe
   
   # Linux
   ./sdl-paint
   ```

2. **Tool Selection**:
   - `P`: Pencil
   - `E`: Eraser
   - `L`: Line Tool
   - `F`: Paint Bucket (Flood Fill)
   - `Q`: Rectangle Tool
   - `Z`: Undo last action

3. **Color Selection**:
   - `R`: Red
   - `G`: Green
   - `B`: Black
   - `Y`: Yellow

4. **Drawing**:
   - Click and drag with selected tool
   - For flood fill, click on area to fill

## Code Structure

```
sdl-paint-app/
├── src/
│   └── main.cpp         # Main application code
├── include/             # Header files
├── lib/                 # Library files (Windows)
├── screenshot.png       # Application screenshot
└── README.md            # This file
```

## Acknowledgments

- Inspired by Vikash Kumar's OpenGL tutorials on Udemy
- Documentation created with assistance from DeepSeek-R1 AI assistant
- Uses the Simple DirectMedia Layer (SDL2) library

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
