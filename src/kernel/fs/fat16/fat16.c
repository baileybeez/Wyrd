#include "wyrd.h"
#include "fat16.h"
#include "lib/mem.h"

#define kExtSep   '.'
#define kPathSep  '/'

#define kFat16_BootSignature   0xAA55
#define kFat16_BytesPerSector  512
#define kFat16_DirEntrySize    32
#define kFat16_NameLen         11
#define kFat16_BaseLen         8
#define kFat16_ExtLen          3
#define kFat16_MaxComponentLen 13
#define kFat16_MaxFatSectors   128
#define kFat16_EocMin          0xFFF8
#define kFat16_BadCluster      0xFFF7
#define kFat16_AttrVolumeId    0x08
#define kFat16_AttrLfnMask     0x0F
#define kFat16_AttrDirectory   0x10
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

typedef struct {
   bool isRoot;
   u16  startCluster;
} Fat16DirRef;

static bool fat16IsEoc(u16 entry)
{
   return entry >= kFat16_EocMin;
}

static bool fat16IsBad(u16 entry)
{
   return entry < 2 || entry == kFat16_BadCluster;
}
 
static u32 fat16ClusterToLba(const Fat16Volume* vol, u16 cluster)
{
   return vol->dataStartLba + ((u32)(cluster - 2) * vol->sectorsPerCluster);
}

static u8 fat16ToUpper(u8 c)
{
   if (c >= 'a' && c <= 'z')
      return c - ('a' - 'A');
   return c;
}

static Fat16Error fat16NameTo8_3(const char* name, u8 out[kFat16_NameLen])
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
      out[baseLen++] = fat16ToUpper((u8)name[i++]);
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
      out[kFat16_BaseLen + extLen++] = fat16ToUpper((u8)name[i++]);
   }

   return kFatErr_OK;
}

static bool fat16DirEntryMatches(const Fat16DirEntry* entry, const u8 name[kFat16_NameLen])
{
   for (u32 i = 0; i < kFat16_NameLen; i++) {
      if (entry->name[i] != (u8)name[i])
         return false;
   }
   return true;
}

static bool fat16EntryIsTerminal(const Fat16DirEntry* e)
{
   return e->name[0] == kFat16_DirEntryEnd;
}

static bool fat16EntryIsSkippable(const Fat16DirEntry* entry)
{
   if (entry->name[0] == kFat16_DirEntryFree)
      return true;
   if ((entry->attr & kFat16_AttrLfnMask) == kFat16_AttrLfnMask)
      return true;
   if (entry->attr & kFat16_AttrVolumeId)
      return true;

   return false;
}

static Fat16Error fat16ScanDirEntries(const Fat16DirEntry* entries, u32 count, const u8 name8_3[kFat16_NameLen], Fat16DirEntry *out)
{
   for (u32 i = 0; i < count; i++) {
      const Fat16DirEntry* e = &entries[i];
      if (fat16EntryIsTerminal(e))
         return kFatErr_FileNotFound;
      if (fat16EntryIsSkippable(e))
         continue;
      if (fat16DirEntryMatches(e, name8_3)) {
         *out = *e;
         return kFatErr_OK;
      }
   }

   return kFatErr_Continue;
}

Fat16Error fat16Mount(Fat16Volume* vol, kFat16ReadSectorsFn fncReadSectors, 
                      void* fatBuffer, u32 fatBufferSize, 
                      void* rootDirBuffer, u32 rootDirBufferSize)
{
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
   
   u32 fatBytes      = (u32)bpb->fatSize16 * bpb->bytesPerSector;
   u32 rootDirBytes  = (u32)bpb->rootEntCount * kFat16_DirEntrySize;
   if (fatBytes > fatBufferSize)
      return kFatErr_FatOverflow;
   if (rootDirBytes > rootDirBufferSize)
      return kFatErr_RootOverflow;

   vol->readSectors        = fncReadSectors;
   vol->bytesPerSector     = bpb->bytesPerSector;
   vol->sectorsPerCluster  = bpb->sectorsPerCluster;
   vol->fatSize16          = bpb->fatSize16;
   vol->rootEntCount       = bpb->rootEntCount;
   vol->fatStartLba        = bpb->reservedSectorCount;
   vol->rootDirStartLba    = vol->fatStartLba + (u32)bpb->numFats * bpb->fatSize16;

   u32 rootDirSectors   = (rootDirBytes + bpb->bytesPerSector - 1) / bpb->bytesPerSector;
   vol->dataStartLba    = vol->rootDirStartLba + rootDirSectors;
   vol->bytesPerCluster = (u32)bpb->sectorsPerCluster * bpb->bytesPerSector;

   vol->fat     = (u16*)fatBuffer;
   vol->rootDir = (u8*)rootDirBuffer;

   if (!fncReadSectors(vol->fatStartLba, vol->fatSize16, vol->fat))
      return kFatErr_DiskRead;
   if (!fncReadSectors(vol->rootDirStartLba, rootDirSectors, vol->rootDir))
      return kFatErr_DiskRead;

   return kFatErr_OK;
}

