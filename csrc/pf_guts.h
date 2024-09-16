/* @(#) pf_guts.h 98/01/28 1.4 */
#ifndef _pf_guts_h
#define _pf_guts_h

#include "pf_io.h"
#include "pf_float.h"

/***************************************************************
** Include file for PForth, a Forth based on 'C'
**
** Author: Phil Burk
** Modified by Chris Richards 2024.
** Copyright 1994 3DO, Phil Burk, Larry Polansky, David Rosenboom
**
** Permission to use, copy, modify, and/or distribute this
** software for any purpose with or without fee is hereby granted.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
** THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
** CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
** FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
** CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
** OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**
***************************************************************/
// Macros to convert numbers to strings
#define str(s) #s
#define xstr(s) str(s)


/*
** PFORTH_VERSION changes when PForth is modified.
** See README file for version info.
*/
#define PROJECT_NAME "rayForth"
// Define the version components
#define PFORTH_FORKED_FROM_VERSION "2.0.2"
#define PFORTH_VERSION_MAJOR 0
#define PFORTH_VERSION_MINOR 0
#define PFORTH_VERSION_PATCH 2

// Construct the version name from the components
#define PFORTH_VERSION_NAME xstr(PFORTH_VERSION_MAJOR) "." xstr(PFORTH_VERSION_MINOR) "." xstr(PFORTH_VERSION_PATCH)
// Automatically calculate the version code as a single integer
#define PFORTH_VERSION_CODE ((PFORTH_VERSION_MAJOR * 10000) + (PFORTH_VERSION_MINOR * 100) + PFORTH_VERSION_PATCH)



/*
** PFORTH_FILE_VERSION changes when incompatible changes are made
** in the ".dic" file format.
**
** FV3 - 950225 - Use ABS_TO_CODEREL for CodePtr. See file "pf_save.c".
** FV4 - 950309 - Added NameSize and CodeSize to pfSaveForth().
** FV5 - 950316 - Added Floats and reserved words.
** FV6 - 961213 - Added ID_LOCAL_PLUSSTORE, ID_COLON_P, etc.
** FV7 - 971203 - Added ID_FILL, (1LOCAL@),  etc., ran out of reserved, resorted.
** FV8 - 980818 - Added Endian flag.
** FV9 - 20100503 - Added support for 64-bit CELL.
** FV10 - 20170103 - Added ID_FILE_FLUSH ID_FILE_RENAME ID_FILE_RESIZE
*/
#define PF_FILE_VERSION (10)   /* Bump this whenever primitives added. */
#define PF_EARLIEST_FILE_VERSION (9)  /* earliest one still compatible */

/***************************************************************
** Sizes and other constants
***************************************************************/

#define TIB_SIZE (256)

#ifndef FALSE
    #define FALSE (0)
#endif
#ifndef TRUE
    #define TRUE (1)
#endif

#define FFALSE (0)
#define FTRUE (-1)
#define SPACE_CHARACTER (' ')

#define FLAG_PRECEDENCE (0x80)
#define FLAG_IMMEDIATE  (0x40)
#define FLAG_SMUDGE     (0x20)
#define MASK_NAME_SIZE  (0x1F)

/* Debug TRACE flags */
#define TRACE_INNER     (0x0002)
#define TRACE_COMPILE   (0x0004)
#define TRACE_SPECIAL   (0x0008)

/* Numeric types returned by NUMBER? */
#define NUM_TYPE_BAD    (0)
#define NUM_TYPE_SINGLE (1)
#define NUM_TYPE_DOUBLE (2)
#define NUM_TYPE_FLOAT  (3)

#define CREATE_BODY_OFFSET  (3*sizeof(cell_t))

