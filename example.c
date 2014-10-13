#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "tool.h"


struct doc_t {
  char data[256];
  int  id;
} documents[] = {
  {"appl", 8},
  {"the", 9},
  {"the", 10}
};
/*  { "The 45 year old man ate the apple", 1 },
  { "The man ate an orange", 2 },
  { "I spy an orange and apple", 3 }
};*/

#define DCOUNT() ( sizeof(documents)/sizeof(struct doc_t))

int term_clean( char *term ) {
  char *p = term;

  // Lower case verything
  while( *p ) {
    *p = tolower( *p );
    p++;
  }

  // Skip numbers crap version
  p = term;
  while( *p && isdigit(*p) ) { p++; }
  if( *p ) {
    return 1;
  } else {
    return 0;
  } 
}

struct iter_t {
  struct run_t *run;
  struct chunk_t *chunk;
  struct field_t *field;
  int k;
  int j;
  int chunk_offset;
  int done;
};

int iter_first( struct iter_t *it, int offset ) {
  it->k = 0;
  it->j = 0;
  chunk_get( it->run, offset, &it->chunk );
  if( !it->chunk ) {
    it->done = 1;
  }

  return 0;
}

int iter_next( struct iter_t *it, int *did ) {

  // Consume the run
  while( it->j < it->chunk->used ) {
    it->j += sizeof(int);
    *did = it->chunk->run[it->k++];
    return 0;
  }

  // Hit potential chunk boundry
  if( it->chunk->prev == -1 ) {
    // We are at origin chunk for this term, done
    it->done = 1;
    return -1;
  } else {
    // Still have chucnks left, grab em
    it->j = it->k = 0;
    chunk_get( it->run, it->chunk->prev, &it->chunk );
    return iter_next( it, did );
  }
}

int index_find( struct index_t *index, int field, const char *word, int *count, 
  int outs[] ) {

  struct term_t *n;
  struct chunk_t *c;
  struct iter_t it = {0}; 
  int rc;
  int did;

  rc=word_find( &index->corpus, word, &n );
  if( rc ) return rc;
  if( !n ) return 0;

  rc=chunk_get( &index->run, n->fields[field].last, &c );
  if( rc ) return rc;

  it.run = &index->run;
  it.field = &n->fields[field];

  iter_first( &it, n->fields[field].last );
  while( iter_next(&it, &did ) == 0 ) {
    printf("%d\n", did );
  } 
  
  return 0; 
}

int main() {
  struct index_t index = {0};
  int rc;
  int k;
  int result[10];

  rc = index_load( "", &index, 1, 100000 );
  if( rc ) {
    printf("error: %s\n", strerror(errno));
    return rc;
  }  

  for( k=0; k<DCOUNT(); k++ ) {
    char *toke = strtok( documents[k].data, " ");
    while( toke ) {
      if( term_clean(toke) ) {   
        index_word_document( &index, 0, toke, documents[k].id );
      }
      toke = strtok( NULL, " " );
    }
  }

  k = 2;
  index_find( &index, 0, "the", &k, result );  
  index_find( &index, 0, "appl", &k, result );  
  return index_save( &index );
}
