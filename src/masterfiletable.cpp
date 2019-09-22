/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include <map>
#include "masterfiletable.h"
#include "stdinfo.h"

#include "bmhapu.h"

#include "mempool.h"
#include "threadpool.h"

#include "cs.h"
#include <string>

#include "ods.h"

#include "threadpool.h"

#define MFT_CL_EVENT		501

struct EventStruct
{
	unsigned char * buffer;
	int nRecords;
	int * reduceHost;
	masterfiletable * mft;
};

bool bmhinsensitive( wchar_t *		pattern,
						 int 			patlen,
						 int *			delta1,
						 unsigned int *	tolower,
						 wchar_t *		text,
						 int			txtlen
					)
{

	for (int j = 0; j <= txtlen - patlen; j += delta1 [ tolower [ text [ j + patlen - 1 ] ] ] )
	{
		int i;

		for (i = patlen - 1; i >= 0 && pattern[i] == tolower [ text [ i + j ] ]; i--);
		
		if (i < 0)
			return true;
	}

	return false;
}

bool bmhsensitive( unsigned char *		pattern,
						 int 			patlen,
						 bmh::binary::wordi_t *			delta1,
						 unsigned char *		text,
						 int			txtlen
					)
{

	for (int j = 0; j <= txtlen - patlen; j += delta1 [ text [ j + patlen - 1 ] ] )
	{
		int i;

		for (i = patlen - 1; i >= 0 && pattern[i] == text [ i + j ] ; i--);
		
		if (i < 0)
			return true;
	}

	return false;
}


masterfiletable::masterfiletable(NTFS * n, reader * r):location(n->mftLocation)
{
	volreader = r;
	nt = n;
	ZeroMemory(mftrecord, ARRAYSIZE(mftrecord));

	InitializeCriticalSection(&crit);
	InitializeCriticalSection(&pathNameCs);
}

void masterfiletable::init( unsigned int maxReadSize )
{
	readSize = maxReadSize;

	const LARGE_INTEGER readfrom = location;
	unsigned long const read = volreader->read(mftrecord, MFT_RECORD_SIZE, location);
	if (read == MFT_RECORD_SIZE)
	{
		FILE_RECORD_SEGMENT_HEADER * f = (FILE_RECORD_SEGMENT_HEADER *)mftrecord;

		patchRecord(f);

		if (*(DWORD*)f->MultiSectorHeader.Signature == 'ELIF')
		{
			mftattributes = getattributes(f);
			
			// mft's TYPE_DATA attribute IS NON-RESIDENT. We are guaranteed this.
			mftclusters = dataruns(mftattributes.find(TYPE_DATA)->second);

			FILE_NAME * fn = (FILE_NAME*)((DWORD)(mftattributes.find(TYPE_FILE_NAME)->second) + (DWORD)(mftattributes.find(TYPE_FILE_NAME)->second->Form.Resident.ValueOffset));
			mftfilename.resize(fn->FileNameLength+1);
			lstrcpyn(&mftfilename[0], fn->FileName, fn->FileNameLength);

			evtComplete = CreateEvent(NULL, FALSE, FALSE, NULL);


		}
	}
}

LCN masterfiletable::get(VCN vcn) const
{
	LCN ret = 0;
	bool found = false;
	std::map<VCN, LCN>::const_iterator i = mftclusters.begin();
	for (; i != mftclusters.end(); ++i)
	{
		if (i->second == 0)	// eof
			break;

		std::map<VCN, LCN>::const_iterator j = i;
		++j;

		ret += i->second;	// not absolute... relative from the previous :-/

		if (i->first <= vcn && vcn < j->first)
		{
			ret += (vcn - i->first);
			found = true;
			break;
		}
	}

	if (!found)
		ret = -1;

	return ret;
}

bool masterfiletable::get(LCN lcn, CLUSTER * cluster) const
{
	bool ret = false;
	if (cluster->size == nt->clusterSizeInSectors * nt->sectorSizeInBytes)
	{
		LARGE_INTEGER r;
		r.QuadPart = lcn * nt->clusterSizeInSectors * nt->sectorSizeInBytes;

		cluster->size = volreader->read(cluster->data, nt->clusterSizeInSectors * nt->sectorSizeInBytes, r);
		if (cluster->size == nt->clusterSizeInSectors * nt->sectorSizeInBytes)
			ret = true;
	}

	return ret;
}

