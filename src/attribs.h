#pragma once

#include "filereference.h"

typedef unsigned long long VCN;	// virtual cluster number
typedef long long LCN; // logical cluster number

typedef unsigned long ATTRIBUTE_TYPE_CODE;

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


#pragma pack(push, 8)
typedef struct _ATTRIBUTE_RECORD_HEADER
{
  ATTRIBUTE_TYPE_CODE	TypeCode;		// code enum from above
  unsigned long			RecordLength;	// how large this attrib record is until the next one or 0xffffffff
  unsigned char			FormCode;		// resident (0) or non-resident (1)
  unsigned char			NameLength;
  unsigned short		NameOffset;
  unsigned short		Flags;			// 0x0001 = compressed, 0x4000 = encrypted, 0x8000 = sparse
  unsigned short		Instance;		// unique per MFT entry

  union
  {
    struct
	{
      unsigned long		ValueLength;
      unsigned short	ValueOffset;
	  unsigned char		ResidentFlags;
      unsigned char		Reserved;
    } Resident;

    struct
	{
      VCN				LowestVcn;
      VCN				HighestVcn;
      unsigned short	MappingPairsOffset;	// from the start of the attribute (attribute_record_header)...				32-33
      unsigned char		Reserved[6];		//	us compressionunitsize, ulong unused									34-39
      long long			AllocatedLength;	//  allocated size of attribute content										40-47
      long long			FileSize;			//  actual size of attribute content										48-55
      long long			ValidDataLength;	//  initialized size of attribute content...								56-63
      long long			TotalAllocated;
    } Nonresident;							// run-list is variable length, and must be at least 1 byte. first byte of runlist = upper nibble, lower nibble.
											// upper nibble = # of bytes in the run length field
											// lower nibble = length
											// values are cluster-sized units, and the offset field is a signed value relative to the previous offset.

											// [nibble run offset][nibble run length] [run length] [run offset]

											// example:
											// 32 c0 1e b5 3a 05 21 70 1b 1f
											// -- ----- --------
											// lower 4 bits: 2 = two bytes in field for run length
											// upper 4 bits: 3 = three bytes in field for run offset
											// c0 1e = length = 7872 clusters (little endian unsigned short, so swap to 1e c0)
											// b5 3a 05 = offset = 342709 clusters

											// next	run: 21 70 1b 1f
											// repeat until highest vcn reached from lowest vcn (which is 0).

  } Form;

} ATTRIBUTE_RECORD_HEADER, *PATTRIBUTE_RECORD_HEADER;
#pragma pack(pop)


#pragma pack(push, 8)
typedef struct _ATTRIBUTE_LIST_ENTRY
{
  ATTRIBUTE_TYPE_CODE   AttributeTypeCode;
  unsigned short		RecordLength;
  unsigned char			AttributeNameLength;
  unsigned char			AttributeNameOffset;
  VCN                   LowestVcn;
  MFT_SEGMENT_REFERENCE SegmentReference;
  unsigned short		Reserved;
  wchar_t               AttributeName[1];
} ATTRIBUTE_LIST_ENTRY, *PATTRIBUTE_LIST_ENTRY;
#pragma pack(pop)


#define ATTRIBUTE_CAN_BE_USED_IN_INDEX	0x02
#define ATTRIBUTE_ALWAYS_RESIDENT		0x04
#define ATTRIBUTE_CAN_BE_NONRESIDENT	0x08
 

struct ATTRIBUTE_DEFINITION_ENTRY
{
	wchar_t				Name[64];
	unsigned int		TypeCode;
	unsigned int		DisplayRule;
	unsigned int		CollationRule;
	unsigned int		Flags;
	unsigned long long	MinimumSize;
	unsigned long long  MaximumSize;
};



#define VOLUME_INFORMATION_FLAG_DIRTY				0x0001
#define VOLUME_INFORMATION_RESIZE_LOGFILE			0x0002
#define VOLUME_INFORMATION_UPGRADE_VOLUME_NEXT_TIME	0x0004
#define VOLUME_INFORMATION_MOUNTED_IN_NT			0x0008
#define VOLUME_INFORMATION_DELETING_JOURNAL			0x0010
#define VOLUME_INFORMATION_REPAIR_OBJIDS			0x0020
#define VOLUME_INFORMATION_MODIFIED_BY_CHKDSK		0x8000


struct VOLUME_INFORMATION_ATTRIBUTE
{
	unsigned char		Reserved[8];
	unsigned char		Major;
	unsigned char		Minor;
	unsigned short		Flags;
};