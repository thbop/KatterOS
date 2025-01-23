#ifdef __INTELLISENSE__
#define far
#endif

#pragma once
#include "stdint.h"

void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient, uint32_t *remainder);

void _cdecl x86_Video_WriteCharTeletype(char c, uint8_t page);

void _cdecl x86_Disk_Reset(uint8_t driveNumber);

void _cdecl x86_Disk_Read(uint8_t driveNumber,
                          uint16_t cylinder,
                          uint16_t sector,
                          uint16_t head,
                          uint8_t count,
                          uint8_t far *destBuffer);

void _cdecl x86_Disk_GetDriveParams(uint8_t driveNumber,
                                    uint8_t *driveTypeOut,
                                    uint16_t *cylindersOut,
                                    uint16_t *sectorsOut,
                                    uint16_t *headsOut);
