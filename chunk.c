#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <gumbo.h>
#include <unicode/ubrk.h>
#include <unicode/ustring.h>
#include "streamvbytedelta.h"

#include "tool.h"

uint8_t * chunk_compress( chunk_t *chunk, uint32_t *bcount );
int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk );

int chunk_init( chunk_t *chunk ) {
	memset( chunk, 0, sizeof(*chunk));
	chunk->max_size = 2<<22;
	chunk->capacity = 0;
	chunk->buffer = (uint32_t *)malloc( sizeof(uint32_t) * chunk->capacity );

	return 0;
}

// Push value, returns 1 if we are now full
int chunk_push( chunk_t *chunk, uint32_t value ) {
	assert( chunk->is_compressed == 0 );

	if( chunk->size == chunk->capacity ) {
		if( chunk->capacity == chunk->max_size ) return -1;
		chunk->capacity = chunk->capacity ? chunk->capacity << 1 : 4;
		chunk->buffer = (uint32_t*)realloc( chunk->buffer, sizeof(uint32_t)*chunk->capacity );
	}
	chunk->buffer[ chunk->size++ ] = value;

	return chunk->size == chunk->capacity;
}

int chunk_full( chunk_t *chunk ) {
	return chunk->size == chunk->max_size;
}

// Get value of offset.  Offset is incremented for you.  Returns -1 if you reach capacity
// otherwise returns 0 on success
int chunk_get( chunk_t *chunk, uint32_t *off, uint32_t *value ) {
	if( chunk->is_compressed || *off >= chunk->capacity ) return -1;
	*value = chunk->buffer[ (*off)++ ];
	return 0;
}

// An empty chunk_t object is valid, so check if we have allocated any data before freeing
void chunk_free( chunk_t *chunk ) {
	if( chunk->capacity > 0 ) free( chunk->buffer );
  else if( chunk->is_compressed ) free(chunk->cbuff ); 	
}

chunk_t * chunk_list_push( chunk_t *chunk, uint32_t doc ) {

	if( chunk == NULL || chunk_full( chunk ) ){
		if( chunk ) {
		  chunk_compress( chunk, &chunk->bcount ); 
		}
		chunk_t *c = malloc(sizeof(*c));
		memset(c,0,sizeof(*c));
		chunk_init(c);
		c->next = chunk;
	  chunk_push( c, doc );
    return c;
	} else {	
	  chunk_push( chunk, doc );
	}
	return chunk;
}

// Vint compress chunk assumes entires are sorted in ascending order
uint8_t * chunk_compress( chunk_t *chunk, uint32_t *bcount ) {
	assert( chunk->size > 0 && chunk->is_compressed == 0 );
	// Allocate enough data for the compressed buffer then compress
	uint8_t *cbuffer = malloc( chunk->size * sizeof(uint32_t));
	uint32_t csize = streamvbyte_delta_encode( chunk->buffer, chunk->size, cbuffer, 0 );

	// Record number of bytes used in compression, likely a pointer to a field in head
  *bcount = csize;

	free( chunk->buffer );
	chunk->is_compressed = 1;
  chunk->cbuff = cbuffer;
	return cbuffer;
}

void chunk_decompress( chunk_t *chunk, uint32_t N ) {
	assert( chunk->is_compressed == 1);

	uint32_t *buff = (uint32_t*)malloc( sizeof(uint32_t)*N );

	// Do the varint decoding
	streamvbyte_delta_decode( chunk->cbuff, buff, N, 0 );
	free(chunk->cbuff);
	chunk->buffer = buff;
}

int read_block( FILE *in, uint32_t N, uint32_t bcount, chunk_t *chunk ) {
	// ALlocate enough data to read in the compressed bytes
	uint8_t *cbuffer = malloc( N * sizeof(uint32_t));
  int rc = fread( cbuffer, bcount, 1, in ); // bcount and N will be pointers intside head
	assert(rc == 1 );
	chunk_decompress( chunk, N );

	// Compressed buffer no longer needed
	free(cbuffer);

	return 0;
}

