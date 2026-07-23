#include "bee.h"
#include "fat16.h"

#define kExtSep '.'

#define kFat16BootSignature   0xAA55
#define kFat16BytesPerSector  512
#define kFat16DirEntrySize    32
#define kFat16NameLen         11
#define kFat16BaseLen         8
#define kFat16ExtLen          3
#define kFat16MaxFatSectors   128
#define kFat16EocMin          0xFFF8
#define kFat16BadCluster      0xFFF7
#define kFat16AttrVolumeId    0x08
#define kFat16AttrLfnMask     0x0F
#define kFat16DirEntryFree    0xE5
#define kFat16DirEntryEnd     0x00

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
   u16 fstClusHi;
   u16 wrtTime;
   u16 wrtDate;
   u16 fstClusLo;
   u32 fileSize;
} __attribute__((packed)) Fat16DirEntry;

static bool fat16IsEoc(u16 entry)
{
   return entry >= kFat16EocMin;
}

static bool fat16IsBad(u16 entry)
{
   return entry < 2 || entry == kFat16BadCluster;
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

static Fat16Error fat16NameTo8_3(const char* name, u8 out[kFat16NameLen])
{
   for (u32 i = 0; i < kFat16NameLen; i++)
      out[i] = ' ';

   if (name == nil || name[0] == '\0' || name[0] == kExtSep)
      return kFAT16_BadName;

   u32 i = 0;
   u32 baseLen = 0;
   while (name[i] != '\0' && name[i] != kExtSep) {
      if (baseLen >= kFat16NameLen)
         return kFAT16_BadName;
      out[baseLen++] = fat16ToUpper((u8)name[i++]);
   }

   if (name[i] == '\0')
      return kFAT16_OK;

   ++i;
   u32 extLen = 0;
   while (name[i] != '\0') {
      if (name[i] == kExtSep)
         return kFAT16_BadName;
      if (extLen >= kFat16ExtLen)
         return kFAT16_BadName;
      out[kFat16BaseLen + extLen++] = fat16ToUpper((u8)name[i++]);
   }

   return kFAT16_OK;
}

static bool fat16DirEntryMatches(const Fat16DirEntry* entry, const u8 name[kFat16NameLen])
{
   for (u32 i = 0; i < kFat16NameLen; i++) {
      if (entry->name[i] != (u8)name[i])
         return false;
   }
   return true;
}

Fat16Error fat16Mount(Fat16Volume* vol, kFat16ReadSectorsFn fncReadSectors, 
                      void* fatBuffer, u32 fatBufferSize, 
                      void* rootDirBuffer, u32 rootDirBufferSize)
{
   u8 sector[kFat16BytesPerSector];
   if (!fncReadSectors(0, 1, sector))
      return kFAT16_DiskRead;

   u16 sig = sector[510] | ((u16)sector[511] << 8);
   if (sig != kFat16BootSignature)
      return kFAT16_BadBPB;

   const Fat16BPB* bpb = (const Fat16BPB*)sector;
   if (bpb->bytesPerSector != kFat16BytesPerSector)
      return kFAT16_BadBPB;
   if (bpb->sectorsPerCluster == 0)
      return kFAT16_BadBPB;
   if (bpb->numFats == 0)
      return kFAT16_BadBPB;
   if (bpb->rootEntCount == 0)
      return kFAT16_BadBPB;
   if (bpb->fatSize16 == 0)
      return kFAT16_BadBPB;
   
   u32 fatBytes      = (u32)bpb->fatSize16 * bpb->bytesPerSector;
   u32 rootDirBytes  = (u32)bpb->rootEntCount * kFat16DirEntrySize;
   if (fatBytes > fatBufferSize)
      return kFAT16_FatOverflow;
   if (rootDirBytes > rootDirBufferSize)
      return kFAT16_RootOverflow;

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

   if (!fncReadSectors(vol->fatStartLba, (u8)vol->fatSize16, vol->fat))
      return kFAT16_DiskRead;
   if (!fncReadSectors(vol->rootDirStartLba, (u8)rootDirSectors, vol->rootDir))
      return kFAT16_DiskRead;

   return kFAT16_OK;
}

Fat16Error fat16FindFile(const Fat16Volume* vol, const char* path, u16* outFirstCluster, u32* outFileSize)
{
   u8 name8_3[kFat16NameLen];
   Fat16Error err = fat16NameTo8_3(path, name8_3);
   if (err != kFAT16_OK)
      return err;

   const Fat16DirEntry* entries = (const Fat16DirEntry*)vol->rootDir;
   for (u32 i = 0; i < vol->rootEntCount; i++) {
      const Fat16DirEntry* e = &entries[i];

      if (e->name[0] == kFat16DirEntryEnd)
         break;
      if (e->name[0] == kFat16DirEntryFree)
         continue;
      if ((e->attr & kFat16AttrLfnMask) == kFat16AttrLfnMask)
         continue;
      if (e->attr & kFat16AttrVolumeId)
         continue;

      if (fat16DirEntryMatches(e, name8_3)) {
         *outFirstCluster = e->fstClusLo;
         *outFileSize     = e->fileSize;
         return kFAT16_OK;
      }
   }

   return kFAT16_FileNotFound;
}

Fat16Error fat16ReadFile(const Fat16Volume* vol, u16 firstCluster, u32 fileSize, void* dest)
{
   if (fileSize == 0)
      return kFAT16_OK;
   if (fat16IsBad(firstCluster))
      return kFAT16_BadCluster;

   u8* out       = (u8*)dest;
   u16 cluster   = firstCluster;
   u32 remaining = fileSize;
   while (remaining > 0) {
      if (fat16IsBad(cluster))
         return kFat16BadCluster;

      u32 lba = fat16ClusterToLba(vol, cluster);
      if (!vol->readSectors(lba, vol->sectorsPerCluster, out))
         return kFAT16_DiskRead;

      u32 chunk = min(remaining, vol->bytesPerCluster);
      out       += chunk;
      remaining -= chunk;

      u16 next = vol->fat[cluster];
      if (fat16IsEoc(next)) {
         if (remaining > 0)
            return kFAT16_ShortRead;

         return kFAT16_OK;
      }

      cluster = next;
   }

   return kFAT16_OK;
}