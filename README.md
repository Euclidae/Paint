Simple Paint Application using SDL2

![image](https://github.com/user-attachments/assets/be226a33-59be-428a-81b5-184679a1b9a5)


This is an enhanced paint application built with SDL2, featuring a text-based user interface (TUI), essential drawing tools, and color options. The project was inspired by Vikash Kumar's OpenGL tutorials on Udemy, and the documentation was created with assistance from DeepSeek-R1 AI assistant.
Key Enhancements

    🖥️ Text User Interface (TUI): Right-side panel showing available tools and active selections

    🎨 Improved Visual Feedback: Active tools highlighted in yellow

    📐 Canvas Area: Dedicated drawing space (1080x720) with separate UI area

    🔤 Font Rendering: Using SDL_ttf for text display

    🛡️ Boundary Checking: Tools only work within canvas area

    📱 Responsive Design: Better window positioning and scaling

Features

    🖌️ Drawing Tools: Pencil, Eraser, Line, Rectangle, and Paint Bucket (flood fill)

    🎨 Color Selection: Black, Red, Green, Yellow

    ↩️ Undo Function: Reverse your last action

    🖱️ Intuitive Interface: Keyboard shortcuts + mouse drawing

    ⚡ Real-time Preview: See your drawing as you create it

    🔠 Tool Status Display: Visual indication of active tool/color

Dependencies

    SDL2 (Simple DirectMedia Layer)

    SDL2_ttf (TrueType Font support)

Installation
Windows

    Install vcpkg

    Install dependencies:
    powershell

vcpkg install sdl2 sdl2-ttf

Clone repository:
bash

git clone https://github.com/yourusername/sdl-paint-app.git

Compile with:
bash

    g++ src/main.cpp -Iinclude -Llib -lSDL2 -lSDL2_ttf -o sdl-paint.exe

Linux

    Install dependencies:
    bash

# Debian/Ubuntu
sudo apt install libsdl2-dev libsdl2-ttf-dev g++

# Fedora
sudo dnf install SDL2-devel SDL2_ttf-devel gcc-c++

Clone repository:
bash

git clone https://github.com/yourusername/sdl-paint-app.git

Compile with:
bash

    g++ src/main.cpp -lSDL2 -lSDL2_ttf -o sdl-paint

Usage

    Place arial.ttf in the same directory as the executable

    Run the application:
    bash

    # Windows
    sdl-paint.exe

    # Linux
    ./sdl-paint

    Tool Selection (Keyboard Shortcuts):

        P: Pencil Tool

        E: Eraser Tool

        L: Line Tool

        F: Paint Bucket (Flood Fill)

        Q: Rectangle Tool

        Z: Undo last action

        ESC: Quit application

    Color Selection:

        R: Red

        G: Green

        B: Black

        Y: Yellow

    Interface:

        Left area: Drawing canvas (1080x720 pixels)

        Right panel: Tool information and status

        Active tools highlighted in yellow

Technical Notes

    Font Handling:

        Looks for arial.ttf in current directory 

        Checks common system font paths as fallback

        TTF initialization is handled automatically

    Canvas Boundaries:

        Drawing only occurs within canvas area (left 1080px)

        UI panel (right 200px) handles tool information

    Enhanced Features:

        Active tool highlighting in TUI

        Real-time preview for all tools

        Robust input validation

        Improved memory management

File Structure

sdl-paint-app/
├── main.cpp         # Main application code
├── arial.ttf            # Font file (required)
└── README.md            # This documentation

Acknowledgments

    Inspired by Vikash Kumar's OpenGL tutorials on Udemy

    Also inspired by Dr. Mike Shah's C++ series.

    Documentation created with assistance from DeepSeek-R1 AI assistant

    Uses Simple DirectMedia Layer (SDL2) and SDL_ttf libraries

    Includes Arial font (Microsoft font license)

License

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
