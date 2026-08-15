#include "wyrd.h"
#include "fat16.h"
#include "lib/mem.h"

#define kExtSep   '.'
#define kPathSep  '/'

#define kFat16_BootSignature   0xAA55
#define kFat16_MaxFatSectors   128
#define kFat16_EocMin          0xFFF8
#define kFat16_BadCluster      0xFFF7
#define kFat16_DirEntryFree    0xE5
#define kFat16_DirEntryEnd     0x00

typedef struct {
   u8  jmpBoot[3];
   u8  oemName[8];
   u16 bytesPerSector;
   u8  sectorsPerCluster;
   u16 reservedSectorCount;
   u8  numFats;
   u16 rootEntCount;
   u16 totSec16;
   u8  media;
   u16 fatSize16;
   u16 sectorsPerTrack;
   u16 numHeads;
   u32 hiddenSectors;
   u32 totSec32;
} __attribute__((packed)) Fat16BPB;
_Static_assert(sizeof(Fat16BPB) == 36, "BPB struct is invalid size");

static u16 _fat16FatEntry(const Fat16Volume* vol, u16 cluster)
{
   u32 count = ((u32)vol->fatSize16 * vol->bytesPerSector) / 2;
   if (cluster >= count)
      return kFat16_BadCluster;

   return vol->fat[cluster];
}

static bool _fat16IsEoc(u16 entry)
{
   return entry >= kFat16_EocMin;
}

static bool _fat16IsBadCluster(const Fat16Volume* vol, u16 cluster)
{
   return cluster < 2 || cluster == kFat16_BadCluster || cluster >= vol->clusterCount;
}
 
static u32 _fat16ClusterToLba(const Fat16Volume* vol, u16 cluster)
{
   return vol->dataStartLba + ((u32)(cluster - 2) * vol->sectorsPerCluster);
}

static u8 _fat16ToUpper(u8 c)
{
   if (c >= 'a' && c <= 'z')
      return c - ('a' - 'A');
   return c;
}

static u8 _fat16ToLower(u8 c)
{
   if (c >= 'A' && c <= 'Z')
      return c + ('a' - 'A');
   return c;
}

static bool _fat16DirEntryMatches(const Fat16DirEntry* entry, const u8 name[kFat16_NameLen])
{
   for (u32 i = 0; i < kFat16_NameLen; i++) {
      if (entry->name[i] != name[i])
         return false;
   }
   return true;
}

static bool _fat16EntryIsTerminal(const Fat16DirEntry* e)
{
   return e->name[0] == kFat16_DirEntryEnd;
}

static bool _fat16EntryIsSkippable(const Fat16DirEntry* entry)
{
   if (entry->name[0] == kFat16_DirEntryFree)
      return true;
   if ((entry->attr & kFat16_AttrLfnMask) == kFat16_AttrLfnMask)
      return true;
   if (entry->attr & kFat16_AttrVolumeId)
      return true;

   return false;
}

static Fat16Error _fat16ReadDirSector(const Fat16Volume* vol, u16 cluster, u16 offset, u8* dst)
{
   if (_fat16IsEoc(cluster))
      return kFatErr_BadCluster;
   
   if (_fat16IsBadCluster(vol, cluster))
      return kFatErr_BadCluster;

   u32 lba = _fat16ClusterToLba(vol, cluster);
   if (!vol->readSectors(lba + offset, 1, dst))
      return kFatErr_DiskRead;
   
   return kFatErr_OK;
}

static Fat16Error _fat16SkipToNextValidEntry(const Fat16Volume* vol, Fat16DirIterator* iter)
{
   const Fat16DirEntry* entry = fat16DirIterEntry(iter);
   while (_fat16EntryIsSkippable(entry)) {
      Fat16Error err = fat16DirIterNext(vol, iter);
      if (err != kFatErr_OK)
         return err;

      entry = fat16DirIterEntry(iter);
   }
   
   if (_fat16EntryIsTerminal(entry))
      return kFatErr_EndOfDir;

   return kFatErr_OK;
}

