/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "parallelfhp.h"
#include "delta1.h"

#include <emmintrin.h>

namespace boyermoore
{
	namespace horspool
	{
		const int REGISTER_SIZE = sizeof(__m128i);
		namespace parallel
		{
			__m128i masks[REGISTER_SIZE];
			
			void init();
		}
	}
}

void boyermoore::horspool::parallel::init()
{
	unsigned char nmask[REGISTER_SIZE] = {0};


	for (int i = 0; i < REGISTER_SIZE; ++i)
	{
		masks[i] = _mm_set_epi8(nmask[15], nmask[14], nmask[13], nmask[12], 
								nmask[11], nmask[10], nmask[9], nmask[8], 
								nmask[7], nmask[6], nmask[5], nmask[4], 
								nmask[3], nmask[2], nmask[1], nmask[0]);
		
		nmask[i] = ~0;

	}
}

std::list<std::string::size_type>  boyermoore::horspool::parallel::find(const std::string & pattern, const std::string & inText)
{
	std::list<std::string::size_type> offsets;

	__m128i pat, txt, xor;

	std::vector<wordi_t> d1 = delta1(pattern);

	const wordi_t m = pattern.length();
	const wordi_t n = inText.length();
	
	const wordi_t mod = m % REGISTER_SIZE;

	wordi_t j = 0;
	wordi_t i;

	if (mod == 0)
	{
		for (; j <= n - m; j += d1 [ inText [ j + m - 1 ] ])
		{
			i = m - REGISTER_SIZE;

			for ( ; i >= 0; i -= REGISTER_SIZE )
			{
				pat = _mm_loadu_si128((__m128i*)(pattern.c_str() + i));
				txt = _mm_loadu_si128((__m128i*)(inText.c_str() + i + j));
				xor = _mm_xor_si128(pat, txt);

				if  (xor.m128i_u64[0] != 0 || xor.m128i_u64[1] != 0)
					break;
			}

			if (i < 0)
			{
				offsets.push_back(j);
				//j += d1[ inText [ j + m - 1 ] ];
			}
		}
	}
	else
	{
		__m128i mask = masks[mod];
		__m128i remainder = _mm_setzero_si128();

		char buf[REGISTER_SIZE] = {0};
		

		for (i = 0; i < mod; ++i)
			remainder.m128i_u8[i] = pattern[i];

		for (; j <= n - m; j += d1 [ inText [ j + m - 1 ] ])
		{
			i = m - REGISTER_SIZE;

			for (; i >= 0; i -= REGISTER_SIZE)
			{
				pat = _mm_loadu_si128((__m128i*)(pattern.c_str() + i));
				txt = _mm_loadu_si128((__m128i*)(inText.c_str() + i + j));
				xor = _mm_xor_si128(pat, txt);

				if  (xor.m128i_u64[0] != 0 || xor.m128i_u64[1] != 0)
					break;
			}

			if (i < 0) // was i < register_size...
			{
				// think a little here... what if inText + j + 16 > tlen?
				if (j + REGISTER_SIZE > (signed)n) // then use buf from above...
				{
					memcpy(buf, inText.c_str() + j, (size_t)mod);
					txt = _mm_loadu_si128( (__m128i*) (buf));
				}
				else
					txt = _mm_loadu_si128( (__m128i*) (inText.c_str() + j ));

				__m128i and = _mm_and_si128(txt, mask);
				xor = _mm_xor_si128(and, remainder);

				if (xor.m128i_u64[0] == 0 && xor.m128i_u64[1] == 0)
				{
					offsets.push_back((size_t)j);
					//j += d1[ inText [ j + m - 1 ] ];
				}

			}

		}
	}
		
	return offsets;
}