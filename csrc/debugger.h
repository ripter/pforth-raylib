#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "pforth.h"

// Define the maximum number of tokens we can register
#define MAX_TOKENS 1024

// Structure to hold an ExecToken and its corresponding name
typedef struct {
    ExecToken XT;
    const char *name;
} TokenEntry;

// Global array to store the mapping between ExecTokens and names
static TokenEntry TokenRegistry[MAX_TOKENS];
static size_t TokenRegistryCount = 0;

// Macro to register a token name with its ExecToken
#define REGISTER_NAME(XT, CName) \
    do { \
        if (TokenRegistryCount < MAX_TOKENS) { \
            TokenRegistry[TokenRegistryCount].XT = (XT); \
            TokenRegistry[TokenRegistryCount].name = (CName); \
            TokenRegistryCount++; \
        } else { \
            fprintf(stderr, "Error: TokenRegistry is full, can't register more tokens.\n"); \
        } \
    } while (0)

// Function to get the token name from an ExecToken
static const char *tTokenNameFromXT(ExecToken XT) {
    for (size_t i = 0; i < TokenRegistryCount; i++) {
        if (TokenRegistry[i].XT == XT) {
            return TokenRegistry[i].name;
        }
    }
    return "UNKNOWN_TOKEN";  // Return a default value if not found
}

// Function to get the ExecToken from a token name
static ExecToken GetXTFromTokenName(const char *name) {
    for (size_t i = 0; i < TokenRegistryCount; i++) {
        if (strcmp(TokenRegistry[i].name, name) == 0) {
            return TokenRegistry[i].XT;
        }
    }
    return (ExecToken)-1;  // Return a sentinel value if not found
}

// Macro to retrieve the token name from an ExecToken
#define GET_NAME_FROM_XT(XT) GetTokenNameFromXT(XT)

// Macro to retrieve the ExecToken from a token name
#define GET_XT_FROM_NAME(name) GetXTFromTokenName(name)

#endif // DEBUGGER_H
