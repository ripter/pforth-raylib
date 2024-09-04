#ifndef _raylib_pf_raylib_h
#define _raylib_pf_raylib_h

/* Define the XT values for the raylib words. */
#define RAYLIB_XT_VALUES \
    ID_INIT_WINDOW, \
    ID_SET_TARGET_FPS, \
    ID_WINDOW_SHOULD_CLOSE, \
    ID_BEGIN_DRAWING, \
    ID_CLEAR_BACKGROUND, \
    ID_DRAW_TEXT, \
    ID_END_DRAWING, \
    ID_CLOSE_WINDOW \



/* Macro to add all the XT tokens to the dictionary */
#define ADD_RAYLIB_WORDS_TO_DICTIONARY \
    pfDebugMessage("pfBuildDictionary: Adding raylib words\n"); \
    CreateDicEntryC( ID_INIT_WINDOW, "INIT-WINDOW", 0 ); \
    CreateDicEntryC( ID_SET_TARGET_FPS, "SET-TARGET-FPS", 0 ); \
    CreateDicEntryC( ID_WINDOW_SHOULD_CLOSE, "WINDOW-SHOULD-CLOSE", 0 ); \
    CreateDicEntryC( ID_BEGIN_DRAWING, "BEGIN-DRAWING", 0 ); \
    CreateDicEntryC( ID_CLEAR_BACKGROUND, "CLEAR-BACKGROUND", 0 ); \
    CreateDicEntryC( ID_DRAW_TEXT, "DRAW-TEXT", 0 ); \
    CreateDicEntryC( ID_END_DRAWING, "END-DRAWING", 0 ); \
    CreateDicEntryC( ID_CLOSE_WINDOW, "CLOSE-WINDOW", 0 ); \


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
            printf( "\nInitWindow( %d, %d, %d, %s )\n", width, height, len, (char *)gScratch ); \
        } else { \
            fprintf(stderr, "\nError: Invalid string or length. Please use s\" to create a simple string.\n"); \
            break; \
        } \
        break; \
    } \
    case ID_SET_TARGET_FPS: {  /* ( +n --  ) */ \
        SetTargetFPS(TOS); \
        M_DROP; \
        break; \
    } \
    case ID_WINDOW_SHOULD_CLOSE: {  /* ( -- +n ) */ \
        int result = WindowShouldClose(); \
        printf( "\nWindowShouldClose() = %d \n", result ); \
        M_PUSH(result); \
        break; \
    } \
    case ID_BEGIN_DRAWING: {  /* ( --  ) */ \
        BeginDrawing(); \
        break; \
    } \
    case ID_CLEAR_BACKGROUND: {  /* ( n n n n --  ) */ \
        int red = M_POP; \
        int green = M_POP; \
        int blue = M_POP; \
        int alpha = TOS; \
        M_DROP; \
        ClearBackground((Color){ red, blue, green, alpha }); \
        break; \
    } \
    case ID_DRAW_TEXT: {  /* ( +n +n c-addr u n n n n --  ) */ \
        int alpha = TOS; \
        int blue = M_POP; \
        int green = M_POP; \
        int red = M_POP; \
        int fontSize = M_POP; \
        int posY = M_POP; \
        int posX = M_POP; \
        int len = M_POP; \
        CharPtr = (char *) M_POP;  /* title string, not null terminated. */ \
        if (CharPtr != NULL && len > 0 && len < TIB_SIZE) { \
            M_DROP; \
            pfCopyMemory(gScratch, CharPtr, len); \
            gScratch[len] = '\0'; \
            DrawText(gScratch, posX, posY, fontSize, (Color){ red, green, blue, alpha }); \
        } \
        break; \
    } \
    case ID_END_DRAWING: {  /* ( --  ) */ \
        EndDrawing(); \
        break; \
    } \
    case ID_CLOSE_WINDOW: {  /* ( --  ) */ \
        CloseWindow(); \
        break; \
    } \

#endif  /* _raylib_pf_raylib_h */

            // void DrawText(const char *text, int posX, int posY, int fontSize, Color color); 
            // DrawText(gScratch, x, y, fontSize, BLACK); 
            // printf( "\nDrawText( %s, %d, %d, %d, (Color){ %d %d %d %d } )", (char *)gScratch, posX, posY, fontSize, red, green, blue, alpha ); 


