#include "wyrd.h"
#include "fat16backend.h"
#include "fs/fat16/fat16.h"
#include "fs/vfs/vfs.h"
#include "lib/panic.h"
#include "string.h"

#define kFat16RootCluster   0

static const Fat16Volume* _vol          = nil;
static VfsBackend         _fat16backend = {0};

static Fat16DirCursor* _dirCursor(Dir* dir)
{
   return (Fat16DirCursor*)(u8*)dir->priv;
}

static Fat16FileCursor* _fileCursor(File* file)
{
   return (Fat16FileCursor*)(u8*)file->priv;
}

static VfsError _advanceDirCursor(Fat16DirCursor* cursor)
{
   Fat16Error err = fat16DirIterNext(_vol, &cursor->iter);
   if (err == kFatErr_OK)
      return kVfsErr_OK;

   cursor->exhausted = true;
   return err == kFatErr_EndOfDir ? kVfsErr_OK : kVfsErr_IO;
}

static VfsError _mapError(Fat16Error err)
{
   switch (err) 
   {
      case kFatErr_OK:           return kVfsErr_OK;
      case kFatErr_EndOfDir:     return kVfsErr_EndOfDirectory;
      case kFatErr_FileNotFound: return kVfsErr_NotFound;
      case kFatErr_BadName:      return kVfsErr_NotFound;
      default:                   return kVfsErr_IO;
   }
}

static VfsError _fat16backend_getRoot(VfsNodeRef* outNode)
{
   if (!_vol) 
      return kVfsErr_Uninitialized;
      
   outNode->id   = kFat16RootCluster;
   outNode->size = 0;
   outNode->type = kNodeType_Directory;
   return kVfsErr_OK;
}

static VfsError _fat16backend_lookup(const VfsNodeRef* node, const char* name, VfsNodeRef* outNode)
{
   if (!_vol)
      return kVfsErr_Uninitialized;

   u8 name8_3[kFat16_NameLen];
   Fat16Error err = fat16NameTo8_3(name, name8_3);
   if (err != kFatErr_OK)
      return kVfsErr_NotFound;

   Fat16DirEntry entry;
   Fat16DirRef   ref = { .isRoot = (node->id == kFat16RootCluster), 
                         .startCluster = (u16)node->id };

   err = fat16FindInDir(_vol, ref, name8_3, &entry);
   if (err != kFatErr_OK)
      return _mapError(err);

   outNode->id   = entry.firstClusterLow;
   outNode->size = entry.fileSize;
   outNode->type = (entry.attr & kFat16_AttrDirectory) ? kNodeType_Directory : kNodeType_File;
   return kVfsErr_OK;
}

static VfsError _fat16backend_openDir(const VfsNodeRef* node, Dir* outDir)
{
   if (!_vol)
      return kVfsErr_Uninitialized;

   Fat16DirRef ref = { .isRoot = (node->id == kFat16RootCluster), 
                       .startCluster = (u16)node->id };

   Fat16DirCursor* cursor = _dirCursor(outDir);
   Fat16Error err = fat16DirIterInit(_vol, ref, cursor->scratch, &cursor->iter);
   if (err == kFatErr_EndOfDir) {
      cursor->open = true;
      return kVfsErr_OK;
   }

   if (err != kFatErr_OK) 
      return _mapError(err);
   
   cursor->open      = true;
   cursor->exhausted = false;
   return kVfsErr_OK;
}

static VfsError _fat16backend_readDir(Dir* dir, DirEntry* outEntry)
{
   Fat16DirCursor* cursor = _dirCursor(dir);
   if (cursor->exhausted || !cursor->open)
      return kVfsErr_EndOfDirectory;

   const Fat16DirEntry* entry = fat16DirIterEntry(&cursor->iter);
   if (entry == nil) {
      cursor->exhausted = true;
      return kVfsErr_EndOfDirectory;
   }

   if (fat168_3ToName((u8*)entry->name, outEntry->name) != kFatErr_OK) {
      cursor->exhausted = true;
      return kVfsErr_IO;
   }

   outEntry->type = (entry->attr & kFat16_AttrDirectory) ? 
                        kNodeType_Directory : kNodeType_File;
   outEntry->size = entry->fileSize;
   outEntry->modifiedTime = 0; // TODO: 

   return _advanceDirCursor(cursor);
}

static VfsError _fat16backend_closeDir(Dir* dir)
{
   Fat16DirCursor* cursor = _dirCursor(dir);
   cursor->open      = false;
   cursor->exhausted = true;
   return kVfsErr_OK;
}

static VfsError _fat16backend_openFile(const VfsNodeRef* node, File* outFile)
{
   if (!_vol)
      return kVfsErr_Uninitialized;

   Fat16FileCursor* cursor = _fileCursor(outFile);
   cursor->firstCluster    = (u16)node->id;
   cursor->fileSize        = node->size;
   cursor->open            = true;
   return kVfsErr_OK;
}

static VfsError _fat16backend_readFile(File* file, u32 offset, u32 len, void* dest, u32* outRead)
{
   *outRead = 0;
   if (!_vol)
      return kVfsErr_Uninitialized;
      
   Fat16FileCursor* cursor = _fileCursor(file);
   if (!cursor->open)
      return kVfsErr_NotOpen;

   Fat16Error err = fat16ReadFileRange(_vol, cursor->firstCluster, cursor->fileSize, 
                                       offset, len, dest, outRead);

   return _mapError(err);
}

static VfsError _fat16backend_closeFile(File* file)
{
   Fat16FileCursor* cursor = _fileCursor(file);
   cursor->open = false;
   return kVfsErr_OK;
}

const VfsBackend* vfsMountFat16(const Fat16Volume* vol)
{
   _vol = vol;
   _fat16backend.mode      = kVfsMode_FAT16;
   _fat16backend.root      = _fat16backend_getRoot;
   _fat16backend.lookup    = _fat16backend_lookup;
   _fat16backend.openDir   = _fat16backend_openDir;
   _fat16backend.readDir   = _fat16backend_readDir;
   _fat16backend.closeDir  = _fat16backend_closeDir;
   _fat16backend.openFile  = _fat16backend_openFile;
   _fat16backend.readFile  = _fat16backend_readFile;
   _fat16backend.closeFile = _fat16backend_closeFile;

   return (const VfsBackend*)&_fat16backend;
}
