#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>

#include "util.h"

uint64_t * _bsearch (uint64_t key, uint64_t *base, size_t nmemb ) {
  size_t l, u, idx;
  uint64_t *p;
  int comparison;

  l = 0;
  u = nmemb;

  int n = 0;
  while (l < u) {
		++n;
		idx = (l + u) / 2;
		p = base + idx;
		comparison = key - *p;
		if (comparison < 0) { 
			u = idx;
		} else if (comparison > 0) {
			l = idx + 1;
		} else {
			return p;
		}

  }

  return NULL;
}

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main( int argc, char **argv ) {

	struct stat st;
	int c;
	int fd;
	uint64_t *addr;
	size_t    size;
	uint64_t term=0;

	while((c = getopt(argc, argv, "f:t:")) != -1 ) {
		switch(c) {
			case 'f':
				fd = open( optarg, O_RDONLY );
				if( fd == -1 ) {
					handle_error( "open" );					
        }
				break;
			case 't':
				term = atoll( optarg );
				break;
		}
	}

	if( fstat( fd, &st) == -1 ) {
 		handle_error( "stat" );
	}

	addr = (uint64_t *)mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
	if( addr == MAP_FAILED ) {
    handle_error( "mmap" );
	}
	size = st.st_size / sizeof(*addr);

	printf("Size: %lu %lu\n", size, term );

	clock_t tic = clock();
	int max = 10000;
	int fcount = 0;
	for( int k=0; k<max; k++ ) {
		uint64_t *r = _bsearch( term, addr, size );
		fcount += r != NULL;
	}
	clock_t toc = clock();
	double res = toc-tic;
	res /= CLOCKS_PER_SEC;
	printf("Took: %g seconds %d\n", res, fcount );
	return EXIT_SUCCESS;
}

