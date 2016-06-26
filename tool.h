#ifndef __CHUNK__H
#define __CHUNK__H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

int forward_mmap( forward_t *forward, const char *filename );
document_t * forward_get_document_at( forward_t *forward, int k );
char * forward_get_document_url( forward_t *forward, document_t *doc );
uint32_t * forward_get_document_terms( forward_t *forward, document_t *doc );

#endif