Fat16Error fat16Mount(Fat16Volume* vol, Fat16ReadSectorsFn fncReadSectors, 
                      void* fatBuffer, u32 fatBufferSize, 
                      void* rootDirBuffer, u32 rootDirBufferSize)
{
   vol->mounted = false;

   u8 sector[kFat16_BytesPerSector];
   if (!fncReadSectors(0, 1, sector))
      return kFatErr_DiskRead;

   u16 sig = sector[510] | ((u16)sector[511] << 8);
   if (sig != kFat16_BootSignature)
      return kFatErr_BadBPB;

   const Fat16BPB* bpb = (const Fat16BPB*)sector;
   if (bpb->bytesPerSector != kFat16_BytesPerSector)
      return kFatErr_BadBPB;
   if (bpb->sectorsPerCluster == 0)
      return kFatErr_BadBPB;
   if (bpb->numFats == 0)
      return kFatErr_BadBPB;
   if (bpb->rootEntCount == 0)
      return kFatErr_BadBPB;
   if (bpb->fatSize16 == 0)
      return kFatErr_BadBPB;
   if (bpb->jmpBoot[0] != 0xEB && bpb->jmpBoot[0] != 0xE9)
      return kFatErr_BadBPB;
   if (bpb->media < 0xF0)
      return kFatErr_BadBPB;
   
   u32 fatBytes       = (u32)bpb->fatSize16 * bpb->bytesPerSector;
   u32 rootDirBytes   = (u32)bpb->rootEntCount * kFat16_DirEntrySize;
   u32 rootDirSectors = (rootDirBytes + bpb->bytesPerSector - 1) / bpb->bytesPerSector;
   if (fatBytes > fatBufferSize)
      return kFatErr_FatOverflow;
   if (rootDirSectors * bpb->bytesPerSector > rootDirBufferSize)
      return kFatErr_RootOverflow;
      
   vol->readSectors        = fncReadSectors;
   vol->bytesPerSector     = bpb->bytesPerSector;
   vol->sectorsPerCluster  = bpb->sectorsPerCluster;
   vol->fatSize16          = bpb->fatSize16;
   vol->rootEntCount       = bpb->rootEntCount;
   vol->fatStartLba        = bpb->reservedSectorCount;
   vol->rootDirStartLba    = vol->fatStartLba + (u32)bpb->numFats * bpb->fatSize16;

   vol->dataStartLba    = vol->rootDirStartLba + rootDirSectors;
   vol->bytesPerCluster = (u32)bpb->sectorsPerCluster * bpb->bytesPerSector;

   u32 totSec = bpb->totSec16 ? bpb->totSec16 : bpb->totSec32;
   if (totSec == 0)
      return kFatErr_BadBPB;
   if (totSec <= vol->dataStartLba)
      return kFatErr_BadBPB;
   vol->clusterCount    = (totSec - vol->dataStartLba) / vol->sectorsPerCluster + 2;

   vol->fat     = (u16*)fatBuffer;
   vol->rootDir = (u8*)rootDirBuffer;

   if (!fncReadSectors(vol->fatStartLba, vol->fatSize16, vol->fat))
      return kFatErr_DiskRead;
   if (!fncReadSectors(vol->rootDirStartLba, rootDirSectors, vol->rootDir))
      return kFatErr_DiskRead;

   vol->mounted = true;
   return kFatErr_OK;
}

Fat16Error fat16DirIterInit(const Fat16Volume* vol, Fat16DirRef dir, u8* scratch, Fat16DirIterator* out)
{
   out->isRoot = dir.isRoot;
   out->entryIndex     = 0;
   out->hopCount       = 0;
   out->clusterOffset  = 0;
   out->scratch        = scratch;
   if (out->isRoot) {
      out->base           = (const Fat16DirEntry*)vol->rootDir;
      out->count          = vol->rootEntCount;
      out->maxHops        = 0;
      out->cluster        = 0;
   } else {
      if (_fat16IsEoc(dir.startCluster))
         return kFatErr_EndOfDir;
      
      out->cluster = dir.startCluster;
      out->count   = vol->bytesPerSector / kFat16_DirEntrySize;
      out->maxHops = vol->clusterCount;
      out->base    = (const Fat16DirEntry*)out->scratch;
      
      Fat16Error err = _fat16ReadDirSector(vol, dir.startCluster, 0, out->scratch);
      if (err != kFatErr_OK)
         return err;
   }
   
   return _fat16SkipToNextValidEntry(vol, out);
}

Fat16Error fat16DirIterNext(const Fat16Volume* vol, Fat16DirIterator* iter)
{
   if (++iter->entryIndex >= iter->count) {
      if (iter->isRoot)
         return kFatErr_EndOfDir;

      iter->entryIndex = 0;
      if (++iter->clusterOffset >= vol->sectorsPerCluster) {
         if (++iter->hopCount > iter->maxHops)
            return kFatErr_BadCluster;

         u16 next = _fat16FatEntry(vol, iter->cluster);
         if (_fat16IsEoc(next))
            return kFatErr_EndOfDir;
         if (_fat16IsBadCluster(vol, next))
            return kFatErr_BadCluster;

         iter->cluster       = next;
         iter->clusterOffset = 0;
      }

      Fat16Error err = _fat16ReadDirSector(vol, iter->cluster, iter->clusterOffset, iter->scratch);
      if (err != kFatErr_OK)
         return err;
   }

   return _fat16SkipToNextValidEntry(vol, iter);
}

