# SDL and ImGui Functions Reference

This document explains all SDL2 and Dear ImGui functions used in the Enough Image Editor project.

## SDL Core Functions

### SDL_Init(Uint32 flags)
**Purpose**: Initializes SDL subsystems
**Parameters**:
- `flags`: Subsystem flags to initialize
  - `SDL_INIT_VIDEO`: Video subsystem
  - `SDL_INIT_AUDIO`: Audio subsystem
  - `SDL_INIT_TIMER`: Timer subsystem
**Returns**: 0 on success, negative on error
**Usage**: `SDL_Init(SDL_INIT_VIDEO)`

### SDL_Quit()
**Purpose**: Cleans up all initialized SDL subsystems
**Parameters**: None
**Returns**: void
**Usage**: Called at program exit

### SDL_GetError()
**Purpose**: Gets the last error message set by SDL
**Parameters**: None
**Returns**: `const char*` - Error message string
**Usage**: `std::cerr << SDL_GetError()`

## SDL Window Functions

### SDL_CreateWindow(title, x, y, w, h, flags)
**Purpose**: Creates a window
**Parameters**:
- `title`: Window title string
- `x, y`: Window position (use `SDL_WINDOWPOS_CENTERED` for centering)
- `w, h`: Window dimensions in pixels
- `flags`: Window creation flags
  - `SDL_WINDOW_SHOWN`: Window is visible
  - `SDL_WINDOW_RESIZABLE`: Window can be resized
  - `SDL_WINDOW_FULLSCREEN`: Fullscreen window
**Returns**: `SDL_Window*` pointer or NULL on failure
**Usage**: `SDL_CreateWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN)`

### SDL_DestroyWindow(SDL_Window* window)
**Purpose**: Destroys a window
**Parameters**: `window` - Window to destroy
**Returns**: void
**Usage**: `SDL_DestroyWindow(window)`

## SDL Renderer Functions

### SDL_CreateRenderer(window, index, flags)
**Purpose**: Creates a rendering context for a window
**Parameters**:
- `window`: Target window
- `index`: Index of rendering driver (-1 for first supporting requested flags)
- `flags`: Renderer flags
  - `SDL_RENDERER_ACCELERATED`: Use hardware acceleration
  - `SDL_RENDERER_PRESENTVSYNC`: Enable VSync
**Returns**: `SDL_Renderer*` pointer or NULL on failure

### SDL_DestroyRenderer(SDL_Renderer* renderer)
**Purpose**: Destroys a renderer
**Parameters**: `renderer` - Renderer to destroy
**Returns**: void

### SDL_SetRenderTarget(renderer, texture)
**Purpose**: Sets render target (NULL for default framebuffer)
**Parameters**:
- `renderer`: The renderer
- `texture`: Target texture or NULL for default
**Returns**: 0 on success, negative on error

### SDL_GetRenderTarget(SDL_Renderer* renderer)
**Purpose**: Gets current render target
**Parameters**: `renderer` - The renderer
**Returns**: `SDL_Texture*` - Current target or NULL if default

### SDL_SetRenderDrawColor(renderer, r, g, b, a)
**Purpose**: Sets color for drawing operations
**Parameters**:
- `renderer`: The renderer
- `r, g, b, a`: Red, green, blue, alpha values (0-255)
**Returns**: 0 on success, negative on error

### SDL_SetRenderDrawBlendMode(renderer, blendMode)
**Purpose**: Sets blend mode for drawing operations
**Parameters**:
- `renderer`: The renderer
- `blendMode`: Blend mode enum
  - `SDL_BLENDMODE_NONE`: No blending
  - `SDL_BLENDMODE_BLEND`: Alpha blending
  - `SDL_BLENDMODE_ADD`: Additive blending
  - `SDL_BLENDMODE_MOD`: Color modulation
**Returns**: 0 on success, negative on error

### SDL_RenderClear(SDL_Renderer* renderer)
**Purpose**: Clears the current render target with draw color
**Parameters**: `renderer` - The renderer
**Returns**: 0 on success, negative on error

### SDL_RenderCopy(renderer, texture, srcrect, dstrect)
**Purpose**: Copies texture to render target
**Parameters**:
- `renderer`: The renderer
- `texture`: Source texture
- `srcrect`: Source rectangle (NULL for entire texture)
- `dstrect`: Destination rectangle (NULL for entire target)
**Returns**: 0 on success, negative on error

