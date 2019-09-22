#pragma once

#include <windows.h>
#include <stack>
#include "cs.h"


template <class of>
class pool
{
private:
	CRITICAL_SECTION c;
	std::stack<of *> s;
	volatile LONG outstanding;
public:
	pool()
	{
		outstanding = 0;
		InitializeCriticalSection(&c);
	}

	~pool()
	{
		while (outstanding > 0)
			Sleep(1000);

		{
			cs _(c);

			of * q = NULL;
			while (s.size() > 0)
			{
				q = s.top();
				s.pop();
				delete q;
			}
		}

		DeleteCriticalSection(&c);
		

	}

	of * get()
	{
		cs _(c);
		of * q = NULL;

		if (s.size() == 0)
			q = new of();
		else
		{
			q = s.top();
			s.pop();
		}

		outstanding++;	// ok, since we're in a cs
		
		return q;
	}

	void free(of * p)
	{
		cs _(c);
		s.push(p);
		
		outstanding--;	// ok, since we're in a cs
	}

};