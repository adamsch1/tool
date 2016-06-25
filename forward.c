

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

typedef struct {
	uint32_t id;
	time_t   time;
	uint32_t tcount;

	off_t hterms;
	off_t hurl;

} document_t;

typedef struct {
	FILE *file;
	FILE *heap;
	uint64_t  count;

  document_t *daddr;
  size_t dsize;

  char *haddr; 

	uint32_t last_id;
} forward_t;


int forward_open( forward_t *forward, const char *file ) {
	char *h = alloca(strlen(file)+2);
	strcpy(h, file);
	strcat(h, "_");
	forward->file = fopen( file, "a+");
	forward->heap = fopen( h, "a+");
	forward->last_id = 0;
}

int forward_close( forward_t *forward ) {
	fclose(forward->file);
	fclose(forward->heap);
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
//	fwrite( &count, sizeof(count), 1, heap );
	return fwrite( terms, sizeof(uint32_t), count, heap );
}

char * forward_get_document_url( forward_t *forward, document_t *doc ) {
	return forward->haddr + doc->hurl;
}

uint32_t * forward_get_document_terms( forward_t *forward, document_t *doc ) {
	return (uint32_t*)(forward->haddr + doc->hterms);
}

int forward_mmap( forward_t *forward, const char *filename ) {
  struct stat st;

  int fd = open( filename, O_RDONLY );
  if( fd == -1 ) return -1;

  if( fstat( fd, &st) == -1 ) {
    return -1;
  }

  forward->daddr = (document_t *)mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
  if( forward->daddr == MAP_FAILED ) {
    return -1;
  }

  forward->dsize = st.st_size / sizeof(document_t);

	char *h = alloca(strlen(filename)+2);
	strcpy(h, filename);
	strcat(h, "_");

  {
		int fd = open( h, O_RDONLY );
		if( fd == -1 ) return -1;

		if( fstat( fd, &st) == -1 ) {
			return -1;
		}

		forward->haddr = (char *)mmap( NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
		if( forward->haddr == MAP_FAILED ) {
			return -1;
		}
  }

  return 0;
}

document_t * interpolation_search (uint64_t key, document_t *base, size_t size ) {
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

document_t *forward_read( forward_t *forward, uint32_t id ) {
  return interpolation_search( id, forward->daddr, forward->dsize );
}

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

  p = forward_read( &ff, 1234 );
  p = forward_read( &ff, 1235 );

	char *url = forward_get_document_url( &ff, p );
	uint32_t *pterms = forward_get_document_terms( &ff, p );
}
