#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_amd_printf : enable


#define SEQUENCE_NUMBER_STRIDE			512
#define MFT_RECORD_SIZE					1024

#ifdef __GPU__
	#define BARRIER ((void) 0)
#else
	#define BARRIER barrier(CLK_LOCAL_MEM_FENCE)
#endif


typedef struct _MULTI_SECTOR_HEADER
{
  uchar  Signature[4];										//	4	| offset 0
  ushort UpdateSequenceArrayOffset;							//	2	| offset 4
  ushort UpdateSequenceArraySize;							//	2	| offset 6
} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;

#define MSH_SIGNATURE_OFFSET									0
#define MSH_USNARRAY_OFFSET										4
#define MSH_USNARRAY_SIZE_OFFSET								6


#define FILE_RECORD_SEGMENT_IN_USE		0x0001

typedef struct _MFT_SEGMENT_REFERENCE
{
	uint	SegmentNumberLowPart;							//	4	| offset 0
	ushort	SegmentNumberHighPart;							//	2	| offset 4
	ushort	SegmentNumber;									//	2	| offset 6
} MFT_SEGMENT_REFERENCE, * PMFT_SEGMENT_REFERENCE;

#define MSR_SEGMENT_LOW_OFFSET									0
#define MSR_SEGMENT_HIGH_OFFSET									4
#define MSR_SEGMENT_NUMBER_OFFSET								6


typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, * PFILE_REFERENCE;

typedef struct _FILE_RECORD_SEGMENT_HEADER
{
  MULTI_SECTOR_HEADER   MultiSectorHeader;					//	8	|  offset 0
  ulong					Lsn;								//	8	|  offset 8
  ushort				SequenceNumber;						//	2	|  offset 16
  ushort				ReferenceCount;						//	2	|  offset 18
  ushort				FirstAttributeOffset;				//	2	|  offset 20
  ushort				Flags;								//	2	|  offset 22
  uint					FirstFreeByte;						//	4	|  offset 24
  uint					BytesAvailable;						//	4	|  offset 28
  FILE_REFERENCE        BaseFileRecordSegment;				//	8	|  offset 32
  ushort				NextAttributeInstance;				//	2	|  offset 40

} FILE_RECORD_SEGMENT_HEADER;

typedef ulong VCN;
typedef long LCN;

typedef uint ATTRIBUTE_TYPE_CODE;

typedef enum _ATTRIBUTE_TYPE_CODES
{
	TYPE_STANDARD_INFORMATION =		0x010,
	TYPE_ATTRIBUTE_LIST =			0x020,
	TYPE_FILE_NAME =				0x030,
	TYPE_OBJECT_ID =				0x040,
	TYPE_SECURITY_DESCRIPTOR =		0x050,
	TYPE_VOLUME_NAME =				0x060,
	TYPE_VOLUME_INFORMATION =		0x070,
	TYPE_DATA =						0x080,
	TYPE_INDEX_ROOT =				0x090,
	TYPE_INDEX_ALLOCATION =			0x0a0,
	TYPE_BITMAP =					0x0b0,
	TYPE_REPARSE_POINT =			0x0c0,
	TYPE_EA_INFORMATION =			0x0d0,
	TYPE_EA =						0x0e0,
	TYPE_LOGGED_UTILITY_STREAM =	0x100,
	TYPE_END_OF_ATTRIBUTES_MARKER = 0xffffffff
} ATTRIBUTE_TYPE_CODES;

typedef enum _FORM_CODES
{
	FORM_CODE_RESIDENT =			0,
	FORM_CODE_NONRESIDENT =			1
} FORM_CODES;

typedef struct _ATTRIBUTE_RECORD_HEADER
{
  uint			TypeCode;			// offset 0
  uint			RecordLength;		// offset 4
  uchar			FormCode;			// offset 8
  uchar			NameLength;			// offset 9
  ushort		NameOffset;			// offset 10
  ushort		Flags;				// offset 12
  ushort		Instance;			// offset 14

  union
  {
    struct
	{
      uint		ValueLength;		// offset 16
      ushort	ValueOffset;		// offset 20
	  uchar		ResidentFlags;		// offset 22
      uchar		Reserved;			// offset 23
    } Resident;

    struct
	{
      VCN			LowestVcn;			// offset 16
      VCN			HighestVcn;			// offset 24
      ushort		MappingPairsOffset;	// offset 32
      uchar			Reserved[6];		// offset 34
      long			AllocatedLength;	// offset 40
      long			FileSize;			// offset 48
      long			ValidDataLength;	// offset 56
      long			TotalAllocated;		// offset 64
    } Nonresident;

  } Form;

} ATTRIBUTE_RECORD_HEADER __attribute__ ((aligned (8)));



typedef struct _DUPLICATED_INFORMATION
{
	long	CreationTime;				// offset 0
	long	LastModificationTime;		// offset 8
	long	LastChangeTime;				// offset 16
	long	LastAccessTime;				// offset 24
	long	AllocatedLength;			// offset 32
	long	FileSize;					// offset 40
	uint	FileAttributes;				// offset 48
	uint	ReparsePointTag;			// offset 52

} DUPLICATED_INFORMATION;

