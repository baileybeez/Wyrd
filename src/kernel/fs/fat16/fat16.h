#pragma once
#include "wyrd.h"

typedef enum
{
   kFAT16_OK             = 0,
   kFAT16_DiskRead       = 1,
   kFAT16_BadBPB         = 2,
   kFAT16_FileNotFound   = 3,
   kFAT16_BadCluster     = 4,
   kFAT16_FatOverflow    = 5,
   kFAT16_RootOverflow   = 6,
   kFAT16_ShortRead      = 7,
   kFAT16_BadName        = 8
} Fat16Error;

typedef bool (*kFat16ReadSectorsFn)(u32 lba, u8 count, void* dest);

typedef struct {
   kFat16ReadSectorsFn readSectors;
   u16  bytesPerSector;
   u8   sectorsPerCluster;
   u16  fatSize16;
   u16  rootEntCount;
   u32  fatStartLba;
   u32  rootDirStartLba;
   u32  dataStartLba;
   u32  bytesPerCluster;
   u16* fat;
   u8*  rootDir; 
} Fat16Volume;

Fat16Error fat16Mount(Fat16Volume* vol, kFat16ReadSectorsFn fncReadSectors, 
                      void* fatBuffer, u32 fatBufferSize, 
                      void* rootDirBuffer, u32 rootDirBufferSize);

Fat16Error fat16FindFile(const Fat16Volume* vol, const char* path, u16* outFirstCluster, u32* outFileSize);

Fat16Error fat16ReadFile(const Fat16Volume* vol, u16 firstCluster, u32 fileSize, void* dest);
