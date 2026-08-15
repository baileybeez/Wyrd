#include "wyrd.h"
#include "fat16backend.h"
#include "fs/fat16/fat16.h"
#include "fs/vfs/vfs.h"
#include "lib/panic.h"
#include "string.h"

#define kFat16RootCluster   0

static const Fat16Volume* _vol          = nil;
static VfsBackend         _fat16backend = {0};

static Fat16Cursor* _cursor(Dir* dir)
{
   return (Fat16Cursor*)(u8*)dir->priv;
}

static VfsError _fat16backend_getRoot(VfsNodeRef* outNode)
{
   if (!_vol) 
      return kVfsErr_Uninitialized;
      
   outNode->id  = kFat16RootCluster;
   outNode->aux = 0;
   outNode->type = kNodeType_Directory;
   return kVfsErr_OK;
}

static VfsError _fat16backend_lookup(const VfsNodeRef* node, const char* name, VfsNodeRef* outNode)
{
   u8 name8_3[kFat16_NameLen];
   Fat16Error err = fat16NameTo8_3(name, name8_3);
   if (err != kFatErr_OK)
      return kVfsErr_NotFound;

   Fat16DirRef ref = { .isRoot = (node->id == kFat16RootCluster), 
                       .startCluster = (u16)node->id };

   u8 scratch[kFat16_BytesPerSector];
   Fat16DirIterator iter;
   err = fat16DirIterInit(_vol, ref, scratch, &iter);
   if (err == kFatErr_EndOfDir)
      return kVfsErr_NotFound;
   
   while (err == kFatErr_OK) {
      const Fat16DirEntry* entry = fat16DirIterEntry(&iter);
      if (entry != nil) {
         if (strncmp((const char*)entry->name, (const char*)name8_3, kFat16_NameLen) == 0) {
            outNode->id   = entry->firstClusterLow;
            outNode->aux  = entry->fileSize;
            outNode->type = (entry->attr & kFat16_AttrDirectory) ? 
                              kNodeType_Directory : kNodeType_File;
            return kVfsErr_OK;
         }
         
         err = fat16DirIterNext(_vol, &iter);
      } else {
         err = kFatErr_FileNotFound;
      }
   }
   return kVfsErr_NotFound;
}

static VfsError _fat16backend_openDir(const VfsNodeRef* node, Dir* outDir)
{
   Fat16DirRef ref = { .isRoot = (node->id == kFat16RootCluster), 
                       .startCluster = (u16)node->id };

   Fat16Cursor* cursor = _cursor(outDir);
   Fat16Error err = fat16DirIterInit(_vol, ref, cursor->scratch, &cursor->iter);
   if (err == kFatErr_EndOfDir) {
      cursor->exhausted = true;
      return kVfsErr_OK;
   }

   cursor->open = true;
   cursor->exhausted = false;
   return kVfsErr_OK;
}

static VfsError _fat16backend_readDir(Dir* dir, DirEntry* outEntry)
{
   VfsError ret = kVfsErr_EndOfDirectory;
   Fat16Cursor* cursor = _cursor(dir);
   if (cursor->exhausted || !cursor->open)
      return kVfsErr_EndOfDirectory;

   const Fat16DirEntry* entry = fat16DirIterEntry(&cursor->iter);
   if (entry != nil) {
      u8 prettyName[kFat16_MaxComponentLen];
      fat168_3ToName(entry->name, prettyName);
      strncpy((char*)outEntry->name, (const char*)prettyName, kFat16_MaxComponentLen);
      outEntry->name[kFat16_MaxComponentLen] = '\0';

      outEntry->type = (entry->attr & kFat16_AttrDirectory) ? 
                           kNodeType_Directory : kNodeType_File;
      outEntry->size = entry->fileSize;
      outEntry->modifiedTime = 0; // TODO: 

      Fat16Error err = fat16DirIterNext(_vol, &cursor->iter);
      if (err == kFatErr_EndOfDir) {
         cursor->exhausted = true;
         return kVfsErr_OK;
      }

      ret = kVfsErr_OK;
   }

   return ret;
}

static VfsError _fat16backend_closeDir(Dir* dir)
{
   Fat16Cursor* cursor = _cursor(dir);
   cursor->open = false;
   return kVfsErr_OK;
}

const VfsBackend* vfsMountFat16(const Fat16Volume* vol)
{
   _vol = vol;
   _fat16backend.mode     = kVfsMode_FAT16;
   _fat16backend.root     = _fat16backend_getRoot;
   _fat16backend.lookup   = _fat16backend_lookup;
   _fat16backend.openDir  = _fat16backend_openDir;
   _fat16backend.readDir  = _fat16backend_readDir;
   _fat16backend.closeDir = _fat16backend_closeDir;

   return (const VfsBackend*)&_fat16backend;
}