/***************************************************************
** Primitive Token IDS
** Do NOT change the order of these IDs or dictionary files will break!
***************************************************************/
enum cforth_primitive_ids {
  ID_EXIT = 0, /* ID_EXIT must always be zero. */
  ID_1MINUS,
  ID_1PLUS,
  ID_2DUP,
  ID_2LITERAL,
  ID_2LITERAL_P,
  ID_2MINUS,
  ID_2OVER,
  ID_2PLUS,
  ID_2SWAP,
  ID_2_R_FETCH,
  ID_2_R_FROM,
  ID_2_TO_R,
  ID_ACCEPT_P,
  ID_ALITERAL,
  ID_ALITERAL_P,
  ID_ALLOCATE,
  ID_AND,
  ID_ARSHIFT,
  ID_BAIL,
  ID_BODY_OFFSET,
  ID_BRANCH,
  ID_BYE,
  ID_CALL_C,
  ID_CFETCH,
  ID_CMOVE,
  ID_CMOVE_UP,
  ID_COLON,
  ID_COLON_P,
  ID_COMPARE,
  ID_COMP_EQUAL,
  ID_COMP_GREATERTHAN,
  ID_COMP_LESSTHAN,
  ID_COMP_NOT_EQUAL,
  ID_COMP_U_GREATERTHAN,
  ID_COMP_U_LESSTHAN,
  ID_COMP_ZERO_EQUAL,
  ID_COMP_ZERO_GREATERTHAN,
  ID_COMP_ZERO_LESSTHAN,
  ID_COMP_ZERO_NOT_EQUAL,
  ID_CR,
  ID_CREATE,
  ID_CREATE_P,
  ID_CSTORE,
  ID_DEFER,
  ID_DEFER_P,
  ID_DEPTH,
  ID_DIVIDE,
  ID_DOT,
  ID_DOTS,
  ID_DO_P,
  ID_DROP,
  ID_DUMP,
  ID_DUP,
  ID_D_MINUS,
  ID_D_MTIMES,
  ID_D_MUSMOD,
  ID_D_PLUS,
  ID_D_UMSMOD,
  ID_D_UMTIMES,
  ID_EMIT,
  ID_EMIT_P,
  ID_EOL,
  ID_ERRORQ_P,
  ID_EXECUTE,
  ID_FETCH,
  ID_FILE_CLOSE,
  ID_FILE_CREATE,
  ID_FILE_OPEN,
  ID_FILE_POSITION,
  ID_FILE_READ,
  ID_FILE_REPOSITION,
  ID_FILE_RO,
  ID_FILE_RW,
  ID_FILE_SIZE,
  ID_FILE_WRITE,
  ID_FILL,
  ID_FIND,
  ID_FINDNFA,
  ID_FLUSHEMIT,
  ID_FREE,
  ID_HERE,
  ID_NUMBERQ_P,
  ID_I,
  ID_INCLUDE_FILE,
  ID_J,
  ID_KEY,
  ID_LEAVE_P,
  ID_LITERAL,
  ID_LITERAL_P,
  ID_LOADSYS,
  ID_LOCAL_COMPILER,
  ID_LOCAL_ENTRY,
  ID_LOCAL_EXIT,
  ID_LOCAL_FETCH,
  ID_LOCAL_FETCH_1,
  ID_LOCAL_FETCH_2,
  ID_LOCAL_FETCH_3,
  ID_LOCAL_FETCH_4,
  ID_LOCAL_FETCH_5,
  ID_LOCAL_FETCH_6,
  ID_LOCAL_FETCH_7,
  ID_LOCAL_FETCH_8,
  ID_LOCAL_PLUSSTORE,
  ID_LOCAL_STORE,
  ID_LOCAL_STORE_1,
  ID_LOCAL_STORE_2,
  ID_LOCAL_STORE_3,
  ID_LOCAL_STORE_4,
  ID_LOCAL_STORE_5,
  ID_LOCAL_STORE_6,
  ID_LOCAL_STORE_7,
  ID_LOCAL_STORE_8,
  ID_LOOP_P,
  ID_LSHIFT,
  ID_MAX,
  ID_MIN,
  ID_MINUS,
  ID_NAME_TO_PREVIOUS,
  ID_NAME_TO_TOKEN,
  ID_NOOP,
  ID_NUMBERQ,
  ID_OR,
  ID_OVER,
  ID_PICK,
  ID_PLUS,
  ID_PLUSLOOP_P,
  ID_PLUS_STORE,
  ID_QDO_P,
  ID_QDUP,
  ID_QTERMINAL,
  ID_QUIT_P,
  ID_REFILL,
  ID_RESIZE,
  ID_SOURCE_LINE_NUMBER_FETCH, /* used to be ID_RESTORE_INPUT */
  ID_ROLL,
  ID_ROT,
  ID_RP_FETCH,
  ID_RP_STORE,
  ID_RSHIFT,
  ID_R_DROP,
  ID_R_FETCH,
  ID_R_FROM,
  ID_SAVE_FORTH_P,
  ID_SOURCE_LINE_NUMBER_STORE, /* used to be ID_SAVE_INPUT */
  ID_SCAN,
  ID_SEMICOLON,
  ID_SKIP,
  ID_SOURCE,
  ID_SOURCE_ID,
  ID_SOURCE_ID_POP,
  ID_SOURCE_ID_PUSH,
  ID_SOURCE_SET,
  ID_SP_FETCH,
  ID_SP_STORE,
  ID_STORE,
  ID_SWAP,
  ID_TEST1,
  ID_TEST2,
  ID_TEST3,
  ID_TICK,
  ID_TIMES,
  ID_TO_R,
  ID_TYPE,
  ID_TYPE_P,
  ID_VAR_BASE,
  ID_VAR_CODE_BASE,
  ID_VAR_CODE_LIMIT,
  ID_VAR_CONTEXT,
  ID_VAR_DP,
  ID_VAR_ECHO,
  ID_VAR_HEADERS_BASE,
  ID_VAR_HEADERS_LIMIT,
  ID_VAR_HEADERS_PTR,
  ID_VAR_NUM_TIB,
  ID_VAR_OUT,
  ID_VAR_RETURN_CODE,
  ID_VAR_SOURCE_ID,
  ID_VAR_STATE,
  ID_VAR_TO_IN,
  ID_VAR_TRACE_FLAGS,
  ID_VAR_TRACE_LEVEL,
  ID_VAR_TRACE_STACK,
  ID_VLIST,
  ID_WORD,
  ID_WORD_FETCH,
  ID_WORD_STORE,
  ID_XOR,
  ID_ZERO_BRANCH,
  ID_CATCH,
  ID_THROW,
  ID_INTERPRET,
  ID_FILE_WO,
  ID_FILE_BIN,
  /* Added to support 64 bit operation. */
  ID_CELL,
  ID_CELLS,
  /* DELETE-FILE */
  ID_FILE_DELETE,
  ID_FILE_FLUSH,   /* FLUSH-FILE */
  ID_FILE_RENAME,  /* (RENAME-FILE) */
  ID_FILE_RESIZE,  /* RESIZE-FILE */
  ID_SLEEP_P,      /* (SLEEP) V2.0.0 */
  ID_VAR_BYE_CODE, /* BYE-CODE */
  ID_VERSION_CODE,
/* If you add a word here, take away one reserved word below. */
#ifdef PF_SUPPORT_FP
  /* Only reserve space if we are adding FP so that we can detect
  ** unsupported primitives when loading dictionary.
  */
  ID_RESERVED03,
  ID_RESERVED04,
  ID_RESERVED05,
  ID_RESERVED06,
  ID_RESERVED07,
  ID_RESERVED08,
  ID_RESERVED09,
  ID_FP_D_TO_F,
  ID_FP_FSTORE,
  ID_FP_FTIMES,
  ID_FP_FPLUS,
  ID_FP_FMINUS,
  ID_FP_FSLASH,
  ID_FP_F_ZERO_LESS_THAN,
  ID_FP_F_ZERO_EQUALS,
  ID_FP_F_LESS_THAN,
  ID_FP_F_TO_D,
  ID_FP_FFETCH,
  ID_FP_FDEPTH,
  ID_FP_FDROP,
  ID_FP_FDUP,
  ID_FP_FLITERAL,
  ID_FP_FLITERAL_P,
  ID_FP_FLOAT_PLUS,
  ID_FP_FLOATS,
  ID_FP_FLOOR,
  ID_FP_FMAX,
  ID_FP_FMIN,
  ID_FP_FNEGATE,
  ID_FP_FOVER,
  ID_FP_FROT,
  ID_FP_FROUND,
  ID_FP_FSWAP,
  ID_FP_FSTAR_STAR,
  ID_FP_FABS,
  ID_FP_FACOS,
  ID_FP_FACOSH,
  ID_FP_FALOG,
  ID_FP_FASIN,
  ID_FP_FASINH,
  ID_FP_FATAN,
  ID_FP_FATAN2,
  ID_FP_FATANH,
  ID_FP_FCOS,
  ID_FP_FCOSH,
  ID_FP_FLN,
  ID_FP_FLNP1,
  ID_FP_FLOG,
  ID_FP_FSIN,
  ID_FP_FSINCOS,
  ID_FP_FSINH,
  ID_FP_FSQRT,
  ID_FP_FTAN,
  ID_FP_FTANH,
  ID_FP_FPICK,
#endif