### SDL_RenderCopyEx(renderer, texture, srcrect, dstrect, angle, center, flip)
**Purpose**: Copies texture with rotation and flipping
**Parameters**:
- `renderer`: The renderer
- `texture`: Source texture
- `srcrect`: Source rectangle
- `dstrect`: Destination rectangle
- `angle`: Rotation angle in degrees
- `center`: Point around which to rotate
- `flip`: Flip flags (`SDL_FLIP_NONE`, `SDL_FLIP_HORIZONTAL`, `SDL_FLIP_VERTICAL`)
**Returns**: 0 on success, negative on error

### SDL_RenderDrawRect(renderer, rect)
**Purpose**: Draws rectangle outline
**Parameters**:
- `renderer`: The renderer
- `rect`: Rectangle to draw
**Returns**: 0 on success, negative on error

### SDL_RenderFillRect(renderer, rect)
**Purpose**: Fills a rectangle
**Parameters**:
- `renderer`: The renderer
- `rect`: Rectangle to fill
**Returns**: 0 on success, negative on error

### SDL_RenderPresent(SDL_Renderer* renderer)
**Purpose**: Updates screen with rendering
**Parameters**: `renderer` - The renderer
**Returns**: void

### SDL_RenderFlush(SDL_Renderer* renderer)
**Purpose**: Forces immediate rendering of queued commands
**Parameters**: `renderer` - The renderer
**Returns**: 0 on success, negative on error

### SDL_RenderSetScale(renderer, scaleX, scaleY)
**Purpose**: Sets device independent resolution for rendering
**Parameters**:
- `renderer`: The renderer
- `scaleX, scaleY`: Scale factors
**Returns**: 0 on success, negative on error

### SDL_RenderReadPixels(renderer, rect, format, pixels, pitch)
**Purpose**: Reads pixels from render target
**Parameters**:
- `renderer`: The renderer
- `rect`: Area to read (NULL for entire target)
- `format`: Pixel format
- `pixels`: Buffer to store pixels
- `pitch`: Length of pixel row in bytes
**Returns**: 0 on success, negative on error

## SDL Texture Functions

### SDL_CreateTexture(renderer, format, access, w, h)
**Purpose**: Creates a texture
**Parameters**:
- `renderer`: The renderer
- `format`: Pixel format (e.g., `SDL_PIXELFORMAT_RGBA8888`)
- `access`: Texture access patterns
  - `SDL_TEXTUREACCESS_STATIC`: Changes rarely, not lockable
  - `SDL_TEXTUREACCESS_STREAMING`: Changes frequently, lockable
  - `SDL_TEXTUREACCESS_TARGET`: Can be used as render target
- `w, h`: Texture dimensions
**Returns**: `SDL_Texture*` pointer or NULL on failure

### SDL_CreateTextureFromSurface(renderer, surface)
**Purpose**: Creates texture from surface
**Parameters**:
- `renderer`: The renderer
- `surface`: Source surface
**Returns**: `SDL_Texture*` pointer or NULL on failure

### SDL_DestroyTexture(SDL_Texture* texture)
**Purpose**: Destroys a texture
**Parameters**: `texture` - Texture to destroy
**Returns**: void

### SDL_QueryTexture(texture, format, access, w, h)
**Purpose**: Gets texture attributes
**Parameters**:
- `texture`: The texture
- `format`: Pointer to store format (can be NULL)
- `access`: Pointer to store access (can be NULL)
- `w, h`: Pointers to store dimensions (can be NULL)
**Returns**: 0 on success, negative on error

### SDL_SetTextureBlendMode(texture, blendMode)
**Purpose**: Sets texture blend mode
**Parameters**:
- `texture`: The texture
- `blendMode`: Blend mode enum
**Returns**: 0 on success, negative on error

### SDL_SetTextureAlphaMod(texture, alpha)
**Purpose**: Sets texture alpha modulation
**Parameters**:
- `texture`: The texture
- `alpha`: Alpha value (0-255)
**Returns**: 0 on success, negative on error

### SDL_SetTextureColorMod(texture, r, g, b)
**Purpose**: Sets texture color modulation
**Parameters**:
- `texture`: The texture
- `r, g, b`: Color values (0-255)
**Returns**: 0 on success, negative on error

## SDL Surface Functions

### SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask)
**Purpose**: Creates an RGB surface
**Parameters**:
- `flags`: Surface flags (usually 0)
- `width, height`: Surface dimensions
- `depth`: Bits per pixel
- `Rmask, Gmask, Bmask, Amask`: Pixel format masks
**Returns**: `SDL_Surface*` pointer or NULL on failure

### SDL_FreeSurface(SDL_Surface* surface)
**Purpose**: Frees surface memory
**Parameters**: `surface` - Surface to free
**Returns**: void

