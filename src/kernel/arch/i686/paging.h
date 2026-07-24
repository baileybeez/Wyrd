#pragma once
#include "wyrd.h"

#define kInvalidPhysical 0xFFFFFFFF

#define kPageSize 4096

#define kPageFlag_Present  0x01
#define kPageFlag_Writable 0x02
#define kPageFlag_User     0x04
//      kPageFlag_PS       0x80

void pagingInit();
bool pagingMapPage(u32 virtualAddr, u32 physicalAddr, u32 flags);
bool pagingUnmapPage(u32 virtualAddr);
u32  pagingGetPhysical(u32 virtualAddr);
void pagingInvalidatePage(u32 virtualAddr);