  //********** Raylib XT values **********
  // rcore - Window-related functions
  XT_INIT_WINDOW,
  XT_CLOSE_WINDOW,
  XT_WINDOW_SHOULD_CLOSE,
  XT_IS_WINDOW_READY,
  XT_IS_WINDOW_FULLSCREEN,
  XT_IS_WINDOW_HIDDEN,
  XT_IS_WINDOW_MINIMIZED,
  XT_IS_WINDOW_MAXIMIZED,
  XT_IS_WINDOW_FOCUSED,
  XT_IS_WINDOW_RESIZED,
  XT_IS_WINDOW_STATE,
  XT_SET_WINDOW_STATE,
  XT_CLEAR_WINDOW_STATE,
  XT_TOGGLE_FULLSCREEN,
  XT_TOGGLE_BORDERLESS_WINDOWED,
  XT_MAXIMIZE_WINDOW,
  XT_MINIMIZE_WINDOW,
  XT_RESTORE_WINDOW,
  XT_SET_WINDOW_ICON,
  XT_SET_WINDOW_ICONS,
  XT_SET_WINDOW_TITLE,
  XT_SET_WINDOW_POSITION,
  XT_SET_WINDOW_MONITOR,
  XT_SET_WINDOW_MIN_SIZE,
  XT_SET_WINDOW_MAX_SIZE,
  XT_SET_WINDOW_SIZE,
  XT_SET_WINDOW_OPACITY,
  XT_SET_WINDOW_FOCUSED,
  XT_GET_WINDOW_HANDLE,
  XT_GET_SCREEN_WIDTH,
  XT_GET_SCREEN_HEIGHT,
  XT_GET_RENDER_WIDTH,
  XT_GET_RENDER_HEIGHT,
  XT_GET_MONITOR_COUNT,
  XT_GET_CURRENT_MONITOR,
  XT_GET_MONITOR_POSITION,
  XT_GET_MONITOR_WIDTH,
  XT_GET_MONITOR_HEIGHT,
  XT_GET_MONITOR_PHYSICAL_WIDTH,
  XT_GET_MONITOR_PHYSICAL_HEIGHT,
  XT_GET_MONITOR_REFRESH_RATE,
  XT_GET_WINDOW_POSITION,
  XT_GET_WINDOW_SCALE_DPI,
  XT_GET_MONITOR_NAME,
  XT_SET_CLIPBOARD_TEXT,
  XT_GET_CLIPBOARD_TEXT,
  XT_ENABLE_EVENT_WAITING,
  XT_DISABLE_EVENT_WAITING,
  // Cursor-related functions
  XT_SHOW_CURSOR,
  XT_HIDE_CURSOR,
  XT_IS_CURSOR_HIDDEN,
  XT_ENABLE_CURSOR,
  XT_DISABLE_CURSOR,
  XT_IS_CURSOR_ON_SCREEN,
  // Drawing-related functions
  XT_CLEAR_BACKGROUND,
  XT_BEGIN_DRAWING,
  XT_END_DRAWING,
  XT_BEGIN_MODE2D,
  XT_END_MODE2D,
  XT_BEGIN_MODE3D,
  XT_END_MODE3D,
  XT_BEGIN_TEXTURE_MODE,
  XT_END_TEXTURE_MODE,
  XT_BEGIN_SHADER_MODE,
  XT_END_SHADER_MODE,
  XT_BEGIN_BLEND_MODE,
  XT_END_BLEND_MODE,
  XT_BEGIN_SCISSOR_MODE,
  XT_END_SCISSOR_MODE,
  XT_BEGIN_VR_STEREO_MODE,
  XT_END_VR_STEREO_MODE,
  // VR stereo config functions for VR simulator
  XT_LOAD_VR_STEREO_CONFIG,
  XT_UNLOAD_VR_STEREO_CONFIG,
  // Shader management functions
  XT_LOAD_SHADER,
  XT_LOAD_SHADER_FROM_MEMORY,
  XT_IS_SHADER_READY,
  XT_GET_SHADER_LOCATION,
  XT_GET_SHADER_LOCATION_ATTRIB,
  XT_SET_SHADER_VALUE,
  XT_SET_SHADER_VALUE_V,
  XT_SET_SHADER_VALUE_MATRIX,
  XT_SET_SHADER_VALUE_TEXTURE,
  XT_UNLOAD_SHADER,
  // Screen-space-related functions
  XT_GET_MOUSE_RAY,
  XT_GET_CAMERA_MATRIX,
  XT_GET_CAMERA_MATRIX_2D,
  XT_GET_WORLD_TO_SCREEN,
  XT_GET_SCREEN_TO_WORLD_2D,
  XT_GET_WORLD_TO_SCREEN_EX,
  XT_GET_WORLD_TO_SCREEN_2D,
  // Timing-related functions
  XT_SET_TARGET_FPS,
  XT_GET_FRAME_TIME,
  XT_GET_TIME,
  XT_GET_FPS,
  // Custom frame control functions
  XT_SWAP_SCREEN_BUFFER,
  XT_POLL_INPUT_EVENTS,
  XT_WAIT_TIME,
  // Random values generation functions
  XT_SET_RANDOM_SEED,
  XT_GET_RANDOM_VALUE,
  XT_LOAD_RANDOM_SEQUENCE,
  XT_UNLOAD_RANDOM_SEQUENCE,
  // Misc. functions
  XT_TAKE_SCREENSHOT,
  XT_SET_CONFIG_FLAGS,
  XT_OPEN_URL,
  XT_TRACE_LOG,
  XT_SET_TRACE_LOG_LEVEL,
  XT_MEM_ALLOC,
  XT_MEM_REALLOC,
  XT_MEM_FREE,
  // Set custom callbacks
  XT_SET_TRACE_LOG_CALLBACK,
  XT_SET_LOAD_FILE_DATA_CALLBACK,
  XT_SET_SAVE_FILE_DATA_CALLBACK,
  XT_SET_LOAD_FILE_TEXT_CALLBACK,
  XT_SET_SAVE_FILE_TEXT_CALLBACK,
  // Files management functions
  XT_LOAD_FILE_DATA,
  XT_UNLOAD_FILE_DATA,
  XT_SAVE_FILE_DATA,
  XT_EXPORT_DATA_AS_CODE,
  XT_LOAD_FILE_TEXT,
  XT_UNLOAD_FILE_TEXT,
  XT_SAVE_FILE_TEXT,
  XT_FILE_EXISTS,
  XT_DIRECTORY_EXISTS,
  XT_IS_FILE_EXTENSION,
  XT_GET_FILE_LENGTH,
  XT_GET_FILE_EXTENSION,
  XT_GET_FILE_NAME,
  XT_GET_FILE_NAME_WITHOUT_EXT,
  XT_GET_DIRECTORY_PATH,
  XT_GET_PREV_DIRECTORY_PATH,
  XT_GET_WORKING_DIRECTORY,
  XT_GET_APPLICATION_DIRECTORY,
  XT_CHANGE_DIRECTORY,
  XT_IS_PATH_FILE,
  XT_LOAD_DIRECTORY_FILES,
  XT_LOAD_DIRECTORY_FILES_EX,
  XT_UNLOAD_DIRECTORY_FILES,
  XT_IS_FILE_DROPPED,
  XT_LOAD_DROPPED_FILES,
  XT_UNLOAD_DROPPED_FILES,
  XT_GET_FILE_MOD_TIME,
  // Compression/Encoding functionality
  XT_COMPRESS_DATA,
  XT_DECOMPRESS_DATA,
  XT_ENCODE_DATA_BASE64,
  XT_DECODE_DATA_BASE64,
  // Automation events functionality
  XT_LOAD_AUTOMATION_EVENT_LIST,
  XT_UNLOAD_AUTOMATION_EVENT_LIST,
  XT_EXPORT_AUTOMATION_EVENT_LIST,
  XT_SET_AUTOMATION_EVENT_LIST,
  XT_SET_AUTOMATION_EVENT_BASE_FRAME,
  XT_START_AUTOMATION_EVENT_RECORDING,
  XT_STOP_AUTOMATION_EVENT_RECORDING,
  XT_PLAY_AUTOMATION_EVENT,
  // Input-related functions: keyboard
  XT_IS_KEY_PRESSED,
  XT_IS_KEY_PRESSED_REPEAT,
  XT_IS_KEY_DOWN,
  XT_IS_KEY_RELEASED,
  XT_IS_KEY_UP,
  XT_GET_KEY_PRESSED,
  XT_GET_CHAR_PRESSED,
  XT_SET_EXIT_KEY,
  // Input-related functions: gamepads
  XT_IS_GAMEPAD_AVAILABLE,
  XT_GET_GAMEPAD_NAME,
  XT_IS_GAMEPAD_BUTTON_PRESSED,
  XT_IS_GAMEPAD_BUTTON_DOWN,
  XT_IS_GAMEPAD_BUTTON_RELEASED,
  XT_IS_GAMEPAD_BUTTON_UP,
  XT_GET_GAMEPAD_BUTTON_PRESSED,
  XT_GET_GAMEPAD_AXIS_COUNT,
  XT_GET_GAMEPAD_AXIS_MOVEMENT,
  XT_SET_GAMEPAD_MAPPINGS,
  // Input-related functions: mouse
  XT_IS_MOUSE_BUTTON_PRESSED,
  XT_IS_MOUSE_BUTTON_DOWN,
  XT_IS_MOUSE_BUTTON_RELEASED,
  XT_IS_MOUSE_BUTTON_UP,
  XT_GET_MOUSE_X,
  XT_GET_MOUSE_Y,
  XT_GET_MOUSE_POSITION,
  XT_GET_MOUSE_DELTA,
  XT_SET_MOUSE_POSITION,
  XT_SET_MOUSE_OFFSET,
  XT_SET_MOUSE_SCALE,
  XT_GET_MOUSE_WHEEL_MOVE,
  XT_GET_MOUSE_WHEEL_MOVE_V,
  XT_SET_MOUSE_CURSOR,
  // Input-related functions: touch
  XT_GET_TOUCH_X,
  XT_GET_TOUCH_Y,
  XT_GET_TOUCH_POSITION,
  XT_GET_TOUCH_POINT_ID,
  XT_GET_TOUCH_POINT_COUNT,
  // Gestures and Touch Handling Functions (Module: rgestures)
  XT_SET_GESTURES_ENABLED,
  XT_IS_GESTURE_DETECTED,
  XT_GET_GESTURE_DETECTED,
  XT_GET_GESTURE_HOLD_DURATION,
  XT_GET_GESTURE_DRAG_VECTOR,
  XT_GET_GESTURE_DRAG_ANGLE,
  XT_GET_GESTURE_PINCH_VECTOR,
  XT_GET_GESTURE_PINCH_ANGLE,
  // Camera System Functions (Module: rcamera)
  XT_UPDATE_CAMERA,
  XT_UPDATE_CAMERA_PRO,
  //
  // rshapes
  // Basic shapes drawing functions
  XT_SET_SHAPES_TEXTURE,
  XT_DRAW_PIXEL,
  XT_DRAW_PIXEL_V,
  XT_DRAW_LINE,
  XT_DRAW_LINE_V,
  XT_DRAW_LINE_EX,
  XT_DRAW_LINE_STRIP,
  XT_DRAW_LINE_BEZIER,
  XT_DRAW_CIRCLE,
  XT_DRAW_CIRCLE_SECTOR,
  XT_DRAW_CIRCLE_SECTOR_LINES,
  XT_DRAW_CIRCLE_GRADIENT,
  XT_DRAW_CIRCLE_V,
  XT_DRAW_CIRCLE_LINES,
  XT_DRAW_CIRCLE_LINES_V,
  XT_DRAW_ELLIPSE,
  XT_DRAW_ELLIPSE_LINES,
  XT_DRAW_RING,
  XT_DRAW_RING_LINES,
  XT_DRAW_RECTANGLE,
  XT_DRAW_RECTANGLE_V,
  XT_DRAW_RECTANGLE_REC,
  XT_DRAW_RECTANGLE_PRO,
  XT_DRAW_RECTANGLE_GRADIENT_V,
  XT_DRAW_RECTANGLE_GRADIENT_H,
  XT_DRAW_RECTANGLE_GRADIENT_EX,
  XT_DRAW_RECTANGLE_LINES,
  XT_DRAW_RECTANGLE_LINES_EX,
  XT_DRAW_RECTANGLE_ROUNDED,
  XT_DRAW_RECTANGLE_ROUNDED_LINES,
  XT_DRAW_TRIANGLE,
  XT_DRAW_TRIANGLE_LINES,
  XT_DRAW_TRIANGLE_FAN,
  XT_DRAW_TRIANGLE_STRIP,
  XT_DRAW_POLY,
  XT_DRAW_POLY_LINES,
  XT_DRAW_POLY_LINES_EX,
  // Splines drawing functions
  XT_DRAW_SPLINE_LINEAR,
  XT_DRAW_SPLINE_BASIS,
  XT_DRAW_SPLINE_CATMULL_ROM,
  XT_DRAW_SPLINE_BEZIER_QUADRATIC,
  XT_DRAW_SPLINE_BEZIER_CUBIC,
  XT_DRAW_SPLINE_SEGMENT_LINEAR,
  XT_DRAW_SPLINE_SEGMENT_BASIS,
  XT_DRAW_SPLINE_SEGMENT_CATMULL_ROM,
  XT_DRAW_SPLINE_SEGMENT_BEZIER_QUADRATIC,
  XT_DRAW_SPLINE_SEGMENT_BEZIER_CUBIC,
  // Spline segment point evaluation functions, for a given t [0.0f .. 1.0f]
  XT_GET_SPLINE_POINT_LINEAR,
  XT_GET_SPLINE_POINT_BASIS,
  XT_GET_SPLINE_POINT_CATMULL_ROM,
  XT_GET_SPLINE_POINT_BEZIER_QUAD,
  XT_GET_SPLINE_POINT_BEZIER_CUBIC,
  // Basic shapes collision detection functions
  XT_CHECK_COLLISION_RECS,
  XT_CHECK_COLLISION_CIRCLES,
  XT_CHECK_COLLISION_CIRCLE_REC,
  XT_CHECK_COLLISION_POINT_REC,
  XT_CHECK_COLLISION_POINT_CIRCLE,
  XT_CHECK_COLLISION_POINT_TRIANGLE,
  XT_CHECK_COLLISION_POINT_POLY,
  XT_CHECK_COLLISION_LINES,
  XT_CHECK_COLLISION_POINT_LINE,
  XT_GET_COLLISION_REC,
  //
  // rtextures
  // Image loading functions
  XT_LOAD_IMAGE,
  XT_LOAD_IMAGE_RAW,
  XT_LOAD_IMAGE_SVG,
  XT_LOAD_IMAGE_ANIM,
  XT_LOAD_IMAGE_FROM_MEMORY,
  XT_LOAD_IMAGE_FROM_TEXTURE,
  XT_LOAD_IMAGE_FROM_SCREEN,
  XT_IS_IMAGE_READY,
  XT_UNLOAD_IMAGE,
  XT_EXPORT_IMAGE,
  XT_EXPORT_IMAGE_TO_MEMORY,
  XT_EXPORT_IMAGE_AS_CODE,
  // Image generation functions
  XT_GEN_IMAGE_COLOR,
  XT_GEN_IMAGE_GRADIENT_LINEAR,
  XT_GEN_IMAGE_GRADIENT_RADIAL,
  XT_GEN_IMAGE_GRADIENT_SQUARE,
  XT_GEN_IMAGE_CHECKED,
  XT_GEN_IMAGE_WHITE_NOISE,
  XT_GEN_IMAGE_PERLIN_NOISE,
  XT_GEN_IMAGE_CELLULAR,
  XT_GEN_IMAGE_TEXT,
  // Image manipulation functions
  XT_IMAGE_COPY,
  XT_IMAGE_FROM_IMAGE,
  XT_IMAGE_TEXT,
  XT_IMAGE_TEXT_EX,
  XT_IMAGE_FORMAT,
  XT_IMAGE_TO_POT,
  XT_IMAGE_CROP,
  XT_IMAGE_ALPHA_CROP,
  XT_IMAGE_ALPHA_CLEAR,
  XT_IMAGE_ALPHA_MASK,
  XT_IMAGE_ALPHA_PREMULTIPLY,
  XT_IMAGE_BLUR_GAUSSIAN,
  XT_IMAGE_RESIZE,
  XT_IMAGE_RESIZE_NN,
  XT_IMAGE_RESIZE_CANVAS,
  XT_IMAGE_MIPMAPS,
  XT_IMAGE_DITHER,
  XT_IMAGE_FLIP_VERTICAL,
  XT_IMAGE_FLIP_HORIZONTAL,
  XT_IMAGE_ROTATE,
  XT_IMAGE_ROTATE_CW,
  XT_IMAGE_ROTATE_CCW,
  XT_IMAGE_COLOR_TINT,
  XT_IMAGE_COLOR_INVERT,
  XT_IMAGE_COLOR_GRAYSCALE,
  XT_IMAGE_COLOR_CONTRAST,
  XT_IMAGE_COLOR_BRIGHTNESS,
  XT_IMAGE_COLOR_REPLACE,
  XT_LOAD_IMAGE_COLORS,
  XT_LOAD_IMAGE_PALETTE,
  XT_UNLOAD_IMAGE_COLORS,
  XT_UNLOAD_IMAGE_PALETTE,
  XT_GET_IMAGE_ALPHA_BORDER,
  XT_GET_IMAGE_COLOR,
  // Image drawing functions
  XT_IMAGE_CLEAR_BACKGROUND,
  XT_IMAGE_DRAW_PIXEL,
  XT_IMAGE_DRAW_PIXEL_V,
  XT_IMAGE_DRAW_LINE,
  XT_IMAGE_DRAW_LINE_V,
  XT_IMAGE_DRAW_CIRCLE,
  XT_IMAGE_DRAW_CIRCLE_V,
  XT_IMAGE_DRAW_CIRCLE_LINES,
  XT_IMAGE_DRAW_CIRCLE_LINES_V,
  XT_IMAGE_DRAW_RECTANGLE,
  XT_IMAGE_DRAW_RECTANGLE_V,
  XT_IMAGE_DRAW_RECTANGLE_REC,
  XT_IMAGE_DRAW_RECTANGLE_LINES,
  XT_IMAGE_DRAW,
  XT_IMAGE_DRAW_TEXT,
  XT_IMAGE_DRAW_TEXT_EX,
  // Texture loading functions
  XT_LOAD_TEXTURE,
  XT_LOAD_TEXTURE_FROM_IMAGE,
  XT_LOAD_TEXTURE_CUBEMAP,
  XT_LOAD_RENDER_TEXTURE,
  XT_IS_TEXTURE_READY,
  XT_UNLOAD_TEXTURE,
  XT_IS_RENDER_TEXTURE_READY,
  XT_UNLOAD_RENDER_TEXTURE,
  XT_UPDATE_TEXTURE,
  XT_UPDATE_TEXTURE_REC,
  // Texture configuration functions
  XT_GEN_TEXTURE_MIPMAPS,
  XT_SET_TEXTURE_FILTER,
  XT_SET_TEXTURE_WRAP,
  // Texture drawing functions
  XT_DRAW_TEXTURE,
  XT_DRAW_TEXTURE_V,
  XT_DRAW_TEXTURE_EX,
  XT_DRAW_TEXTURE_REC,
  XT_DRAW_TEXTURE_PRO,
  XT_DRAW_TEXTURE_NPATCH,
  // Color/pixel related functions
  XT_FADE,
  XT_COLOR_TO_INT,
  XT_COLOR_NORMALIZE,
  XT_COLOR_FROM_NORMALIZED,
  XT_COLOR_TO_HSV,
  XT_COLOR_FROM_HSV,
  XT_COLOR_TINT,
  XT_COLOR_BRIGHTNESS,
  XT_COLOR_CONTRAST,
  XT_COLOR_ALPHA,
  XT_COLOR_ALPHA_BLEND,
  XT_GET_COLOR,
  XT_GET_PIXEL_COLOR,
  XT_SET_PIXEL_COLOR,
  XT_GET_PIXEL_DATA_SIZE,
  //
  // rtext
  // Font loading/unloading functions
  XT_GET_FONT_DEFAULT,
  XT_LOAD_FONT,
  XT_LOAD_FONT_EX,
  XT_LOAD_FONT_FROM_IMAGE,
  XT_LOAD_FONT_FROM_MEMORY,
  XT_IS_FONT_READY,
  XT_LOAD_FONT_DATA,
  XT_GEN_IMAGE_FONT_ATLAS,
  XT_UNLOAD_FONT_DATA,
  XT_UNLOAD_FONT,
  XT_EXPORT_FONT_AS_CODE,
  // Text drawing functions
  XT_DRAW_FPS,
  XT_DRAW_TEXT,
  XT_DRAW_TEXT_EX,
  XT_DRAW_TEXT_PRO,
  XT_DRAW_TEXT_CODEPOINT,
  XT_DRAW_TEXT_CODEPOINTS,
  // Text font info functions
  XT_SET_TEXT_LINE_SPACING,
  XT_MEASURE_TEXT,
  XT_MEASURE_TEXT_EX,
  XT_GET_GLYPH_INDEX,
  XT_GET_GLYPH_INFO,
  XT_GET_GLYPH_ATLAS_REC,
  // Text codepoints management functions (unicode characters)
  XT_LOAD_UTF8,
  XT_UNLOAD_UTF8,
  XT_LOAD_CODEPOINTS,
  XT_UNLOAD_CODEPOINTS,
  XT_GET_CODEPOINT_COUNT,
  XT_GET_CODEPOINT,
  XT_GET_CODEPOINT_NEXT,
  XT_GET_CODEPOINT_PREVIOUS,
  XT_CODEPOINT_TO_UTF8,
  // Text strings management functions (no UTF-8 strings, only byte chars)
  XT_TEXT_COPY,
  XT_TEXT_IS_EQUAL,
  XT_TEXT_LENGTH,
  XT_TEXT_FORMAT,
  XT_TEXT_SUBTEXT,
  XT_TEXT_REPLACE,
  XT_TEXT_INSERT,
  XT_TEXT_JOIN,
  XT_TEXT_SPLIT,
  XT_TEXT_APPEND,
  XT_TEXT_FIND_INDEX,
  XT_TEXT_TO_UPPER,
  XT_TEXT_TO_LOWER,
  XT_TEXT_TO_PASCAL,
  XT_TEXT_TO_INTEGER,
  //
  // rmodels
  // Basic geometric 3D shapes drawing functions
  XT_DRAW_LINE_3D,
  XT_DRAW_POINT_3D,
  XT_DRAW_CIRCLE_3D,
  XT_DRAW_TRIANGLE_3D,
  XT_DRAW_TRIANGLE_STRIP_3D,
  XT_DRAW_CUBE,
  XT_DRAW_CUBE_V,
  XT_DRAW_CUBE_WIRES,
  XT_DRAW_CUBE_WIRES_V,
  XT_DRAW_SPHERE,
  XT_DRAW_SPHERE_EX,
  XT_DRAW_SPHERE_WIRES,
  XT_DRAW_CYLINDER,
  XT_DRAW_CYLINDER_EX,
  XT_DRAW_CYLINDER_WIRES,
  XT_DRAW_CYLINDER_WIRES_EX,
  XT_DRAW_CAPSULE,
  XT_DRAW_CAPSULE_WIRES,
  XT_DRAW_PLANE,
  XT_DRAW_RAY,
  XT_DRAW_GRID,
  // Model management functions
  XT_LOAD_MODEL,
  XT_LOAD_MODEL_FROM_MESH,
  XT_IS_MODEL_READY,
  XT_UNLOAD_MODEL,
  XT_GET_MODEL_BOUNDING_BOX,
  // Model drawing functions
  XT_DRAW_MODEL,
  XT_DRAW_MODEL_EX,
  XT_DRAW_MODEL_WIRES,
  XT_DRAW_MODEL_WIRES_EX,
  XT_DRAW_BOUNDING_BOX,
  XT_DRAW_BILLBOARD,
  XT_DRAW_BILLBOARD_REC,
  XT_DRAW_BILLBOARD_PRO,
  // Mesh management functions
  XT_UPLOAD_MESH,
  XT_UPDATE_MESH_BUFFER,
  XT_UNLOAD_MESH,
  XT_DRAW_MESH,
  XT_DRAW_MESH_INSTANCED,
  XT_EXPORT_MESH,
  XT_GET_MESH_BOUNDING_BOX,
  XT_GEN_MESH_TANGENTS,
  // Mesh generation functions
  XT_GEN_MESH_POLY,
  XT_GEN_MESH_PLANE,
  XT_GEN_MESH_CUBE,
  XT_GEN_MESH_SPHERE,
  XT_GEN_MESH_HEMI_SPHERE,
  XT_GEN_MESH_CYLINDER,
  XT_GEN_MESH_CONE,
  XT_GEN_MESH_TORUS,
  XT_GEN_MESH_KNOT,
  XT_GEN_MESH_HEIGHTMAP,
  XT_GEN_MESH_CUBICMAP,
  // Material loading/unloading functions
  XT_LOAD_MATERIALS,
  XT_LOAD_MATERIAL_DEFAULT,
  XT_IS_MATERIAL_READY,
  XT_UNLOAD_MATERIAL,
  XT_SET_MATERIAL_TEXTURE,
  XT_SET_MODEL_MESH_MATERIAL,
  // Model animations loading/unloading functions
  XT_LOAD_MODEL_ANIMATIONS,
  XT_UPDATE_MODEL_ANIMATION,
  XT_UNLOAD_MODEL_ANIMATION,
  XT_UNLOAD_MODEL_ANIMATIONS,
  XT_IS_MODEL_ANIMATION_VALID,
  // Collision detection functions
  XT_CHECK_COLLISION_SPHERES,
  XT_CHECK_COLLISION_BOXES,
  XT_CHECK_COLLISION_BOX_SPHERE,
  XT_GET_RAY_COLLISION_SPHERE,
  XT_GET_RAY_COLLISION_BOX,
  XT_GET_RAY_COLLISION_MESH,
  XT_GET_RAY_COLLISION_TRIANGLE,
  XT_GET_RAY_COLLISION_QUAD,
  //
  // raudio
  // Audio device management functions
  XT_INIT_AUDIO_DEVICE,
  XT_CLOSE_AUDIO_DEVICE,
  XT_IS_AUDIO_DEVICE_READY,
  XT_SET_MASTER_VOLUME,
  XT_GET_MASTER_VOLUME,
  // Wave/Sound loading/unloading functions
  XT_LOAD_WAVE,
  XT_LOAD_WAVE_FROM_MEMORY,
  XT_IS_WAVE_READY,
  XT_LOAD_SOUND,
  XT_LOAD_SOUND_FROM_WAVE,
  XT_LOAD_SOUND_ALIAS,
  XT_IS_SOUND_READY,
  XT_UPDATE_SOUND,
  XT_UNLOAD_WAVE,
  XT_UNLOAD_SOUND,
  XT_UNLOAD_SOUND_ALIAS,
  XT_EXPORT_WAVE,
  XT_EXPORT_WAVE_AS_CODE,
  // Wave/Sound management functions
  XT_PLAY_SOUND,
  XT_STOP_SOUND,
  XT_PAUSE_SOUND,
  XT_RESUME_SOUND,
  XT_IS_SOUND_PLAYING,
  XT_SET_SOUND_VOLUME,
  XT_SET_SOUND_PITCH,
  XT_SET_SOUND_PAN,
  XT_WAVE_COPY,
  XT_WAVE_CROP,
  XT_WAVE_FORMAT,
  XT_LOAD_WAVE_SAMPLES,
  XT_UNLOAD_WAVE_SAMPLES,
  // Music management functions
  XT_LOAD_MUSIC_STREAM,
  XT_LOAD_MUSIC_STREAM_FROM_MEMORY,
  XT_IS_MUSIC_READY,
  XT_UNLOAD_MUSIC_STREAM,
  XT_PLAY_MUSIC_STREAM,
  XT_IS_MUSIC_STREAM_PLAYING,
  XT_UPDATE_MUSIC_STREAM,
  XT_STOP_MUSIC_STREAM,
  XT_PAUSE_MUSIC_STREAM,
  XT_RESUME_MUSIC_STREAM,
  XT_SEEK_MUSIC_STREAM,
  XT_SET_MUSIC_VOLUME,
  XT_SET_MUSIC_PITCH,
  XT_SET_MUSIC_PAN,
  XT_GET_MUSIC_TIME_LENGTH,
  XT_GET_MUSIC_TIME_PLAYED,
  // AudioStream management functions
  XT_LOAD_AUDIO_STREAM,
  XT_IS_AUDIO_STREAM_READY,
  XT_UNLOAD_AUDIO_STREAM,
  XT_UPDATE_AUDIO_STREAM,
  XT_IS_AUDIO_STREAM_PROCESSED,
  XT_PLAY_AUDIO_STREAM,
  XT_PAUSE_AUDIO_STREAM,
  XT_RESUME_AUDIO_STREAM,
  XT_IS_AUDIO_STREAM_PLAYING,
  XT_STOP_AUDIO_STREAM,
  XT_SET_AUDIO_STREAM_VOLUME,
  XT_SET_AUDIO_STREAM_PITCH,
  XT_SET_AUDIO_STREAM_PAN,
  XT_SET_AUDIO_STREAM_BUFFER_SIZE_DEFAULT,
  XT_SET_AUDIO_STREAM_CALLBACK,
  XT_ATTACH_AUDIO_STREAM_PROCESSOR,
  XT_DETACH_AUDIO_STREAM_PROCESSOR,
  XT_ATTACH_AUDIO_MIXED_PROCESSOR,
  XT_DETACH_AUDIO_MIXED_PROCESSOR,

