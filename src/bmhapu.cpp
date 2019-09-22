/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "cs.h"
#include "bmhapu.h"



#include <map>

namespace bmh
{
	namespace binary
	{
		const int REGISTER_SIZE = sizeof(wordi_t);
		wordi_t masks[REGISTER_SIZE];

		const int ALPHABET_SIZE = 256;

		CRITICAL_SECTION crit;
		std::map<unsigned int, int> kernels;
	}

}

void bmh::binary::init()
{
	InitializeCriticalSection(&crit);

	masks[0] = 0;
	
	for (int i = 1; i < REGISTER_SIZE; ++i)
		masks[i] = (masks[i-1] << 8) | 0xff;


}

void bmh::binary::deinit()
{
	DeleteCriticalSection(&crit);
}


bool bmh::binary::delta1(const unsigned char * pattern, const int length, context & ctx)
{
	bool ret = false;

	if (length > 0)
	ctx.pattern = (unsigned char *)malloc(length + 1);
	else
		ctx.pattern = NULL;

	if (ctx.pattern)
	{
		memcpy(ctx.pattern, pattern, length + 1);
		ctx.length = length;

		try
		{
			std::vector<wordi_t> badshift(ALPHABET_SIZE, length);
		
			for ( wordi_t i = 0 ; i < (wordi_t)length - 1 ; ++i )
				badshift[ pattern[ i ] ] = length - i - 1;

			ctx.d1 = badshift;
		}
		catch (...)
		{
		}

		// if len(d1) == 0, then we failed at allocating badshift,
		// so just return false. Otherwise, fix up.
		if (ctx.d1.size() != 0)
		{
			ctx.remainder = 0;
			ctx.mod = length % REGISTER_SIZE;
			if (ctx.mod != 0)
			{
				ctx.mask = masks[ctx.mod];
				unsigned char remainderc[REGISTER_SIZE] = {0};

				for (word_t i = 0; i < ctx.mod; ++i)
					remainderc[i] = pattern[i];

				ctx.remainder = *(word_t*)(remainderc);
			}
		}

		ret = true;
	}


	return ret;
}
