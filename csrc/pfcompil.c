/* @(#) pfcompil.c 98/01/26 1.5 */
/***************************************************************
** Compiler for PForth based on 'C'
**
** These routines could be left out of an execute only version.
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, David Rosenboom
**
** Permission to use, copy, modify, and/or distribute this
** software for any purpose with or without fee is hereby granted.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
** THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
** CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
** FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
** CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
** OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**
****************************************************************
** 941004 PLB Extracted IO calls from pforth_main.c
** 950320 RDG Added underflow checking for FP stack
***************************************************************/

#include "pf_all.h"
#include "pfcompil.h"

#define ABORT_RETURN_CODE   (10)
#define UINT32_MASK  ((sizeof(ucell_t)-1))

/***************************************************************/
/************** Static Prototypes ******************************/
/***************************************************************/

static void  ffStringColon( const ForthStringPtr FName );
static cell_t CheckRedefinition( const ForthStringPtr FName );
static void  ffUnSmudge( void );
static cell_t FindAndCompile( const char *theWord );
static cell_t ffCheckDicRoom( void );

#ifndef PF_NO_INIT
    static void CreateDeferredC( ExecToken DefaultXT, const char *CName );
#endif

cell_t NotCompiled( const char *FunctionName )
{
    MSG("Function ");
    MSG(FunctionName);
    MSG(" not compiled in this version of PForth.\n");
    return -1;
}

#ifndef PF_NO_SHELL
/***************************************************************
** Create an entry in the Dictionary for the given ExecutionToken.
** FName is name in Forth format.
*/
void CreateDicEntry( ExecToken XT, const ForthStringPtr FName, ucell_t Flags )
{
    cfNameLinks *cfnl;

    cfnl = (cfNameLinks *) gCurrentDictionary->dic_HeaderPtr;

/* Set link to previous header, if any. */
    if( gVarContext )
    {
        WRITE_CELL_DIC( &cfnl->cfnl_PreviousName, ABS_TO_NAMEREL( gVarContext ) );
    }
    else
    {
        cfnl->cfnl_PreviousName = 0;
    }

/* Put Execution token in header. */
    WRITE_CELL_DIC( &cfnl->cfnl_ExecToken, XT );

/* Advance Header Dictionary Pointer */
    gCurrentDictionary->dic_HeaderPtr += sizeof(cfNameLinks);

/* Laydown name. */
    gVarContext = gCurrentDictionary->dic_HeaderPtr;
    pfCopyMemory( (uint8_t *) gCurrentDictionary->dic_HeaderPtr, FName, (*FName)+1 );
    gCurrentDictionary->dic_HeaderPtr += (*FName)+1;

/* Set flags. */
    *(char*)gVarContext |= (char) Flags;

/* Align to quad byte boundaries with zeroes. */
    while( gCurrentDictionary->dic_HeaderPtr & UINT32_MASK )
    {
        *(char*)(gCurrentDictionary->dic_HeaderPtr++) = 0;
    }
}

/***************************************************************
** Convert name then create dictionary entry.
*/
void CreateDicEntryC( ExecToken XT, const char *CName, ucell_t Flags )
{
    ForthString FName[40];
    CStringToForth( FName, CName, sizeof(FName) );
    CreateDicEntry( XT, FName, Flags );
}

/***************************************************************
** Convert absolute namefield address to previous absolute name
** field address or NULL.
*/
const ForthString *NameToPrevious( const ForthString *NFA )
{
    cell_t RelNamePtr;
    const cfNameLinks *cfnl;

/* DBUG(("\nNameToPrevious: NFA = 0x%x\n", (cell_t) NFA)); */
    cfnl = (const cfNameLinks *) ( ((const char *) NFA) - sizeof(cfNameLinks) );

    RelNamePtr = READ_CELL_DIC((const cell_t *) (&cfnl->cfnl_PreviousName));
/* DBUG(("\nNameToPrevious: RelNamePtr = 0x%x\n", (cell_t) RelNamePtr )); */
    if( RelNamePtr )
    {
        return ( (ForthString *) NAMEREL_TO_ABS( RelNamePtr ) );
    }
    else
    {
        return NULL;
    }
}
/***************************************************************
** Convert NFA to ExecToken.
*/
ExecToken NameToToken( const ForthString *NFA )
{
    const cfNameLinks *cfnl;

/* Convert absolute namefield address to absolute link field address. */
    cfnl = (const cfNameLinks *) ( ((const char *) NFA) - sizeof(cfNameLinks) );

    return READ_CELL_DIC((const cell_t *) (&cfnl->cfnl_ExecToken));
}

/***************************************************************
** Find XTs needed by compiler.
*/
cell_t FindSpecialXTs( void )
{
    if( ffFindC( "(QUIT)", &gQuitP_XT ) == 0) goto nofind;
    if( ffFindC( "NUMBER?", &gNumberQ_XT ) == 0) goto nofind;
    if( ffFindC( "ACCEPT", &gAcceptP_XT ) == 0) goto nofind;
DBUG(("gNumberQ_XT = 0x%x\n", (unsigned int)gNumberQ_XT ));
    return 0;

nofind:
    ERR("FindSpecialXTs failed!\n");
    return -1;
}

