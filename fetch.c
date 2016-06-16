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

typedef struct {
	uint32_t size;
	uint32_t cap;
	uint8_t  *buff;
} buff_t;

static inline int nearest_power( uint64_t num ) {
	num--;
	num |= num >> 1;
	num |= num >> 2;
	num |= num >> 4;
	num |= num >> 8;
	num |= num >> 16;
	num |= num >> 32;
	return ++num;
}

void buff_grow( buff_t *b, size_t size ) {
	if( b->size + size >= b->cap ) {
		b->cap = nearest_power( b->cap + size + 1 );
		b->buff = realloc( b->buff, b->cap );
	}
}

void buff_cat( buff_t *b, const char *p ) {
	if( p == NULL || *p == 0 ) return;
	size_t len = strlen(p);
	buff_grow( b, len);
	memcpy( b->buff+b->size, p, len );
	b->size += len;
}

void buff_catn( buff_t *b, const char *p, size_t rsize ) {
	if( p == NULL || *p == 0 ) return;
	buff_grow( b, rsize);
	memcpy( b->buff+b->size, p, rsize );
	b->size += rsize;
	b->buff[b->size] = 0;
}

void buff_free( buff_t *b ) {
	if( b->cap ) {
		free(b->buff);
	}
}

typedef struct {
  UChar *str;
  UBreakIterator *boundary;

  int32_t start;
  int32_t end;	
} tokenizer_t;

void tokenizer_init( tokenizer_t *t, const char *input ) {
  memset( t, 0, sizeof(*t));
	UErrorCode status = U_ZERO_ERROR;

  t->str = malloc( sizeof(UChar) * strlen(input) );
  u_uastrcpy(t->str, input);

	t->boundary = ubrk_open(UBRK_WORD, "en_us", t->str, -1, &status );
}

int tokenizer_next( tokenizer_t *t, char *word, size_t size ) {
  UChar   savedEndChar;
  int k;

  // start iterator
  if( t->end == 0 ) {
    t->start = ubrk_first(t->boundary);
  }

  // Find next word
again:
  t->end = ubrk_next(t->boundary);
  if( t->end == UBRK_DONE ) {
    return -1;
  }

	// Null terminate
  savedEndChar = t->str[t->end];
  t->str[t->end] = 0;

	// Skip unct
	if( t->end - t->start == 1 && u_ispunct( t->str[t->start] ) ) {
    t->str[t->end] = savedEndChar;
    t->start = t->end;
    goto again;
	}

	// Skip whitespace
	for( k=t->start; k<t->end; k++ ) {
	  if( u_isspace( t->str[k] ) == 1 ) {
      t->str[t->end] = savedEndChar;
      t->start = t->end;
			goto again;
		}
  }

	// Copy to C bffer
  u_austrncpy(word, t->str+t->start, size-1);
  word[size-1] = 0;

  printf("string[%2d..%2d] \"%s\" %d\n", t->start, t->end-1, word, u_isspace( t->str[t->start])); 
  t->str[t->end] = savedEndChar;
  t->start = t->end;
  return 0;
}

void tokenizer_free( tokenizer_t *t ) {
	ubrk_close(t->boundary);
  free(t->str);
}

// Parse Crawl 
typedef struct {
	buff_t buff;

	GumboOutput *output;
	GumboNode *node;
} crawl_parse_t;

void crawl_parse_init( crawl_parse_t *c ) {
	memset( c, 0, sizeof(*c));
}

void crawl_parse_real( crawl_parse_t *c, GumboNode *node ) {

	uint32_t k;
	if( node->type == GUMBO_NODE_TEXT ) {
		buff_cat( &c->buff, node->v.text.text );
	} else if( node->type == GUMBO_NODE_ELEMENT &&
			node->v.element.tag != GUMBO_TAG_SCRIPT &&
			node->v.element.tag != GUMBO_TAG_STYLE ) {
		GumboVector *children = &node->v.element.children;
		for(  k=0; k<children->length; ++k ) {
			if( k > 0 && c->buff.size ) {
				buff_cat( &c->buff, " ");
			}
			crawl_parse_real( c, (GumboNode*)children->data[k] );
		}
	}
}

void crawl_parse_parse( crawl_parse_t *c, uint8_t *content ) {
	c->output = gumbo_parse( (const char *)content );
	crawl_parse_real(c, c->output->root);
	gumbo_destroy_output( &kGumboDefaultOptions, c->output );
}

void crawl_parse_free( crawl_parse_t *c ) {
	buff_free( &c->buff );
}


// Fetch web page using CURL
typedef struct {
  CURL *curl_handle;
	char *url;

	buff_t buff;
} crawl_fetch_t;


void crawl_fetch_init( crawl_fetch_t *f, const char *url) {
	memset( f, 0, sizeof(*f));
	f->url = strdup(url);
}

size_t crawl_fetch_data( void *content, size_t size, size_t nmemb, void *userp ) {
	size_t rsize = size * nmemb;
	crawl_fetch_t *f = (crawl_fetch_t *)userp;
	buff_catn( &f->buff, content, rsize );
	return rsize;
}

CURLcode crawl_fetch_fetch( crawl_fetch_t *f ) {
	CURLcode res;
	f->curl_handle = curl_easy_init();
  curl_easy_setopt(f->curl_handle, CURLOPT_URL, f->url );
  curl_easy_setopt(f->curl_handle, CURLOPT_WRITEFUNCTION, crawl_fetch_data);
  curl_easy_setopt(f->curl_handle, CURLOPT_WRITEDATA, (void *)f);
  res = curl_easy_perform(f->curl_handle);
	printf("%s\n", curl_easy_strerror(res));
  curl_easy_cleanup(f->curl_handle);
	return res;
} 

void crawl_fetch_free( crawl_fetch_t *f ) {
	free( f->url );
	buff_free( &f->buff );
}

void	fetchtest() {
	crawl_fetch_t f;
	crawl_parse_t p;
	tokenizer_t t;
  char word[1024];

	crawl_fetch_init( &f, "http://web.mit.edu");
	crawl_fetch_fetch( &f );

	crawl_parse_init( &p );
	crawl_parse_parse( &p, f.buff.buff );

	tokenizer_init( &t, (char *)p.buff.buff );
  while( !tokenizer_next( &t, word, sizeof(word) ) ) {
  }
  tokenizer_free( &t );

	crawl_parse_free( &p );
	crawl_fetch_free( &f );
}

int main() {
	fetchtest();
}






