/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

/*

NTFS Volume looks like this:

[ $Boot | MFT | Free Space | More Meta data | Free Space ]

The MFT lists the boot sector file $Boot.
$Boot lists where to find the MFT.
MFT also lists itself.

$MFTMirr is a copy of the first four records of the MFT
$LogFile is a journal of all events waiting to be written to disk.

At the end of the volume is a copy of the boot sector (cluster 0).
$Bitmap metadata file references this copy and only says that the cluster is in use.

All files can be relocated except for $Boot.

From: http://technet.microsoft.com/en-us/library/cc781134(WS.10).aspx
http://support.microsoft.com/kb/174619

 http://blogs.technet.com/b/joscon/archive/2011/01/06/how-hard-links-work.aspx

http://blogs.technet.com/b/askcore/archive/2009/12/30/ntfs-metafiles.aspx
http://blogs.technet.com/b/askcore/archive/2010/08/25/ntfs-file-attributes.aspx
http://blogs.technet.com/b/askcore/archive/2010/02/18/understanding-the-2-tb-limit-in-windows-storage.aspx
http://blogs.technet.com/b/askcore/archive/2009/10/16/the-four-stages-of-ntfs-file-growth.aspx
http://blogs.technet.com/b/askcore/archive/2010/10/08/gpt-in-windows.aspx
http://blogs.technet.com/b/askperf/archive/2010/12/03/performance-counter-for-iscsi.aspx
http://msdn.microsoft.com/en-us/library/bb470039(v=vs.85).aspx

*/

#include <windows.h>
#include <stdio.h>

#include "ntfs.h"

#include <vector>
#include <string>
#include <map>

NTFS * init(BOOT_SECTOR * bootsector)
{
	NTFS * ntfs = NULL;
	const unsigned long ntfssignature = 'SFTN';

	if (bootsector)
	{

		if (((ULONG)(bootsector->OEMIdentifier & 0xffffffff)) == ntfssignature)
		{
			ntfs = (NTFS*)malloc(sizeof(NTFS));
			if (NULL != ntfs)
			{
				ntfs->sectorSizeInBytes = bootsector->BPB.BytesPerSector;
				ntfs->clusterSizeInSectors = bootsector->BPB.SectorsPerCluster;
				ntfs->mftLocation.QuadPart = (long long)bootsector->BPB.BytesPerSector * (long long)bootsector->BPB.SectorsPerCluster * bootsector->ExtendedBPB.LogicalClusterNumberForMFT;
			}
		}
	}

	return ntfs;
}

void deinit(NTFS * ntfs)
{
	free(ntfs);
}

