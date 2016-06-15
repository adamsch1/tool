#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>

#include <algorithm>
#include <iterator>

int dumper( char *tmplate, char *buffer, uint64_t blength ) {
	FILE *outs;
	int fd;
	char *dupe = strdup(tmplate);

	if( blength == 0 ) {
		return 0;
	}

	fd = mkstemp( dupe );
	if( fd == -1 ) {
		//error("Could not open output file");
	}

	outs = fdopen( fd, "r+b");
	fwrite( buffer, blength, 1, outs );

	fclose( outs );
	free( dupe );

	return 0;
}

int main( int argc, char **argv ) {

	uint64_t *buffer;
	uint64_t data;
	uint64_t k=0;
	uint64_t mpercent = 50;
	uint64_t msize = 0;
	uint64_t moffset = 0;
	char * tmplate = strdup("con_XXXXXX");

	// Only argument is %X where X is 1-100 specifing how much system memory to use
	if( argc > 1 ) {
		if( argv[1][0] == '%' )  {
			uint64_t size = atoi( &argv[1][1] );
			if( size > 0 && size < 100 ) mpercent = size;  
		}

		if( argc > 2 ) {
			// I lied, also output template
			tmplate = argv[2];
		}
	}

	// Calculate how much memory we have and the nallocate mpercent of that for our use
	struct sysinfo info = {0};
	// Not portable but meh.
	sysinfo( &info );
	msize = (uint64_t)(info.totalram * mpercent / 100.0);
	while( (buffer= (uint64_t*)malloc( msize )) == NULL ) {
		// In case we cannot allocate fallback half
		msize /= 2;
	}

	// Max number of entries given size of our buffer
	moffset = msize / sizeof(uint64_t);

	while( fread( &data, sizeof(data),1, stdin) > 0)  {
		buffer[k++] = data;
		if( k == moffset ) {
			// I found std::sort to be fastest on my system, hence g++
			std::sort( buffer, buffer + k );
			dumper( tmplate, (char*)buffer, k * sizeof(uint64_t));
			k = 0;
		}
	}
	dumper( tmplate, (char*)buffer, k * sizeof(uint64_t));
	free(tmplate);
}

