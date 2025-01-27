#include "fat.h"
#include "stdio.h"
#include "memdefs.h"
#include "utility.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"

#define SECTOR_SIZE           512
#define MAX_PATH_SIZE         256
#define MAX_FILE_HANDLES      10
#define ROOT_DIRECTORY_HANDLE -1

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
    FAT_FILE public;
    bool opened;
    uint32_t firstCluster;
    uint32_t currentCluster;
    uint32_t currentSectorInCluster;
    uint8_t buffer[SECTOR_SIZE]

} FAT_FileData;

typedef struct {
    union {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;
    FAT_FileData openFiles[MAX_FILE_HANDLES];
    
} FAT_Data;


// Globals

static FAT_Data far *g_Data;
static uint8_t  far *g_Fat = NULL;  // File Allocation Table
static uint32_t g_DataSectionLba;


bool FAT_ReadBootSector(DISK *disk) {
    return DISK_ReadSectors(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK *disk) {
    return DISK_ReadSectors(disk, g_Data->BS.BootSector.reservedSectors, g_Data->BS.BootSector.sectorsPerFat, g_Fat);
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
        printf("FAT: Failed to read FAT!\r\n");
        return false;
    }

    // Read root directory
    uint32_t rootDirLba  = g_Data->BS.BootSector.reservedSectors + g_Data->BS.BootSector.sectorsPerFat * g_Data->BS.BootSector.fatCount;
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.dirEntryCount;


    // Setup root directory file
    g_Data->RootDirectory.opened                 = true;
    g_Data->RootDirectory.public.handle          = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.public.isDirectory     = true;
    g_Data->RootDirectory.public.position        = 0;
    g_Data->RootDirectory.public.size            = rootDirSize;
    g_Data->RootDirectory.firstCluster           = rootDirLba;
    g_Data->RootDirectory.currentCluster         = rootDirLba;
    g_Data->RootDirectory.currentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.buffer)) {
        printf("FAT: Failed to read root directory!\r\n");
        return false;
    }

    // Calculate data section
    uint32_t rootDirSectors = ( rootDirSize + g_Data->BS.BootSector.bytesPerSector - 1 ) / g_Data->BS.BootSector.bytesPerSector;
    g_DataSectionLba = rootDirLba + rootDirSectors;

    // Reset opened files
    for ( int i = 0; i < MAX_FILE_HANDLES; i++ ) {
        g_Data->openFiles[i].opened = false;
    }

    return true;
}

uint32_t FAT_ClusterToLba(uint32_t cluster) {
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.sectorsPerCluser;
}

FAT_FILE *FAT_OpenEntry(DISK *disk, FAT_DirectoryEntry *entry) {
    int handle = -1;
    for ( int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++ ) {
        if ( !g_Data->openFiles[i].opened ) handle = i;
    }

    // If we ran out of handles
    if ( handle < 0 ) {
        printf("FAT: Ran out of file handles!\r\n");
        return NULL;
    }

    // Setup fields
    FAT_FileData far *fd       = &g_Data->openFiles[handle];
    fd->public.handle          = handle;
    fd->public.isDirectory     = entry->attributes & FAT_ATTRIBUTE_DIRECTORY;
    fd->public.position        = 0;
    fd->public.size            = entry->size;
    fd->currentCluster         =
    fd->firstCluster           = entry->firstClusterLow + ((uint32_t)entry->firstClusterHigh << 16);
    fd->currentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->currentCluster), 1, fd->buffer)) {
        printf("FAT: Read error!\r\n");
        return NULL;
    }

    fd->opened = true;
    return &fd->public;
}

