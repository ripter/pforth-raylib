/* @(#) pf_inner.c 98/03/16 1.7 */
/***************************************************************
** Inner Interpreter for Forth based on 'C'
**
** Author: Phil Burk
** Modified by Chris Richards 2024.
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
**
** 940502 PLB Creation.
** 940505 PLB More macros.
** 940509 PLB Moved all stack stuff into pfCatch.
** 941014 PLB Converted to flat secondary strusture.
** 941027 rdg added casts to ID_SP_FETCH, ID_RP_FETCH,
**             and ID_HERE for armcc
** 941130 PLB Made w@ unsigned
**
***************************************************************/

#include "pf_all.h"

#if defined(WIN32) && !defined(__MINGW32__)
#include <crtdbg.h>
#endif

#define SYSTEM_LOAD_FILE "system.fth"

/***************************************************************
** Macros for data stack access.
** TOS is cached in a register in pfCatch.
***************************************************************/

#define STKPTR     (DataStackPtr)
#define M_POP      (*(STKPTR++))
#define M_PUSH(n)  {*(--(STKPTR)) = (cell_t) (n);}
#define M_STACK(n) (STKPTR[n])

#define TOS      (TopOfStack)
#define PUSH_TOS M_PUSH(TOS)
#define M_DUP    PUSH_TOS;
#define M_DROP   { TOS = M_POP; }

#define ASCII_EOT   (0x04)

/***************************************************************
** Macros for Floating Point stack access.
***************************************************************/
#ifdef PF_SUPPORT_FP
#define FP_STKPTR   (FloatStackPtr)
#define M_FP_SPZERO (gCurrentTask->td_FloatStackBase)
#define M_FP_POP    (*(FP_STKPTR++))
#define M_FP_PUSH(n) {*(--(FP_STKPTR)) = (PF_FLOAT) (n);}
#define M_FP_STACK(n) (FP_STKPTR[n])

#define FP_TOS      (fpTopOfStack)
#define PUSH_FP_TOS M_FP_PUSH(FP_TOS)
#define M_FP_DUP    PUSH_FP_TOS;
#define M_FP_DROP   { FP_TOS = M_FP_POP; }
#endif

/***************************************************************
** Macros for return stack access.
***************************************************************/

#define TORPTR (ReturnStackPtr)
#define M_R_DROP {TORPTR++;}
#define M_R_POP (*(TORPTR++))
#define M_R_PICK(n) (TORPTR[n])
#define M_R_PUSH(n) {*(--(TORPTR)) = (cell_t) (n);}

/***************************************************************
** Misc Forth macros
***************************************************************/

#define M_BRANCH   { InsPtr = (cell_t *) (((uint8_t *) InsPtr) + READ_CELL_DIC(InsPtr)); }

/* Cache top of data stack like in JForth. */
#ifdef PF_SUPPORT_FP
#define LOAD_REGISTERS \
    { \
        STKPTR = gCurrentTask->td_StackPtr; \
        TOS = M_POP; \
        FP_STKPTR = gCurrentTask->td_FloatStackPtr; \
        FP_TOS = M_FP_POP; \
        TORPTR = gCurrentTask->td_ReturnPtr; \
     }

#define SAVE_REGISTERS \
    { \
        gCurrentTask->td_ReturnPtr = TORPTR; \
        M_PUSH( TOS ); \
        gCurrentTask->td_StackPtr = STKPTR; \
        M_FP_PUSH( FP_TOS ); \
        gCurrentTask->td_FloatStackPtr = FP_STKPTR; \
     }

#else
/* Cache top of data stack like in JForth. */
#define LOAD_REGISTERS \
    { \
        STKPTR = gCurrentTask->td_StackPtr; \
        TOS = M_POP; \
        TORPTR = gCurrentTask->td_ReturnPtr; \
     }

#define SAVE_REGISTERS \
    { \
        gCurrentTask->td_ReturnPtr = TORPTR; \
        M_PUSH( TOS ); \
        gCurrentTask->td_StackPtr = STKPTR; \
     }
#endif

#define M_DOTS \
    SAVE_REGISTERS; \
    ffDotS( ); \
    LOAD_REGISTERS;

#define DO_VAR(varname) { PUSH_TOS; TOS = (cell_t) &varname; }

#ifdef PF_SUPPORT_FP
#define M_THROW(err) \
    { \
        ExceptionReturnCode = (ThrowCode)(err); \
        TORPTR = InitialReturnStack; /* Will cause return to 'C' */ \
        STKPTR = InitialDataStack; \
        FP_STKPTR = InitialFloatStack; \
    }
#else
#define M_THROW(err) \
    { \
        ExceptionReturnCode = (err); \
        TORPTR = InitialReturnStack; /* Will cause return to 'C' */ \
        STKPTR = InitialDataStack; \
    }
#endif

/***************************************************************
** Other macros
***************************************************************/

#define BINARY_OP( op ) { TOS = M_POP op TOS; }
#define endcase break

#if defined(PF_NO_SHELL) || !defined(PF_SUPPORT_TRACE)
    #define TRACENAMES /* no names */
#else
/* Display name of executing routine. */
static void TraceNames( ExecToken Token, cell_t Level )
{
    char *DebugName;
    cell_t i;

    if( ffTokenToName( Token, &DebugName ) )
    {
        cell_t NumSpaces;
        if( gCurrentTask->td_OUT > 0 ) EMIT_CR;
        EMIT( '>' );
        for( i=0; i<Level; i++ )
        {
            MSG( "  " );
        }
        TypeName( DebugName );
/* Space out to column N then .S */
        NumSpaces = 30 - gCurrentTask->td_OUT;
        for( i=0; i < NumSpaces; i++ )
        {
            EMIT( ' ' );
        }
        ffDotS();
/* No longer needed?        gCurrentTask->td_OUT = 0; */ /* !!! Hack for ffDotS() */

    }
    else
    {
        MSG_NUM_H("Couldn't find Name for ", Token);
    }
}

#define TRACENAMES \
    if( (gVarTraceLevel > Level) ) \
    { SAVE_REGISTERS; TraceNames( Token, Level ); LOAD_REGISTERS; }
#endif /* PF_NO_SHELL */

/* Use local copy of CODE_BASE for speed. */
#define LOCAL_CODEREL_TO_ABS( a ) ((cell_t *) (((cell_t) a) + CodeBase))

/* Truncate the unsigned double cell integer LO/HI to an uint64_t. */
static uint64_t UdToUint64( ucell_t Lo, ucell_t Hi )
{
#if (PF_SIZEOF_CELL == 4)
    return (((uint64_t)Lo) | (((uint64_t)Hi) >> (sizeof(ucell_t) * 8)));
#else
    (void)Hi;
    return Lo;
#endif
}

/* Return TRUE if the unsigned double cell integer LO/HI is not greater
 * then the greatest uint64_t.
 */
static int UdIsUint64( ucell_t Hi )
{
    return (( 2 * sizeof(ucell_t) == sizeof(uint64_t) )
        ? TRUE
        : Hi == 0);
}

static const char *pfSelectFileModeCreate( cell_t fam );
static const char *pfSelectFileModeOpen( cell_t fam );

/**************************************************************/
static const char *pfSelectFileModeCreate( cell_t fam )
{
    const char *famText = NULL;
    switch( fam )
    {
    case (PF_FAM_WRITE_ONLY + PF_FAM_BINARY_FLAG):
        famText = PF_FAM_BIN_CREATE_WO;
        break;
    case (PF_FAM_READ_WRITE + PF_FAM_BINARY_FLAG):
        famText = PF_FAM_BIN_CREATE_RW;
        break;
    case PF_FAM_WRITE_ONLY:
        famText = PF_FAM_CREATE_WO;
        break;
    case PF_FAM_READ_WRITE:
        famText = PF_FAM_CREATE_RW;
        break;
    default:
        famText = "illegal";
        break;
    }
    return famText;
}

/**************************************************************/
static const char *pfSelectFileModeOpen( cell_t fam )
{
    const char *famText = NULL;
    switch( fam )
    {
    case (PF_FAM_READ_ONLY + PF_FAM_BINARY_FLAG):
        famText = PF_FAM_BIN_OPEN_RO;
        break;
    case (PF_FAM_WRITE_ONLY + PF_FAM_BINARY_FLAG):
        famText = PF_FAM_BIN_CREATE_WO;
        break;
    case (PF_FAM_READ_WRITE + PF_FAM_BINARY_FLAG):
        famText = PF_FAM_BIN_OPEN_RW;
        break;
    case PF_FAM_READ_ONLY:
        famText = PF_FAM_OPEN_RO;
        break;
    case PF_FAM_WRITE_ONLY:
        famText = PF_FAM_CREATE_WO;
        break;
    case PF_FAM_READ_WRITE:
    default:
        famText = PF_FAM_OPEN_RW;
        break;
    }
    return famText;
}

