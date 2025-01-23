#include "stdint.h"
#include "stdio.h"
#include "x86.h"

void _cdecl cstart_(uint16_t bootDrive) {
    uint8_t driveType;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;

    x86_Disk_GetDriveParams(bootDrive, &driveType, &cylinders, &sectors, &heads);

    printf("Type: %u\r\nCylinders: %u\r\nSectors: %u\r\nHeads: %u", driveType, cylinders, sectors, heads);
    for(;;);
}
