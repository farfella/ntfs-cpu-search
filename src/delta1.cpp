/*
Copyright (c) Ateeq Sharfuddin. All rights reserved.
Contact author to license for commercial purposes.
*/

#include "delta1.h"

namespace boyermoore
{
	std::vector<wordi_t> delta1(const std::string & pattern)
	{
		const std::string::size_type m = pattern.length();
		
		// create the delta1 vector, initially setting all elements to
		// pattern.length().
		std::vector<wordi_t> badshift(ALPHABET_SIZE, m);
		
		// update table for characters in the pattern
		for ( std::string::size_type i = 0 ; i < m - 1 ; ++i )
			badshift[ (std::string::size_type)pattern[ i ] ] = m - i - 1;
		
		return badshift;
	}

	std::vector<wordi_t> delta1(const unsigned char * pattern, word_t const length)
	{
		const word_t m = length;
		
		std::vector<wordi_t> badshift(ALPHABET_SIZE, m);
		
		for ( word_t i = 0 ; i < m - 1 ; ++i )
			badshift[ pattern[ i ] ] = m - i - 1;
		
		return badshift;
	}
	
}