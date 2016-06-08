#include "tool.h"

#define int_cmp(a,b) ((int)(a).term-(int)(b).term)
KBTREE_INIT(chunk, chunk_t, int_cmp)

struct _sorter_t {
  kbtree_t(chunk) *b;
};

sorter_t * sorter_init( ) {
	sorter_t *s = malloc(sizeof(sorter_t));
	s->b = kb_init( chunk, KB_DEFAULT_SIZE);
	return s;
}


void sorter_push( sorter_t *sorter, uint32_t term, uint32_t doc ) {
	chunk_t t;
  chunk_t *p;

	t.term = term;

	p = kb_getp( chunk, sorter->b, &t );

	if( !p ) {
		//printf("here: %d %x %x %x\n", term, p, &t, sorter->b );
		// okay put an uninitialized entry in the btree.  (important)
		kb_putp( chunk, sorter->b, &t );
		// Now get actual address to entry that was allocated internally via btree
		p = kb_getp( chunk, sorter->b, &t );
		memset( p, 0,sizeof(*p));
		p->term = term;
		// Now we can initialize our object since we have self referential pointers
		chunk_init(p);
		chunk_list_push( p, doc );
	} else {
		chunk_list_push( p, doc );
	}
}

void sorter_dump( sorter_t *sorter ) {
	kbitr_t itr;
	//chunk_list_t t = {0};
  //chunk_list_t *p;
	/*
	struct list_head *pos;
	kb_itr_first( chunk_list, sorter->b, &itr );
	for(; kb_itr_valid(&itr); kb_itr_next( chunk_list, sorter->b, &itr)) {
		p = &kb_itr_key(chunk_list_t, &itr);
		list_for_each( pos, &p->list) {
			chunk_t *chunk = list_entry( pos, chunk_t, list);
			chunk->term = p->term;
		  printf("%d ", chunk->term);
		}
		printf("\n");
	}*/
}
