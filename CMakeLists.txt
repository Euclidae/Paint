cmake_minimum_required(VERSION 3.12)
project(EnoughImageEditor VERSION 1.0.2 LANGUAGES C CXX)

# Enough Image Editor - A graphics editor built with SDL2 and Dear ImGui
# Features layer management, drawing tools, and image filters
# Originally procedural, later refactored to object-oriented architecture
#
# Project Status: Complete and no longer under active development
# Many features are known to be broken or unreliable
#
# Repository: Paint (folder name)
# Executable: EnoughImageEditor
# Build Systems: CMake (primary), Make (alternative)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Disable WebP if having issues - use CACHE FORCE to override
set(SDL2_IMAGE_WEBP OFF CACHE BOOL "Enable WebP support" FORCE)

# Find required packages
find_package(PkgConfig REQUIRED)

# For MSYS2/Windows, use pkg-config directly to avoid linking issues
if(WIN32)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
else()
    # Try to find SDL2 using different methods for other platforms
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        message(STATUS "SDL2 not found via CMake, trying pkg-config")
        pkg_check_modules(SDL2 REQUIRED sdl2)
        pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
        pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    else()
        message(STATUS "SDL2 found via CMake")
        find_package(SDL2_ttf REQUIRED)
        find_package(SDL2_image REQUIRED)
    endif()
endif()

# Find OpenGL for ImGui
find_package(OpenGL REQUIRED)

# Check for pthread
find_package(Threads REQUIRED)

# Source files organized by component
set(CANVAS_SOURCES
    canvas/Canvas.cpp
    canvas/Layer.cpp
)

set(TOOLS_SOURCES
    tools/ToolManager.cpp
)

set(EDITOR_SOURCES
    editor/Editor.cpp
)

set(UI_SOURCES
    ui/UI.cpp
)

# ImGui source files
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_impl_sdl2.cpp
    imgui/imgui_impl_sdlrenderer2.cpp
)

# TinyFileDialogs
set(TFD_SOURCES
    tinyfiledialogs/tinyfiledialogs.c
)

# All source files
set(ALL_SOURCES
    main.cpp
    ${CANVAS_SOURCES}
    ${TOOLS_SOURCES}
    ${EDITOR_SOURCES}
    ${UI_SOURCES}
    ${IMGUI_SOURCES}
    ${TFD_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/editor
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs
)

# Handle MSYS2 paths specifically
if(CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    # Add MSYS2 specific include paths
    target_include_directories(${PROJECT_NAME} PRIVATE
        /ucrt64/include/SDL2
    )

    # Add MSYS2 specific link directories
    target_link_directories(${PROJECT_NAME} PRIVATE
        /ucrt64/lib
    )
endif()

# Link libraries
if(WIN32)
    # For Windows/MSYS2, always use pkg-config to avoid linking issues
    # and ensure proper header paths for SDL2/ convention
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        OpenGL::GL
        Threads::Threads
    )
    target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER})
elseif(SDL2_FOUND AND TARGET SDL2::SDL2)
    # Modern SDL2 target for other platforms
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2::SDL2main
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        SDL2_image::SDL2_image
        OpenGL::GL
        Threads::Threads
    )
else()
    # Fallback to pkg-config
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        OpenGL::GL
        Threads::Threads
    )
    target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER})
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    endif()
elseif(APPLE)
    # macOS-specific settings
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
else()
    # Linux-specific settings
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()

# Custom target for running setup.py
add_custom_target(setup
    COMMAND ${CMAKE_COMMAND} -E env python3 ${CMAKE_SOURCE_DIR}/setup.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running setup.py to download dependencies"
)

# Make setup a dependency of the main target if ImGui doesn't exist
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/imgui/imgui.h)
    add_dependencies(${PROJECT_NAME} setup)
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Custom targets for convenience
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Enough Image Editor"
)

# Copy SDL2 DLLs to output directory on Windows
if(WIN32)
    # Define DLL paths
    set(MSYS2_BIN_DIR "C:/msys64/ucrt64/bin")
    set(DLLS_TO_COPY
        "${MSYS2_BIN_DIR}/SDL2.dll"
        "${MSYS2_BIN_DIR}/SDL2_ttf.dll"
        "${MSYS2_BIN_DIR}/SDL2_image.dll"
    )

    # Copy DLLs after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL2 DLLs..."
        COMMENT "Copying required DLLs to output directory"
    )

    foreach(DLL ${DLLS_TO_COPY})
        if(EXISTS ${DLL})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
endif()