bool masterfiletable::get(LCN lcn, long long amount, char * buffer) const
{
	bool ret = false;
	
	LARGE_INTEGER r;
	r.QuadPart = lcn * nt->clusterSizeInSectors * nt->sectorSizeInBytes;
	ULONG const to = volreader->read(buffer, amount, r);

	if (to == amount)
		ret = true;

	return ret;
}


CLUSTER * masterfiletable::get(LCN lcn) const
{
	LARGE_INTEGER r;
	r.QuadPart = lcn * nt->clusterSizeInSectors * nt->sectorSizeInBytes;

	unsigned char * buffer = (unsigned char*) malloc (nt->clusterSizeInSectors * nt->sectorSizeInBytes + sizeof (unsigned long));
	CLUSTER* rv = NULL;

	if (NULL != buffer)
	{

		if ((nt->clusterSizeInSectors * nt->sectorSizeInBytes + sizeof(unsigned long)) > memory::pool::chunksize())
		{
			ods("clustersize + ulong > mempool::chunksize... unexpected\n");
			ExitProcess(0);
		}


		rv = (CLUSTER*)buffer;
		rv->size = nt->clusterSizeInSectors * nt->sectorSizeInBytes;

		if (!get(lcn, rv))
		{
			free(buffer);
			rv = NULL;
		}
	}

	return rv;
}

bool masterfiletable::getEntry(unsigned long long entry, FILE_RECORD_SEGMENT_HEADER * header) const
{
	LARGE_INTEGER location;
	location.QuadPart = entry * MFT_RECORD_SIZE;

	VCN vcn = entry * MFT_RECORD_SIZE / ((unsigned long long)nt->clusterSizeInSectors * nt->sectorSizeInBytes);
	LCN lcn = get(vcn);

	bool ret = false;

	if (lcn != -1)
	{
		CLUSTER * cluster = get(lcn);

		long long locationInCluster = entry * MFT_RECORD_SIZE % ((unsigned long long) nt->clusterSizeInSectors * nt->sectorSizeInBytes);

		memcpy(header, (char*)cluster->data + locationInCluster, MFT_RECORD_SIZE);
		patchRecord(header);

		ret = true;

		free(cluster);

	}


	return ret;

}

// records are 1024 bytes always...
void masterfiletable::patchRecord(FILE_RECORD_SEGMENT_HEADER * f) const
{
	unsigned char * record = (unsigned char *) f;
	/* there will be three items in the fixup array*
	/* first item = the check value */
	/* second value = to replace the last word in the first cluster, if check values match */
	/* third value = to replace the last word in the second cluster, if check values match */
	unsigned short size = f->MultiSectorHeader.UpdateSequenceArraySize;
	unsigned short * offset = (WORD*)(record + f->MultiSectorHeader.UpdateSequenceArrayOffset);

	unsigned int j = 1;
	for (unsigned int i = SEQUENCE_NUMBER_STRIDE; i <= MFT_RECORD_SIZE; i += SEQUENCE_NUMBER_STRIDE)
	{
		unsigned short * value = (WORD *)(record + i - 2);
		if (*value == offset[0])
			*value = offset[j];
		else if (offset[j] != *value)	// houston, we have a problem
		{
			ods("a particular record could not be patched\n");
			return;
		}

		j++;
	}
}