  NUM_PRIMITIVES /* This must always be LAST */
};


/***************************************************************
** THROW Codes
***************************************************************/
/* ANSI standard definitions needed by pForth */
#define THROW_ABORT            (-1)
#define THROW_ABORT_QUOTE      (-2)
#define THROW_STACK_OVERFLOW   (-3)
#define THROW_STACK_UNDERFLOW  (-4)
#define THROW_UNDEFINED_WORD  (-13)
#define THROW_EXECUTING       (-14)
#define THROW_PAIRS           (-22)
#define THROW_FLOAT_STACK_UNDERFLOW  ( -45)
#define THROW_QUIT            (-56)
#define THROW_FLUSH_FILE      (-68)
#define THROW_RESIZE_FILE     (-74)

/* THROW codes unique to pForth */
#define THROW_BYE            (-256) /* Exit program. */
#define THROW_SEMICOLON      (-257) /* Error detected at ; */
#define THROW_DEFERRED       (-258) /* Not a deferred word. Used in system.fth */

/***************************************************************
** Structures
***************************************************************/

typedef struct pfTaskData_s
{
    cell_t   *td_StackPtr;       /* Primary data stack */
    cell_t   *td_StackBase;
    cell_t   *td_StackLimit;
    cell_t   *td_ReturnPtr;      /* Return stack */
    cell_t   *td_ReturnBase;
    cell_t   *td_ReturnLimit;
#ifdef PF_SUPPORT_FP
    PF_FLOAT  *td_FloatStackPtr;
    PF_FLOAT  *td_FloatStackBase;
    PF_FLOAT  *td_FloatStackLimit;
#endif
    cell_t   *td_InsPtr;          /* Instruction pointer, "PC" */
    FileStream   *td_InputStream;
/* Terminal. */
    char    td_TIB[TIB_SIZE];   /* Buffer for terminal input. */
    cell_t    td_IN;              /* Index into Source */
    cell_t    td_SourceNum;       /* #TIB after REFILL */
    char   *td_SourcePtr;       /* Pointer to TIB or other source. */
    cell_t   td_LineNumber;      /* Incremented on every refill. */
    cell_t    td_OUT;             /* Current output column. */
} pfTaskData_t;

