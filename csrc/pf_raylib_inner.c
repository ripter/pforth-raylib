#include <stdio.h>
#include "pf_clib.h"
#include "pf_guts.h"
#include "pf_raylib.h"
#include "pforth.h"
#include "raylib.h"


#define STKPTR     (DataStackPtr)
#define M_POP      (*(STKPTR++))
#define M_PUSH(n)  {*(--(STKPTR)) = (cell_t) (n);}
#define M_STACK(n) (STKPTR[n])

#define TOS      (*TopOfStack)
#define M_DROP   { TOS = M_POP; }


// Raylib Primitive implementation of the Raylib words.
ThrowCode pfRaylibCatch( ExecToken XT, cell_t *TopOfStack, cell_t *DataStackPtr, cell_t *ReturnStackPtr ) {
  char *CharPtr = NULL;


  switch (XT) {
    case ID_LOAD_IMAGE: {      /* ( c-addr u -- c-addr2 ior ) */
      cell_t len = TOS;        /* length of the filename string. */
      CharPtr = (char *)M_POP; /* filename string, not null terminated. */
      
      pfCopyMemory(gScratch, CharPtr, len);
      gScratch[len] = '\0';

      printf("len: %lu\n", len);
      printf("CharPtr: %s\n", gScratch);

      M_PUSH(42);
      M_PUSH(23);
      M_PUSH(16);
      M_PUSH(15);
      M_PUSH(8);
      M_PUSH(4);
      TOS = -19; 
      // *(DataStackPtr - 1) = 42;
      // *(DataStackPtr) = 23;
      // *(DataStackPtr + 1) = 16;

      // DataStackPtr[-1] = 42;
      // DataStackPtr[0] = 23;
      // DataStackPtr[1] = 16;
      // DataStackPtr += 2; // Move the DataStackPtr to the next cell.
      // *DataStackPtr = 42;
      // *(DataStackPtr + 1) = 23;
      // *(DataStackPtr + 2) = 16;

      // cell_t len = TOS;        /* length of the filename string. */
      // CharPtr = (char *)M_POP; /* filename string, not null terminated. */
      // if (CharPtr == NULL) {
      //   M_PUSH(0);
      //   TOS = -1; // null string pointer.
      //   break;
      // } else if (len == 0) {
      //   M_PUSH(0);
      //   TOS = -2; // empty string.
      //   break;
      // } else if (len > TIB_SIZE) {
      //   M_PUSH(0);
      //   TOS = -3; // string too long.
      //   break;
      // }

      // pfCopyMemory(gScratch, CharPtr, len);
      // gScratch[len] = '\0';
      // Image image = LoadImage(gScratch);

      // if (image.data == NULL) {
      //   M_PUSH(0);
      //   TOS = -4; // image failed to load.
      //   break;
      // }

      // M_PUSH(&image);
      // TOS = 0;
      break;
    }
    default:
      break;
  }
  return 0;
}
