#include "tool.h"
#include <math.h>

int ltest() {
  chunk_list_t clist = {0};
  struct list_head *pos;

  int k;
  uint32_t count =0;
  uint32_t bcount=0;

  INIT_LIST_HEAD( &clist.list );
  for( k=0; k<2<<24; k++ ) {
    count++;
    chunk_list_push( &clist, 1 );
  }
  for( k=0; k<2<<24; k++ ) {
    count++;
    chunk_list_push( &clist, 1 );
  }

  k = 0;
  list_for_each( pos, &clist.list) {
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
	int r;
  for( int k=1; k<2<<24; k++ ) {
    sorter_push( s, k % 1000,k );
  }
}
int main() {
	srand( time(0));
//	ltest();

	stest2();
}

