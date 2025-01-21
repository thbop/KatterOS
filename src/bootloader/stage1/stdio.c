#include "stdio.h"
#include "stdint.h"
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

int *_printf_number(int *argp, int length, bool sign, int radix);

void _cdecl printf(const char *fmt, ...) {
    int *argp = (int*)&fmt;
    int state = _PRINTFSTATE_NORMAL;
    int length = _PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;

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
                case 's':
                    puts(*(char**)argp);
                    argp++;
                    break;
                case '%':
                    putc('%');
                    break;
                
                case 'd':
                case 'i':
                    radix = 10; sign = true;
                    argp = _printf_number(argp, length, sign, radix);
                    break;
                
                case 'u':
                    radix = 10; sign = false;
                    argp = _printf_number(argp, length, sign, radix);
                    break;
                
                case 'X':
                case 'x':
                case 'p':
                    radix = 16; sign = false;
                    argp = _printf_number(argp, length, sign, radix);
                    break;
                
                case 'o':
                    radix = 8; sign = false;
                    argp = _printf_number(argp, length, sign, radix);
                    break;
                
                // Ignore invalid specifiers
                default: break;
            }
            // Reset state
            state = _PRINTFSTATE_NORMAL;
            length = _PRINTF_LENGTH_DEFAULT;
            radix = 10;
            sign = false;
            break;

        }
        fmt++;
    }
}

const char g_HexChars[] = "0123456789ABCDEF";

int *_printf_number(int *argp, int length, bool sign, int radix) {
    char buffer[32];     // Store result
    uint64_t number;     // Number
    int number_sign = 1; // 1 or -1
    int pos = 0;         // Current position in the buffer

    // Process length
    switch (length) {
        case _PRINTF_LENGTH_SHORT_SHORT:
        case _PRINTF_LENGTH_SHORT:
        case _PRINTF_LENGTH_DEFAULT:
            if ( sign ) {
                int n = *argp;
                if ( n < 0 ) {
                    n = -n;
                    number_sign = -1;
                }
                number = (uint64_t)n;
            }
            else {
                number = *(unsigned int*)argp;
            }
            argp++;
            break;

        case _PRINTF_LENGTH_LONG:
            if ( sign ) {
                long int n = *(long int*)argp;
                if ( n < 0 ) {
                    n = -n;
                    number_sign = -1;
                }
                number = (uint64_t)n;
            }
            else {
                number = *(unsigned long int*)argp;
            }
            argp += 2;
            break;
        
        case _PRINTF_LENGTH_LONG_LONG:
            if ( sign ) {
                long long int n = *(long long int*)argp;
                if ( n < 0 ) {
                    n = -n;
                    number_sign = -1;
                }
                number = (uint64_t)n;
            }
            else {
                number = *(unsigned long long*)argp;
            }
            argp += 4;
            break;
    }

    // Convert number to ASCII
    do {
        uint32_t rem;
        x86_div64_32(number, radix, &number, &rem);
        // number = number / radix;
        // rem = number % radix;

        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // Add sign if needed
    if (sign && number_sign < 0)
        buffer[pos++] = '-';
    
    // Print in reverse order
    while ( --pos >= 0 )
        putc(buffer[pos]);
    
    return argp;

}