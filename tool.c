#include "tool.h"
#include <math.h>

int ltest() {
  chunk_t chunk = {0};
  struct list_head *pos;

  int k;
  uint32_t count =0;

	chunk_init(&chunk);
	chunk_list_init( &chunk );
  for( k=0; k<2<<24; k++ ) {
    count++;
    chunk_list_push( &chunk, 1 );
  }
  for( k=0; k<2<<24; k++ ) {
    count++;
    chunk_list_push( &chunk, 1 );
  }

  k = 0;
  list_for_each( pos, &chunk.list) {
    k++;
  }
  printf("count: %d %d\n",k, count);
  return 0;
}

void stest() {
	sorter_t *s;
	s = sorter_init();
  for( int k=0; k<2<<24; k++ ) {
    sorter_push( s, 1,k );
  }
}

void stest1() {
	sorter_t *s;
	s = sorter_init();
	sorter_push(s,1,1);
	sorter_push(s,1,2);
	sorter_push(s,1,2);
}

void stest2() {
	sorter_t *s;
	s = sorter_init();
  for( int k=1; k<2<<24; k++ ) {
    sorter_push( s, k % 1000,k );
  }
	sorter_dump( s );
}
int main() {
//	ltest();

	stest2();
}

