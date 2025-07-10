#!/usr/bin/env python3
"""
Setup script for Enough Image Editor
Downloads ImGui and TinyFileDialogs, plus tries to install SDL2 if you're on Linux

This is probably overkill but I got tired of people asking "why won't it compile"
"""

import os
import sys
import urllib.request
import zipfile
import shutil
import time
import subprocess
import platform
from pathlib import Path

def detect_linux_distro():
    """Figure out what Linux distro we're on so we can install SDL2"""
    try:
        with open('/etc/os-release', 'r') as f:
            content = f.read().lower()

        if 'ubuntu' in content or 'debian' in content:
            return 'ubuntu'
        elif 'fedora' in content or 'rhel' in content or 'centos' in content:
            return 'fedora'
        elif 'arch' in content or 'manjaro' in content:
            return 'arch'
        else:
            return 'unknown'
    except:
        return 'unknown'

def install_sdl2():
    """Try to install SDL2 if we're on a supported Linux distro"""
    if platform.system() != 'Linux':
        print("Not on Linux, skipping SDL2 auto-install")
        return False

    distro = detect_linux_distro()

    if distro == 'ubuntu':
        print("Detected Ubuntu/Debian - attempting to install SDL2...")
        cmd = ['sudo', 'apt', 'update']
        try:
            subprocess.run(cmd, check=True)
            cmd = ['sudo', 'apt', 'install', '-y', 'libsdl2-dev', 'libsdl2-ttf-dev', 'libsdl2-image-dev']
            subprocess.run(cmd, check=True)
            print("SDL2 installed successfully!")
            return True
        except subprocess.CalledProcessError:
            print("Failed to install SDL2 - you might need to do it manually")
            return False

    elif distro == 'fedora':
        print("Detected Fedora/RHEL - attempting to install SDL2...")
        cmd = ['sudo', 'dnf', 'install', '-y', 'SDL2-devel', 'SDL2_ttf-devel', 'SDL2_image-devel']
        try:
            subprocess.run(cmd, check=True)
            print("SDL2 installed successfully!")
            return True
        except subprocess.CalledProcessError:
            print("Failed to install SDL2 - you might need to do it manually")
            return False

    elif distro == 'arch':
        print("Detected Arch Linux - attempting to install SDL2...")
        cmd = ['sudo', 'pacman', '-S', '--noconfirm', 'sdl2', 'sdl2_ttf', 'sdl2_image']
        try:
            subprocess.run(cmd, check=True)
            print("SDL2 installed successfully!")
            return True
        except subprocess.CalledProcessError:
            print("Failed to install SDL2 - you might need to do it manually")
            return False

    else:
        print(f"Unknown Linux distro ({distro}) - can't auto-install SDL2")
        return False

def download_with_retries(url, filepath, max_retries=3):
    """Download a file, retry if it fails because internet is flaky"""
    print(f"Downloading {filepath.name}...")

    for attempt in range(max_retries):
        try:
            if attempt > 0:
                print(f"  Attempt {attempt + 1}/{max_retries}...")
                time.sleep(2)  # Give it a moment

            urllib.request.urlretrieve(url, filepath)
            print(f"Got {filepath.name}")
            return True

        except Exception as e:
            if attempt == max_retries - 1:
                print(f"Couldn't download {filepath.name} after {max_retries} tries")
                print(f"  Error: {e}")
                print(f"  You can try downloading manually from: {url}")
                return False
            else:
                print(f"  Failed: {e}")

    return False

def extract_zip_file(zip_path, extract_to):
    """Extract a zip file and hope it's not corrupted"""
    print(f"Extracting {zip_path.name}...")

    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
        print(f"Extracted {zip_path.name}")
        return True
    except zipfile.BadZipFile:
        print(f"{zip_path.name} is corrupted - try downloading again")
        return False
    except Exception as e:
        print(f"Extraction failed: {e}")
        return False

def cleanup_temp_files():
    """Remove any leftover zip files and temp directories"""
    project_dir = Path(__file__).parent

    # List of temp files/dirs to clean up
    temp_items = [
        'imgui_temp.zip',
        'tfd_temp.zip',
        'tinyfiledialogs.zip',  # The specific one you mentioned
        'temp_extract',
        'temp_extract_tfd'
    ]

    for item_name in temp_items:
        item_path = project_dir / item_name
        if item_path.exists():
            try:
                if item_path.is_dir():
                    shutil.rmtree(item_path)
                else:
                    item_path.unlink()
                print(f"  Cleaned up {item_name}")
            except Exception as e:
                print(f"  Warning: couldn't remove {item_name}: {e}")

