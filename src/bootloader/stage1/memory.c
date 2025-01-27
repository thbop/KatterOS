#include "memory.h"

int memcpy(const void far *dst, const void far *src, uint16_t num) {
    uint8_t far *u8Dst       = (uint8_t far*)dst;
    const uint8_t far *u8Src = (uint8_t far*)src;

    for (uint16_t i = 0; i < num; i++) {
        u8Dst[0] = u8Src[0];
        ++u8Dst;
        ++u8Src;
    }
}

void far *memset(const void far *ptr, int value, uint16_t num) {
    uint8_t far *u8Ptr = (uint8_t far*)ptr;

    for ( uint16_t i = 0; i < num; i++ ) {
        u8Ptr[i] = (uint8_t)value;
    }
    return ptr;
}

int memcmp(const void far *ptr1, const void far *ptr2, uint16_t num) {
    const uint8_t far *u8Ptr1 = (const uint8_t far*)ptr1;
    const uint8_t far *u8Ptr2 = (const uint8_t far*)ptr2;

    for ( uint16_t i = 0; i < num; i++ )
        if ( u8Ptr1[i] != u8Ptr2[i] )
            return 1;
    
    return 0;
}