/**************************************************************/
ThrowCode pfCatch( ExecToken XT )
{
    cell_t  TopOfStack;    
    register cell_t *DataStackPtr; /* Cache for faster execution. */
    register cell_t *ReturnStackPtr;
    register cell_t *InsPtr = NULL;
    register cell_t  Token;
    cell_t           Scratch;

#ifdef PF_SUPPORT_FP
    PF_FLOAT       fpTopOfStack;
    PF_FLOAT      *FloatStackPtr;
    PF_FLOAT       fpScratch;
    PF_FLOAT       fpTemp;
    PF_FLOAT      *InitialFloatStack;
#endif
#ifdef PF_SUPPORT_TRACE
    cell_t Level = 0;
#endif
    cell_t        *LocalsPtr = NULL;
    cell_t         Temp;
    cell_t        *InitialReturnStack;
    cell_t        *InitialDataStack;
    cell_t         FakeSecondary[2];
    char          *CharPtr;
    cell_t        *CellPtr;
    FileStream    *FileID;
    uint8_t       *CodeBase = (uint8_t *) CODE_BASE;
    ThrowCode      ExceptionReturnCode = 0;
    

/* FIXME
    gExecutionDepth += 1;
    PRT(("pfCatch( 0x%x ), depth = %d\n", XT, gExecutionDepth ));
*/

/*
** Initialize FakeSecondary this way to avoid having stuff in the data section,
** which is not supported for some embedded system loaders.
*/
    FakeSecondary[0] = 0;
    FakeSecondary[1] = ID_EXIT; /* For EXECUTE */

/* Move data from task structure to registers for speed. */
    LOAD_REGISTERS;

/* Save initial stack depths for THROW */
    InitialReturnStack = TORPTR;
    InitialDataStack   = STKPTR ;
#ifdef PF_SUPPORT_FP
    InitialFloatStack  = FP_STKPTR;
#endif

    Token = XT;

    do
    {
DBUG(("pfCatch: Token = 0x%x\n", Token ));

/* --------------------------------------------------------------- */
/* If secondary, thread down code tree until we hit a primitive. */
        while( !IsTokenPrimitive( Token ) )
        {
#ifdef PF_SUPPORT_TRACE
            if((gVarTraceFlags & TRACE_INNER) )
            {
                MSG("pfCatch: Secondary Token = 0x");
                ffDotHex(Token);
                MSG_NUM_H(", InsPtr = 0x", InsPtr);
            }
            TRACENAMES;
#endif

/* Save IP on return stack like a JSR. */
            M_R_PUSH( InsPtr );

/* Convert execution token to absolute address. */
            InsPtr = (cell_t *) ( LOCAL_CODEREL_TO_ABS(Token) );

/* Fetch token at IP. */
            Token = READ_CELL_DIC(InsPtr++);

#ifdef PF_SUPPORT_TRACE
/* Bump level for trace display */
            Level++;
#endif
        }


#ifdef PF_SUPPORT_TRACE
        TRACENAMES;
#endif

/* Execute primitive Token. */
        switch( Token )
        {

    /* Pop up a level in Forth inner interpreter.
    ** Used to implement semicolon.
    ** Put first in switch because ID_EXIT==0 */
        case ID_EXIT:
            InsPtr = ( cell_t *) M_R_POP;
#ifdef PF_SUPPORT_TRACE
            Level--;
#endif
            endcase;

        case ID_1MINUS:  TOS--; endcase;

        case ID_1PLUS:   TOS++; endcase;

#ifndef PF_NO_SHELL
        case ID_2LITERAL:
            ff2Literal( TOS, M_POP );
            M_DROP;
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_2LITERAL_P:
/* hi part stored first, put on top of stack */
            PUSH_TOS;
            TOS = READ_CELL_DIC(InsPtr++);
            M_PUSH(READ_CELL_DIC(InsPtr++));
            endcase;

        case ID_2MINUS:  TOS -= 2; endcase;

        case ID_2PLUS:   TOS += 2; endcase;


        case ID_2OVER:  /* ( a b c d -- a b c d a b ) */
            PUSH_TOS;
            Scratch = M_STACK(3);
            M_PUSH(Scratch);
            TOS = M_STACK(3);
            endcase;

        case ID_2SWAP:  /* ( a b c d -- c d a b ) */
            Scratch = M_STACK(0);    /* c */
            M_STACK(0) = M_STACK(2); /* a */
            M_STACK(2) = Scratch;    /* c */
            Scratch = TOS;           /* d */
            TOS = M_STACK(1);        /* b */
            M_STACK(1) = Scratch;    /* d */
            endcase;

        case ID_2DUP:   /* ( a b -- a b a b ) */
            PUSH_TOS;
            Scratch = M_STACK(1);
            M_PUSH(Scratch);
            endcase;

        case ID_2_R_FETCH:
            PUSH_TOS;
            M_PUSH( (*(TORPTR+1)) );
            TOS = (*(TORPTR));
            endcase;

        case ID_2_R_FROM:
            PUSH_TOS;
            TOS = M_R_POP;
            M_PUSH( M_R_POP );
            endcase;

        case ID_2_TO_R:
            M_R_PUSH( M_POP );
            M_R_PUSH( TOS );
            M_DROP;
            endcase;

        case ID_ACCEPT_P: /* ( c-addr +n1 -- +n2 ) */
            CharPtr = (char *) M_POP;
            TOS = ioAccept( CharPtr, TOS );
            endcase;

#ifndef PF_NO_SHELL
        case ID_ALITERAL:
            ffALiteral( ABS_TO_CODEREL(TOS) );
            M_DROP;
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_ALITERAL_P:
            PUSH_TOS;
            TOS = (cell_t) LOCAL_CODEREL_TO_ABS( READ_CELL_DIC(InsPtr++) );
            endcase;

/* Allocate some extra and put validation identifier at base */
#define PF_MEMORY_VALIDATOR  (0xA81B4D69)
        case ID_ALLOCATE:
            /* Allocate at least one cell's worth because we clobber first cell. */
            if ( TOS < sizeof(cell_t) )
            {
                Temp = sizeof(cell_t);
            }
            else
            {
                Temp = TOS;
            }
            /* Allocate extra cells worth because we store validation info. */
            CellPtr = (cell_t *) pfAllocMem( Temp + sizeof(cell_t) );
            if( CellPtr )
            {
/* This was broken into two steps because different compilers incremented
** CellPtr before or after the XOR step. */
                Temp = (cell_t)CellPtr ^ PF_MEMORY_VALIDATOR;
                *CellPtr++ = Temp;
                M_PUSH( (cell_t) CellPtr );
                TOS = 0;
            }
            else
            {
                M_PUSH( 0 );
                TOS = -1;  /* FIXME Fix error code. */
            }
            endcase;

        case ID_AND:     BINARY_OP( & ); endcase;

        case ID_ARSHIFT:     BINARY_OP( >> ); endcase;  /* Arithmetic right shift */

        case ID_BODY_OFFSET:
            PUSH_TOS;
            TOS = CREATE_BODY_OFFSET;
            endcase;

/* Branch is followed by an offset relative to address of offset. */
        case ID_BRANCH:
DBUGX(("Before Branch: IP = 0x%x\n", InsPtr ));
            M_BRANCH;
DBUGX(("After Branch: IP = 0x%x\n", InsPtr ));
            endcase;

        case ID_BYE:
            EMIT_CR;
            M_THROW( THROW_BYE );
            endcase;

        case ID_BAIL:
            MSG("Emergency exit.\n");
            EXIT(1);
            endcase;

        case ID_CATCH:
            Scratch = TOS;
            TOS = M_POP;
            SAVE_REGISTERS;
            Scratch = pfCatch( Scratch );
            LOAD_REGISTERS;
            M_PUSH( TOS );
            TOS = Scratch;
            endcase;

        case ID_CALL_C:
            SAVE_REGISTERS;
            Scratch = READ_CELL_DIC(InsPtr++);
            CallUserFunction( Scratch & 0xFFFF,
                (Scratch >> 31) & 1,
                (Scratch >> 24) & 0x7F );
            LOAD_REGISTERS;
            endcase;

        /* Support 32/64 bit operation. */
        case ID_CELL:
                M_PUSH( TOS );
                TOS = sizeof(cell_t);
                endcase;

        case ID_CELLS:
                TOS = TOS * sizeof(cell_t);
                endcase;

        case ID_CFETCH:   TOS = *((uint8_t *) TOS); endcase;

        case ID_CMOVE: /* ( src dst n -- ) */
            {
                register char *DstPtr = (char *) M_POP; /* dst */
                CharPtr = (char *) M_POP;    /* src */
                for( Scratch=0; (ucell_t) Scratch < (ucell_t) TOS ; Scratch++ )
                {
                    *DstPtr++ = *CharPtr++;
                }
                M_DROP;
            }
            endcase;

        case ID_CMOVE_UP: /* ( src dst n -- ) */
            {
                register char *DstPtr = ((char *) M_POP) + TOS; /* dst */
                CharPtr = ((char *) M_POP) + TOS;;    /* src */
                for( Scratch=0; (ucell_t) Scratch < (ucell_t) TOS ; Scratch++ )
                {
                    *(--DstPtr) = *(--CharPtr);
                }
                M_DROP;
            }
            endcase;

#ifndef PF_NO_SHELL
        case ID_COLON:
            SAVE_REGISTERS;
            ffColon( );
            LOAD_REGISTERS;
            endcase;
        case ID_COLON_P:  /* ( $name xt -- ) */
            CreateDicEntry( TOS, (char *) M_POP, 0 );
            M_DROP;
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_COMPARE:
            {
                const char *s1, *s2;
                cell_t len1;
                s2 = (const char *) M_POP;
                len1 = M_POP;
                s1 = (const char *) M_POP;
                TOS = ffCompare( s1, len1, s2, TOS );
            }
            endcase;

/* ( a b -- flag , Comparisons ) */
        case ID_COMP_EQUAL:
            TOS = ( TOS == M_POP ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_NOT_EQUAL:
            TOS = ( TOS != M_POP ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_GREATERTHAN:
            TOS = ( M_POP > TOS ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_LESSTHAN:
            TOS = (  M_POP < TOS ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_U_GREATERTHAN:
            TOS = ( ((ucell_t)M_POP) > ((ucell_t)TOS) ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_U_LESSTHAN:
            TOS = ( ((ucell_t)M_POP) < ((ucell_t)TOS) ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_ZERO_EQUAL:
            TOS = ( TOS == 0 ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_ZERO_NOT_EQUAL:
            TOS = ( TOS != 0 ) ? FTRUE : FALSE ;
            endcase;
        case ID_COMP_ZERO_GREATERTHAN:
            TOS = ( TOS > 0 ) ? FTRUE : FFALSE ;
            endcase;
        case ID_COMP_ZERO_LESSTHAN:
            TOS = ( TOS < 0 ) ? FTRUE : FFALSE ;
            endcase;

        case ID_CR:
            EMIT_CR;
            endcase;

#ifndef PF_NO_SHELL
        case ID_CREATE:
            SAVE_REGISTERS;
            ffCreate();
            LOAD_REGISTERS;
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_CREATE_P:
            PUSH_TOS;
/* Put address of body on stack.  Insptr points after code start. */
            TOS = (cell_t) ((char *)InsPtr - sizeof(cell_t) + CREATE_BODY_OFFSET );
            endcase;

        case ID_CSTORE: /* ( c caddr -- ) */
            *((uint8_t *) TOS) = (uint8_t) M_POP;
            M_DROP;
            endcase;

/* Double precision add. */
        case ID_D_PLUS:  /* D+ ( al ah bl bh -- sl sh ) */
            {
                register ucell_t ah,al,bl,sh,sl;
#define bh TOS
                bl = M_POP;
                ah = M_POP;
                al = M_POP;
                sh = 0;
                sl = al + bl;
                if( sl < bl ) sh = 1; /* Carry */
                sh += ah + bh;
                M_PUSH( sl );
                TOS = sh;
#undef bh
            }
            endcase;

/* Double precision subtract. */
        case ID_D_MINUS:  /* D- ( al ah bl bh -- sl sh ) */
            {
                register ucell_t ah,al,bl,sh,sl;
#define bh TOS
                bl = M_POP;
                ah = M_POP;
                al = M_POP;
                sh = 0;
                sl = al - bl;
                if( al < bl ) sh = 1; /* Borrow */
                sh = ah - bh - sh;
                M_PUSH( sl );
                TOS = sh;
#undef bh
            }
            endcase;

/* Assume 8-bit char and calculate cell width. */
#define NBITS ((sizeof(ucell_t)) * 8)
/* Define half the number of bits in a cell. */
#define HNBITS (NBITS / 2)
/* Assume two-complement arithmetic to calculate lower half. */
#define LOWER_HALF(n) ((n) & (((ucell_t)1 << HNBITS) - 1))
#define HIGH_BIT ((ucell_t)1 << (NBITS - 1))

/* Perform cell*cell bit multiply for a 2 cell result, by factoring into half cell quantities.
 * Using an improved algorithm suggested by Steve Green.
 * Converted to 64-bit by Aleksej Saushev.
 */
        case ID_D_UMTIMES:  /* UM* ( a b -- lo hi ) */
            {
                ucell_t ahi, alo, bhi, blo; /* input parts */
                ucell_t lo, hi, temp;
/* Get values from stack. */
                ahi = M_POP;
                bhi = TOS;
/* Break into hi and lo 16 bit parts. */
                alo = LOWER_HALF(ahi);
                ahi = ahi >> HNBITS;
                blo = LOWER_HALF(bhi);
                bhi = bhi >> HNBITS;

                lo = 0;
                hi = 0;
/* higher part: ahi * bhi */
                hi += ahi * bhi;
/* middle (overlapping) part: ahi * blo */
                temp = ahi * blo;
                lo += LOWER_HALF(temp);
                hi += temp >> HNBITS;
/* middle (overlapping) part: alo * bhi  */
                temp = alo * bhi;
                lo += LOWER_HALF(temp);
                hi += temp >> HNBITS;
/* lower part: alo * blo */
                temp = alo * blo;
/* its higher half overlaps with middle's lower half: */
                lo += temp >> HNBITS;
/* process carry: */
                hi += lo >> HNBITS;
                lo = LOWER_HALF(lo);
/* combine lower part of result: */
                lo = (lo << HNBITS) + LOWER_HALF(temp);

                M_PUSH( lo );
                TOS = hi;
            }
            endcase;

/* Perform cell*cell bit multiply for 2 cell result, using shift and add. */
        case ID_D_MTIMES:  /* M* ( a b -- pl ph ) */
            {
                ucell_t ahi, alo, bhi, blo; /* input parts */
                ucell_t lo, hi, temp;
                int sg;
/* Get values from stack. */
                ahi = M_POP;
                bhi = TOS;

/* Calculate product sign: */
                sg = ((cell_t)(ahi ^ bhi) < 0);
/* Take absolute values and reduce to um* */
                if ((cell_t)ahi < 0) ahi = (ucell_t)(-(cell_t)ahi);
                if ((cell_t)bhi < 0) bhi = (ucell_t)(-(cell_t)bhi);

/* Break into hi and lo 16 bit parts. */
                alo = LOWER_HALF(ahi);
                ahi = ahi >> HNBITS;
                blo = LOWER_HALF(bhi);
                bhi = bhi >> HNBITS;

                lo = 0;
                hi = 0;
/* higher part: ahi * bhi */
                hi += ahi * bhi;
/* middle (overlapping) part: ahi * blo */
                temp = ahi * blo;
                lo += LOWER_HALF(temp);
                hi += temp >> HNBITS;
/* middle (overlapping) part: alo * bhi  */
                temp = alo * bhi;
                lo += LOWER_HALF(temp);
                hi += temp >> HNBITS;
/* lower part: alo * blo */
                temp = alo * blo;
/* its higher half overlaps with middle's lower half: */
                lo += temp >> HNBITS;
/* process carry: */
                hi += lo >> HNBITS;
                lo = LOWER_HALF(lo);
/* combine lower part of result: */
                lo = (lo << HNBITS) + LOWER_HALF(temp);

/* Negate product if one operand negative. */
                if(sg)
                {
                    /* lo = (ucell_t)(- lo); */
                    lo = ~lo + 1;
                    hi = ~hi + ((lo == 0) ? 1 : 0);
                }

                M_PUSH( lo );
                TOS = hi;
            }
            endcase;

#define DULT(du1l,du1h,du2l,du2h) ( (du2h<du1h) ? FALSE : ( (du2h==du1h) ? (du1l<du2l) : TRUE) )
/* Perform 2 cell by 1 cell divide for 1 cell result and remainder, using shift and subtract. */
        case ID_D_UMSMOD:  /* UM/MOD ( al ah bdiv -- rem q ) */
            {
                ucell_t ah,al, q,di, bl,bh, sl,sh;
                ah = M_POP;
                al = M_POP;
                bh = TOS;
                bl = 0;
                q = 0;
                for( di=0; di<NBITS; di++ )
                {
                    if( !DULT(al,ah,bl,bh) )
                    {
                        sh = 0;
                        sl = al - bl;
                        if( al < bl ) sh = 1; /* Borrow */
                        sh = ah - bh - sh;
                        ah = sh;
                        al = sl;
                        q |= 1;
                    }
                    q = q << 1;
                    bl = (bl >> 1) | (bh << (NBITS-1));
                    bh = bh >> 1;
                }
                if( !DULT(al,ah,bl,bh) )
                {

                    al = al - bl;
                    q |= 1;
                }
                M_PUSH( al );  /* rem */
                TOS = q;
            }
            endcase;

/* Perform 2 cell by 1 cell divide for 2 cell result and remainder, using shift and subtract. */
        case ID_D_MUSMOD:  /* MU/MOD ( al am bdiv -- rem ql qh ) */
            {
                register ucell_t ah,am,al,ql,qh,di;
#define bdiv ((ucell_t)TOS)
                ah = 0;
                am = M_POP;
                al = M_POP;
                qh = ql = 0;
                for( di=0; di<2*NBITS; di++ )
                {
                    if( bdiv <= ah )
                    {
                        ah = ah - bdiv;
                        ql |= 1;
                    }
                    qh = (qh << 1) | (ql >> (NBITS-1));
                    ql = ql << 1;
                    ah = (ah << 1) | (am >> (NBITS-1));
                    am = (am << 1) | (al >> (NBITS-1));
                    al = al << 1;
DBUG(("XX ah,m,l = 0x%8x,%8x,%8x - qh,l = 0x%8x,%8x\n", ah,am,al, qh,ql ));
                }
                if( bdiv <= ah )
                {
                    ah = ah - bdiv;
                    ql |= 1;
                }
                M_PUSH( ah ); /* rem */
                M_PUSH( ql );
                TOS = qh;
#undef bdiv
            }
            endcase;

#ifndef PF_NO_SHELL
        case ID_DEFER:
            ffDefer( );
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_DEFER_P:
            endcase;

        case ID_DEPTH:
            PUSH_TOS;
            TOS = gCurrentTask->td_StackBase - STKPTR;
            endcase;

        case ID_DIVIDE:     BINARY_OP( / ); endcase;

        case ID_DOT:
            ffDot( TOS );
            M_DROP;
            endcase;

        case ID_DOTS:
            M_DOTS;
            endcase;

        case ID_DROP:  M_DROP; endcase;

        case ID_DUMP:
            Scratch = M_POP;
            DumpMemory( (char *) Scratch, TOS );
            M_DROP;
            endcase;

        case ID_DUP:   M_DUP; endcase;

        case ID_DO_P: /* ( limit start -- ) ( R: -- start limit ) */
            M_R_PUSH( TOS );
            M_R_PUSH( M_POP );
            M_DROP;
            endcase;

        case ID_EOL:    /* ( -- end_of_line_char ) */
            PUSH_TOS;
            TOS = (cell_t) '\n';
            endcase;

        case ID_ERRORQ_P:  /* ( flag num -- , quit if flag true ) */
            Scratch = TOS;
            M_DROP;
            if(TOS)
            {
                M_THROW(Scratch);
            }
            else
            {
                M_DROP;
            }
            endcase;

        case ID_EMIT_P:
            EMIT( (char) TOS );
            M_DROP;
            endcase;

        case ID_EXECUTE:
/* Save IP on return stack like a JSR. */
            M_R_PUSH( InsPtr );
#ifdef PF_SUPPORT_TRACE
/* Bump level for trace. */
            Level++;
#endif
            if( IsTokenPrimitive( TOS ) )
            {
                WRITE_CELL_DIC( (cell_t *) &FakeSecondary[0], TOS);   /* Build a fake secondary and execute it. */
                InsPtr = &FakeSecondary[0];
            }
            else
            {
                InsPtr = (cell_t *) LOCAL_CODEREL_TO_ABS(TOS);
            }
            M_DROP;
            endcase;

        case ID_FETCH:
#if (defined(PF_BIG_ENDIAN_DIC) || defined(PF_LITTLE_ENDIAN_DIC))
            if( IN_DICS( TOS ) )
            {
                TOS = (cell_t) READ_CELL_DIC((cell_t *)TOS);
            }
            else
            {
                TOS = *((cell_t *)TOS);
            }
#else
            TOS = *((cell_t *)TOS);
#endif
            endcase;

        case ID_FILE_CREATE: /* ( c-addr u fam -- fid ior ) */
/* Build NUL terminated name string. */
            Scratch = M_POP; /* u */
            Temp = M_POP;    /* caddr */
            if( Scratch < TIB_SIZE-2 )
            {
                const char *famText = pfSelectFileModeCreate( TOS );
                pfCopyMemory( gScratch, (char *) Temp, (ucell_t) Scratch );
                gScratch[Scratch] = '\0';
                DBUG(("Create file = %s with famTxt %s\n", gScratch, famText ));
                FileID = sdOpenFile( gScratch, famText );
                TOS = ( FileID == NULL ) ? -1 : 0 ;
                M_PUSH( (cell_t) FileID );
            }
            else
            {
                ERR("Filename too large for name buffer.\n");
                M_PUSH( 0 );
                TOS = -2;
            }
            endcase;

        case ID_FILE_DELETE: /* ( c-addr u -- ior ) */
/* Build NUL terminated name string. */
            Temp = M_POP;    /* caddr */
            if( TOS < TIB_SIZE-2 )
            {
                pfCopyMemory( gScratch, (char *) Temp, (ucell_t) TOS );
                gScratch[TOS] = '\0';
                DBUG(("Delete file = %s\n", gScratch ));
                TOS = sdDeleteFile( gScratch );
            }
            else
            {
                ERR("Filename too large for name buffer.\n");
                TOS = -2;
            }
            endcase;

        case ID_FILE_OPEN: /* ( c-addr u fam -- fid ior ) */
            /* Build NUL terminated name string. */
            Scratch = M_POP; /* u */
            Temp = M_POP;    /* caddr */
            if( Scratch < TIB_SIZE-2 )
            {
                const char *famText = pfSelectFileModeOpen( TOS );
                pfCopyMemory( gScratch, (char *) Temp, (ucell_t) Scratch );
                gScratch[Scratch] = '\0';
                DBUG(("Open file = %s\n", gScratch ));
                FileID = sdOpenFile( gScratch, famText );

                TOS = ( FileID == NULL ) ? -1 : 0 ;
                M_PUSH( (cell_t) FileID );
            }
            else
            {
                ERR("Filename too large for name buffer.\n");
                M_PUSH( 0 );
                TOS = -2;
            }
            endcase;

        case ID_FILE_CLOSE: /* ( fid -- ior ) */
            TOS = sdCloseFile( (FileStream *) TOS );
            endcase;

        case ID_FILE_READ: /* ( addr len fid -- u2 ior ) */
            FileID = (FileStream *) TOS;
            Scratch = M_POP;
            CharPtr = (char *) M_POP;
            Temp = sdReadFile( CharPtr, 1, Scratch, FileID );
            /* TODO check feof() or ferror() */
            M_PUSH(Temp);
            TOS = 0;
            endcase;

        /* TODO Why does this crash when passed an illegal FID? */
        case ID_FILE_SIZE: /* ( fid -- ud ior ) */
/* Determine file size by seeking to end and returning position. */
            FileID = (FileStream *) TOS;
            {
                file_offset_t endposition = -1;
                file_offset_t original = sdTellFile( FileID );
                if (original >= 0)
                {
                    sdSeekFile( FileID, 0, PF_SEEK_END );
                    endposition = sdTellFile( FileID );
                    /* Restore original position. */
                    sdSeekFile( FileID, original, PF_SEEK_SET );
                }
                if (endposition < 0)
                {
                    M_PUSH(0); /* low */
                    M_PUSH(0); /* high */
                    TOS = -4;  /* TODO proper error number */
                }
                else
                {
                    M_PUSH(endposition); /* low */
                    /* We do not support double precision file offsets.*/
                    M_PUSH(0); /* high */
                    TOS = 0;   /* OK */
                }
            }
            endcase;

        case ID_FILE_WRITE: /* ( addr len fid -- ior ) */
            FileID = (FileStream *) TOS;
            Scratch = M_POP;
            CharPtr = (char *) M_POP;
            Temp = sdWriteFile( CharPtr, 1, Scratch, FileID );
            TOS = (Temp != Scratch) ? -3 : 0;
            endcase;

        case ID_FILE_REPOSITION: /* ( ud fid -- ior ) */
            {
                file_offset_t offset;
                cell_t offsetHigh;
                cell_t offsetLow;
                FileID = (FileStream *) TOS;
                offsetHigh = M_POP;
                offsetLow = M_POP;
                /* We do not support double precision file offsets in pForth.
                 * So check to make sure the high bits are not used.
                 */
                if (offsetHigh != 0)
                {
                    TOS = -3; /* TODO err num? */
                    break;
                }
                offset = (file_offset_t)offsetLow;
                TOS = sdSeekFile( FileID, offset, PF_SEEK_SET );
            }
            endcase;

        case ID_FILE_POSITION: /* ( fid -- ud ior ) */
            {
                file_offset_t position;
                FileID = (FileStream *) TOS;
                position = sdTellFile( FileID );
                if (position < 0)
                {
                    M_PUSH(0); /* low */
                    M_PUSH(0); /* high */
                    TOS = -4;  /* TODO proper error number */
                }
                else
                {
                    M_PUSH(position); /* low */
                    /* We do not support double precision file offsets.*/
                    M_PUSH(0); /* high */
                    TOS = 0; /* OK */
                }
            }
            endcase;

        case ID_FILE_RO: /* (  -- fam ) */
            PUSH_TOS;
            TOS = PF_FAM_READ_ONLY;
            endcase;

        case ID_FILE_RW: /* ( -- fam ) */
            PUSH_TOS;
            TOS = PF_FAM_READ_WRITE;
            endcase;

        case ID_FILE_WO: /* ( -- fam ) */
            PUSH_TOS;
            TOS = PF_FAM_WRITE_ONLY;
            endcase;

        case ID_FILE_BIN: /* ( fam1 -- fam2 ) */
            TOS = TOS | PF_FAM_BINARY_FLAG;
            endcase;

	case ID_FILE_FLUSH: /* ( fileid -- ior ) */
	    {
		FileStream *Stream = (FileStream *) TOS;
		TOS = (sdFlushFile( Stream ) == 0) ? 0 : THROW_FLUSH_FILE;
	    }
	    endcase;

	case ID_FILE_RENAME: /* ( oldName newName -- ior ) */
	    {
		char *New = (char *) TOS;
		char *Old = (char *) M_POP;
		TOS = sdRenameFile( Old, New );
	    }
	    endcase;

	case ID_FILE_RESIZE: /* ( ud fileid -- ior ) */
	    {
		FileStream *File = (FileStream *) TOS;
		ucell_t SizeHi = (ucell_t) M_POP;
		ucell_t SizeLo = (ucell_t) M_POP;
		TOS = ( UdIsUint64( SizeHi )
			? sdResizeFile( File, UdToUint64( SizeLo, SizeHi ))
			: THROW_RESIZE_FILE );
	    }
	    endcase;

        case ID_FILL: /* ( caddr num charval -- ) */
            {
                register char *DstPtr;
                Temp = M_POP;    /* num */
                DstPtr = (char *) M_POP; /* dst */
                for( Scratch=0; (ucell_t) Scratch < (ucell_t) Temp ; Scratch++ )
                {
                    *DstPtr++ = (char) TOS;
                }
                M_DROP;
            }
            endcase;

#ifndef PF_NO_SHELL
        case ID_FIND:  /* ( $addr -- $addr 0 | xt +-1 ) */
            TOS = ffFind( (char *) TOS, (ExecToken *) &Temp );
            M_PUSH( Temp );
            endcase;

        case ID_FINDNFA:
            TOS = ffFindNFA( (const ForthString *) TOS, (const ForthString **) &Temp );
            M_PUSH( (cell_t) Temp );
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_FLUSHEMIT:
            sdTerminalFlush();
            endcase;

/* Validate memory before freeing. Clobber validator and first word. */
        case ID_FREE:   /* ( addr -- result ) */
            if( TOS == 0 )
            {
                ERR("FREE passed NULL!\n");
                TOS = -2; /* FIXME error code */
            }
            else
            {
                CellPtr = (cell_t *) TOS;
                CellPtr--;
                if( ((ucell_t)*CellPtr) != ((ucell_t)CellPtr ^ PF_MEMORY_VALIDATOR))
                {
                    TOS = -2; /* FIXME error code */
                }
                else
                {
                    CellPtr[0] = 0xDeadBeef;
                    pfFreeMem((char *)CellPtr);
                    TOS = 0;
                }
            }
            endcase;

#include "pfinnrfp.h"

        case ID_HERE:
            PUSH_TOS;
            TOS = (cell_t)CODE_HERE;
            endcase;

        case ID_NUMBERQ_P:   /* ( addr -- 0 | n 1 ) */
/* Convert using number converter in 'C'.
** Only supports single precision for bootstrap.
*/
            TOS = (cell_t) ffNumberQ( (char *) TOS, &Temp );
            if( TOS == NUM_TYPE_SINGLE)
            {
                M_PUSH( Temp );   /* Push single number */
            }
            endcase;

        case ID_I:  /* ( -- i , DO LOOP index ) */
            PUSH_TOS;
            TOS = M_R_PICK(1);
            endcase;

#ifndef PF_NO_SHELL
        case ID_INCLUDE_FILE:
            FileID = (FileStream *) TOS;
            M_DROP;    /* Drop now so that INCLUDE has a clean stack. */
            SAVE_REGISTERS;
            Scratch = ffIncludeFile( FileID );
            LOAD_REGISTERS;
            if( Scratch ) M_THROW(Scratch)
            endcase;
#endif  /* !PF_NO_SHELL */

#ifndef PF_NO_SHELL
        case ID_INTERPRET:
            SAVE_REGISTERS;
            Scratch = ffInterpret();
            LOAD_REGISTERS;
            if( Scratch ) M_THROW(Scratch)
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_J:  /* ( -- j , second DO LOOP index ) */
            PUSH_TOS;
            TOS = M_R_PICK(3);
            endcase;

        case ID_KEY:
            PUSH_TOS;
            TOS = ioKey();
            if (TOS == ASCII_EOT) {
                M_THROW(THROW_BYE);
            }
            endcase;

#ifndef PF_NO_SHELL
        case ID_LITERAL:
            ffLiteral( TOS );
            M_DROP;
            endcase;
#endif /* !PF_NO_SHELL */

        case ID_LITERAL_P:
            DBUG(("ID_LITERAL_P: InsPtr = 0x%x, *InsPtr = 0x%x\n", InsPtr, *InsPtr ));
            PUSH_TOS;
            TOS = READ_CELL_DIC(InsPtr++);
            endcase;

#ifndef PF_NO_SHELL
        case ID_LOCAL_COMPILER: DO_VAR(gLocalCompiler_XT); endcase;
#endif /* !PF_NO_SHELL */

        case ID_LOCAL_FETCH: /* ( i <local> -- n , fetch from local ) */
            TOS = *(LocalsPtr - TOS);
            endcase;

#define LOCAL_FETCH_N(num) \
        case ID_LOCAL_FETCH_##num: /* ( <local> -- n , fetch from local ) */ \
            PUSH_TOS; \
            TOS = *(LocalsPtr -(num)); \
            endcase;

        LOCAL_FETCH_N(1);
        LOCAL_FETCH_N(2);
        LOCAL_FETCH_N(3);
        LOCAL_FETCH_N(4);
        LOCAL_FETCH_N(5);
        LOCAL_FETCH_N(6);
        LOCAL_FETCH_N(7);
        LOCAL_FETCH_N(8);

        case ID_LOCAL_STORE:  /* ( n i <local> -- , store n in local ) */
            *(LocalsPtr - TOS) = M_POP;
            M_DROP;
            endcase;

#define LOCAL_STORE_N(num) \
        case ID_LOCAL_STORE_##num:  /* ( n <local> -- , store n in local ) */ \
            *(LocalsPtr - (num)) = TOS; \
            M_DROP; \
            endcase;

        LOCAL_STORE_N(1);
        LOCAL_STORE_N(2);
        LOCAL_STORE_N(3);
        LOCAL_STORE_N(4);
        LOCAL_STORE_N(5);
        LOCAL_STORE_N(6);
        LOCAL_STORE_N(7);
        LOCAL_STORE_N(8);

        case ID_LOCAL_PLUSSTORE:  /* ( n i <local> -- , add n to local ) */
            *(LocalsPtr - TOS) += M_POP;
            M_DROP;
            endcase;

        case ID_LOCAL_ENTRY: /* ( x0 x1 ... xn n -- ) */
        /* create local stack frame */
            {
                cell_t i = TOS;
                cell_t *lp;
                DBUG(("LocalEntry: n = %d\n", TOS));
                /* End of locals. Create stack frame */
                DBUG(("LocalEntry: before RP@ = 0x%x, LP = 0x%x\n",
                    TORPTR, LocalsPtr));
                M_R_PUSH(LocalsPtr);
                LocalsPtr = TORPTR;
                TORPTR -= TOS;
                DBUG(("LocalEntry: after RP@ = 0x%x, LP = 0x%x\n",
                    TORPTR, LocalsPtr));
                lp = TORPTR;
                while(i-- > 0)
                {
                    *lp++ = M_POP;    /* Load local vars from stack */
                }
                M_DROP;
            }
            endcase;

        case ID_LOCAL_EXIT: /* cleanup up local stack frame */
            DBUG(("LocalExit: before RP@ = 0x%x, LP = 0x%x\n",
                TORPTR, LocalsPtr));
            TORPTR = LocalsPtr;
            LocalsPtr = (cell_t *) M_R_POP;
            DBUG(("LocalExit: after RP@ = 0x%x, LP = 0x%x\n",
                TORPTR, LocalsPtr));
            endcase;

#ifndef PF_NO_SHELL
        case ID_LOADSYS:
            MSG("Load "); MSG(SYSTEM_LOAD_FILE); EMIT_CR;
            FileID = sdOpenFile(SYSTEM_LOAD_FILE, "r");
            if( FileID )
            {
                SAVE_REGISTERS;
                Scratch = ffIncludeFile( FileID ); /* Also closes the file. */
                LOAD_REGISTERS;
                if( Scratch ) M_THROW(Scratch);
            }
            else
            {
                 ERR(SYSTEM_LOAD_FILE); ERR(" could not be opened!\n");
            }
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_LEAVE_P: /* ( R: index limit --  ) */
            M_R_DROP;
            M_R_DROP;
            M_BRANCH;
            endcase;

        case ID_LOOP_P: /* ( R: index limit -- | index limit ) */
            Temp = M_R_POP; /* limit */
            Scratch = M_R_POP + 1; /* index */
            if( Scratch == Temp )
            {
                InsPtr++;   /* skip branch offset, exit loop */
            }
            else
            {
/* Push index and limit back to R */
                M_R_PUSH( Scratch );
                M_R_PUSH( Temp );
/* Branch back to just after (DO) */
                M_BRANCH;
            }
            endcase;

        case ID_LSHIFT:     BINARY_OP( << ); endcase;

        case ID_MAX:
            Scratch = M_POP;
            TOS = ( TOS > Scratch ) ? TOS : Scratch ;
            endcase;

        case ID_MIN:
            Scratch = M_POP;
            TOS = ( TOS < Scratch ) ? TOS : Scratch ;
            endcase;

        case ID_MINUS:     BINARY_OP( - ); endcase;

#ifndef PF_NO_SHELL
        case ID_NAME_TO_TOKEN:
            TOS = (cell_t) NameToToken((ForthString *)TOS);
            endcase;

        case ID_NAME_TO_PREVIOUS:
            TOS = (cell_t) NameToPrevious((ForthString *)TOS);
            endcase;
#endif

        case ID_NOOP:
            endcase;

        case ID_OR:     BINARY_OP( | ); endcase;

        case ID_OVER:
            PUSH_TOS;
            TOS = M_STACK(1);
            endcase;

        case ID_PICK: /* ( ... n -- sp(n) ) */
            TOS = M_STACK(TOS);
            endcase;

        case ID_PLUS:     BINARY_OP( + ); endcase;

        case ID_PLUS_STORE:   /* ( n addr -- , add n to *addr ) */
#if (defined(PF_BIG_ENDIAN_DIC) || defined(PF_LITTLE_ENDIAN_DIC))
            if( IN_DICS( TOS ) )
            {
                Scratch = READ_CELL_DIC((cell_t *)TOS);
                Scratch += M_POP;
                WRITE_CELL_DIC((cell_t *)TOS,Scratch);
            }
            else
            {
                *((cell_t *)TOS) += M_POP;
            }
#else
            *((cell_t *)TOS) += M_POP;
#endif
            M_DROP;
            endcase;

        case ID_PLUSLOOP_P: /* ( delta -- ) ( R: index limit -- | index limit ) */
            {
		cell_t Limit = M_R_POP;
		cell_t OldIndex = M_R_POP;
		cell_t Delta = TOS; /* add TOS to index, not 1 */
		cell_t NewIndex = OldIndex + Delta;
		cell_t OldDiff = OldIndex - Limit;

		/* This exploits this idea (lifted from Gforth):
		   (x^y)<0 is equivalent to (x<0) != (y<0) */
                if( ((OldDiff ^ (OldDiff + Delta)) /* is the limit crossed? */
		     & (OldDiff ^ Delta))          /* is it a wrap-around? */
		    < 0 )
		{
                    InsPtr++;   /* skip branch offset, exit loop */
                }
                else
                {
/* Push index and limit back to R */
                    M_R_PUSH( NewIndex );
                    M_R_PUSH( Limit );
/* Branch back to just after (DO) */
                    M_BRANCH;
                }
                M_DROP;
            }
            endcase;

        case ID_QDO_P: /* (?DO) ( limit start -- ) ( R: -- start limit ) */
            Scratch = M_POP;  /* limit */
            if( Scratch == TOS )
            {
/* Branch to just after (LOOP) */
                M_BRANCH;
            }
            else
            {
                M_R_PUSH( TOS );
                M_R_PUSH( Scratch );
                InsPtr++;   /* skip branch offset, enter loop */
            }
            M_DROP;
            endcase;

        case ID_QDUP:     if( TOS ) M_DUP; endcase;

        case ID_QTERMINAL:  /* WARNING: Typically not fully implemented! */
            PUSH_TOS;
            TOS = sdQueryTerminal();
            endcase;

        case ID_QUIT_P: /* Stop inner interpreter, go back to user. */
#ifdef PF_SUPPORT_TRACE
            Level = 0;
#endif
            M_THROW(THROW_QUIT);
            endcase;

        case ID_R_DROP:
            M_R_DROP;
            endcase;

        case ID_R_FETCH:
            PUSH_TOS;
            TOS = (*(TORPTR));
            endcase;

        case ID_R_FROM:
            PUSH_TOS;
            TOS = M_R_POP;
            endcase;

        case ID_REFILL:
            PUSH_TOS;
            TOS = (ffRefill() > 0) ? FTRUE : FFALSE;
            endcase;

/* Resize memory allocated by ALLOCATE. */
        case ID_RESIZE:  /* ( addr1 u -- addr2 result ) */
            {
                cell_t *Addr1 = (cell_t *) M_POP;
                /* Point to validator below users address. */
                cell_t *FreePtr = Addr1 - 1;
                if( ((ucell_t)*FreePtr) != ((ucell_t)FreePtr ^ PF_MEMORY_VALIDATOR))
                {
                    /* 090218 - Fixed bug, was returning zero. */
                    M_PUSH( Addr1 );
                    TOS = -3;
                }
                else
                {
                    /* Try to allocate. */
                    CellPtr = (cell_t *) pfAllocMem( TOS + sizeof(cell_t) );
                    if( CellPtr )
                    {
                        /* Copy memory including validation. */
                        pfCopyMemory( (char *) CellPtr, (char *) FreePtr, TOS + sizeof(cell_t) );
                        *CellPtr = (cell_t)(((ucell_t)CellPtr) ^ (ucell_t)PF_MEMORY_VALIDATOR);
                        /* 090218 - Fixed bug that was incrementing the address twice. Thanks Reinhold Straub. */
                        /* Increment past validator to user address. */
                        M_PUSH( (cell_t) (CellPtr + 1) );
                        TOS = 0; /* Result code. */
                        /* Mark old cell as dead so we can't free it twice. */
                        FreePtr[0] = 0xDeadBeef;
                        pfFreeMem((char *) FreePtr);
                    }
                    else
                    {
                        /* 090218 - Fixed bug, was returning zero. */
                        M_PUSH( Addr1 );
                        TOS = -4;  /* FIXME Fix error code. */
                    }
                }
            }
            endcase;

/*
** RP@ and RP! are called secondaries so we must
** account for the return address pushed before calling.
*/
        case ID_RP_FETCH:    /* ( -- rp , address of top of return stack ) */
            PUSH_TOS;
            TOS = (cell_t)TORPTR;  /* value before calling RP@ */
            endcase;

        case ID_RP_STORE:    /* ( rp -- , address of top of return stack ) */
            TORPTR = (cell_t *) TOS;
            M_DROP;
            endcase;

        case ID_ROLL: /* ( xu xu-1 xu-1 ... x0 u -- xu-1 xu-1 ... x0 xu ) */
            {
                cell_t ri;
                cell_t *srcPtr, *dstPtr;
                Scratch = M_STACK(TOS);
                srcPtr = &M_STACK(TOS-1);
                dstPtr = &M_STACK(TOS);
                for( ri=0; ri<TOS; ri++ )
                {
                    *dstPtr-- = *srcPtr--;
                }
                TOS = Scratch;
                STKPTR++;
            }
            endcase;

        case ID_ROT:  /* ( a b c -- b c a ) */
            Scratch = M_POP;    /* b */
            Temp = M_POP;       /* a */
            M_PUSH( Scratch );  /* b */
            PUSH_TOS;           /* c */
            TOS = Temp;         /* a */
            endcase;

/* Logical right shift */
        case ID_RSHIFT:     { TOS = ((ucell_t)M_POP) >> TOS; } endcase;

#ifndef PF_NO_SHELL
        case ID_SAVE_FORTH_P:   /* ( $name Entry NameSize CodeSize -- err ) */
            {
                cell_t NameSize, CodeSize, EntryPoint;
                CodeSize = TOS;
                NameSize = M_POP;
                EntryPoint = M_POP;
                ForthStringToC( gScratch, (char *) M_POP, sizeof(gScratch) );
                TOS =  ffSaveForth( gScratch, EntryPoint, NameSize, CodeSize );
            }
            endcase;
#endif

        case ID_SLEEP_P:
            TOS = sdSleepMillis(TOS);
            endcase;

        case ID_SP_FETCH:    /* ( -- sp , address of top of stack, sorta ) */
            PUSH_TOS;
            TOS = (cell_t)STKPTR;
            endcase;

        case ID_SP_STORE:    /* ( sp -- , address of top of stack, sorta ) */
            STKPTR = (cell_t *) TOS;
            M_DROP;
            endcase;

        case ID_STORE: /* ( n addr -- , write n to addr ) */
#if (defined(PF_BIG_ENDIAN_DIC) || defined(PF_LITTLE_ENDIAN_DIC))
            if( IN_DICS( TOS ) )
            {
                WRITE_CELL_DIC((cell_t *)TOS,M_POP);
            }
            else
            {
                *((cell_t *)TOS) = M_POP;
            }
#else
            *((cell_t *)TOS) = M_POP;
#endif
            M_DROP;
            endcase;

        case ID_SCAN: /* ( addr cnt char -- addr' cnt' ) */
            Scratch = M_POP; /* cnt */
            Temp = M_POP;    /* addr */
            TOS = ffScan( (char *) Temp, Scratch, (char) TOS, &CharPtr );
            M_PUSH((cell_t) CharPtr);
            endcase;

#ifndef PF_NO_SHELL
        case ID_SEMICOLON:
            SAVE_REGISTERS;
            Scratch = ffSemiColon();
            LOAD_REGISTERS;
            if( Scratch ) M_THROW( Scratch );
            endcase;
#endif /* !PF_NO_SHELL */

        case ID_SKIP: /* ( addr cnt char -- addr' cnt' ) */
            Scratch = M_POP; /* cnt */
            Temp = M_POP;    /* addr */
            TOS = ffSkip( (char *) Temp, Scratch, (char) TOS, &CharPtr );
            M_PUSH((cell_t) CharPtr);
            endcase;

        case ID_SOURCE:  /* ( -- c-addr num ) */
            PUSH_TOS;
            M_PUSH( (cell_t) gCurrentTask->td_SourcePtr );
            TOS = (cell_t) gCurrentTask->td_SourceNum;
            endcase;

        case ID_SOURCE_SET: /* ( c-addr num -- ) */
            gCurrentTask->td_SourcePtr = (char *) M_POP;
            gCurrentTask->td_SourceNum = TOS;
            M_DROP;
            endcase;

        case ID_SOURCE_ID:
            PUSH_TOS;
            TOS = ffConvertStreamToSourceID( gCurrentTask->td_InputStream ) ;
            endcase;

        case ID_SOURCE_ID_POP:
            PUSH_TOS;
            TOS = ffConvertStreamToSourceID( ffPopInputStream() ) ;
            endcase;

        case ID_SOURCE_ID_PUSH:  /* ( source-id -- ) */
            TOS = (cell_t)ffConvertSourceIDToStream( TOS );
            Scratch = ffPushInputStream((FileStream *) TOS );
            if( Scratch )
            {
                M_THROW(Scratch);
            }
            else M_DROP;
            endcase;

	case ID_SOURCE_LINE_NUMBER_FETCH: /* ( -- linenr ) */
	    PUSH_TOS;
	    TOS = gCurrentTask->td_LineNumber;
	    endcase;

	case ID_SOURCE_LINE_NUMBER_STORE: /* ( linenr -- ) */
	    gCurrentTask->td_LineNumber = TOS;
	    TOS = M_POP;
	    endcase;

        case ID_SWAP:
            Scratch = TOS;
            TOS = *STKPTR;
            *STKPTR = Scratch;
            endcase;

        case ID_TEST1:
            PUSH_TOS;
            M_PUSH( 0x11 );
            M_PUSH( 0x22 );
            TOS = 0x33;
            endcase;

        case ID_TEST2:
            endcase;

        case ID_THROW:  /* ( k*x err -- k*x | i*x err , jump to where CATCH was called ) */
            if(TOS)
            {
                M_THROW(TOS);
            }
            else M_DROP;
            endcase;

#ifndef PF_NO_SHELL
        case ID_TICK:
            PUSH_TOS;
            CharPtr = (char *) ffWord( (char) ' ' );
            TOS = ffFind( CharPtr, (ExecToken *) &Temp );
            if( TOS == 0 )
            {
                ERR("' could not find ");
                ioType( (char *) CharPtr+1, *CharPtr );
                EMIT_CR;
                M_THROW(-13);
            }
            else
            {
                TOS = Temp;
            }
            endcase;
#endif  /* !PF_NO_SHELL */

        case ID_TIMES: BINARY_OP( * ); endcase;

        case ID_TYPE:
            Scratch = M_POP; /* addr */
            ioType( (char *) Scratch, TOS );
            M_DROP;
            endcase;

        case ID_TO_R:
            M_R_PUSH( TOS );
            M_DROP;
            endcase;

        case ID_VAR_BASE: DO_VAR(gVarBase); endcase;
        case ID_VAR_BYE_CODE: DO_VAR(gVarByeCode); endcase;
        case ID_VAR_CODE_BASE: DO_VAR(gCurrentDictionary->dic_CodeBase); endcase;
        case ID_VAR_CODE_LIMIT: DO_VAR(gCurrentDictionary->dic_CodeLimit); endcase;
        case ID_VAR_CONTEXT: DO_VAR(gVarContext); endcase;
        case ID_VAR_DP: DO_VAR(gCurrentDictionary->dic_CodePtr.Cell); endcase;
        case ID_VAR_ECHO: DO_VAR(gVarEcho); endcase;
        case ID_VAR_HEADERS_BASE: DO_VAR(gCurrentDictionary->dic_HeaderBase); endcase;
        case ID_VAR_HEADERS_LIMIT: DO_VAR(gCurrentDictionary->dic_HeaderLimit); endcase;
        case ID_VAR_HEADERS_PTR: DO_VAR(gCurrentDictionary->dic_HeaderPtr); endcase;
        case ID_VAR_NUM_TIB: DO_VAR(gCurrentTask->td_SourceNum); endcase;
        case ID_VAR_OUT: DO_VAR(gCurrentTask->td_OUT); endcase;
        case ID_VAR_STATE: DO_VAR(gVarState); endcase;
        case ID_VAR_TO_IN: DO_VAR(gCurrentTask->td_IN); endcase;
        case ID_VAR_TRACE_FLAGS: DO_VAR(gVarTraceFlags); endcase;
        case ID_VAR_TRACE_LEVEL: DO_VAR(gVarTraceLevel); endcase;
        case ID_VAR_TRACE_STACK: DO_VAR(gVarTraceStack); endcase;
        case ID_VAR_RETURN_CODE: DO_VAR(gVarReturnCode); endcase;

        case ID_VERSION_CODE:
            M_PUSH( TOS );
            TOS = PFORTH_VERSION_CODE;
            endcase;

        case ID_WORD:
            TOS = (cell_t) ffWord( (char) TOS );
            endcase;

        case ID_WORD_FETCH: /* ( waddr -- w ) */
#if (defined(PF_BIG_ENDIAN_DIC) || defined(PF_LITTLE_ENDIAN_DIC))
            if( IN_DICS( TOS ) )
            {
                TOS = (uint16_t) READ_SHORT_DIC((uint16_t *)TOS);
            }
            else
            {
                TOS = *((uint16_t *)TOS);
            }
#else
            TOS = *((uint16_t *)TOS);
#endif
            endcase;

        case ID_WORD_STORE: /* ( w waddr -- ) */

#if (defined(PF_BIG_ENDIAN_DIC) || defined(PF_LITTLE_ENDIAN_DIC))
            if( IN_DICS( TOS ) )
            {
                WRITE_SHORT_DIC((uint16_t *)TOS,(uint16_t)M_POP);
            }
            else
            {
                *((uint16_t *)TOS) = (uint16_t) M_POP;
            }
#else
            *((uint16_t *)TOS) = (uint16_t) M_POP;
#endif
            M_DROP;
            endcase;

        case ID_XOR: BINARY_OP( ^ ); endcase;


/* Branch is followed by an offset relative to address of offset. */
        case ID_ZERO_BRANCH:
DBUGX(("Before 0Branch: IP = 0x%x\n", InsPtr ));
            if( TOS == 0 )
            {
                M_BRANCH;
            }
            else
            {
                InsPtr++;      /* skip over offset */
            }
            M_DROP;
DBUGX(("After 0Branch: IP = 0x%x\n", InsPtr ));
            endcase;

        //************************************************************ 
        // Raylib words
        //************************************************************
        // rcore - Window-related functions
        case XT_INIT_WINDOW: { /* ( +width +height +title -- ) */
          // RAYLIB: void InitWindow(int width, int height, const char *title);
          ERR("InitWindow not implemented\n");
        } break;
        case XT_CLOSE_WINDOW: { /* ( -- ) */
          // RAYLIB: void CloseWindow(void);
          ERR("CloseWindow not implemented\n");
        } break;
        case XT_WINDOW_SHOULD_CLOSE: { /* ( -- bool ) */
          // RAYLIB: bool WindowShouldClose(void);
          ERR("WindowShouldClose not implemented\n");
        } break;
        case XT_IS_WINDOW_READY: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowReady(void);
          ERR("IsWindowReady not implemented\n");
        } break;
        case XT_IS_WINDOW_FULLSCREEN: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowFullscreen(void);
          ERR("IsWindowFullscreen not implemented\n");
        } break;
        case XT_IS_WINDOW_HIDDEN: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowHidden(void);
          ERR("IsWindowHidden not implemented\n");
        } break;
        case XT_IS_WINDOW_MINIMIZED: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowMinimized(void);
          ERR("IsWindowMinimized not implemented\n");
        } break;
        case XT_IS_WINDOW_MAXIMIZED: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowMaximized(void);
          ERR("IsWindowMaximized not implemented\n");
        } break;
        case XT_IS_WINDOW_FOCUSED: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowFocused(void);
          ERR("IsWindowFocused not implemented\n");
        } break;
        case XT_IS_WINDOW_RESIZED: { /* ( -- bool ) */
          // RAYLIB: bool IsWindowResized(void);
          ERR("IsWindowResized not implemented\n");
        } break;
        case XT_IS_WINDOW_STATE: { /* ( +flag -- bool ) */
          // RAYLIB: bool IsWindowState(unsigned int flag);
          ERR("IsWindowState not implemented\n");
        } break;
        case XT_SET_WINDOW_STATE: { /* ( +flags -- ) */
          // RAYLIB: void SetWindowState(unsigned int flags);
          ERR("SetWindowState not implemented\n");
        } break;
        case XT_CLEAR_WINDOW_STATE: { /* ( +flags -- ) */
          // RAYLIB: void ClearWindowState(unsigned int flags);
          ERR("ClearWindowState not implemented\n");
        } break;
        case XT_TOGGLE_FULLSCREEN: { /* ( -- ) */
          // RAYLIB: void ToggleFullscreen(void);
          ERR("ToggleFullscreen not implemented\n");
        } break;
        case XT_TOGGLE_BORDERLESS_WINDOWED: { /* ( -- ) */
          // RAYLIB: void ToggleBorderlessWindowed(void);
          ERR("ToggleBorderlessWindowed not implemented\n");
        } break;
        case XT_MAXIMIZE_WINDOW: { /* ( -- ) */
          // RAYLIB: void MaximizeWindow(void);
          ERR("MaximizeWindow not implemented\n");
        } break;
        case XT_MINIMIZE_WINDOW: { /* ( -- ) */
          // RAYLIB: void MinimizeWindow(void);
          ERR("MinimizeWindow not implemented\n");
        } break;
        case XT_RESTORE_WINDOW: { /* ( -- ) */
          // RAYLIB: void RestoreWindow(void);
          ERR("RestoreWindow not implemented\n");
        } break;
        case XT_SET_WINDOW_ICON: { /* ( +image -- ) */
          // RAYLIB: void SetWindowIcon(Image image);
          ERR("SetWindowIcon not implemented\n");
        } break;
        case XT_SET_WINDOW_ICONS: { /* ( +images +count -- ) */
          // RAYLIB: void SetWindowIcons(Image *images, int count);
          ERR("SetWindowIcons not implemented\n");
        } break;
        case XT_SET_WINDOW_TITLE: { /* ( +title -- ) */
          // RAYLIB: void SetWindowTitle(const char *title);
          ERR("SetWindowTitle not implemented\n");
        } break;
        case XT_SET_WINDOW_POSITION: { /* ( +x +y -- ) */
          // RAYLIB: void SetWindowPosition(int x, int y);
          ERR("SetWindowPosition not implemented\n");
        } break;
        case XT_SET_WINDOW_MONITOR: { /* ( +monitor -- ) */
          // RAYLIB: void SetWindowMonitor(int monitor);
          ERR("SetWindowMonitor not implemented\n");
        } break;
        case XT_SET_WINDOW_MIN_SIZE: { /* ( +width +height -- ) */
          // RAYLIB: void SetWindowMinSize(int width, int height);
          ERR("SetWindowMinSize not implemented\n");
        } break;
        case XT_SET_WINDOW_MAX_SIZE: { /* ( +width +height -- ) */
          // RAYLIB: void SetWindowMaxSize(int width, int height);
          ERR("SetWindowMaxSize not implemented\n");
        } break;
        case XT_SET_WINDOW_SIZE: { /* ( +width +height -- ) */
          // RAYLIB: void SetWindowSize(int width, int height);
          ERR("SetWindowSize not implemented\n");
        } break;
        case XT_SET_WINDOW_OPACITY: { /* ( +opacity -- ) */
          // RAYLIB: void SetWindowOpacity(float opacity);
          ERR("SetWindowOpacity not implemented\n");
        } break;
        case XT_SET_WINDOW_FOCUSED: { /* ( -- ) */
            // RAYLIB: void SetWindowFocused(void);
            break;
        }
        case XT_GET_WINDOW_HANDLE: { /* ( -- void* ) */
            // RAYLIB: void *GetWindowHandle(void);
            break;
        }
        case XT_GET_SCREEN_WIDTH: { /* ( -- int ) */
            // RAYLIB: int GetScreenWidth(void);
            break;
        }
        case XT_GET_SCREEN_HEIGHT: { /* ( -- int ) */
            // RAYLIB: int GetScreenHeight(void);
            break;
        }
        case XT_GET_RENDER_WIDTH: { /* ( -- int ) */
            // RAYLIB: int GetRenderWidth(void);
            break;
        }
        case XT_GET_RENDER_HEIGHT: { /* ( -- int ) */
            // RAYLIB: int GetRenderHeight(void);
            break;
        }
        case XT_GET_MONITOR_COUNT: { /* ( -- int ) */
            // RAYLIB: int GetMonitorCount(void);
            break;
        }
        case XT_GET_CURRENT_MONITOR: { /* ( -- int ) */
            // RAYLIB: int GetCurrentMonitor(void);
            break;
        }
        case XT_GET_MONITOR_POSITION: { /* ( +monitor -- Vector2 ) */
            // RAYLIB: Vector2 GetMonitorPosition(int monitor);
            break;
        }
        case XT_GET_MONITOR_WIDTH: { /* ( +monitor -- int ) */
            // RAYLIB: int GetMonitorWidth(int monitor);
            break;
        }
        case XT_GET_MONITOR_HEIGHT: { /* ( +monitor -- int ) */
            // RAYLIB: int GetMonitorHeight(int monitor);
            break;
        }
        case XT_GET_MONITOR_PHYSICAL_WIDTH: { /* ( +monitor -- int ) */
            // RAYLIB: int GetMonitorPhysicalWidth(int monitor);
            break;
        }
        case XT_GET_MONITOR_PHYSICAL_HEIGHT: { /* ( +monitor -- int ) */
            // RAYLIB: int GetMonitorPhysicalHeight(int monitor);
            break;
        }
        case XT_GET_MONITOR_REFRESH_RATE: { /* ( +monitor -- int ) */
            // RAYLIB: int GetMonitorRefreshRate(int monitor);
            break;
        }
        case XT_GET_WINDOW_POSITION: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetWindowPosition(void);
            break;
        }
        case XT_GET_WINDOW_SCALE_DPI: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetWindowScaleDPI(void);
            break;
        }
        case XT_GET_MONITOR_NAME: { /* ( +monitor -- const char* ) */
            // RAYLIB: const char *GetMonitorName(int monitor);
            break;
        }
        case XT_SET_CLIPBOARD_TEXT: { /* ( +text -- ) */
            // RAYLIB: void SetClipboardText(const char *text);
            break;
        }
        case XT_GET_CLIPBOARD_TEXT: { /* ( -- const char* ) */
            // RAYLIB: const char *GetClipboardText(void);
            break;
        }
        case XT_ENABLE_EVENT_WAITING: { /* ( -- ) */
            // RAYLIB: void EnableEventWaiting(void);
            break;
        }
        case XT_DISABLE_EVENT_WAITING: { /* ( -- ) */
            // RAYLIB: void DisableEventWaiting(void);
            break;
        }
        // rcore - Cursor-related functions
        case XT_SHOW_CURSOR: { /* ( -- ) */
          // RAYLIB: void ShowCursor(void);
          break;
        }
        case XT_HIDE_CURSOR: { /* ( -- ) */
          // RAYLIB: void HideCursor(void);
          break;
        }
        case XT_IS_CURSOR_HIDDEN: { /* ( -- bool ) */
          // RAYLIB: bool IsCursorHidden(void);
          break;
        }
        case XT_ENABLE_CURSOR: { /* ( -- ) */
          // RAYLIB: void EnableCursor(void);
          break;
        }
        case XT_DISABLE_CURSOR: { /* ( -- ) */
          // RAYLIB: void DisableCursor(void);
          break;
        }
        case XT_IS_CURSOR_ON_SCREEN: { /* ( -- bool ) */
          // RAYLIB: bool IsCursorOnScreen(void);
          break;
        }
        // rcore - Drawing-related functions
        case XT_CLEAR_BACKGROUND: { /* ( +color -- ) */
          // RAYLIB: void ClearBackground(Color color);
          break;
        }
        case XT_BEGIN_DRAWING: { /* ( -- ) */
          // RAYLIB: void BeginDrawing(void);
          break;
        }
        case XT_END_DRAWING: { /* ( -- ) */
          // RAYLIB: void EndDrawing(void);
          break;
        }
        case XT_BEGIN_MODE2D: { /* ( +camera2D -- ) */
          // RAYLIB: void BeginMode2D(Camera2D camera);
          break;
        }
        case XT_END_MODE2D: { /* ( -- ) */
          // RAYLIB: void EndMode2D(void);
          break;
        }
        case XT_BEGIN_MODE3D: { /* ( +camera3D -- ) */
          // RAYLIB: void BeginMode3D(Camera3D camera);
          break;
        }
        case XT_END_MODE3D: { /* ( -- ) */
          // RAYLIB: void EndMode3D(void);
          break;
        }
        case XT_BEGIN_TEXTURE_MODE: { /* ( +renderTexture2D -- ) */
          // RAYLIB: void BeginTextureMode(RenderTexture2D target);
          break;
        }
        case XT_END_TEXTURE_MODE: { /* ( -- ) */
          // RAYLIB: void EndTextureMode(void);
          break;
        }
        case XT_BEGIN_SHADER_MODE: { /* ( +shader -- ) */
          // RAYLIB: void BeginShaderMode(Shader shader);
          break;
        }
        case XT_END_SHADER_MODE: { /* ( -- ) */
          // RAYLIB: void EndShaderMode(void);
          break;
        }
        case XT_BEGIN_BLEND_MODE: { /* ( +mode -- ) */
          // RAYLIB: void BeginBlendMode(int mode);
          break;
        }
        case XT_END_BLEND_MODE: { /* ( -- ) */
          // RAYLIB: void EndBlendMode(void);
          break;
        }
        case XT_BEGIN_SCISSOR_MODE: { /* ( +x +y +width +height -- ) */
          // RAYLIB: void BeginScissorMode(int x, int y, int width, int height);
          break;
        }
        case XT_END_SCISSOR_MODE: { /* ( -- ) */
          // RAYLIB: void EndScissorMode(void);
          break;
        }
        case XT_BEGIN_VR_STEREO_MODE: { /* ( +vrStereoConfig -- ) */
          // RAYLIB: void BeginVrStereoMode(VrStereoConfig config);
          break;
        }
        case XT_END_VR_STEREO_MODE: { /* ( -- ) */
          // RAYLIB: void EndVrStereoMode(void);
          break;
        }
        // rcore - VR stereo config functions for VR simulator
        case XT_LOAD_VR_STEREO_CONFIG: { /* ( +vrDeviceInfo -- vrStereoConfig )
                                          */
          // RAYLIB: VrStereoConfig LoadVrStereoConfig(VrDeviceInfo device);
          break;
        }
        case XT_UNLOAD_VR_STEREO_CONFIG: { /* ( +vrStereoConfig -- ) */
          // RAYLIB: void UnloadVrStereoConfig(VrStereoConfig config);
          break;
        }
          // rcore - Shader management functions
        case XT_LOAD_SHADER: { /* ( +vsFileName +fsFileName -- shader ) */
          // RAYLIB: Shader LoadShader(const char *vsFileName, const char
          // *fsFileName);
          break;
        }
        case XT_LOAD_SHADER_FROM_MEMORY: { /* ( +vsCode +fsCode -- shader ) */
          // RAYLIB: Shader LoadShaderFromMemory(const char *vsCode, const char
          // *fsCode);
          break;
        }
        case XT_IS_SHADER_READY: { /* ( +shader -- bool ) */
          // RAYLIB: bool IsShaderReady(Shader shader);
          break;
        }
        case XT_GET_SHADER_LOCATION: { /* ( +shader +uniformName -- int ) */
          // RAYLIB: int GetShaderLocation(Shader shader, const char
          // *uniformName);
          break;
        }
        case XT_GET_SHADER_LOCATION_ATTRIB: { /* ( +shader +attribName -- int ) */
          // RAYLIB: int GetShaderLocationAttrib(Shader shader, const char
          // *attribName);
          break;
        }
        case XT_SET_SHADER_VALUE: { /* ( +shader +locIndex +value +uniformType  -- ) */
          // RAYLIB: void SetShaderValue(Shader shader, int locIndex, const void
          // *value, int uniformType);
          break;
        }
        case XT_SET_SHADER_VALUE_V: { /* ( +shader +locIndex +value +uniformType +count -- ) */
          // RAYLIB: void SetShaderValueV(Shader shader, int locIndex, const
          // void *value, int uniformType, int count);
          break;
        }
        case XT_SET_SHADER_VALUE_MATRIX: { /* ( +shader +locIndex +mat -- ) */
          // RAYLIB: void SetShaderValueMatrix(Shader shader, int locIndex,
          // Matrix mat);
          break;
        }
        case XT_SET_SHADER_VALUE_TEXTURE: { /* ( +shader +locIndex +texture -- ) */
          // RAYLIB: void SetShaderValueTexture(Shader shader, int locIndex,
          // Texture2D texture);
          break;
        }
        case XT_UNLOAD_SHADER: { /* ( +shader -- ) */
          // RAYLIB: void UnloadShader(Shader shader);
          break;
        }
        // rcore - Screen-space-related functions
        case XT_GET_MOUSE_RAY: { /* ( +mousePosition +camera -- ray ) */
          // RAYLIB: Ray GetMouseRay(Vector2 mousePosition, Camera camera);
          break;
        }
        case XT_GET_CAMERA_MATRIX: { /* ( +camera -- matrix ) */
          // RAYLIB: Matrix GetCameraMatrix(Camera camera);
          break;
        }
        case XT_GET_CAMERA_MATRIX_2D: { /* ( +camera2D -- matrix ) */
          // RAYLIB: Matrix GetCameraMatrix2D(Camera2D camera);
          break;
        }
        case XT_GET_WORLD_TO_SCREEN: { /* ( +position3D +camera -- vector2 ) */
          // RAYLIB: Vector2 GetWorldToScreen(Vector3 position, Camera camera);
          break;
        }
        case XT_GET_SCREEN_TO_WORLD_2D: { /* ( +position2D +camera2D -- vector2
                                             ) */
          // RAYLIB: Vector2 GetScreenToWorld2D(Vector2 position, Camera2D
          // camera);
          break;
        }
        case XT_GET_WORLD_TO_SCREEN_EX: { /* ( +position3D +camera +width
                                             +height -- vector2 ) */
          // RAYLIB: Vector2 GetWorldToScreenEx(Vector3 position, Camera camera,
          // int width, int height);
          break;
        }
        case XT_GET_WORLD_TO_SCREEN_2D: { /* ( +position2D +camera2D -- vector2 ) */
          // RAYLIB: Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera);
        } break; 
        // rcore - Timing-related functions
        case XT_SET_TARGET_FPS: { /* ( +fps -- ) */
          // RAYLIB: void SetTargetFPS(int fps);
        } break; 
        case XT_GET_FRAME_TIME: { /* ( -- float ) */
          // RAYLIB: float GetFrameTime(void);
        } break; 
        case XT_GET_TIME: { /* ( -- double ) */
          // RAYLIB: double GetTime(void);
        } break; 
        case XT_GET_FPS: { /* ( -- int ) */
          // RAYLIB: int GetFPS(void);
        } break; 
        // rcore - Custom frame control functions
        case XT_SWAP_SCREEN_BUFFER: { /* ( -- ) */
          // RAYLIB: void SwapScreenBuffer(void);
        } break;
        case XT_POLL_INPUT_EVENTS: { /* ( -- ) */
          // RAYLIB: void PollInputEvents(void);
        } break;
        case XT_WAIT_TIME: { /* ( +seconds -- ) */
          // RAYLIB: void WaitTime(double seconds);
        } break;
        // rcore - Random values generation functions
        case XT_SET_RANDOM_SEED: { /* ( +seed -- ) */
          // RAYLIB: void SetRandomSeed(unsigned int seed);
        } break;
        case XT_GET_RANDOM_VALUE: { /* ( +min +max -- int ) */
          // RAYLIB: int GetRandomValue(int min, int max);
        } break;
        case XT_LOAD_RANDOM_SEQUENCE: { /* ( +count +min +max -- int* ) */
          // RAYLIB: int *LoadRandomSequence(unsigned int count, int min, int
          // max);
        } break;
        case XT_UNLOAD_RANDOM_SEQUENCE: { /* ( +sequence -- ) */
          // RAYLIB: void UnloadRandomSequence(int *sequence);
        } break;
        // rcore - Misc. functions
        case XT_TAKE_SCREENSHOT: { /* ( +fileName -- ) */
          // RAYLIB: void TakeScreenshot(const char *fileName);
        } break;
        case XT_SET_CONFIG_FLAGS: { /* ( +flags -- ) */
          // RAYLIB: void SetConfigFlags(unsigned int flags);
        } break;
        case XT_OPEN_URL: { /* ( +url -- ) */
          // RAYLIB: void OpenURL(const char *url);
        } break;
        case XT_TRACE_LOG: { /* ( +logLevel +text ... -- ) */
          // RAYLIB: void TraceLog(int logLevel, const char *text, ...);
        } break;
        case XT_SET_TRACE_LOG_LEVEL: { /* ( +logLevel -- ) */
          // RAYLIB: void SetTraceLogLevel(int logLevel);
        } break;
        case XT_MEM_ALLOC: { /* ( +size -- void* ) */
                             // RAYLIB: void *MemAlloc(unsigned int size);
        } break;
        case XT_MEM_REALLOC: { /* ( +ptr +size -- void* ) */
          // RAYLIB: void *MemRealloc(void *ptr, unsigned int size);
        } break;
        case XT_MEM_FREE: { /* ( +ptr -- ) */
                            // RAYLIB: void MemFree(void *ptr);
        } break;
        //
        // rcore - Set custom callbacks
        case XT_SET_TRACE_LOG_CALLBACK: { /* ( +callback -- ) */
          // RAYLIB: void SetTraceLogCallback(TraceLogCallback callback);
        } break;
        case XT_SET_LOAD_FILE_DATA_CALLBACK: { /* ( +callback -- ) */
          // RAYLIB: void SetLoadFileDataCallback(LoadFileDataCallback
          // callback);
        } break;
        case XT_SET_SAVE_FILE_DATA_CALLBACK: { /* ( +callback -- ) */
          // RAYLIB: void SetSaveFileDataCallback(SaveFileDataCallback
          // callback);
        } break;
        case XT_SET_LOAD_FILE_TEXT_CALLBACK: { /* ( +callback -- ) */
          // RAYLIB: void SetLoadFileTextCallback(LoadFileTextCallback
          // callback);
        } break;
        case XT_SET_SAVE_FILE_TEXT_CALLBACK: { /* ( +callback -- ) */
          // RAYLIB: void SetSaveFileTextCallback(SaveFileTextCallback
          // callback);
        } break;
        //
        // rcore - Files management functions
        case XT_LOAD_FILE_DATA: { /* ( +fileName +dataSize -- unsigned char* )
                                   */
          // RAYLIB: unsigned char *LoadFileData(const char *fileName, int
          // *dataSize);
        } break;
        case XT_UNLOAD_FILE_DATA: { /* ( +data -- ) */
          // RAYLIB: void UnloadFileData(unsigned char *data);
        } break;
        case XT_SAVE_FILE_DATA: { /* ( +fileName +data +dataSize -- bool ) */
          // RAYLIB: bool SaveFileData(const char *fileName, void *data, int
          // dataSize);
        } break;
        case XT_EXPORT_DATA_AS_CODE: { /* ( +data +dataSize +fileName -- bool )
                                        */
          // RAYLIB: bool ExportDataAsCode(const unsigned char *data, int
          // dataSize, const char *fileName);
        } break;
        case XT_LOAD_FILE_TEXT: { /* ( +fileName -- char* ) */
          // RAYLIB: char *LoadFileText(const char *fileName);
        } break;
        case XT_UNLOAD_FILE_TEXT: { /* ( +text -- ) */
                                    // RAYLIB: void UnloadFileText(char *text);
        } break;
        case XT_SAVE_FILE_TEXT: { /* ( +fileName +text -- bool ) */
          // RAYLIB: bool SaveFileText(const char *fileName, char *text);
        } break;
        case XT_FILE_EXISTS: { /* ( +fileName -- bool ) */
                               // RAYLIB: bool FileExists(const char *fileName);
        } break;
        case XT_DIRECTORY_EXISTS: { /* ( +dirPath -- bool ) */
          // RAYLIB: bool DirectoryExists(const char *dirPath);
        } break;
        case XT_IS_FILE_EXTENSION: { /* ( +fileName +ext -- bool ) */
          // RAYLIB: bool IsFileExtension(const char *fileName, const char
          // *ext);
        } break;
        case XT_GET_FILE_LENGTH: { /* ( +fileName -- int ) */
          // RAYLIB: int GetFileLength(const char *fileName);
        } break;
        case XT_GET_FILE_EXTENSION: { /* ( +fileName -- const char* ) */
          // RAYLIB: const char *GetFileExtension(const char *fileName);
        } break;
        case XT_GET_FILE_NAME: { /* ( +filePath -- const char* ) */
          // RAYLIB: const char *GetFileName(const char *filePath);
        } break;
        case XT_GET_FILE_NAME_WITHOUT_EXT: { /* ( +filePath -- const char* ) */
          // RAYLIB: const char *GetFileNameWithoutExt(const char *filePath);
        } break;
        case XT_GET_DIRECTORY_PATH: { /* ( +filePath -- const char* ) */
          // RAYLIB: const char *GetDirectoryPath(const char *filePath);
        } break;
        case XT_GET_PREV_DIRECTORY_PATH: { /* ( +dirPath -- const char* ) */
          // RAYLIB: const char *GetPrevDirectoryPath(const char *dirPath);
        } break;
        case XT_GET_WORKING_DIRECTORY: { /* ( -- const char* ) */
          // RAYLIB: const char *GetWorkingDirectory(void);
        } break;
        case XT_GET_APPLICATION_DIRECTORY: { /* ( -- const char* ) */
          // RAYLIB: const char *GetApplicationDirectory(void);
        } break;
        case XT_CHANGE_DIRECTORY: { /* ( +dir -- bool ) */
          // RAYLIB: bool ChangeDirectory(const char *dir);
        } break;
        case XT_IS_PATH_FILE: { /* ( +path -- bool ) */
                                // RAYLIB: bool IsPathFile(const char *path);
        } break;
        case XT_LOAD_DIRECTORY_FILES: { /* ( +dirPath -- FilePathList ) */
          // RAYLIB: FilePathList LoadDirectoryFiles(const char *dirPath);
        } break;
        case XT_LOAD_DIRECTORY_FILES_EX: { /* ( +basePath +filter +scanSubdirs
                                              -- FilePathList ) */
          // RAYLIB: FilePathList LoadDirectoryFilesEx(const char *basePath,
          // const char *filter, bool scanSubdirs);
        } break;
        case XT_UNLOAD_DIRECTORY_FILES: { /* ( +files -- ) */
          // RAYLIB: void UnloadDirectoryFiles(FilePathList files);
        } break;
        case XT_IS_FILE_DROPPED: { /* ( -- bool ) */
                                   // RAYLIB: bool IsFileDropped(void);
        } break;
        case XT_LOAD_DROPPED_FILES: { /* ( -- FilePathList ) */
          // RAYLIB: FilePathList LoadDroppedFiles(void);
        } break;
        case XT_UNLOAD_DROPPED_FILES: { /* ( +files -- ) */
          // RAYLIB: void UnloadDroppedFiles(FilePathList files);
        } break;
        case XT_GET_FILE_MOD_TIME: { /* ( +fileName -- long ) */
          // RAYLIB: long GetFileModTime(const char *fileName);
        } break;
        //
        // rcore - Compression/Encoding functionality
        case XT_COMPRESS_DATA: { /* ( +data +dataSize -- unsigned char*
                                    +compDataSize ) */
          // RAYLIB: unsigned char *CompressData(const unsigned char *data, int
          // dataSize, int *compDataSize);
        } break;
        case XT_DECOMPRESS_DATA: { /* ( +compData +compDataSize -- unsigned
                                      char* +dataSize ) */
          // RAYLIB: unsigned char *DecompressData(const unsigned char
          // *compData, int compDataSize, int *dataSize);
        } break;
        case XT_ENCODE_DATA_BASE64: { /* ( +data +dataSize -- char* +outputSize
                                         ) */
          // RAYLIB: char *EncodeDataBase64(const unsigned char *data, int
          // dataSize, int *outputSize);
        } break;
        case XT_DECODE_DATA_BASE64: { /* ( +data -- unsigned char* +outputSize )
                                       */
          // RAYLIB: unsigned char *DecodeDataBase64(const unsigned char *data,
          // int *outputSize);
        } break;
        //
        // rcore - Automation events functionality
        case XT_LOAD_AUTOMATION_EVENT_LIST: { /* ( +fileName -- AutomationEventList ) */
            // RAYLIB: AutomationEventList LoadAutomationEventList(const char *fileName);
        } break;
        case XT_UNLOAD_AUTOMATION_EVENT_LIST: { /* ( +list -- ) */
            // RAYLIB: void UnloadAutomationEventList(AutomationEventList *list);
        } break;
        case XT_EXPORT_AUTOMATION_EVENT_LIST: { /* ( +list +fileName -- bool ) */
            // RAYLIB: bool ExportAutomationEventList(AutomationEventList list, const char *fileName);
        } break;
        case XT_SET_AUTOMATION_EVENT_LIST: { /* ( +list -- ) */
            // RAYLIB: void SetAutomationEventList(AutomationEventList *list);
        } break;
        case XT_SET_AUTOMATION_EVENT_BASE_FRAME: { /* ( +frame -- ) */
            // RAYLIB: void SetAutomationEventBaseFrame(int frame);
        } break;
        case XT_START_AUTOMATION_EVENT_RECORDING: { /* ( -- ) */
            // RAYLIB: void StartAutomationEventRecording(void);
        } break;
        case XT_STOP_AUTOMATION_EVENT_RECORDING: { /* ( -- ) */
            // RAYLIB: void StopAutomationEventRecording(void);
        } break;
        case XT_PLAY_AUTOMATION_EVENT: { /* ( +event -- ) */
            // RAYLIB: void PlayAutomationEvent(AutomationEvent event);
        } break;
        //
        // rcore - Input-related functions: keyboard
        case XT_IS_KEY_PRESSED: { /* ( +key -- bool ) */
            // RAYLIB: bool IsKeyPressed(int key);
        } break;
        case XT_IS_KEY_PRESSED_REPEAT: { /* ( +key -- bool ) */
            // RAYLIB: bool IsKeyPressedRepeat(int key);
        } break;
        case XT_IS_KEY_DOWN: { /* ( +key -- bool ) */
            // RAYLIB: bool IsKeyDown(int key);
        } break;
        case XT_IS_KEY_RELEASED: { /* ( +key -- bool ) */
            // RAYLIB: bool IsKeyReleased(int key);
        } break;
        case XT_IS_KEY_UP: { /* ( +key -- bool ) */
            // RAYLIB: bool IsKeyUp(int key);
        } break;
        case XT_GET_KEY_PRESSED: { /* ( -- int ) */
            // RAYLIB: int GetKeyPressed(void);
        } break;
        case XT_GET_CHAR_PRESSED: { /* ( -- int ) */
            // RAYLIB: int GetCharPressed(void);
        } break;
        case XT_SET_EXIT_KEY: { /* ( +key -- ) */
            // RAYLIB: void SetExitKey(int key);
        } break;
        //
        // rcore - Input-related functions: gamepads
        case XT_IS_GAMEPAD_AVAILABLE: { /* ( +gamepad -- bool ) */
            // RAYLIB: bool IsGamepadAvailable(int gamepad);
        } break;
        case XT_GET_GAMEPAD_NAME: { /* ( +gamepad -- const char* ) */
            // RAYLIB: const char *GetGamepadName(int gamepad);
        } break;
        case XT_IS_GAMEPAD_BUTTON_PRESSED: { /* ( +gamepad +button -- bool ) */
            // RAYLIB: bool IsGamepadButtonPressed(int gamepad, int button);
        } break;
        case XT_IS_GAMEPAD_BUTTON_DOWN: { /* ( +gamepad +button -- bool ) */
            // RAYLIB: bool IsGamepadButtonDown(int gamepad, int button);
        } break;
        case XT_IS_GAMEPAD_BUTTON_RELEASED: { /* ( +gamepad +button -- bool ) */
            // RAYLIB: bool IsGamepadButtonReleased(int gamepad, int button);
        } break;
        case XT_IS_GAMEPAD_BUTTON_UP: { /* ( +gamepad +button -- bool ) */
            // RAYLIB: bool IsGamepadButtonUp(int gamepad, int button);
        } break;
        case XT_GET_GAMEPAD_BUTTON_PRESSED: { /* ( -- int ) */
            // RAYLIB: int GetGamepadButtonPressed(void);
        } break;
        case XT_GET_GAMEPAD_AXIS_COUNT: { /* ( +gamepad -- int ) */
            // RAYLIB: int GetGamepadAxisCount(int gamepad);
        } break;
        case XT_GET_GAMEPAD_AXIS_MOVEMENT: { /* ( +gamepad +axis -- float ) */
            // RAYLIB: float GetGamepadAxisMovement(int gamepad, int axis);
        } break;
        case XT_SET_GAMEPAD_MAPPINGS: { /* ( +mappings -- int ) */
            // RAYLIB: int SetGamepadMappings(const char *mappings);
        } break;
        //
        // rcore - Input-related functions: mouse
        case XT_IS_MOUSE_BUTTON_PRESSED: { /* ( +button -- bool ) */
            // RAYLIB: bool IsMouseButtonPressed(int button);
        } break;
        case XT_IS_MOUSE_BUTTON_DOWN: { /* ( +button -- bool ) */
            // RAYLIB: bool IsMouseButtonDown(int button);
        } break;
        case XT_IS_MOUSE_BUTTON_RELEASED: { /* ( +button -- bool ) */
            // RAYLIB: bool IsMouseButtonReleased(int button);
        } break;
        case XT_IS_MOUSE_BUTTON_UP: { /* ( +button -- bool ) */
            // RAYLIB: bool IsMouseButtonUp(int button);
        } break;
        case XT_GET_MOUSE_X: { /* ( -- int ) */
            // RAYLIB: int GetMouseX(void);
        } break;
        case XT_GET_MOUSE_Y: { /* ( -- int ) */
            // RAYLIB: int GetMouseY(void);
        } break;
        case XT_GET_MOUSE_POSITION: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetMousePosition(void);
        } break;
        case XT_GET_MOUSE_DELTA: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetMouseDelta(void);
        } break;
        case XT_SET_MOUSE_POSITION: { /* ( +x +y -- ) */
            // RAYLIB: void SetMousePosition(int x, int y);
        } break;
        case XT_SET_MOUSE_OFFSET: { /* ( +offsetX +offsetY -- ) */
            // RAYLIB: void SetMouseOffset(int offsetX, int offsetY);
        } break;
        case XT_SET_MOUSE_SCALE: { /* ( +scaleX +scaleY -- ) */
            // RAYLIB: void SetMouseScale(float scaleX, float scaleY);
        } break;
        case XT_GET_MOUSE_WHEEL_MOVE: { /* ( -- float ) */
            // RAYLIB: float GetMouseWheelMove(void);
        } break;
        case XT_GET_MOUSE_WHEEL_MOVE_V: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetMouseWheelMoveV(void);
        } break;
        case XT_SET_MOUSE_CURSOR: { /* ( +cursor -- ) */
            // RAYLIB: void SetMouseCursor(int cursor);
        } break;
        //
        // rcore - Input-related functions: touch
        case XT_GET_TOUCH_X: { /* ( -- int ) */
            // RAYLIB: int GetTouchX(void);
        } break;
        case XT_GET_TOUCH_Y: { /* ( -- int ) */
            // RAYLIB: int GetTouchY(void);
        } break;
        case XT_GET_TOUCH_POSITION: { /* ( +index -- Vector2 ) */
            // RAYLIB: Vector2 GetTouchPosition(int index);
        } break;
        case XT_GET_TOUCH_POINT_ID: { /* ( +index -- int ) */
            // RAYLIB: int GetTouchPointId(int index);
        } break;
        case XT_GET_TOUCH_POINT_COUNT: { /* ( -- int ) */
            // RAYLIB: int GetTouchPointCount(void);
        } break;
        //
        // Gestures and Touch Handling Functions (Module: rgestures)
        case XT_SET_GESTURES_ENABLED: { /* ( +flags -- ) */
            // RAYLIB: void SetGesturesEnabled(unsigned int flags);
        } break;
        case XT_IS_GESTURE_DETECTED: { /* ( +gesture -- bool ) */
            // RAYLIB: bool IsGestureDetected(unsigned int gesture);
        } break;
        case XT_GET_GESTURE_DETECTED: { /* ( -- int ) */
            // RAYLIB: int GetGestureDetected(void);
        } break;
        case XT_GET_GESTURE_HOLD_DURATION: { /* ( -- float ) */
            // RAYLIB: float GetGestureHoldDuration(void);
        } break;
        case XT_GET_GESTURE_DRAG_VECTOR: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetGestureDragVector(void);
        } break;
        case XT_GET_GESTURE_DRAG_ANGLE: { /* ( -- float ) */
            // RAYLIB: float GetGestureDragAngle(void);
        } break;
        case XT_GET_GESTURE_PINCH_VECTOR: { /* ( -- Vector2 ) */
            // RAYLIB: Vector2 GetGesturePinchVector(void);
        } break;
        case XT_GET_GESTURE_PINCH_ANGLE: { /* ( -- float ) */
            // RAYLIB: float GetGesturePinchAngle(void);
        } break;
        //
        // rcore - Camera System Functions (Module: rcamera)
        case XT_UPDATE_CAMERA: { /* ( +camera +mode -- ) */
            // RAYLIB: void UpdateCamera(Camera *camera, int mode);
        } break;
        case XT_UPDATE_CAMERA_PRO: { /* ( +camera +movement +rotation +zoom -- ) */
            // RAYLIB: void UpdateCameraPro(Camera *camera, Vector3 movement, Vector3 rotation, float zoom);
        } break;
        //
        // rshape - Basic shapes drawing functions
        case XT_SET_SHAPES_TEXTURE: { /* ( +texture +source -- ) */
          // RAYLIB: void SetShapesTexture(Texture2D texture, Rectangle source);
        } break;
        case XT_DRAW_PIXEL: { /* ( +posX +posY +color -- ) */
            // RAYLIB: void DrawPixel(int posX, int posY, Color color);
        } break;
        case XT_DRAW_PIXEL_V: { /* ( +position +color -- ) */
            // RAYLIB: void DrawPixelV(Vector2 position, Color color);
        } break;
        case XT_DRAW_LINE: { /* ( +startPosX +startPosY +endPosX +endPosY +color -- ) */
            // RAYLIB: void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
        } break;
        case XT_DRAW_LINE_V: { /* ( +startPos +endPos +color -- ) */
            // RAYLIB: void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
        } break;
        case XT_DRAW_LINE_EX: { /* ( +startPos +endPos +thick +color -- ) */
            // RAYLIB: void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
        } break;
        case XT_DRAW_LINE_STRIP: { /* ( +points +pointCount +color -- ) */
            // RAYLIB: void DrawLineStrip(Vector2 *points, int pointCount, Color color);
        } break;
        case XT_DRAW_LINE_BEZIER: { /* ( +startPos +endPos +thick +color -- ) */
            // RAYLIB: void DrawLineBezier(Vector2 startPos, Vector2 endPos, float thick, Color color);
        } break;
        case XT_DRAW_CIRCLE: { /* ( +centerX +centerY +radius +color -- ) */
            // RAYLIB: void DrawCircle(int centerX, int centerY, float radius, Color color);
        } break;
        case XT_DRAW_CIRCLE_SECTOR: { /* ( +center +radius +startAngle +endAngle +segments +color -- ) */
            // RAYLIB: void DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);
        } break;
        case XT_DRAW_CIRCLE_SECTOR_LINES: { /* ( +center +radius +startAngle +endAngle +segments +color -- ) */
            // RAYLIB: void DrawCircleSectorLines(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);
        } break;
        case XT_DRAW_CIRCLE_GRADIENT: { /* ( +centerX +centerY +radius +color1 +color2 -- ) */
            // RAYLIB: void DrawCircleGradient(int centerX, int centerY, float radius, Color color1, Color color2);
        } break;
        case XT_DRAW_CIRCLE_V: { /* ( +center +radius +color -- ) */
            // RAYLIB: void DrawCircleV(Vector2 center, float radius, Color color);
        } break;
        case XT_DRAW_CIRCLE_LINES: { /* ( +centerX +centerY +radius +color -- ) */
            // RAYLIB: void DrawCircleLines(int centerX, int centerY, float radius, Color color);
        } break;
        case XT_DRAW_CIRCLE_LINES_V: { /* ( +center +radius +color -- ) */
            // RAYLIB: void DrawCircleLinesV(Vector2 center, float radius, Color color);
        } break;
        case XT_DRAW_ELLIPSE: { /* ( +centerX +centerY +radiusH +radiusV +color -- ) */
            // RAYLIB: void DrawEllipse(int centerX, int centerY, float radiusH, float radiusV, Color color);
        } break;
        case XT_DRAW_ELLIPSE_LINES: { /* ( +centerX +centerY +radiusH +radiusV +color -- ) */
            // RAYLIB: void DrawEllipseLines(int centerX, int centerY, float radiusH, float radiusV, Color color);
        } break;
        case XT_DRAW_RING: { /* ( +center +innerRadius +outerRadius +startAngle +endAngle +segments +color -- ) */
            // RAYLIB: void DrawRing(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color);
        } break;
        case XT_DRAW_RING_LINES: { /* ( +center +innerRadius +outerRadius +startAngle +endAngle +segments +color -- ) */
            // RAYLIB: void DrawRingLines(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color);
        } break;
        case XT_DRAW_RECTANGLE: { /* ( +posX +posY +width +height +color -- ) */
            // RAYLIB: void DrawRectangle(int posX, int posY, int width, int height, Color color);
        } break;
        case XT_DRAW_RECTANGLE_V: { /* ( +position +size +color -- ) */
            // RAYLIB: void DrawRectangleV(Vector2 position, Vector2 size, Color color);
        } break;
        case XT_DRAW_RECTANGLE_REC: { /* ( +rec +color -- ) */
            // RAYLIB: void DrawRectangleRec(Rectangle rec, Color color);
        } break;
        case XT_DRAW_RECTANGLE_PRO: { /* ( +rec +origin +rotation +color -- ) */
            // RAYLIB: void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
        } break;
        case XT_DRAW_RECTANGLE_GRADIENT_V: { /* ( +posX +posY +width +height +color1 +color2 -- ) */
            // RAYLIB: void DrawRectangleGradientV(int posX, int posY, int width, int height, Color color1, Color color2);
        } break;
        case XT_DRAW_RECTANGLE_GRADIENT_H: { /* ( +posX +posY +width +height +color1 +color2 -- ) */
            // RAYLIB: void DrawRectangleGradientH(int posX, int posY, int width, int height, Color color1, Color color2);
        } break;
        case XT_DRAW_RECTANGLE_GRADIENT_EX: { /* ( +rec +col1 +col2 +col3 +col4 -- ) */
            // RAYLIB: void DrawRectangleGradientEx(Rectangle rec, Color col1, Color col2, Color col3, Color col4);
        } break;
        case XT_DRAW_RECTANGLE_LINES: { /* ( +posX +posY +width +height +color -- ) */
            // RAYLIB: void DrawRectangleLines(int posX, int posY, int width, int height, Color color);
        } break;
        case XT_DRAW_RECTANGLE_LINES_EX: { /* ( +rec +lineThick +color -- ) */
            // RAYLIB: void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
        } break;
        case XT_DRAW_RECTANGLE_ROUNDED: { /* ( +rec +roundness +segments +color -- ) */
            // RAYLIB: void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
        } break;
        case XT_DRAW_RECTANGLE_ROUNDED_LINES: { /* ( +rec +roundness +segments +lineThick +color -- ) */
            // RAYLIB: void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, float lineThick, Color color);
        } break;
        case XT_DRAW_TRIANGLE: { /* ( +v1 +v2 +v3 +color -- ) */
            // RAYLIB: void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
        } break;
        case XT_DRAW_TRIANGLE_LINES: { /* ( +v1 +v2 +v3 +color -- ) */
            // RAYLIB: void DrawTriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
        } break;
        case XT_DRAW_TRIANGLE_FAN: { /* ( +points +pointCount +color -- ) */
            // RAYLIB: void DrawTriangleFan(Vector2 *points, int pointCount, Color color);
        } break;
        case XT_DRAW_TRIANGLE_STRIP: { /* ( +points +pointCount +color -- ) */
            // RAYLIB: void DrawTriangleStrip(Vector2 *points, int pointCount, Color color);
        } break;
        case XT_DRAW_POLY: { /* ( +center +sides +radius +rotation +color -- ) */
            // RAYLIB: void DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color);
        } break;
        case XT_DRAW_POLY_LINES: { /* ( +center +sides +radius +rotation +color -- ) */
            // RAYLIB: void DrawPolyLines(Vector2 center, int sides, float radius, float rotation, Color color);
        } break;
        case XT_DRAW_POLY_LINES_EX: { /* ( +center +sides +radius +rotation +lineThick +color -- ) */
            // RAYLIB: void DrawPolyLinesEx(Vector2 center, int sides, float radius, float rotation, float lineThick, Color color);
        } break;
        //
        // rshapes - Splines drawing functions
        case XT_DRAW_SPLINE_LINEAR: { /* ( +points +pointCount +thick +color -- ) */
            // RAYLIB: void DrawSplineLinear(Vector2 *points, int pointCount, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_BASIS: { /* ( +points +pointCount +thick +color -- ) */
            // RAYLIB: void DrawSplineBasis(Vector2 *points, int pointCount, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_CATMULL_ROM: { /* ( +points +pointCount +thick +color -- ) */
            // RAYLIB: void DrawSplineCatmullRom(Vector2 *points, int pointCount, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_BEZIER_QUADRATIC: { /* ( +points +pointCount +thick +color -- ) */
            // RAYLIB: void DrawSplineBezierQuadratic(Vector2 *points, int pointCount, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_BEZIER_CUBIC: { /* ( +points +pointCount +thick +color -- ) */
            // RAYLIB: void DrawSplineBezierCubic(Vector2 *points, int pointCount, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_SEGMENT_LINEAR: { /* ( +p1 +p2 +thick +color -- ) */
            // RAYLIB: void DrawSplineSegmentLinear(Vector2 p1, Vector2 p2, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_SEGMENT_BASIS: { /* ( +p1 +p2 +p3 +p4 +thick +color -- ) */
            // RAYLIB: void DrawSplineSegmentBasis(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_SEGMENT_CATMULL_ROM: { /* ( +p1 +p2 +p3 +p4 +thick +color -- ) */
            // RAYLIB: void DrawSplineSegmentCatmullRom(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_SEGMENT_BEZIER_QUADRATIC: { /* ( +p1 +c2 +p3 +thick +color -- ) */
            // RAYLIB: void DrawSplineSegmentBezierQuadratic(Vector2 p1, Vector2 c2, Vector2 p3, float thick, Color color);
        } break;
        case XT_DRAW_SPLINE_SEGMENT_BEZIER_CUBIC: { /* ( +p1 +c2 +c3 +p4 +thick +color -- ) */
            // RAYLIB: void DrawSplineSegmentBezierCubic(Vector2 p1, Vector2 c2, Vector2 c3, Vector2 p4, float thick, Color color);
        } break;
        //
        // rshapes - Spline segment point evaluation functions, for a given t [0.0f .. 1.0f]
        case XT_GET_SPLINE_POINT_LINEAR: { /* ( +startPos +endPos +t -- Vector2 ) */
            // RAYLIB: Vector2 GetSplinePointLinear(Vector2 startPos, Vector2 endPos, float t);
        } break;
        case XT_GET_SPLINE_POINT_BASIS: { /* ( +p1 +p2 +p3 +p4 +t -- Vector2 ) */
            // RAYLIB: Vector2 GetSplinePointBasis(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float t);
        } break;
        case XT_GET_SPLINE_POINT_CATMULL_ROM: { /* ( +p1 +p2 +p3 +p4 +t -- Vector2 ) */
            // RAYLIB: Vector2 GetSplinePointCatmullRom(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float t);
        } break;
        case XT_GET_SPLINE_POINT_BEZIER_QUAD: { /* ( +p1 +c2 +p3 +t -- Vector2 ) */
            // RAYLIB: Vector2 GetSplinePointBezierQuad(Vector2 p1, Vector2 c2, Vector2 p3, float t);
        } break;
        case XT_GET_SPLINE_POINT_BEZIER_CUBIC: { /* ( +p1 +c2 +c3 +p4 +t -- Vector2 ) */
            // RAYLIB: Vector2 GetSplinePointBezierCubic(Vector2 p1, Vector2 c2, Vector2 c3, Vector2 p4, float t);
        } break;
        //
        // rshapes - Basic shapes collision detection functions
        case XT_CHECK_COLLISION_RECS: { /* ( +rec1 +rec2 -- bool ) */
            // RAYLIB: bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
        } break;
        case XT_CHECK_COLLISION_CIRCLES: { /* ( +center1 +radius1 +center2 +radius2 -- bool ) */
            // RAYLIB: bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2, float radius2);
        } break;
        case XT_CHECK_COLLISION_CIRCLE_REC: { /* ( +center +radius +rec -- bool ) */
            // RAYLIB: bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
        } break;
        case XT_CHECK_COLLISION_POINT_REC: { /* ( +point +rec -- bool ) */
            // RAYLIB: bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
        } break;
        case XT_CHECK_COLLISION_POINT_CIRCLE: { /* ( +point +center +radius -- bool ) */
            // RAYLIB: bool CheckCollisionPointCircle(Vector2 point, Vector2 center, float radius);
        } break;
        case XT_CHECK_COLLISION_POINT_TRIANGLE: { /* ( +point +p1 +p2 +p3 -- bool ) */
            // RAYLIB: bool CheckCollisionPointTriangle(Vector2 point, Vector2 p1, Vector2 p2, Vector2 p3);
        } break;
        case XT_CHECK_COLLISION_POINT_POLY: { /* ( +point +points +pointCount -- bool ) */
            // RAYLIB: bool CheckCollisionPointPoly(Vector2 point, Vector2 *points, int pointCount);
        } break;
        case XT_CHECK_COLLISION_LINES: { /* ( +startPos1 +endPos1 +startPos2 +endPos2 +collisionPoint -- bool ) */
            // RAYLIB: bool CheckCollisionLines(Vector2 startPos1, Vector2 endPos1, Vector2 startPos2, Vector2 endPos2, Vector2 *collisionPoint);
        } break;
        case XT_CHECK_COLLISION_POINT_LINE: { /* ( +point +p1 +p2 +threshold -- bool ) */
            // RAYLIB: bool CheckCollisionPointLine(Vector2 point, Vector2 p1, Vector2 p2, int threshold);
        } break;
        case XT_GET_COLLISION_REC: { /* ( +rec1 +rec2 -- Rectangle ) */
            // RAYLIB: Rectangle GetCollisionRec(Rectangle rec1, Rectangle rec2);
        } break;
        //
        // rtextures - Image loading functions
        case XT_LOAD_IMAGE: { /* ( +fileName -- Image ) */
            // RAYLIB: Image LoadImage(const char *fileName);
        } break;
        case XT_LOAD_IMAGE_RAW: { /* ( +fileName +width +height +format +headerSize -- Image ) */
            // RAYLIB: Image LoadImageRaw(const char *fileName, int width, int height, int format, int headerSize);
        } break;
        case XT_LOAD_IMAGE_SVG: { /* ( +fileNameOrString +width +height -- Image ) */
            // RAYLIB: Image LoadImageSvg(const char *fileNameOrString, int width, int height);
        } break;
        case XT_LOAD_IMAGE_ANIM: { /* ( +fileName +frames -- Image ) */
            // RAYLIB: Image LoadImageAnim(const char *fileName, int *frames);
        } break;
        case XT_LOAD_IMAGE_FROM_MEMORY: { /* ( +fileType +fileData +dataSize -- Image ) */
            // RAYLIB: Image LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);
        } break;
        case XT_LOAD_IMAGE_FROM_TEXTURE: { /* ( +texture -- Image ) */
            // RAYLIB: Image LoadImageFromTexture(Texture2D texture);
        } break;
        case XT_LOAD_IMAGE_FROM_SCREEN: { /* ( -- Image ) */
            // RAYLIB: Image LoadImageFromScreen(void);
        } break;
        case XT_IS_IMAGE_READY: { /* ( +image -- bool ) */
            // RAYLIB: bool IsImageReady(Image image);
        } break;
        case XT_UNLOAD_IMAGE: { /* ( +image -- ) */
            // RAYLIB: void UnloadImage(Image image);
        } break;
        case XT_EXPORT_IMAGE: { /* ( +image +fileName -- bool ) */
            // RAYLIB: bool ExportImage(Image image, const char *fileName);
        } break;
        case XT_EXPORT_IMAGE_TO_MEMORY: { /* ( +image +fileType +fileSize -- unsigned char* ) */
            // RAYLIB: unsigned char *ExportImageToMemory(Image image, const char *fileType, int *fileSize);
        } break;
        case XT_EXPORT_IMAGE_AS_CODE: { /* ( +image +fileName -- bool ) */
            // RAYLIB: bool ExportImageAsCode(Image image, const char *fileName);
        } break;
        //
        // rtextures - Image generation functions
        case XT_GEN_IMAGE_COLOR: { /* ( +width +height +color -- Image ) */
            // RAYLIB: Image GenImageColor(int width, int height, Color color);
        } break;
        case XT_GEN_IMAGE_GRADIENT_LINEAR: { /* ( +width +height +direction +start +end -- Image ) */
            // RAYLIB: Image GenImageGradientLinear(int width, int height, int direction, Color start, Color end);
        } break;
        case XT_GEN_IMAGE_GRADIENT_RADIAL: { /* ( +width +height +density +inner +outer -- Image ) */
            // RAYLIB: Image GenImageGradientRadial(int width, int height, float density, Color inner, Color outer);
        } break;
        case XT_GEN_IMAGE_GRADIENT_SQUARE: { /* ( +width +height +density +inner +outer -- Image ) */
            // RAYLIB: Image GenImageGradientSquare(int width, int height, float density, Color inner, Color outer);
        } break;
        case XT_GEN_IMAGE_CHECKED: { /* ( +width +height +checksX +checksY +col1 +col2 -- Image ) */
            // RAYLIB: Image GenImageChecked(int width, int height, int checksX, int checksY, Color col1, Color col2);
        } break;
        case XT_GEN_IMAGE_WHITE_NOISE: { /* ( +width +height +factor -- Image ) */
            // RAYLIB: Image GenImageWhiteNoise(int width, int height, float factor);
        } break;
        case XT_GEN_IMAGE_PERLIN_NOISE: { /* ( +width +height +offsetX +offsetY +scale -- Image ) */
            // RAYLIB: Image GenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale);
        } break;
        case XT_GEN_IMAGE_CELLULAR: { /* ( +width +height +tileSize -- Image ) */
            // RAYLIB: Image GenImageCellular(int width, int height, int tileSize);
        } break;
        case XT_GEN_IMAGE_TEXT: { /* ( +width +height +text -- Image ) */
            // RAYLIB: Image GenImageText(int width, int height, const char *text);
        } break;
        //
        // rtextures - Image manipulation functions
        case XT_IMAGE_COPY: { /* ( +image -- Image ) */
          // RAYLIB: Image ImageCopy(Image image);
        } break;
        case XT_IMAGE_FROM_IMAGE: { /* ( +image +rec -- Image ) */
          // RAYLIB: Image ImageFromImage(Image image, Rectangle rec);
        } break;
        case XT_IMAGE_TEXT: { /* ( +text +fontSize +color -- Image ) */
            // RAYLIB: Image ImageText(const char *text, int fontSize, Color color);
        } break;
        case XT_IMAGE_TEXT_EX: { /* ( +font +text +fontSize +spacing +tint -- Image ) */
            // RAYLIB: Image ImageTextEx(Font font, const char *text, float fontSize, float spacing, Color tint);
        } break;
        case XT_IMAGE_FORMAT: { /* ( +image +newFormat -- ) */
            // RAYLIB: void ImageFormat(Image *image, int newFormat);
        } break;
        case XT_IMAGE_TO_POT: { /* ( +image +fill -- ) */
            // RAYLIB: void ImageToPOT(Image *image, Color fill);
        } break;
        case XT_IMAGE_CROP: { /* ( +image +crop -- ) */
            // RAYLIB: void ImageCrop(Image *image, Rectangle crop);
        } break;
        case XT_IMAGE_ALPHA_CROP: { /* ( +image +threshold -- ) */
            // RAYLIB: void ImageAlphaCrop(Image *image, float threshold);
        } break;
        case XT_IMAGE_ALPHA_CLEAR: { /* ( +image +color +threshold -- ) */
            // RAYLIB: void ImageAlphaClear(Image *image, Color color, float threshold);
        } break;
        case XT_IMAGE_ALPHA_MASK: { /* ( +image +alphaMask -- ) */
            // RAYLIB: void ImageAlphaMask(Image *image, Image alphaMask);
        } break;
        case XT_IMAGE_ALPHA_PREMULTIPLY: { /* ( +image -- ) */
            // RAYLIB: void ImageAlphaPremultiply(Image *image);
        } break;
        case XT_IMAGE_BLUR_GAUSSIAN: { /* ( +image +blurSize -- ) */
            // RAYLIB: void ImageBlurGaussian(Image *image, int blurSize);
        } break;
        case XT_IMAGE_RESIZE: { /* ( +image +newWidth +newHeight -- ) */
            // RAYLIB: void ImageResize(Image *image, int newWidth, int newHeight);
        } break;
        case XT_IMAGE_RESIZE_NN: { /* ( +image +newWidth +newHeight -- ) */
            // RAYLIB: void ImageResizeNN(Image *image, int newWidth, int newHeight);
        } break;
        case XT_IMAGE_RESIZE_CANVAS: { /* ( +image +newWidth +newHeight +offsetX +offsetY +fill -- ) */
            // RAYLIB: void ImageResizeCanvas(Image *image, int newWidth, int newHeight, int offsetX, int offsetY, Color fill);
        } break;
        case XT_IMAGE_MIPMAPS: { /* ( +image -- ) */
            // RAYLIB: void ImageMipmaps(Image *image);
        } break;
        case XT_IMAGE_DITHER: { /* ( +image +rBpp +gBpp +bBpp +aBpp -- ) */
            // RAYLIB: void ImageDither(Image *image, int rBpp, int gBpp, int bBpp, int aBpp);
        } break;
        case XT_IMAGE_FLIP_VERTICAL: { /* ( +image -- ) */
            // RAYLIB: void ImageFlipVertical(Image *image);
        } break;
        case XT_IMAGE_FLIP_HORIZONTAL: { /* ( +image -- ) */
            // RAYLIB: void ImageFlipHorizontal(Image *image);
        } break;
        case XT_IMAGE_ROTATE: { /* ( +image +degrees -- ) */
            // RAYLIB: void ImageRotate(Image *image, int degrees);
        } break;
        case XT_IMAGE_ROTATE_CW: { /* ( +image -- ) */
            // RAYLIB: void ImageRotateCW(Image *image);
        } break;
        case XT_IMAGE_ROTATE_CCW: { /* ( +image -- ) */
            // RAYLIB: void ImageRotateCCW(Image *image);
        } break;
        case XT_IMAGE_COLOR_TINT: { /* ( +image +color -- ) */
            // RAYLIB: void ImageColorTint(Image *image, Color color);
        } break;
        case XT_IMAGE_COLOR_INVERT: { /* ( +image -- ) */
            // RAYLIB: void ImageColorInvert(Image *image);
        } break;
        case XT_IMAGE_COLOR_GRAYSCALE: { /* ( +image -- ) */
            // RAYLIB: void ImageColorGrayscale(Image *image);
        } break;
        case XT_IMAGE_COLOR_CONTRAST: { /* ( +image +contrast -- ) */
            // RAYLIB: void ImageColorContrast(Image *image, float contrast);
        } break;
        case XT_IMAGE_COLOR_BRIGHTNESS: { /* ( +image +brightness -- ) */
            // RAYLIB: void ImageColorBrightness(Image *image, int brightness);
        } break;
        case XT_IMAGE_COLOR_REPLACE: { /* ( +image +color +replace -- ) */
            // RAYLIB: void ImageColorReplace(Image *image, Color color, Color replace);
        } break;
        case XT_LOAD_IMAGE_COLORS: { /* ( +image -- Color* ) */
            // RAYLIB: Color *LoadImageColors(Image image);
        } break;
        case XT_LOAD_IMAGE_PALETTE: { /* ( +image +maxPaletteSize +colorCount -- Color* ) */
            // RAYLIB: Color *LoadImagePalette(Image image, int maxPaletteSize, int *colorCount);
        } break;
        case XT_UNLOAD_IMAGE_COLORS: { /* ( +colors -- ) */
            // RAYLIB: void UnloadImageColors(Color *colors);
        } break;
        case XT_UNLOAD_IMAGE_PALETTE: { /* ( +colors -- ) */
            // RAYLIB: void UnloadImagePalette(Color *colors);
        } break;
        case XT_GET_IMAGE_ALPHA_BORDER: { /* ( +image +threshold -- Rectangle ) */
            // RAYLIB: Rectangle GetImageAlphaBorder(Image image, float threshold);
        } break;
        case XT_GET_IMAGE_COLOR: { /* ( +image +x +y -- Color ) */
            // RAYLIB: Color GetImageColor(Image image, int x, int y);
        } break;
        //
        // rtextures - Image drawing functions
        case XT_IMAGE_CLEAR_BACKGROUND: { /* ( +dst +color -- ) */
            // RAYLIB: void ImageClearBackground(Image *dst, Color color);
        } break;
        case XT_IMAGE_DRAW_PIXEL: { /* ( +dst +posX +posY +color -- ) */
            // RAYLIB: void ImageDrawPixel(Image *dst, int posX, int posY, Color color);
        } break;
        case XT_IMAGE_DRAW_PIXEL_V: { /* ( +dst +position +color -- ) */
            // RAYLIB: void ImageDrawPixelV(Image *dst, Vector2 position, Color color);
        } break;
        case XT_IMAGE_DRAW_LINE: { /* ( +dst +startPosX +startPosY +endPosX +endPosY +color -- ) */
            // RAYLIB: void ImageDrawLine(Image *dst, int startPosX, int startPosY, int endPosX, int endPosY, Color color);
        } break;
        case XT_IMAGE_DRAW_LINE_V: { /* ( +dst +start +end +color -- ) */
            // RAYLIB: void ImageDrawLineV(Image *dst, Vector2 start, Vector2 end, Color color);
        } break;
        case XT_IMAGE_DRAW_CIRCLE: { /* ( +dst +centerX +centerY +radius +color -- ) */
            // RAYLIB: void ImageDrawCircle(Image *dst, int centerX, int centerY, int radius, Color color);
        } break;
        case XT_IMAGE_DRAW_CIRCLE_V: { /* ( +dst +center +radius +color -- ) */
            // RAYLIB: void ImageDrawCircleV(Image *dst, Vector2 center, int radius, Color color);
        } break;
        case XT_IMAGE_DRAW_CIRCLE_LINES: { /* ( +dst +centerX +centerY +radius +color -- ) */
            // RAYLIB: void ImageDrawCircleLines(Image *dst, int centerX, int centerY, int radius, Color color);
        } break;
        case XT_IMAGE_DRAW_CIRCLE_LINES_V: { /* ( +dst +center +radius +color -- ) */
            // RAYLIB: void ImageDrawCircleLinesV(Image *dst, Vector2 center, int radius, Color color);
        } break;
        case XT_IMAGE_DRAW_RECTANGLE: { /* ( +dst +posX +posY +width +height +color -- ) */
            // RAYLIB: void ImageDrawRectangle(Image *dst, int posX, int posY, int width, int height, Color color);
        } break;
        case XT_IMAGE_DRAW_RECTANGLE_V: { /* ( +dst +position +size +color -- ) */
            // RAYLIB: void ImageDrawRectangleV(Image *dst, Vector2 position, Vector2 size, Color color);
        } break;
        case XT_IMAGE_DRAW_RECTANGLE_REC: { /* ( +dst +rec +color -- ) */
            // RAYLIB: void ImageDrawRectangleRec(Image *dst, Rectangle rec, Color color);
        } break;
        case XT_IMAGE_DRAW_RECTANGLE_LINES: { /* ( +dst +rec +thick +color -- ) */
            // RAYLIB: void ImageDrawRectangleLines(Image *dst, Rectangle rec, int thick, Color color);
        } break;
        case XT_IMAGE_DRAW: { /* ( +dst +src +srcRec +dstRec +tint -- ) */
            // RAYLIB: void ImageDraw(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
        } break;
        case XT_IMAGE_DRAW_TEXT: { /* ( +dst +text +posX +posY +fontSize +color -- ) */
            // RAYLIB: void ImageDrawText(Image *dst, const char *text, int posX, int posY, int fontSize, Color color);
        } break;
        case XT_IMAGE_DRAW_TEXT_EX: { /* ( +dst +font +text +position +fontSize +spacing +tint -- ) */
            // RAYLIB: void ImageDrawTextEx(Image *dst, Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);
        } break;
        //
        // rtextures - Texture loading functions
        case XT_LOAD_TEXTURE: { /* ( +fileName -- Texture2D ) */
            // RAYLIB: Texture2D LoadTexture(const char *fileName);
        } break;
        case XT_LOAD_TEXTURE_FROM_IMAGE: { /* ( +image -- Texture2D ) */
            // RAYLIB: Texture2D LoadTextureFromImage(Image image);
        } break;
        case XT_LOAD_TEXTURE_CUBEMAP: { /* ( +image +layout -- TextureCubemap ) */
            // RAYLIB: TextureCubemap LoadTextureCubemap(Image image, int layout);
        } break;
        case XT_LOAD_RENDER_TEXTURE: { /* ( +width +height -- RenderTexture2D ) */
            // RAYLIB: RenderTexture2D LoadRenderTexture(int width, int height);
        } break;
        case XT_IS_TEXTURE_READY: { /* ( +texture -- bool ) */
            // RAYLIB: bool IsTextureReady(Texture2D texture);
        } break;
        case XT_UNLOAD_TEXTURE: { /* ( +texture -- ) */
            // RAYLIB: void UnloadTexture(Texture2D texture);
        } break;
        case XT_IS_RENDER_TEXTURE_READY: { /* ( +target -- bool ) */
            // RAYLIB: bool IsRenderTextureReady(RenderTexture2D target);
        } break;
        case XT_UNLOAD_RENDER_TEXTURE: { /* ( +target -- ) */
            // RAYLIB: void UnloadRenderTexture(RenderTexture2D target);
        } break;
        case XT_UPDATE_TEXTURE: { /* ( +texture +pixels -- ) */
            // RAYLIB: void UpdateTexture(Texture2D texture, const void *pixels);
        } break;
        case XT_UPDATE_TEXTURE_REC: { /* ( +texture +rec +pixels -- ) */
            // RAYLIB: void UpdateTextureRec(Texture2D texture, Rectangle rec, const void *pixels);
        } break;
        //
        // rtextures - Texture configuration functions
        case XT_GEN_TEXTURE_MIPMAPS: { /* ( +texture -- ) */
            // RAYLIB: void GenTextureMipmaps(Texture2D *texture);
        } break;
        case XT_SET_TEXTURE_FILTER: { /* ( +texture +filter -- ) */
            // RAYLIB: void SetTextureFilter(Texture2D texture, int filter);
        } break;
        case XT_SET_TEXTURE_WRAP: { /* ( +texture +wrap -- ) */
            // RAYLIB: void SetTextureWrap(Texture2D texture, int wrap);
        } break;
        //
        // rtextures - Texture drawing functions
        case XT_DRAW_TEXTURE: { /* ( +texture +posX +posY +tint -- ) */
            // RAYLIB: void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
        } break;
        case XT_DRAW_TEXTURE_V: { /* ( +texture +position +tint -- ) */
            // RAYLIB: void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
        } break;
        case XT_DRAW_TEXTURE_EX: { /* ( +texture +position +rotation +scale +tint -- ) */
            // RAYLIB: void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
        } break;
        case XT_DRAW_TEXTURE_REC: { /* ( +texture +source +position +tint -- ) */
            // RAYLIB: void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
        } break;
        case XT_DRAW_TEXTURE_PRO: { /* ( +texture +source +dest +origin +rotation +tint -- ) */
            // RAYLIB: void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
        } break;
        case XT_DRAW_TEXTURE_NPATCH: { /* ( +texture +nPatchInfo +dest +origin +rotation +tint -- ) */
            // RAYLIB: void DrawTextureNPatch(Texture2D texture, NPatchInfo nPatchInfo, Rectangle dest, Vector2 origin, float rotation, Color tint);
        } break;
        //
        // rtextures - Color/pixel related functions
        case XT_FADE: { /* ( +color +alpha -- Color ) */
            // RAYLIB: Color Fade(Color color, float alpha);
        } break;
        case XT_COLOR_TO_INT: { /* ( +color -- int ) */
            // RAYLIB: int ColorToInt(Color color);
        } break;
        case XT_COLOR_NORMALIZE: { /* ( +color -- Vector4 ) */
            // RAYLIB: Vector4 ColorNormalize(Color color);
        } break;
        case XT_COLOR_FROM_NORMALIZED: { /* ( +normalized -- Color ) */
            // RAYLIB: Color ColorFromNormalized(Vector4 normalized);
        } break;
        case XT_COLOR_TO_HSV: { /* ( +color -- Vector3 ) */
            // RAYLIB: Vector3 ColorToHSV(Color color);
        } break;
        case XT_COLOR_FROM_HSV: { /* ( +hue +saturation +value -- Color ) */
            // RAYLIB: Color ColorFromHSV(float hue, float saturation, float value);
        } break;
        case XT_COLOR_TINT: { /* ( +color +tint -- Color ) */
            // RAYLIB: Color ColorTint(Color color, Color tint);
        } break;
        case XT_COLOR_BRIGHTNESS: { /* ( +color +factor -- Color ) */
            // RAYLIB: Color ColorBrightness(Color color, float factor);
        } break;
        case XT_COLOR_CONTRAST: { /* ( +color +contrast -- Color ) */
            // RAYLIB: Color ColorContrast(Color color, float contrast);
        } break;
        case XT_COLOR_ALPHA: { /* ( +color +alpha -- Color ) */
            // RAYLIB: Color ColorAlpha(Color color, float alpha);
        } break;
        case XT_COLOR_ALPHA_BLEND: { /* ( +dst +src +tint -- Color ) */
            // RAYLIB: Color ColorAlphaBlend(Color dst, Color src, Color tint);
        } break;
        case XT_GET_COLOR: { /* ( +hexValue -- Color ) */
            // RAYLIB: Color GetColor(unsigned int hexValue);
        } break;
        case XT_GET_PIXEL_COLOR: { /* ( +srcPtr +format -- Color ) */
            // RAYLIB: Color GetPixelColor(void *srcPtr, int format);
        } break;
        case XT_SET_PIXEL_COLOR: { /* ( +dstPtr +color +format -- ) */
            // RAYLIB: void SetPixelColor(void *dstPtr, Color color, int format);
        } break;
        case XT_GET_PIXEL_DATA_SIZE: { /* ( +width +height +format -- int ) */
            // RAYLIB: int GetPixelDataSize(int width, int height, int format);
        } break;
        //
        // rtext
        // rtext - Font loading/unloading functions
        case XT_GET_FONT_DEFAULT: { /* ( -- Font ) */
            // RAYLIB: Font GetFontDefault(void);
        } break;
        case XT_LOAD_FONT: { /* ( +fileName -- Font ) */
            // RAYLIB: Font LoadFont(const char *fileName);
        } break;
        case XT_LOAD_FONT_EX: { /* ( +fileName +fontSize +codepoints +codepointCount -- Font ) */
            // RAYLIB: Font LoadFontEx(const char *fileName, int fontSize, int *codepoints, int codepointCount);
        } break;
        case XT_LOAD_FONT_FROM_IMAGE: { /* ( +image +key +firstChar -- Font ) */
            // RAYLIB: Font LoadFontFromImage(Image image, Color key, int firstChar);
        } break;
        case XT_LOAD_FONT_FROM_MEMORY: { /* ( +fileType +fileData +dataSize +fontSize +codepoints +codepointCount -- Font ) */
            // RAYLIB: Font LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount);
        } break;
        case XT_IS_FONT_READY: { /* ( +font -- bool ) */
            // RAYLIB: bool IsFontReady(Font font);
        } break;
        case XT_LOAD_FONT_DATA: { /* ( +fileData +dataSize +fontSize +codepoints +codepointCount +type -- GlyphInfo* ) */
            // RAYLIB: GlyphInfo *LoadFontData(const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount, int type);
        } break;
        case XT_GEN_IMAGE_FONT_ATLAS: { /* ( +glyphs +glyphRecs +glyphCount +fontSize +padding +packMethod -- Image ) */
            // RAYLIB: Image GenImageFontAtlas(const GlyphInfo *glyphs, Rectangle **glyphRecs, int glyphCount, int fontSize, int padding, int packMethod);
        } break;
        case XT_UNLOAD_FONT_DATA: { /* ( +glyphs +glyphCount -- ) */
            // RAYLIB: void UnloadFontData(GlyphInfo *glyphs, int glyphCount);
        } break;
        case XT_UNLOAD_FONT: { /* ( +font -- ) */
            // RAYLIB: void UnloadFont(Font font);
        } break;
        case XT_EXPORT_FONT_AS_CODE: { /* ( +font +fileName -- bool ) */
            // RAYLIB: bool ExportFontAsCode(Font font, const char *fileName);
        } break;
        // rtext - Text drawing functions
        case XT_DRAW_FPS: { /* ( +posX +posY -- ) */
            // RAYLIB: void DrawFPS(int posX, int posY);
        } break;
        case XT_DRAW_TEXT: { /* ( +text +posX +posY +fontSize +color -- ) */
            // RAYLIB: void DrawText(const char *text, int posX, int posY, int fontSize, Color color);
        } break;
        case XT_DRAW_TEXT_EX: { /* ( +font +text +position +fontSize +spacing +tint -- ) */
            // RAYLIB: void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);
        } break;
        case XT_DRAW_TEXT_PRO: { /* ( +font +text +position +origin +rotation +fontSize +spacing +tint -- ) */
            // RAYLIB: void DrawTextPro(Font font, const char *text, Vector2 position, Vector2 origin, float rotation, float fontSize, float spacing, Color tint);
        } break;
        case XT_DRAW_TEXT_CODEPOINT: { /* ( +font +codepoint +position +fontSize +tint -- ) */
            // RAYLIB: void DrawTextCodepoint(Font font, int codepoint, Vector2 position, float fontSize, Color tint);
        } break;
        case XT_DRAW_TEXT_CODEPOINTS: { /* ( +font +codepoints +codepointCount +position +fontSize +spacing +tint -- ) */
            // RAYLIB: void DrawTextCodepoints(Font font, const int *codepoints, int codepointCount, Vector2 position, float fontSize, float spacing, Color tint);
        } break;
        // rtext - Text font info functions
        case XT_SET_TEXT_LINE_SPACING: { /* ( +spacing -- ) */
            // RAYLIB: void SetTextLineSpacing(int spacing);
        } break;
        case XT_MEASURE_TEXT: { /* ( +text +fontSize -- int ) */
            // RAYLIB: int MeasureText(const char *text, int fontSize);
        } break;
        case XT_MEASURE_TEXT_EX: { /* ( +font +text +fontSize +spacing -- Vector2 ) */
            // RAYLIB: Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing);
        } break;
        case XT_GET_GLYPH_INDEX: { /* ( +font +codepoint -- int ) */
            // RAYLIB: int GetGlyphIndex(Font font, int codepoint);
        } break;
        case XT_GET_GLYPH_INFO: { /* ( +font +codepoint -- GlyphInfo ) */
            // RAYLIB: GlyphInfo GetGlyphInfo(Font font, int codepoint);
        } break;
        case XT_GET_GLYPH_ATLAS_REC: { /* ( +font +codepoint -- Rectangle ) */
            // RAYLIB: Rectangle GetGlyphAtlasRec(Font font, int codepoint);
        } break;
        // rtext - Text codepoints management functions (unicode characters)
        case XT_LOAD_UTF8: { /* ( +codepoints +length -- char* ) */
            // RAYLIB: char *LoadUTF8(const int *codepoints, int length);
        } break;
        case XT_UNLOAD_UTF8: { /* ( +text -- ) */
            // RAYLIB: void UnloadUTF8(char *text);
        } break;
        case XT_LOAD_CODEPOINTS: { /* ( +text +count -- int* ) */
            // RAYLIB: int *LoadCodepoints(const char *text, int *count);
        } break;
        case XT_UNLOAD_CODEPOINTS: { /* ( +codepoints -- ) */
            // RAYLIB: void UnloadCodepoints(int *codepoints);
        } break;
        case XT_GET_CODEPOINT_COUNT: { /* ( +text -- int ) */
            // RAYLIB: int GetCodepointCount(const char *text);
        } break;
        case XT_GET_CODEPOINT: { /* ( +text +codepointSize -- int ) */
            // RAYLIB: int GetCodepoint(const char *text, int *codepointSize);
        } break;
        case XT_GET_CODEPOINT_NEXT: { /* ( +text +codepointSize -- int ) */
            // RAYLIB: int GetCodepointNext(const char *text, int *codepointSize);
        } break;
        case XT_GET_CODEPOINT_PREVIOUS: { /* ( +text +codepointSize -- int ) */
            // RAYLIB: int GetCodepointPrevious(const char *text, int *codepointSize);
        } break;
        case XT_CODEPOINT_TO_UTF8: { /* ( +codepoint +utf8Size -- char* ) */
            // RAYLIB: const char *CodepointToUTF8(int codepoint, int *utf8Size);
        } break;
        //
        // rtext - Text strings management functions (no UTF-8 strings, only byte chars)
        case XT_TEXT_COPY: { /* ( +dst +src -- int ) */
            // RAYLIB: int TextCopy(char *dst, const char *src);
        } break;
        case XT_TEXT_IS_EQUAL: { /* ( +text1 +text2 -- bool ) */
            // RAYLIB: bool TextIsEqual(const char *text1, const char *text2);
        } break;
        case XT_TEXT_LENGTH: { /* ( +text -- unsigned int ) */
            // RAYLIB: unsigned int TextLength(const char *text);
        } break;
        case XT_TEXT_FORMAT: { /* ( +text ... -- const char* ) */
            // RAYLIB: const char *TextFormat(const char *text, ...);
        } break;
        case XT_TEXT_SUBTEXT: { /* ( +text +position +length -- const char* ) */
            // RAYLIB: const char *TextSubtext(const char *text, int position, int length);
        } break;
        case XT_TEXT_REPLACE: { /* ( +text +replace +by -- char* ) */
            // RAYLIB: char *TextReplace(char *text, const char *replace, const char *by);
        } break;
        case XT_TEXT_INSERT: { /* ( +text +insert +position -- char* ) */
            // RAYLIB: char *TextInsert(const char *text, const char *insert, int position);
        } break;
        case XT_TEXT_JOIN: { /* ( +textList +count +delimiter -- const char* ) */
            // RAYLIB: const char *TextJoin(const char **textList, int count, const char *delimiter);
        } break;
        case XT_TEXT_SPLIT: { /* ( +text +delimiter +count -- const char** ) */
            // RAYLIB: const char **TextSplit(const char *text, char delimiter, int *count);
        } break;
        case XT_TEXT_APPEND: { /* ( +text +append +position -- ) */
            // RAYLIB: void TextAppend(char *text, const char *append, int *position);
        } break;
        case XT_TEXT_FIND_INDEX: { /* ( +text +find -- int ) */
            // RAYLIB: int TextFindIndex(const char *text, const char *find);
        } break;
        case XT_TEXT_TO_UPPER: { /* ( +text -- const char* ) */
            // RAYLIB: const char *TextToUpper(const char *text);
        } break;
        case XT_TEXT_TO_LOWER: { /* ( +text -- const char* ) */
            // RAYLIB: const char *TextToLower(const char *text);
        } break;
        case XT_TEXT_TO_PASCAL: { /* ( +text -- const char* ) */
            // RAYLIB: const char *TextToPascal(const char *text);
        } break;
        case XT_TEXT_TO_INTEGER: { /* ( +text -- int ) */
            // RAYLIB: int TextToInteger(const char *text);
        } break;
        //
        // rmodels
        // rmodels - Basic geometric 3D shapes drawing functions
        case XT_DRAW_LINE_3D: { /* ( +startPos +endPos +color -- ) */
            // RAYLIB: void DrawLine3D(Vector3 startPos, Vector3 endPos, Color color);
        } break;
        case XT_DRAW_POINT_3D: { /* ( +position +color -- ) */
            // RAYLIB: void DrawPoint3D(Vector3 position, Color color);
        } break;
        case XT_DRAW_CIRCLE_3D: { /* ( +center +radius +rotationAxis +rotationAngle +color -- ) */
            // RAYLIB: void DrawCircle3D(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, Color color);
        } break;
        case XT_DRAW_TRIANGLE_3D: { /* ( +v1 +v2 +v3 +color -- ) */
            // RAYLIB: void DrawTriangle3D(Vector3 v1, Vector3 v2, Vector3 v3, Color color);
        } break;
        case XT_DRAW_TRIANGLE_STRIP_3D: { /* ( +points +pointCount +color -- ) */
            // RAYLIB: void DrawTriangleStrip3D(Vector3 *points, int pointCount, Color color);
        } break;
        case XT_DRAW_CUBE: { /* ( +position +width +height +length +color -- ) */
            // RAYLIB: void DrawCube(Vector3 position, float width, float height, float length, Color color);
        } break;
        case XT_DRAW_CUBE_V: { /* ( +position +size +color -- ) */
            // RAYLIB: void DrawCubeV(Vector3 position, Vector3 size, Color color);
        } break;
        case XT_DRAW_CUBE_WIRES: { /* ( +position +width +height +length +color -- ) */
            // RAYLIB: void DrawCubeWires(Vector3 position, float width, float height, float length, Color color);
        } break;
        case XT_DRAW_CUBE_WIRES_V: { /* ( +position +size +color -- ) */
            // RAYLIB: void DrawCubeWiresV(Vector3 position, Vector3 size, Color color);
        } break;
        case XT_DRAW_SPHERE: { /* ( +centerPos +radius +color -- ) */
            // RAYLIB: void DrawSphere(Vector3 centerPos, float radius, Color color);
        } break;
        case XT_DRAW_SPHERE_EX: { /* ( +centerPos +radius +rings +slices +color -- ) */
            // RAYLIB: void DrawSphereEx(Vector3 centerPos, float radius, int rings, int slices, Color color);
        } break;
        case XT_DRAW_SPHERE_WIRES: { /* ( +centerPos +radius +rings +slices +color -- ) */
            // RAYLIB: void DrawSphereWires(Vector3 centerPos, float radius, int rings, int slices, Color color);
        } break;
        case XT_DRAW_CYLINDER: { /* ( +position +radiusTop +radiusBottom +height +slices +color -- ) */
            // RAYLIB: void DrawCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color);
        } break;
        case XT_DRAW_CYLINDER_EX: { /* ( +startPos +endPos +startRadius +endRadius +sides +color -- ) */
            // RAYLIB: void DrawCylinderEx(Vector3 startPos, Vector3 endPos, float startRadius, float endRadius, int sides, Color color);
        } break;
        case XT_DRAW_CYLINDER_WIRES: { /* ( +position +radiusTop +radiusBottom +height +slices +color -- ) */
            // RAYLIB: void DrawCylinderWires(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color);
        } break;
        case XT_DRAW_CYLINDER_WIRES_EX: { /* ( +startPos +endPos +startRadius +endRadius +sides +color -- ) */
            // RAYLIB: void DrawCylinderWiresEx(Vector3 startPos, Vector3 endPos, float startRadius, float endRadius, int sides, Color color);
        } break;
        case XT_DRAW_CAPSULE: { /* ( +startPos +endPos +radius +slices +rings +color -- ) */
            // RAYLIB: void DrawCapsule(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color);
        } break;
        case XT_DRAW_CAPSULE_WIRES: { /* ( +startPos +endPos +radius +slices +rings +color -- ) */
            // RAYLIB: void DrawCapsuleWires(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color);
        } break;
        case XT_DRAW_PLANE: { /* ( +centerPos +size +color -- ) */
            // RAYLIB: void DrawPlane(Vector3 centerPos, Vector2 size, Color color);
        } break;
        case XT_DRAW_RAY: { /* ( +ray +color -- ) */
            // RAYLIB: void DrawRay(Ray ray, Color color);
        } break;
        case XT_DRAW_GRID: { /* ( +slices +spacing -- ) */
            // RAYLIB: void DrawGrid(int slices, float spacing);
        } break;
        // rmodels - Model management functions
        case XT_LOAD_MODEL: { /* ( +fileName -- Model ) */
            // RAYLIB: Model LoadModel(const char *fileName);
        } break;
        case XT_LOAD_MODEL_FROM_MESH: { /* ( +mesh -- Model ) */
            // RAYLIB: Model LoadModelFromMesh(Mesh mesh);
        } break;
        case XT_IS_MODEL_READY: { /* ( +model -- bool ) */
            // RAYLIB: bool IsModelReady(Model model);
        } break;
        case XT_UNLOAD_MODEL: { /* ( +model -- ) */
            // RAYLIB: void UnloadModel(Model model);
        } break;
        case XT_GET_MODEL_BOUNDING_BOX: { /* ( +model -- BoundingBox ) */
            // RAYLIB: BoundingBox GetModelBoundingBox(Model model);
        } break;
        // rmodels - Model drawing functions
        case XT_DRAW_MODEL: { /* ( +model +position +scale +tint -- ) */
            // RAYLIB: void DrawModel(Model model, Vector3 position, float scale, Color tint);
        } break;
        case XT_DRAW_MODEL_EX: { /* ( +model +position +rotationAxis +rotationAngle +scale +tint -- ) */
            // RAYLIB: void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint);
        } break;
        case XT_DRAW_MODEL_WIRES: { /* ( +model +position +scale +tint -- ) */
            // RAYLIB: void DrawModelWires(Model model, Vector3 position, float scale, Color tint);
        } break;
        case XT_DRAW_MODEL_WIRES_EX: { /* ( +model +position +rotationAxis +rotationAngle +scale +tint -- ) */
            // RAYLIB: void DrawModelWiresEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint);
        } break;
        case XT_DRAW_BOUNDING_BOX: { /* ( +box +color -- ) */
            // RAYLIB: void DrawBoundingBox(BoundingBox box, Color color);
        } break;
        case XT_DRAW_BILLBOARD: { /* ( +camera +texture +position +size +tint -- ) */
            // RAYLIB: void DrawBillboard(Camera camera, Texture2D texture, Vector3 position, float size, Color tint);
        } break;
        case XT_DRAW_BILLBOARD_REC: { /* ( +camera +texture +source +position +size +tint -- ) */
            // RAYLIB: void DrawBillboardRec(Camera camera, Texture2D texture, Rectangle source, Vector3 position, Vector2 size, Color tint);
        } break;
        case XT_DRAW_BILLBOARD_PRO: { /* ( +camera +texture +source +position +up +size +origin +rotation +tint -- ) */
            // RAYLIB: void DrawBillboardPro(Camera camera, Texture2D texture, Rectangle source, Vector3 position, Vector3 up, Vector2 size, Vector2 origin, float rotation, Color tint);
        } break;
        // rmodels - Mesh management functions
        case XT_UPLOAD_MESH: { /* ( +mesh +dynamic -- ) */
            // RAYLIB: void UploadMesh(Mesh *mesh, bool dynamic);
        } break;
        case XT_UPDATE_MESH_BUFFER: { /* ( +mesh +index +data +dataSize +offset -- ) */
            // RAYLIB: void UpdateMeshBuffer(Mesh mesh, int index, const void *data, int dataSize, int offset);
        } break;
        case XT_UNLOAD_MESH: { /* ( +mesh -- ) */
            // RAYLIB: void UnloadMesh(Mesh mesh);
        } break;
        case XT_DRAW_MESH: { /* ( +mesh +material +transform -- ) */
            // RAYLIB: void DrawMesh(Mesh mesh, Material material, Matrix transform);
        } break;
        case XT_DRAW_MESH_INSTANCED: { /* ( +mesh +material +transforms +instances -- ) */
            // RAYLIB: void DrawMeshInstanced(Mesh mesh, Material material, const Matrix *transforms, int instances);
        } break;
        case XT_EXPORT_MESH: { /* ( +mesh +fileName -- bool ) */
            // RAYLIB: bool ExportMesh(Mesh mesh, const char *fileName);
        } break;
        case XT_GET_MESH_BOUNDING_BOX: { /* ( +mesh -- BoundingBox ) */
            // RAYLIB: BoundingBox GetMeshBoundingBox(Mesh mesh);
        } break;
        case XT_GEN_MESH_TANGENTS: { /* ( +mesh -- ) */
            // RAYLIB: void GenMeshTangents(Mesh *mesh);
        } break;
        // rmodels - Mesh generation functions
        case XT_GEN_MESH_POLY: { /* ( +sides +radius -- Mesh ) */
            // RAYLIB: Mesh GenMeshPoly(int sides, float radius);
        } break;
        case XT_GEN_MESH_PLANE: { /* ( +width +length +resX +resZ -- Mesh ) */
            // RAYLIB: Mesh GenMeshPlane(float width, float length, int resX, int resZ);
        } break;
        case XT_GEN_MESH_CUBE: { /* ( +width +height +length -- Mesh ) */
            // RAYLIB: Mesh GenMeshCube(float width, float height, float length);
        } break;
        case XT_GEN_MESH_SPHERE: { /* ( +radius +rings +slices -- Mesh ) */
            // RAYLIB: Mesh GenMeshSphere(float radius, int rings, int slices);
        } break;
        case XT_GEN_MESH_HEMI_SPHERE: { /* ( +radius +rings +slices -- Mesh ) */
            // RAYLIB: Mesh GenMeshHemiSphere(float radius, int rings, int slices);
        } break;
        case XT_GEN_MESH_CYLINDER: { /* ( +radius +height +slices -- Mesh ) */
            // RAYLIB: Mesh GenMeshCylinder(float radius, float height, int slices);
        } break;
        case XT_GEN_MESH_CONE: { /* ( +radius +height +slices -- Mesh ) */
            // RAYLIB: Mesh GenMeshCone(float radius, float height, int slices);
        } break;
        case XT_GEN_MESH_TORUS: { /* ( +radius +size +radSeg +sides -- Mesh ) */
            // RAYLIB: Mesh GenMeshTorus(float radius, float size, int radSeg, int sides);
        } break;
        case XT_GEN_MESH_KNOT: { /* ( +radius +size +radSeg +sides -- Mesh ) */
            // RAYLIB: Mesh GenMeshKnot(float radius, float size, int radSeg, int sides);
        } break;
        case XT_GEN_MESH_HEIGHTMAP: { /* ( +heightmap +size -- Mesh ) */
            // RAYLIB: Mesh GenMeshHeightmap(Image heightmap, Vector3 size);
        } break;
        case XT_GEN_MESH_CUBICMAP: { /* ( +cubicmap +cubeSize -- Mesh ) */
            // RAYLIB: Mesh GenMeshCubicmap(Image cubicmap, Vector3 cubeSize);
        } break;
        // rmodels - Material loading/unloading functions
        case XT_LOAD_MATERIALS: { /* ( +fileName +materialCount -- Material* ) */
            // RAYLIB: Material *LoadMaterials(const char *fileName, int *materialCount);
        } break;
        case XT_LOAD_MATERIAL_DEFAULT: { /* ( -- Material ) */
            // RAYLIB: Material LoadMaterialDefault(void);
        } break;
        case XT_IS_MATERIAL_READY: { /* ( +material -- bool ) */
            // RAYLIB: bool IsMaterialReady(Material material);
        } break;
        case XT_UNLOAD_MATERIAL: { /* ( +material -- ) */
            // RAYLIB: void UnloadMaterial(Material material);
        } break;
        case XT_SET_MATERIAL_TEXTURE: { /* ( +material +mapType +texture -- ) */
            // RAYLIB: void SetMaterialTexture(Material *material, int mapType, Texture2D texture);
        } break;
        case XT_SET_MODEL_MESH_MATERIAL: { /* ( +model +meshId +materialId -- ) */
            // RAYLIB: void SetModelMeshMaterial(Model *model, int meshId, int materialId);
        } break;
        // rmodles - Model animations loading/unloading functions
        case XT_LOAD_MODEL_ANIMATIONS: { /* ( +fileName +animCount -- ModelAnimation* ) */
            // RAYLIB: ModelAnimation *LoadModelAnimations(const char *fileName, int *animCount);
        } break;
        case XT_UPDATE_MODEL_ANIMATION: { /* ( +model +anim +frame -- ) */
            // RAYLIB: void UpdateModelAnimation(Model model, ModelAnimation anim, int frame);
        } break;
        case XT_UNLOAD_MODEL_ANIMATION: { /* ( +anim -- ) */
            // RAYLIB: void UnloadModelAnimation(ModelAnimation anim);
        } break;
        case XT_UNLOAD_MODEL_ANIMATIONS: { /* ( +animations +animCount -- ) */
            // RAYLIB: void UnloadModelAnimations(ModelAnimation *animations, int animCount);
        } break;
        case XT_IS_MODEL_ANIMATION_VALID: { /* ( +model +anim -- bool ) */
            // RAYLIB: bool IsModelAnimationValid(Model model, ModelAnimation anim);
        } break;
        // rmodels - Collision detection functions
        case XT_CHECK_COLLISION_SPHERES: { /* ( +center1 +radius1 +center2 +radius2 -- bool ) */
            // RAYLIB: bool CheckCollisionSpheres(Vector3 center1, float radius1, Vector3 center2, float radius2);
        } break;
        case XT_CHECK_COLLISION_BOXES: { /* ( +box1 +box2 -- bool ) */
            // RAYLIB: bool CheckCollisionBoxes(BoundingBox box1, BoundingBox box2);
        } break;
        case XT_CHECK_COLLISION_BOX_SPHERE: { /* ( +box +center +radius -- bool ) */
            // RAYLIB: bool CheckCollisionBoxSphere(BoundingBox box, Vector3 center, float radius);
        } break;
        case XT_GET_RAY_COLLISION_SPHERE: { /* ( +ray +center +radius -- RayCollision ) */
            // RAYLIB: RayCollision GetRayCollisionSphere(Ray ray, Vector3 center, float radius);
        } break;
        case XT_GET_RAY_COLLISION_BOX: { /* ( +ray +box -- RayCollision ) */
            // RAYLIB: RayCollision GetRayCollisionBox(Ray ray, BoundingBox box);
        } break;
        case XT_GET_RAY_COLLISION_MESH: { /* ( +ray +mesh +transform -- RayCollision ) */
            // RAYLIB: RayCollision GetRayCollisionMesh(Ray ray, Mesh mesh, Matrix transform);
        } break;
        case XT_GET_RAY_COLLISION_TRIANGLE: { /* ( +ray +p1 +p2 +p3 -- RayCollision ) */
            // RAYLIB: RayCollision GetRayCollisionTriangle(Ray ray, Vector3 p1, Vector3 p2, Vector3 p3);
        } break;
        case XT_GET_RAY_COLLISION_QUAD: { /* ( +ray +p1 +p2 +p3 +p4 -- RayCollision ) */
            // RAYLIB: RayCollision GetRayCollisionQuad(Ray ray, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4);
        } break;
        // 
        // raudio
        // raudio - Audio device management functions
        case XT_INIT_AUDIO_DEVICE: { /* ( -- void ) */
            // RAYLIB: void InitAudioDevice(void);
        } break;
        case XT_CLOSE_AUDIO_DEVICE: { /* ( -- void ) */
            // RAYLIB: void CloseAudioDevice(void);
        } break;
        case XT_IS_AUDIO_DEVICE_READY: { /* ( -- bool ) */
            // RAYLIB: bool IsAudioDeviceReady(void);
        } break;
        case XT_SET_MASTER_VOLUME: { /* ( +volume -- void ) */
            // RAYLIB: void SetMasterVolume(float volume);
        } break;
        case XT_GET_MASTER_VOLUME: { /* ( -- float ) */
            // RAYLIB: float GetMasterVolume(void);
        } break;
        // raudio - Wave/Sound loading/unloading functions
        case XT_LOAD_WAVE: { /* ( +fileName -- Wave ) */
            // RAYLIB: Wave LoadWave(const char *fileName);
        } break;
        case XT_LOAD_WAVE_FROM_MEMORY: { /* ( +fileType +fileData +dataSize -- Wave ) */
            // RAYLIB: Wave LoadWaveFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);
        } break;
        case XT_IS_WAVE_READY: { /* ( +wave -- bool ) */
            // RAYLIB: bool IsWaveReady(Wave wave);
        } break;
        case XT_LOAD_SOUND: { /* ( +fileName -- Sound ) */
            // RAYLIB: Sound LoadSound(const char *fileName);
        } break;
        case XT_LOAD_SOUND_FROM_WAVE: { /* ( +wave -- Sound ) */
            // RAYLIB: Sound LoadSoundFromWave(Wave wave);
        } break;
        case XT_LOAD_SOUND_ALIAS: { /* ( +source -- Sound ) */
            // RAYLIB: Sound LoadSoundAlias(Sound source);
        } break;
        case XT_IS_SOUND_READY: { /* ( +sound -- bool ) */
            // RAYLIB: bool IsSoundReady(Sound sound);
        } break;
        case XT_UPDATE_SOUND: { /* ( +sound +data +sampleCount -- void ) */
            // RAYLIB: void UpdateSound(Sound sound, const void *data, int sampleCount);
        } break;
        case XT_UNLOAD_WAVE: { /* ( +wave -- void ) */
            // RAYLIB: void UnloadWave(Wave wave);
        } break;
        case XT_UNLOAD_SOUND: { /* ( +sound -- void ) */
            // RAYLIB: void UnloadSound(Sound sound);
        } break;
        case XT_UNLOAD_SOUND_ALIAS: { /* ( +alias -- void ) */
            // RAYLIB: void UnloadSoundAlias(Sound alias);
        } break;
        case XT_EXPORT_WAVE: { /* ( +wave +fileName -- bool ) */
            // RAYLIB: bool ExportWave(Wave wave, const char *fileName);
        } break;
        case XT_EXPORT_WAVE_AS_CODE: { /* ( +wave +fileName -- bool ) */
            // RAYLIB: bool ExportWaveAsCode(Wave wave, const char *fileName);
        } break;
        // raudio - Wave/Sound management functions
        case XT_PLAY_SOUND: { /* ( +sound -- void ) */
            // RAYLIB: void PlaySound(Sound sound);
        } break;
        case XT_STOP_SOUND: { /* ( +sound -- void ) */
            // RAYLIB: void StopSound(Sound sound);
        } break;
        case XT_PAUSE_SOUND: { /* ( +sound -- void ) */
            // RAYLIB: void PauseSound(Sound sound);
        } break;
        case XT_RESUME_SOUND: { /* ( +sound -- void ) */
            // RAYLIB: void ResumeSound(Sound sound);
        } break;
        case XT_IS_SOUND_PLAYING: { /* ( +sound -- bool ) */
            // RAYLIB: bool IsSoundPlaying(Sound sound);
        } break;
        case XT_SET_SOUND_VOLUME: { /* ( +sound +volume -- void ) */
            // RAYLIB: void SetSoundVolume(Sound sound, float volume);
        } break;
        case XT_SET_SOUND_PITCH: { /* ( +sound +pitch -- void ) */
            // RAYLIB: void SetSoundPitch(Sound sound, float pitch);
        } break;
        case XT_SET_SOUND_PAN: { /* ( +sound +pan -- void ) */
            // RAYLIB: void SetSoundPan(Sound sound, float pan);
        } break;
        case XT_WAVE_COPY: { /* ( +wave -- Wave ) */
            // RAYLIB: Wave WaveCopy(Wave wave);
        } break;
        case XT_WAVE_CROP: { /* ( +wave +initSample +finalSample -- void ) */
            // RAYLIB: void WaveCrop(Wave *wave, int initSample, int finalSample);
        } break;
        case XT_WAVE_FORMAT: { /* ( +wave +sampleRate +sampleSize +channels -- void ) */
            // RAYLIB: void WaveFormat(Wave *wave, int sampleRate, int sampleSize, int channels);
        } break;
        case XT_LOAD_WAVE_SAMPLES: { /* ( +wave -- float* ) */
            // RAYLIB: float *LoadWaveSamples(Wave wave);
        } break;
        case XT_UNLOAD_WAVE_SAMPLES: { /* ( +samples -- void ) */
            // RAYLIB: void UnloadWaveSamples(float *samples);
        } break;
        // raudio - Music management functions
        case XT_LOAD_MUSIC_STREAM: { /* ( +fileName -- Music ) */
            // RAYLIB: Music LoadMusicStream(const char *fileName);
        } break;
        case XT_LOAD_MUSIC_STREAM_FROM_MEMORY: { /* ( +fileType +data +dataSize -- Music ) */
            // RAYLIB: Music LoadMusicStreamFromMemory(const char *fileType, const unsigned char *data, int dataSize);
        } break;
        case XT_IS_MUSIC_READY: { /* ( +music -- bool ) */
            // RAYLIB: bool IsMusicReady(Music music);
        } break;
        case XT_UNLOAD_MUSIC_STREAM: { /* ( +music -- void ) */
            // RAYLIB: void UnloadMusicStream(Music music);
        } break;
        case XT_PLAY_MUSIC_STREAM: { /* ( +music -- void ) */
            // RAYLIB: void PlayMusicStream(Music music);
        } break;
        case XT_IS_MUSIC_STREAM_PLAYING: { /* ( +music -- bool ) */
            // RAYLIB: bool IsMusicStreamPlaying(Music music);
        } break;
        case XT_UPDATE_MUSIC_STREAM: { /* ( +music -- void ) */
            // RAYLIB: void UpdateMusicStream(Music music);
        } break;
        case XT_STOP_MUSIC_STREAM: { /* ( +music -- void ) */
            // RAYLIB: void StopMusicStream(Music music);
        } break;
        case XT_PAUSE_MUSIC_STREAM: { /* ( +music -- void ) */
            // RAYLIB: void PauseMusicStream(Music music);
        } break;
        case XT_RESUME_MUSIC_STREAM: { /* ( +music -- void ) */
            // RAYLIB: void ResumeMusicStream(Music music);
        } break;
        case XT_SEEK_MUSIC_STREAM: { /* ( +music +position -- void ) */
            // RAYLIB: void SeekMusicStream(Music music, float position);
        } break;
        case XT_SET_MUSIC_VOLUME: { /* ( +music +volume -- void ) */
            // RAYLIB: void SetMusicVolume(Music music, float volume);
        } break;
        case XT_SET_MUSIC_PITCH: { /* ( +music +pitch -- void ) */
            // RAYLIB: void SetMusicPitch(Music music, float pitch);
        } break;
        case XT_SET_MUSIC_PAN: { /* ( +music +pan -- void ) */
            // RAYLIB: void SetMusicPan(Music music, float pan);
        } break;
        case XT_GET_MUSIC_TIME_LENGTH: { /* ( +music -- float ) */
            // RAYLIB: float GetMusicTimeLength(Music music);
        } break;
        case XT_GET_MUSIC_TIME_PLAYED: { /* ( +music -- float ) */
            // RAYLIB: float GetMusicTimePlayed(Music music);
        } break;
        // raudio - AudioStream management functions
        case XT_LOAD_AUDIO_STREAM: { /* ( +sampleRate +sampleSize +channels -- AudioStream ) */
            // RAYLIB: AudioStream LoadAudioStream(unsigned int sampleRate, unsigned int sampleSize, unsigned int channels);
        } break;
        case XT_IS_AUDIO_STREAM_READY: { /* ( +stream -- bool ) */
            // RAYLIB: bool IsAudioStreamReady(AudioStream stream);
        } break;
        case XT_UNLOAD_AUDIO_STREAM: { /* ( +stream -- void ) */
            // RAYLIB: void UnloadAudioStream(AudioStream stream);
        } break;
        case XT_UPDATE_AUDIO_STREAM: { /* ( +stream +data +frameCount -- void ) */
            // RAYLIB: void UpdateAudioStream(AudioStream stream, const void *data, int frameCount);
        } break;
        case XT_IS_AUDIO_STREAM_PROCESSED: { /* ( +stream -- bool ) */
            // RAYLIB: bool IsAudioStreamProcessed(AudioStream stream);
        } break;
        case XT_PLAY_AUDIO_STREAM: { /* ( +stream -- void ) */
            // RAYLIB: void PlayAudioStream(AudioStream stream);
        } break;
        case XT_PAUSE_AUDIO_STREAM: { /* ( +stream -- void ) */
            // RAYLIB: void PauseAudioStream(AudioStream stream);
        } break;
        case XT_RESUME_AUDIO_STREAM: { /* ( +stream -- void ) */
            // RAYLIB: void ResumeAudioStream(AudioStream stream);
        } break;
        case XT_IS_AUDIO_STREAM_PLAYING: { /* ( +stream -- bool ) */
            // RAYLIB: bool IsAudioStreamPlaying(AudioStream stream);
        } break;
        case XT_STOP_AUDIO_STREAM: { /* ( +stream -- void ) */
            // RAYLIB: void StopAudioStream(AudioStream stream);
        } break;
        case XT_SET_AUDIO_STREAM_VOLUME: { /* ( +stream +volume -- void ) */
            // RAYLIB: void SetAudioStreamVolume(AudioStream stream, float volume);
        } break;
        case XT_SET_AUDIO_STREAM_PITCH: { /* ( +stream +pitch -- void ) */
            // RAYLIB: void SetAudioStreamPitch(AudioStream stream, float pitch);
        } break;
        case XT_SET_AUDIO_STREAM_PAN: { /* ( +stream +pan -- void ) */
            // RAYLIB: void SetAudioStreamPan(AudioStream stream, float pan);
        } break;
        case XT_SET_AUDIO_STREAM_BUFFER_SIZE_DEFAULT: { /* ( +size -- void ) */
            // RAYLIB: void SetAudioStreamBufferSizeDefault(int size);
        } break;
        case XT_SET_AUDIO_STREAM_CALLBACK: { /* ( +stream +callback -- void ) */
            // RAYLIB: void SetAudioStreamCallback(AudioStream stream, AudioCallback callback);
        } break;
        case XT_ATTACH_AUDIO_STREAM_PROCESSOR: { /* ( +stream +processor -- void ) */
            // RAYLIB: void AttachAudioStreamProcessor(AudioStream stream, AudioCallback processor);
        } break;
        case XT_DETACH_AUDIO_STREAM_PROCESSOR: { /* ( +stream +processor -- void ) */
            // RAYLIB: void DetachAudioStreamProcessor(AudioStream stream, AudioCallback processor);
        } break;
        case XT_ATTACH_AUDIO_MIXED_PROCESSOR: { /* ( +processor -- void ) */
            // RAYLIB: void AttachAudioMixedProcessor(AudioCallback processor);
        } break;
        case XT_DETACH_AUDIO_MIXED_PROCESSOR: { /* ( +processor -- void ) */
            // RAYLIB: void DetachAudioMixedProcessor(AudioCallback processor);
        } break;




          //************************************************************
          // End of Raylib words
          //************************************************************

        default:
          ERR("pfCatch: Unrecognised token = 0x");
          ffDotHex(Token);
          ERR(" at 0x");
          ffDotHex((cell_t)InsPtr);
          EMIT_CR;
          InsPtr = 0;
        } // switch(Token)

        if (InsPtr) {
          Token = READ_CELL_DIC(
              InsPtr++); /* Traverse to next token in secondary. */
        }

    } while( (InitialReturnStack - TORPTR) > 0 );

    SAVE_REGISTERS;

    return ExceptionReturnCode;
}
