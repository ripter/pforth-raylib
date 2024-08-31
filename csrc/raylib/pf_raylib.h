#ifndef _raylib_pf_raylib_h
#define _raylib_pf_raylib_h

/* Define the XT values for the raylib words. */
#define RAYLIB_XT_VALUES \
    ID_INIT_WINDOW         



/* Macro to add all the XT tokens to the dictionary */
#define ADD_RAYLIB_WORDS_TO_DICTIONARY \
    pfDebugMessage("pfBuildDictionary: Adding raylib words\n"); \
    CreateDicEntryC( ID_INIT_WINDOW, "INIT-WINDOW", 0 ); 


/* Define the raylib words */
#define RAYLIB_WORDS \
    case ID_INIT_WINDOW: {  /* ( +n +n c-addr u --  ) */ \
        cell_t len = TOS;  /* length of the title string. */ \
        CharPtr = (char *) M_POP;  /* title string, not null terminated. */ \
        cell_t height = M_POP; \
        cell_t width = M_POP; \
        if (Temp != NULL && len > 0 && len < TIB_SIZE) { \
            M_DROP; \
            pfCopyMemory(gScratch, CharPtr, len); \
            gScratch[len] = '\0'; \
            InitWindow(width, height, gScratch); \
            printf( "\nInitWindow( %d, %d, %d, %s )", width, height, len, (char *)gScratch ); \
        } else { \
            fprintf(stderr, "Error: Invalid string or length. Please use s\" to create a simple string.\n"); \
            break; \
        } \
        break; \
    }

#endif  /* _raylib_pf_raylib_h */

//         cell_t width = M_POP; 
//         InitWindow( 800, 600, "raylib-pforth" ); 
//         // char* title = (char*) TOS; 

//         pfCopyMemory( gScratch, (char *) Temp, (ucell_t) TOS ); 
//         gScratch[TOS] = '\0'; 

        // if (len >= TIB_SIZE) { 
        //     printf("String length exceeds buffer size\n"); 
        //     break; 
        // } \
        // pfCopyMemory( gScratch, (char *) title, len ); 
        // gScratch[len] = '\0'; 
