#pragma once

#include "filereference.h"

// http://msdn.microsoft.com/en-us/library/bb470123(v=VS.85).aspx

typedef struct _DUPLICATED_INFORMATION
{
	long long		CreationTime;
	long long		LastModificationTime;
	long long		LastChangeTime;
	long long		LastAccessTime;
	long long		AllocatedLength;
	long long		FileSize;
	unsigned long	FileAttributes;
	unsigned long	ReparsePointTag;
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
	wchar_t					FileName[1];
} FILE_NAME, * PFILE_NAME;