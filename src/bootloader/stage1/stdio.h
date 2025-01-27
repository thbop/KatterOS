#pragma once

#ifdef __INTELLISENSE__
#define far
#endif

void putc(char c);
void puts(const char *str);

void _cdecl printf(const char *fmt, ...);
