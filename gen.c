#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define BUFF

#define BILLION (1024*1024*1024ull)
#define TRILLION (BILLION*1024ull)
#define MAX ( BILLION*1)
int main() {
	uint64_t k;
  int rc;
	for( k=0;k<MAX; k++ ) {
#ifdef BUFF
		rc = fwrite( &k, sizeof(k), 1, stdout );
#else
		rc = write( STDOUT_FILENO, &k, sizeof(k));
#endif
	}

}