typedef struct pfNode
{
    struct pfNode *n_Next;
    struct pfNode *n_Prev;
} pfNode;

/* Structure of header entry in dictionary. These will be stored in dictionary specific endian format*/
typedef struct cfNameLinks
{
    cell_t       cfnl_PreviousName;   /* name relative address of previous */
    ExecToken  cfnl_ExecToken;      /* Execution token for word. */
/* Followed by variable length name field. */
} cfNameLinks;

#define PF_DICF_ALLOCATED_SEGMENTS  ( 0x0001)
typedef struct pfDictionary_s
{
    pfNode  dic_Node;
    ucell_t  dic_Flags;
/* Headers contain pointers to names and dictionary. */

    ucell_t dic_HeaderBaseUnaligned;

    ucell_t dic_HeaderBase;
    ucell_t dic_HeaderPtr;
    ucell_t dic_HeaderLimit;
/* Code segment contains tokenized code and data. */
    ucell_t dic_CodeBaseUnaligned;
    ucell_t dic_CodeBase;
    union
    {
        cell_t  *Cell;
        uint8_t *Byte;
    } dic_CodePtr;
    ucell_t dic_CodeLimit;
} pfDictionary_t;

/* Save state of include when nesting files. */
typedef struct IncludeFrame
{
    FileStream   *inf_FileID;
    cell_t         inf_LineNumber;
    cell_t         inf_SourceNum;
    cell_t         inf_IN;
    char          inf_SaveTIB[TIB_SIZE];
} IncludeFrame;