def setup_imgui():
    """Download and setup Dear ImGui - this is required"""
    print("\n=== Setting up ImGui ===")

    project_dir = Path(__file__).parent
    imgui_dir = project_dir / "imgui"

    # Check if already set up
    if imgui_dir.exists() and (imgui_dir / "imgui.h").exists():
        print("ImGui already exists, skipping download")
        return True

    # Download ImGui
    imgui_url = "https://github.com/ocornut/imgui/archive/v1.90.1.zip"
    zip_file = project_dir / "imgui_temp.zip"

    if not download_with_retries(imgui_url, zip_file):
        return False

    # Extract
    temp_dir = project_dir / "temp_extract"
    if not extract_zip_file(zip_file, temp_dir):
        zip_file.unlink()
        return False

    # Set up the imgui folder
    if imgui_dir.exists():
        shutil.rmtree(imgui_dir)

    imgui_dir.mkdir()

    # Copy files from the extracted folder
    extracted_imgui = temp_dir / "imgui-1.90.1"

    # Copy core ImGui files
    for file in extracted_imgui.glob("*.cpp"):
        shutil.copy2(file, imgui_dir)
    for file in extracted_imgui.glob("*.h"):
        shutil.copy2(file, imgui_dir)

    # Copy SDL2 backends (we need these specific ones)
    backends_dir = extracted_imgui / "backends"
    needed_backends = [
        "imgui_impl_sdl2.cpp",
        "imgui_impl_sdl2.h",
        "imgui_impl_sdlrenderer2.cpp",
        "imgui_impl_sdlrenderer2.h"
    ]

    for backend in needed_backends:
        src = backends_dir / backend
        if src.exists():
            shutil.copy2(src, imgui_dir)

    # Clean up
    shutil.rmtree(temp_dir)
    zip_file.unlink()

    print("ImGui setup complete")
    return True

def setup_tinyfiledialogs():
    """Download and setup TinyFileDialogs - this is optional but nice to have"""
    print("\n=== Setting up TinyFileDialogs ===")

    project_dir = Path(__file__).parent
    tfd_dir = project_dir / "tinyfiledialogs"

    # Check if already set up
    if tfd_dir.exists() and (tfd_dir / "tinyfiledialogs.h").exists():
        print("TinyFileDialogs already exists, skipping download")
        return True

    # Download TinyFileDialogs
    tfd_url = "https://sourceforge.net/projects/tinyfiledialogs/files/tinyfiledialogs.zip/download"
    zip_file = project_dir / "tfd_temp.zip"

    if not download_with_retries(tfd_url, zip_file):
        print("  TinyFileDialogs is optional, continuing without it...")
        return False

    # Extract
    temp_dir = project_dir / "temp_extract_tfd"
    if not extract_zip_file(zip_file, temp_dir):
        zip_file.unlink()
        return False

    # Find the actual source files (sourceforge zips are weird)
    tfd_source = None
    for item in temp_dir.rglob("tinyfiledialogs.c"):
        tfd_source = item.parent
        break

    if not tfd_source:
        print("Couldn't find tinyfiledialogs source in the archive")
        shutil.rmtree(temp_dir)
        zip_file.unlink()
        return False

    # Set up the tinyfiledialogs folder
    if tfd_dir.exists():
        shutil.rmtree(tfd_dir)

    tfd_dir.mkdir()

    # Copy the source files
    for file in tfd_source.glob("tinyfiledialogs.*"):
        shutil.copy2(file, tfd_dir)

    # Clean up
    shutil.rmtree(temp_dir)
    zip_file.unlink()

    print("TinyFileDialogs setup complete")
    return True

def main():
    print("Enough Image Editor Setup")
    print("=" * 30)
    print("This script will download ImGui and TinyFileDialogs,")
    print("plus try to install SDL2 if you're on Linux.")
    print("If something breaks, you can probably fix it manually.\n")

    # Make sure we have a bin directory
    bin_dir = Path(__file__).parent / "bin"
    bin_dir.mkdir(exist_ok=True)

    # Clean up any leftover temp files first
    print("Cleaning up any leftover temp files...")
    cleanup_temp_files()

    # Try to install SDL2 first
    print("\n=== Checking SDL2 ===")
    sdl_installed = install_sdl2()

    # Set up dependencies
    imgui_ok = setup_imgui()
    tfd_ok = setup_tinyfiledialogs()

    # Final cleanup
    print("\nCleaning up temp files...")
    cleanup_temp_files()

    # Results
    print("\n" + "=" * 40)
    if imgui_ok:
        print("Setup completed!")
        print("  - ImGui: OK")
        print(f"  - TinyFileDialogs: {'OK' if tfd_ok else 'FAILED (optional)'}")
        print(f"  - SDL2: {'OK' if sdl_installed else '? (check manually)'}")

        print("\nNext steps:")
        print("1. Run 'make' to build the project")
        print("2. Run './bin/EnoughImageEditor' to start")

        if not sdl_installed:
            print("\nIf the build fails, you might need to install SDL2 manually:")
            print("  Ubuntu/Debian: sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev")
            print("  Fedora/RHEL: sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel")
            print("  Arch: sudo pacman -S sdl2 sdl2_ttf sdl2_image")
            print("  macOS: brew install sdl2 sdl2_ttf sdl2_image")

        return 0
    else:
        print("Setup failed - ImGui is required")
        print("Try running again or download manually from:")
        print("https://github.com/ocornut/imgui/archive/v1.90.1.zip")
        return 1

if __name__ == "__main__":
    sys.exit(main())
