#ifndef __CHUNK__H
#define __CHUNK__H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "kbtree.h"

// Array of document ids.  Can be compressed.  Can be chained in memory as a list
typedef struct {
  size_t   capacity;
  uint32_t size;

  uint32_t is_compressed : 1;
  uint32_t max_size      : 25;

  union {
    uint32_t *buffer;
    uint8_t *cbuff;
  };

	uint32_t term;
  uint32_t bcount;

  struct list_head list;
} chunk_t;

typedef struct _sorter_t sorter_t;

uint8_t * chunk_compress( chunk_t *chunk, uint32_t *bcount );
int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk );
int chunk_init( chunk_t *chunk );
int chunk_push( chunk_t *chunk, uint32_t value );
int chunk_get( chunk_t *chunk, uint32_t *off, uint32_t *value );
void chunk_free( chunk_t *chunk );

int chunk_list_push( chunk_t *chunk, uint32_t doc );
void chunk_decompress( chunk_t *chunk, uint32_t N );

void chunk_list_init( chunk_t *list );

sorter_t * sorter_init();
void sorter_push( sorter_t *sorter, uint32_t term, uint32_t doc );
void sorter_dump( sorter_t *sorter );
#endif
