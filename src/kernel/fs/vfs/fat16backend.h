#pragma once
#include "wyrd.h"
#include "fs/fat16/fat16.h"
#include "fs/vfs/vfs.h"

_Static_assert(kNameBufferSize >= kFat16_MaxComponentLen, 
               "VFS name buffer cannot hold a FAT 8.3 name");

typedef struct {
   Fat16DirIterator iter;
   u8 scratch[kFat16_BytesPerSector] __attribute__((aligned(8)));
   bool open;
   bool exhausted;
} Fat16DirCursor;
_Static_assert(sizeof(Fat16DirCursor) <= kVfsPrivateDirSize, 
               "Fat16 Directory Cursor is too large");

typedef struct {
   u32 fileSize;
   u16 firstCluster;
   bool open;
} Fat16FileCursor;
_Static_assert(sizeof(Fat16FileCursor) <= kVfsPrivateFileSize, 
               "Fat16 File Cursor is too large");

const VfsBackend* vfsMountFat16(const Fat16Volume* vol);
