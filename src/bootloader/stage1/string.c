#include "string.h"
#include "stdint.h"

// Return a pointer to the first instance of a character
const char *strchr(const char *str, char chr) {
    if (str == NULL) return NULL;
    while (*str) {
        if ( chr == *str ) return str;
        ++str;
    }

    return NULL;
}

char *strcpy(char *dst, const char *src) {
    if (dst == NULL) return NULL;
    if (src == NULL) { *dst = '\0'; return dst; }

    char *dest = dst;
    while ( *src ) {
        *dst = *src;
        ++dst;
        ++src;
    }
    *dst = '\0';
    return dest;
}