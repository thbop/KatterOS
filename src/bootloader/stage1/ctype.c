#include "ctype.h"

bool islower(char chr) {
    return 'a' < chr && chr < 'z';
}
char toupper(char chr) {
    return islower(chr) ? chr + 0x20 : chr;
}