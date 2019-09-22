#pragma once

#pragma pack(push, 1)

struct BIOS_PARAMETER_BLOCK
{
	unsigned short			BytesPerSector;
	unsigned char			SectorsPerCluster;
	unsigned short			ReservedSectors;	// always 0, must be 0
	unsigned char			Reserved1[3];		// must be 0
	unsigned short			Reserved2;			// must be 0
	unsigned char			MediaDescriptor;	// F8 indicates HDD, F0 indicates 3.5" floppy
	unsigned short			Reserved3;			// must be 0
	unsigned short			NotChecked1;		// ?
	unsigned short			NotChecked2;		// ?
	unsigned long			NotChecked3;		// ?
	unsigned long			Reserved4;			// must be 0
	
};

struct EXTENDED_BIOS_PARAMETER_BLOCK
{
	unsigned long			NotChecked4;						// ?
	unsigned long long		TotalSectors;
	unsigned long long		LogicalClusterNumberForMFT;
	unsigned long long		LogicalClusterNumberForMFTMirror;
	unsigned char			ClustersPerMFTRecord;
	unsigned char			NotChecked5[3];						// ?
	unsigned char			ClustersPerIndexBuffer;
	unsigned char			NotChecked6[3];						// ?
	unsigned long long		VolumeSerialNumber;
	unsigned long			NotChecked7;						// ?
};

struct BOOT_SECTOR
{
	unsigned char					JumpInstruction[3];
	unsigned long long				OEMIdentifier;
	BIOS_PARAMETER_BLOCK			BPB;
	EXTENDED_BIOS_PARAMETER_BLOCK	ExtendedBPB;
	unsigned char					BootStrapCodeAndEndOfSector[1];

	// EndOfSector is 2 bytes at the end of the sector
};

#pragma pack(pop)

#include "reader.h"

namespace bootsector
{
	BOOT_SECTOR * read(reader * volume);
}

