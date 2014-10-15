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

int main() {
  struct index_t index = {0};
  int rc;
  int k;
  int result[10];
  struct iter_t it = {0};

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

  k = 10;
  index_find( &index, 0, "the", &it, &k, result );  
//  index_find( &index, 0, "appl", &k, result );  
  return index_save( &index );
}