/***************************************************************
** Build a dictionary from scratch.
*/
#ifndef PF_NO_INIT
PForthDictionary pfBuildDictionary( cell_t HeaderSize, cell_t CodeSize )
{
    pfDictionary_t *dic;

    dic = pfCreateDictionary( HeaderSize, CodeSize );
    if( !dic ) goto nomem;

    pfDebugMessage("pfBuildDictionary: Start adding dictionary entries.\n");

    gCurrentDictionary = dic;
    gNumPrimitives = NUM_PRIMITIVES;

    CreateDicEntryC( ID_EXIT, "EXIT", 0 );
    pfDebugMessage("pfBuildDictionary: added EXIT\n");
    CreateDicEntryC( ID_1MINUS, "1-", 0 );
    pfDebugMessage("pfBuildDictionary: added 1-\n");
    CreateDicEntryC( ID_1PLUS, "1+", 0 );
    CreateDicEntryC( ID_2_R_FETCH, "2R@", 0 );
    CreateDicEntryC( ID_2_R_FROM, "2R>", 0 );
    CreateDicEntryC( ID_2_TO_R, "2>R", 0 );
    CreateDicEntryC( ID_2DUP, "2DUP", 0 );
    CreateDicEntryC( ID_2LITERAL, "2LITERAL", FLAG_IMMEDIATE );
    CreateDicEntryC( ID_2LITERAL_P, "(2LITERAL)", 0 );
    CreateDicEntryC( ID_2MINUS, "2-", 0 );
    CreateDicEntryC( ID_2PLUS, "2+", 0 );
    CreateDicEntryC( ID_2OVER, "2OVER", 0 );
    CreateDicEntryC( ID_2SWAP, "2SWAP", 0 );
    CreateDicEntryC( ID_ACCEPT_P, "(ACCEPT)", 0 );
    CreateDeferredC( ID_ACCEPT_P, "ACCEPT" );
    CreateDicEntryC( ID_ALITERAL, "ALITERAL", FLAG_IMMEDIATE );
    CreateDicEntryC( ID_ALITERAL_P, "(ALITERAL)", 0 );
    CreateDicEntryC( ID_ALLOCATE, "ALLOCATE", 0 );
    pfDebugMessage("pfBuildDictionary: added ALLOCATE\n");
    CreateDicEntryC( ID_ARSHIFT, "ARSHIFT", 0 );
    CreateDicEntryC( ID_AND, "AND", 0 );
    CreateDicEntryC( ID_BAIL, "BAIL", 0 );
    CreateDicEntryC( ID_BRANCH, "BRANCH", 0 );
    CreateDicEntryC( ID_BODY_OFFSET, "BODY_OFFSET", 0 );
    CreateDicEntryC( ID_BYE, "BYE", 0 );
    CreateDicEntryC( ID_CATCH, "CATCH", 0 );
    CreateDicEntryC( ID_CELL, "CELL", 0 );
    CreateDicEntryC( ID_CELLS, "CELLS", 0 );
    CreateDicEntryC( ID_CFETCH, "C@", 0 );
    CreateDicEntryC( ID_CMOVE, "CMOVE", 0 );
    CreateDicEntryC( ID_CMOVE_UP, "CMOVE>", 0 );
    CreateDicEntryC( ID_COLON, ":", 0 );
    CreateDicEntryC( ID_COLON_P, "(:)", 0 );
    CreateDicEntryC( ID_COMPARE, "COMPARE", 0 );
    CreateDicEntryC( ID_COMP_EQUAL, "=", 0 );
    CreateDicEntryC( ID_COMP_NOT_EQUAL, "<>", 0 );
    CreateDicEntryC( ID_COMP_GREATERTHAN, ">", 0 );
    CreateDicEntryC( ID_COMP_U_GREATERTHAN, "U>", 0 );
    pfDebugMessage("pfBuildDictionary: added U>\n");
    CreateDicEntryC( ID_COMP_LESSTHAN, "<", 0 );
    CreateDicEntryC( ID_COMP_U_LESSTHAN, "U<", 0 );
    CreateDicEntryC( ID_COMP_ZERO_EQUAL, "0=", 0 );
    CreateDicEntryC( ID_COMP_ZERO_NOT_EQUAL, "0<>", 0 );
    CreateDicEntryC( ID_COMP_ZERO_GREATERTHAN, "0>", 0 );
    CreateDicEntryC( ID_COMP_ZERO_LESSTHAN, "0<", 0 );
    CreateDicEntryC( ID_CR, "CR", 0 );
    CreateDicEntryC( ID_CREATE, "CREATE", 0 );
    CreateDicEntryC( ID_CREATE_P, "(CREATE)", 0 );
    CreateDicEntryC( ID_D_PLUS, "D+", 0 );
    CreateDicEntryC( ID_D_MINUS, "D-", 0 );
    CreateDicEntryC( ID_D_UMSMOD, "UM/MOD", 0 );
    CreateDicEntryC( ID_D_MUSMOD, "MU/MOD", 0 );
    CreateDicEntryC( ID_D_MTIMES, "M*", 0 );
    pfDebugMessage("pfBuildDictionary: added M*\n");
    CreateDicEntryC( ID_D_UMTIMES, "UM*", 0 );
    CreateDicEntryC( ID_DEFER, "DEFER", 0 );
    CreateDicEntryC( ID_CSTORE, "C!", 0 );
    CreateDicEntryC( ID_DEPTH, "DEPTH",  0 );
    pfDebugMessage("pfBuildDictionary: added DEPTH\n");
    CreateDicEntryC( ID_DIVIDE, "/", 0 );
    CreateDicEntryC( ID_DOT, ".",  0 );
    CreateDicEntryC( ID_DOTS, ".S",  0 );
    pfDebugMessage("pfBuildDictionary: added .S\n");
    CreateDicEntryC( ID_DO_P, "(DO)", 0 );
    CreateDicEntryC( ID_DROP, "DROP", 0 );
    CreateDicEntryC( ID_DUMP, "DUMP", 0 );
    CreateDicEntryC( ID_DUP, "DUP",  0 );
    CreateDicEntryC( ID_EMIT_P, "(EMIT)",  0 );
    pfDebugMessage("pfBuildDictionary: added (EMIT)\n");
    CreateDeferredC( ID_EMIT_P, "EMIT");
    pfDebugMessage("pfBuildDictionary: added EMIT\n");
    CreateDicEntryC( ID_EOL, "EOL",  0 );
    CreateDicEntryC( ID_ERRORQ_P, "(?ERROR)",  0 );
    CreateDicEntryC( ID_ERRORQ_P, "?ERROR",  0 );
    CreateDicEntryC( ID_EXECUTE, "EXECUTE",  0 );
    CreateDicEntryC( ID_FETCH, "@",  0 );
    CreateDicEntryC( ID_FILL, "FILL", 0 );
    CreateDicEntryC( ID_FIND, "FIND",  0 );
    CreateDicEntryC( ID_FILE_CREATE, "CREATE-FILE",  0 );
    CreateDicEntryC( ID_FILE_DELETE, "DELETE-FILE",  0 );
    CreateDicEntryC( ID_FILE_OPEN, "OPEN-FILE",  0 );
    CreateDicEntryC( ID_FILE_CLOSE, "CLOSE-FILE",  0 );
    CreateDicEntryC( ID_FILE_READ, "READ-FILE",  0 );
    CreateDicEntryC( ID_FILE_SIZE, "FILE-SIZE",  0 );
    CreateDicEntryC( ID_FILE_WRITE, "WRITE-FILE",  0 );
    CreateDicEntryC( ID_FILE_POSITION, "FILE-POSITION",  0 );
    CreateDicEntryC( ID_FILE_REPOSITION, "REPOSITION-FILE",  0 );
    CreateDicEntryC( ID_FILE_FLUSH, "FLUSH-FILE",  0 );
    CreateDicEntryC( ID_FILE_RENAME, "(RENAME-FILE)",  0 );
    CreateDicEntryC( ID_FILE_RESIZE, "(RESIZE-FILE)",  0 );
    CreateDicEntryC( ID_FILE_RO, "R/O",  0 );
    CreateDicEntryC( ID_FILE_RW, "R/W",  0 );
    CreateDicEntryC( ID_FILE_WO, "W/O",  0 );
    CreateDicEntryC( ID_FILE_BIN, "BIN",  0 );
    CreateDicEntryC( ID_FINDNFA, "FINDNFA",  0 );
    CreateDicEntryC( ID_FLUSHEMIT, "FLUSHEMIT",  0 );
    CreateDicEntryC( ID_FREE, "FREE",  0 );
#include "pfcompfp.h"
    CreateDicEntryC( ID_HERE, "HERE",  0 );
    CreateDicEntryC( ID_NUMBERQ_P, "(SNUMBER?)",  0 );
    CreateDicEntryC( ID_I, "I",  0 );
    CreateDicEntryC( ID_INTERPRET, "INTERPRET", 0 );
    CreateDicEntryC( ID_J, "J",  0 );
    CreateDicEntryC( ID_INCLUDE_FILE, "INCLUDE-FILE",  0 );
    CreateDicEntryC( ID_KEY, "KEY",  0 );
    CreateDicEntryC( ID_LEAVE_P, "(LEAVE)", 0 );
    CreateDicEntryC( ID_LITERAL, "LITERAL", FLAG_IMMEDIATE );
    CreateDicEntryC( ID_LITERAL_P, "(LITERAL)", 0 );
    CreateDicEntryC( ID_LOADSYS, "LOADSYS", 0 );
    CreateDicEntryC( ID_LOCAL_COMPILER, "LOCAL-COMPILER", 0 );
    CreateDicEntryC( ID_LOCAL_ENTRY, "(LOCAL.ENTRY)", 0 );
    CreateDicEntryC( ID_LOCAL_EXIT, "(LOCAL.EXIT)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH, "(LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_1, "(1_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_2, "(2_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_3, "(3_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_4, "(4_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_5, "(5_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_6, "(6_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_7, "(7_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_FETCH_8, "(8_LOCAL@)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE, "(LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_1, "(1_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_2, "(2_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_3, "(3_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_4, "(4_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_5, "(5_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_6, "(6_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_7, "(7_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_STORE_8, "(8_LOCAL!)", 0 );
    CreateDicEntryC( ID_LOCAL_PLUSSTORE, "(LOCAL+!)", 0 );
    CreateDicEntryC( ID_LOOP_P, "(LOOP)", 0 );
    CreateDicEntryC( ID_LSHIFT, "LSHIFT", 0 );
    CreateDicEntryC( ID_MAX, "MAX", 0 );
    CreateDicEntryC( ID_MIN, "MIN", 0 );
    CreateDicEntryC( ID_MINUS, "-", 0 );
    CreateDicEntryC( ID_NAME_TO_TOKEN, "NAME>", 0 );
    CreateDicEntryC( ID_NAME_TO_PREVIOUS, "PREVNAME", 0 );
    CreateDicEntryC( ID_NOOP, "NOOP", 0 );
    CreateDeferredC( ID_NUMBERQ_P, "NUMBER?" );
    CreateDicEntryC( ID_OR, "OR", 0 );
    CreateDicEntryC( ID_OVER, "OVER", 0 );
    pfDebugMessage("pfBuildDictionary: added OVER\n");
    CreateDicEntryC( ID_PICK, "PICK",  0 );
    CreateDicEntryC( ID_PLUS, "+",  0 );
    CreateDicEntryC( ID_PLUSLOOP_P, "(+LOOP)", 0 );
    CreateDicEntryC( ID_PLUS_STORE, "+!",  0 );
    CreateDicEntryC( ID_QUIT_P, "(QUIT)",  0 );
    CreateDeferredC( ID_QUIT_P, "QUIT" );
    CreateDicEntryC( ID_QDO_P, "(?DO)", 0 );
    CreateDicEntryC( ID_QDUP, "?DUP",  0 );
    CreateDicEntryC( ID_QTERMINAL, "?TERMINAL",  0 );
    CreateDicEntryC( ID_QTERMINAL, "KEY?",  0 );
    CreateDicEntryC( ID_REFILL, "REFILL",  0 );
    CreateDicEntryC( ID_RESIZE, "RESIZE",  0 );
    CreateDicEntryC( ID_ROLL, "ROLL",  0 );
    CreateDicEntryC( ID_ROT, "ROT",  0 );
    CreateDicEntryC( ID_RSHIFT, "RSHIFT",  0 );
    CreateDicEntryC( ID_R_DROP, "RDROP",  0 );
    CreateDicEntryC( ID_R_FETCH, "R@",  0 );
    CreateDicEntryC( ID_R_FROM, "R>",  0 );
    CreateDicEntryC( ID_RP_FETCH, "RP@",  0 );
    CreateDicEntryC( ID_RP_STORE, "RP!",  0 );
    CreateDicEntryC( ID_SEMICOLON, ";",  FLAG_IMMEDIATE );
    CreateDicEntryC( ID_SP_FETCH, "SP@",  0 );
    CreateDicEntryC( ID_SP_STORE, "SP!",  0 );
    CreateDicEntryC( ID_STORE, "!",  0 );
    CreateDicEntryC( ID_SAVE_FORTH_P, "(SAVE-FORTH)",  0 );
    CreateDicEntryC( ID_SCAN, "SCAN",  0 );
    CreateDicEntryC( ID_SKIP, "SKIP",  0 );
    CreateDicEntryC( ID_SLEEP_P, "(SLEEP)", 0 );
    CreateDicEntryC( ID_SOURCE, "SOURCE",  0 );
    CreateDicEntryC( ID_SOURCE_SET, "SET-SOURCE",  0 );
    CreateDicEntryC( ID_SOURCE_ID, "SOURCE-ID",  0 );
    CreateDicEntryC( ID_SOURCE_ID_PUSH, "PUSH-SOURCE-ID",  0 );
    CreateDicEntryC( ID_SOURCE_ID_POP, "POP-SOURCE-ID",  0 );
    CreateDicEntryC( ID_SOURCE_LINE_NUMBER_FETCH, "SOURCE-LINE-NUMBER@",  0 );
    CreateDicEntryC( ID_SOURCE_LINE_NUMBER_STORE, "SOURCE-LINE-NUMBER!",  0 );
    CreateDicEntryC( ID_SWAP, "SWAP",  0 );
    CreateDicEntryC( ID_TEST1, "TEST1",  0 );
    CreateDicEntryC( ID_TEST2, "TEST2",  0 );
    CreateDicEntryC( ID_TICK, "'", 0 );
    CreateDicEntryC( ID_TIMES, "*", 0 );
    CreateDicEntryC( ID_THROW, "THROW", 0 );
    CreateDicEntryC( ID_TO_R, ">R", 0 );
    CreateDicEntryC( ID_TYPE, "TYPE", 0 );
    CreateDicEntryC( ID_VAR_BASE, "BASE", 0 );
    CreateDicEntryC( ID_VAR_BYE_CODE, "BYE-CODE", 0 );
    CreateDicEntryC( ID_VAR_CODE_BASE, "CODE-BASE", 0 );
    CreateDicEntryC( ID_VAR_CODE_LIMIT, "CODE-LIMIT", 0 );
    CreateDicEntryC( ID_VAR_CONTEXT, "CONTEXT", 0 );
    CreateDicEntryC( ID_VAR_DP, "DP", 0 );
    CreateDicEntryC( ID_VAR_ECHO, "ECHO", 0 );
    CreateDicEntryC( ID_VAR_HEADERS_PTR, "HEADERS-PTR", 0 );
    CreateDicEntryC( ID_VAR_HEADERS_BASE, "HEADERS-BASE", 0 );
    CreateDicEntryC( ID_VAR_HEADERS_LIMIT, "HEADERS-LIMIT", 0 );
    CreateDicEntryC( ID_VAR_NUM_TIB, "#TIB", 0 );
    CreateDicEntryC( ID_VAR_RETURN_CODE, "RETURN-CODE", 0 );
    CreateDicEntryC( ID_VAR_TRACE_FLAGS, "TRACE-FLAGS", 0 );
    CreateDicEntryC( ID_VAR_TRACE_LEVEL, "TRACE-LEVEL", 0 );
    CreateDicEntryC( ID_VAR_TRACE_STACK, "TRACE-STACK", 0 );
    CreateDicEntryC( ID_VAR_OUT, "OUT", 0 );
    CreateDicEntryC( ID_VAR_STATE, "STATE", 0 );
    CreateDicEntryC( ID_VAR_TO_IN, ">IN", 0 );
    CreateDicEntryC( ID_VERSION_CODE, "VERSION_CODE", 0 );
    CreateDicEntryC( ID_WORD, "WORD", 0 );
    CreateDicEntryC( ID_WORD_FETCH, "W@", 0 );
    CreateDicEntryC( ID_WORD_STORE, "W!", 0 );
    CreateDicEntryC( ID_XOR, "XOR", 0 );
    CreateDicEntryC( ID_ZERO_BRANCH, "0BRANCH", 0 );

    //****************************************************************
    // Add raylib words
    CreateDicEntryC(XT_INIT_WINDOW, "INIT-WINDOW", 0);
    CreateDicEntryC(XT_CLOSE_WINDOW, "CLOSE-WINDOW", 0);
    CreateDicEntryC(XT_WINDOW_SHOULD_CLOSE, "WINDOW-SHOULD-CLOSE", 0);
    CreateDicEntryC(XT_IS_WINDOW_READY, "IS-WINDOW-READY", 0);
    CreateDicEntryC(XT_IS_WINDOW_FULLSCREEN, "IS-WINDOW-FULLSCREEN", 0);
    CreateDicEntryC(XT_IS_WINDOW_HIDDEN, "IS-WINDOW-HIDDEN", 0);
    CreateDicEntryC(XT_IS_WINDOW_MINIMIZED, "IS-WINDOW-MINIMIZED", 0);
    CreateDicEntryC(XT_IS_WINDOW_MAXIMIZED, "IS-WINDOW-MAXIMIZED", 0);
    CreateDicEntryC(XT_IS_WINDOW_FOCUSED, "IS-WINDOW-FOCUSED", 0);
    CreateDicEntryC(XT_IS_WINDOW_RESIZED, "IS-WINDOW-RESIZED", 0);
    CreateDicEntryC(XT_IS_WINDOW_STATE, "IS-WINDOW-STATE", 0);
    CreateDicEntryC(XT_SET_WINDOW_STATE, "SET-WINDOW-STATE", 0);
    CreateDicEntryC(XT_CLEAR_WINDOW_STATE, "CLEAR-WINDOW-STATE", 0);
    CreateDicEntryC(XT_TOGGLE_FULLSCREEN, "TOGGLE-FULLSCREEN", 0);
    CreateDicEntryC(XT_TOGGLE_BORDERLESS_WINDOWED, "TOGGLE-BORDERLESS-WINDOWED", 0);
    CreateDicEntryC(XT_MAXIMIZE_WINDOW, "MAXIMIZE-WINDOW", 0);
    CreateDicEntryC(XT_MINIMIZE_WINDOW, "MINIMIZE-WINDOW", 0);
    CreateDicEntryC(XT_RESTORE_WINDOW, "RESTORE-WINDOW", 0);
    CreateDicEntryC(XT_SET_WINDOW_ICON, "SET-WINDOW-ICON", 0);
    CreateDicEntryC(XT_SET_WINDOW_ICONS, "SET-WINDOW-ICONS", 0);
    CreateDicEntryC(XT_SET_WINDOW_TITLE, "SET-WINDOW-TITLE", 0);
    CreateDicEntryC(XT_SET_WINDOW_POSITION, "SET-WINDOW-POSITION", 0);
    CreateDicEntryC(XT_SET_WINDOW_MONITOR, "SET-WINDOW-MONITOR", 0);
    CreateDicEntryC(XT_SET_WINDOW_MIN_SIZE, "SET-WINDOW-MIN-SIZE", 0);
    CreateDicEntryC(XT_SET_WINDOW_MAX_SIZE, "SET-WINDOW-MAX-SIZE", 0);
    CreateDicEntryC(XT_SET_WINDOW_SIZE, "SET-WINDOW-SIZE", 0);
    CreateDicEntryC(XT_SET_WINDOW_OPACITY, "SET-WINDOW-OPACITY", 0);
    CreateDicEntryC(XT_SET_WINDOW_FOCUSED, "SET-WINDOW-FOCUSED", 0);
    CreateDicEntryC(XT_GET_WINDOW_HANDLE, "GET-WINDOW-HANDLE", 0);
    CreateDicEntryC(XT_GET_SCREEN_WIDTH, "GET-SCREEN-WIDTH", 0);
    CreateDicEntryC(XT_GET_SCREEN_HEIGHT, "GET-SCREEN-HEIGHT", 0);
    CreateDicEntryC(XT_GET_RENDER_WIDTH, "GET-RENDER-WIDTH", 0);
    CreateDicEntryC(XT_GET_RENDER_HEIGHT, "GET-RENDER-HEIGHT", 0);
    CreateDicEntryC(XT_GET_MONITOR_COUNT, "GET-MONITOR-COUNT", 0);
    CreateDicEntryC(XT_GET_CURRENT_MONITOR, "GET-CURRENT-MONITOR", 0);
    CreateDicEntryC(XT_GET_MONITOR_POSITION, "GET-MONITOR-POSITION", 0);
    CreateDicEntryC(XT_GET_MONITOR_WIDTH, "GET-MONITOR-WIDTH", 0);
    CreateDicEntryC(XT_GET_MONITOR_HEIGHT, "GET-MONITOR-HEIGHT", 0);
    CreateDicEntryC(XT_GET_MONITOR_PHYSICAL_WIDTH, "GET-MONITOR-PHYSICAL-WIDTH", 0);
    CreateDicEntryC(XT_GET_MONITOR_PHYSICAL_HEIGHT, "GET-MONITOR-PHYSICAL-HEIGHT", 0);
    CreateDicEntryC(XT_GET_MONITOR_REFRESH_RATE, "GET-MONITOR-REFRESH-RATE", 0);
    CreateDicEntryC(XT_GET_WINDOW_POSITION, "GET-WINDOW-POSITION", 0);
    CreateDicEntryC(XT_GET_WINDOW_SCALE_DPI, "GET-WINDOW-SCALE-DPI", 0);
    CreateDicEntryC(XT_GET_MONITOR_NAME, "GET-MONITOR-NAME", 0);
    CreateDicEntryC(XT_SET_CLIPBOARD_TEXT, "SET-CLIPBOARD-TEXT", 0);
    CreateDicEntryC(XT_GET_CLIPBOARD_TEXT, "GET-CLIPBOARD-TEXT", 0);
    CreateDicEntryC(XT_ENABLE_EVENT_WAITING, "ENABLE-EVENT-WAITING", 0);
    CreateDicEntryC(XT_DISABLE_EVENT_WAITING, "DISABLE-EVENT-WAITING", 0);
    CreateDicEntryC(XT_SHOW_CURSOR, "SHOW-CURSOR", 0);
    CreateDicEntryC(XT_HIDE_CURSOR, "HIDE-CURSOR", 0);
    CreateDicEntryC(XT_IS_CURSOR_HIDDEN, "IS-CURSOR-HIDDEN", 0);
    CreateDicEntryC(XT_ENABLE_CURSOR, "ENABLE-CURSOR", 0);
    CreateDicEntryC(XT_DISABLE_CURSOR, "DISABLE-CURSOR", 0);
    CreateDicEntryC(XT_IS_CURSOR_ON_SCREEN, "IS-CURSOR-ON-SCREEN", 0);
    CreateDicEntryC(XT_CLEAR_BACKGROUND, "CLEAR-BACKGROUND", 0);
    CreateDicEntryC(XT_BEGIN_DRAWING, "BEGIN-DRAWING", 0);
    CreateDicEntryC(XT_END_DRAWING, "END-DRAWING", 0);
    CreateDicEntryC(XT_BEGIN_MODE2D, "BEGIN-MODE2D", 0);
    CreateDicEntryC(XT_END_MODE2D, "END-MODE2D", 0);
    CreateDicEntryC(XT_BEGIN_MODE3D, "BEGIN-MODE3D", 0);
    CreateDicEntryC(XT_END_MODE3D, "END-MODE3D", 0);
    CreateDicEntryC(XT_BEGIN_TEXTURE_MODE, "BEGIN-TEXTURE-MODE", 0);
    CreateDicEntryC(XT_END_TEXTURE_MODE, "END-TEXTURE-MODE", 0);
    CreateDicEntryC(XT_BEGIN_SHADER_MODE, "BEGIN-SHADER-MODE", 0);
    CreateDicEntryC(XT_END_SHADER_MODE, "END-SHADER-MODE", 0);
    CreateDicEntryC(XT_BEGIN_BLEND_MODE, "BEGIN-BLEND-MODE", 0);
    CreateDicEntryC(XT_END_BLEND_MODE, "END-BLEND-MODE", 0);
    CreateDicEntryC(XT_BEGIN_SCISSOR_MODE, "BEGIN-SCISSOR-MODE", 0);
    CreateDicEntryC(XT_END_SCISSOR_MODE, "END-SCISSOR-MODE", 0);
    CreateDicEntryC(XT_BEGIN_VR_STEREO_MODE, "BEGIN-VR-STEREO-MODE", 0);
    CreateDicEntryC(XT_END_VR_STEREO_MODE, "END-VR-STEREO-MODE", 0);
    CreateDicEntryC(XT_LOAD_VR_STEREO_CONFIG, "LOAD-VR-STEREO-CONFIG", 0);
    CreateDicEntryC(XT_UNLOAD_VR_STEREO_CONFIG, "UNLOAD-VR-STEREO-CONFIG", 0);
    CreateDicEntryC(XT_LOAD_SHADER, "LOAD-SHADER", 0);
    CreateDicEntryC(XT_LOAD_SHADER_FROM_MEMORY, "LOAD-SHADER-FROM-MEMORY", 0);
    CreateDicEntryC(XT_IS_SHADER_READY, "IS-SHADER-READY", 0);
    CreateDicEntryC(XT_GET_SHADER_LOCATION, "GET-SHADER-LOCATION", 0);
    CreateDicEntryC(XT_GET_SHADER_LOCATION_ATTRIB, "GET-SHADER-LOCATION-ATTRIB", 0);
    CreateDicEntryC(XT_SET_SHADER_VALUE, "SET-SHADER-VALUE", 0);
    CreateDicEntryC(XT_SET_SHADER_VALUE_V, "SET-SHADER-VALUE-V", 0);
    CreateDicEntryC(XT_SET_SHADER_VALUE_MATRIX, "SET-SHADER-VALUE-MATRIX", 0);
    CreateDicEntryC(XT_SET_SHADER_VALUE_TEXTURE, "SET-SHADER-VALUE-TEXTURE", 0);
    CreateDicEntryC(XT_UNLOAD_SHADER, "UNLOAD-SHADER", 0);
    CreateDicEntryC(XT_GET_MOUSE_RAY, "GET-MOUSE-RAY", 0);
    CreateDicEntryC(XT_GET_CAMERA_MATRIX, "GET-CAMERA-MATRIX", 0);
    CreateDicEntryC(XT_GET_CAMERA_MATRIX_2D, "GET-CAMERA-MATRIX-2D", 0);
    CreateDicEntryC(XT_GET_WORLD_TO_SCREEN, "GET-WORLD-TO-SCREEN", 0);
    CreateDicEntryC(XT_GET_SCREEN_TO_WORLD_2D, "GET-SCREEN-TO-WORLD-2D", 0);
    CreateDicEntryC(XT_GET_WORLD_TO_SCREEN_EX, "GET-WORLD-TO-SCREEN-EX", 0);
    CreateDicEntryC(XT_GET_WORLD_TO_SCREEN_2D, "GET-WORLD-TO-SCREEN-2D", 0);
    CreateDicEntryC(XT_SET_TARGET_FPS, "SET-TARGET-FPS", 0);
    CreateDicEntryC(XT_GET_FRAME_TIME, "GET-FRAME-TIME", 0);
    CreateDicEntryC(XT_GET_TIME, "GET-TIME", 0);
    CreateDicEntryC(XT_GET_FPS, "GET-FPS", 0);
    CreateDicEntryC(XT_SWAP_SCREEN_BUFFER, "SWAP-SCREEN-BUFFER", 0);
    CreateDicEntryC(XT_POLL_INPUT_EVENTS, "POLL-INPUT-EVENTS", 0);
    CreateDicEntryC(XT_WAIT_TIME, "WAIT-TIME", 0);
    CreateDicEntryC(XT_SET_RANDOM_SEED, "SET-RANDOM-SEED", 0);
    CreateDicEntryC(XT_GET_RANDOM_VALUE, "GET-RANDOM-VALUE", 0);
    CreateDicEntryC(XT_LOAD_RANDOM_SEQUENCE, "LOAD-RANDOM-SEQUENCE", 0);
    CreateDicEntryC(XT_UNLOAD_RANDOM_SEQUENCE, "UNLOAD-RANDOM-SEQUENCE", 0);
    CreateDicEntryC(XT_TAKE_SCREENSHOT, "TAKE-SCREENSHOT", 0);
    CreateDicEntryC(XT_SET_CONFIG_FLAGS, "SET-CONFIG-FLAGS", 0);
    CreateDicEntryC(XT_OPEN_URL, "OPEN-URL", 0);
    CreateDicEntryC(XT_TRACE_LOG, "TRACE-LOG", 0);
    CreateDicEntryC(XT_SET_TRACE_LOG_LEVEL, "SET-TRACE-LOG-LEVEL", 0);
    CreateDicEntryC(XT_MEM_ALLOC, "MEM-ALLOC", 0);
    CreateDicEntryC(XT_MEM_REALLOC, "MEM-REALLOC", 0);
    CreateDicEntryC(XT_MEM_FREE, "MEM-FREE", 0);
    CreateDicEntryC(XT_SET_TRACE_LOG_CALLBACK, "SET-TRACE-LOG-CALLBACK", 0);
    CreateDicEntryC(XT_SET_LOAD_FILE_DATA_CALLBACK, "SET-LOAD-FILE-DATA-CALLBACK", 0);
    CreateDicEntryC(XT_SET_SAVE_FILE_DATA_CALLBACK, "SET-SAVE-FILE-DATA-CALLBACK", 0);
    CreateDicEntryC(XT_SET_LOAD_FILE_TEXT_CALLBACK, "SET-LOAD-FILE-TEXT-CALLBACK", 0);
    CreateDicEntryC(XT_SET_SAVE_FILE_TEXT_CALLBACK, "SET-SAVE-FILE-TEXT-CALLBACK", 0);
    CreateDicEntryC(XT_LOAD_FILE_DATA, "LOAD-FILE-DATA", 0);
    CreateDicEntryC(XT_UNLOAD_FILE_DATA, "UNLOAD-FILE-DATA", 0);
    CreateDicEntryC(XT_SAVE_FILE_DATA, "SAVE-FILE-DATA", 0);
    CreateDicEntryC(XT_EXPORT_DATA_AS_CODE, "EXPORT-DATA-AS-CODE", 0);
    CreateDicEntryC(XT_LOAD_FILE_TEXT, "LOAD-FILE-TEXT", 0);
    CreateDicEntryC(XT_UNLOAD_FILE_TEXT, "UNLOAD-FILE-TEXT", 0);
    CreateDicEntryC(XT_SAVE_FILE_TEXT, "SAVE-FILE-TEXT", 0);
    CreateDicEntryC(XT_FILE_EXISTS, "FILE-EXISTS", 0);
    CreateDicEntryC(XT_DIRECTORY_EXISTS, "DIRECTORY-EXISTS", 0);
    CreateDicEntryC(XT_IS_FILE_EXTENSION, "IS-FILE-EXTENSION", 0);
    CreateDicEntryC(XT_GET_FILE_LENGTH, "GET-FILE-LENGTH", 0);
    CreateDicEntryC(XT_GET_FILE_EXTENSION, "GET-FILE-EXTENSION", 0);
    CreateDicEntryC(XT_GET_FILE_NAME, "GET-FILE-NAME", 0);
    CreateDicEntryC(XT_GET_FILE_NAME_WITHOUT_EXT, "GET-FILE-NAME-WITHOUT-EXT", 0);
    CreateDicEntryC(XT_GET_DIRECTORY_PATH, "GET-DIRECTORY-PATH", 0);
    CreateDicEntryC(XT_GET_PREV_DIRECTORY_PATH, "GET-PREV-DIRECTORY-PATH", 0);
    CreateDicEntryC(XT_GET_WORKING_DIRECTORY, "GET-WORKING-DIRECTORY", 0);
    CreateDicEntryC(XT_GET_APPLICATION_DIRECTORY, "GET-APPLICATION-DIRECTORY", 0);
    CreateDicEntryC(XT_CHANGE_DIRECTORY, "CHANGE-DIRECTORY", 0);
    CreateDicEntryC(XT_IS_PATH_FILE, "IS-PATH-FILE", 0);
    CreateDicEntryC(XT_LOAD_DIRECTORY_FILES, "LOAD-DIRECTORY-FILES", 0);
    CreateDicEntryC(XT_LOAD_DIRECTORY_FILES_EX, "LOAD-DIRECTORY-FILES-EX", 0);
    CreateDicEntryC(XT_UNLOAD_DIRECTORY_FILES, "UNLOAD-DIRECTORY-FILES", 0);
    CreateDicEntryC(XT_IS_FILE_DROPPED, "IS-FILE-DROPPED", 0);
    CreateDicEntryC(XT_LOAD_DROPPED_FILES, "LOAD-DROPPED-FILES", 0);
    CreateDicEntryC(XT_UNLOAD_DROPPED_FILES, "UNLOAD-DROPPED-FILES", 0);
    CreateDicEntryC(XT_GET_FILE_MOD_TIME, "GET-FILE-MOD-TIME", 0);
    CreateDicEntryC(XT_COMPRESS_DATA, "COMPRESS-DATA", 0);
    CreateDicEntryC(XT_DECOMPRESS_DATA, "DECOMPRESS-DATA", 0);
    CreateDicEntryC(XT_ENCODE_DATA_BASE64, "ENCODE-DATA-BASE64", 0);
    CreateDicEntryC(XT_DECODE_DATA_BASE64, "DECODE-DATA-BASE64", 0);
    CreateDicEntryC(XT_LOAD_AUTOMATION_EVENT_LIST, "LOAD-AUTOMATION-EVENT-LIST", 0);
    CreateDicEntryC(XT_UNLOAD_AUTOMATION_EVENT_LIST, "UNLOAD-AUTOMATION-EVENT-LIST", 0);
    CreateDicEntryC(XT_EXPORT_AUTOMATION_EVENT_LIST, "EXPORT-AUTOMATION-EVENT-LIST", 0);
    CreateDicEntryC(XT_SET_AUTOMATION_EVENT_LIST, "SET-AUTOMATION-EVENT-LIST", 0);
    CreateDicEntryC(XT_SET_AUTOMATION_EVENT_BASE_FRAME, "SET-AUTOMATION-EVENT-BASE-FRAME", 0);
    CreateDicEntryC(XT_START_AUTOMATION_EVENT_RECORDING, "START-AUTOMATION-EVENT-RECORDING", 0);
    CreateDicEntryC(XT_STOP_AUTOMATION_EVENT_RECORDING, "STOP-AUTOMATION-EVENT-RECORDING", 0);
    CreateDicEntryC(XT_PLAY_AUTOMATION_EVENT, "PLAY-AUTOMATION-EVENT", 0);
    CreateDicEntryC(XT_IS_KEY_PRESSED, "IS-KEY-PRESSED", 0);
    CreateDicEntryC(XT_IS_KEY_PRESSED_REPEAT, "IS-KEY-PRESSED-REPEAT", 0);
    CreateDicEntryC(XT_IS_KEY_DOWN, "IS-KEY-DOWN", 0);
    CreateDicEntryC(XT_IS_KEY_RELEASED, "IS-KEY-RELEASED", 0);
    CreateDicEntryC(XT_IS_KEY_UP, "IS-KEY-UP", 0);
    CreateDicEntryC(XT_GET_KEY_PRESSED, "GET-KEY-PRESSED", 0);
    CreateDicEntryC(XT_GET_CHAR_PRESSED, "GET-CHAR-PRESSED", 0);
    CreateDicEntryC(XT_SET_EXIT_KEY, "SET-EXIT-KEY", 0);
    CreateDicEntryC(XT_IS_GAMEPAD_AVAILABLE, "IS-GAMEPAD-AVAILABLE", 0);
    CreateDicEntryC(XT_GET_GAMEPAD_NAME, "GET-GAMEPAD-NAME", 0);
    CreateDicEntryC(XT_IS_GAMEPAD_BUTTON_PRESSED, "IS-GAMEPAD-BUTTON-PRESSED", 0);
    CreateDicEntryC(XT_IS_GAMEPAD_BUTTON_DOWN, "IS-GAMEPAD-BUTTON-DOWN", 0);
    CreateDicEntryC(XT_IS_GAMEPAD_BUTTON_RELEASED, "IS-GAMEPAD-BUTTON-RELEASED", 0);
    CreateDicEntryC(XT_IS_GAMEPAD_BUTTON_UP, "IS-GAMEPAD-BUTTON-UP", 0);
    CreateDicEntryC(XT_GET_GAMEPAD_BUTTON_PRESSED, "GET-GAMEPAD-BUTTON-PRESSED", 0);
    CreateDicEntryC(XT_GET_GAMEPAD_AXIS_COUNT, "GET-GAMEPAD-AXIS-COUNT", 0);
    CreateDicEntryC(XT_GET_GAMEPAD_AXIS_MOVEMENT, "GET-GAMEPAD-AXIS-MOVEMENT", 0);
    CreateDicEntryC(XT_SET_GAMEPAD_MAPPINGS, "SET-GAMEPAD-MAPPINGS", 0);
    CreateDicEntryC(XT_IS_MOUSE_BUTTON_PRESSED, "IS-MOUSE-BUTTON-PRESSED", 0);
    CreateDicEntryC(XT_IS_MOUSE_BUTTON_DOWN, "IS-MOUSE-BUTTON-DOWN", 0);
    CreateDicEntryC(XT_IS_MOUSE_BUTTON_RELEASED, "IS-MOUSE-BUTTON-RELEASED", 0);
    CreateDicEntryC(XT_IS_MOUSE_BUTTON_UP, "IS-MOUSE-BUTTON-UP", 0);
    CreateDicEntryC(XT_GET_MOUSE_X, "GET-MOUSE-X", 0);
    CreateDicEntryC(XT_GET_MOUSE_Y, "GET-MOUSE-Y", 0);
    CreateDicEntryC(XT_GET_MOUSE_POSITION, "GET-MOUSE-POSITION", 0);
    CreateDicEntryC(XT_GET_MOUSE_DELTA, "GET-MOUSE-DELTA", 0);
    CreateDicEntryC(XT_SET_MOUSE_POSITION, "SET-MOUSE-POSITION", 0);
    CreateDicEntryC(XT_SET_MOUSE_OFFSET, "SET-MOUSE-OFFSET", 0);
    CreateDicEntryC(XT_SET_MOUSE_SCALE, "SET-MOUSE-SCALE", 0);
    CreateDicEntryC(XT_GET_MOUSE_WHEEL_MOVE, "GET-MOUSE-WHEEL-MOVE", 0);
    CreateDicEntryC(XT_GET_MOUSE_WHEEL_MOVE_V, "GET-MOUSE-WHEEL-MOVE-V", 0);
    CreateDicEntryC(XT_SET_MOUSE_CURSOR, "SET-MOUSE-CURSOR", 0);
    CreateDicEntryC(XT_GET_TOUCH_X, "GET-TOUCH-X", 0);
    CreateDicEntryC(XT_GET_TOUCH_Y, "GET-TOUCH-Y", 0);
    CreateDicEntryC(XT_GET_TOUCH_POSITION, "GET-TOUCH-POSITION", 0);
    CreateDicEntryC(XT_GET_TOUCH_POINT_ID, "GET-TOUCH-POINT-ID", 0);
    CreateDicEntryC(XT_GET_TOUCH_POINT_COUNT, "GET-TOUCH-POINT-COUNT", 0);
    CreateDicEntryC(XT_SET_GESTURES_ENABLED, "SET-GESTURES-ENABLED", 0);
    CreateDicEntryC(XT_IS_GESTURE_DETECTED, "IS-GESTURE-DETECTED", 0);
    CreateDicEntryC(XT_GET_GESTURE_DETECTED, "GET-GESTURE-DETECTED", 0);
    CreateDicEntryC(XT_GET_GESTURE_HOLD_DURATION, "GET-GESTURE-HOLD-DURATION", 0);
    CreateDicEntryC(XT_GET_GESTURE_DRAG_VECTOR, "GET-GESTURE-DRAG-VECTOR", 0);
    CreateDicEntryC(XT_GET_GESTURE_DRAG_ANGLE, "GET-GESTURE-DRAG-ANGLE", 0);
    CreateDicEntryC(XT_GET_GESTURE_PINCH_VECTOR, "GET-GESTURE-PINCH-VECTOR", 0);
    CreateDicEntryC(XT_GET_GESTURE_PINCH_ANGLE, "GET-GESTURE-PINCH-ANGLE", 0);
    CreateDicEntryC(XT_UPDATE_CAMERA, "UPDATE-CAMERA", 0);
    CreateDicEntryC(XT_UPDATE_CAMERA_PRO, "UPDATE-CAMERA-PRO", 0);
    CreateDicEntryC(XT_SET_SHAPES_TEXTURE, "SET-SHAPES-TEXTURE", 0);
    CreateDicEntryC(XT_DRAW_PIXEL, "DRAW-PIXEL", 0);
    CreateDicEntryC(XT_DRAW_PIXEL_V, "DRAW-PIXEL-V", 0);
    CreateDicEntryC(XT_DRAW_LINE, "DRAW-LINE", 0);
    CreateDicEntryC(XT_DRAW_LINE_V, "DRAW-LINE-V", 0);
    CreateDicEntryC(XT_DRAW_LINE_EX, "DRAW-LINE-EX", 0);
    CreateDicEntryC(XT_DRAW_LINE_STRIP, "DRAW-LINE-STRIP", 0);
    CreateDicEntryC(XT_DRAW_LINE_BEZIER, "DRAW-LINE-BEZIER", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE, "DRAW-CIRCLE", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_SECTOR, "DRAW-CIRCLE-SECTOR", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_SECTOR_LINES, "DRAW-CIRCLE-SECTOR-LINES", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_GRADIENT, "DRAW-CIRCLE-GRADIENT", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_V, "DRAW-CIRCLE-V", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_LINES, "DRAW-CIRCLE-LINES", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_LINES_V, "DRAW-CIRCLE-LINES-V", 0);
    CreateDicEntryC(XT_DRAW_ELLIPSE, "DRAW-ELLIPSE", 0);
    CreateDicEntryC(XT_DRAW_ELLIPSE_LINES, "DRAW-ELLIPSE-LINES", 0);
    CreateDicEntryC(XT_DRAW_RING, "DRAW-RING", 0);
    CreateDicEntryC(XT_DRAW_RING_LINES, "DRAW-RING-LINES", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE, "DRAW-RECTANGLE", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_V, "DRAW-RECTANGLE-V", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_REC, "DRAW-RECTANGLE-REC", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_PRO, "DRAW-RECTANGLE-PRO", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_GRADIENT_V, "DRAW-RECTANGLE-GRADIENT-V", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_GRADIENT_H, "DRAW-RECTANGLE-GRADIENT-H", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_GRADIENT_EX, "DRAW-RECTANGLE-GRADIENT-EX", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_LINES, "DRAW-RECTANGLE-LINES", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_LINES_EX, "DRAW-RECTANGLE-LINES-EX", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_ROUNDED, "DRAW-RECTANGLE-ROUNDED", 0);
    CreateDicEntryC(XT_DRAW_RECTANGLE_ROUNDED_LINES, "DRAW-RECTANGLE-ROUNDED-LINES", 0);
    CreateDicEntryC(XT_DRAW_TRIANGLE, "DRAW-TRIANGLE", 0);
    CreateDicEntryC(XT_DRAW_TRIANGLE_LINES, "DRAW-TRIANGLE-LINES", 0);
    CreateDicEntryC(XT_DRAW_TRIANGLE_FAN, "DRAW-TRIANGLE-FAN", 0);
    CreateDicEntryC(XT_DRAW_TRIANGLE_STRIP, "DRAW-TRIANGLE-STRIP", 0);
    CreateDicEntryC(XT_DRAW_POLY, "DRAW-POLY", 0);
    CreateDicEntryC(XT_DRAW_POLY_LINES, "DRAW-POLY-LINES", 0);
    CreateDicEntryC(XT_DRAW_POLY_LINES_EX, "DRAW-POLY-LINES-EX", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_LINEAR, "DRAW-SPLINE-LINEAR", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_BASIS, "DRAW-SPLINE-BASIS", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_CATMULL_ROM, "DRAW-SPLINE-CATMULL-ROM", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_BEZIER_QUADRATIC, "DRAW-SPLINE-BEZIER-QUADRATIC", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_BEZIER_CUBIC, "DRAW-SPLINE-BEZIER-CUBIC", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_SEGMENT_LINEAR, "DRAW-SPLINE-SEGMENT-LINEAR", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_SEGMENT_BASIS, "DRAW-SPLINE-SEGMENT-BASIS", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_SEGMENT_CATMULL_ROM, "DRAW-SPLINE-SEGMENT-CATMULL-ROM", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_SEGMENT_BEZIER_QUADRATIC, "DRAW-SPLINE-SEGMENT-BEZIER-QUADRATIC", 0);
    CreateDicEntryC(XT_DRAW_SPLINE_SEGMENT_BEZIER_CUBIC, "DRAW-SPLINE-SEGMENT-BEZIER-CUBIC", 0);
    CreateDicEntryC(XT_GET_SPLINE_POINT_LINEAR, "GET-SPLINE-POINT-LINEAR", 0);
    CreateDicEntryC(XT_GET_SPLINE_POINT_BASIS, "GET-SPLINE-POINT-BASIS", 0);
    CreateDicEntryC(XT_GET_SPLINE_POINT_CATMULL_ROM, "GET-SPLINE-POINT-CATMULL-ROM", 0);
    CreateDicEntryC(XT_GET_SPLINE_POINT_BEZIER_QUAD, "GET-SPLINE-POINT-BEZIER-QUAD", 0);
    CreateDicEntryC(XT_GET_SPLINE_POINT_BEZIER_CUBIC, "GET-SPLINE-POINT-BEZIER-CUBIC", 0);
    CreateDicEntryC(XT_CHECK_COLLISION_RECS, "CHECK-COLLISION-RECS", 0);
    CreateDicEntryC(XT_CHECK_COLLISION_CIRCLES, "CHECK-COLLISION-CIRCLES", 0);
    CreateDicEntryC(XT_CHECK_COLLISION_CIRCLE_REC, "CHECK-COLLISION-CIRCLE-REC", 0);
    CreateDicEntryC(XT_CHECK_COLLISION_POINT_REC, "CHECK-COLLISION-POINT-REC", 0);
    CreateDicEntryC(XT_CHECK_COLLISION_POINT_CIRCLE, "CHECK-COLLISION-POINT-CIRCLE", 0);
    CreateDicEntryC(XT_LOAD_IMAGE, "LOAD-IMAGE", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_RAW, "LOAD-IMAGE-RAW", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_SVG, "LOAD-IMAGE-SVG", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_ANIM, "LOAD-IMAGE-ANIM", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_FROM_MEMORY, "LOAD-IMAGE-FROM-MEMORY", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_FROM_TEXTURE, "LOAD-IMAGE-FROM-TEXTURE", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_FROM_SCREEN, "LOAD-IMAGE-FROM-SCREEN", 0);
    CreateDicEntryC(XT_IS_IMAGE_READY, "IS-IMAGE-READY", 0);
    CreateDicEntryC(XT_UNLOAD_IMAGE, "UNLOAD-IMAGE", 0);
    CreateDicEntryC(XT_EXPORT_IMAGE, "EXPORT-IMAGE", 0);
    CreateDicEntryC(XT_EXPORT_IMAGE_TO_MEMORY, "EXPORT-IMAGE-TO-MEMORY", 0);
    CreateDicEntryC(XT_EXPORT_IMAGE_AS_CODE, "EXPORT-IMAGE-AS-CODE", 0);
    CreateDicEntryC(XT_GEN_IMAGE_COLOR, "GEN-IMAGE-COLOR", 0);
    CreateDicEntryC(XT_GEN_IMAGE_GRADIENT_LINEAR, "GEN-IMAGE-GRADIENT-LINEAR", 0);
    CreateDicEntryC(XT_GEN_IMAGE_GRADIENT_RADIAL, "GEN-IMAGE-GRADIENT-RADIAL", 0);
    CreateDicEntryC(XT_GEN_IMAGE_GRADIENT_SQUARE, "GEN-IMAGE-GRADIENT-SQUARE", 0);
    CreateDicEntryC(XT_GEN_IMAGE_CHECKED, "GEN-IMAGE-CHECKED", 0);
    CreateDicEntryC(XT_GEN_IMAGE_WHITE_NOISE, "GEN-IMAGE-WHITE-NOISE", 0);
    CreateDicEntryC(XT_GEN_IMAGE_PERLIN_NOISE, "GEN-IMAGE-PERLIN-NOISE", 0);
    CreateDicEntryC(XT_GEN_IMAGE_CELLULAR, "GEN-IMAGE-CELLULAR", 0);
    CreateDicEntryC(XT_GEN_IMAGE_TEXT, "GEN-IMAGE-TEXT", 0);
    CreateDicEntryC(XT_IMAGE_COPY, "IMAGE-COPY", 0);
    CreateDicEntryC(XT_IMAGE_FROM_IMAGE, "IMAGE-FROM-IMAGE", 0);
    CreateDicEntryC(XT_IMAGE_TEXT, "IMAGE-TEXT", 0);
    CreateDicEntryC(XT_IMAGE_TEXT_EX, "IMAGE-TEXT-EX", 0);
    CreateDicEntryC(XT_IMAGE_FORMAT, "IMAGE-FORMAT", 0);
    CreateDicEntryC(XT_IMAGE_TO_POT, "IMAGE-TO-POT", 0);
    CreateDicEntryC(XT_IMAGE_CROP, "IMAGE-CROP", 0);
    CreateDicEntryC(XT_IMAGE_ALPHA_CROP, "IMAGE-ALPHA-CROP", 0);
    CreateDicEntryC(XT_IMAGE_ALPHA_CLEAR, "IMAGE-ALPHA-CLEAR", 0);
    CreateDicEntryC(XT_IMAGE_ALPHA_MASK, "IMAGE-ALPHA-MASK", 0);
    CreateDicEntryC(XT_IMAGE_ALPHA_PREMULTIPLY, "IMAGE-ALPHA-PREMULTIPLY", 0);
    CreateDicEntryC(XT_IMAGE_BLUR_GAUSSIAN, "IMAGE-BLUR-GAUSSIAN", 0);
    CreateDicEntryC(XT_IMAGE_RESIZE, "IMAGE-RESIZE", 0);
    CreateDicEntryC(XT_IMAGE_RESIZE_NN, "IMAGE-RESIZE-NN", 0);
    CreateDicEntryC(XT_IMAGE_RESIZE_CANVAS, "IMAGE-RESIZE-CANVAS", 0);
    CreateDicEntryC(XT_IMAGE_MIPMAPS, "IMAGE-MIPMAPS", 0);
    CreateDicEntryC(XT_IMAGE_DITHER, "IMAGE-DITHER", 0);
    CreateDicEntryC(XT_IMAGE_FLIP_VERTICAL, "IMAGE-FLIP-VERTICAL", 0);
    CreateDicEntryC(XT_IMAGE_FLIP_HORIZONTAL, "IMAGE-FLIP-HORIZONTAL", 0);
    CreateDicEntryC(XT_IMAGE_ROTATE, "IMAGE-ROTATE", 0);
    CreateDicEntryC(XT_IMAGE_ROTATE_CW, "IMAGE-ROTATE-CW", 0);
    CreateDicEntryC(XT_IMAGE_ROTATE_CCW, "IMAGE-ROTATE-CCW", 0);
    CreateDicEntryC(XT_IMAGE_COLOR_TINT, "IMAGE-COLOR-TINT", 0);
    CreateDicEntryC(XT_IMAGE_COLOR_INVERT, "IMAGE-COLOR-INVERT", 0);
    CreateDicEntryC(XT_IMAGE_COLOR_GRAYSCALE, "IMAGE-COLOR-GRAYSCALE", 0);
    CreateDicEntryC(XT_IMAGE_COLOR_CONTRAST, "IMAGE-COLOR-CONTRAST", 0);
    CreateDicEntryC(XT_IMAGE_COLOR_BRIGHTNESS, "IMAGE-COLOR-BRIGHTNESS", 0);
    CreateDicEntryC(XT_IMAGE_COLOR_REPLACE, "IMAGE-COLOR-REPLACE", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_COLORS, "LOAD-IMAGE-COLORS", 0);
    CreateDicEntryC(XT_LOAD_IMAGE_PALETTE, "LOAD-IMAGE-PALETTE", 0);
    CreateDicEntryC(XT_UNLOAD_IMAGE_COLORS, "UNLOAD-IMAGE-COLORS", 0);
    CreateDicEntryC(XT_UNLOAD_IMAGE_PALETTE, "UNLOAD-IMAGE-PALETTE", 0);
    CreateDicEntryC(XT_GET_IMAGE_ALPHA_BORDER, "GET-IMAGE-ALPHA-BORDER", 0);
    CreateDicEntryC(XT_GET_IMAGE_COLOR, "GET-IMAGE-COLOR", 0);
    CreateDicEntryC(XT_IMAGE_CLEAR_BACKGROUND, "IMAGE-CLEAR-BACKGROUND", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_PIXEL, "IMAGE-DRAW-PIXEL", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_PIXEL_V, "IMAGE-DRAW-PIXEL-V", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_LINE, "IMAGE-DRAW-LINE", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_LINE_V, "IMAGE-DRAW-LINE-V", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_CIRCLE, "IMAGE-DRAW-CIRCLE", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_CIRCLE_V, "IMAGE-DRAW-CIRCLE-V", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_CIRCLE_LINES, "IMAGE-DRAW-CIRCLE-LINES", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_CIRCLE_LINES_V, "IMAGE-DRAW-CIRCLE-LINES-V", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_RECTANGLE, "IMAGE-DRAW-RECTANGLE", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_RECTANGLE_V, "IMAGE-DRAW-RECTANGLE-V", 0);
    CreateDicEntryC(XT_IMAGE_DRAW_RECTANGLE_REC, "IMAGE-DRAW-RECTANGLE-REC", 0);
    CreateDicEntryC(XT_LOAD_TEXTURE, "LOAD-TEXTURE", 0);
    CreateDicEntryC(XT_LOAD_TEXTURE_FROM_IMAGE, "LOAD-TEXTURE-FROM-IMAGE", 0);
    CreateDicEntryC(XT_LOAD_TEXTURE_CUBEMAP, "LOAD-TEXTURE-CUBEMAP", 0);
    CreateDicEntryC(XT_LOAD_RENDER_TEXTURE, "LOAD-RENDER-TEXTURE", 0);
    CreateDicEntryC(XT_IS_TEXTURE_READY, "IS-TEXTURE-READY", 0);
    CreateDicEntryC(XT_UNLOAD_TEXTURE, "UNLOAD-TEXTURE", 0);
    CreateDicEntryC(XT_IS_RENDER_TEXTURE_READY, "IS-RENDER-TEXTURE-READY", 0);
    CreateDicEntryC(XT_UNLOAD_RENDER_TEXTURE, "UNLOAD-RENDER-TEXTURE", 0);
    CreateDicEntryC(XT_UPDATE_TEXTURE, "UPDATE-TEXTURE", 0);
    CreateDicEntryC(XT_UPDATE_TEXTURE_REC, "UPDATE-TEXTURE-REC", 0);
    CreateDicEntryC(XT_GEN_TEXTURE_MIPMAPS, "GEN-TEXTURE-MIPMAPS", 0);
    CreateDicEntryC(XT_SET_TEXTURE_FILTER, "SET-TEXTURE-FILTER", 0);
    CreateDicEntryC(XT_SET_TEXTURE_WRAP, "SET-TEXTURE-WRAP", 0);
    CreateDicEntryC(XT_DRAW_TEXTURE, "DRAW-TEXTURE", 0);
    CreateDicEntryC(XT_DRAW_TEXTURE_V, "DRAW-TEXTURE-V", 0);
    CreateDicEntryC(XT_DRAW_TEXTURE_EX, "DRAW-TEXTURE-EX", 0);
    CreateDicEntryC(XT_DRAW_TEXTURE_REC, "DRAW-TEXTURE-REC", 0);
    CreateDicEntryC(XT_DRAW_TEXTURE_PRO, "DRAW-TEXTURE-PRO", 0);
    CreateDicEntryC(XT_DRAW_TEXTURE_NPATCH, "DRAW-TEXTURE-NPATCH", 0);
    CreateDicEntryC(XT_FADE, "FADE", 0);
    CreateDicEntryC(XT_COLOR_TO_INT, "COLOR-TO-INT", 0);
    CreateDicEntryC(XT_COLOR_NORMALIZE, "COLOR-NORMALIZE", 0);
    CreateDicEntryC(XT_COLOR_FROM_NORMALIZED, "COLOR-FROM-NORMALIZED", 0);
    CreateDicEntryC(XT_COLOR_TO_HSV, "COLOR-TO-HSV", 0);
    CreateDicEntryC(XT_COLOR_FROM_HSV, "COLOR-FROM-HSV", 0);
    CreateDicEntryC(XT_COLOR_TINT, "COLOR-TINT", 0);
    CreateDicEntryC(XT_COLOR_BRIGHTNESS, "COLOR-BRIGHTNESS", 0);
    CreateDicEntryC(XT_COLOR_CONTRAST, "COLOR-CONTRAST", 0);
    CreateDicEntryC(XT_COLOR_ALPHA, "COLOR-ALPHA", 0);
    CreateDicEntryC(XT_COLOR_ALPHA_BLEND, "COLOR-ALPHA-BLEND", 0);
    CreateDicEntryC(XT_GET_COLOR, "GET-COLOR", 0);
    CreateDicEntryC(XT_GET_PIXEL_COLOR, "GET-PIXEL-COLOR", 0);
    CreateDicEntryC(XT_SET_PIXEL_COLOR, "SET-PIXEL-COLOR", 0);
    CreateDicEntryC(XT_GET_PIXEL_DATA_SIZE, "GET-PIXEL-DATA-SIZE", 0);
    CreateDicEntryC(XT_GET_FONT_DEFAULT, "GET-FONT-DEFAULT", 0);
    CreateDicEntryC(XT_LOAD_FONT, "LOAD-FONT", 0);
    CreateDicEntryC(XT_LOAD_FONT_EX, "LOAD-FONT-EX", 0);
    CreateDicEntryC(XT_LOAD_FONT_FROM_IMAGE, "LOAD-FONT-FROM-IMAGE", 0);
    CreateDicEntryC(XT_LOAD_FONT_FROM_MEMORY, "LOAD-FONT-FROM-MEMORY", 0);
    CreateDicEntryC(XT_IS_FONT_READY, "IS-FONT-READY", 0);
    CreateDicEntryC(XT_LOAD_FONT_DATA, "LOAD-FONT-DATA", 0);
    CreateDicEntryC(XT_GEN_IMAGE_FONT_ATLAS, "GEN-IMAGE-FONT-ATLAS", 0);
    CreateDicEntryC(XT_UNLOAD_FONT_DATA, "UNLOAD-FONT-DATA", 0);
    CreateDicEntryC(XT_UNLOAD_FONT, "UNLOAD-FONT", 0);
    CreateDicEntryC(XT_EXPORT_FONT_AS_CODE, "EXPORT-FONT-AS-CODE", 0);
    CreateDicEntryC(XT_DRAW_FPS, "DRAW-FPS", 0);
    CreateDicEntryC(XT_DRAW_TEXT, "DRAW-TEXT", 0);
    CreateDicEntryC(XT_DRAW_TEXT_EX, "DRAW-TEXT-EX", 0);
    CreateDicEntryC(XT_DRAW_TEXT_PRO, "DRAW-TEXT-PRO", 0);
    CreateDicEntryC(XT_DRAW_TEXT_CODEPOINT, "DRAW-TEXT-CODEPOINT", 0);
    CreateDicEntryC(XT_DRAW_TEXT_CODEPOINTS, "DRAW-TEXT-CODEPOINTS", 0);
    CreateDicEntryC(XT_SET_TEXT_LINE_SPACING, "SET-TEXT-LINE-SPACING", 0);
    CreateDicEntryC(XT_MEASURE_TEXT, "MEASURE-TEXT", 0);
    CreateDicEntryC(XT_MEASURE_TEXT_EX, "MEASURE-TEXT-EX", 0);
    CreateDicEntryC(XT_GET_GLYPH_INDEX, "GET-GLYPH-INDEX", 0);
    CreateDicEntryC(XT_GET_GLYPH_INFO, "GET-GLYPH-INFO", 0);
    CreateDicEntryC(XT_GET_GLYPH_ATLAS_REC, "GET-GLYPH-ATLAS-REC", 0);
    CreateDicEntryC(XT_LOAD_UTF8, "LOAD-UTF8", 0);
    CreateDicEntryC(XT_UNLOAD_UTF8, "UNLOAD-UTF8", 0);
    CreateDicEntryC(XT_LOAD_CODEPOINTS, "LOAD-CODEPOINTS", 0);
    CreateDicEntryC(XT_UNLOAD_CODEPOINTS, "UNLOAD-CODEPOINTS", 0);
    CreateDicEntryC(XT_GET_CODEPOINT_COUNT, "GET-CODEPOINT-COUNT", 0);
    CreateDicEntryC(XT_GET_CODEPOINT, "GET-CODEPOINT", 0);
    CreateDicEntryC(XT_GET_CODEPOINT_NEXT, "GET-CODEPOINT-NEXT", 0);
    CreateDicEntryC(XT_GET_CODEPOINT_PREVIOUS, "GET-CODEPOINT-PREVIOUS", 0);
    CreateDicEntryC(XT_CODEPOINT_TO_UTF8, "CODEPOINT-TO-UTF8", 0);
    CreateDicEntryC(XT_TEXT_COPY, "TEXT-COPY", 0);
    CreateDicEntryC(XT_TEXT_IS_EQUAL, "TEXT-IS-EQUAL", 0);
    CreateDicEntryC(XT_TEXT_LENGTH, "TEXT-LENGTH", 0);
    CreateDicEntryC(XT_TEXT_FORMAT, "TEXT-FORMAT", 0);
    CreateDicEntryC(XT_TEXT_SUBTEXT, "TEXT-SUBTEXT", 0);
    CreateDicEntryC(XT_TEXT_REPLACE, "TEXT-REPLACE", 0);
    CreateDicEntryC(XT_TEXT_INSERT, "TEXT-INSERT", 0);
    CreateDicEntryC(XT_TEXT_JOIN, "TEXT-JOIN", 0);
    CreateDicEntryC(XT_TEXT_SPLIT, "TEXT-SPLIT", 0);
    CreateDicEntryC(XT_TEXT_APPEND, "TEXT-APPEND", 0);
    CreateDicEntryC(XT_TEXT_FIND_INDEX, "TEXT-FIND-INDEX", 0);
    CreateDicEntryC(XT_TEXT_TO_UPPER, "TEXT-TO-UPPER", 0);
    CreateDicEntryC(XT_TEXT_TO_LOWER, "TEXT-TO-LOWER", 0);
    CreateDicEntryC(XT_TEXT_TO_PASCAL, "TEXT-TO-PASCAL", 0);
    CreateDicEntryC(XT_TEXT_TO_INTEGER, "TEXT-TO-INTEGER", 0);
    CreateDicEntryC(XT_DRAW_LINE_3D, "DRAW-LINE-3D", 0);
    CreateDicEntryC(XT_DRAW_POINT_3D, "DRAW-POINT-3D", 0);
    CreateDicEntryC(XT_DRAW_CIRCLE_3D, "DRAW-CIRCLE-3D", 0);
    CreateDicEntryC(XT_DRAW_TRIANGLE_3D, "DRAW-TRIANGLE-3D", 0);
    CreateDicEntryC(XT_DRAW_TRIANGLE_STRIP_3D, "DRAW-TRIANGLE-STRIP-3D", 0);
    CreateDicEntryC(XT_DRAW_CUBE, "DRAW-CUBE", 0);
    CreateDicEntryC(XT_DRAW_CUBE_V, "DRAW-CUBE-V", 0);
    CreateDicEntryC(XT_DRAW_CUBE_WIRES, "DRAW-CUBE-WIRES", 0);
    CreateDicEntryC(XT_DRAW_CUBE_WIRES_V, "DRAW-CUBE-WIRES-V", 0);
    CreateDicEntryC(XT_DRAW_SPHERE, "DRAW-SPHERE", 0);
    CreateDicEntryC(XT_DRAW_SPHERE_EX, "DRAW-SPHERE-EX", 0);
    CreateDicEntryC(XT_DRAW_SPHERE_WIRES, "DRAW-SPHERE-WIRES", 0);
    CreateDicEntryC(XT_DRAW_CYLINDER, "DRAW-CYLINDER", 0);
    CreateDicEntryC(XT_DRAW_CYLINDER_EX, "DRAW-CYLINDER-EX", 0);
    CreateDicEntryC(XT_DRAW_CYLINDER_WIRES, "DRAW-CYLINDER-WIRES", 0);
    CreateDicEntryC(XT_DRAW_CYLINDER_WIRES_EX, "DRAW-CYLINDER-WIRES-EX", 0);
    CreateDicEntryC(XT_DRAW_CAPSULE, "DRAW-CAPSULE", 0);
    CreateDicEntryC(XT_DRAW_CAPSULE_WIRES, "DRAW-CAPSULE-WIRES", 0);
    CreateDicEntryC(XT_DRAW_PLANE, "DRAW-PLANE", 0);
    CreateDicEntryC(XT_DRAW_RAY, "DRAW-RAY", 0);
    CreateDicEntryC(XT_DRAW_GRID, "DRAW-GRID", 0);
    CreateDicEntryC(XT_LOAD_MODEL, "LOAD-MODEL", 0);
    CreateDicEntryC(XT_LOAD_MODEL_FROM_MESH, "LOAD-MODEL-FROM-MESH", 0);
    CreateDicEntryC(XT_IS_MODEL_READY, "IS-MODEL-READY", 0);
    CreateDicEntryC(XT_UNLOAD_MODEL, "UNLOAD-MODEL", 0);
    CreateDicEntryC(XT_GET_MODEL_BOUNDING_BOX, "GET-MODEL-BOUNDING-BOX", 0);
    CreateDicEntryC(XT_DRAW_MODEL, "DRAW-MODEL", 0);
    CreateDicEntryC(XT_DRAW_MODEL_EX, "DRAW-MODEL-EX", 0);
    CreateDicEntryC(XT_DRAW_MODEL_WIRES, "DRAW-MODEL-WIRES", 0);
    CreateDicEntryC(XT_DRAW_MODEL_WIRES_EX, "DRAW-MODEL-WIRES-EX", 0);
    CreateDicEntryC(XT_DRAW_BOUNDING_BOX, "DRAW-BOUNDING-BOX", 0);
    CreateDicEntryC(XT_DRAW_BILLBOARD, "DRAW-BILLBOARD", 0);
    CreateDicEntryC(XT_DRAW_BILLBOARD_REC, "DRAW-BILLBOARD-REC", 0);
    CreateDicEntryC(XT_DRAW_BILLBOARD_PRO, "DRAW-BILLBOARD-PRO", 0);
    CreateDicEntryC(XT_UPLOAD_MESH, "UPLOAD-MESH", 0);
    CreateDicEntryC(XT_UPDATE_MESH_BUFFER, "UPDATE-MESH-BUFFER", 0);
    CreateDicEntryC(XT_UNLOAD_MESH, "UNLOAD-MESH", 0);
    CreateDicEntryC(XT_DRAW_MESH, "DRAW-MESH", 0);
    CreateDicEntryC(XT_DRAW_MESH_INSTANCED, "DRAW-MESH-INSTANCED", 0);
    CreateDicEntryC(XT_EXPORT_MESH, "EXPORT-MESH", 0);
    CreateDicEntryC(XT_GET_MESH_BOUNDING_BOX, "GET-MESH-BOUNDING-BOX", 0);
    CreateDicEntryC(XT_GEN_MESH_TANGENTS, "GEN-MESH-TANGENTS", 0);
    CreateDicEntryC(XT_GEN_MESH_POLY, "GEN-MESH-POLY", 0);
    CreateDicEntryC(XT_GEN_MESH_PLANE, "GEN-MESH-PLANE", 0);
    CreateDicEntryC(XT_GEN_MESH_CUBE, "GEN-MESH-CUBE", 0);
    CreateDicEntryC(XT_GEN_MESH_SPHERE, "GEN-MESH-SPHERE", 0);
    CreateDicEntryC(XT_GEN_MESH_HEMI_SPHERE, "GEN-MESH-HEMI-SPHERE", 0);
    CreateDicEntryC(XT_GEN_MESH_CYLINDER, "GEN-MESH-CYLINDER", 0);
    CreateDicEntryC(XT_GEN_MESH_CONE, "GEN-MESH-CONE", 0);
    CreateDicEntryC(XT_GEN_MESH_TORUS, "GEN-MESH-TORUS", 0);
    CreateDicEntryC(XT_GEN_MESH_KNOT, "GEN-MESH-KNOT", 0);
    CreateDicEntryC(XT_GEN_MESH_HEIGHTMAP, "GEN-MESH-HEIGHTMAP", 0);
    CreateDicEntryC(XT_GEN_MESH_CUBICMAP, "GEN-MESH-CUBICMAP", 0);
    CreateDicEntryC(XT_LOAD_MATERIALS, "LOAD-MATERIALS", 0);
    CreateDicEntryC(XT_LOAD_MATERIAL_DEFAULT, "LOAD-MATERIAL-DEFAULT", 0);
    CreateDicEntryC(XT_IS_MATERIAL_READY, "IS-MATERIAL-READY", 0);
    CreateDicEntryC(XT_UNLOAD_MATERIAL, "UNLOAD-MATERIAL", 0);
    CreateDicEntryC(XT_SET_MATERIAL_TEXTURE, "SET-MATERIAL-TEXTURE", 0);
    CreateDicEntryC(XT_SET_MODEL_MESH_MATERIAL, "SET-MODEL-MESH-MATERIAL", 0);
    CreateDicEntryC(XT_LOAD_MODEL_ANIMATIONS, "LOAD-MODEL-ANIMATIONS", 0);
    CreateDicEntryC(XT_UPDATE_MODEL_ANIMATION, "UPDATE-MODEL-ANIMATION", 0);
    CreateDicEntryC(XT_INIT_AUDIO_DEVICE, "INIT-AUDIO-DEVICE", 0);
    CreateDicEntryC(XT_CLOSE_AUDIO_DEVICE, "CLOSE-AUDIO-DEVICE", 0);
    CreateDicEntryC(XT_IS_AUDIO_DEVICE_READY, "IS-AUDIO-DEVICE-READY", 0);
    CreateDicEntryC(XT_SET_MASTER_VOLUME, "SET-MASTER-VOLUME", 0);
    CreateDicEntryC(XT_GET_MASTER_VOLUME, "GET-MASTER-VOLUME", 0);
    CreateDicEntryC(XT_LOAD_WAVE, "LOAD-WAVE", 0);
    CreateDicEntryC(XT_LOAD_WAVE_FROM_MEMORY, "LOAD-WAVE-FROM-MEMORY", 0);
    CreateDicEntryC(XT_IS_WAVE_READY, "IS-WAVE-READY", 0);
    CreateDicEntryC(XT_LOAD_SOUND, "LOAD-SOUND", 0);
    CreateDicEntryC(XT_LOAD_SOUND_FROM_WAVE, "LOAD-SOUND-FROM-WAVE", 0);
    CreateDicEntryC(XT_LOAD_SOUND_ALIAS, "LOAD-SOUND-ALIAS", 0);
    CreateDicEntryC(XT_IS_SOUND_READY, "IS-SOUND-READY", 0);
    CreateDicEntryC(XT_UPDATE_SOUND, "UPDATE-SOUND", 0);
    CreateDicEntryC(XT_UNLOAD_WAVE, "UNLOAD-WAVE", 0);
    CreateDicEntryC(XT_UNLOAD_SOUND, "UNLOAD-SOUND", 0);
    CreateDicEntryC(XT_UNLOAD_SOUND_ALIAS, "UNLOAD-SOUND-ALIAS", 0);
    CreateDicEntryC(XT_EXPORT_WAVE, "EXPORT-WAVE", 0);
    CreateDicEntryC(XT_EXPORT_WAVE_AS_CODE, "EXPORT-WAVE-AS-CODE", 0);
    CreateDicEntryC(XT_PLAY_SOUND, "PLAY-SOUND", 0);
    CreateDicEntryC(XT_STOP_SOUND, "STOP-SOUND", 0);
    CreateDicEntryC(XT_PAUSE_SOUND, "PAUSE-SOUND", 0);
    CreateDicEntryC(XT_RESUME_SOUND, "RESUME-SOUND", 0);
    CreateDicEntryC(XT_IS_SOUND_PLAYING, "IS-SOUND-PLAYING", 0);
    CreateDicEntryC(XT_SET_SOUND_VOLUME, "SET-SOUND-VOLUME", 0);
    CreateDicEntryC(XT_SET_SOUND_PITCH, "SET-SOUND-PITCH", 0);
    CreateDicEntryC(XT_SET_SOUND_PAN, "SET-SOUND-PAN", 0);
    CreateDicEntryC(XT_WAVE_COPY, "WAVE-COPY", 0);
    CreateDicEntryC(XT_WAVE_CROP, "WAVE-CROP", 0);
    CreateDicEntryC(XT_WAVE_FORMAT, "WAVE-FORMAT", 0);
    CreateDicEntryC(XT_LOAD_WAVE_SAMPLES, "LOAD-WAVE-SAMPLES", 0);
    CreateDicEntryC(XT_UNLOAD_WAVE_SAMPLES, "UNLOAD-WAVE-SAMPLES", 0);
    CreateDicEntryC(XT_LOAD_MUSIC_STREAM, "LOAD-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_LOAD_MUSIC_STREAM_FROM_MEMORY, "LOAD-MUSIC-STREAM-FROM-MEMORY", 0);
    CreateDicEntryC(XT_IS_MUSIC_READY, "IS-MUSIC-READY", 0);
    CreateDicEntryC(XT_UNLOAD_MUSIC_STREAM, "UNLOAD-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_PLAY_MUSIC_STREAM, "PLAY-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_IS_MUSIC_STREAM_PLAYING, "IS-MUSIC-STREAM-PLAYING", 0);
    CreateDicEntryC(XT_UPDATE_MUSIC_STREAM, "UPDATE-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_STOP_MUSIC_STREAM, "STOP-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_PAUSE_MUSIC_STREAM, "PAUSE-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_RESUME_MUSIC_STREAM, "RESUME-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_SEEK_MUSIC_STREAM, "SEEK-MUSIC-STREAM", 0);
    CreateDicEntryC(XT_SET_MUSIC_VOLUME, "SET-MUSIC-VOLUME", 0);
    CreateDicEntryC(XT_SET_MUSIC_PITCH, "SET-MUSIC-PITCH", 0);
    CreateDicEntryC(XT_SET_MUSIC_PAN, "SET-MUSIC-PAN", 0);
    CreateDicEntryC(XT_GET_MUSIC_TIME_LENGTH, "GET-MUSIC-TIME-LENGTH", 0);
    CreateDicEntryC(XT_GET_MUSIC_TIME_PLAYED, "GET-MUSIC-TIME-PLAYED", 0);
    CreateDicEntryC(XT_LOAD_AUDIO_STREAM, "LOAD-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_IS_AUDIO_STREAM_READY, "IS-AUDIO-STREAM-READY", 0);
    CreateDicEntryC(XT_UNLOAD_AUDIO_STREAM, "UNLOAD-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_UPDATE_AUDIO_STREAM, "UPDATE-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_IS_AUDIO_STREAM_PROCESSED, "IS-AUDIO-STREAM-PROCESSED", 0);
    CreateDicEntryC(XT_PLAY_AUDIO_STREAM, "PLAY-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_PAUSE_AUDIO_STREAM, "PAUSE-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_RESUME_AUDIO_STREAM, "RESUME-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_IS_AUDIO_STREAM_PLAYING, "IS-AUDIO-STREAM-PLAYING", 0);
    CreateDicEntryC(XT_STOP_AUDIO_STREAM, "STOP-AUDIO-STREAM", 0);
    CreateDicEntryC(XT_SET_AUDIO_STREAM_VOLUME, "SET-AUDIO-STREAM-VOLUME", 0);
    CreateDicEntryC(XT_SET_AUDIO_STREAM_PITCH, "SET-AUDIO-STREAM-PITCH", 0);
    CreateDicEntryC(XT_SET_AUDIO_STREAM_PAN, "SET-AUDIO-STREAM-PAN", 0);
    CreateDicEntryC(XT_SET_AUDIO_STREAM_BUFFER_SIZE_DEFAULT, "SET-AUDIO-STREAM-BUFFER-SIZE-DEFAULT", 0);
    CreateDicEntryC(XT_SET_AUDIO_STREAM_CALLBACK, "SET-AUDIO-STREAM-CALLBACK", 0);
    CreateDicEntryC(XT_ATTACH_AUDIO_STREAM_PROCESSOR, "ATTACH-AUDIO-STREAM-PROCESSOR", 0);
    CreateDicEntryC(XT_DETACH_AUDIO_STREAM_PROCESSOR, "DETACH-AUDIO-STREAM-PROCESSOR", 0);
    CreateDicEntryC(XT_ATTACH_AUDIO_MIXED_PROCESSOR, "ATTACH-AUDIO-MIXED-PROCESSOR", 0);
    CreateDicEntryC(XT_DETACH_AUDIO_MIXED_PROCESSOR, "DETACH-AUDIO-MIXED-PROCESSOR", 0);
    //
    // Structs and access methods
    CreateDicEntryC(XT_TEXTURE_GET_WIDTH, "TEXTURE-GET-WIDTH", 0);
    CreateDicEntryC(XT_TEXTURE_GET_HEIGHT, "TEXTURE-GET-HEIGHT", 0);
    // End raylib words
    //****************************************************************


    pfDebugMessage("pfBuildDictionary: FindSpecialXTs\n");
    if( FindSpecialXTs() < 0 ) goto error;

    if( CompileCustomFunctions() < 0 ) goto error; /* Call custom 'C' call builder. */

#ifdef PF_DEBUG
    DumpMemory( (void *)dic->dic_HeaderBase, 256 );
    DumpMemory( (void *)dic->dic_CodeBase, 256 );
#endif

    pfDebugMessage("pfBuildDictionary: Finished adding dictionary entries.\n");
    return (PForthDictionary) dic;

error:
    pfDebugMessage("pfBuildDictionary: Error adding dictionary entries.\n");
    pfDeleteDictionary( dic );
    return NULL;

nomem:
    return NULL;
}
#endif /* !PF_NO_INIT */

/*
** ( xt -- nfa 1 , x 0 , find NFA in dictionary from XT )
** 1 for IMMEDIATE values
*/
cell_t ffTokenToName( ExecToken XT, const ForthString **NFAPtr )
{
    const ForthString *NameField;
    cell_t Searching = TRUE;
    cell_t Result = 0;
    ExecToken TempXT;

    NameField = (ForthString *) gVarContext;
DBUGX(("\ffCodeToName: gVarContext = 0x%x\n", gVarContext));

    do
    {
        TempXT = NameToToken( NameField );

        if( TempXT == XT )
        {
DBUGX(("ffCodeToName: NFA = 0x%x\n", NameField));
            *NFAPtr = NameField ;
            Result = 1;
            Searching = FALSE;
        }
        else
        {
            NameField = NameToPrevious( NameField );
            if( NameField == NULL )
            {
                *NFAPtr = 0;
                Searching = FALSE;
            }
        }
    } while ( Searching);

    return Result;
}

/*
** ( $name -- $addr 0 | nfa -1 | nfa 1 , find NFA in dictionary )
** 1 for IMMEDIATE values
*/
cell_t ffFindNFA( const ForthString *WordName, const ForthString **NFAPtr )
{
    const ForthString *WordChar;
    uint8_t WordLen;
    const char *NameField, *NameChar;
    int8_t NameLen;
    cell_t Searching = TRUE;
    cell_t Result = 0;

    WordLen = (uint8_t) ((ucell_t)*WordName & 0x1F);
    WordChar = WordName+1;

    NameField = (ForthString *) gVarContext;
DBUG(("\nffFindNFA: WordLen = %d, WordName = %*s\n", WordLen, WordLen, WordChar ));
DBUG(("\nffFindNFA: gVarContext = 0x%x\n", gVarContext));
    do
    {
        NameLen = (uint8_t) ((ucell_t)(*NameField) & MASK_NAME_SIZE);
        NameChar = NameField+1;
/* DBUG(("   %c\n", (*NameField & FLAG_SMUDGE) ? 'S' : 'V' )); */
        if( ((*NameField & FLAG_SMUDGE) == 0) &&
            (NameLen == WordLen) &&
            ffCompareTextCaseN( NameChar, WordChar, WordLen ) ) /* FIXME - slow */
        {
DBUG(("ffFindNFA: found it at NFA = 0x%x\n", NameField));
            *NFAPtr = NameField ;
            Result = ((*NameField) & FLAG_IMMEDIATE) ? 1 : -1;
            Searching = FALSE;
        }
        else
        {
            NameField = NameToPrevious( NameField );
            if( NameField == NULL )
            {
                *NFAPtr = WordName;
                Searching = FALSE;
            }
        }
    } while ( Searching);
DBUG(("ffFindNFA: returns 0x%x\n", Result));
    return Result;
}


/***************************************************************
** ( $name -- $name 0 | xt -1 | xt 1 )
** 1 for IMMEDIATE values
*/
cell_t ffFind( const ForthString *WordName, ExecToken *pXT )
{
    const ForthString *NFA;
    cell_t Result;

    Result = ffFindNFA( WordName, &NFA );
DBUG(("ffFind: %8s at 0x%x\n", WordName+1, NFA)); /* WARNING, not NUL terminated. %Q */
    if( Result )
    {
        *pXT = NameToToken( NFA );
    }
    else
    {
        *pXT = (ExecToken) WordName;
    }

    return Result;
}

/****************************************************************
** Find name when passed 'C' string.
*/
cell_t ffFindC( const char *WordName, ExecToken *pXT )
{
DBUG(("ffFindC: %s\n", WordName ));
    CStringToForth( gScratch, WordName, sizeof(gScratch) );
    return ffFind( gScratch, pXT );
}


/***********************************************************/
/********* Compiling New Words *****************************/
/***********************************************************/
#define DIC_SAFETY_MARGIN  (400)

/*************************************************************
**  Check for dictionary overflow.
*/
static cell_t ffCheckDicRoom( void )
{
    cell_t RoomLeft;
    RoomLeft = (char *)gCurrentDictionary->dic_HeaderLimit -
           (char *)gCurrentDictionary->dic_HeaderPtr;
    if( RoomLeft < DIC_SAFETY_MARGIN )
    {
        pfReportError("ffCheckDicRoom", PF_ERR_HEADER_ROOM);
        return PF_ERR_HEADER_ROOM;
    }

    RoomLeft = (char *)gCurrentDictionary->dic_CodeLimit -
               (char *)gCurrentDictionary->dic_CodePtr.Byte;
    if( RoomLeft < DIC_SAFETY_MARGIN )
    {
        pfReportError("ffCheckDicRoom", PF_ERR_CODE_ROOM);
        return PF_ERR_CODE_ROOM;
    }
    return 0;
}

/*************************************************************
**  Create a dictionary entry given a string name.
*/
void ffCreateSecondaryHeader( const ForthStringPtr FName)
{
    pfDebugMessage("ffCreateSecondaryHeader()\n");
/* Check for dictionary overflow. */
    if( ffCheckDicRoom() ) return;

    pfDebugMessage("ffCreateSecondaryHeader: CheckRedefinition()\n");
    CheckRedefinition( FName );
/* Align CODE_HERE */
    CODE_HERE = (cell_t *)( (((ucell_t)CODE_HERE) + UINT32_MASK) & ~UINT32_MASK);
    CreateDicEntry( (ExecToken) ABS_TO_CODEREL(CODE_HERE), FName, FLAG_SMUDGE );
}

/*************************************************************
** Begin compiling a secondary word.
*/
static void ffStringColon( const ForthStringPtr FName)
{
    ffCreateSecondaryHeader( FName );
    gVarState = 1;
}

/*************************************************************
** Read the next ExecToken from the Source and create a word.
*/
void ffColon( void )
{
    char *FName;

    gDepthAtColon = DATA_STACK_DEPTH;

    FName = ffWord( SPACE_CHARACTER );
    if( *FName > 0 )
    {
        ffStringColon( FName );
    }
}

/*************************************************************
** Check to see if name is already in dictionary.
*/
static cell_t CheckRedefinition( const ForthStringPtr FName )
{
    cell_t flag;
    ExecToken XT;

    flag = ffFind( FName, &XT);
    if ( flag && !gVarQuiet)
    {
        ioType( FName+1, (cell_t) *FName );
        MSG( " redefined.\n" ); /* FIXME - allow user to run off this warning. */
    }
    return flag;
}

void ffStringCreate( char *FName)
{
    ffCreateSecondaryHeader( FName );

    CODE_COMMA( ID_CREATE_P );
    CODE_COMMA( ID_EXIT );
    ffFinishSecondary();

}

/* Read the next ExecToken from the Source and create a word. */
void ffCreate( void )
{
    char *FName;

    FName = ffWord( SPACE_CHARACTER );
    if( *FName > 0 )
    {
        ffStringCreate( FName );
    }
}

void ffStringDefer( const ForthStringPtr FName, ExecToken DefaultXT )
{
    pfDebugMessage("ffStringDefer()\n");
    ffCreateSecondaryHeader( FName );

    CODE_COMMA( ID_DEFER_P );
    CODE_COMMA( DefaultXT );

    ffFinishSecondary();

}
#ifndef PF_NO_INIT
/* Convert name then create deferred dictionary entry. */
static void CreateDeferredC( ExecToken DefaultXT, const char *CName )
{
    char FName[40];
    CStringToForth( FName, CName, sizeof(FName) );
    ffStringDefer( FName, DefaultXT );
}
#endif

/* Read the next token from the Source and create a word. */
void ffDefer( void )
{
    char *FName;

    FName = ffWord( SPACE_CHARACTER );
    if( *FName > 0 )
    {
        ffStringDefer( FName, ID_QUIT_P );
    }
}

/* Unsmudge the word to make it visible. */
static void ffUnSmudge( void )
{
    *(char*)gVarContext &= ~FLAG_SMUDGE;
}

/* Implement ; */
ThrowCode ffSemiColon( void )
{
    ThrowCode exception = 0;
    gVarState = 0;

    if( (gDepthAtColon != DATA_STACK_DEPTH) &&
        (gDepthAtColon != DEPTH_AT_COLON_INVALID) ) /* Ignore if no ':' */
    {
        exception = THROW_SEMICOLON;
    }
    else
    {
        ffFinishSecondary();
    }
    gDepthAtColon = DEPTH_AT_COLON_INVALID;
    return exception;
}

/* Finish the definition of a Forth word. */
void ffFinishSecondary( void )
{
    CODE_COMMA( ID_EXIT );
    ffUnSmudge();
}

/**************************************************************/
/* Used to pull a number from the dictionary to the stack */
void ff2Literal( cell_t dHi, cell_t dLo )
{
    CODE_COMMA( ID_2LITERAL_P );
    CODE_COMMA( dHi );
    CODE_COMMA( dLo );
}
void ffALiteral( cell_t Num )
{
    CODE_COMMA( ID_ALITERAL_P );
    CODE_COMMA( Num );
}
void ffLiteral( cell_t Num )
{
    CODE_COMMA( ID_LITERAL_P );
    CODE_COMMA( Num );
}

#ifdef PF_SUPPORT_FP
void ffFPLiteral( PF_FLOAT fnum )
{
    /* Hack for Metrowerks compiler which won't compile the
     * original expression.
     */
    PF_FLOAT  *temp;
    cell_t    *dicPtr;

/* Make sure that literal float data is float aligned. */
    dicPtr = CODE_HERE + 1;
    while( (((ucell_t) dicPtr++) & (sizeof(PF_FLOAT) - 1)) != 0)
    {
        DBUG((" comma NOOP to align FPLiteral\n"));
        CODE_COMMA( ID_NOOP );
    }
    CODE_COMMA( ID_FP_FLITERAL_P );

    temp = (PF_FLOAT *)CODE_HERE;
    WRITE_FLOAT_DIC(temp,fnum);   /* Write to dictionary. */
    temp++;
    CODE_HERE = (cell_t *) temp;
}
#endif /* PF_SUPPORT_FP */

/**************************************************************/
static ThrowCode FindAndCompile( const char *theWord )
{
    cell_t Flag;
    ExecToken XT;
    cell_t Num;
    ThrowCode exception = 0;

    Flag = ffFind( theWord, &XT);
DBUG(("FindAndCompile: theWord = %8s, XT = 0x%x, Flag = %d\n", theWord, XT, Flag ));

/* Is it a normal word ? */
    if( Flag == -1 )
    {
        if( gVarState )  /* compiling? */
        {
            CODE_COMMA( XT );
        }
        else
        {
            exception = pfCatch( XT );
        }
    }
    else if ( Flag == 1 ) /* or is it IMMEDIATE ? */
    {
DBUG(("FindAndCompile: IMMEDIATE, theWord = 0x%x\n", theWord ));
        exception = pfCatch( XT );
    }
    else /* try to interpret it as a number. */
    {
/* Call deferred NUMBER? */
        cell_t NumResult;

DBUG(("FindAndCompile: not found, try number?\n" ));
        PUSH_DATA_STACK( theWord );   /* Push text of number */
        exception = pfCatch( gNumberQ_XT );
        if( exception ) goto error;

DBUG(("FindAndCompile: after number?\n" ));
        NumResult = POP_DATA_STACK;  /* Success? */
        switch( NumResult )
        {
        case NUM_TYPE_SINGLE:
            if( gVarState )  /* compiling? */
            {
                Num = POP_DATA_STACK;
                ffLiteral( Num );
            }
            break;

        case NUM_TYPE_DOUBLE:
            if( gVarState )  /* compiling? */
            {
                Num = POP_DATA_STACK;  /* get hi portion */
                ff2Literal( Num, POP_DATA_STACK );
            }
            break;

#ifdef PF_SUPPORT_FP
        case NUM_TYPE_FLOAT:
            if( gVarState )  /* compiling? */
            {
                ffFPLiteral( *gCurrentTask->td_FloatStackPtr++ );
            }
            break;
#endif

        case NUM_TYPE_BAD:
        default:
            ioType( theWord+1, *theWord );
            MSG( "  ? - unrecognized word!\n" );
            exception = THROW_UNDEFINED_WORD;
            break;

        }
    }
error:
    return exception;
}

/**************************************************************
** Forth outer interpreter.  Parses words from Source.
** Executes them or compiles them based on STATE.
*/
ThrowCode ffInterpret( void )
{
    cell_t flag;
    char *theWord;
    ThrowCode exception = 0;

/* Is there any text left in Source ? */
    while( gCurrentTask->td_IN < (gCurrentTask->td_SourceNum) )
    {

        pfDebugMessage("ffInterpret: calling ffWord(()\n");
        theWord = ffLWord( SPACE_CHARACTER );
        DBUG(("ffInterpret: theWord = 0x%x, Len = %d\n", theWord, *theWord ));

        if( *theWord > 0 )
        {
            flag = 0;
            if( gLocalCompiler_XT )
            {
                PUSH_DATA_STACK( theWord );   /* Push word. */
                exception = pfCatch( gLocalCompiler_XT );
                if( exception ) goto error;
                flag = POP_DATA_STACK;  /* Compiled local? */
            }
            if( flag == 0 )
            {
                exception = FindAndCompile( theWord );
                if( exception ) goto error;
            }
        }

        DBUG(("ffInterpret: IN=%d, SourceNum=%d\n", gCurrentTask->td_IN,
            gCurrentTask->td_SourceNum ) );
    }
error:
    return exception;
}

/**************************************************************/
ThrowCode ffOK( void )
{
    cell_t exception = 0;
/* Check for stack underflow.   %Q what about overflows? */
    if( (gCurrentTask->td_StackBase - gCurrentTask->td_StackPtr) < 0 )
    {
        exception = THROW_STACK_UNDERFLOW;
    }
#ifdef PF_SUPPORT_FP  /* Check floating point stack too! */
    else if((gCurrentTask->td_FloatStackBase - gCurrentTask->td_FloatStackPtr) < 0)
    {
        exception = THROW_FLOAT_STACK_UNDERFLOW;
    }
#endif
    else if( gCurrentTask->td_InputStream == PF_STDIN)
    {
        if( !gVarState )  /* executing? */
        {
            if( !gVarQuiet )
            {
                MSG( "   ok\n" );
                if(gVarTraceStack) ffDotS();
            }
            else
            {
                EMIT_CR;
            }
        }
    }
    return exception;
}

/***************************************************************
** Cleanup Include stack by popping and closing files.
***************************************************************/
void pfHandleIncludeError( void )
{
    FileStream *cur;

    while( (cur = ffPopInputStream()) != PF_STDIN)
    {
        DBUG(("ffCleanIncludeStack: closing 0x%x\n", cur ));
        sdCloseFile(cur);
    }
}

/***************************************************************
** Interpret input in a loop.
***************************************************************/
ThrowCode ffOuterInterpreterLoop( void )
{
    cell_t exception = 0;
    do
    {
        exception = ffRefill();
        if(exception <= 0) break;

        exception = ffInterpret();
        if( exception == 0 )
        {
            exception = ffOK();
        }

    } while( exception == 0 );
    return exception;
}

/***************************************************************
** Include then close a file
***************************************************************/

ThrowCode ffIncludeFile( FileStream *InputFile )
{
    ThrowCode exception;

/* Push file stream. */
    exception = ffPushInputStream( InputFile );
    if( exception < 0 ) return exception;

/* Run outer interpreter for stream. */
    exception = ffOuterInterpreterLoop();
    if( exception )
    {
        int i;
/* Report line number and nesting level. */
        MSG("INCLUDE error on line #"); ffDot(gCurrentTask->td_LineNumber);
        MSG(", level = ");  ffDot(gIncludeIndex );
        EMIT_CR

/* Dump line of error and show offset in line for >IN */
        for( i=0; i<gCurrentTask->td_SourceNum; i++ )
        {
            char c = gCurrentTask->td_SourcePtr[i];
            if( c == '\t' ) c = ' ';
            EMIT(c);
        }
        EMIT_CR;
        for( i=0; i<(gCurrentTask->td_IN - 1); i++ ) EMIT('^');
        EMIT_CR;
    }

/* Pop file stream. */
    ffPopInputStream();

/* ANSI spec specifies that this should also close the file. */
    sdCloseFile(InputFile);

    return exception;
}

#endif /* !PF_NO_SHELL */

/***************************************************************
** Save current input stream on stack, use this new one.
***************************************************************/
Err ffPushInputStream( FileStream *InputFile )
{
    Err Result = 0;
    IncludeFrame *inf;

/* Push current input state onto special include stack. */
    if( gIncludeIndex < MAX_INCLUDE_DEPTH )
    {
        inf = &gIncludeStack[gIncludeIndex++];
        inf->inf_FileID = gCurrentTask->td_InputStream;
        inf->inf_IN = gCurrentTask->td_IN;
        inf->inf_LineNumber = gCurrentTask->td_LineNumber;
        inf->inf_SourceNum = gCurrentTask->td_SourceNum;
/* Copy TIB plus any NUL terminator into saved area. */
        if( (inf->inf_SourceNum > 0) && (inf->inf_SourceNum < (TIB_SIZE-1)) )
        {
            pfCopyMemory( inf->inf_SaveTIB, gCurrentTask->td_TIB, inf->inf_SourceNum+1 );
        }

/* Set new current input. */
        DBUG(( "ffPushInputStream: InputFile = 0x%x\n", InputFile ));
        gCurrentTask->td_InputStream = InputFile;
        gCurrentTask->td_LineNumber = 0;
    }
    else
    {
        ERR("ffPushInputStream: max depth exceeded.\n");
        return -1;
    }


    return Result;
}

/***************************************************************
** Go back to reading previous stream.
** Just return gCurrentTask->td_InputStream upon underflow.
***************************************************************/
FileStream *ffPopInputStream( void )
{
    IncludeFrame *inf;
    FileStream *Result;

DBUG(("ffPopInputStream: gIncludeIndex = %d\n", gIncludeIndex));
    Result = gCurrentTask->td_InputStream;

/* Restore input state. */
    if( gIncludeIndex > 0 )
    {
        inf = &gIncludeStack[--gIncludeIndex];
        gCurrentTask->td_InputStream = inf->inf_FileID;
        DBUG(("ffPopInputStream: stream = 0x%x\n", gCurrentTask->td_InputStream ));
        gCurrentTask->td_IN = inf->inf_IN;
        gCurrentTask->td_LineNumber = inf->inf_LineNumber;
        gCurrentTask->td_SourceNum = inf->inf_SourceNum;
/* Copy TIB plus any NUL terminator into saved area. */
        if( (inf->inf_SourceNum > 0) && (inf->inf_SourceNum < (TIB_SIZE-1)) )
        {
            pfCopyMemory( gCurrentTask->td_TIB, inf->inf_SaveTIB, inf->inf_SourceNum+1 );
        }

    }
DBUG(("ffPopInputStream: return = 0x%x\n", Result ));

    return Result;
}

/***************************************************************
** Convert file pointer to value consistent with SOURCE-ID.
***************************************************************/
cell_t ffConvertStreamToSourceID( FileStream *Stream )
{
    cell_t Result;
    if(Stream == PF_STDIN)
    {
        Result = 0;
    }
    else if(Stream == NULL)
    {
        Result = -1;
    }
    else
    {
        Result = (cell_t) Stream;
    }
    return Result;
}

/***************************************************************
** Convert file pointer to value consistent with SOURCE-ID.
***************************************************************/
FileStream * ffConvertSourceIDToStream( cell_t id )
{
    FileStream *stream;

    if( id == 0 )
    {
        stream = PF_STDIN;
    }
    else if( id == -1 )
    {
        stream = NULL;
    }
    else
    {
        stream = (FileStream *) id;
    }
    return stream;
}

/**************************************************************
** Receive line from input stream.
** Return length, or -1 for EOF.
*/
#define BACKSPACE  (8)
static cell_t readLineFromStream( char *buffer, cell_t maxChars, FileStream *stream )
{
    int   c;
    int   len;
    char *p;
    static int lastChar = 0;
    int   done = 0;

DBUGX(("readLineFromStream(0x%x, 0x%x, 0x%x)\n", buffer, len, stream ));
    p = buffer;
    len = 0;
    while( (len < maxChars) && !done )
    {
        c = sdInputChar(stream);
        switch(c)
        {
            case EOF:
                DBUG(("EOF\n"));
                done = 1;
                if( len <= 0 ) len = -1;
                break;

            case '\n':
                DBUGX(("EOL=\\n\n"));
                if( lastChar != '\r' ) done = 1;
                break;

            case '\r':
                DBUGX(("EOL=\\r\n"));
                done = 1;
                break;

            default:
                *p++ = (char) c;
                len++;
                break;
        }
        lastChar = c;
    }

/* NUL terminate line to simplify printing when debugging. */
    if( (len >= 0) && (len < maxChars) ) p[len] = '\0';

    return len;
}

/**************************************************************
** ( -- , fill Source from current stream )
** Return 1 if successful, 0 for EOF, or a negative error.
*/
cell_t ffRefill( void )
{
    cell_t Num;
    cell_t Result = 1;

/* reset >IN for parser */
    gCurrentTask->td_IN = 0;

/* get line from current stream */
    if( gCurrentTask->td_InputStream == PF_STDIN )
    {
    /* ACCEPT is deferred so we call it through the dictionary. */
        ThrowCode throwCode;
        PUSH_DATA_STACK( gCurrentTask->td_SourcePtr );
        PUSH_DATA_STACK( TIB_SIZE );
        throwCode = pfCatch( gAcceptP_XT );
        if (throwCode) {
            Result = throwCode;
            goto error;
        }
        Num = POP_DATA_STACK;
        if( Num < 0 )
        {
            Result = Num;
            goto error;
        }
    }
    else
    {
        Num = readLineFromStream( gCurrentTask->td_SourcePtr, TIB_SIZE,
            gCurrentTask->td_InputStream );
        if( Num == EOF )
        {
            Result = 0;
            Num = 0;
        }
    }

    gCurrentTask->td_SourceNum = Num;
    gCurrentTask->td_LineNumber++;  /* Bump for include. */

/* echo input if requested */
    if( gVarEcho && ( Num > 0))
    {
        ioType( gCurrentTask->td_SourcePtr, gCurrentTask->td_SourceNum );
        EMIT_CR;
    }

error:
    return Result;
}
