/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "cs.h"

cs::cs(CRITICAL_SECTION &c):_c(c)
{
	EnterCriticalSection(&_c);
}

cs::~cs()
{
	LeaveCriticalSection(&_c);
}