#define FILE_NAME_FLAG_POSIX	0
#define FILE_NAME_FLAG_WIN32	1
#define FILE_NAME_FLAG_DOS		2
#define FILE_NAME_FLAG_WIN32DOS	3

typedef struct _FILE_NAME
{
	FILE_REFERENCE			ParentDirectory;
	DUPLICATED_INFORMATION	Info;
	unsigned char			FileNameLength;
	unsigned char			Flags;			// 0: posix (case sensitive and allows all unicode except NULL and '/'), 1: win32, 2: dos, 3: win32 & dos
	unsigned short			FileName[1];
} FILE_NAME, * PFILE_NAME;




#define FLAGS_OFFSET		22

__kernel
void patchRecord()
{
}


__kernel
void inUse(__global ushort * records, __global int * error)
{
	const int global_id = get_global_id(0);
	const int record_offset = global_id * MFT_RECORD_SIZE;

	if ( (records[ (record_offset + FLAGS_OFFSET) / sizeof(ushort)] & FILE_RECORD_SEGMENT_IN_USE) == 0)
		error[global_id] = 1;
	else
		error[global_id] = 0;
}



#define MFT_SEG_HDR_FIRST_ATTRIBUTE_OFFSET		20
#define RECORD_LENGTH_OFFSET					4

__kernel
void getAttributes(const __global ushort * records, __global int * error, __global int * offsets)
{
	const int global_id = get_global_id(0);
	const int record_offset = global_id * MFT_RECORD_SIZE;
	
	const __global uint * int_records = (__global uint *)records;

	
	uint attribute_offset;
	uint type_code = 0;

	int ctr = 0;

	if (error[global_id] != 0)
		return;

	offsets[global_id * 3 + 0] = 0;
	offsets[global_id * 3 + 1] = 0;
	offsets[global_id * 3 + 2] = 0;

	attribute_offset = records [ ( record_offset + MFT_SEG_HDR_FIRST_ATTRIBUTE_OFFSET ) / sizeof (ushort) ];

	
	do
	{
		type_code = int_records [ ( record_offset + attribute_offset ) / sizeof(uint) ];
		if (type_code == TYPE_FILE_NAME)
			offsets[global_id * 3 + ctr++] = attribute_offset;

		if (ctr == 3)
			break;
		
		attribute_offset += int_records[ (record_offset + attribute_offset + RECORD_LENGTH_OFFSET) / sizeof(uint) ];


	} while (type_code != 0xffffffff && attribute_offset < MFT_RECORD_SIZE);

	if (offsets[global_id * 3] == 0)
		error[global_id] = 1;

}


#define VALUE_OFFSET				20
#define FILENAME_LENGTH_OFFSET		64
#define FILENAME_NAME_OFFSET		66

