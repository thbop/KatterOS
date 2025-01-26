#pragma once

#include "stdint.h"
#include "disk.h"

#pragma pack(push, 1)
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
} FAT_DirectoryEntry;
#pragma pack(pop) // Packs

typedef struct {
    int handle;
    bool isDirectory;
    uint32_t position;
    uint32_t size;
} FAT_FILE;

bool FAT_Initialize(DISK *disk);
FAT_FILE far *FAT_Open(DISK *disk, const char *path);
uint32_t FAT_Read(DISK *disk, FAT_FILE far *file, uint32_t byteCount, void *buffer);
bool FAT_ReadEntry(DISK *disk, FAT_FILE far *file, FAT_DirectoryEntry *dirEntry);
void FAT_Close(DISK *disk);