### SDL_ConvertSurfaceFormat(surface, format, flags)
**Purpose**: Converts surface to specified pixel format
**Parameters**:
- `surface`: Source surface
- `format`: Target pixel format
- `flags`: Conversion flags (usually 0)
**Returns**: `SDL_Surface*` pointer or NULL on failure

### SDL_LockSurface(SDL_Surface* surface)
**Purpose**: Locks surface for direct pixel access
**Parameters**: `surface` - Surface to lock
**Returns**: 0 on success, negative on error

### SDL_UnlockSurface(SDL_Surface* surface)
**Purpose**: Unlocks previously locked surface
**Parameters**: `surface` - Surface to unlock
**Returns**: void

## SDL Color Functions

### SDL_MapRGBA(format, r, g, b, a)
**Purpose**: Maps RGBA values to pixel value
**Parameters**:
- `format`: Pixel format
- `r, g, b, a`: Color component values
**Returns**: `Uint32` pixel value

### SDL_GetRGBA(pixel, format, r, g, b, a)
**Purpose**: Gets RGBA values from pixel
**Parameters**:
- `pixel`: Pixel value
- `format`: Pixel format
- `r, g, b, a`: Pointers to store color components
**Returns**: void

## SDL Event Functions

### SDL_PollEvent(SDL_Event* event)
**Purpose**: Polls for pending events
**Parameters**: `event` - Pointer to store event data
**Returns**: 1 if event pending, 0 if none

### SDL_Event Structure
**Purpose**: Contains event information
**Key Fields**:
- `type`: Event type (`SDL_QUIT`, `SDL_MOUSEBUTTONDOWN`, etc.)
- `button`: Mouse button event data
  - `x, y`: Mouse coordinates
  - `button`: Button index
- `motion`: Mouse motion event data
  - `x, y`: Mouse coordinates
  - `xrel, yrel`: Relative motion

### SDL_Point Structure
**Purpose**: Represents a 2D point
**Fields**:
- `x`: X coordinate
- `y`: Y coordinate

### SDL_Rect Structure
**Purpose**: Represents a rectangle
**Fields**:
- `x, y`: Top-left corner coordinates
- `w, h`: Width and height

### SDL_PointInRect(point, rect)
**Purpose**: Tests if point is inside rectangle
**Parameters**:
- `point`: Point to test
- `rect`: Rectangle to test against
**Returns**: `SDL_bool` (true if inside)

## SDL Utility Functions

### SDL_Delay(Uint32 ms)
**Purpose**: Waits specified milliseconds
**Parameters**: `ms` - Milliseconds to wait
**Returns**: void

### SDL_ComposeCustomBlendMode(srcColorFactor, dstColorFactor, colorOperation, srcAlphaFactor, dstAlphaFactor, alphaOperation)
**Purpose**: Creates custom blend mode
**Parameters**: Various blend factor enums
**Returns**: `SDL_BlendMode` custom blend mode

## SDL_image Functions

### IMG_Init(int flags)
**Purpose**: Initializes SDL_image
**Parameters**: `flags` - Image format flags (`IMG_INIT_PNG`, `IMG_INIT_JPG`)
**Returns**: Flags successfully initialized

### IMG_Quit()
**Purpose**: Cleans up SDL_image
**Parameters**: None
**Returns**: void

### IMG_Load(const char* file)
**Purpose**: Loads image from file
**Parameters**: `file` - File path
**Returns**: `SDL_Surface*` pointer or NULL on failure

### IMG_GetError()
**Purpose**: Gets SDL_image error message
**Parameters**: None
**Returns**: `const char*` error string

## SDL_ttf Functions

### TTF_Init()
**Purpose**: Initializes SDL_ttf
**Parameters**: None
**Returns**: 0 on success, -1 on error

### TTF_Quit()
**Purpose**: Cleans up SDL_ttf
**Parameters**: None
**Returns**: void

### TTF_OpenFont(file, ptsize)
**Purpose**: Opens font file
**Parameters**:
- `file`: Font file path
- `ptsize`: Point size
**Returns**: `TTF_Font*` pointer or NULL on failure

### TTF_CloseFont(TTF_Font* font)
**Purpose**: Closes font
**Parameters**: `font` - Font to close
**Returns**: void

### TTF_GetError()
**Purpose**: Gets SDL_ttf error message
**Parameters**: None
**Returns**: `const char*` error string

## ImGui Core Functions

### IMGUI_CHECKVERSION()
**Purpose**: Checks ImGui version compatibility
**Parameters**: None
**Returns**: void

### ImGui::CreateContext()
**Purpose**: Creates ImGui context
**Parameters**: None
**Returns**: `ImGuiContext*` pointer