// for non-resident types.
// arh->FormCode MUST BE FORM_CODE_NONRESIDENT
std::map<VCN, LCN> masterfiletable::dataruns(ATTRIBUTE_RECORD_HEADER * arh) const
{
	std::map<VCN, LCN> ret;

	VCN lowest = arh->Form.Nonresident.LowestVcn;
	VCN highest = arh->Form.Nonresident.HighestVcn;

	unsigned short PairsOffset = arh->Form.Nonresident.MappingPairsOffset;
	unsigned char const * p = ((BYTE *)arh + PairsOffset);

	VCN current = lowest;
	long long nowlcn = 0;
	do
	{
		BYTE FirstByteInRunLength = *p;
		BYTE LengthSize = FirstByteInRunLength & 0xf;
		BYTE OffsetSize = FirstByteInRunLength >> 4;


		if (LengthSize < 1 || LengthSize > 8)
		{
			// problem with file...
			ret.clear();
			return ret;
		}
			

		p++;

		unsigned long long DataRunLength = 0;
		memcpy(&DataRunLength, p, LengthSize);

			
		p += LengthSize;

		long long lcn = 0;

		if (OffsetSize < 1 || OffsetSize > 8)
		{
			// problem with file...
			ret.clear();
			return ret;
		}
			

		if (p[OffsetSize-1] & 0x80)
			lcn = -1;

		memcpy(&lcn, p, OffsetSize);

		ret[current] = lcn;

		nowlcn += lcn;


		current += DataRunLength;
		p += OffsetSize;

	} while (current <= highest);

	ret[current] = 0;

	return ret;
}

attributeMultiMap masterfiletable::getattributes(const FILE_RECORD_SEGMENT_HEADER * header)
{
	attributeMultiMap attributes;
	ATTRIBUTE_RECORD_HEADER * arh = (ATTRIBUTE_RECORD_HEADER *)( (DWORD)header + header->FirstAttributeOffset);

	if (arh->TypeCode == 0)
	{
		// this is unexpected...
		ods("mft::getattributes: unexpected typecode (0).\n");
	}
	else
		do
		{
			attributes.insert(std::make_pair(arh->TypeCode, arh));

			arh = (ATTRIBUTE_RECORD_HEADER*) (arh->RecordLength + (unsigned char *)arh);

			if ((DWORD)arh > (DWORD)header + MFT_RECORD_SIZE)
				break;

		} while (arh->TypeCode != 0xffffffff);

	return attributes;
}


volatile long masterfiletable::issues = 0;
volatile long masterfiletable::completes = 0;


int masterfiletable::WorkerFunction(DWORD transferred, ULONG_PTR key, LPOVERLAPPED lpOv, BOOL cancel)
{

	IOCPOVERLAPPED * lpc = CONTAINING_RECORD(lpOv, IOCPOVERLAPPED, ov);

	masterfiletable * mft = (masterfiletable *) lpc->p;

	//memory::pool::free(lpc->b);

	mft->parseRecords((unsigned char*)lpc->b, transferred, lpc->threadId);

	delete lpc;

	return 0;
}

void masterfiletable::workItemCompleted()
{
	long c = InterlockedIncrement(&completes);
	if (fullyIssued)
	{
		if (c >= issues)
			SetEvent(evtComplete);
	}
}

void masterfiletable::teardown()
{

}

bool masterfiletable::matchfilename(attributeMultiMap & attribs)
{
	bool matched = false;

	if (filename.length > 0)
	{
		auto range = attribs.equal_range(TYPE_FILE_NAME);
		for (auto i = range.first; i != range.second; ++i)
		{
			auto attrib = i->second;
			FILE_NAME * fn = (FILE_NAME *) ((char*)attrib + attrib->Form.Resident.ValueOffset);

			if (bmhinsensitive(filename.pattern, filename.length, &filename.d1[0], (unsigned int*)&filename.lower[0], fn->FileName, fn->FileNameLength ))
			{
				matched = true;
				break;
			}
		}
	}
	else
		matched = true;

	return matched;
}

