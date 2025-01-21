#include "stdio.h"
#include "x86.h"

void putc(char c) {
    x86_Video_WriteCharTeletype(c, 0);
}

void puts(const char *str) {
    while (*str) {
        putc(*str);
        str++;
    }
}


#define _PRINTFSTATE_NORMAL        0 // Normal characters
#define _PRINTFSTATE_LENGTH        1 // Check for length characters
#define _PRINTFSTATE_LENGTH_SHORT  2
#define _PRINTFSTATE_LENGTH_LONG   3
#define _PRINTFSTATE_SPECIFIER     4 // What to print

#define _PRINTF_LENGTH_DEFAULT     0
#define _PRINTF_LENGTH_SHORT_SHORT 1
#define _PRINTF_LENGTH_SHORT       2
#define _PRINTF_LENGTH_LONG        3
#define _PRINTF_LENGTH_LONG_LONG   4

void _cdecl printf(const char *fmt, ...) {
    int *argp = (int*)&fmt;
    int state = _PRINTFSTATE_NORMAL;
    int length = _PRINTF_LENGTH_DEFAULT;

    argp++;
    while (*fmt) {
        switch (state) {
        case _PRINTFSTATE_NORMAL:
            switch (*fmt) {
                case '%': state = _PRINTFSTATE_LENGTH; break;
                default:  putc(*fmt); break;
            }
            break;
        
        case _PRINTFSTATE_LENGTH:
            switch (*fmt) {
                case 'h':
                    length = _PRINTF_LENGTH_SHORT;
                    state  = _PRINTFSTATE_LENGTH_SHORT;
                    break;
                case 'l':
                    length = _PRINTF_LENGTH_LONG;
                    state  = _PRINTFSTATE_LENGTH_LONG;
                    break;
                default:
                    goto PRINTFSTATE_SPEC_;

            }
            break;
        
        case _PRINTFSTATE_LENGTH_SHORT:
            if ( *fmt == 'h' ) {
                length = _PRINTF_LENGTH_SHORT_SHORT;
                state = _PRINTFSTATE_SPECIFIER;
            }
            else goto PRINTFSTATE_SPEC_;
            break;

        case _PRINTFSTATE_LENGTH_LONG:
            if ( *fmt == 'l' ) {
                length = _PRINTF_LENGTH_LONG_LONG;
                state = _PRINTFSTATE_SPECIFIER;
            }
            else goto PRINTFSTATE_SPEC_;
            break;
        
        case _PRINTFSTATE_SPECIFIER:
        PRINTFSTATE_SPEC_:
            switch (*fmt) {
                case 'c':
                    putc((char)*argp);
                    argp++;
                    break;
            }
            break;

        }
        fmt++;
    }
}