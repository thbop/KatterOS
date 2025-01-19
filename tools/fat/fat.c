#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
} __attribute__((packed)) BootSector; // Prevent padding

typedef struct {
    uint8_t  name[11];
    uint8_t  attributes;
    uint8_t  _reserved;
    uint8_t  createdTimeTenths;
    uint16_t createdTime;
    uint16_t createdDate;
    uint16_t accessedDate;
    uint16_t firstClusterHigh;
    uint16_t modifiedTime;
    uint16_t amodifiedDate;
    uint16_t firstClusterLow;
    uint32_t size;
} __attribute__((packed)) DirectoryEntry;

// Globals
BootSector g_BootSector;
uint8_t *g_Fat = NULL;                  // File Allocation Table
DirectoryEntry *g_RootDirectory = NULL;


bool readBootSector(FILE *disk) {
    return fread(&g_BootSector, sizeof(g_BootSector), 1, disk) > 0;
}


bool readSectors(FILE *disk, uint32_t lba, uint32_t count, void *bufferOut) {
    bool ok = true;
    ok = ok && (fseek(disk, lba * g_BootSector.bytesPerSector, SEEK_SET) == 0);
    ok = ok && (fread(bufferOut, g_BootSector.bytesPerSector, count, disk) == count);
    return ok;
}

bool readFat(FILE *disk) {
    g_Fat = (uint8_t*)malloc(g_BootSector.sectorsPerFat * g_BootSector.bytesPerSector);
    return readSectors( disk, g_BootSector.reservedSectors, g_BootSector.sectorsPerFat, g_Fat );
}

bool readRootDirectory(FILE *disk) {
    uint32_t lba     = g_BootSector.reservedSectors + g_BootSector.sectorsPerFat * g_BootSector.fatCount;
    printf("%d\n", lba * 512);
    uint32_t size    = sizeof(DirectoryEntry) * g_BootSector.dirEntryCount;
    uint32_t sectors = ( size / g_BootSector.bytesPerSector );
    if ( size % g_BootSector.bytesPerSector > 0 ) // Round up
        sectors++;
    
    g_RootDirectory = (DirectoryEntry*)malloc(sectors * g_BootSector.bytesPerSector);
    return readSectors(disk, lba, sectors, g_RootDirectory);
}

DirectoryEntry *findFile(const char *name) {
    for ( uint32_t i = 0; i < g_BootSector.dirEntryCount; i++ ) {
        // printf("OUT: %d\n", g_RootDirectory[i].name[0]);
        if ( memcmp(name, g_RootDirectory[i].name, 11) == 0 ) {
            return &g_RootDirectory[i];
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Syntax: %s <disk image> <filename>\n", argv[0]);
        return -1;
    }

    FILE *disk = fopen(argv[1], "rb");
    if ( !disk ) {
        fprintf(stderr, "Cannot open disk image %s!\n", argv[1]);
        return -1;
    }

    if ( !readBootSector(disk) ) {
        fprintf(stderr, "Failed to initialize boot sector!\n");
        return -2;
    }


    if ( !readFat(disk) ) {
        fprintf(stderr, "Failed to initialize FAT!\n");
        free(g_Fat);
        return -3;
    }

    if ( !readRootDirectory(disk) ) {
        fprintf(stderr, "Failed to initialize root directory!\n");
        free(g_Fat);
        free(g_RootDirectory);
        return -4;
    }
    
    // printf("%d\n", g_RootDirectory[0].size);

    if ( !findFile(argv[2]) ) {
        fprintf(stderr, "Failed to find file \"%s\"!\n", argv[2]);
        free(g_Fat);
        free(g_RootDirectory);
        return -5;
    }

    free(g_Fat);
    free(g_RootDirectory);
    fclose(disk);
    return 0;
}