#include <stdio.h>
#include "pforth.h"
#include "pf_raylib.h"

ThrowCode pfRaylibCatch( ExecToken XT ) {
  printf("pfRaylibCatch: XT=%lu\n", XT);
  return 0;
}
