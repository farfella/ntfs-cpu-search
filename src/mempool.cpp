/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "mempool.h"
#include "cs.h"
#include "ods.h"

#include <stack>
#include <vector>
#include <cassert>
#include <windows.h>


namespace memory
{
	namespace pool
	{
		unsigned int allocsz;
		CRITICAL_SECTION c;

		LONG maxoutstanding;
		volatile LONG outstanding;

		std::stack<void *> s;

		void * alloc();
	}
}

void memory::pool::init(unsigned int sz, unsigned int init, long maxOutstanding)
{
	InitializeCriticalSection(&c);

	maxoutstanding = maxOutstanding;

	allocsz = (sz % 4096) == 0 ? sz : sz + (4096 - sz % 4096);

	std::vector<void *> allocs;

	for (unsigned int i = 0; i < init; ++i)
		allocs.push_back(alloc());	// ok... alloc() will sleep until it succeeds...

	for (std::vector<void*>::size_type j = 0; j != allocs.size(); ++j)
		free(allocs[j]);

}


void memory::pool::deinit()
{
	while (outstanding > 0)
		Sleep(250);

	{
		cs _(c);

		void * q = NULL;
		while (s.size() > 0)
		{
			q = s.top();
			s.pop();
			VirtualFree(q, 0, MEM_RELEASE);
		}
	}

	DeleteCriticalSection(&c);
}

void * memory::pool::alloc()
{
	cs _(c);

	return VirtualAlloc(NULL, allocsz, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void * memory::pool::get()
{
	/*while (outstanding > maxoutstanding)
		Sleep(1000);*/

	{
		cs _(c);
		void * q = NULL;

		if (s.size() == 0)
		{
		
			q = alloc();
			while (q == NULL)
			{
				Sleep(1000);
				q = alloc();
			}
		}
		else
		{
			q = s.top();
			s.pop();
		}

		InterlockedIncrement(&outstanding);

		assert(q != NULL);

		return q;
	}

	
}

void memory::pool::free(void * p)
{
	assert (p != NULL);

	cs _(c);
	s.push(p);
	InterlockedDecrement(&outstanding);


}

unsigned int memory::pool::chunksize()
{
	return allocsz;
}

int memory::pool::livecount()
{
	return outstanding;
}