
#pragma once

#include <vector>
#include <string>

namespace boyermoore
{
	typedef unsigned __int64 word_t;
	typedef __int64 wordi_t;

	const int ALPHABET_SIZE = 256;

	std::vector<wordi_t> delta1(const std::string & pattern);

	std::vector<wordi_t> delta1(const unsigned char * pattern, const word_t  length);

	
}