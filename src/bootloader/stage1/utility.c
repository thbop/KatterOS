#include "utility.h"

uint32_t align(uint32_t value, uint32_t alignTo) {
    if ( alignTo == 0 ) return value;

    uint32_t rem = value % alignTo;
    return ( rem > 0 ) ? ( value + alignTo - rem ) : value;
}