bool masterfiletable::matchfilecontent(FILE_RECORD_SEGMENT_HEADER * frsh, attributeMultiMap & attribs)
{
	bool matched = false;
	if (filecontent.length > 0)
	{
		char * contents = 0;
		long long contentLength = 0;

		auto kiter = attribs.find(TYPE_DATA);

		if (kiter != attribs.end())
		{
			matched = parseContent(kiter->second);
		}
		else
		{
			// no type data found... check for attributelist
			kiter = attribs.find(TYPE_ATTRIBUTE_LIST);

			MFT_RECORD rec = {0};
			memcpy(&rec, frsh, MFT_RECORD_SIZE);

			// we need the base record...
			if (kiter == attribs.end())
			{
				long long entry = *(long long *)&(frsh->BaseFileRecordSegment);
				entry &= 0xffffffffffff;

				getEntry(entry, &rec.header);

				attribs = getattributes(&rec.header);
				kiter = attribs.find(TYPE_ATTRIBUTE_LIST);
			}

			if (kiter != attribs.end())
			{
				ATTRIBUTE_RECORD_HEADER * arh = kiter->second;

				if (arh->FormCode == FORM_CODE_RESIDENT)
				{
					DWORD len = arh->Form.Resident.ValueLength;
										
					char * p = (char *)arh + arh->Form.Resident.ValueOffset;
					do
					{
						ATTRIBUTE_LIST_ENTRY * ale = (ATTRIBUTE_LIST_ENTRY *)p;
						len -= ale->RecordLength;

						if (ale->AttributeTypeCode == TYPE_DATA)
						{
							long long entry =  *(long long*)&ale->SegmentReference;
							entry &= 0xffffffffffff;

							MFT_RECORD hr = {0};
							getEntry(entry, &hr.header);


							attributeMultiMap const attribs2 = getattributes(&hr.header);
							attributeMultiMap::const_iterator kiter2 = attribs2.find(TYPE_DATA);
							if (kiter2 != attribs2.end())
							{
								matched = parseContent(kiter2->second);
							}
							else
							{
								// this is an error in the NTFS volume itself...
							}

						}

						p += ale->RecordLength;

					} while (len != 0 && !matched);			// do while !found
				}
				else // ooooh, non-resident attribute list... rare
				{
					long long dataSize = arh->Form.Nonresident.ValidDataLength;

					std::map<VCN, LCN> data = dataruns(arh);
					std::map<VCN, LCN>::iterator k = data.begin();

					LCN offset = 0;
					VCN current = 0;

					for (; k != data.end(); ++k)
					{
						if (k->second == 0)
							break;

						current = k->first; 
						offset += k->second;

						std::map<VCN, LCN>::iterator l = k;
						++l;

						LCN offsetCounter = offset;

						while (current < l->first)
						{
							CLUSTER * attrd = get(offsetCounter);

							unsigned int toParse;
							if (dataSize < (long long) attrd->size)
								toParse = (unsigned int)dataSize;
							else
							{
								toParse = attrd->size;
								dataSize -= attrd->size;
							}


							char * p = (char *)attrd->data;
									
							do
							{
								ATTRIBUTE_LIST_ENTRY * ale = (ATTRIBUTE_LIST_ENTRY *)p;
								toParse -= ale->RecordLength;

								if (ale->AttributeTypeCode == TYPE_DATA)
								{
									long long entry =  *(long long*)&ale->SegmentReference;
									entry &= 0xffffffffffff;

									MFT_RECORD hr = {0};
									getEntry(entry, &hr.header);


									attributeMultiMap const attribs2 = getattributes(&hr.header);
									attributeMultiMap::const_iterator kiter2 = attribs2.find(TYPE_DATA);
									if (kiter2 != attribs2.end())
									{
										matched = parseContent(kiter2->second);
									}
									else
									{
										// won't happen unless volume is bad.
									}

								}

								p += ale->RecordLength;
							} while (toParse != 0 && !matched);	// while !found


							free(attrd);
							current ++;
							offsetCounter ++;
						}
					}
				}
			}
					

		}
	}
	else
		matched = true;

	return matched;
}

void masterfiletable::parseRecords(unsigned char * buffer, unsigned long bytesTransferred, unsigned int userContext)
{
	const unsigned long nRecords = bytesTransferred / MFT_RECORD_SIZE;

	if (stopchkFn())
	{
		memory::pool::free(buffer);

		updateFn(nRecords);
		workItemCompleted();
		return;

	}

	for (unsigned long i = 0; i < bytesTransferred; i += MFT_RECORD_SIZE)
	{
		FILE_RECORD_SEGMENT_HEADER * frsh = (FILE_RECORD_SEGMENT_HEADER*)(buffer + i);
		patchRecord ( frsh );

		if (frsh->Flags & FILE_RECORD_SEGMENT_IN_USE)
		{
			

			auto attribs = getattributes(frsh);
			auto attribCopy = attribs;

			bool matched = matchfilename(attribs);
			if (matched)
				matched = matchfilecontent(frsh, attribs);

			if (matched)
			{
				FILE_NAME * longest = NULL;
				std::wstring name;

				auto range = attribCopy.equal_range(TYPE_FILE_NAME);
				for (auto j = range.first; j != range.second; ++j)
				{
					FILE_NAME * fn = (FILE_NAME *) ((char*)j->second + j->second->Form.Resident.ValueOffset);
					if (name.length() < fn->FileNameLength)
					{
						name.assign(fn->FileName, fn->FileNameLength);
						longest = fn;
					}
				}

				if (NULL != longest)
				{
					std::wstring filepath = constructPathFrom(longest);
					resultFn(name, filepath, longest->Info.LastAccessTime);
				}
			}
		}
	}

	memory::pool::free(buffer);
	updateFn(nRecords);
	workItemCompleted();
	return;

}

