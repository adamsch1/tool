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

typedef struct {
	int fd;
	uint64_t *base;
	size_t size;
} inv_t;

// Binary search hacked from libc.
inline uint64_t * _bsearch (uint64_t key, uint64_t *base, size_t nmemb ) {
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

int inv_init( inv_t *inv, const char *filename ) {
	struct stat st;

	memset(inv, 0, sizeof(*inv));

	inv->fd = open( filename, O_RDONLY );
	if( inv->fd == -1 ) return -1;

	if( fstat( inv->fd, &st) == -1 ) {
		return -1;
	}

	inv->base = (uint64_t *)mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, inv->fd, 0 );
	if( inv->base == MAP_FAILED ) {
		return -1;
	}

	inv->size = st.st_size / sizeof(*inv->base);

	return 0;
}

inline uint64_t * inv_search( inv_t *inv, uint64_t key ) {
	return _bsearch( key, inv->base, inv->size );
}

int main( int argc, char **argv ) {

	int c;
	uint64_t term=0;

	inv_t inv;

	while((c = getopt(argc, argv, "f:t:")) != -1 ) {
		switch(c) {
			case 'f':
				inv_init( &inv, optarg );
				break;
			case 't':
				term = atoll( optarg );
				break;
		}
	}

	clock_t tic = clock();
	int max = 10000;
	int fcount = 0;
	for( int k=0; k<max; k++ ) {
//		uint64_t *r = _bsearch( term, inv.base, inv.size );
    uint64_t *r = inv_search( &inv, term );
		fcount += r != NULL;
	}
	clock_t toc = clock();
	double res = toc-tic;
	res /= CLOCKS_PER_SEC;
	printf("Took: %g seconds %d\n", res, fcount );
	return EXIT_SUCCESS;
}

