#ifndef _raylib_pf_raylib_h
#define _raylib_pf_raylib_h
#include <stdbool.h>

bool TryRaylibWord( ExecToken XT, cell_t *TopOfStack, cell_t **DataStackPtr, cell_t **ReturnStackPtr );

/* Define the XT values for the raylib words. */
#define RAYLIB_XT_VALUES \
    ID_TEST_WORD, \
    /* rcore - Window-related functions */ \
    ID_INIT_WINDOW, \
    ID_CLOSE_WINDOW, \
    ID_WINDOW_SHOULD_CLOSE, \
    ID_IS_WINDOW_READY, \
    ID_IS_WINDOW_FULLSCREEN, \
    ID_IS_WINDOW_HIDDEN, \
    ID_IS_WINDOW_MINIMIZED, \
    ID_IS_WINDOW_MAXIMIZED, \
    ID_IS_WINDOW_FOCUSED, \
    ID_IS_WINDOW_RESIZED, \
    ID_IS_WINDOW_STATE, \
    ID_SET_WINDOW_STATE, \
    ID_CLEAR_WINDOW_STATE, \
    ID_TOGGLE_FULLSCREEN, \
    ID_TOGGLE_BORDERLESS_WINDOW, \
    ID_MAXIMIZE_WINDOW, \
    ID_MINIMIZE_WINDOW, \
    ID_RESTORE_WINDOW, \
    ID_SET_WINDOW_ICON, \
    \
    /* rtextures  */ \
    ID_LOAD_IMAGE, \
    ID_LOAD_TEXTURE_FROM_IMAGE, \
    \
    ID_SET_TARGET_FPS, \
    ID_BEGIN_DRAWING, \
    ID_CLEAR_BACKGROUND, \
    ID_DRAW_TEXT, \
    ID_END_DRAWING \



/* Macro to add all the XT tokens to the dictionary */
#define ADD_RAYLIB_WORDS_TO_DICTIONARY \
    CreateDicEntryC( ID_TEST_WORD, "TEST-WORD", 0 ); \
    pfDebugMessage("pfBuildDictionary: Adding raylib words\n"); \
    CreateDicEntryC( ID_INIT_WINDOW, "INIT-WINDOW", 0 ); \
    CreateDicEntryC( ID_CLOSE_WINDOW, "CLOSE-WINDOW", 0 ); \
    CreateDicEntryC( ID_WINDOW_SHOULD_CLOSE, "WINDOW-SHOULD-CLOSE", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_READY, "IS-WINDOW-READY", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_FULLSCREEN, "IS-WINDOW-FULLSCREEN", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_HIDDEN, "IS-WINDOW-HIDDEN", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_MINIMIZED, "IS-WINDOW-MINIMIZED", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_MAXIMIZED, "IS-WINDOW-MAXIMIZED", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_FOCUSED, "IS-WINDOW-FOCUSED", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_RESIZED, "IS-WINDOW-RESIZED", 0 ); \
    CreateDicEntryC( ID_IS_WINDOW_STATE, "IS-WINDOW-STATE", 0 ); \
    CreateDicEntryC( ID_SET_WINDOW_STATE, "SET-WINDOW-STATE", 0 ); \
    CreateDicEntryC( ID_CLEAR_WINDOW_STATE, "CLEAR-WINDOW-STATE", 0 ); \
    CreateDicEntryC( ID_TOGGLE_FULLSCREEN, "TOGGLE-FULLSCREEN", 0 ); \
    CreateDicEntryC( ID_TOGGLE_BORDERLESS_WINDOW, "TOGGLE-BORDERLESS-WINDOW", 0 ); \
    CreateDicEntryC( ID_MAXIMIZE_WINDOW, "MAXIMIZE-WINDOW", 0 ); \
    CreateDicEntryC( ID_MINIMIZE_WINDOW, "MINIMIZE-WINDOW", 0 ); \
    CreateDicEntryC( ID_RESTORE_WINDOW, "RESTORE-WINDOW", 0 ); \
    CreateDicEntryC( ID_SET_WINDOW_ICON, "SET-WINDOW-ICON", 0 ); \
    \
    CreateDicEntryC( ID_LOAD_IMAGE, "LOAD-IMAGE", 0 ); \
    CreateDicEntryC( ID_LOAD_TEXTURE_FROM_IMAGE, "LOAD-TEXTURE-FROM-IMAGE", 0 ); \
    \
    CreateDicEntryC( ID_SET_TARGET_FPS, "SET-TARGET-FPS", 0 ); \
    CreateDicEntryC( ID_BEGIN_DRAWING, "BEGIN-DRAWING", 0 ); \
    CreateDicEntryC( ID_CLEAR_BACKGROUND, "CLEAR-BACKGROUND", 0 ); \
    CreateDicEntryC( ID_DRAW_TEXT, "DRAW-TEXT", 0 ); \
    CreateDicEntryC( ID_END_DRAWING, "END-DRAWING", 0 ); \

#endif  /* _raylib_pf_raylib_h */
