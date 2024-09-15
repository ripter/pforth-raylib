#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"

#include "pforth.h"
#include "pf_clib.h"
#include "pf_guts.h"
#include "pf_raylib.h"
#include "pf_words.h"

#define STKPTR     (*DataStackPtr)
#define M_POP      (*(STKPTR++))
#define M_PUSH(n)  *(--(STKPTR)) = (cell_t) (n)
#define M_STACK(n) (STKPTR[n])

#define TOS      (*TopOfStack)
#define M_DROP   TOS = M_POP
#define M_SET_TOS_BOOL(result)  (TOS = ((result) == false ? pfFALSE : pfTRUE))


/**
 * Updates the stacks and pointers to translate execution of Raylib words.
 */
bool TryRaylibWord(ExecToken XT, cell_t *TopOfStack, cell_t **DataStackPtr,
                   cell_t **ReturnStackPtr) {
  char *CharPtr = NULL;

  switch (XT) {
  //
  // rcore 
  //
  //  Window-related functions
  // Initialize window and OpenGL context
  case ID_INIT_WINDOW: {     /* ( +n +n c-addr u --  ) */
    // void InitWindow(int width, int height, const char *title);
    cell_t len = TOS;        // length of the title string
    CharPtr = (char *)M_POP; // title string, not null terminated
    cell_t height = M_POP;
    cell_t width = M_POP;

    if (CharPtr != NULL && len > 0 && len < TIB_SIZE) {
      M_DROP;
      pfCopyMemory(gScratch, CharPtr, len);
      gScratch[len] = '\0';
      InitWindow(width, height, gScratch);
    } else {
      fprintf(stderr, "\nError: Invalid string or length. Use s\" to create "
                      "a valid string.\n");
    }
  } break;
  // Close window and unload OpenGL context
  case ID_CLOSE_WINDOW: { /* ( --  ) */
    CloseWindow();
  } break;
  // Check if application should close (KEY_ESCAPE pressed or windows close icon clicked)
  case ID_WINDOW_SHOULD_CLOSE: { /* ( -- +n ) */
    M_SET_TOS_BOOL(WindowShouldClose());
  } break;
  // Check if window has been initialized successfully
  case ID_IS_WINDOW_READY: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowReady());
  } break;
  // Check if window is currently fullscreen
  case ID_IS_WINDOW_FULLSCREEN: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowFullscreen());
  } break;
  case ID_IS_WINDOW_HIDDEN: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowHidden());
  } break;
  case ID_IS_WINDOW_MINIMIZED: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowMinimized());
  } break;
  case ID_IS_WINDOW_MAXIMIZED: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowMaximized());
  } break;
  case ID_IS_WINDOW_FOCUSED: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowFocused());
  } break;
  case ID_IS_WINDOW_RESIZED: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowResized());
  } break;
  case ID_IS_WINDOW_STATE: { /* ( -- +n ) */
    M_SET_TOS_BOOL(IsWindowState(TOS));
  } break;
  case ID_SET_WINDOW_STATE: { /* ( +n --  ) */
    int flags = TOS;
    M_DROP;
    SetWindowState(flags);
  } break;
  case ID_CLEAR_WINDOW_STATE: { /* ( +n --  ) */
    int flags = TOS;
    M_DROP;
    ClearWindowState(flags);
  } break;
  case ID_TOGGLE_FULLSCREEN: { /* ( --  ) */
    ToggleFullscreen();
  } break;
  case ID_TOGGLE_BORDERLESS_WINDOW: { /* ( --  ) */
    ToggleBorderlessWindowed();
  } break;
  case ID_MAXIMIZE_WINDOW: { /* ( --  ) */
    MaximizeWindow();
  } break;
  case ID_MINIMIZE_WINDOW: { /* ( --  ) */
    MinimizeWindow();
  } break;
  case ID_RESTORE_WINDOW: { /* ( --  ) */
    RestoreWindow();
  } break;
  // Set icon for window (single image, RGBA 32bit, only PLATFORM_DESKTOP)
  case ID_SET_WINDOW_ICON: { /* ( +n +n c-addr u --  ) */
    // RAYLIB: void SetWindowIcon(Image image);
    printf("TODO: Set the window Icon\n");
  } break; 
  //   void SetWindowIcons(Image *images, int count);              //
  //   Set icon for window (multiple images, RGBA 32bit, only
  //   PLATFORM_DESKTOP)
  //   void SetWindowTitle(const char *title);                     // Set title for window (only PLATFORM_DESKTOP and PLATFORM_WEB)
  //   void SetWindowPosition(int x, int y);                       // Set window position on screen (only PLATFORM_DESKTOP)
  //   void SetWindowMonitor(int monitor);                         // Set monitor for the current window
  //   void SetWindowMinSize(int width, int height);               // Set window minimum dimensions (for FLAG_WINDOW_RESIZABLE)
  //   void SetWindowMaxSize(int width, int height);               // Set window maximum dimensions (for FLAG_WINDOW_RESIZABLE)
  //   void SetWindowSize(int width, int height);                  // Set window dimensions
  //   void SetWindowOpacity(float opacity);                       // Set window opacity [0.0f..1.0f] (only PLATFORM_DESKTOP)
  //   void SetWindowFocused(void);                                // Set window focused (only PLATFORM_DESKTOP)
  //   void *GetWindowHandle(void);                                // Get native window handle
  //   int GetScreenWidth(void);                                   // Get current screen width
  //   int GetScreenHeight(void);                                  // Get current screen height
  //   int GetRenderWidth(void);                                   // Get current render width (it considers HiDPI)
  //   int GetRenderHeight(void);                                  // Get current render height (it considers HiDPI)
  //   int GetMonitorCount(void);                                  // Get number of connected monitors
  //   int GetCurrentMonitor(void);                                // Get current connected monitor
  //   Vector2 GetMonitorPosition(int monitor);                    // Get specified monitor position
  //   int GetMonitorWidth(int monitor);                           // Get specified monitor width (current video mode used by monitor)
  //   int GetMonitorHeight(int monitor);                          // Get specified monitor height (current video mode used by monitor)
  //   int GetMonitorPhysicalWidth(int monitor);                   // Get specified monitor physical width in millimetres
  //   int GetMonitorPhysicalHeight(int monitor);                  // Get specified monitor physical height in millimetres
  //   int GetMonitorRefreshRate(int monitor);                     // Get specified monitor refresh rate
  //   Vector2 GetWindowPosition(void);                            // Get window position XY on monitor
  //   Vector2 GetWindowScaleDPI(void);                            // Get window scale DPI factor
  //   const char *GetMonitorName(int monitor);                    // Get the human-readable, UTF-8 encoded name of the specified monitor
  //   void SetClipboardText(const char *text);                    // Set clipboard text content
  //   const char *GetClipboardText(void);                         // Get clipboard text content
  //   void EnableEventWaiting(void);                              // Enable waiting for events on EndDrawing(), no automatic event polling
  //   void DisableEventWaiting(void);                             // Disable waiting for events on EndDrawing(), automatic events polling

  //   // Cursor-related functions
  //   void ShowCursor(void);                                      // Shows cursor
  //   void HideCursor(void);                                      // Hides cursor
  //   bool IsCursorHidden(void);                                  // Check if cursor is not visible
  //   void EnableCursor(void);                                    // Enables cursor (unlock cursor)
  //   void DisableCursor(void);                                   // Disables cursor (lock cursor)
  //   bool IsCursorOnScreen(void);                                // Check if cursor is on the screen

  //
  // Drawing-related functions
  // Set background color (framebuffer clear color)
  case ID_CLEAR_BACKGROUND: { /* ( n n n n --  ) */
    if (!IsWindowReady()) {
      ERR("Error: Window is not ready.\nClearBackground() requires a window to be initialized.\n");
      break;
    }
    int alpha = TOS;
    int blue = M_POP;
    int green = M_POP;
    int red = M_POP;
    M_DROP;
    printf("Stack: %d %d %d %d\n", red, green, blue, alpha);
    ClearBackground((Color){red, green, blue, alpha});
  } break;
  // Setup canvas (framebuffer) to start drawing
  case ID_BEGIN_DRAWING: { /* ( --  ) */
    BeginDrawing();
  } break;
    // End canvas drawing and swap buffers (double buffering)
  case ID_END_DRAWING: { /* ( --  ) */
    EndDrawing();
  } break;
  //   void BeginMode2D(Camera2D camera);                          // Begin 2D mode with custom camera (2D)
  //   void EndMode2D(void);                                       // Ends 2D mode with custom camera
  //   void BeginMode3D(Camera3D camera);                          // Begin 3D mode with custom camera (3D)
  //   void EndMode3D(void);                                       // Ends 3D mode and returns to default 2D orthographic mode
  //   void BeginTextureMode(RenderTexture2D target);              // Begin drawing to render texture
  //   void EndTextureMode(void);                                  // Ends drawing to render texture
  //   void BeginShaderMode(Shader shader);                        // Begin custom shader drawing
  //   void EndShaderMode(void);                                   // End custom shader drawing (use default shader)
  //   void BeginBlendMode(int mode);                              // Begin blending mode (alpha, additive, multiplied, subtract, custom)
  //   void EndBlendMode(void);                                    // End blending mode (reset to default: alpha blending)
  //   void BeginScissorMode(int x, int y, int width, int height); // Begin scissor mode (define screen area for following drawing)
  //   void EndScissorMode(void);                                  // End scissor mode
  //   void BeginVrStereoMode(VrStereoConfig config);              // Begin stereo rendering (requires VR simulator)
  //   void EndVrStereoMode(void);                                 // End stereo rendering (requires VR simulator)

  //   // VR stereo config functions for VR simulator
  //   VrStereoConfig LoadVrStereoConfig(VrDeviceInfo device);     // Load VR stereo config for VR simulator device parameters
  //   void UnloadVrStereoConfig(VrStereoConfig config);           // Unload VR stereo config

  //   // Shader management functions
  //   // NOTE: Shader functionality is not available on OpenGL 1.1
  //   Shader LoadShader(const char *vsFileName, const char *fsFileName);   // Load shader from files and bind default locations
  //   Shader LoadShaderFromMemory(const char *vsCode, const char *fsCode); // Load shader from code strings and bind default locations
  //   bool IsShaderReady(Shader shader);                                   // Check if a shader is ready
  //   int GetShaderLocation(Shader shader, const char *uniformName);       // Get shader uniform location
  //   int GetShaderLocationAttrib(Shader shader, const char *attribName);  // Get shader attribute location
  //   void SetShaderValue(Shader shader, int locIndex, const void *value, int uniformType);               // Set shader uniform value
  //   void SetShaderValueV(Shader shader, int locIndex, const void *value, int uniformType, int count);   // Set shader uniform value vector
  //   void SetShaderValueMatrix(Shader shader, int locIndex, Matrix mat);         // Set shader uniform value (matrix 4x4)
  //   void SetShaderValueTexture(Shader shader, int locIndex, Texture2D texture); // Set shader uniform value for texture (sampler2d)
  //   void UnloadShader(Shader shader);                                    // Unload shader from GPU memory (VRAM)

  //   // Screen-space-related functions
  //   Ray GetMouseRay(Vector2 mousePosition, Camera camera);      // Get a ray trace from mouse position
  //   Matrix GetCameraMatrix(Camera camera);                      // Get camera transform matrix (view matrix)
  //   Matrix GetCameraMatrix2D(Camera2D camera);                  // Get camera 2d transform matrix
  //   Vector2 GetWorldToScreen(Vector3 position, Camera camera);  // Get the screen space position for a 3d world space position
  //   Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera); // Get the world space position for a 2d camera screen space position
  //   Vector2 GetWorldToScreenEx(Vector3 position, Camera camera, int width, int height); // Get size position for a 3d world space position
  //   Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera); // Get the screen space position for a 2d camera world space position

  //
  // Timing-related functions
  // Set target FPS (maximum)
  case ID_SET_TARGET_FPS: { /* ( +n --  ) */
    // RAYLIB: void SetTargetFPS(int fps);
    SetTargetFPS(TOS);
    M_DROP;
  } break;
  //   float GetFrameTime(void);                                   // Get time in seconds for last frame drawn (delta time)
  //   double GetTime(void);                                       // Get elapsed time in seconds since InitWindow()
  //   int GetFPS(void);                                           // Get current FPS

  //   // Custom frame control functions
  //   // NOTE: Those functions are intended for advance users that want full control over the frame processing
  //   // By default EndDrawing() does this job: draws everything + SwapScreenBuffer() + manage frame timing + PollInputEvents()
  //   // To avoid that behaviour and control frame processes manually, enable in config.h: SUPPORT_CUSTOM_FRAME_CONTROL
  //   void SwapScreenBuffer(void);                                // Swap back buffer with front buffer (screen drawing)
  //   void PollInputEvents(void);                                 // Register all input events
  //   void WaitTime(double seconds);                              // Wait for some time (halt program execution)

  //   // Random values generation functions
  //   void SetRandomSeed(unsigned int seed);                      // Set the seed for the random number generator
  //   int GetRandomValue(int min, int max);                       // Get a random value between min and max (both included)
  //   int *LoadRandomSequence(unsigned int count, int min, int max); // Load random values sequence, no values repeated
  //   void UnloadRandomSequence(int *sequence);                   // Unload random values sequence

  //   // Misc. functions
  //   void TakeScreenshot(const char *fileName);                  // Takes a screenshot of current screen (filename extension defines format)
  //   void SetConfigFlags(unsigned int flags);                    // Setup init configuration flags (view FLAGS)
  //   void OpenURL(const char *url);                              // Open URL with default system browser (if available)

  //   // NOTE: Following functions implemented in module [utils]
  //   //------------------------------------------------------------------
  //   void TraceLog(int logLevel, const char *text, ...);         // Show trace log messages (LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR...)
  //   void SetTraceLogLevel(int logLevel);                        // Set the current threshold (minimum) log level
  //   void *MemAlloc(unsigned int size);                          // Internal memory allocator
  //   void *MemRealloc(void *ptr, unsigned int size);             // Internal memory reallocator
  //   void MemFree(void *ptr);                                    // Internal memory free

  //   // Set custom callbacks
  //   // WARNING: Callbacks setup is intended for advance users
  //   void SetTraceLogCallback(TraceLogCallback callback);         // Set custom trace log
  //   void SetLoadFileDataCallback(LoadFileDataCallback callback); // Set custom file binary data loader
  //   void SetSaveFileDataCallback(SaveFileDataCallback callback); // Set custom file binary data saver
  //   void SetLoadFileTextCallback(LoadFileTextCallback callback); // Set custom file text data loader
  //   void SetSaveFileTextCallback(SaveFileTextCallback callback); // Set custom file text data saver

  //   // Files management functions
  //   unsigned char *LoadFileData(const char *fileName, int *dataSize); // Load file data as byte array (read)
  //   void UnloadFileData(unsigned char *data);                   // Unload file data allocated by LoadFileData()
  //   bool SaveFileData(const char *fileName, void *data, int dataSize); // Save data to file from byte array (write), returns true on success
  //   bool ExportDataAsCode(const unsigned char *data, int dataSize, const char *fileName); // Export data to code (.h), returns true on success
  //   char *LoadFileText(const char *fileName);                   // Load text data from file (read), returns a '\0' terminated string
  //   void UnloadFileText(char *text);                            // Unload file text data allocated by LoadFileText()
  //   bool SaveFileText(const char *fileName, char *text);        // Save text data to file (write), string must be '\0' terminated, returns true on success
  //   //------------------------------------------------------------------

  //   // File system functions
  //   bool FileExists(const char *fileName);                      // Check if file exists
  //   bool DirectoryExists(const char *dirPath);                  // Check if a directory path exists
  //   bool IsFileExtension(const char *fileName, const char *ext); // Check file extension (including point: .png, .wav)
  //   int GetFileLength(const char *fileName);                    // Get file length in bytes (NOTE: GetFileSize() conflicts with windows.h)
  //   const char *GetFileExtension(const char *fileName);         // Get pointer to extension for a filename string (includes dot: '.png')
  //   const char *GetFileName(const char *filePath);              // Get pointer to filename for a path string
  //   const char *GetFileNameWithoutExt(const char *filePath);    // Get filename string without extension (uses static string)
  //   const char *GetDirectoryPath(const char *filePath);         // Get full path for a given fileName with path (uses static string)
  //   const char *GetPrevDirectoryPath(const char *dirPath);      // Get previous directory path for a given path (uses static string)
  //   const char *GetWorkingDirectory(void);                      // Get current working directory (uses static string)
  //   const char *GetApplicationDirectory(void);                  // Get the directory of the running application (uses static string)
  //   bool ChangeDirectory(const char *dir);                      // Change working directory, return true on success
  //   bool IsPathFile(const char *path);                          // Check if a given path is a file or a directory
  //   FilePathList LoadDirectoryFiles(const char *dirPath);       // Load directory filepaths
  //   FilePathList LoadDirectoryFilesEx(const char *basePath, const char *filter, bool scanSubdirs); // Load directory filepaths with extension filtering and recursive directory scan
  //   void UnloadDirectoryFiles(FilePathList files);              // Unload filepaths
  //   bool IsFileDropped(void);                                   // Check if a file has been dropped into window
  //   FilePathList LoadDroppedFiles(void);                        // Load dropped filepaths
  //   void UnloadDroppedFiles(FilePathList files);                // Unload dropped filepaths
  //   long GetFileModTime(const char *fileName);                  // Get file modification time (last write time)

  //   // Compression/Encoding functionality
  //   unsigned char *CompressData(const unsigned char *data, int dataSize, int *compDataSize);        // Compress data (DEFLATE algorithm), memory must be MemFree()
  //   unsigned char *DecompressData(const unsigned char *compData, int compDataSize, int *dataSize);  // Decompress data (DEFLATE algorithm), memory must be MemFree()
  //   char *EncodeDataBase64(const unsigned char *data, int dataSize, int *outputSize);               // Encode data to Base64 string, memory must be MemFree()
  //   unsigned char *DecodeDataBase64(const unsigned char *data, int *outputSize);                    // Decode Base64 string data, memory must be MemFree()

  //   // Automation events functionality
  //   AutomationEventList LoadAutomationEventList(const char *fileName);                // Load automation events list from file, NULL for empty list, capacity = MAX_AUTOMATION_EVENTS
  //   void UnloadAutomationEventList(AutomationEventList *list);                        // Unload automation events list from file
  //   bool ExportAutomationEventList(AutomationEventList list, const char *fileName);   // Export automation events list as text file
  //   void SetAutomationEventList(AutomationEventList *list);                           // Set automation event list to record to
  //   void SetAutomationEventBaseFrame(int frame);                                      // Set automation event internal base frame to start recording
  //   void StartAutomationEventRecording(void);                                         // Start recording automation events (AutomationEventList must be set)
  //   void StopAutomationEventRecording(void);                                          // Stop recording automation events
  //   void PlayAutomationEvent(AutomationEvent event);                                  // Play a recorded automation event

  //   //------------------------------------------------------------------------------------
  //   // Input Handling Functions (Module: core)
  //   //------------------------------------------------------------------------------------

  //   // Input-related functions: keyboard
  //   bool IsKeyPressed(int key);                             // Check if a key has been pressed once
  //   bool IsKeyPressedRepeat(int key);                       // Check if a key has been pressed again (Only PLATFORM_DESKTOP)
  //   bool IsKeyDown(int key);                                // Check if a key is being pressed
  //   bool IsKeyReleased(int key);                            // Check if a key has been released once
  //   bool IsKeyUp(int key);                                  // Check if a key is NOT being pressed
  //   int GetKeyPressed(void);                                // Get key pressed (keycode), call it multiple times for keys queued, returns 0 when the queue is empty
  //   int GetCharPressed(void);                               // Get char pressed (unicode), call it multiple times for chars queued, returns 0 when the queue is empty
  //   void SetExitKey(int key);                               // Set a custom key to exit program (default is ESC)

  //   // Input-related functions: gamepads
  //   bool IsGamepadAvailable(int gamepad);                   // Check if a gamepad is available
  //   const char *GetGamepadName(int gamepad);                // Get gamepad internal name id
  //   bool IsGamepadButtonPressed(int gamepad, int button);   // Check if a gamepad button has been pressed once
  //   bool IsGamepadButtonDown(int gamepad, int button);      // Check if a gamepad button is being pressed
  //   bool IsGamepadButtonReleased(int gamepad, int button);  // Check if a gamepad button has been released once
  //   bool IsGamepadButtonUp(int gamepad, int button);        // Check if a gamepad button is NOT being pressed
  //   int GetGamepadButtonPressed(void);                      // Get the last gamepad button pressed
  //   int GetGamepadAxisCount(int gamepad);                   // Get gamepad axis count for a gamepad
  //   float GetGamepadAxisMovement(int gamepad, int axis);    // Get axis movement value for a gamepad axis
  //   int SetGamepadMappings(const char *mappings);           // Set internal gamepad mappings (SDL_GameControllerDB)

  //   // Input-related functions: mouse
  //   bool IsMouseButtonPressed(int button);                  // Check if a mouse button has been pressed once
  //   bool IsMouseButtonDown(int button);                     // Check if a mouse button is being pressed
  //   bool IsMouseButtonReleased(int button);                 // Check if a mouse button has been released once
  //   bool IsMouseButtonUp(int button);                       // Check if a mouse button is NOT being pressed
  //   int GetMouseX(void);                                    // Get mouse position X
  //   int GetMouseY(void);                                    // Get mouse position Y
  //   Vector2 GetMousePosition(void);                         // Get mouse position XY
  //   Vector2 GetMouseDelta(void);                            // Get mouse delta between frames
  //   void SetMousePosition(int x, int y);                    // Set mouse position XY
  //   void SetMouseOffset(int offsetX, int offsetY);          // Set mouse offset
  //   void SetMouseScale(float scaleX, float scaleY);         // Set mouse scaling
  //   float GetMouseWheelMove(void);                          // Get mouse wheel movement for X or Y, whichever is larger
  //   Vector2 GetMouseWheelMoveV(void);                       // Get mouse wheel movement for both X and Y
  //   void SetMouseCursor(int cursor);                        // Set mouse cursor

  //   // Input-related functions: touch
  //   int GetTouchX(void);                                    // Get touch position X for touch point 0 (relative to screen size)
  //   int GetTouchY(void);                                    // Get touch position Y for touch point 0 (relative to screen size)
  //   Vector2 GetTouchPosition(int index);                    // Get touch position XY for a touch point index (relative to screen size)
  //   int GetTouchPointId(int index);                         // Get touch point identifier for given index
  //   int GetTouchPointCount(void);                           // Get number of touch points

  //   //------------------------------------------------------------------------------------
  //   // Gestures and Touch Handling Functions (Module: rgestures)
  //   //------------------------------------------------------------------------------------
  //   void SetGesturesEnabled(unsigned int flags);      // Enable a set of gestures using flags
  //   bool IsGestureDetected(unsigned int gesture);     // Check if a gesture have been detected
  //   int GetGestureDetected(void);                     // Get latest detected gesture
  //   float GetGestureHoldDuration(void);               // Get gesture hold time in milliseconds
  //   Vector2 GetGestureDragVector(void);               // Get gesture drag vector
  //   float GetGestureDragAngle(void);                  // Get gesture drag angle
  //   Vector2 GetGesturePinchVector(void);              // Get gesture pinch delta
  //   float GetGesturePinchAngle(void);                 // Get gesture pinch angle

  //   //------------------------------------------------------------------------------------
  //   // Camera System Functions (Module: rcamera)
  //   //------------------------------------------------------------------------------------
  //   void UpdateCamera(Camera *camera, int mode);      // Update camera position for selected mode
  //   void UpdateCameraPro(Camera *camera, Vector3 movement, Vector3 rotation, float zoom); // Update camera movement/rotation


  //
  // Drawing-related functions
  // void BeginMode2D(Camera2D camera)
  // void EndMode2D(void)
  // void BeginMode3D(Camera3D camera)
  // void EndMode3D(void)
  // void BeginTextureMode(RenderTexture2D target)
  // void EndTextureMode(void)
  // void BeginShaderMode(Shader shader)
  // void EndShaderMode(void)
  // void BeginBlendMode(int mode)
  // void EndBlendMode(void)
  // void BeginScissorMode(int x, int y, int width, int height)
  // void EndScissorMode(void)
  // void BeginVrStereoMode(VrStereoConfig config)
  // void EndVrStereoMode(void)

    //
  // rtextures
  //
  // Image loading functions
  // NOTE: These functions do not require GPU access
  // Load image from file into CPU memory (RAM)
  case ID_LOAD_IMAGE: {      /* ( c-addr u -- c-addr2 ) */
    // RAYLIB: Image LoadImage(const char *fileName);                                                             
    cell_t len = TOS;        /* length of the filename string. */
    CharPtr = (char *)M_POP; /* filename string, not null terminated. */
    pfCopyMemory(gScratch, CharPtr, len);
    gScratch[len] = '\0';
    Image image = LoadImage(gScratch);
    printf("\nSaving image = %p\n", image);
    TOS = (cell_t)&image;
  } break; 
  // Image LoadImageRaw(const char *fileName, int width, int height,
  // int format, int headerSize);       // Load image from RAW file
  // data
  // Image LoadImageSvg(const char *fileNameOrString, int width, int height);                           // Load image from SVG file data or string with specified size
  // Image LoadImageAnim(const char *fileName, int *frames);                                            // Load image sequence from file (frames appended to image.data)
  // Image LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);      // Load image from memory buffer, fileType refers to extension: i.e. '.png'
  // Image LoadImageFromTexture(Texture2D texture);                                                     // Load image from GPU texture data
  // Image LoadImageFromScreen(void);                                                                   // Load image from screen buffer and (screenshot)
  // bool IsImageReady(Image image);                                                                    // Check if an image is ready
  // void UnloadImage(Image image);                                                                     // Unload image from CPU memory (RAM)
  // bool ExportImage(Image image, const char *fileName);                                               // Export image data to file, returns true on success
  // unsigned char *ExportImageToMemory(Image image, const char *fileType, int *fileSize);              // Export image to memory buffer
  // bool ExportImageAsCode(Image image, const char *fileName);                                         // Export image as code file defining an array of bytes, returns true on success

  // // Image generation functions
  // Image GenImageColor(int width, int height, Color color);                                           // Generate image: plain color
  // Image GenImageGradientLinear(int width, int height, int direction, Color start, Color end);        // Generate image: linear gradient, direction in degrees [0..360], 0=Vertical gradient
  // Image GenImageGradientRadial(int width, int height, float density, Color inner, Color outer);      // Generate image: radial gradient
  // Image GenImageGradientSquare(int width, int height, float density, Color inner, Color outer);      // Generate image: square gradient
  // Image GenImageChecked(int width, int height, int checksX, int checksY, Color col1, Color col2);    // Generate image: checked
  // Image GenImageWhiteNoise(int width, int height, float factor);                                     // Generate image: white noise
  // Image GenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale);           // Generate image: perlin noise
  // Image GenImageCellular(int width, int height, int tileSize);                                       // Generate image: cellular algorithm, bigger tileSize means bigger cells
  // Image GenImageText(int width, int height, const char *text);                                       // Generate image: grayscale image from text data

  // // Image manipulation functions
  // Image ImageCopy(Image image);                                                                      // Create an image duplicate (useful for transformations)
  // Image ImageFromImage(Image image, Rectangle rec);                                                  // Create an image from another image piece
  // Image ImageText(const char *text, int fontSize, Color color);                                      // Create an image from text (default font)
  // Image ImageTextEx(Font font, const char *text, float fontSize, float spacing, Color tint);         // Create an image from text (custom sprite font)
  // void ImageFormat(Image *image, int newFormat);                                                     // Convert image data to desired format
  // void ImageToPOT(Image *image, Color fill);                                                         // Convert image to POT (power-of-two)
  // void ImageCrop(Image *image, Rectangle crop);                                                      // Crop an image to a defined rectangle
  // void ImageAlphaCrop(Image *image, float threshold);                                                // Crop image depending on alpha value
  // void ImageAlphaClear(Image *image, Color color, float threshold);                                  // Clear alpha channel to desired color
  // void ImageAlphaMask(Image *image, Image alphaMask);                                                // Apply alpha mask to image
  // void ImageAlphaPremultiply(Image *image);                                                          // Premultiply alpha channel
  // void ImageBlurGaussian(Image *image, int blurSize);                                                // Apply Gaussian blur using a box blur approximation
  // void ImageResize(Image *image, int newWidth, int newHeight);                                       // Resize image (Bicubic scaling algorithm)
  // void ImageResizeNN(Image *image, int newWidth,int newHeight);                                      // Resize image (Nearest-Neighbor scaling algorithm)
  // void ImageResizeCanvas(Image *image, int newWidth, int newHeight, int offsetX, int offsetY, Color fill);  // Resize canvas and fill with color
  // void ImageMipmaps(Image *image);                                                                   // Compute all mipmap levels for a provided image
  // void ImageDither(Image *image, int rBpp, int gBpp, int bBpp, int aBpp);                            // Dither image data to 16bpp or lower (Floyd-Steinberg dithering)
  // void ImageFlipVertical(Image *image);                                                              // Flip image vertically
  // void ImageFlipHorizontal(Image *image);                                                            // Flip image horizontally
  // void ImageRotate(Image *image, int degrees);                                                       // Rotate image by input angle in degrees (-359 to 359)
  // void ImageRotateCW(Image *image);                                                                  // Rotate image clockwise 90deg
  // void ImageRotateCCW(Image *image);                                                                 // Rotate image counter-clockwise 90deg
  // void ImageColorTint(Image *image, Color color);                                                    // Modify image color: tint
  // void ImageColorInvert(Image *image);                                                               // Modify image color: invert
  // void ImageColorGrayscale(Image *image);                                                            // Modify image color: grayscale
  // void ImageColorContrast(Image *image, float contrast);                                             // Modify image color: contrast (-100 to 100)
  // void ImageColorBrightness(Image *image, int brightness);                                           // Modify image color: brightness (-255 to 255)
  // void ImageColorReplace(Image *image, Color color, Color replace);                                  // Modify image color: replace color
  // Color *LoadImageColors(Image image);                                                               // Load color data from image as a Color array (RGBA - 32bit)
  // Color *LoadImagePalette(Image image, int maxPaletteSize, int *colorCount);                         // Load colors palette from image as a Color array (RGBA - 32bit)
  // void UnloadImageColors(Color *colors);                                                             // Unload color data loaded with LoadImageColors()
  // void UnloadImagePalette(Color *colors);                                                            // Unload colors palette loaded with LoadImagePalette()
  // Rectangle GetImageAlphaBorder(Image image, float threshold);                                       // Get image alpha border rectangle
  // Color GetImageColor(Image image, int x, int y);                                                    // Get image pixel color at (x, y) position

  // // Image drawing functions
  // // NOTE: Image software-rendering functions (CPU)
  // void ImageClearBackground(Image *dst, Color color);                                                // Clear image background with given color
  // void ImageDrawPixel(Image *dst, int posX, int posY, Color color);                                  // Draw pixel within an image
  // void ImageDrawPixelV(Image *dst, Vector2 position, Color color);                                   // Draw pixel within an image (Vector version)
  // void ImageDrawLine(Image *dst, int startPosX, int startPosY, int endPosX, int endPosY, Color color); // Draw line within an image
  // void ImageDrawLineV(Image *dst, Vector2 start, Vector2 end, Color color);                          // Draw line within an image (Vector version)
  // void ImageDrawCircle(Image *dst, int centerX, int centerY, int radius, Color color);               // Draw a filled circle within an image
  // void ImageDrawCircleV(Image *dst, Vector2 center, int radius, Color color);                        // Draw a filled circle within an image (Vector version)
  // void ImageDrawCircleLines(Image *dst, int centerX, int centerY, int radius, Color color);          // Draw circle outline within an image
  // void ImageDrawCircleLinesV(Image *dst, Vector2 center, int radius, Color color);                   // Draw circle outline within an image (Vector version)
  // void ImageDrawRectangle(Image *dst, int posX, int posY, int width, int height, Color color);       // Draw rectangle within an image
  // void ImageDrawRectangleV(Image *dst, Vector2 position, Vector2 size, Color color);                 // Draw rectangle within an image (Vector version)
  // void ImageDrawRectangleRec(Image *dst, Rectangle rec, Color color);                                // Draw rectangle within an image
  // void ImageDrawRectangleLines(Image *dst, Rectangle rec, int thick, Color color);                   // Draw rectangle lines within an image
  // void ImageDraw(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);             // Draw a source image within a destination image (tint applied to source)
  // void ImageDrawText(Image *dst, const char *text, int posX, int posY, int fontSize, Color color);   // Draw text (using default font) within an image (destination)
  // void ImageDrawTextEx(Image *dst, Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint); // Draw text (custom sprite font) within an image (destination)

  // // Texture loading functions
  // // NOTE: These functions require GPU access
  // Texture2D LoadTexture(const char *fileName);                                                       // Load texture from file into GPU memory (VRAM)

  // Load texture from image data
  case ID_LOAD_TEXTURE_FROM_IMAGE: { /* ( c-addr -- c-addr ) */
    // RAYLIB: Texture2D LoadTextureFromImage(Image image);
    Image image = *(Image *)TOS;
    printf("\nLoaded image = %p\n", image);
    Texture2D texture = LoadTextureFromImage(image);
    printf("\ntexture.id = %d\n", texture.id);
    TOS = (cell_t)&texture;
  } break;

  // TextureCubemap LoadTextureCubemap(Image image, int layout);                                        // Load cubemap from image, multiple image cubemap layouts supported
  // RenderTexture2D LoadRenderTexture(int width, int height);                                          // Load texture for rendering (framebuffer)
  // bool IsTextureReady(Texture2D texture);                                                            // Check if a texture is ready
  // void UnloadTexture(Texture2D texture);                                                             // Unload texture from GPU memory (VRAM)
  // bool IsRenderTextureReady(RenderTexture2D target);                                                 // Check if a render texture is ready
  // void UnloadRenderTexture(RenderTexture2D target);                                                  // Unload render texture from GPU memory (VRAM)
  // void UpdateTexture(Texture2D texture, const void *pixels);                                         // Update GPU texture with new data
  // void UpdateTextureRec(Texture2D texture, Rectangle rec, const void *pixels);                       // Update GPU texture rectangle with new data

  // // Texture configuration functions
  // void GenTextureMipmaps(Texture2D *texture);                                                        // Generate GPU mipmaps for a texture
  // void SetTextureFilter(Texture2D texture, int filter);                                              // Set texture scaling filter mode
  // void SetTextureWrap(Texture2D texture, int wrap);                                                  // Set texture wrapping mode

  // // Texture drawing functions
  // void DrawTexture(Texture2D texture, int posX, int posY, Color tint);                               // Draw a Texture2D
  // void DrawTextureV(Texture2D texture, Vector2 position, Color tint);                                // Draw a Texture2D with position defined as Vector2
  // void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);  // Draw a Texture2D with extended parameters
  // void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);            // Draw a part of a texture defined by a rectangle
  // void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint); // Draw a part of a texture defined by a rectangle with 'pro' parameters
  // void DrawTextureNPatch(Texture2D texture, NPatchInfo nPatchInfo, Rectangle dest, Vector2 origin, float rotation, Color tint); // Draws a texture (or part of it) that stretches or shrinks nicely

  // // Color/pixel related functions
  // Color Fade(Color color, float alpha);                                 // Get color with alpha applied, alpha goes from 0.0f to 1.0f
  // int ColorToInt(Color color);                                          // Get hexadecimal value for a Color
  // Vector4 ColorNormalize(Color color);                                  // Get Color normalized as float [0..1]
  // Color ColorFromNormalized(Vector4 normalized);                        // Get Color from normalized values [0..1]
  // Vector3 ColorToHSV(Color color);                                      // Get HSV values for a Color, hue [0..360], saturation/value [0..1]
  // Color ColorFromHSV(float hue, float saturation, float value);         // Get a Color from HSV values, hue [0..360], saturation/value [0..1]
  // Color ColorTint(Color color, Color tint);                             // Get color multiplied with another color
  // Color ColorBrightness(Color color, float factor);                     // Get color with brightness correction, brightness factor goes from -1.0f to 1.0f
  // Color ColorContrast(Color color, float contrast);                     // Get color with contrast correction, contrast values between -1.0f and 1.0f
  // Color ColorAlpha(Color color, float alpha);                           // Get color with alpha applied, alpha goes from 0.0f to 1.0f
  // Color ColorAlphaBlend(Color dst, Color src, Color tint);              // Get src alpha-blended into dst color with tint
  // Color GetColor(unsigned int hexValue);                                // Get Color structure from hexadecimal value
  // Color GetPixelColor(void *srcPtr, int format);                        // Get Color from a source pixel pointer of certain format
  // void SetPixelColor(void *dstPtr, Color color, int format);            // Set color formatted into destination pixel pointer
  // int GetPixelDataSize(int width, int height, int format);              // Get pixel data size in bytes for certain format


  //
  // module: rtext
  //
  // Font loading/unloading functions
  // Font GetFontDefault(void);
  // Font LoadFont(const char *fileName);
  // Font LoadFontEx(const char *fileName, int fontSize, int *codepoints, int codepointCount);
  // Font LoadFontFromImage(Image image, Color key, int firstChar);
  // Font LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount);
  // bool IsFontReady(Font font); 
  // GlyphInfo *LoadFontData(const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount, int type); 
  // Image GenImageFontAtlas(const GlyphInfo *glyphs, Rectangle **glyphRecs, int glyphCount, int fontSize, int padding, int packMethod);
  // void UnloadFontData(GlyphInfo *glyphs, int glyphCount); 
  // void UnloadFont(Font font);
  // bool ExportFontAsCode(Font font, const char *fileName);  

  //
  // Text drawing functions
  // void DrawFPS(int posX, int posY);
  // Draw text (using default font)
  case ID_DRAW_TEXT: { /* ( +n +n c-addr u n n n n --  ) */
    // void DrawText(const char *text, int posX, int posY, int fontSize, Color color);
    int alpha = TOS;
    int blue = M_POP;
    int green = M_POP;
    int red = M_POP;
    int fontSize = M_POP;
    int posY = M_POP;
    int posX = M_POP;
    int len = M_POP;
    CharPtr = (char *)M_POP; /* not null terminated. */
    if (CharPtr != NULL && len > 0 && len < TIB_SIZE) {
      M_DROP;
      pfCopyMemory(gScratch, CharPtr, len);
      gScratch[len] = '\0';
      DrawText(gScratch, posX, posY, fontSize,
               (Color){red, green, blue, alpha});
    }
  } break; 
  // void DrawTextEx(Font font, const char *text, Vector2 position,
  // float fontSize, float spacing, Color tint);
    // void DrawTextPro(Font font, const char *text, Vector2 position, Vector2
    // origin, float rotation, float fontSize, float spacing, Color tint); void
    // DrawTextCodepoint(Font font, int codepoint, Vector2 position, float
    // fontSize, Color tint); void DrawTextCodepoints(Font font, const int
    // *codepoints, int codepointCount, Vector2 position, float fontSize, float
    // spacing, Color tint);

    //
    // Text font info functions
    // void SetTextLineSpacing(int spacing);
    // int MeasureText(const char *text, int fontSize);
    // Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float
    // spacing); int GetGlyphIndex(Font font, int codepoint); GlyphInfo
    // GetGlyphInfo(Font font, int codepoint); Rectangle GetGlyphAtlasRec(Font
    // font, int codepoint);

    //
    // Text codepoints management functions (unicode characters)
    // char *LoadUTF8(const int *codepoints, int length);                // Load
    // UTF-8 text encoded from codepoints array
    //   void UnloadUTF8(char *text);                                      //
    //   Unload UTF-8 text encoded from codepoints array int
    //   *LoadCodepoints(const char *text, int *count);                // Load
    //   all codepoints from a UTF-8 text string, codepoints count returned by
    //   parameter void UnloadCodepoints(int *codepoints); // Unload codepoints
    //   data from memory int GetCodepointCount(const char *text); // Get total
    //   number of codepoints in a UTF-8 encoded string int GetCodepoint(const
    //   char *text, int *codepointSize);           // Get next codepoint in a
    //   UTF-8 encoded string, 0x3f('?') is returned on failure int
    //   GetCodepointNext(const char *text, int *codepointSize);       // Get
    //   next codepoint in a UTF-8 encoded string, 0x3f('?') is returned on
    //   failure int GetCodepointPrevious(const char *text, int *codepointSize);
    //   // Get previous codepoint in a UTF-8 encoded string, 0x3f('?') is
    //   returned on failure const char *CodepointToUTF8(int codepoint, int
    //   *utf8Size);        // Encode one codepoint into UTF-8 byte array (array
    //   length returned as parameter)

    //
    //   // Text strings management functions (no UTF-8 strings, only byte
    //   chars)
    //   // NOTE: Some strings allocate memory internally for returned strings,
    //   just be careful! int TextCopy(char *dst, const char *src); // Copy one
    //   string to another, returns bytes copied bool TextIsEqual(const char
    //   *text1, const char *text2);                               // Check if
    //   two text string are equal unsigned int TextLength(const char *text); //
    //   Get text length, checks for '\0' ending const char *TextFormat(const
    //   char *text, ...);                                        // Text
    //   formatting with variables (sprintf() style) const char
    //   *TextSubtext(const char *text, int position, int length); // Get a
    //   piece of a text string char *TextReplace(char *text, const char
    //   *replace, const char *by);                   // Replace text string
    //   (WARNING: memory must be freed!) char *TextInsert(const char *text,
    //   const char *insert, int position);                 // Insert text in a
    //   position (WARNING: memory must be freed!) const char *TextJoin(const
    //   char **textList, int count, const char *delimiter);        // Join text
    //   strings with delimiter const char **TextSplit(const char *text, char
    //   delimiter, int *count);                 // Split text into multiple
    //   strings void TextAppend(char *text, const char *append, int *position);
    //   // Append text at specific position and move cursor! int
    //   TextFindIndex(const char *text, const char *find); // Find first text
    //   occurrence within a string const char *TextToUpper(const char *text);
    //   // Get upper case version of provided string const char
    //   *TextToLower(const char *text);                      // Get lower case
    //   version of provided string const char *TextToPascal(const char *text);
    //   // Get Pascal case notation version of provided string int
    //   TextToInteger(const char *text);                            // Get
    //   integer value from text (negative values not supported)




  case ID_TEST_WORD: {
    printf("\nINFO: Test word\n");
    M_PUSH(42);
    M_PUSH(23);
    M_PUSH(16);
    M_PUSH(15);
    M_PUSH(8);
    M_PUSH(4);
    TOS = -19;
  } break;
  default:
    printf("\nINFO: Word not found in Raylib words. 0x");
    ffDotHex(XT);
    EMIT_CR;
    return false; // Didn't find the word.
  }
  return true; // Found the word!
}