### ImGui::DestroyContext()
**Purpose**: Destroys ImGui context
**Parameters**: None
**Returns**: void

### ImGui::GetIO()
**Purpose**: Gets ImGui input/output structure
**Parameters**: None
**Returns**: `ImGuiIO&` reference

### ImGui::GetStyle()
**Purpose**: Gets ImGui style structure
**Parameters**: None
**Returns**: `ImGuiStyle&` reference

### ImGui::NewFrame()
**Purpose**: Starts new ImGui frame
**Parameters**: None
**Returns**: void

### ImGui::Render()
**Purpose**: Ends frame and prepares draw data
**Parameters**: None
**Returns**: void

### ImGui::GetDrawData()
**Purpose**: Gets draw data for rendering
**Parameters**: None
**Returns**: `ImDrawData*` pointer

## ImGui Window Functions

### ImGui::Begin(name, p_open, flags)
**Purpose**: Starts window
**Parameters**:
- `name`: Window title
- `p_open`: Pointer to open state (can be NULL)
- `flags`: Window flags
  - `ImGuiWindowFlags_NoResize`: Cannot resize
  - `ImGuiWindowFlags_NoMove`: Cannot move
  - `ImGuiWindowFlags_NoCollapse`: Cannot collapse
  - `ImGuiWindowFlags_AlwaysAutoResize`: Auto-resize to content
**Returns**: `bool` - true if window is open

### ImGui::End()
**Purpose**: Ends current window
**Parameters**: None
**Returns**: void

### ImGui::SetNextWindowPos(pos, cond)
**Purpose**: Sets next window position
**Parameters**:
- `pos`: Position as `ImVec2`
- `cond`: Condition flags (optional)
**Returns**: void

### ImGui::SetNextWindowSize(size, cond)
**Purpose**: Sets next window size
**Parameters**:
- `size`: Size as `ImVec2`
- `cond`: Condition flags (optional)
**Returns**: void

### ImGui::BeginMainMenuBar()
**Purpose**: Starts main menu bar
**Parameters**: None
**Returns**: `bool` - true if menu bar should be drawn

### ImGui::EndMainMenuBar()
**Purpose**: Ends main menu bar
**Parameters**: None
**Returns**: void

### ImGui::BeginMenu(label, enabled)
**Purpose**: Starts menu
**Parameters**:
- `label`: Menu label
- `enabled`: Whether menu is enabled (default true)
**Returns**: `bool` - true if menu is open

### ImGui::EndMenu()
**Purpose**: Ends current menu
**Parameters**: None
**Returns**: void

### ImGui::BeginPopupModal(name, p_open, flags)
**Purpose**: Starts modal popup
**Parameters**:
- `name`: Popup name
- `p_open`: Pointer to open state (can be NULL)
- `flags`: Window flags
**Returns**: `bool` - true if popup is open

### ImGui::EndPopup()
**Purpose**: Ends popup
**Parameters**: None
**Returns**: void

### ImGui::OpenPopup(str_id)
**Purpose**: Opens popup
**Parameters**: `str_id` - Popup identifier
**Returns**: void

### ImGui::CloseCurrentPopup()
**Purpose**: Closes current popup
**Parameters**: None
**Returns**: void

## ImGui Widget Functions

### ImGui::Text(fmt, ...)
**Purpose**: Displays text
**Parameters**: `fmt` - Format string (printf-style)
**Returns**: void

### ImGui::Button(label, size)
**Purpose**: Creates button
**Parameters**:
- `label`: Button text
- `size`: Button size as `ImVec2` (optional)
**Returns**: `bool` - true if clicked

### ImGui::MenuItem(label, shortcut, selected, enabled)
**Purpose**: Creates menu item
**Parameters**:
- `label`: Item text
- `shortcut`: Shortcut text (optional)
- `selected`: Whether item is selected (optional)
- `enabled`: Whether item is enabled (optional)
**Returns**: `bool` - true if clicked

### ImGui::SliderInt(label, v, v_min, v_max, format)
**Purpose**: Creates integer slider
**Parameters**:
- `label`: Slider label
- `v`: Pointer to value
- `v_min, v_max`: Value range
- `format`: Display format (optional)
**Returns**: `bool` - true if value changed

### ImGui::SliderFloat(label, v, v_min, v_max, format, flags)
**Purpose**: Creates float slider
**Parameters**:
- `label`: Slider label
- `v`: Pointer to value
- `v_min, v_max`: Value range
- `format`: Display format (optional)
- `flags`: Slider flags (optional)
**Returns**: `bool` - true if value changed

