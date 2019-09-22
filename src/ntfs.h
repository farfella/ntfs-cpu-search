#pragma once

#include <map>
#include "bootsector.h"


#include <windows.h>

struct NTFS
{
	unsigned long sectorSizeInBytes;
	unsigned long clusterSizeInSectors;

	LARGE_INTEGER mftLocation;
};

NTFS * init(BOOT_SECTOR *);
void deinit(NTFS *);
