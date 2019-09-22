
#pragma once

#include <vector>

/*
	boyermoorehorspool implementation
*/
namespace bmh
{
	namespace binary
	{
		typedef unsigned __int64 word_t;
		typedef __int64 wordi_t;

		struct context;

		struct context
		{
			unsigned char * pattern;
			int	length;
			std::vector< wordi_t > d1;

			word_t	mod;
			word_t	mask;
			word_t	remainder;

			context() { pattern = NULL; length = 0; mod = mask = remainder = 0;  }
		};


		void init();
		void deinit();

		bool delta1(const unsigned char * pattern, const int length, context & ctx);

	}

}