const Fat16DirEntry* fat16DirIterEntry(const Fat16DirIterator* iter)
{
   if(iter->entryIndex >= iter->count)
      return nil;

   return &iter->base[iter->entryIndex];
}

Fat16Error fat16FindInDir(const Fat16Volume* vol, Fat16DirRef dir, const u8 name8_3[kFat16_NameLen], Fat16DirEntry* out)
{
   u8 scratch[kFat16_BytesPerSector] __attribute__((aligned(8)));

   Fat16DirIterator iter;
   Fat16Error err = fat16DirIterInit(vol, dir, scratch, &iter);
   if (err == kFatErr_OK) {
      const Fat16DirEntry* entry;
      while (err == kFatErr_OK) {
         entry = fat16DirIterEntry(&iter);
         if (entry == nil)
            return kFatErr_EndOfDir;

         if (!_fat16EntryIsSkippable(entry) && _fat16DirEntryMatches(entry, name8_3)) {
            *out = *(entry);
            return kFatErr_OK;
         }

         err = fat16DirIterNext(vol, &iter);      
      }
   }

   if (err == kFatErr_EndOfDir)
      return kFatErr_FileNotFound;

   return err;
}

// parse filepath, split on kPathSep ('/') ... look for dirs, then file
Fat16Error fat16FindFile(const Fat16Volume* vol, const char* path, u16* outFirstCluster, u32* outFileSize)
{
   if (path == nil)
      return kFatErr_BadName;

   const char* p = path;
   if (*p == kPathSep)
      p++;

   Fat16DirRef dir = { .isRoot = true, .startCluster = 0 };
   Fat16DirEntry entry;
   bool haveEntry = false;

   while (*p != '\0') {
      char comp[kFat16_MaxComponentLen];
      u32 n = 0;
      while (*p != '\0' && *p != kPathSep) {
         if (n + 1 >= kFat16_MaxComponentLen)
            return kFatErr_BadName;

         comp[n++] = *p++;
      }
      comp[n] = '\0';
      if (n == 0)
         return kFatErr_BadName;

      if (*p == kPathSep)
         p++;

      u8 name8_3[kFat16_NameLen];
      Fat16Error err = fat16NameTo8_3(comp, name8_3);
      if (err != kFatErr_OK)
         return err;

      err = fat16FindInDir(vol, dir, name8_3, &entry);
      if (err != kFatErr_OK)
         return err;

      haveEntry = true;
      if (*p != '\0') {
         if (!(entry.attr & kFat16_AttrDirectory))
            return kFatErr_FileNotFound;

         dir.isRoot = false;
         dir.startCluster = entry.firstClusterLow;
      }
   }

   if (!haveEntry)
      return kFatErr_FileNotFound;

   *outFirstCluster = entry.firstClusterLow;
   *outFileSize     = entry.fileSize;
   return kFatErr_OK;
}

Fat16Error fat16ReadFileRange(const Fat16Volume* vol, u16 firstCluster, u32 fileSize, u32 offset, u32 len, void* dest, u32* outBytesRead)
{
   *outBytesRead = 0;
   if (offset >= fileSize || len == 0)
      return kFatErr_OK;
   if (len > fileSize - offset)
      len = fileSize - offset;
   if (_fat16IsBadCluster(vol, firstCluster))
      return kFatErr_BadCluster;

   u16 cluster = firstCluster;
   u32 skip    = offset / vol->bytesPerCluster;
   for (u32 h = 0; h < skip; h++) {
      u16 next = _fat16FatEntry(vol, cluster);
      if (_fat16IsEoc(next))
         return kFatErr_ShortRead;
      if (_fat16IsBadCluster(vol, next))
         return kFatErr_BadCluster;

      cluster = next;
   }

   u8  scratch[kFat16_BytesPerSector];
   u8* out       = (u8*)dest;
   u32 pos       = offset;
   u32 remaining = len;

   while (remaining > 0) {
      if (_fat16IsBadCluster(vol, cluster))
         return kFatErr_BadCluster;

      u32 posInCluster = pos % vol->bytesPerCluster;
      u32 clusterBytes = min(remaining, vol->bytesPerCluster - posInCluster);
      u32 lba          = _fat16ClusterToLba(vol, cluster) + (posInCluster / vol->bytesPerSector);
      u32 offInSector  = posInCluster % vol->bytesPerSector;
      u32 done         = 0;

      if (offInSector > 0) {
         u32 head = min(clusterBytes, vol->bytesPerSector - offInSector);
         if (!vol->readSectors(lba, 1, scratch))
            return kFatErr_DiskRead;

         memcpy(out, scratch + offInSector, head);
         done          += head;
         *outBytesRead += head;
         lba++;
      }

      u32 fullSectors = (clusterBytes - done) / vol->bytesPerSector;
      if (fullSectors > 0) {
         if (!vol->readSectors(lba, fullSectors, out + done))
            return kFatErr_DiskRead;

         done          += fullSectors * vol->bytesPerSector;
         lba           += fullSectors;
         *outBytesRead += fullSectors * vol->bytesPerSector;
      }

      u32 tail = clusterBytes - done;
      if (tail > 0) {
         if (!vol->readSectors(lba, 1, scratch))
            return kFatErr_DiskRead;

         memcpy(out + done, scratch, tail);
         done          += tail;
         *outBytesRead += tail;
      }

      out       += done;
      pos       += done;
      remaining -= done;

      if (remaining > 0 && (pos % vol->bytesPerCluster) == 0) {
         u16 next = _fat16FatEntry(vol, cluster);
         if (_fat16IsEoc(next))
            return kFatErr_ShortRead;
         if (_fat16IsBadCluster(vol, next))
            return kFatErr_BadCluster;
         cluster = next;
      }
   }

   return kFatErr_OK;
}