### ImGui::Combo(label, current_item, items, items_count)
**Purpose**: Creates combo box
**Parameters**:
- `label`: Combo label
- `current_item`: Pointer to selected index
- `items`: Array of item strings
- `items_count`: Number of items
**Returns**: `bool` - true if selection changed

### ImGui::ColorEdit4(label, col, flags)
**Purpose**: Creates color editor with 4 components (RGBA)
**Parameters**:
- `label`: Editor label
- `col`: Array of 4 floats for RGBA
- `flags`: Color edit flags (optional)
**Returns**: `bool` - true if color changed

### ImGui::Checkbox(label, v)
**Purpose**: Creates checkbox
**Parameters**:
- `label`: Checkbox label
- `v`: Pointer to boolean value
**Returns**: `bool` - true if state changed

### ImGui::InputInt(label, v, step, step_fast, flags)
**Purpose**: Creates integer input field
**Parameters**:
- `label`: Input label
- `v`: Pointer to value
- `step`: Step amount (optional)
- `step_fast`: Fast step amount (optional)
- `flags`: Input flags (optional)
**Returns**: `bool` - true if value changed

### ImGui::InputText(label, buf, buf_size, flags)
**Purpose**: Creates text input field
**Parameters**:
- `label`: Input label
- `buf`: Text buffer
- `buf_size`: Buffer size
- `flags`: Input flags (optional)
**Returns**: `bool` - true if text changed

## ImGui Layout Functions

### ImGui::Separator()
**Purpose**: Adds horizontal separator line
**Parameters**: None
**Returns**: void

### ImGui::SameLine(offset_from_start_x, spacing)
**Purpose**: Keeps next item on same line
**Parameters**:
- `offset_from_start_x`: X offset (optional)
- `spacing`: Spacing (optional)
**Returns**: void

### ImGui::Indent(indent_w)
**Purpose**: Indents next items
**Parameters**: `indent_w` - Indent amount (optional)
**Returns**: void

### ImGui::Unindent(indent_w)
**Purpose**: Unindents next items
**Parameters**: `indent_w` - Unindent amount (optional)
**Returns**: void

### ImGui::PushItemWidth(item_width)
**Purpose**: Pushes item width for next widgets
**Parameters**: `item_width` - Width in pixels
**Returns**: void

### ImGui::PopItemWidth()
**Purpose**: Pops previously pushed item width
**Parameters**: None
**Returns**: void

## ImGui Color Functions

### ImGui::ColorConvertRGBtoHSV(r, g, b, out_h, out_s, out_v)
**Purpose**: Converts RGB to HSV color space
**Parameters**:
- `r, g, b`: RGB input values (0-1)
- `out_h, out_s, out_v`: Pointers for HSV output
**Returns**: void

### ImGui::ColorConvertHSVtoRGB(h, s, v, out_r, out_g, out_b)
**Purpose**: Converts HSV to RGB color space
**Parameters**:
- `h, s, v`: HSV input values
- `out_r, out_g, out_b`: Pointers for RGB output
**Returns**: void

## ImGui Structures

### ImVec2
**Purpose**: 2D vector/point structure
**Fields**:
- `x`: X component
- `y`: Y component

### ImVec4
**Purpose**: 4D vector structure (often used for colors)
**Fields**:
- `x, y, z, w`: Components (or r, g, b, a for colors)

### ImGuiIO
**Purpose**: Input/output context structure
**Key Fields**:
- `DisplaySize`: Display size
- `DisplayFramebufferScale`: Framebuffer scale
- `WantCaptureMouse`: Whether ImGui wants mouse input

### ImGuiStyle
**Purpose**: Style configuration structure
**Key Fields**:
- `Alpha`: Global alpha
- `FrameRounding`: Frame corner rounding
- `Colors[]`: Array of style colors

## Usage Patterns

### Basic ImGui Window
```cpp
if (ImGui::Begin("Window Title")) {
    ImGui::Text("Hello World");
    if (ImGui::Button("Click Me")) {
        // Button clicked
    }
}
ImGui::End();
```

### SDL Texture Creation and Usage
```cpp
SDL_Texture* texture = SDL_CreateTexture(renderer, 
    SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 800, 600);
SDL_SetRenderTarget(renderer, texture);
// Draw to texture...
SDL_SetRenderTarget(renderer, nullptr);
SDL_RenderCopy(renderer, texture, nullptr, nullptr);
```

### Error Checking Pattern
```cpp
if (SDL_Function() != 0) {
    std::cerr << "Error: " << SDL_GetError() << std::endl;
    return false;
}
```

This reference covers the most commonly used SDL2 and ImGui functions in the project. Each function includes practical usage information to help developers understand both the technical details and typical usage patterns.