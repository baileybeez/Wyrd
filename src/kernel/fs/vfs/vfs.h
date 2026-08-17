#pragma once
#include "wyrd.h"

#define kMaxNameLength        12
#define kNameBufferSize       (kMaxNameLength + 1)
#define kMaxPathComponents    32
#define kMaxPath              256
#define kVfsSectorSize        512
#define kVfsPrivateDirSize    640
#define kVfsPrivateFileSize   32

#define kPathSep  '/'
#define kRootDir  "/"

typedef enum {
   kVfsErr_OK              = 0,
   kVfsErr_Uninitialized   = 1,
   kVfsErr_NotADirectory   = 2,
   kVfsErr_EndOfPath       = 3,
   kVfsErr_NameTooLong     = 4,
   kVfsErr_EndOfDirectory  = 5,
   kVfsErr_NotFound        = 6,
   kVfsErr_IO              = 7,
   kVfsErr_NotOpen         = 8,
   kVfsErr_BadArgument     = 9,
} VfsError;

typedef enum {
   kVfsMode_None     = 0,
   kVfsMode_FAT16    = 1,
} VfsMode;

typedef enum {
   kNodeType_File,
   kNodeType_Directory,
} DirNodeType;

typedef struct {
   u32 id;
   u32 size;
   DirNodeType type;
} VfsNodeRef;

typedef struct {
   u8  priv[kVfsPrivateDirSize] __attribute__((aligned(8)));
} Dir;

typedef struct {
   char        name[kNameBufferSize];
   DirNodeType type;
   u32         size;
   u32         modifiedTime;
} DirEntry;

typedef struct {
   u32 size;
   u32 pos;
   u8  priv[kVfsPrivateFileSize] __attribute__((aligned(8)));
} File;

typedef struct {
   VfsMode mode;
   VfsError (*root)(VfsNodeRef* outNode);
   VfsError (*lookup)(const VfsNodeRef* node, const char* name, VfsNodeRef* outNode);
   VfsError (*openDir)(const VfsNodeRef* node, Dir* outDir);
   VfsError (*readDir)(Dir* dir, DirEntry* outEntry);
   VfsError (*closeDir)(Dir* dir);
   VfsError (*openFile)(const VfsNodeRef* node, File* outFile);
   VfsError (*readFile)(File* file, u32 offset, u32 len, void* dest, u32* outRead);
   VfsError (*closeFile)(File* file);
} VfsBackend;

VfsError vfsInit(const VfsBackend* mode);
VfsError vfsDirOpen(const char* path, Dir* outDir);
VfsError vfsDirRead(Dir* entry, DirEntry* outEntry);
VfsError vfsDirClose(Dir* entry);
VfsError vfsFileOpen(const char* filePath, File* outFile);
VfsError vfsFileRead(File* file, void* dest, u32 len, u32* outRead);
VfsError vfsFileReadAt(File* file, u32 offest, void* dest, u32 len, u32* outRead);
VfsError vfsFileSeek(File* file, u32 pos);
VfsError vfsFileClose(File* file);
bool     vfsResolvePath(const char* cwd, const char* path, char* out, u32 outSize);
bool     vfsIsDirectory(const char* path);
