#pragma once

#include <windows.h>

class reader
{
public:
	virtual unsigned long read(void * buffer, unsigned long size, LARGE_INTEGER offset) = 0;
	virtual unsigned long asyncread(void * buffer, unsigned long size, LPOVERLAPPED lpOverlapped) = 0;
};
