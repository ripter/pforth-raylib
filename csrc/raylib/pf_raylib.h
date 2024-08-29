#ifndef _raylib_pf_raylib_h
#define _raylib_pf_raylib_h

/* Define the XT values for the raylib words. */
#define RAYLIB_XT_VALUES \
    ID_INIT_WINDOW         



/* Macro to add all the XT tokens to the dictionary */
#define ADD_RAYLIB_WORDS_TO_DICTIONARY \
    CreateDicEntryC( ID_INIT_WINDOW, "INIT-WINDOW", 0 ); 

#endif  /* _raylib_pf_raylib_h */
