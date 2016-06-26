#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <getopt.h>

#include "tool.h"

void dump( forward_t *ff ) {
	for( size_t k =0; k<ff->dsize; k++ ) {
		document_t *doc = forward_get_document_at( ff, k );
		uint32_t *terms  = forward_get_document_terms( ff, doc );
		for( size_t j=0; j<doc->tcount; j++ ) {
			uint64_t pair = terms[j];
		  pair = (pair << 32) | doc->id;
			fwrite( &pair, sizeof(pair), 1, stdout );
		}
	}
}

int main( int argc, char **argv) {

	glob_t bglob = {0};
	char *pattern = NULL;
	int c;

	while((c=getopt(argc, argv, "p:")) != -1 ) {
		switch(c) {
			case 'p':
				pattern = strdup(optarg);
			break;
		}
	}
	if( !pattern ) {
		fprintf( stderr, "Must specify wildcard pattern for files\n");
		return EXIT_FAILURE;
	}
	
	glob( pattern, 0, NULL, &bglob );	

	forward_t ff;
	for( size_t k=0; k<bglob.gl_pathc; k++ ) {
		if( forward_mmap( &ff, bglob.gl_pathv[k] ) ) {
			fprintf( stderr, "Could not open file: %s\n", bglob.gl_pathv[k]);
		}

		dump( &ff );
	}

}


