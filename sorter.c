#include "tool.h"

#define int_cmp(a,b) ((int)(b).term-(int)(a).term)
KBTREE_INIT(chunk_list, chunk_list_t, int_cmp)

struct _sorter_t {
  kbtree_t(chunk_list) *b;
};

sorter_t * sorter_init( ) {
	sorter_t *s = malloc(sizeof(sorter_t));
	s->b = kb_init( chunk_list, KB_DEFAULT_SIZE );
	return s;
}


void sorter_push( sorter_t *sorter, uint32_t term, uint32_t doc ) {
	chunk_list_t t = {0};
  chunk_list_t *p;

	t.term = term;

	p = kb_getp( chunk_list, sorter->b, &t );

	if( !p ) {
		//printf("here: %d %x %x %x\n", term, p, &t, sorter->b );
		// okay put an uninitialized entry in the btree.  (important)
		kb_putp( chunk_list, sorter->b, &t );
		// Now get actual address to entry that was allocated internally via btree
		p = kb_getp( chunk_list, sorter->b, &t );
		// Now we can initialize our object since we have self referential pointers
		chunk_list_init(p);
		chunk_list_push( p, doc );
	} else {
		chunk_list_push( p, doc );
	}
}

