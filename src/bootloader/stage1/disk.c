#include "disk.h"

// typedef struct {
//     uint8_t id;
//     uint16_t cylinders;
//     uint16_t sectors;
//     uint16_t heads;
// } DISK;

bool DISK_Initialize(DISK *disk, uint8_t driveNumber) {

    uint8_t driveType;
    uint16_t cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(driveNumber, &driveType, &cylinders, &sectors, &heads))
        return false;

    disk->id        = driveNumber;
    disk->cylinders = cylinders + 1;
    disk->heads     = heads + 1;
    disk->sectors   = sectors;

    return true;
}

void DISK_LBA2CHS(DISK *disk, uint16_t lba, uint16_t *cylindersOut, uint16_t *sectorsOut, uint16_t *headsOut) {
    *cylindersOut = ( lba / disk->sectors ) / disk->heads;
    *sectorsOut   = lba % disk->sectors + 1;
    *headsOut     = ( lba / disk->sectors ) % disk->heads;
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint8_t count, uint8_t far *buffer) {
    uint16_t cylinder, sector, head;
    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for ( int i = 0; i < 3; i++ ) {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, count, buffer))
            return true;
        
        x86_Disk_Reset(disk->id);
    }
    return false;
}