#define MAX_INCLUDE_DEPTH (16)

/***************************************************************
** Prototypes
***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

ThrowCode pfCatch( ExecToken XT );

#ifdef __cplusplus
}
#endif

/***************************************************************
** External Globals
***************************************************************/
extern pfTaskData_t *gCurrentTask;
extern pfDictionary_t *gCurrentDictionary;
extern char          gScratch[TIB_SIZE];
extern cell_t         gNumPrimitives;

extern ExecToken     gLocalCompiler_XT;      /* CFA of (LOCAL) compiler. */
extern ExecToken     gNumberQ_XT;         /* XT of NUMBER? */
extern ExecToken     gQuitP_XT;           /* XT of (QUIT) */
extern ExecToken     gAcceptP_XT;         /* XT of ACCEPT */

#define DEPTH_AT_COLON_INVALID (-100)
extern cell_t         gDepthAtColon;

/* Global variables. */
extern cell_t        gVarContext;    /* Points to last name field. */
extern cell_t        gVarState;      /* 1 if compiling. */
extern cell_t        gVarBase;       /* Numeric Base. */
extern cell_t        gVarByeCode;    /* BYE-CODE returned on exit */
extern cell_t        gVarEcho;       /* Echo input from file. */
extern cell_t        gVarEchoAccept; /* Echo input from ACCEPT. */
extern cell_t        gVarTraceLevel;
extern cell_t        gVarTraceStack;
extern cell_t        gVarTraceFlags;
extern cell_t        gVarQuiet;      /* Suppress unnecessary messages, OK, etc. */
extern cell_t        gVarReturnCode; /* Returned to caller of Forth, eg. UNIX shell. */

