#include "tool.h"

typedef struct entry_t entry_t;

struct entry_t {
	chunk_t *chunk;
	uint32_t term;
};

#define int_cmp(a,b) ((int)(a).term-(int)(b).term)
KBTREE_INIT(chunk, entry_t, int_cmp)

struct _sorter_t {
  kbtree_t(chunk) *b;
};

sorter_t * sorter_init( ) {
	sorter_t *s = malloc(sizeof(sorter_t));
	s->b = kb_init( chunk, KB_DEFAULT_SIZE);
	return s;
}


void sorter_push( sorter_t *sorter, uint32_t term, uint32_t doc ) {
	entry_t t;
  entry_t *p;

	t.term = term;

	p = kb_getp( chunk, sorter->b, &t );

	if( !p ) {
		// okay put an uninitialized entry in the btree.  (important)
		kb_putp( chunk, sorter->b, &t );
		// Now get actual address to entry that was allocated internally via btree
		p = kb_getp( chunk, sorter->b, &t );
		// Now we can initialize our object since we have self referential pointers
		p->chunk = malloc( sizeof( *p->chunk ));
		memset( p->chunk, 0, sizeof(*p->chunk));
		chunk_init(p->chunk);
		p->term = term;
		chunk_list_push( p->chunk, doc );
	} else if( chunk_full( p->chunk ) ) {
		p->chunk = chunk_list_push( p->chunk, doc );
	} else {
		p->chunk = chunk_list_push( p->chunk, doc );
	}
}

void sorter_dump( sorter_t *sorter ) {
	kbitr_t itr;
	struct list_head *pos;
	entry_t *p;
	kb_itr_first( chunk, sorter->b, &itr );
	int k=0;
	for(; kb_itr_valid(&itr); kb_itr_next( chunk, sorter->b, &itr)) {
		k++;
		p = &kb_itr_key(entry_t, &itr);

	}
	printf("%d \n",k);
}
