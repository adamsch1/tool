

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define _FILE_OFFSET_BITS 64

// A document, has a url and a bunch of terms plus 
typedef struct {
	uint32_t id;
	time_t   time;
	uint32_t tcount;

	off_t hterms; // byte offsets in forward heap file
	off_t hurl;   

} document_t;

// Maintains forward index, which is a bunch of documents
typedef struct {
	FILE *file;    // Fixed size entries, the document_t go here
	FILE *heap;    // Variable sized entries, like url and terms go here
	uint64_t  count; 

	int dfd;
  document_t *daddr; // Mmap to document_ file
  size_t dsize;      // NUber of entries in this mmap of size document_t

	int hfd;
  char *haddr;  // mmap to heap file

	uint32_t last_id; // Last ide written N+1 id > last_id or error
} forward_t;


int forward_open( forward_t *forward, const char *file ) {
	char *h = alloca(strlen(file)+2);
	strcpy(h, file);
	strcat(h, "_");
	forward->file = fopen( file, "a+");
	forward->heap = fopen( h, "a+");
	forward->last_id = 0;

	return 0;
}

int forward_close( forward_t *forward ) {
	fclose(forward->file);
	fclose(forward->heap);

	return 0;
}

int forward_write( forward_t *forward, document_t *doc ) {
	if( doc->id < forward->last_id ) {
		return 0;
	}
	forward->last_id = doc->id;
	return fwrite( doc, sizeof( *doc ), 1, forward->file );
}

int document_set_url( document_t *doc, const char *url, FILE *heap ) {
	doc->hurl = ftello( heap );
	return fwrite( url, strlen(url)+1,1,heap);  
}

int document_set_terms( document_t *doc, uint32_t *terms, int count, FILE *heap ) {
	doc->hterms = ftello( heap );
  doc->tcount = count;
	return fwrite( terms, sizeof(uint32_t), count, heap );
}

document_t * forward_get_document_at( forward_t *forward, int k ) {
	if( k >= (int)forward->dsize ) return NULL;
	return forward->daddr + k;
}

// THe document has the byte offsets to our url in the heap file
char * forward_get_document_url( forward_t *forward, document_t *doc ) {
	return forward->haddr + doc->hurl;
}

// THe document has the byte offsets to our list of terms in the heap file
uint32_t * forward_get_document_terms( forward_t *forward, document_t *doc ) {
	return (uint32_t*)(forward->haddr + doc->hterms);
}

// mmap the forward index for reading [not writing]
int forward_mmap( forward_t *forward, const char *filename ) {
  struct stat st;

  forward->dfd = open( filename, O_RDONLY );
  if( forward->dfd == -1 ) return -1;

  if( fstat( forward->dfd, &st) == -1 ) {
    goto bail;
  }

  forward->daddr = (document_t *)mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, forward->dfd, 0 );
  if( forward->daddr == MAP_FAILED ) {
    goto bail;
  }

  forward->dsize = st.st_size / sizeof(document_t);

	char *h = alloca(strlen(filename)+2);
	strcpy(h, filename);
	strcat(h, "_");

  {
		forward->hfd = open( h, O_RDONLY );
		if( forward->hfd == -1 ) {
			goto bail;
		}

		if( fstat( forward->hfd, &st) == -1 ) {
			goto bail;
		}

		forward->haddr = (char *)mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, forward->hfd, 0 );
		if( forward->haddr == MAP_FAILED ) {
			goto bail;
		}
  }

  return 0;

bail:
	if( forward->dfd ) close(forward->dfd );
	if( forward->hfd ) close(forward->hfd );
	return -1;
}

// Hacked from wikipedia - does an interpolation search against the document_t file which 
// is keyed off the monotomically increasing document ID.  Each entry is fixed size so
// we can perform this search.  The random sized stuff is in a separate file to simplify
// finding documents after we write them. 
static document_t * interpolation_search (uint64_t key, document_t *base, size_t size ) {
    int low = 0;
    int high = size - 1;
    int mid;

    while (base[high].id != base[low].id && key >= base[low].id && key <= base[high].id) {
        mid = low + ((key - base[low].id) * (high - low) / (base[high].id - base[low].id));

        if (base[mid].id < key)
            low = mid + 1;
        else if (key < base[mid].id)
            high = mid - 1;
        else
            return &base[mid];
    }

    if (key == base[low].id)
        return &base[low] ;
    else
        return NULL;
}

document_t *forward_document_read( forward_t *forward, uint32_t id ) {
  return interpolation_search( id, forward->daddr, forward->dsize );
}

#if 0
int main() {
	forward_t ff = {0};
  document_t dd = {0};

	forward_open( &ff, "for.db");

	dd.id = 1234;
	dd.time = time(0);
	dd.tcount = 0;
	document_set_url( &dd, "http://www.mit.edu", ff.heap );

	uint32_t terms[] = { 1,2,3,4};
	document_set_terms( &dd, terms, 4, ff.heap );

	forward_write( &ff, &dd );

	dd.id = 1235;
	dd.time = time(0);
	dd.tcount = 0;
	document_set_url( &dd, "http://www.msu.edu", ff.heap );

	uint32_t terms2[] = { 1,2,3};
	document_set_terms( &dd, terms2, 3, ff.heap );

	forward_write( &ff, &dd );

	forward_close( &ff );	

	forward_mmap( &ff, "for.db");

  document_t *p;

  p = forward_document_read( &ff, 1234 );
  p = forward_document_read( &ff, 1235 );

	char *url = forward_get_document_url( &ff, p );
	uint32_t *pterms = forward_get_document_terms( &ff, p );
}
#endif
