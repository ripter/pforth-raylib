#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"

#include "pforth.h"
#include "pf_clib.h"
#include "pf_guts.h"
#include "pf_raylib.h"

#define STKPTR     (*DataStackPtr)
#define M_POP      (*(STKPTR++))
#define M_PUSH(n)  *(--(STKPTR)) = (cell_t) (n)
#define M_STACK(n) (STKPTR[n])

#define TOS      (*TopOfStack)
#define M_DROP   TOS = M_POP

/**
 * Updates the stacks and pointers to translate execution of Raylib words.
 */
bool TryRaylibWord(ExecToken XT, cell_t *TopOfStack, cell_t **DataStackPtr,
                   cell_t **ReturnStackPtr) {
    char *CharPtr = NULL;

    switch (XT) {
        // rcore - Window-related functions
        case ID_INIT_WINDOW: {     /* ( +n +n c-addr u --  ) */
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
                fprintf(stderr, "\nError: Invalid string or length. Use s\" to create a valid string.\n");
            }
        } break;

        case ID_TEST_WORD:
            printf("\nINFO: Test word\n");
            M_PUSH(42);
            M_PUSH(23);
            M_PUSH(16);
            M_PUSH(15);
            M_PUSH(8);
            M_PUSH(4);
            TOS = -19;
            break;

        default:
            return false; // Didn't find the word.
    }
    return true; // Found the word!
}