extern IncludeFrame  gIncludeStack[MAX_INCLUDE_DEPTH];
extern cell_t         gIncludeIndex;
/***************************************************************
** Macros
***************************************************************/


/* Endian specific macros for creating target dictionaries for machines with

** different endian-ness.

*/

#if defined(PF_BIG_ENDIAN_DIC)

#define WRITE_FLOAT_DIC             WriteFloatBigEndian
#define WRITE_CELL_DIC(addr,data)   WriteCellBigEndian((uint8_t *)(addr),(ucell_t)(data))
#define WRITE_SHORT_DIC(addr,data)  Write16BigEndian((uint8_t *)(addr),(uint16_t)(data))
#define READ_FLOAT_DIC              ReadFloatBigEndian
#define READ_CELL_DIC(addr)         ReadCellBigEndian((const uint8_t *)(addr))
#define READ_SHORT_DIC(addr)        Read16BigEndian((const uint8_t *)(addr))

#elif defined(PF_LITTLE_ENDIAN_DIC)

#define WRITE_FLOAT_DIC             WriteFloatLittleEndian
#define WRITE_CELL_DIC(addr,data)   WriteCellLittleEndian((uint8_t *)(addr),(ucell_t)(data))
#define WRITE_SHORT_DIC(addr,data)  Write16LittleEndian((uint8_t *)(addr),(uint16_t)(data))
#define READ_FLOAT_DIC              ReadFloatLittleEndian
#define READ_CELL_DIC(addr)         ReadCellLittleEndian((const uint8_t *)(addr))
#define READ_SHORT_DIC(addr)        Read16LittleEndian((const uint8_t *)(addr))