Fat16Error fat168_3ToName(u8 str8_3[kFat16_NameLen], char outName[kFat16_MaxComponentLen])
{
   if (str8_3[0] == ' ')
      return kFatErr_BadName;

   u32 idx = 0;
   for (u32 i = 0; i < kFat16_BaseLen && str8_3[i] != ' '; i++) {
      outName[idx++] = _fat16ToLower(str8_3[i]);
   }

   char c = str8_3[kFat16_BaseLen];
   if (c != '\0' && c != ' ')
      outName[idx++] = '.';

   for (u32 i = kFat16_BaseLen; i < kFat16_NameLen && str8_3[i] != ' '; i++) {
      outName[idx++] = _fat16ToLower(str8_3[i]);
   }
   outName[idx] = '\0';
   return kFatErr_OK;
}

Fat16Error fat16NameTo8_3(const char* name, u8 out[kFat16_NameLen])
{
   for (u32 i = 0; i < kFat16_NameLen; i++)
      out[i] = ' ';

   if (name == nil || name[0] == '\0' || name[0] == kExtSep)
      return kFatErr_BadName;

   u32 i = 0;
   u32 baseLen = 0;
   while (name[i] != '\0' && name[i] != kExtSep) {
      if (baseLen >= kFat16_BaseLen)
         return kFatErr_BadName;
      out[baseLen++] = _fat16ToUpper((u8)name[i++]);
   }

   if (name[i] == '\0')
      return kFatErr_OK;

   ++i;
   u32 extLen = 0;
   while (name[i] != '\0') {
      if (name[i] == kExtSep)
         return kFatErr_BadName;
      if (extLen >= kFat16_ExtLen)
         return kFatErr_BadName;
      out[kFat16_BaseLen + extLen++] = _fat16ToUpper((u8)name[i++]);
   }

   return kFatErr_OK;
}

#ifdef kIncludeSelfTests
#include "drivers/serial/serial.h"

#define kFat16_SelfTestPath    "/tmp/file.txt"
#define kFat16_SelfTestBufSize 512

void fat16SelfTest(const Fat16Volume* vol)
{
   static u8 buffer[kFat16_SelfTestBufSize];

   serialPrintf("[FAT16] selftest: resolving %s...\n", kFat16_SelfTestPath);

   u16 cluster;
   u32 size;
   Fat16Error err = fat16FindFile(vol, kFat16_SelfTestPath, &cluster, &size);
   if (err != kFatErr_OK) {
      serialPrintf("[FAT16] selftest: FAIL - fat16FindFile returned %x\n", err);
      return;
   }

   serialPrintf("[FAT16] found: firstCluster=%x size=%x bytes\n", cluster, size);

   u32 toRead    = min(size, (u32)(kFat16_SelfTestBufSize - 1));
   u32 bytesRead = 0;
   err = fat16ReadFileRange(vol, cluster, size, 0, toRead, buffer, &bytesRead);
   if (err != kFatErr_OK) {
      serialPrintf("[FAT16] selftest: FAIL - fat16ReadFileRange returned %x\n", err);
      return;
   }

   buffer[toRead] = '\0';
   serialPrintf("[FAT16] contents (%x bytes):\n%s\n", toRead, buffer);
   serialPrintf("[FAT16] selftest: PASS\n");
}
#endif
