/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "ods.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

#define DEVICE_DEBUG_PORT	1
#define DEVICE_FILE			2
#define DEVICE_CONSOLE		4
#define DEVICE_MESSAGE		8

unsigned long devices = 8;
FILE * logfile;

void ods(char const * fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buffer[4096];

	DWORD tid = GetCurrentThreadId();
	DWORD pid = GetCurrentProcessId();
	
	SYSTEMTIME systime;
	GetLocalTime(&systime);

	vsnprintf_s(buffer, _TRUNCATE, fmt, args);

	if (devices & DEVICE_DEBUG_PORT)
		OutputDebugStringA(buffer);

	if (devices & DEVICE_FILE)
		fprintf(logfile, "[%d.%d] %d/%d/%d %.2d:%.2d:%.2d.%.3d %s", pid, tid, systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds, buffer);

	if (devices & DEVICE_CONSOLE)
		fprintf(stdout, "[%d.%d] %d/%d/%d %.2d:%.2d:%.2d.%.3d %s", pid, tid, systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds, buffer);
	
	if (devices & DEVICE_MESSAGE)
		MessageBoxA(NULL, buffer, "NTFS Search", MB_OK);

}