#else

#define WRITE_FLOAT_DIC(addr,data)  { *((PF_FLOAT *)(addr)) = (PF_FLOAT)(data); }
#define WRITE_CELL_DIC(addr,data)   { *((cell_t *)(addr)) = (cell_t)(data); }
#define WRITE_SHORT_DIC(addr,data)  { *((int16_t *)(addr)) = (int16_t)(data); }
#define READ_FLOAT_DIC(addr)        ( *((PF_FLOAT *)(addr)) )
#define READ_CELL_DIC(addr)         ( *((const ucell_t *)(addr)) )
#define READ_SHORT_DIC(addr)        ( *((const uint16_t *)(addr)) )

#endif


#define HEADER_HERE (gCurrentDictionary->dic_HeaderPtr.Cell)
#define CODE_HERE (gCurrentDictionary->dic_CodePtr.Cell)
#define CODE_COMMA( N ) WRITE_CELL_DIC(CODE_HERE++,(N))
#define NAME_BASE (gCurrentDictionary->dic_HeaderBase)
#define CODE_BASE (gCurrentDictionary->dic_CodeBase)
#define NAME_SIZE (gCurrentDictionary->dic_HeaderLimit - gCurrentDictionary->dic_HeaderBase)
#define CODE_SIZE (gCurrentDictionary->dic_CodeLimit - gCurrentDictionary->dic_CodeBase)

#define IN_CODE_DIC(addr) ( ( ((uint8_t *)(addr)) >= gCurrentDictionary->dic_CodeBase)   && ( ((uint8_t *)(addr)) < gCurrentDictionary->dic_CodeLimit) )

#define IN_NAME_DIC(addr) ( ( ((uint8_t *)(addr)) >= gCurrentDictionary->dic_HeaderBase) && ( ((uint8_t *)(addr)) < gCurrentDictionary->dic_HeaderLimit) )
#define IN_DICS(addr) (IN_CODE_DIC(addr) || IN_NAME_DIC(addr))

/* Address conversion */
#define ABS_TO_NAMEREL( a ) ((cell_t)  (((ucell_t) a) - NAME_BASE ))
#define ABS_TO_CODEREL( a ) ((cell_t)  (((ucell_t) a) - CODE_BASE ))
#define NAMEREL_TO_ABS( a ) ((ucell_t) (((cell_t) a) + NAME_BASE))
#define CODEREL_TO_ABS( a ) ((ucell_t) (((cell_t) a) + CODE_BASE))

/* The check for >0 is only needed for CLONE testing. !!! */
#define IsTokenPrimitive(xt) ((xt<gNumPrimitives) && (xt>=0))

#define FREE_VAR(v) { if (v) { pfFreeMem((void *)(v)); v = 0; } }

#define DATA_STACK_DEPTH (gCurrentTask->td_StackBase - gCurrentTask->td_StackPtr)
#define DROP_DATA_STACK (gCurrentTask->td_StackPtr++)
#define POP_DATA_STACK (*gCurrentTask->td_StackPtr++)
#define PUSH_DATA_STACK(x) {*(--(gCurrentTask->td_StackPtr)) = (cell_t) x; }

/* Force Quad alignment. */
#define QUADUP(x) (((x)+3)&~3)

#ifndef MIN
#define MIN(a,b)  ( ((a)<(b)) ? (a) : (b) )
#endif
#ifndef MAX
#define MAX(a,b)  ( ((a)>(b)) ? (a) : (b) )
#endif

#ifndef TOUCH
    #define TOUCH(argument) ((void)argument)
#endif

/***************************************************************
** I/O related macros
***************************************************************/

#define EMIT(c)  ioEmit(c)
#define EMIT_CR  EMIT('\n');

#define MSG(cs)   pfMessage(cs)
#define ERR(x)    MSG(x)

#define DBUG(x)  /* PRT(x) */
#define DBUGX(x) /* DBUG(x) */

#define MSG_NUM_D(msg,num) { MSG(msg); ffDot((cell_t) num); EMIT_CR; }
#define MSG_NUM_H(msg,num) { MSG(msg); ffDotHex((cell_t) num); EMIT_CR; }

#define DBUG_NUM_D(msg,num) { pfDebugMessage(msg); pfDebugPrintDecimalNumber((cell_t) num); pfDebugMessage("\n"); }

#endif  /* _pf_guts_h */
