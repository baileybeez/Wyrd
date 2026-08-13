#pragma once
#include "wyrd.h"

#define kFat16_BytesPerSector  512
#define kFat16_DirEntrySize    32
#define kFat16_NameLen         11
#define kFat16_BaseLen         8
#define kFat16_ExtLen          3
#define kFat16_MaxComponentLen 13
#define kFat16_AttrVolumeId    0x08
#define kFat16_AttrLfnMask     0x0F
#define kFat16_AttrDirectory   0x10

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
   kFatErr_EndOfDir       = 9
} Fat16Error;

// this should always match xxxReadSectors as defined in the drivers (i.e ATA)
typedef bool (*Fat16ReadSectorsFn)(u32 lba, u32 count, void* dest);

typedef struct {
   Fat16ReadSectorsFn readSectors;
   u16  bytesPerSector;
   u8   sectorsPerCluster;
   u16  fatSize16;
   u16  rootEntCount;
   u32  fatStartLba;
   u32  rootDirStartLba;
   u32  dataStartLba;
   u32  bytesPerCluster;
   u32  clusterCount;
   u16* fat;
   u8*  rootDir; 
   bool mounted;
} Fat16Volume;

typedef struct {
   u8  name[11];
   u8  attr;
   u8  ntRes;
   u8  crtTimeTenth;
   u16 crtTime;
   u16 crtDate;
   u16 lstAccDate;
   u16 firstClusterHigh;
   u16 wrtTime;
   u16 wrtDate;
   u16 firstClusterLow;
   u32 fileSize;
} __attribute__((packed)) Fat16DirEntry;
_Static_assert(sizeof(Fat16DirEntry) == kFat16_DirEntrySize, "Fat16DirEntry is the wrong size (not packed?)");

typedef struct {
   const Fat16DirEntry* base;
   u16   entryIndex;
   u16   count;
   u16   cluster;
   u16   clusterOffset;
   u32   maxHops;
   u32   hopCount;
   bool  isRoot;
   u8    scratch[kFat16_BytesPerSector] __attribute__((aligned(8)));
} Fat16DirIterator;

typedef struct {
   bool isRoot;
   u16  startCluster;
} Fat16DirRef;

typedef void (*FilePrintFnc)(const char* name);

Fat16Error fat16Mount(Fat16Volume* vol, Fat16ReadSectorsFn fncReadSectors, 
                      void* fatBuffer, u32 fatBufferSize, 
                      void* rootDirBuffer, u32 rootDirBufferSize);
Fat16Error fat16NameTo8_3(const char* name, u8 out[kFat16_NameLen]);
Fat16Error fat16FindInDir(const Fat16Volume* vol, Fat16DirRef dir, const u8 name8_3[kFat16_NameLen], Fat16DirEntry* out);
Fat16Error fat16FindFile(const Fat16Volume* vol, const char* path, u16* outFirstCluster, u32* outFileSize);
Fat16Error fat16ReadFileRange(const Fat16Volume* vol, u16 firstCluster, u32 fileSize,
                              u32 offset, u32 len, void* dest, u32* outBytesRead);
Fat16Error fat16DirIterInit(const Fat16Volume* vol, Fat16DirRef dir, Fat16DirIterator* outIter);
Fat16Error fat16DirIterNext(const Fat16Volume* vol, Fat16DirIterator* iter);
const Fat16DirEntry* fat16DirIterEntry(const Fat16DirIterator* iter);

#ifdef kIncludeSelfTests
void fat16SelfTest(const Fat16Volume* vol);
#endif