/*
__kernel __attribute__((reqd_work_group_size(256, 1, 1)))
void matchFileName(		__global ushort *	records,
						__global int *		error,
						__global int *		attributes,
						__global ushort *	pattern,
						const int 			patlen,
						__global int *		delta1,
						__global uint *		tolower
					)
{

	int global_id = get_global_id(0);
	int record_offset = global_id * MFT_RECORD_SIZE;

	uint attribute_offset;
	ushort value_offset;

	__global uchar * char_records = (__global uchar *)records;

	uchar filename_length;
	uint filename_offset;

	int i, j;

	int ctr;

	if (error[global_id] != 0)
		return;
	

	for ( ctr = 0 ; ctr < 3; ctr ++)
	{
		attribute_offset = attributes[global_id * 3 + ctr];

		if (attribute_offset != 0)
		{
			value_offset = records [ (record_offset + attribute_offset + VALUE_OFFSET) / sizeof(ushort) ];
			filename_length = char_records [ record_offset + attribute_offset + value_offset + FILENAME_LENGTH_OFFSET ];
			filename_offset = record_offset + attribute_offset + value_offset + FILENAME_NAME_OFFSET;


			for (j = 0; j <= filename_length - patlen; j += delta1 [ tolower [ records [ filename_offset/sizeof(ushort) + j + patlen - 1 ] ] ] )
			{
				for (i = patlen - 1; i >= 0 && pattern[i] == tolower [ records [ filename_offset / sizeof(ushort) + i + j ] ]; i--);

				if (i < 0)
				{
					return;
				}
			}
		}

	}
	

	error[global_id] = 1;
	

}

*/
__kernel __attribute__((reqd_work_group_size(256, 1, 1)))
void matchFileName(		__global ushort *	records,
						__global int *		error,
						__global int *		attributes,
						__global ushort *	pattern,
						const int 			patlen,
						__global int *		delta1,
						__global uint *		tolower
					)
{

	int global_id = get_global_id(0);
	int record_offset = global_id * MFT_RECORD_SIZE;

	uint attribute_offset;
	ushort value_offset;

	__global uchar * char_records = (__global uchar *)records;

	uchar filename_length;
	uint filename_offset;

	int i, j;

	int ctr;

	__local ushort p[256];

	for (j = 0; j < patlen; ++j)
		p[j] = pattern[j];


	BARRIER;

	if (error[global_id] != 0)
		return;
	
	ctr = 0;
	attribute_offset = attributes[global_id * 3 + ctr];

	if (attribute_offset == 0)
	{
		error[global_id] = 1;
		return;
	}

	
	value_offset = records [ (record_offset + attribute_offset + VALUE_OFFSET) / sizeof(ushort) ];
	filename_length = char_records [ record_offset + attribute_offset + value_offset + FILENAME_LENGTH_OFFSET ];
	filename_offset = record_offset + attribute_offset + value_offset + FILENAME_NAME_OFFSET;

	for (j = 0; j <= filename_length - patlen; j += delta1 [ tolower [ records [ filename_offset/sizeof(ushort) + j + patlen - 1 ] ] ] )
	{
		for (i = patlen - 1; i >= 0 && p[i] == tolower [ records [ filename_offset / sizeof(ushort) + i + j ] ]; i--);
		
		if (i < 0)
			return;
	}

	ctr++;
	attribute_offset = attributes[global_id * 3 + ctr];

	if (attribute_offset == 0)
	{
		error[global_id] = 1;
		return;
	}
	
	value_offset = records [ (record_offset + attribute_offset + VALUE_OFFSET) / sizeof(ushort) ];
	filename_length = char_records [ record_offset + attribute_offset + value_offset + FILENAME_LENGTH_OFFSET ];
	filename_offset = record_offset + attribute_offset + value_offset + FILENAME_NAME_OFFSET;

	for (j = 0; j <= filename_length - patlen; j += delta1 [ tolower [ records [ filename_offset/sizeof(ushort) + j + patlen - 1 ] ] ] )
	{
		for (i = patlen - 1; i >= 0 && p[i] == tolower [ records [ filename_offset / sizeof(ushort) + i + j ] ]; i--);

		if (i < 0)
			return;
	}
	

	ctr++;
	attribute_offset = attributes[global_id * 3 + ctr];

	if (attribute_offset == 0)
	{
		error[global_id] = 1;
		return;
	}

	value_offset = records [ (record_offset + attribute_offset + VALUE_OFFSET) / sizeof(ushort) ];
	filename_length = char_records [ record_offset + attribute_offset + value_offset + FILENAME_LENGTH_OFFSET ];
	filename_offset = record_offset + attribute_offset + value_offset + FILENAME_NAME_OFFSET;

	for (j = 0; j <= filename_length - patlen; j += delta1 [ tolower [ records [ filename_offset/sizeof(ushort) + j + patlen - 1 ] ] ] )
	{
		for (i = patlen - 1; i >= 0 && p[i] == tolower [ records [ filename_offset / sizeof(ushort) + i + j ] ]; i--);

		if (i < 0)
		{
			return;
		}
	}

	error[global_id] = 1;

}



__kernel
void reduce ( __global int * error, __global int * valids)
{
	int globalId = get_global_id(0);
	unsigned int k;


	if (error[globalId] == 0)
	{
		k = atomic_inc(valids);
		valids[k+1] = globalId;
	}
}



#define REGISTER_SIZE	8

typedef long wordi_t;
typedef ulong word_t;

/*
__kernel
void findx(	__global uchar * pattern,
			const int m,
			__global wordi_t * delta1,
			__global uchar * text,
			const int n,
			__global int * found)
{
	int globalId = get_global_id(0);
	int offset = globalId * m;


	int i = m - 1;
	wordi_t d;

	int k;
	int j =0;

	found[globalId] = 0;

	
	// need to think about the end boundary case...
	
	if (offset + i <= n)
	{
		
		for (; i >= 0 && pattern[i] == text [offset + i]; i--);
		if (i < 0)
			found[globalId] = 1;
		else
		{
			d = delta1[ text [offset + i]];
			if (d != m)
			{
				offset += d;
				i = m - 1;
				if (offset + i <= n)
				{
					for (; pattern[i] == text [offset + i]; i--);
			
					if (i < 0)
						found[globalId] = 1;

				}

			}
		
		}
		
	}



}
*/


__kernel
void findx(	__global uchar * pattern,
			const int m,
			__global wordi_t * delta1,
			__global uchar * text,
			const int n,
			__global int * found)
{
	int globalId = get_global_id(0);
	int offset = globalId * m;

	__local uchar p[256];
	__local wordi_t d1[256];

	int i = m - 1;
	wordi_t d;

	int k;
	int j =0;

	for (; j < m; j++)
		p[j] = pattern[j];

	for (j = 0; j < 256; j++)
		d1[j] = delta1[j];

	j = 0;

	BARRIER;

	found[globalId] = 0;

	
	// need to think about the end boundary case...
	
	if (offset + i <= n)
	{
		for (; i >= 0 && p[i] == text [offset + i]; i--);

		if (i < 0)
			found[globalId] = 1;
		else
		{
			d = d1[ text [offset + i]];
			if (d != m)
			{
				offset += d;
				i = m - 1;
				if (offset + i <= n)
				{
					for (; p[i] == text [offset + i]; i--);
			
					if (i < 0)
						found[globalId] = 1;

				}

			}
		
		}
		
	}



}
