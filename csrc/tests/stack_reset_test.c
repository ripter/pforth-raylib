#include <stdio.h>
#include <stdbool.h>
#include "../pforth.h"
#include "../pf_all.h"


#define DEFAULT_RETURN_DEPTH (512)
#define DEFAULT_USER_DEPTH (512)
#define PF_DEFAULT_DICTIONARY "pforth.dic"

ThrowCode DoStackTest();

int main( int argc, char **argv ) {
  // Create a pforth for testing.
  ThrowCode Result = DoStackTest();
}


ThrowCode DoStackTest() {
  pfTaskData_t *cftd;
  pfDictionary_t *dic = NULL;
  ThrowCode Result = 0;
  ExecToken EntryPoint = 0;

  // initalize the globals
  pfInit();

  // Create the pForth kernal.
  cftd = pfCreateTask(DEFAULT_USER_DEPTH, DEFAULT_RETURN_DEPTH);

  if (cftd) {
    pfSetCurrentTask(cftd);


    dic = pfLoadDictionary(PF_DEFAULT_DICTIONARY, &EntryPoint);
    if (dic == NULL)
      goto error2;

    if (EntryPoint != 0) {
      Result = pfCatch(EntryPoint);
    }

    /* Clean up after running Forth. */
    pfDeleteDictionary(dic);
    pfDeleteTask(cftd);
  }

  return Result ? Result : gVarByeCode;

error2:
  MSG("pfDoForth: Error occured.\n");
  pfDeleteTask(cftd);
  return -1;
}
