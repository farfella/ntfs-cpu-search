/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "bootsector.h"
#include "reader.h"
#include <string.h>
#include <malloc.h>

BOOT_SECTOR * bootsector::read(reader * volumereader)
{
	const LARGE_INTEGER location = {0};

	ULONG size = 512;	// in almost all cases, sector size is 512

	BOOT_SECTOR * b = (BOOT_SECTOR *)malloc( size );

	if (b)
	{
		ZeroMemory(b, size);

		if (volumereader->read(b, size, location) == 0)
		{
			free(b);
			b = NULL;
		}
		
		if (b && b->BPB.BytesPerSector > size)	// boot sector says size > 512, so adjust
		{
			size = b->BPB.BytesPerSector;
			free(b);
			b = (BOOT_SECTOR *)malloc( size );
			if (b)
			{
				if (volumereader->read(b, size, location) == 0)
				{
					free(b);
					b = NULL;
				}
			}
		}

	}

	return b;
}
