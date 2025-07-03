#!/usr/bin/env python3
"""
Enhanced Setup script for Paint project
Uses GitHub for TinyFileDialogs and adds local SDL2 support
Tested on Fedora 38
"""

import os
import shutil
import urllib.request
import zipfile
import sys
import tarfile
import subprocess

def download_file(url, filename):
    """Download a file from URL with error handling"""
    print(f"Downloading {filename}...")
    try:
        urllib.request.urlretrieve(url, filename)
    except urllib.error.HTTPError as e:
        print(f"HTTP Error {e.code}: {e.reason}")
        print(f"Failed to download: {url}")
        sys.exit(1)
    except Exception as e:
        print(f"Download error: {str(e)}")
        sys.exit(1)
    print(f"Downloaded {filename}")

def extract_zip(zip_path, extract_to):
    """Extract a zip file"""
    print(f"Extracting {zip_path}...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
    except zipfile.BadZipFile:
        print("Error: Invalid zip file. Download may have failed.")
        sys.exit(1)
    print(f"Extracted to {extract_to}")

def extract_tar_gz(tar_path, extract_to):
    """Extract a .tar.gz file"""
    print(f"Extracting {tar_path}...")
    try:
        with tarfile.open(tar_path, "r:gz") as tar_ref:
            tar_ref.extractall(extract_to)
    except tarfile.TarError as e:
        print(f"Error extracting tar.gz file: {str(e)}")
        sys.exit(1)
    print(f"Extracted to {extract_to}")

def setup_imgui():
    """Download and setup ImGui"""
    imgui_url = "https://github.com/ocornut/imgui/archive/refs/tags/v1.90.4.zip"

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
    ]

    # Copy main ImGui files
    for file in imgui_files:
        src = f"temp/imgui-master/{file}"  # Note: version might be in folder name
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
    """Download and setup TinyFileDialogs from GitHub"""
    print("Setting up TinyFileDialogs from GitHub...")
    
    # Create tinyfiledialogs directory
    if os.path.exists("tinyfiledialogs"):
        shutil.rmtree("tinyfiledialogs")
    os.makedirs("tinyfiledialogs")
    
    # Check if git is available
    git_available = shutil.which("git") is not None
    
    if git_available:
        print("Cloning repository with git...")
        subprocess.run(["git", "clone", "https://github.com/native-toolkit/libtinyfiledialogs.git"], 
                       check=True)
    else:
        print("Git not available, downloading ZIP instead...")
        github_url = "https://github.com/native-toolkit/libtinyfiledialogs/archive/refs/heads/main.zip"
        download_file(github_url, "libtinyfiledialogs.zip")
        extract_zip("libtinyfiledialogs.zip", ".")
        os.rename("libtinyfiledialogs-main", "libtinyfiledialogs")
    
    # Verify the repository was cloned/downloaded
    if not os.path.exists("libtinyfiledialogs"):
        print("Error: Failed to obtain TinyFileDialogs source")
        sys.exit(1)
    
    # Copy required files
    required_files = [
        "tinyfiledialogs.h",
        "tinyfiledialogs.c",
        "LICENSE"
    ]
    
    for file in required_files:
        src = os.path.join("libtinyfiledialogs", file)
        dst = os.path.join("tinyfiledialogs", file)
        if os.path.exists(src):
            shutil.copy2(src, dst)
            print(f"Copied {file}")
        else:
            print(f"Warning: Could not find {file}")
    
    # Cleanup
    print("Cleaning up...")
    if os.path.exists("libtinyfiledialogs"):
        shutil.rmtree("libtinyfiledialogs")
    
    if not git_available and os.path.exists("libtinyfiledialogs.zip"):
        os.remove("libtinyfiledialogs.zip")
    
    print("TinyFileDialogs setup complete!")

