#pragma once
#include "wyrd.h"
#include "fs/fat16/fat16.h"
#include "fs/vfs/vfs.h"

typedef struct {
   Fat16DirIterator iter;
   u8 scratch[kFat16_BytesPerSector] __attribute__((aligned(8)));
   bool open;
   bool exhausted;
} Fat16Cursor;
_Static_assert(sizeof(Fat16Cursor) <= kVfsPrivateDirSize, "Fat16 Cursor is too large");

const VfsBackend* vfsMountFat16(const Fat16Volume* vol);