std::wstring masterfiletable::constructPathFrom(FILE_NAME * filename)
{
	std::wstring name;
	std::wstring path;

	MFT_RECORD record = {0};
	FILE_NAME * fn = filename;
	std::wstring current;

	name.assign(filename->FileName, filename->FileNameLength);

	std::map<VCN, std::wstring>::iterator pn = pathNames.end();

	VCN entry = fn->ParentDirectory.SegmentNumberHighPart;
	entry <<= 16;
	entry |= fn->ParentDirectory.SegmentNumberLowPart;

	do
	{
		{
			cs _(pathNameCs);
			pn = pathNames.find(entry);
		}
		if (pn != pathNames.end())
		{
			current = pn->second;
			path = current + L"\\" + path;
			entry = parentVCN[entry];
		}
		else if (getEntry(entry, &record.header))
		{
			attributeMultiMap am = getattributes(&record.header);
			attributeMultiMap::iterator amit = am.find(TYPE_FILE_NAME);
			attributeMultiMap::iterator touse = amit;
			if (amit != am.end())
			{
				std::wstring part;
				auto is = am.equal_range(TYPE_FILE_NAME);
				for (amit = is.first; amit != is.second; ++amit)
				{
			
					fn = (FILE_NAME*)((DWORD)(amit->second) + (DWORD)(amit->second->Form.Resident.ValueOffset));
					//
					if (part.length() < fn->FileNameLength)
					{
						part.assign(fn->FileName, fn->FileNameLength);
						touse = amit;
					}
				}


				fn = (FILE_NAME*)((DWORD)(touse->second) + (DWORD)(touse->second->Form.Resident.ValueOffset));

				current = part;
				path = current + L"\\" + path;

				VCN newParent = fn->ParentDirectory.SegmentNumberHighPart;
				newParent <<= 16;
				newParent |= fn->ParentDirectory.SegmentNumberLowPart;
				
				{
					cs _(pathNameCs);
					pathNames[entry] = current;
					parentVCN[entry] = newParent;
				}
				entry = newParent;

			}
		}
		else
		{
			OutputDebugString(L"constructPathFrom: getEntry failed");
			break;
		}

	} while (current != L".");


	path += name;

	return path;
}


bool masterfiletable::existsInContent(char * data, unsigned int dataSize )
{
	bool found = false;

	if (dataSize > 0)
	{
		found = bmhsensitive(filecontent.pattern, filecontent.length, &filecontent.d1[0], (unsigned char*)data, dataSize);
	}


	return found;
}
	

