#ifndef _raylib_pf_raylib_h
#define _raylib_pf_raylib_h

ThrowCode pfRaylibCatch( ExecToken XT, cell_t *TopOfStack, cell_t *DataStackPtr, cell_t *ReturnStackPtr );

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
    \
    CreateDicEntryC( ID_SET_TARGET_FPS, "SET-TARGET-FPS", 0 ); \
    CreateDicEntryC( ID_BEGIN_DRAWING, "BEGIN-DRAWING", 0 ); \
    CreateDicEntryC( ID_CLEAR_BACKGROUND, "CLEAR-BACKGROUND", 0 ); \
    CreateDicEntryC( ID_DRAW_TEXT, "DRAW-TEXT", 0 ); \
    CreateDicEntryC( ID_END_DRAWING, "END-DRAWING", 0 ); \


/* Define the raylib words */
#define RAYLIB_WORDS \
    case ID_INIT_WINDOW: {  /* ( +n +n c-addr u --  ) */ \
        cell_t len = TOS;  /* length of the title string. */ \
        CharPtr = (char *) M_POP;  /* title string, not null terminated. */ \
        cell_t height = M_POP; \
        cell_t width = M_POP; \
        if (CharPtr != NULL && len > 0 && len < TIB_SIZE) { \
            M_DROP; \
            pfCopyMemory(gScratch, CharPtr, len); \
            gScratch[len] = '\0'; \
            InitWindow(width, height, gScratch); \
        } else { \
            fprintf(stderr, "\nError: Invalid string or length. Please use s\" to create a simple string.\n"); \
            break; \
        } \
    } break; \
    case ID_CLOSE_WINDOW: {  /* ( --  ) */ \
        CloseWindow(); \
    } break; \
    case ID_WINDOW_SHOULD_CLOSE: {  /* ( -- +n ) */ \
        TOS = WindowShouldClose(); \
    } break; \
    case ID_IS_WINDOW_READY: {  /* ( -- +n ) */ \
        int result = IsWindowReady(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_FULLSCREEN: {  /* ( -- +n ) */ \
        int result = IsWindowFullscreen(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_HIDDEN: {  /* ( -- +n ) */ \
        int result = IsWindowHidden(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_MINIMIZED: {  /* ( -- +n ) */ \
        int result = IsWindowMinimized(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_MAXIMIZED: {  /* ( -- +n ) */ \
        int result = IsWindowMaximized(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_FOCUSED: {  /* ( -- +n ) */ \
        int result = IsWindowFocused(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_RESIZED: {  /* ( -- +n ) */ \
        int result = IsWindowResized(); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_IS_WINDOW_STATE: {  /* ( -- +n ) */ \
        int result = IsWindowState(TOS); \
        TOS = result == pfFALSE ? pfFALSE : pfTRUE; \
    } break; \
    case ID_SET_WINDOW_STATE: {  /* ( +n --  ) */ \
        int flags = TOS; \
        M_DROP; \
        SetWindowState(flags); \
    } break; \
    case ID_CLEAR_WINDOW_STATE: {  /* ( +n --  ) */ \
        int flags = TOS; \
        M_DROP; \
        ClearWindowState(flags); \
    } break; \
    case ID_TOGGLE_FULLSCREEN: {  /* ( --  ) */ \
        ToggleFullscreen(); \
    } break; \
    case ID_TOGGLE_BORDERLESS_WINDOW: {  /* ( --  ) */ \
        ToggleBorderlessWindowed(); \
    } break; \
    case ID_MAXIMIZE_WINDOW: {  /* ( --  ) */ \
        MaximizeWindow(); \
    } break; \
    case ID_MINIMIZE_WINDOW: {  /* ( --  ) */ \
        MinimizeWindow(); \
    } break; \
    case ID_RESTORE_WINDOW: {  /* ( --  ) */ \
        RestoreWindow(); \
    } break; \
    case ID_SET_WINDOW_ICON: {  /* ( +n +n c-addr u --  ) */ \
        printf("TODO: Set the window Icon\n"); \
    } break; \
    \
    \
    \
    \
    \
    /** \ 
     * rtextures \ 
     */ \
    \
    case ID_LOAD_IMAGE: {      /* ( c-addr u -- c-addr2 ior ) */ \
        cell_t len = TOS;        /* length of the filename string. */ \
        CharPtr = (char *)M_POP; /* filename string, not null terminated. */ \ 
        if (CharPtr == NULL) { \
            M_PUSH(0); \
            TOS = -1; /* null string pointer. */ \
            break; \
        } else if (len == 0) { \
            M_PUSH(0); \
            TOS = -2; /* empty string. */ \
            break; \
        } else if (len > TIB_SIZE) { \
            M_PUSH(0); \
            TOS = -3; /* string too long. */ \
            break; \
        } \
        \
        pfCopyMemory(gScratch, CharPtr, len); \
        gScratch[len] = '\0'; \
        Image image = LoadImage(gScratch); \
        \
        if (image.data == NULL) { \
            M_PUSH(0); \
            TOS = -4; /* image failed to load. */ \
            break; \
        } \
        \
        M_PUSH(&image); \
        TOS = 0; \
    } break; \
    case ID_SET_TARGET_FPS: {  /* ( +n --  ) */ \
        SetTargetFPS(TOS); \
        M_DROP; \
    } break; \
    case ID_BEGIN_DRAWING: {  /* ( --  ) */ \
        BeginDrawing(); \
    } break; \
    case ID_CLEAR_BACKGROUND: {  /* ( n n n n --  ) */ \
        int alpha = TOS; \
        int blue = M_POP; \
        int green = M_POP; \
        int red = M_POP; \
        M_DROP; \
        ClearBackground((Color){ red, green, blue, alpha }); \
    } break; \
    case ID_DRAW_TEXT: {  /* ( +n +n c-addr u n n n n --  ) */ \
        int alpha = TOS; \
        int blue = M_POP; \
        int green = M_POP; \
        int red = M_POP; \
        int fontSize = M_POP; \
        int posY = M_POP; \
        int posX = M_POP; \
        int len = M_POP; \
        CharPtr = (char *) M_POP;  /* not null terminated. */ \
        if (CharPtr != NULL && len > 0 && len < TIB_SIZE) { \
            M_DROP; \
            pfCopyMemory(gScratch, CharPtr, len); \
            gScratch[len] = '\0'; \
            DrawText(gScratch, posX, posY, fontSize, (Color){ red, green, blue, alpha }); \
        } \
    } break; \
    case ID_END_DRAWING: {  /* ( --  ) */ \
        EndDrawing(); \
    } break; \

#endif  /* _raylib_pf_raylib_h */
