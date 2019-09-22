#pragma once

#include "filereference.h"

typedef struct _MULTI_SECTOR_HEADER
{
  unsigned char  Signature[4];						// "FILE" "BAAD" for example
  unsigned short UpdateSequenceArrayOffset;			// offset to fixup array
  unsigned short UpdateSequenceArraySize;			// number of entries in fixup array
} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;

#define FILE_RECORD_SEGMENT_IN_USE		0x0001
#define FILE_FILE_NAME_INDEX_PRESENT	0x0002

typedef struct _FILE_RECORD_SEGMENT_HEADER			// an MFT entry
{
  MULTI_SECTOR_HEADER   MultiSectorHeader;
  unsigned long long	Lsn;						// $LogFile Sequence Number
  unsigned short		SequenceNumber;				// incremented when entry is either allocated or unallocated, determined by OS.
  unsigned short		ReferenceCount;				// Link count: how many directories have entries for this MFT entry...
  unsigned short		FirstAttributeOffset;		// Attribute to first offset: end of file marker (0xffffffff) exists after last attribute.
  unsigned short		Flags;						// two values: 1: in-use and 2: directory (if in-use bit is 0, file was deleted...)
  unsigned long			FirstFreeByte;				// used size of mft entry
  unsigned long			BytesAvailable;				// allocated size of mft entry
  FILE_REFERENCE        BaseFileRecordSegment;		// file reference to base record: if a file needs multiple MFT entries, all will have this same BaseFileRecordSegment
  unsigned short		NextAttributeInstance;		// next attribute id
  
  // UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;		// attributes and fixup values...
  unsigned long			SegmentNumberLowPart;
  unsigned short		SegmentNumberHighPart;

} FILE_RECORD_SEGMENT_HEADER;

typedef union _MFT_RECORD
{
	unsigned char				Data[MFT_RECORD_SIZE];
	FILE_RECORD_SEGMENT_HEADER	header;
} MFT_RECORD;