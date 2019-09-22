#pragma once

// http://msdn.microsoft.com/en-us/library/bb545266(v=VS.85).aspx

// present in every base file record and is resident by definition.
typedef struct _STANDARD_INFORMATION
{
	long long			CreationTime;
	long long			LastModificationTime;
	long long			LastChangeTime;
	long long			LastAccessTime;
	unsigned long		FileAttributes;
	unsigned long		MaximumVersions;
	unsigned long		VersionNumber;
	unsigned long		ClassId;
	unsigned long		OwnerId;
	unsigned long		SecurityId;
	unsigned long long	QuotaCharged;
	unsigned long long	Usn;
} STANDARD_INFORMATION, * PSTANDARD_INFORMATION;