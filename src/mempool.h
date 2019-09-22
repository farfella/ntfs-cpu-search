#pragma once

namespace memory
{
	namespace pool
	{
		void init(unsigned int sz, unsigned int init, long maxOutstanding);	// allocation size, initial count
		void deinit();

		unsigned int chunksize();

		void * get();
		void free(void * p);


		int livecount();
	}
}