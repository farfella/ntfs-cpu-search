#pragma once

#include <windows.h>

#include <vector>

typedef int (* ThreadWorkerFunction)(DWORD transferred, ULONG_PTR key, LPOVERLAPPED ov, BOOL cancel);

struct IOCPOVERLAPPED
{

	OVERLAPPED				ov;
	ThreadWorkerFunction	fn;
	const void *			p;
	HANDLE					evt;
	unsigned int			sqn;
	void *					b;
	unsigned int			threadId;
	unsigned int			reserved;
	unsigned int			reserved2;

};

class threadpool
{
public:
	threadpool();
	bool init(unsigned int n);
	bool attach(HANDLE file, ULONG_PTR key);
	bool post(DWORD transferred, ULONG_PTR key, LPOVERLAPPED lpOverlapped);
	void deinit();

private:
	std::vector<HANDLE> threads;
	HANDLE iocp;
};