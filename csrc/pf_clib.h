/* @(#) pf_clib.h 2024-09-12 Updated to C17 (C only) */
#pragma once
#ifndef _PF_CLIB_H
#define _PF_CLIB_H

/***************************************************************
** Include file for PForth tools
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, David Rosenboom
** Updated by Chris Richards 2024 to C17 (C only)
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
***************************************************************/

#include <string.h>  // Include string.h for standard library memory functions
#include <stdlib.h>  // Include for exit function

/* Use standard library functions */
#define pfCStringLength strlen
#define pfSetMemory     memset
#define pfCopyMemory    memcpy
#define EXIT(n)         exit(n)

/* Functions that perform character case conversion without triggering macro issues */
char pfCharToUpper(char c);
char pfCharToLower(char c);

#endif /* _PF_CLIB_H */
