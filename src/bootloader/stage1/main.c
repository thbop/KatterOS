#include "stdint.h"
#include "stdio.h"
#include "disk.h"
#include "fat.h"

void _cdecl cstart_(uint16_t bootDrive) {
    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive)) {
        printf("DISK: Failed to initialize disk!\r\n");
        goto end;
    }

    if (!FAT_Initialize(&disk)) {
        printf("FAT: Failed to intialize FAT!\r\n");
        goto end;
    }

    // Browse files in root
    FAT_FILE far* fd = FAT_Open(&disk, "/");
    FAT_DirectoryEntry entry;
    int i = 0;
    while (FAT_ReadEntry(&disk, fd, &entry) && i++ < 10) {
        printf("    ");
        for ( int i = 0; i < 11; i++ )
            putc(entry.name[i]);
        printf("\r\n");
    }
    FAT_Close(fd);

end:
    for(;;);
}
