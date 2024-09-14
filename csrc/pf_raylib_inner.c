#include <stdio.h>
#include <stdbool.h>
#include "pf_clib.h"
#include "pf_guts.h"
#include "pf_raylib.h"
#include "pforth.h"
#include "raylib.h"


#define STKPTR     (*DataStackPtr)
#define M_POP      (*(STKPTR++))
#define M_PUSH(n)  {*(--(STKPTR)) = (cell_t) (n);}
#define M_STACK(n) (STKPTR[n])

#define TOS      (*TopOfStack)
#define M_DROP   { TOS = M_POP; }


// Raylib Primitive implementation of the Raylib words.
bool pfRaylibCatch( ExecToken XT, cell_t *TopOfStack, cell_t **DataStackPtr, cell_t *ReturnStackPtr ) {
  char *CharPtr = NULL;


  switch (XT) {
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
      return false; // Didn't find the word.
  }
  return true; // Found the word!
}
