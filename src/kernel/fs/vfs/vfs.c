#include "wyrd.h"
#include "vfs.h"
#include "fs/vfs/fat16backend.h"
#include "lib/mem.h"
#include "string.h"

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
   VfsNodeRef node = { .id = 0, .size = 0, .type = kNodeType_Directory };
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

static bool _isBackendComplete(const VfsBackend* backend)
{
   return backend->root     != nil && backend->lookup    != nil &&
          backend->openDir  != nil && backend->openFile  != nil &&
          backend->readDir  != nil && backend->readFile  != nil &&
          backend->closeDir != nil && backend->closeFile != nil;
}

VfsError vfsInit(const VfsBackend* backend)
{
   _backend = nil;
   if (backend == nil || backend->mode == kVfsMode_None || !_isBackendComplete(backend))
      return kVfsErr_Uninitialized;

   _backend = backend;
   return kVfsErr_OK;
}

VfsError vfsDirOpen(const char* path, Dir* outDir)
{
   if (_backend == nil)
      return kVfsErr_Uninitialized;
   if (path == nil || outDir == nil)
      return kVfsErr_BadArgument;

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
   if (_backend == nil)
      return kVfsErr_Uninitialized;
   if (entry == nil || outEntry == nil)
      return kVfsErr_BadArgument;

   return _backend->readDir(entry, outEntry);
}

VfsError vfsDirClose(Dir* entry)
{
   if (_backend == nil)
      return kVfsErr_Uninitialized;
   if (entry == nil)
      return kVfsErr_BadArgument;
      
   return _backend->closeDir(entry);
}

static bool _pushComponent(const char** parts, u32* lens, u32* depth, const char* start, u32 len)
{
   if (len == 0 || (len == 1 && start[0] == '.'))
      return true;
   if (len == 2 && start[0] == '.' && start[1] == '.') {
      if (*depth > 0)
         (*depth)--;

      return true;
   }

   if (*depth >= kMaxPathComponents)
      return false;

   parts[*depth] = start;
   lens[*depth]  = len;
   (*depth)++;  
   return true;
}

static bool _splitPath(const char* path, const char** parts, u32* lens, u32* depth)
{
   const char* p = path;
   while (*p != '\0') {
      const char* q = p;
      while (*p != kPathSep && *p != '\0')
         p++;

      if (!_pushComponent(parts, lens, depth, q, (u32)(p - q)))
         return false;

      if (*p == kPathSep)
         p++;
   }

   return true;
}

// adjust the CWD based on the relPath provided 
// if relPath doesn't start with a /, we first parse CWD into components
// then we can parse and apply components from relPath
// set outPath to the new CWD
// returns true if successful, false if not
bool vfsResolvePath(const char* cwd, const char* relPath, char* outPath, u32 capacity)
{
   const char* parts[kMaxPathComponents];
   u32         lens[kMaxPathComponents];
   u32         depth = 0;
   u32         idx   = 0;

   if (capacity < 2)
      return false;

   if (*relPath != kPathSep && !_splitPath(cwd, parts, lens, &depth))
      return false;

   if (!_splitPath(relPath, parts, lens, &depth))
      return false;

   outPath[idx++] = kPathSep;
   for (u32 i = 0; i < depth; i++) {
      u32 need = lens[i] + (i > 0 ? 1 : 0);
      if (idx + need + 1 > capacity)
         return false;

      if (i > 0)
         outPath[idx++] = kPathSep;

      memcpy(outPath + idx, parts[i], lens[i]);
      idx += lens[i];
   }

   outPath[idx++] = '\0';
   return true;
}

bool vfsIsDirectory(const char* path)
{
   if (_backend == nil || path == nil)
      return false;

   VfsNodeRef ref;
   VfsError err = _resolve(path, &ref);
   if (err != kVfsErr_OK)
      return false;

   return ref.type == kNodeType_Directory;
}

VfsError vfsFileOpen(const char* filePath, File* outFile)
{
   if (_backend == nil)
      return kVfsErr_Uninitialized;
   if (filePath == nil || outFile == nil)
      return kVfsErr_BadArgument;
      
   VfsNodeRef node;
   VfsError err = _resolve(filePath, &node);
   if (err != kVfsErr_OK)
      return err;
      
   if (node.type != kNodeType_File)
      return kVfsErr_NotFound;

   err = _backend->openFile(&node, outFile);
   if (err != kVfsErr_OK)
      return err;

   outFile->size = node.size;
   outFile->pos  = 0;
   return kVfsErr_OK;
}

VfsError vfsFileRead(File* file, void* dest, u32 len, u32* outRead)
{
   u32 pos = file != nil ? file->pos : 0;
   VfsError err = vfsFileReadAt(file, pos, dest, len, outRead);
   if (err == kVfsErr_OK)
      file->pos += *outRead;

   return err;
}

VfsError vfsFileReadAt(File* file, u32 offset, void* dest, u32 len, u32* outRead)
{
   if (_backend == nil)
      return kVfsErr_Uninitialized;
   if (file == nil || dest == nil || outRead == nil)
      return kVfsErr_BadArgument;
      
   *outRead = 0;
   if (offset >= file->size || len == 0)
      return kVfsErr_OK;

   return _backend->readFile(file, offset, min(len, file->size - offset), dest, outRead);
}

VfsError vfsFileSeek(File* file, u32 pos)
{
   if (file == nil)
      return kVfsErr_BadArgument;
 
   file->pos = min(pos, file->size);
   return kVfsErr_OK;

}

VfsError vfsFileClose(File* file)
{
   if (_backend == nil)
      return kVfsErr_Uninitialized;
   if (file == nil)
      return kVfsErr_BadArgument;
 
   VfsError err = _backend->closeFile(file);
   file->size = 0;
   file->pos  = 0;
   return err;
}
