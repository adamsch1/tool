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
#include <string>
#include <vector>

#include "streamvbytedelta.h"

typedef struct {
  UChar *str;
  UBreakIterator *boundary;

  int32_t start;
  int32_t end;	
} tokenizer_t;

void tokenizer_init( tokenizer_t *t, const char *input ) {
  memset( t, 0, sizeof(*t));
	UErrorCode status = U_ZERO_ERROR;

  t->str = (UChar *)malloc( sizeof(UChar) * strlen(input) );
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

	UErrorCode ecode;
	u_strToLower( t->str+t->start, size-1, t->str+t->start, size-1, "", &ecode ); 
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
	std::string buff;

	GumboOutput *output;
	GumboNode *node;
} crawl_parse_t;

void crawl_parse_init( crawl_parse_t *c ) {
	memset( c, 0, sizeof(*c));
}

void crawl_parse_real( crawl_parse_t *c, GumboNode *node ) {

	uint32_t k;
	if( node->type == GUMBO_NODE_TEXT ) {
		c->buff += node->v.text.text;
	} else if( node->type == GUMBO_NODE_ELEMENT &&
			node->v.element.tag != GUMBO_TAG_SCRIPT &&
			node->v.element.tag != GUMBO_TAG_STYLE ) {
		GumboVector *children = &node->v.element.children;
		for(  k=0; k<children->length; ++k ) {
			if( k > 0 && c->buff.size() ) {
				c->buff += " ";
			}
			crawl_parse_real( c, (GumboNode*)children->data[k] );
		}
	}
}

void crawl_parse_parse( crawl_parse_t *c, const char * content ) {
	c->output = gumbo_parse( content );
	crawl_parse_real(c, c->output->root);
	gumbo_destroy_output( &kGumboDefaultOptions, c->output );
}

void crawl_parse_free( crawl_parse_t *c ) {
}


// Fetch web page using CURL
typedef struct {
  CURL *curl_handle;
	char *url;

	std::string buff;
} crawl_fetch_t;

typedef struct {
	crawl_fetch_t fetch;
	crawl_parse_t parse;
	tokenizer_t   token;
} handle_t;

int crawl_multi( std::vector<handle_t> handles ) {
	CURLM *multi_handle;

	multi_handle = curl_multi_init();

	for( auto it : handles ) {
		curl_multi_add_handle( multi_handle, it.fetch.curl_handle );
	}

	int num = handles.size();
	curl_multi_perform( multi_handle, &num );


	do {
		CURLMcode mc;
		int numfds;
		
		mc =curl_multi_perform( multi_handle, &num );

		mc = curl_multi_wait( multi_handle, NULL, 0, 1000, &numfds );
		if( mc != CURLM_OK ) {
		} else {
			if( !numfds ) {
			} else {
				curl_multi_perform( multi_handle, &num );
			}
		}

	} while( num );
}

size_t crawl_fetch_data( void *content, size_t size, size_t nmemb, void *userp ) {
	size_t rsize = size * nmemb;
	crawl_fetch_t *f = (crawl_fetch_t *)userp;
	f->buff.append( (char *)content, rsize );
	return rsize;
}


void crawl_fetch_init( crawl_fetch_t *f, const char *url  ) {
	memset( f, 0, sizeof(*f));
	CURLcode res;
	f->curl_handle = curl_easy_init();
  curl_easy_setopt(f->curl_handle, CURLOPT_URL, f->url );
  curl_easy_setopt(f->curl_handle, CURLOPT_WRITEFUNCTION, crawl_fetch_data);
  curl_easy_setopt(f->curl_handle, CURLOPT_WRITEDATA, (void *)f);
	curl_easy_setopt(f->curl_handle, CURLOPT_FOLLOWLOCATION, 1);
}

void crawl_fetch_free( crawl_fetch_t *f ) {
	free( f->url );
}


void handle_init( handle_t *hand, const char *url ) {
	crawl_fetch_init( &hand->fetch, url );
	crawl_parse_init( &hand->parse ); 
}

void handle_run( handle_t *hand ) {
  char word[1024];
	crawl_parse_parse( &hand->parse, hand->fetch.buff.c_str() );
	tokenizer_init( &hand->token, (char *)hand->parse.buff.c_str() );
  while( !tokenizer_next( &hand->token, word, sizeof(word) ) ) {
  }
  tokenizer_free( &hand->token );
	crawl_fetch_free( &hand->fetch );
}

int main( int argc, char **argv ) {
	std::vector<handle_t> handles; 
	handle_t hand{};

	handle_init( &hand, "http://web.mit.edu" );
	handles.push_back(hand);
  crawl_multi( handles );
/*	while( fread( &data, 1, sizeof(data), stdin ) > 0 ) {
		handles[0].fetch.url = strdup(data);
					
	}
	*/
}

