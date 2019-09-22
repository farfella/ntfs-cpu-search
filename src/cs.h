#pragma once

#include <windows.h>

class cs
{
public:
	cs(CRITICAL_SECTION & c);
	~cs();
private:
	CRITICAL_SECTION & _c;
};
