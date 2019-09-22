/*
Copyright (c). Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#pragma once

#include "reader.h"
#include "ntfs.h"
#include "msstructs.h"
#include "attribs.h"
#include "filename.h"
#include "filereference.h"
#include "mftentry.h"

#include <list>
#include <vector>
#include <string>

#include "bmhwide.h"
#include "bmhapu.h"

// CLUSTER.size = clusterSizeInSectors * sectorSizeInBytes;
struct CLUSTER
{
	unsigned long size;
	unsigned char data[1];
};

typedef std::map<unsigned long, ATTRIBUTE_RECORD_HEADER *> attributeMap;
typedef std::multimap<unsigned long, ATTRIBUTE_RECORD_HEADER *> attributeMultiMap;

typedef std::map<VCN, LCN> datarunMap;


typedef void (* rangefn)(int, int);
typedef void (* updatefn)(int);
typedef void (* resultfn)(std::wstring, std::wstring , long long );
typedef bool (* stopreqfn)();

class masterfiletable
{
private:
	static volatile long issues;
	static volatile long completes;

	const LARGE_INTEGER location;

	reader * volreader;
	NTFS * nt;

	datarunMap mftclusters;	// mft on disk... it's allocated in blocks. last element has <..., LCN = 0>.

	

	
	std::wstring  mftfilename;			// mft file name should be "$MFT"

	unsigned char mftrecord[MFT_RECORD_SIZE];
	attributeMultiMap  mftattributes;

	void patchRecord( FILE_RECORD_SEGMENT_HEADER * f) const;

	
	
	// for non-resident types.
	std::map<VCN, LCN> dataruns(ATTRIBUTE_RECORD_HEADER * arh) const;
	

	mutable std::vector<bmh::binary::wordi_t> d1;
	mutable std::vector<bmh::binary::wordi_t> d1fc;

	//mutable bmh::apu::context filename;
	mutable bmh::wide::ci::context filename;
	mutable bmh::binary::context filecontent;

	
	static int WorkerFunction(DWORD transferred, ULONG_PTR key, LPOVERLAPPED ov, BOOL cancel);

	void workItemCompleted();

	void teardown();

	void parseRecords(unsigned char * buffer, unsigned long transferred, unsigned int userContext);
	
	bool matchfilename(attributeMultiMap & attribs);
	bool matchfilecontent(FILE_RECORD_SEGMENT_HEADER * frsh, attributeMultiMap & attribs);

	bool parseContent(ATTRIBUTE_RECORD_HEADER * dataHeader);
	bool existsInContent(char * data, unsigned int dataSize );


	HANDLE evtComplete;
	mutable volatile long fullyIssued;

	CRITICAL_SECTION crit;
	std::map<VCN, std::wstring>		  pathNames;
	std::map<VCN, VCN>				  parentVCN;
	CRITICAL_SECTION				  pathNameCs;

	std::wstring constructPathFrom(FILE_NAME * filename);

	unsigned int					  readSize;


	mutable updatefn				  updateFn;
	mutable resultfn				  resultFn;
	mutable stopreqfn				  stopchkFn;

public:
	masterfiletable(NTFS * nt, reader * vr);

	void init( unsigned int maxreadSize);

	std::list<long long> findfiles(const std::wstring named, const std::string content, rangefn rf, updatefn uf, resultfn resf, stopreqfn stopfn) const;

	bool getEntry(unsigned long long entry, FILE_RECORD_SEGMENT_HEADER * header) const;

	LCN get(VCN vcn) const;

	CLUSTER * get(LCN lcn) const;
	bool get(LCN lcn, CLUSTER * cluster) const;
	bool get(LCN startingLcn, long long amount, char * buffer) const;

	static attributeMultiMap getattributes(const FILE_RECORD_SEGMENT_HEADER * header);


	bool waitToFinish();

	//static void CL_CALLBACK masterfiletable::ClEvCallbackFn(cl_event ev, cl_int event_command_exec_status, void * user_data);

};
