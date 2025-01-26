#include "fat.h"
#include "stdio.h"
#include "memdefs.h"
#include "utility.h"

#define SECTOR_SIZE 512

#pragma pack(push, 1)
typedef struct {
    uint8_t  bootJumpInstruction[3];
    uint8_t  oemIdentifier[8];
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluser;
    uint16_t reservedSectors;
    uint8_t  fatCount;
    uint16_t dirEntryCount;
    uint16_t totalSectors;
    uint8_t  mediaDescriptorType;
    uint16_t sectorsPerFat;
    uint16_t sectorsPerTrack;
    uint16_t heads;
    uint32_t hiddenSectors;
    uint32_t largeSectors;

    // Extended boot record
    uint8_t  driveNumber;
    uint8_t  _reserved;       
    uint8_t  signature;
    uint32_t volumeId;
    uint8_t  volumeLabel[11];
    uint8_t  systemId[8];

    // Skip code
} FAT_BootSector;
#pragma pack(pop) // Packs

typedef struct {
    union {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;
    
} FAT_Data;


// Globals

static FAT_Data           far *g_Data;
static uint8_t            far *g_Fat = NULL;                  // File Allocation Table
static FAT_DirectoryEntry far *g_RootDirectory = NULL;
static uint32_t g_RootDirectoryEnd;                                  // How many sectors to the data section


bool FAT_ReadBootSector(DISK *disk) {
    return DISK_ReadSectors(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK *disk) {
    return DISK_ReadSectors(disk, g_Data->BS.BootSector.reservedSectors, g_Data->BS.BootSector.sectorsPerFat, g_Fat);
}

bool FAT_ReadRootDirectory(DISK *disk) {
    uint32_t lba     = g_Data->BS.BootSector.reservedSectors + g_Data->BS.BootSector.sectorsPerFat * g_Data->BS.BootSector.fatCount;
    uint32_t size    = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.dirEntryCount;
    uint32_t sectors = ( size / g_Data->BS.BootSector.bytesPerSector );

    
    g_RootDirectoryEnd = lba + sectors;
    g_RootDirectory = (FAT_DirectoryEntry*)malloc(sectors * g_Data->BS.BootSector.bytesPerSector);
    return DISK_ReadSectors(disk, lba, sectors, g_RootDirectory);
}

bool FAT_Initialize(DISK *disk) {
    g_Data = MEMORY_FAT_MIN;

    // Read Boot Sector
    if ( !FAT_ReadBootSector(disk) ) {
        printf("FAT: Boot sector read failed!\r\n");
        return false;
    }

    // Read FAT
    g_Fat = (uint8_t far*)(g_Data + sizeof(FAT_Data));
    uint32_t fatSize = g_Data->BS.BootSector.bytesPerSector * g_Data->BS.BootSector.sectorsPerFat;
    if ( sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE ) {
        printf("FAT: Not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
        return false;
    }
    if ( !FAT_ReadFat(disk) ) {
        printf("FAT: Failed to read FAT!");
        return false;
    }

    // Read root directory
    g_RootDirectory = (FAT_DirectoryEntry far*)(g_Fat + fatSize);
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.dirEntryCount;
    rootDirSize = align(rootDirSize, g_Data->BS.BootSector.bytesPerSector);

    if ( sizeof(FAT_Data) + fatSize + rootDirSize >= MEMORY_FAT_SIZE ) {
        printf("FAT: Not enough memory to read root directory! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize + rootDirSize, MEMORY_FAT_SIZE);
        return false;
    }

    if ( !FAT_ReadRootDirectory(disk) ) {
        printf("FAT: Failed to read root directory!\r\n");
        return false;
    }

    return true;
}

FAT_FILE far *FAT_Open(DISK *disk, const char *path) {

}



// DirectoryEntry *findFile(const char *name) {
//     for ( uint32_t i = 0; i < g_BootSector.dirEntryCount; i++ ) {
//         // printf("OUT: %d\n", g_RootDirectory[i].name[0]);
//         if ( memcmp(name, g_RootDirectory[i].name, 11) == 0 ) {
//             return &g_RootDirectory[i];
//         }
//     }
//     return NULL;
// }

// bool readFile(DirectoryEntry *fileEntry, FILE *disk, uint8_t *outputBuffer) {
//     bool ok = true;
//     uint16_t currentCluster = fileEntry->firstClusterLow;

//     do {
//         uint32_t lba = g_RootDirectoryEnd + (currentCluster-2) * g_BootSector.sectorsPerCluser; // -2 since the first 2 clusters are reserved
//         ok = ok && readSectors(disk, lba, g_BootSector.sectorsPerCluser, outputBuffer);
//         outputBuffer += g_BootSector.sectorsPerCluser * g_BootSector.bytesPerSector; // Cheeky

//         uint32_t fatIndex = (currentCluster * 3) / 2; // currentCluster * 3/2 since each FAT entry is 12 bits
//         if (currentCluster % 2 == 0)
//             currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
//         else
//             currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
//     } while (ok && currentCluster < 0x0FF8); // 0x0FF8 marks the end of the cluster chain

//     return ok;
// }

// int main(int argc, char **argv) {
//     if (argc < 3) {
//         printf("Syntax: %s <disk image> <filename>\n", argv[0]);
//         return -1;
//     }

//     FILE *disk = fopen(argv[1], "rb");
//     if ( !disk ) {
//         fprintf(stderr, "Cannot open disk image %s!\n", argv[1]);
//         return -1;
//     }

//     if ( !readBootSector(disk) ) {
//         fprintf(stderr, "Failed to initialize boot sector!\n");
//         return -2;
//     }


//     if ( !readFat(disk) ) {
//         fprintf(stderr, "Failed to initialize FAT!\n");
//         free(g_Fat);
//         return -3;
//     }

//     if ( !readRootDirectory(disk) ) {
//         fprintf(stderr, "Failed to initialize root directory!\n");
//         free(g_Fat);
//         free(g_RootDirectory);
//         return -4;
//     }
    
//     // printf("%d\n", g_RootDirectory[0].size);
//     DirectoryEntry *fileEntry = findFile(argv[2]);
//     if ( !fileEntry ) {
//         fprintf(stderr, "Failed to find file \"%s\"!\n", argv[2]);
//         free(g_Fat);
//         free(g_RootDirectory);
//         return -5;
//     }
//     uint8_t *buffer = (uint8_t*)malloc(fileEntry->size + g_BootSector.bytesPerSector);
//     if ( !readFile(fileEntry, disk, buffer) ) {
//         fprintf(stderr, "Failed to read file \"%s\"!\n", argv[2]);
//         free(g_Fat);
//         free(g_RootDirectory);
//         free(buffer);
//         return -6;
//     }
//     for ( size_t i = 0; i < fileEntry->size; i++ ) {
//         if ( isprint(buffer[i]) ) fputc(buffer[i], stdout);
//         else                      printf("<%02X>", buffer[i]);
//     }
//     printf("\n");
    
//     free(g_Fat);
//     free(g_RootDirectory);
//     free(buffer);
//     fclose(disk);
//     return 0;
// }