bool masterfiletable::parseContent(ATTRIBUTE_RECORD_HEADER * dataHeader)
{
	ATTRIBUTE_RECORD_HEADER * arh = dataHeader;

	bool ret = false;
	if (arh->FormCode == FORM_CODE_RESIDENT)
	{
		if (arh->NameLength == 0 && arh->Form.Resident.ValueLength > 0)
		{
			if (existsInContent((char*)arh + arh->Form.Resident.ValueOffset, arh->Form.Resident.ValueLength))
				ret = true;
		}

	}
	else
	{
		long long dataSize = arh->Form.Nonresident.ValidDataLength;
		if (dataSize > 0 && !ret)
		{

			std::map<VCN, LCN> data = dataruns(arh);
			
			LCN offset = 0;
			VCN current = 0;

			char * tmpbuffer = (char*) malloc ( filecontent.length * 2 + 1);
			bool first = true;

			for (std::map<VCN, LCN>::iterator k = data.begin(); k != data.end() && !ret; ++k)
			{
				if (k->second == 0)
					break;


				current = k->first; 
				offset += k->second;

				std::map<VCN, LCN>::iterator l = k;
				++l;

				LCN offsetCounter = offset;

				long long amount = (l->first - current) * nt->clusterSizeInSectors * nt->sectorSizeInBytes;

				char * buffer = (char*) malloc(amount);

				if (NULL != buffer)
				{
					if (get(offsetCounter, amount, buffer))
					{
						if (!first && tmpbuffer)
						{
							memcpy(tmpbuffer + filecontent.length, buffer, filecontent.length);
							tmpbuffer[filecontent.length * 2] = 0;
							if (strstr(tmpbuffer, (char*)filecontent.pattern) != 0)
							{
								ret = true;
								break;	// match on a boundary...
							}
						}

						UINT toParse;
						if (amount > dataSize)
						{
							toParse = (UINT)dataSize;
							amount = dataSize;
						}
						else
							toParse = (UINT)amount;

						if (existsInContent(buffer, toParse))
							ret = true;

						if (!ret && tmpbuffer)
						{
							memcpy(tmpbuffer, buffer + toParse - filecontent.length, filecontent.length);
						}
					}

					free(buffer);
				}

				dataSize -= amount;

				if (dataSize < 0)
					break;
				
				first = false;
				
			}

			free(tmpbuffer);

		}

	}

	return ret;
}


// NOTE: NOT DESIGNED TO BE THREAD-SAFE... ONLY ONE FINDFILES CAN BE CALLED AT A TIME...
std::list<long long> masterfiletable::findfiles(const std::wstring named, const std::string content, rangefn rf, updatefn uf, resultfn resf, stopreqfn stopfn) const
{
	std::list<long long> output;
	LARGE_INTEGER location = {0};
	
	if (bmh::wide::ci::delta1(named.c_str(), named.length(), filename))
	{
	}
	else
		filename.length = 0;

	if (bmh::binary::delta1((unsigned char *)content.c_str(), content.length(), filecontent))
	{
	}
	else
	{
		filecontent.length = 0;
	}

	fullyIssued = false;

	rf(0, mftclusters.rbegin()->first * ((unsigned long long)nt->clusterSizeInSectors * nt->sectorSizeInBytes )/ MFT_RECORD_SIZE );
	updateFn = uf;
	resultFn = resf;
	stopchkFn = stopfn;
	
	for (datarunMap::const_iterator i = mftclusters.begin(); i != mftclusters.end(); ++i)
	{
		if (i->second == 0LL)	// eof;
			break;

		datarunMap::const_iterator j = i;
		++j;	// next record in the dataruns... used to compute LCNs.
		const VCN length = j->first - i->first;

		location.QuadPart += i->second * nt->clusterSizeInSectors * nt->sectorSizeInBytes;

		for (unsigned long long k = 0; k < length * nt->clusterSizeInSectors * nt->sectorSizeInBytes && !stopchkFn(); )
		{
			LARGE_INTEGER r;
			r.QuadPart = location.QuadPart;
			r.QuadPart += k;
			
			IOCPOVERLAPPED * lpOv = new IOCPOVERLAPPED;
			ZeroMemory(lpOv, sizeof(IOCPOVERLAPPED));
			lpOv->fn = WorkerFunction;
			lpOv->ov.Offset = r.LowPart;
			lpOv->ov.OffsetHigh = r.HighPart;
			lpOv->p = this;
			

			void * buffer = memory::pool::get();

			lpOv->b = buffer;

			unsigned long amountToRead = memory::pool::chunksize();
			if ((length * nt->clusterSizeInSectors * nt->sectorSizeInBytes) - k < amountToRead)
				amountToRead = (unsigned long)(length * nt->clusterSizeInSectors * nt->sectorSizeInBytes - k);


			InterlockedIncrement(&issues);
			volreader->asyncread(buffer, amountToRead, &lpOv->ov);

			k += memory::pool::chunksize();

		}

	}

	fullyIssued = true;

	return output;
}


bool masterfiletable::waitToFinish()
{
	WaitForSingleObject(evtComplete, INFINITE);


	teardown();

	return true;
}