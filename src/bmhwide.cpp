/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "bmhwide.h"

namespace bmh
{
	namespace wide
	{
		const int ALPHABET_SIZE = 65536;
		const int BASE_ALPHABET_SIZE = 128;
		
		

		namespace ci
		{
			int tolower[ALPHABET_SIZE];

			context::context():lower(tolower) { pattern = NULL; length = 0; }
		}
		

	}

}

void bmh::wide::ci::init()
{
	for (int i = 0; i < ALPHABET_SIZE; ++i)
		tolower[i] = i;

	for (int i = 'A'; i < 'Z'; ++i)
		tolower[i] = i - 'A' + 'a';
}


bool bmh::wide::ci::delta1(const wchar_t * pattern, const unsigned int length, context & ctx)
{
	bool ret = false;

	if (length > 0)
		ctx.pattern = (wchar_t *)malloc(length * 2 + 1);
	else
		ctx.pattern = 0;

	if (NULL != ctx.pattern)
	{
		ctx.length = length;

		std::vector< int > badshift(ALPHABET_SIZE, length);

		for (unsigned int i = 0; i < length; ++i)
			ctx.pattern [ i ] = tolower[ pattern [i] ];
		ctx.pattern[length] = 0;

		for ( unsigned int i = 0 ; i < length - 1 ; ++i )
		{
			if (i < BASE_ALPHABET_SIZE)
				badshift [ tolower [ pattern [ i ] ] ] = length - i - 1;
		
			badshift[ pattern[ i ] ] = length - i - 1;

			ctx.pattern [ i ] = tolower[ pattern [i] ];
		}

		

		ctx.d1 = badshift;

		ret = true;
	}

	return ret;

}
