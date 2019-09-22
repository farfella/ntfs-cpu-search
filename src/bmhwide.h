#pragma once

#include <vector>

namespace bmh
{
	namespace wide
	{
		namespace ci
		{


			void init();

			struct context;

			struct context
			{
				wchar_t * pattern;
				unsigned int	length;
				std::vector< int > d1;
				const int * lower;

				context();
				~context() { free(pattern); }
			};

			bool delta1(const wchar_t * pattern, const unsigned int length, context & ctx );

		}
	}
}