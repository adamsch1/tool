#ifndef __TOOL__H
#define __TOOL__H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "tree.h"

enum { 
  ER_DOCID = -1,  // Docid given was not monotonically increasing
  ER_FULL=-2      // Inverted file is full
};

/**
 *  Posix file abstraction, including memory mapping
 */
struct file_t {
  char *filename;
  int fd;
  int size;
  void *map;
};

/**
 *  Field information, multiple indexable fields per document supported.
 */
struct field_t {
  int first;
  int last;
  int count;
};

/**
 *  A term, with a name and multiple fields.  
 *  TODO: Gank out the entry field so we don't write it to disk
 */
struct term_t {
  RB_ENTRY(term_t) entry;

  char name[64]; 
  struct field_t fields[];
};

// Terms are variable length 
#define TERM_SIZE(f) ( sizeof( struct term_t )+((1+f)*sizeof(struct field_t))) 

/**
 *  Corpus or term file header.  Written/read at offset 0 of corpus file
 */
struct corpusheader_t {
  int64_t count;        // # of terms used [with repeats]
  int     field_count;  // # of fields indexable
  int64_t size;         // # of distinct terms
  int     last_docid;   // last document id indexed.
};

/**
 *  Run file header.  Written/read at offset 0 of run file
 */
struct runheader_t {
  int64_t size;
  int64_t last;
};

/**
 *  Stores a run of document id's associated with a single term.  Multiple
 *  chunks are maintained on disk as a linked-list.   The term entry in the
 *  corpus maintains start/end pointers to these lists.
 */
struct chunk_t {
  int size;
  int prev;
  int next;
  int used;
  int docid;
  int run[];
};

/**
 *  Main DS for a corpus
 */
struct corpus_t {
  struct corpusheader_t header;
  struct file_t *file;
  int  term_count;
  void *map;
};

/**
 *  Main DS for a run
 */
struct run_t {
  struct runheader_t header;
  struct file_t *file;
  void *map;
};

/**
 *  Main DS for an index, optional
 */
struct index_t {
  struct corpus_t corpus;
  struct run_t run;

  int (*chunk_size)( const struct term_t *word, int field );
};

/**
 * Iterator 
 */
struct iter_t {
  struct run_t *run;
  struct chunk_t *chunk;
  struct field_t *field;
  int k;
  int j;
  int start;
  int chunk_offset;
  int done;
};

/**
 *  Most functions return 0 on success, and non-zero on a low level error
 *  eg malloc() failed etc.  Check the errno.
 */

/**
 *  Grab a chunk pointer from the run file with ID [byte offset currently]
 */
int chunk_get( struct run_t *run, int id, struct chunk_t **outs );

/**
 *  Find term in corpus, if it exists
 */
int word_find( struct corpus_t *corpus, const char *word,
  struct term_t **outs );

/**
 *  Merge sorts the disk term set against the in-core term data structure
 *  [currently RB tree].  As our in-core data structure is already sorted,
 *  this is a linear operation.  The write sink is indicated by the file
 *  descriptor fd so it can be a local file or remote etc.
 */
int corpus_flush( struct corpus_t *corpus, int fd );

/**
 *  Allocate and initialize a new term
 */
int word_make( const char *word, int field_count, struct term_t **outs );

/**
 *  Store word in-core [currently a RB-TREE].  Call corpus_flush to persist
 *  when done indexing.
 */
int word_add( struct term_t *word );

/**
 *  Convenience functions: Load an index from disk
 */
int index_load( const char *filename, struct index_t *index, int field_count,
  int file_size );

/**
 *  Convenience functions:  Save an index to disk
 */
int index_save( struct index_t *index );

/**
 *  Given word, a document identifier and the logical field, add to the
 *  inverted index.
 */
int index_word_document( struct index_t *index, int field, const char *word,
  int docid);

/**
 *  Initialize an iterator in preperation for walking document "runs" which 
 *  will allows you to find all the documents that contain a given term.
 */
int iter_first( struct index_t *index, int field, const char *word, 
  struct iter_t *it );

/**
 * Returns the next document identifier.  
 */
inline int iter_next( struct iter_t *it, int *did );

/**
 *  Wrapper around iter_first.  Given a term, this will prepare an iterator
 *  and copy over count number of document identifiers into the outs array.
 *  count will be set to the real number of document ids copied if less than
 *  count results were found.  The iterator can be resused to builk copy
 *  additional sets of document ids.
 */
int index_find( struct index_t *index, int field, const char *word, 
  struct iter_t *it, int *count, int outs[] );

/**
 *  
 */
unsigned fnv32( const char *term, unsigned prev );

/**
 * 
 */
uint32_t crc32(uint32_t crc, const void *buf, size_t size);

#endif
