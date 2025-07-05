#!/usr/bin/env python3
import os
import sys
import platform
import subprocess
import urllib.request
import zipfile
import shutil
from pathlib import Path

class EnoughSetup:
    def __init__(self):
        self.project_root = Path(__file__).parent
        self.system = platform.system().lower()

        self.dependencies = {
            "imgui": {
                "url": "https://github.com/ocornut/imgui/archive/v1.90.1.zip",
                "extract_name": "imgui-1.90.1"
            },
            "tinyfiledialogs": {
                "url": "https://github.com/native-toolkit/tinyfiledialogs/archive/refs/tags/v3.15.1.zip",
                "extract_name": "tinyfiledialogs-3.15.1"
            }
        }

        self.fonts = [
            "https://github.com/google/fonts/raw/main/apache/roboto/Roboto-Regular.ttf",
            "https://github.com/google/fonts/raw/main/apache/opensans/OpenSans-Regular.ttf",
            "https://github.com/google/fonts/raw/main/ofl/lato/Lato-Regular.ttf"
        ]

    def download_file(self, url, filename):
        print(f"Downloading {filename}...")
        try:
            urllib.request.urlretrieve(url, filename)
            print(f"Downloaded {filename}")
            return True
        except Exception as e:
            print(f"Failed to download {filename}: {e}")
            return False

    def extract_archive(self, archive_path, extract_to):
        print(f"Extracting {archive_path.name}...")
        try:
            with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                zip_ref.extractall(extract_to)
            print(f"Extracted {archive_path.name}")
            return True
        except Exception as e:
            print(f"Failed to extract {archive_path.name}: {e}")
            return False

    def setup_imgui(self):
        print("Setting up ImGui...")
        imgui_zip = self.project_root / "imgui.zip"

        if not self.download_file(self.dependencies["imgui"]["url"], imgui_zip):
            return False

        if not self.extract_archive(imgui_zip, self.project_root):
            return False

        extracted_dir = self.project_root / self.dependencies["imgui"]["extract_name"]
        target_dir = self.project_root / "imgui"

        if target_dir.exists():
            shutil.rmtree(target_dir)

        # Move core imgui files
        target_dir.mkdir()
        for file in extracted_dir.glob("*.cpp"):
            shutil.move(str(file), str(target_dir))
        for file in extracted_dir.glob("*.h"):
            shutil.move(str(file), str(target_dir))

        # Move backend files
        backends_src = extracted_dir / "backends"
        if backends_src.exists():
            for backend_file in backends_src.glob("*"):
                if "sdl2" in backend_file.name.lower() or "opengl" in backend_file.name.lower():
                    shutil.move(str(backend_file), str(target_dir))

        shutil.rmtree(extracted_dir)
        imgui_zip.unlink()
        print("ImGui setup complete")
        return True

    def setup_tinyfiledialogs(self):
        print("Setting up TinyFileDialogs...")
        tfd_zip = self.project_root / "tinyfiledialogs.zip"

        if not self.download_file(self.dependencies["tinyfiledialogs"]["url"], tfd_zip):
            return False

        if not self.extract_archive(tfd_zip, self.project_root):
            return False

        extracted_dir = self.project_root / self.dependencies["tinyfiledialogs"]["extract_name"]
        target_dir = self.project_root / "tinyfiledialogs"

        if target_dir.exists():
            shutil.rmtree(target_dir)

        target_dir.mkdir()
        for file in extracted_dir.glob("*.c"):
            shutil.move(str(file), str(target_dir))
        for file in extracted_dir.glob("*.h"):
            shutil.move(str(file), str(target_dir))

        shutil.rmtree(extracted_dir)
        tfd_zip.unlink()
        print("TinyFileDialogs setup complete")
        return True

    def setup_fonts(self):
        print("Downloading fonts...")
        fonts_dir = self.project_root / "fonts"
        fonts_dir.mkdir(exist_ok=True)

        for url in self.fonts:
            filename = fonts_dir / url.split("/")[-1]
            self.download_file(url, filename)

        print("Fonts download complete")

    def setup_sdl2_windows(self):
        print("Setting up SDL2 for Windows...")
        sdl_url = "https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-devel-2.28.5-VC.zip"
        sdl_zip = self.project_root / "sdl2.zip"

        if not self.download_file(sdl_url, sdl_zip):
            return False

        if not self.extract_archive(sdl_zip, self.project_root):
            return False

        extracted_dir = self.project_root / "SDL2-2.28.5"
        target_dir = self.project_root / "sdl2"

        if target_dir.exists():
            shutil.rmtree(target_dir)

        target_dir.mkdir()

        # Copy includes
        include_src = extracted_dir / "include"
        include_dst = target_dir / "include"
        if include_src.exists():
            shutil.copytree(include_src, include_dst)

        # Copy libs
        lib_src = extracted_dir / "lib"
        lib_dst = target_dir / "lib"
        if lib_src.exists():
            shutil.copytree(lib_src, lib_dst)

        shutil.rmtree(extracted_dir)
        sdl_zip.unlink()
        print("SDL2 setup complete")
        return True

    def install_linux_dependencies(self):
        print("Installing Linux dependencies...")

        # Detect Linux distribution
        try:
            with open('/etc/os-release', 'r') as f:
                os_info = f.read().lower()
        except:
            os_info = ""

        if 'ubuntu' in os_info or 'debian' in os_info:
            cmd = ["sudo", "apt-get", "update", "&&", "sudo", "apt-get", "install", "-y",
                   "libsdl2-dev", "libgl1-mesa-dev", "libglew-dev", "build-essential"]
        elif 'fedora' in os_info or 'rhel' in os_info:
            cmd = ["sudo", "dnf", "install", "-y", "SDL2-devel", "mesa-libGL-devel",
                   "glew-devel", "gcc-c++", "make"]
        elif 'arch' in os_info:
            cmd = ["sudo", "pacman", "-S", "--noconfirm", "sdl2", "mesa", "glew", "base-devel"]
        else:
            print("Unknown Linux distribution. Please install SDL2, OpenGL, and build tools manually.")
            return False

        try:
            subprocess.run(" ".join(cmd), shell=True, check=True)
            print("Linux dependencies installed")
            return True
        except subprocess.CalledProcessError:
            print("Failed to install dependencies. Please install manually.")
            return False

    def check_make(self):
        try:
            subprocess.run(["make", "--version"], capture_output=True, check=True)
            return True
        except:
            return False

    def install_make(self):
        if self.system == "windows":
            print("Make not found. Please install MinGW or Visual Studio Build Tools.")
            print("Or use the build script in the bin directory.")
            return False
        else:
            print("Make not found. Installing...")
            if self.system == "linux":
                return self.install_linux_dependencies()
            else:
                print("Please install make manually for your system.")
                return False

    def update_makefile(self):
        makefile_path = self.project_root / "Makefile"
        if not makefile_path.exists():
            print("Makefile not found, skipping update")
            return

        print("Updating Makefile for SDL2 paths...")

        # Simple makefile update for Windows SDL2 paths
        if self.system == "windows":
            with open(makefile_path, 'r') as f:
                content = f.read()

            # Update include and lib paths
            content = content.replace("-I/usr/include/SDL2", "-Isdl2/include")
            content = content.replace("-lSDL2", "-Lsdl2/lib/x64 -lSDL2 -lSDL2main")

            with open(makefile_path, 'w') as f:
                f.write(content)
            print("Makefile updated for Windows SDL2")

    def run_setup(self):
        print("Enough Project Setup")
        print("=" * 40)
        print(f"Platform: {platform.system()}")
        print()

        # Create directories
        for dir_name in ["fonts", "bin"]:
            (self.project_root / dir_name).mkdir(exist_ok=True)

        # Setup dependencies
        if not self.setup_imgui():
            return False
        if not self.setup_tinyfiledialogs():
            return False

        self.setup_fonts()

        # Platform-specific setup
        if self.system == "linux":
            self.install_linux_dependencies()
        elif self.system == "windows":
            self.setup_sdl2_windows()
            self.update_makefile()

        # Check for make
        if not self.check_make():
            self.install_make()

        print("\nSetup complete! Run 'make' to build the project.")
        return True

def main():
    setup = EnoughSetup()
    if setup.run_setup():
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
