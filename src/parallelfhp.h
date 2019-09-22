/*
Copyright (c). Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#pragma once

#include <list>
#include <string>
#include "delta1.h"



namespace boyermoore
{
	namespace horspool
	{
		namespace parallel
		{
			void init();
			std::list<std::string::size_type>  find(const std::string & pattern, const std::string & inText);
		}
	}
}