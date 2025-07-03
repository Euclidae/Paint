#!/usr/bin/env python3
"""
Setup script for Paint project
Downloads ImGui and TinyFileDialogs dependencies
"""

import os
import shutil
import urllib.request
import zipfile
import sys

def download_file(url, filename):
    """Download a file from URL"""
    print(f"Downloading {filename}...")
    urllib.request.urlretrieve(url, filename)
    print(f"Downloaded {filename}")

def extract_zip(zip_path, extract_to):
    """Extract a zip file"""
    print(f"Extracting {zip_path}...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_to)
    print(f"Extracted to {extract_to}")

def setup_imgui():
    """Download and setup ImGui"""
    imgui_url = "https://github.com/ocornut/imgui/archive/refs/heads/master.zip"

    # Download ImGui
    download_file(imgui_url, "imgui.zip")

    # Extract
    extract_zip("imgui.zip", "temp")

    # Create imgui directory
    if os.path.exists("imgui"):
        shutil.rmtree("imgui")
    os.makedirs("imgui")

    # List of files we need from ImGui
    imgui_files = [
        "imgui.h",
        "imgui.cpp",
        "imgui_demo.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "imgui_internal.h",
        "imconfig.h",
        "imstb_rectpack.h",
        "imstb_textedit.h",
        "imstb_truetype.h"
    ]

    # Backend files we need
    backend_files = [
        "backends/imgui_impl_sdl2.cpp",
        "backends/imgui_impl_sdl2.h",
        "backends/imgui_impl_sdlrenderer2.cpp",
        "backends/imgui_impl_sdlrenderer2.h",
        "backends/imgui_impl_sdlrenderer3.cpp",
        "backends/imgui_impl_sdlrenderer3.h",
        "backends/imgui_impl_opengl2.cpp",
        "backends/imgui_impl_opengl2.h",
        "backends/imgui_impl_opengl3.cpp",
        "backends/imgui_impl_opengl3.h",
        "backends/imgui_impl_opengl3_loader.h"
    ]

    # Copy main ImGui files
    for file in imgui_files:
        src = f"temp/imgui-master/{file}"
        dst = f"imgui/{file}"
        if os.path.exists(src):
            shutil.copy2(src, dst)
            print(f"Copied {file}")

    # Copy backend files
    for file in backend_files:
        src = f"temp/imgui-master/{file}"
        dst = f"imgui/{os.path.basename(file)}"
        if os.path.exists(src):
            shutil.copy2(src, dst)
            print(f"Copied {os.path.basename(file)}")

    # Cleanup
    os.remove("imgui.zip")
    shutil.rmtree("temp")

    print("ImGui setup complete!")

def setup_tinyfiledialogs():
    """Download and setup TinyFileDialogs"""
    tfd_url = "https://sourceforge.net/projects/tinyfiledialogs/files/tinyfiledialogs-3.18.1.zip/download"

    # Download TinyFileDialogs
    download_file(tfd_url, "tinyfiledialogs.zip")

    # Extract
    extract_zip("tinyfiledialogs.zip", "temp")

    # Create tinyfiledialogs directory structure
    if os.path.exists("tinyfiledialogs"):
        shutil.rmtree("tinyfiledialogs")
    os.makedirs("tinyfiledialogs")

    # Find the extracted folder (version number might vary)
    temp_folder = None
    for item in os.listdir("temp"):
        if item.startswith("tinyfiledialogs"):
            temp_folder = f"temp/{item}"
            break

    if not temp_folder:
        print("Error: Could not find tinyfiledialogs folder")
        return

    # Copy main files
    main_files = [
        "tinyfiledialogs.h",
        "tinyfiledialogs.c",
        "README.txt",
        "hello.c",
        "hello_wchar_t.c"
    ]

    for file in main_files:
        src = f"{temp_folder}/{file}"
        dst = f"tinyfiledialogs/{file}"
        if os.path.exists(src):
            shutil.copy2(src, dst)
            print(f"Copied {file}")

    # Copy more_dialogs folder
    more_dialogs_src = f"{temp_folder}/more_dialogs"
    more_dialogs_dst = "tinyfiledialogs/more_dialogs"
    if os.path.exists(more_dialogs_src):
        shutil.copytree(more_dialogs_src, more_dialogs_dst)
        print("Copied more_dialogs/")

    # Copy dll_cs_lua_R_fortran_pascal folder
    dll_folder_src = f"{temp_folder}/dll_cs_lua_R_fortran_pascal"
    dll_folder_dst = "tinyfiledialogs/dll_cs_lua_R_fortran_pascal"
    if os.path.exists(dll_folder_src):
        shutil.copytree(dll_folder_src, dll_folder_dst)
        print("Copied dll_cs_lua_R_fortran_pascal/")

    # Cleanup
    os.remove("tinyfiledialogs.zip")
    shutil.rmtree("temp")

    print("TinyFileDialogs setup complete!")

def create_gitignore():
    """Create .gitignore to exclude dependencies"""
    gitignore_content = """# Dependencies (downloaded by setup.py)
imgui/
tinyfiledialogs/

# Build artifacts
bin/
*.o
*.exe

# IDE files
.vscode/
*.swp
*.swo

# Other
imgui.ini
"""

    with open(".gitignore", "w") as f:
        f.write(gitignore_content)

    print("Created .gitignore")

def main():
    """Main setup function"""
    print("Setting up Paint project dependencies...")
    print("=" * 50)

    # Check if we're in the right directory
    if not os.path.exists("main.cpp"):
        print("Error: Run this script from the project root directory")
        sys.exit(1)

    # Setup dependencies
    setup_imgui()
    setup_tinyfiledialogs()
    create_gitignore()

    print("\n" + "=" * 50)
    print("Setup complete! You can now build the project.")
    print("Dependencies downloaded:")
    print("- ImGui -> imgui/")
    print("- TinyFileDialogs -> tinyfiledialogs/")
    print("- .gitignore created")

    print("\nNote: These folders are now in .gitignore")
    print("Run this script again if you need to re-download dependencies")

if __name__ == "__main__":
    main()
