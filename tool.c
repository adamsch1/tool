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
#include "tool.h"

/**
 *  Comparator for RB declarations as required by man tree.h 3
 */
int wordcmp( struct term_t *a, struct term_t *b ) {
  return strcmp(a->name, b->name);
}

/** 
 *  Use Red/Black tree from tree.h [BSD/Linux].  Stores in core terms
 *  before written to disk
 */
RB_HEAD(wordtree, term_t) head = RB_INITIALIZER(&head);
RB_GENERATE(wordtree, term_t, entry, wordcmp);

/**
 *  Release resources assocaited withe a file_t instance
 */
int file_close( struct file_t *in ) {

  // Cleanup the memory mappings and close the underlying files
  munmap( in->map, in->size );
  close( in->fd );

  // Free allocations
  free( in->filename );
  free( in );
}

/**
 *  This opens/creates a file and memory maps it.
 */
int file_open( const char *fpath, int file_size, struct file_t **outs ) {
  int fd=0;
  struct stat sb;
  void *map = 0;
  int real_size = 0;
  struct file_t *file=0;

  *outs = 0;

  fd = open(fpath, O_RDWR|O_CREAT,0660);
  if( fd == -1 ) goto error;
  if( fstat( fd, &sb) == -1 ) goto error;
  
  file = calloc(1,sizeof(struct file_t));
  if( !file ) goto error;
 
  file->filename = strdup(fpath);
  if( file->filename == NULL ) goto error;

  if( sb.st_size < file_size ) {
    if( ftruncate( fd, file_size ) ) goto error;
    real_size = file_size;
  } else {
    real_size = sb.st_size;
  }

  map = mmap( NULL, real_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  if( map == MAP_FAILED ) goto error;

  file->fd = fd;
  file->map = map;
  file->size = real_size;

  *outs = file;
  return 0;
 
error:
  if( fd ) close(fd);
  if( file && file->filename) free(file->filename);
  if( file ) free(file);

  return -1;  
}

#define CHUNK_SIZE() ( sizeof(struct chunk_t) )
/**
 *  Grab a chunk pointer from the run file with ID [byte offset currently]
 */
int chunk_get( struct run_t *run, int id, struct chunk_t **outs ) {
  struct chunk_t *chunk = 0;

  *outs = 0;
  if( id >= run->header.size ) {
    return 0;
  }

  chunk = (struct chunk_t *)(((char*)run->map) + id);
  *outs = chunk;

  return 0;
}

/**
 *  Allocates a new chunk on disk.
 */
int chunk_allocate( struct run_t *run, int size, 
  struct chunk_t **outs, int *id ) {

  int real_size = 0;
  struct chunk_t *chunk = 0;

  *outs = 0;

  real_size = size + sizeof(struct chunk_t);

  if( run->header.last + real_size >= run->header.size ) {
    return ER_FULL;
  }

  chunk = (struct chunk_t *)(((char*)run->map) + run->header.last);
  chunk->size = size;
  chunk->prev = chunk->next = chunk->docid = 0;

  *id = run->header.last++;

  run->header.last += real_size;

  *outs = chunk;

  return 0;
}

/**
 *  Allocate an initialize a new term
 */
int word_make( const char *word, int field_count, struct term_t **outs ) {
  struct term_t *n = calloc(1,TERM_SIZE(field_count));

  if( !n ) { *outs = 0 ; return -1; }

  strncpy( n->name, word, sizeof(n->name));

  *outs = n;

  return 0; 
}

/**
 *  Store word in-core [currently a RB-TREE]
 */
int word_add( struct term_t *word ) {
  RB_INSERT(wordtree, &head, word );
}

/**
 *  Taken from bsearch but modified so we dont'y have to copy the key data
 *  in order to search through our terms.  Probably unecessary but whatevs
 */
static void * tsearch( const char *key, char *base, size_t nmemb, int term_size) {
  register size_t lim;
  register int cmp;
  register const struct term_t *p;

  for (lim = nmemb; lim != 0; lim >>= 1) {
    p = (struct term_t *)(base + (lim >> 1) * term_size);
    cmp = strcmp( key, p->name );
    if (cmp == 0)
      return ((void *)p);
    if (cmp > 0) {  /* key > p: move right */
      base = (char *)p + term_size;
      lim--;
    }    /* else move left */
  }
  return (NULL);
}

/**
 *  Find the term, if it exists.  First search in-core, then hits disk.
 *  since data on disk is mmemaped and fixed size we just binary search that
 */
int word_find( struct corpus_t *corpus, const char *word, 
  struct term_t **outs ) {

  struct term_t *temp = RB_ROOT(&head);
  int comp;
  while (temp) {
    comp = strcmp(word,temp->name);
    if( comp < 0 ) { 
      temp = RB_LEFT(temp, entry);
    } else if( comp > 0 ) {
      temp = RB_RIGHT(temp,entry);
    } else {
      *outs = temp;
      return 0;
    }
  }

  *outs = tsearch( word, corpus->map, corpus->header.count, 
    TERM_SIZE(corpus->header.field_count) );
  return 0;
}

/**
 *  Get a term from the corpus file given the logical offset k
 */
int corpus_get( struct corpus_t *corpus, int k, struct term_t **outs ) {
  struct term_t *term;

  *outs = 0;
  
  if( k >= corpus->header.count ) return 0;

  term = (struct term_t *)(corpus->map + k * 
    TERM_SIZE(corpus->header.field_count));

  *outs = term;
  return 0;
}

/**
 *  Given word, a document identifier and the logical field, add to the 
 *  inverted index.
 */
int index_word_document( struct index_t *index, int field, const char *word, 
  int docid) {

  struct term_t *n;
  struct chunk_t *c;
  int rc;
  int id;

  if( index->corpus.header.last_docid >= docid ) {
    return ER_DOCID;
  } else {
    index->corpus.header.last_docid = docid;
  }

  // See if word exists 
  rc=word_find( &index->corpus, word, &n);
  if( rc ) return rc;

  if( !n ) {
    // New term
    rc=word_make( word, 1, &n );
    if( rc ) return rc; 

    n->fields[field].count = 1;

    // Allocate a chunk
    rc=chunk_allocate( &index->run, index->chunk_size(n,field), &c, &id);
    if( rc || !c ) return rc; 

    // Setup term properties to point to this chunk so we can find it later
    n->fields[field].first = id; 
    n->fields[field].last = id;
     
    // Finish setting up chunk  
    c->docid = docid;
    c->prev = -1;

    // Add the word in core.
    word_add(n);

    // In-core term count
    index->corpus.term_count++;
  } else {
    // Existing word grab most recent chunk from disk
    rc = chunk_get( &index->run, n->fields[field].last, &c );
    if( rc ) return rc;

    // It's full?
    if( c->used + sizeof(int) >= c->size ) {
      struct chunk_t *nc=0;

      n->fields[field].count++;

      // Allocate a new chunk
      rc=chunk_allocate( &index->run, index->chunk_size(n,field), &nc, &id);
      if( rc || !c ) return rc; 
  
      // Setup chunk
      nc->docid = docid;

      // New chunk points previously to old chunk
      nc->prev = n->fields[field].last;

      // Old chunk points to the new chunk
      c->next = id;
   
      // New chunk is the last chunk in the term properties
      n->fields[field].last = id;

      c = nc;
    }
  }

  // Finally, add docid to this chunk
  c->run[ c->used ] = docid;
  c->used += sizeof(int);
  n->fields[field].count++;

  return 0; 
}

/**
 *  Merge sorts the disk term set against the in-core term data structure
 *  [currently RB tree].  As our in-core data structure is already sorted,
 *  this is a linear operation.  The write sink is indicated by the file 
 *  descriptor fd so it can be a local file or remote etc.
 */
int corpus_flush( struct corpus_t *corpus, int fd ) {
  struct term_t *a;
  struct term_t *b;
  int rc;
  int k=0;

  // Write the final number of terms synced to disk
  corpus->header.count += corpus->term_count;
  write(fd, &corpus->header, sizeof(corpus->header));
  
  // Readjust as corpus_get relies on the term count header matching the true
  // number of terms on disk
  corpus->header.count -= corpus->term_count;

  // Get lexicographical first term from disk and in-core
  corpus_get( corpus, k, &b );
  a = RB_MIN(wordtree, &head );

  // While we have more data in both sources
  while( a && b ) {
    // Compare them lexicographical and write them in order
    rc = strcmp( a->name, b->name );
    if( rc < 0 ) {
      write(fd, a, TERM_SIZE(corpus->header.field_count));
      a = RB_NEXT(wordtree, &head, a );
    } else if( rc > 0 ) {
      write(fd, b, TERM_SIZE(corpus->header.field_count));
      corpus_get( corpus, k, &b );
    } else {
      // They are equal, wait what?  This should never occur!
      write(fd, a, TERM_SIZE(corpus->header.field_count));
      write(fd, b, TERM_SIZE(corpus->header.field_count));
      corpus_get( corpus, k, &b );
      a = RB_NEXT(wordtree, &head, a );
    }
    k++;
  }

  // We hit the end of a term source [either disk or in-core]
  // write the remaining one, if any

  while( a ) {
    write(fd, a, TERM_SIZE(corpus->header.field_count));
    a = RB_NEXT(wordtree, &head, a );
  }

  while( b ) {  
    write(fd, b, TERM_SIZE(corpus->header.field_count));
    k++;
    corpus_get( corpus, k, &b );
  }
  
}

/** 
 *  Convenience functions:  Save an index to disk
 */
int index_save( struct index_t *index ) {
  char buffer[] = "./fileXXXXXXXX";
  int fd;

  // Merge sort the new corpus to disk
  fd = mkstemp(buffer);
  if( fd == -1 ) return -1;
  corpus_flush( &index->corpus, fd );

  // Temp corpus to our original file name
  rename( buffer, "./corpus.data" );

  memcpy( index->run.map-sizeof(index->run.header), &index->run.header, sizeof(index->run.header));

  file_close( index->run.file );
  file_close( index->corpus.file );

  return 0;
}

static int chunk_size( const struct term_t *term, int field ) {
  return 1 * sizeof(int);
  //return (1<<term->fields[field].count);
}

/**
 *  Convenience functions: Load an index from disk
 */
int index_load( const char *filename, struct index_t *index, 
  int field_count, int file_size ) {

  struct file_t *rfile = 0;
  struct file_t *cfile = 0;
  int rc;

  // Always have two files, one for the corpus, the other for the inverted
  // index we call the run file
  rc = file_open( "./run.data", 1000, &rfile );
  if( rc ) goto error;
  
  rc = file_open( "./corpus.data", file_size, &cfile ); 
  if( rc ) goto error;

  // Copy in the header for each file as well as the memory map pointer
  memcpy( &index->run.header, rfile->map, sizeof(index->run.header));
  index->run.map = rfile->map + sizeof(index->run.header);
  index->run.header.size = rfile->size;

  memcpy( &index->corpus.header, cfile->map, sizeof(index->corpus.header));
  index->corpus.map = cfile->map + sizeof(index->corpus.header);
  index->corpus.header.size = cfile->size;

  // Set aside the file_t structure for later use
  index->corpus.file = cfile;
  index->run.file = rfile;

  index->chunk_size = chunk_size;

  // Is this a new file?  If so set the field_count
  if( !index->corpus.header.field_count ) {
    index->corpus.header.field_count = field_count;
  }

  return 0;

error:
  if( rfile ) file_close( rfile );
  if( cfile ) file_close( cfile );

  return -1;
}

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
  int real_count = 0;

  rc=word_find( &index->corpus, word, &n );
  if( rc ) return rc;
  if( !n ) return 0;

  rc=chunk_get( &index->run, n->fields[field].last, &c );
  if( rc ) return rc;

  it.run = &index->run;
  it.field = &n->fields[field];

  iter_first( &it, n->fields[field].last );
  while( iter_next(&it, &did ) == 0 && real_count < *count ) {
    outs[real_count++] = did;
  }
  *count = real_count;
  return 0;
}


/*
int main() {
  struct term_t *n;
  struct file_t *file;
  int rc;
  struct index_t index = {0};

  rc=index_load( "", &index, 1 );
  if( rc ) {
    printf("%s\n", strerror(errno));
    return -1;
  }

  rc=index_word_document( &index, 0, "SLUG", 69 );
  if( !rc ) { 
    printf("%s\n", strerror(errno));
  }

  index_save( &index );
}
*/