uint32_t FAT_NextCluster(uint32_t currentCluster) {
    uint32_t fatIndex = currentCluster * 3 / 2;
    if (currentCluster % 2 == 0)
        return (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
}

uint32_t FAT_Read(DISK *disk, FAT_FILE far *file, uint32_t byteCount, void *buffer) {
    // Get file data
    FAT_FileData far* fd = (file->handle == ROOT_DIRECTORY_HANDLE)
        ? &g_Data->RootDirectory
        : &g_Data->openFiles[file->handle];
    
    uint8_t *u8buffer = (uint8_t*)buffer;


    byteCount = min(byteCount, fd->public.size - fd->public.position);

    while (byteCount > 0) {
        
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8buffer, fd->buffer + fd->public.position % SECTOR_SIZE, take);
        u8buffer += take;
        fd->public.position += take;
        byteCount -= take;

        // See if we need to read more data
        if ( leftInBuffer == take ) {
            // Special handling for root directory
            if ( fd->public.handle == ROOT_DIRECTORY_HANDLE ) {
                ++fd->currentCluster;
                if (!DISK_ReadSectors(disk, fd->currentCluster, 1, fd->buffer)) {
                    printf("FAT: Failed to read root directory!\r\n");
                    break;
                }
            }
            else {
                // Calculate next cluster and sector to read
                if ( ++fd->currentSectorInCluster >= g_Data->BS.BootSector.sectorsPerCluser ) {
                    fd->currentSectorInCluster = 0; // If past probably 2
                    fd->currentCluster = FAT_NextCluster(fd->currentCluster);
                }
                if (fd->currentCluster >= 0xFF8) {
                    printf("FAT: Read file error EOF!\r\n");
                    break;
                }

                // Read next sector
                if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->currentCluster) + fd->currentSectorInCluster, 1, fd->buffer)) {
                    printf("FAT: Failed to read file!\r\n");
                    break;
                }
            }
        }
    }

    return u8buffer - (uint8_t*)buffer;
}

bool FAT_ReadEntry(DISK *disk, FAT_FILE far *file, FAT_DirectoryEntry *dirEntry) {
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry); // Return true on success
}

void FAT_Close(FAT_FILE far *file) {
    if ( file->handle == ROOT_DIRECTORY_HANDLE ) {
        file->position = 0;
        g_Data->RootDirectory.currentCluster = g_Data->RootDirectory.firstCluster;
    }
    else {
        g_Data->openFiles[file->handle].opened = false;
    }
}

bool FAT_FindFile(DISK *disk, FAT_FILE far *file, const char *name, FAT_DirectoryEntry *entryOut) {
    char fatName[11];

    // Convert to FAT12 filename format
    memset(fatName, ' ', sizeof(fatName));

    const char *ext = strchr(name, '.'); // File names can only have one period
    if ( ext == NULL )
        ext = name + 11; // No file extension

    for ( int i = 0; i < 8 && name + i < ext; i++ ) {
        fatName[i] = toupper(name[i]);
    }
    if ( ext != NULL ) {
        for ( int i = 0; i < 3 && ext[i+1]; i++ ) {
            fatName[i + 8] = toupper(ext[i+1]);
        }
    }

    FAT_DirectoryEntry entry;
    while ( FAT_ReadEntry(disk, file, &entry) ) {
        if (!memcmp(fatName, entry.name, 11)) {
            *entryOut = entry;
            return true;
        }
    }
    return false;
}

FAT_FILE far *FAT_Open(DISK *disk, const char *path) {
    char name[MAX_PATH_SIZE];

    // Ignore leading slash
    if ( path[0] == '/' ) path++;

    FAT_FILE far *current = &g_Data->RootDirectory.public;

    while (*path) {
        // Get file name
        bool isLast = false;
        const char *delim = strchr(path, '/');
        if ( delim != NULL ) {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len; // Exit loop
            isLast = true;
        }

        // Find directory entry
        FAT_DirectoryEntry entry;
        if ( FAT_FindFile(disk, current, name, &entry) ) {

            FAT_Close(current);

            // Check if it is a file
            if (!isLast && entry.attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
                printf("FAT: \"%s\" is not a directory\r\n", name);
                return NULL;
            }
            current = FAT_OpenEntry(disk, &entry);
        }
        else { // File not found
            FAT_Close(current);
            printf("FAT: Could not find \"%s\"!\r\n", name);
            return NULL;
        }
    }
    return current;
}


