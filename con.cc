
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>

#define MEGA (1024*1024llu)
#define MSIZE (MEGA*1024*2)
#define MOFFSET (MSIZE/sizeof(uint64_t))

#include <parallel/algorithm>
#include <algorithm>
#include <iterator>

#define PMODE 1

int icmp( const void *a, const void *b ) {
	return *(uint64_t*)a - *(uint64_t*)b;
}

int main( int argc, char **argv ) {

	uint64_t *buffer;
	uint64_t data;
	uint64_t k=0;
	uint64_t bcount=0;
	uint64_t mpercent = 50;
	uint64_t msize = 0;
	uint64_t moffset = 0;

	if( argc > 1 ) {
		if( argv[1][0] == '%' )  {
			uint64_t size = atoi( &argv[1][1] );
			if( size > 0 && size < 100 ) mpercent = size;  
		}
	}

	struct sysinfo info = {0};
	sysinfo( &info );
	msize = (uint64_t)(info.totalram * mpercent / 100.0);
	while( (buffer= (uint64_t*)malloc( msize )) == NULL ) {
		msize /= 2;
	}

	moffset = msize / sizeof(uint64_t);

	while( fread( &data, sizeof(data),1, stdin) > 0)  {
		buffer[k++] = data;
		if( k == moffset ) {
			bcount++;
#ifdef PMODE
//			__gnu_parallel::sort( buffer, buffer + k );
			std::sort( buffer, buffer + k );
#else 
			qsort( buffer, moffset, sizeof(uint64_t), icmp );
#endif
			fwrite( buffer, MSIZE, 1, stdout );
			k = 0;
		}
	}
	printf("%ld %llu %llu\n",bcount, MSIZE, MOFFSET);
}

