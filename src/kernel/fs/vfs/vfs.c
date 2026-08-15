#include "wyrd.h"
#include "vfs.h"
#include "fs/vfs/fat16backend.h"
#include "lib/mem.h"
#include "string.h"

#define kExtSep   '.'
#define kPathSep  '/'

static const VfsBackend* _backend = nil;

static VfsError _splitComponent(const char** path, char* outName, u32 nameLen)
{
   const char* p = *path;
   while (*p == kPathSep)
      p++;

   if (*p == '\0') { 
      *path = p;
      return kVfsErr_EndOfPath;
   }

   const char* q = strchr(p, kPathSep);   
   u32 len = q  ? (u32)q - (u32)p : strlen(p);
   if (len >= nameLen)
      return kVfsErr_NameTooLong;

   strncpy(outName, p, len);
   outName[len] = '\0';

   *path = p + len;
   return kVfsErr_OK;
}

static VfsError _resolve(const char* path, VfsNodeRef* outNode)
{
   VfsNodeRef node = { .id = 0, .aux = 0, .type = kNodeType_Directory };
   VfsError   err = _backend->root(&node);
   if (err != kVfsErr_OK)
      return err;

   const char* p    = path;
   char        name[kNameBufferSize];

   while((err = _splitComponent(&p, name, sizeof(name))) == kVfsErr_OK) {
      if (node.type != kNodeType_Directory)
         return kVfsErr_NotADirectory;

      err = _backend->lookup(&node, name, &node);
      if (err != kVfsErr_OK)
         return err;
   }

   if (err != kVfsErr_EndOfPath)
      return err;

   *outNode = node;
   return kVfsErr_OK;
}

VfsError vfsInit(const VfsBackend* backend)
{
   _backend = backend;
   if (_backend == nil || _backend->mode == kVfsMode_None)
      return kVfsErr_Uninitialized;

   return kVfsErr_OK;
}

VfsError vfsDirOpen(const char* path, Dir* outDir)
{
   VfsNodeRef  node;

   VfsError err = _resolve(path, &node);
   if (err != kVfsErr_OK)
      return err;

   if (node.type != kNodeType_Directory)
      return kVfsErr_NotADirectory;

   return _backend->openDir(&node, outDir);
}

VfsError vfsDirRead(Dir* entry, DirEntry* outEntry)
{
   return _backend->readDir(entry, outEntry);
}

VfsError vfsDirClose(Dir* entry)
{
   return _backend->closeDir(entry);
}
