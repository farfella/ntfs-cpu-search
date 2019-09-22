/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "threadpool.h"
#include "ods.h"
#include "gle.h"
#include <vector>

const ULONG_PTR terminatekey = -1;

DWORD WINAPI Worker(LPVOID param)
{
	ULONG_PTR key;

	HANDLE CompletionPort = param;

	unsigned int id = GetCurrentThreadId();

	do
	{
		DWORD transferred;
		LPOVERLAPPED lpOv;

		if (GetQueuedCompletionStatus(CompletionPort, &transferred, &key, &lpOv, INFINITE))
		{
			if (key != terminatekey)
			{
				IOCPOVERLAPPED * lpc = CONTAINING_RECORD(lpOv, IOCPOVERLAPPED, ov);
				lpc->threadId = id;
				lpc->fn(transferred, key, lpOv, FALSE);
			}
		}
		else
		{
			DWORD const le = gle();
			if (le == ERROR_OPERATION_ABORTED)
			{
				IOCPOVERLAPPED * lpc = CONTAINING_RECORD(lpOv, IOCPOVERLAPPED, ov);
				lpc->threadId = id;
				lpc->fn(0, key, lpOv, TRUE);
			}
			else
				ods("GQCS: error %d\n", le);
		}

	} while (key != terminatekey);

	return NOERROR;
}

threadpool::threadpool()
{
	iocp = NULL;
}

bool threadpool::init(unsigned int n)
{
	DWORD le = 0;

	if (n <= 64)
	{
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, n);
		
		if (iocp != NULL)
		{
			for (unsigned int i = 0; i < n; ++i)
			{
				HANDLE t = CreateThread(NULL, 0, Worker, iocp, 0, 0);
				if (t != NULL)
					threads.push_back(t);
				else
				{
					le = gle();
					ods("thread::pool::init: CT failed, le = %d\n", le);
					break;
				}
			}
		}
		else
		{
			le = gle();
			ods("tpi: CICP failed, gle=%d", le);
		}
	}
	else
	{
		ods("thread::pool::init: n > 64 not allowed.\n");
		le = ERROR_INVALID_PARAMETER;
	}

	return le == 0;
}

bool threadpool::attach(HANDLE f, ULONG_PTR key)
{
	DWORD le = 0;
	if (key == terminatekey)
	{
		ods("thread::pool::attach: cannot use key %u\n", key);
		le = ERROR_INVALID_PARAMETER;
	}
	else
	{
		if (!CreateIoCompletionPort(f, iocp, key, NULL))
		{
			le = gle();
			ods("thread::pool::atach: CIoCP failed, le=%d\n", le);
		}
	}

	return le == 0;
}

bool threadpool::post(DWORD transferred, ULONG_PTR key, LPOVERLAPPED lpOverlapped)
{
	return PostQueuedCompletionStatus(iocp, transferred, key, lpOverlapped) != 0;
}

void threadpool::deinit()
{
	DWORD le = 0;

	// push termination
	for (std::vector<HANDLE>::size_type s = 0; s < threads.size(); ++s)
		if (!PostQueuedCompletionStatus(iocp, 0, terminatekey, NULL))
		{
			le = gle();
			ods("thread::pool::deinit: PQCS failed, le=%d\n", le);
		}

	// wait for all for 10 seconds
	DWORD wait = WaitForMultipleObjects(threads.size(), &threads[0], TRUE, 10*1000);
	if (wait == WAIT_TIMEOUT)
		ods("tpd: WFMO timedout\n");
	else if (wait == WAIT_FAILED)
	{
		le = gle();
		ods("tpd: WFMO failed, gle=%d", le);
	}

	for (std::vector<HANDLE>::size_type s = 0; s < threads.size(); ++s)
		CloseHandle(threads[s]);

	CloseHandle(iocp);
}