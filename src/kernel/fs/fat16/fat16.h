#pragma once
#include "wyrd.h"

typedef enum
{
   kFatErr_OK             = 0,
   kFatErr_DiskRead       = 1,
   kFatErr_BadBPB         = 2,
   kFatErr_FileNotFound   = 3,
   kFatErr_BadCluster     = 4,
   kFatErr_FatOverflow    = 5,
   kFatErr_RootOverflow   = 6,
   kFatErr_ShortRead      = 7,
   kFatErr_BadName        = 8,
   kFatErr_Continue       = 9
} Fat16Error;

// this should always match xxxReadSectors as defined in the drivers (i.e ATA)
typedef bool (*kFat16ReadSectorsFn)(u32 lba, u32 count, void* dest);

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
Fat16Error fat16ReadFileRange(const Fat16Volume* vol, u16 firstCluster, u32 fileSize,
                              u32 offset, u32 len, void* dest);

#ifdef kIncludeSelfTests
void fat16SelfTest(const Fat16Volume* vol);
#endif