Fat16Error fat16FindInDir(const Fat16Volume* vol, Fat16DirRef dir, u8 name8_3[kFat16_NameLen], Fat16DirEntry* out)
{
   if (dir.isRoot) {
      return fat16ScanDirEntries((const Fat16DirEntry*)vol->rootDir, vol->rootEntCount, name8_3, out);
   }

   u8  scratch[kFat16_BytesPerSector];
   u32 entriesPerSector = vol->bytesPerSector / kFat16_DirEntrySize;
   u32 maxHops          = ((u32)vol->fatSize16 * vol->bytesPerSector) / 2;

   u16 cluster = dir.startCluster;
   u32 hops    = 0;
   while (!fat16IsEoc(cluster)) {
      if (fat16IsBad(cluster))
         return kFatErr_BadCluster;

      u32 baseLba = fat16ClusterToLba(vol, cluster);
      for (u32 s = 0; s < vol->sectorsPerCluster; s++) {
         if (!vol->readSectors(baseLba + s, 1, scratch))
            return kFatErr_DiskRead;

         Fat16Error err = fat16ScanDirEntries((const Fat16DirEntry*)scratch, entriesPerSector, name8_3, out);
         if (err != kFatErr_Continue)
            return err;
      }

      if (++hops > maxHops)
         return kFatErr_BadCluster;
         
      cluster = vol->fat[cluster];
   }

   return kFatErr_FileNotFound;
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

Fat16Error fat16ReadFile(const Fat16Volume* vol, u16 firstCluster, u32 fileSize, void* dest)
{
   if (fileSize == 0)
      return kFatErr_OK;
   if (fat16IsBad(firstCluster))
      return kFatErr_BadCluster;

   u8 scratch[kFat16_BytesPerSector];
   u8* out       = (u8*)dest;
   u16 cluster   = firstCluster;
   u32 remaining = fileSize;
   while (remaining > 0) {
      if (fat16IsBad(cluster))
         return kFatErr_BadCluster;

      u32 baseLba       = fat16ClusterToLba(vol, cluster);
      u32 clusterBytes  = min(remaining, vol->bytesPerCluster);
      u32 fullSectors   = clusterBytes / vol->bytesPerSector;
      u32 tail          = clusterBytes % vol->bytesPerSector;

      if (fullSectors > 0) {
         if (!vol->readSectors(baseLba, fullSectors, out))
            return kFatErr_DiskRead;   

         out += fullSectors * vol->bytesPerSector;
      }

      if (tail > 0) {
         if (!vol->readSectors(baseLba + fullSectors, 1, scratch))
            return kFatErr_DiskRead;
         memcpy(out, scratch, tail);
         out += tail;
      }
      
      remaining -= clusterBytes;

      u16 next = vol->fat[cluster];
      if (fat16IsEoc(next)) {
         if (remaining > 0)
            return kFatErr_ShortRead;

         return kFatErr_OK;
      }

      cluster = next;
   }

   return kFatErr_OK;
}

Fat16Error fat16ReadFileRange(const Fat16Volume* vol, u16 firstCluster, u32 fileSize, u32 offset, u32 len, void* dest)
{
   if (offset >= fileSize || len == 0)
      return kFatErr_OK;
   if (len > fileSize - offset)
      len = fileSize - offset;
   if (fat16IsBad(firstCluster))
      return kFatErr_BadCluster;

   u16 cluster = firstCluster;
   u32 skip    = offset / vol->bytesPerCluster;
   for (u32 h = 0; h < skip; h++) {
      u16 next = vol->fat[cluster];
      if (fat16IsEoc(next) || fat16IsBad(next))
         return kFatErr_ShortRead;

      cluster = next;
   }

   u8  scratch[kFat16_BytesPerSector];
   u8* out       = (u8*)dest;
   u32 pos       = offset;
   u32 remaining = len;

   while (remaining > 0) {
      if (fat16IsBad(cluster))
         return kFatErr_BadCluster;

      u32 sectorInClus = (pos % vol->bytesPerCluster) / vol->bytesPerSector;
      u32 offInSector  = pos % vol->bytesPerSector;
      u32 lba          = fat16ClusterToLba(vol, cluster) + sectorInClus;
      u32 chunk        = min(remaining, vol->bytesPerSector - offInSector);

      if (offInSector == 0 && chunk == vol->bytesPerSector) {
         if (!vol->readSectors(lba, 1, out))
            return kFatErr_DiskRead;
      } else {
         if (!vol->readSectors(lba, 1, scratch))
            return kFatErr_DiskRead;
         memcpy(out, scratch + offInSector, chunk);
      }

      out       += chunk;
      pos       += chunk;
      remaining -= chunk;

      if (remaining > 0 && (pos % vol->bytesPerCluster) == 0) {
         u16 next = vol->fat[cluster];
         if (fat16IsEoc(next))
            return kFatErr_ShortRead;
         cluster = next;
      }
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

   u32 toRead = min(size, (u32)(kFat16_SelfTestBufSize - 1));
   err = fat16ReadFileRange(vol, cluster, size, 0, toRead, buffer);
   if (err != kFatErr_OK) {
      serialPrintf("[FAT16] selftest: FAIL - fat16ReadFileRange returned %x\n", err);
      return;
   }

   buffer[toRead] = '\0';
   serialPrintf("[FAT16] contents (%x bytes):\n%s\n", toRead, buffer);
   serialPrintf("[FAT16] selftest: PASS\n");
}
#endif
