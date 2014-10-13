#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "tool.h"


struct doc_t {
  char data[256];
  int  id;
} documents[] = {
  {"the", 1},
  {"the", 2},
  {"the", 3}
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

int index_find( struct index_t *index, int field, const char *word ) {
  struct term_t *n;
  struct chunk_t *c;
  int rc;
  int k;
  int j;

  rc=word_find( &index->corpus, word, &n );
  if( rc ) return rc;
  if( !n ) return 0;

  rc=chunk_get( &index->run, n->fields[field].last, &c );
  if( rc ) return rc;

  for( j=0,k=0; j<c->used; k++,j+=sizeof(int) ) {
    printf("doc: %d\n", c->run[k]);
  }
  
}

int main() {
  struct index_t index = {0};
  int rc;
  int k;

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

  index_find( &index, 0, "the" );  
  return index_save( &index );
}
