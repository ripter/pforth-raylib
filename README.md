# PForth Raylib Edition
## A PForth Fork for desktop application development written in C

A Forth implementation that includes Raylib and other C libraries used for desktop and game development. Forked from the amazing [pForth](https://github.com/philburk/pforth).

This is a C99 project that tries to match the coding guidelines set by Raylib.

**This is a Work In Progress.**


### Future Plans

I intend to remove CMake and platforms that are not supported by Raylib in the future. The project will use Make as the build tool. For now, the only platform being tested is platforms/unix/Makefile.

For now, the main Makefile can be run like this:

```sh
cd platforms/unix/
make
```


### Linking Raylib

This project uses `pkg-config` to link to Raylib. If you encounter issues where Raylib functions (e.g., `EndDrawing`, `DrawText`) aren't found during linking, it may be due to `pkg-config` not correctly detecting your Raylib installation—especially if you built Raylib from source.

You can check if `pkg-config` knows about Raylib by running:

```
pkg-config --cflags raylib
```


### Bugs

* Lots of missing words.
* file paths in .fth files are relative to the pforth executable location. They should be relative to the file location instead.
* Segfaults instead of giving error or crash info.
* Hexcode instead of word on unknown word.
* TOS does not work if the stack is empty.
* 

---

---


## PForth Base
This work was forked from the [pForth](https://github.com/philburk/pforth) created by Phil Burk with Larry Polansky, David Rosenboom and Darren Gibbs and Support for 64-bit cells by Aleksej Saushev.

The PForth base was last updated on November 27, 2022

PForth is based on ANSI-Forth but is not 100% compatible. https://forth-standard.org/standard/words

Code for the base pForth is maintained on GitHub at: https://github.com/philburk/pforth

Documentation for base pForth at: http://www.softsynth.com/pforth/

  

## LEGAL NOTICE

Permission to use, copy, modify, and/or distribute this
software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## Contents of SDK

    platforms - tools for building pForth on various platforms
    platforms/unix - Makefile for unix

    csrc - pForth kernel in ANSI 'C'
    csrc/pf_main.c - main() application for a standalone Forth
    csrc/stdio - I/O code using basic stdio for generic platforms
    csrc/posix - I/O code for Posix platform
    csrc/win32 - I/O code for basic WIN32 platform
    csrc/win32_console - I/O code for WIN32 console that supports command line history

    fth - Forth code
    fth/util - utility functions

## How to Build pForth

Building pForth involves two steps:
1) building the C based Forth kernel
2) building the Forth dictionary file using: ./pforth -i system.fth
3) optional build of standalone executable with built-in dictionary

We have provided build scripts to simplify this process.

On Unix and MacOS using Makefile:

    cd platforms/unix
    make all
    ./pforth_standalone
    
For more details, see the [Wiki](https://github.com/philburk/pforth/wiki/Compiling-on-Unix)


## How to Run pForth

To run the all-in-one pForth enter:

    ./pforth_standalone
    
OR, to run using the dictionary file, enter:

    ./pforth

Quick check of Forth:

    3 4 + .
    words
    bye

To compile source code files use:

    INCLUDE filename

To create a custom dictionary enter in pForth:

    c" newfilename.dic" SAVE-FORTH
    
The name must end in ".dic".

To run PForth with the new dictionary enter in the shell:

    pforth -dnewfilename.dic

To run PForth and automatically include a forth file:
    pforth myprogram.fth
    
## How to Test pForth

PForth comes with a small test suite.  To test the Core words,
you can use the coretest developed by John Hayes.

On Unix and MacOS using Makefile:

    cd platforms/unix
    make test

To run the other tests, enter:

    pforth t_corex.fth
    pforth t_strings.fth
    pforth t_locals.fth
    pforth t_alloc.fth

They will report the number of tests that pass or fail.

You can also test pForth kernel without loading a dictionary using option "-i".
Only the primitive words defined in C will be available.
This might be necessary if the dictionary can't be built.

    ./pforth -i
    3 4 + .
    23 77 swap .s
    loadsys
