/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "volumereader.h"
#include "threadpool.h"
#include "cs.h"

extern threadpool tp;

#define READ_KEY		500

class volumereader : public reader
{
private:
	HANDLE h;
	HANDLE h2;

	CRITICAL_SECTION synccs;

	HANDLE event;

	static int WorkerFunction(DWORD transferred, ULONG_PTR key, LPOVERLAPPED lpOv, BOOL cancel)
	{
		IOCPOVERLAPPED * lpc = CONTAINING_RECORD(lpOv, IOCPOVERLAPPED, ov);
		volumereader * r = (volumereader*)lpc->p;
		lpc->sqn = transferred;
		SetEvent(r->event);
		return 0;
	}
public:

	volumereader()
	{
		h = INVALID_HANDLE_VALUE;
		h2 = INVALID_HANDLE_VALUE;
		event = NULL;
		InitializeCriticalSection(&synccs);
	}

	~volumereader()
	{
		deinit();
	}

	bool init(std::wstring const volumePath)
	{
		h = CreateFile(volumePath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING, 0);

		h2 = CreateFile(volumePath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);

		// used for blocking reads
		if (h != INVALID_HANDLE_VALUE)
		{
			event = CreateEvent(NULL, FALSE, FALSE, NULL);
			tp.attach(h, READ_KEY);
		}


		return h != INVALID_HANDLE_VALUE;
	}

	void deinit()
	{
		if (event)
			CloseHandle(event);

		if (h != INVALID_HANDLE_VALUE)
			CloseHandle(h);

		if (h2 != INVALID_HANDLE_VALUE)
			CloseHandle(h2);

		h = INVALID_HANDLE_VALUE;
		h2 = INVALID_HANDLE_VALUE;

		DeleteCriticalSection(&synccs);
	}

	virtual unsigned long asyncread(void * buffer, unsigned long size, LPOVERLAPPED lpOverlapped)
	{
		unsigned long amountRead = 0;
		if (!ReadFile(h, buffer, size, &amountRead, lpOverlapped))
		{
			amountRead = 0;
		}

		return amountRead;
	}

	virtual unsigned long read(void * buffer, unsigned long size, LARGE_INTEGER offset)
	{
		cs _(synccs);
		unsigned long amountRead = 0;
		SetFilePointerEx(h2, offset, NULL, FILE_BEGIN);
		if (!ReadFile(h2, buffer, size, &amountRead, 0))
		{
			amountRead = 0;
		}

		return amountRead;
	}
};

reader * createvolumereader(std::wstring const volumename)	// like C:
{
	volumereader * vr = new volumereader();
	if (vr->init(L"\\\\.\\" + volumename))
		return vr;
	else
	{
		delete vr;
		return NULL;
	}
}

void freevolumereader(reader * r)
{
	volumereader * vr = static_cast<volumereader *>(r);
	vr->deinit();
	delete vr;
}