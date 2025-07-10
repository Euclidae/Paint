# Paint Makefile - Updated for OOP architecture
CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++20
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image -lpthread -lGL

# Directories (imgui and tinyfiledialogs are in current directory)
SRC_DIR = .
IMGUI_DIR = imgui
TFD_DIR = tinyfiledialogs
BIN_DIR = bin

# Include paths
INCLUDES = -I$(IMGUI_DIR) -I$(TFD_DIR) -I$(SRC_DIR) -Icanvas -Itools -Ieditor -Iui

# Try to use sdl2-config if available, otherwise use direct flags
SDL2_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || echo "-I/usr/include/SDL2 -D_REENTRANT")
SDL2_LIBS := $(shell sdl2-config --libs 2>/dev/null || echo "-lSDL2")

# Source files
SOURCES = main.cpp canvas/Canvas.cpp canvas/Layer.cpp tools/ToolManager.cpp editor/Editor.cpp ui/UI.cpp
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/imgui_impl_sdl2.cpp $(IMGUI_DIR)/imgui_impl_sdlrenderer2.cpp
TFD_SOURCES = $(TFD_DIR)/tinyfiledialogs.c

# Target
TARGET = $(BIN_DIR)/EnoughImageEditor

.PHONY: all clean install deps-check deps-install help force

all: deps-check $(TARGET)

$(TARGET): $(SOURCES) $(IMGUI_SOURCES) $(TFD_SOURCES)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SDL2_CFLAGS) $(SOURCES) $(IMGUI_SOURCES) $(TFD_SOURCES) -o $(TARGET) $(SDL2_LIBS) $(LIBS)

clean:
	rm -rf $(BIN_DIR) *.o

install: all
	sudo cp $(TARGET) /usr/local/bin/

# Flexible dependency checking that works on different distros
deps-check:
	@echo "Checking dependencies..."
	@if ! test -d $(IMGUI_DIR); then \
		echo "ERROR: ImGui not found in $(IMGUI_DIR)"; \
		echo "ImGui directory missing - please ensure imgui/ folder exists"; \
		exit 1; \
	fi
	@if ! test -d $(TFD_DIR); then \
		echo "ERROR: TinyFileDialogs not found in $(TFD_DIR)"; \
		echo "TinyFileDialogs directory missing - please ensure tinyfiledialogs/ folder exists"; \
		exit 1; \
	fi
	@if ! command -v sdl2-config >/dev/null 2>&1; then \
		echo "WARNING: sdl2-config not found, using fallback flags"; \
		if ! ldconfig -p 2>/dev/null | grep -q libSDL2 && ! ls /usr/lib*/libSDL2* >/dev/null 2>&1 && ! ls /usr/local/lib*/libSDL2* >/dev/null 2>&1; then \
			echo "ERROR: SDL2 library not found"; \
			echo "Install with:"; \
			echo "  Fedora/RHEL: sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel"; \
			echo "  Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev"; \
			echo "  Arch: sudo pacman -S sdl2 sdl2_ttf sdl2_image"; \
			exit 1; \
		fi; \
	else \
		echo "SDL2 found via sdl2-config"; \
	fi
	@if ! ldconfig -p 2>/dev/null | grep -q libSDL2_ttf && ! ls /usr/lib*/libSDL2_ttf* >/dev/null 2>&1 && ! ls /usr/local/lib*/libSDL2_ttf* >/dev/null 2>&1; then \
		echo "ERROR: SDL2_ttf library not found"; \
		echo "Install with:"; \
		echo "  Fedora/RHEL: sudo dnf install SDL2_ttf-devel"; \
		echo "  Ubuntu/Debian: sudo apt-get install libsdl2-ttf-dev"; \
		echo "  Arch: sudo pacman -S sdl2_ttf"; \
		exit 1; \
	fi
	@if ! ldconfig -p 2>/dev/null | grep -q libSDL2_image && ! ls /usr/lib*/libSDL2_image* >/dev/null 2>&1 && ! ls /usr/local/lib*/libSDL2_image* >/dev/null 2>&1; then \
		echo "ERROR: SDL2_image library not found"; \
		echo "Install with:"; \
		echo "  Fedora/RHEL: sudo dnf install SDL2_image-devel"; \
		echo "  Ubuntu/Debian: sudo apt-get install libsdl2-image-dev"; \
		echo "  Arch: sudo pacman -S sdl2_image"; \
		exit 1; \
	fi
	@echo "All dependencies found!"

# Quick dependency installation for common distros
deps-install:
	@echo "Attempting to install SDL2 dependencies..."
	@if command -v dnf >/dev/null 2>&1; then \
		echo "Detected Fedora/RHEL, installing with dnf..."; \
		sudo dnf install -y SDL2-devel SDL2_ttf-devel SDL2_image-devel gcc-c++ make; \
	elif command -v apt-get >/dev/null 2>&1; then \
		echo "Detected Ubuntu/Debian, installing with apt..."; \
		sudo apt-get update && sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev build-essential; \
	elif command -v pacman >/dev/null 2>&1; then \
		echo "Detected Arch Linux, installing with pacman..."; \
		sudo pacman -S --needed sdl2 sdl2_ttf sdl2_image base-devel; \
	elif command -v zypper >/dev/null 2>&1; then \
		echo "Detected openSUSE, installing with zypper..."; \
		sudo zypper install -y libSDL2-devel libSDL2_ttf-devel libSDL2_image-devel gcc-c++ make; \
	else \
		echo "Unknown package manager. Please install manually:"; \
		echo "- SDL2 development libraries"; \
		echo "- SDL2_ttf development libraries"; \
		echo "- SDL2_image development libraries"; \
		echo "- C++ compiler (g++ or clang++)"; \
		echo "- make"; \
	fi

# Build without dependency checks (for CI or when you know deps are installed)
force: $(TARGET)

help:
	@echo "Paint Build System"
	@echo "=================="
	@echo "make all         - Build the project (default)"
	@echo "make clean       - Clean build files"
	@echo "make install     - Install to /usr/local/bin"
	@echo "make deps-check  - Check dependencies"
	@echo "make deps-install- Auto-install dependencies for your distro"
	@echo "make force       - Build without dependency checks"
	@echo "make help        - Show this help"
	@echo ""
	@echo "For Fedora users:"
	@echo "sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel"
	@echo ""
	@echo "Quick start:"
	@echo "1. make deps-install  # (install SDL2 if needed)"
	@echo "2. make all           # (build the project)"
	@echo "3. ./bin/EnoughImageEditor        # (run the application)"
