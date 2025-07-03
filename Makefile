# Corrected Makefile
CXX := clang++
TARGET := bin/paint
SOURCES := main.cpp \
           tinyfiledialogs/tinyfiledialogs.c \
           imgui/imgui.cpp \
           imgui/imgui_draw.cpp \
           imgui/imgui_tables.cpp \
           imgui/imgui_widgets.cpp \
           imgui/imgui_impl_sdl2.cpp \
           imgui/imgui_impl_sdlrenderer2.cpp

INCLUDES := -I/usr/include/SDL2 -Iimgui -Itinyfiledialogs
CXXFLAGS := -std=c++17 -Wall -Wextra
LDFLAGS := -lSDL2 -lSDL2_ttf -lSDL2_image -ldl

# Fedora-specific adjustments (tested on Fedora 38)
ifeq ($(shell grep -q 'Fedora' /etc/os-release; echo $$?),0)
    INCLUDES += -I/usr/include/SDL2
    LDFLAGS += -lm
endif

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
