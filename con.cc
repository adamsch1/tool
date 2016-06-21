#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>

extern "C" {
  #include "util.h"
}

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

char tmp[255];

int main( int argc, char **argv ) {

	uint64_t *buffer;
	uint64_t data;
	uint64_t k=0;
	uint64_t mpercent = 50;
	uint64_t msize = 0;
	uint64_t moffset = 0;
	uint64_t size = 0;
	int c;
	char * tmplate = NULL;

	while((c = getopt(argc, argv, "m:o:")) != -1 ) {
		switch(c) {
			case 'm':
				size = atoi( optarg );
				if( size > 0 && size < 100 ) mpercent = size;  
				break;
			case 'o':
				tmplate = strdup(optarg);
				break;
		}
	}
	if( tmplate == NULL ) tmplate = strdup("con_XXXXXX");

	// Calculate how much memory we have and the nallocate mpercent of that for our use
	
	msize = ramsize() * mpercent / 100.0;
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

