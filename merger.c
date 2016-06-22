#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <getopt.h>

typedef struct {
	FILE *in;
	uint64_t entry;
} ifile_t;


void ifile_init( ifile_t *file ) {
	memset( file, 0, sizeof(*file));
}

// Read next tuple (term, doc) from the index.
int ifile_read( ifile_t *file ) {
	return fread( &file->entry, sizeof(file->entry), 1, file->in ) != 1;
}

// Close file, release memory
void ifile_close( ifile_t *file ) {
	fclose( file->in );
}

void ifile_write( ifile_t *file, uint64_t entry ) {
	fwrite( &entry, sizeof(entry), 1, file->in );
}

// Write what we have in our buffer to disk
void ifile_flush( ifile_t *file ) {
	fflush(file->in);
}

static inline int compare_ifile( const void *va, const void *vb ) {
	const ifile_t *a = *(ifile_t **)va;
	const ifile_t *b = *(ifile_t **)vb;

	return (a->entry - b->entry);
}

// Merge multiple sorted ifiles.  This is a O(N*M) operation where M is the numer of 
// files.
static void merge( ifile_t **files, size_t nfiles, ifile_t *outs ) {
  size_t k;

	ifile_t *temp;

	// Read in initial tuple for each ifile
	for( k=0; k<nfiles; k++ ) {
		if( ifile_read( files[k] ) ) {
			// EOF remove file
			ifile_close( files[k] );
			memmove( &files[k], &files[k+1], nfiles-k-1*sizeof(ifile_t*));
			nfiles--;
		}
	}

	qsort( files, nfiles, sizeof(ifile_t *), compare_ifile ); 

	while( nfiles ) {
		ifile_t *f = files[0];

		// Write lowest tuple to output
		ifile_write( outs, f->entry );

		// Read next tuple for this file
		if( ifile_read( f ) ) {
			// EOF case
			temp = f;
			// Move above it down one in the array [the delete]
			memmove( &files[0], &files[1], (nfiles-1)*sizeof(ifile_t*));
			nfiles--;
			// Copy the dude we just deleted to the end of the array
			files[nfiles] = temp;
			ifile_close( temp );
		} else if( nfiles == 1 ) {
			continue;
		} else  {
			// Binary search to see where this file should be inserted as it's tupe changed
			size_t lo = 1;
			size_t hi = nfiles;
			size_t probe = lo;
			size_t count_of_smaller_lines;

			while (lo < hi) {
				int cmp = compare_ifile( files[0], files[probe]);
				if (cmp < 0 || cmp == 0 )  {
				  hi = probe;
				} else {
				  lo = probe + 1;
				}
				probe = (lo + hi) / 2;
			}

			count_of_smaller_lines = lo - 1;

			// Preserve the one we are moving
			temp = files[0];
			// Copy everything down up to the point of insertion
			memmove( &files[0], &files[1], count_of_smaller_lines*sizeof(ifile_t*));
			// Insert or guy
			files[count_of_smaller_lines] = temp;
		}

	}

	ifile_flush( outs );
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

	ifile_t **pfiles = malloc( bglob.gl_pathc* sizeof(ifile_t*));
	int count=0;
	for( size_t k=0; k<bglob.gl_pathc; k++, count++ ) {
		pfiles[k] = malloc(sizeof(ifile_t));
		ifile_init( pfiles[k] );
		pfiles[k]->in = fopen( bglob.gl_pathv[k], "rb");
	}

	ifile_t out;
	ifile_init(&out);
	out.in = fopen("output", "wb");
	merge( pfiles, count, &out);
}