def setup_sdl2_local():
    """Download and setup SDL2 locally"""
    sdl2_url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.2/SDL2-2.30.2.tar.gz"
    sdl2_ttf_url = "https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-2.20.2.tar.gz"
    sdl2_image_url = "https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz"

    # Create SDL directory
    sdl_dir = "sdl"
    if not os.path.exists(sdl_dir):
        os.makedirs(sdl_dir)

    print("\nSetting up SDL2 locally...")
    download_file(sdl2_url, "sdl2.tar.gz")
    extract_tar_gz("sdl2.tar.gz", sdl_dir)
    
    download_file(sdl2_ttf_url, "sdl2_ttf.tar.gz")
    extract_tar_gz("sdl2_ttf.tar.gz", sdl_dir)
    
    download_file(sdl2_image_url, "sdl2_image.tar.gz")
    extract_tar_gz("sdl2_image.tar.gz", sdl_dir)

    # Build SDL2
    sdl2_src = os.path.join(sdl_dir, "SDL2-2.30.2")
    print(f"Building SDL2 in {sdl2_src}...")
    os.chdir(sdl2_src)
    subprocess.run(["./configure", "--prefix", os.path.abspath("../local")], check=True)
    subprocess.run(["make", "-j", str(os.cpu_count())], check=True)
    subprocess.run(["make", "install"], check=True)
    os.chdir("../..")
    
    # Build SDL2_ttf
    sdl2_ttf_src = os.path.join(sdl_dir, "SDL2_ttf-2.20.2")
    print(f"Building SDL2_ttf in {sdl2_ttf_src}...")
    os.chdir(sdl2_ttf_src)
    subprocess.run(["./configure", "--prefix", os.path.abspath("../local"), 
                   "--with-sdl-prefix", os.path.abspath("../local")], check=True)
    subprocess.run(["make", "-j", str(os.cpu_count())], check=True)
    subprocess.run(["make", "install"], check=True)
    os.chdir("../..")
    
    # Build SDL2_image
    sdl2_image_src = os.path.join(sdl_dir, "SDL2_image-2.8.2")
    print(f"Building SDL2_image in {sdl2_image_src}...")
    os.chdir(sdl2_image_src)
    subprocess.run(["./configure", "--prefix", os.path.abspath("../local"), 
                   "--with-sdl-prefix", os.path.abspath("../local")], check=True)
    subprocess.run(["make", "-j", str(os.cpu_count())], check=True)
    subprocess.run(["make", "install"], check=True)
    os.chdir("../..")
    
    print("SDL2 local setup complete!")

def create_gitignore():
    """Create .gitignore to exclude dependencies"""
    gitignore_content = """# Dependencies (downloaded by setup.py)
imgui/
tinyfiledialogs/
sdl/

# Build artifacts
bin/
*.o
*.exe
*.a
*.la
*.lo
*.so

# Local installations
local/

# IDE files
.vscode/
*.swp
*.swo

# Other
imgui.ini
"""

    with open(".gitignore", "w") as f:
        f.write(gitignore_content)

def main():
    """Main setup function"""
    print("Setting up Paint project dependencies...")
    print("=" * 50)
    
    if os.path.exists("/etc/fedora-release"):
        print("Fedora system detected - applying compatibility fixes")

    if not os.path.exists("main.cpp"):
        print("Error: Run this script from the project root directory")
        sys.exit(1)

    setup_imgui()
    setup_tinyfiledialogs()
    
    # Ask about local SDL2 installation
    local_sdl = input("\nInstall SDL2 locally? (y/n): ").lower() == 'y'
    if local_sdl:
        setup_sdl2_local()
    
    create_gitignore()

    print("\n" + "=" * 50)
    print("Setup complete! Update your Makefile as follows:")
    print("")
    if local_sdl:
        print("For LOCAL SDL2:")
        print("  INCLUDES := -Isdl/local/include/SDL2 -Iimgui -Itinyfiledialogs")
        print("  LDFLAGS := -Lsdl/local/lib -Wl,-rpath,sdl/local/lib -lSDL2 -lSDL2_ttf -lSDL2_image -ldl -lm")
    else:
        print("For SYSTEM SDL2:")
        print("  INCLUDES := -I/usr/include/SDL2 -Iimgui -Itinyfiledialogs")
        print("  LDFLAGS := -lSDL2 -lSDL2_ttf -lSDL2_image -ldl -lm")
    print("")
    print("Fedora users may need to install build dependencies:")
    print("  sudo dnf install cmake gcc-c++ make libtool autoconf automake")
    print("=" * 50)

if __name__ == "__main__":
    main()