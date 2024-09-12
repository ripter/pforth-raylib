/* @(#) pf_clib.c 2024-09-12 Updated to C17 */
/***************************************************************
** Duplicate functions from stdlib for PForth based on 'C'
**
** This code duplicates some of the code in the 'C' lib
** because it reduces the dependency on foreign libraries
** for monitor mode where no OS is available.
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
****************************************************************
** 961124 PLB Advance pointers in pfCopyMemory() and pfSetMemory()
***************************************************************/

#include "pf_all.h"

/* Convert a character to uppercase */
char pfCharToUpper(char c) {
    return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

/* Convert a character to lowercase */
char pfCharToLower(char c) {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}
