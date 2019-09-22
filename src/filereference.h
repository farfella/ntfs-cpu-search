
#pragma once

const unsigned int MFT_RECORD_SIZE = 1024;

// http://msdn.microsoft.com/en-us/library/bb470211(v=VS.85).aspx

typedef struct _MFT_SEGMENT_REFERENCE
{
	unsigned long SegmentNumberLowPart;
	unsigned short SegmentNumberHighPart;
	unsigned short SegmentNumber;
} MFT_SEGMENT_REFERENCE, * PMFT_SEGMENT_REFERENCE;

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, * PFILE_REFERENCE;