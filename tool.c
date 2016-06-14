#include "tool.h"
#include <math.h>

int ltest() {
  chunk_t chunk = {0};

  int k;
  uint32_t count =0;

	chunk_init(&chunk);
  for( k=0; k<2<<24; k++ ) {
    count++;
    chunk_list_push( &chunk, 1 );
  }
  for( k=0; k<2<<24; k++ ) {
    count++;
    chunk_list_push( &chunk, 1 );
  }

  k = 0;
	chunk_t *c;
  printf("count: %d %d\n",k, count);
  return 0;
}

/*
void stest() {
	sorter_t *s;
	s = sorter_init();
  for( int k=0; k<2<<24; k++ ) {
    sorter_push( s, 1,k );
  }
}*/

/*
void stest1() {
	sorter_t *s;
	s = sorter_init();
	sorter_push(s,1,1);
	sorter_push(s,1,2);
	sorter_push(s,1,2);
}

void stest3() {
	sorter_t *s;
	s = sorter_init();
	for( int k=1; k<2<<24; k++ ) {
    sorter_push( s, k,k);
	}
	sorter_dump( s );
}

void stest2() {
	sorter_t *s;
	s = sorter_init();
  for( int k=1; k<2<<24; k++ ) {
    sorter_push( s, k % 1000,k );
  }
	sorter_dump( s );
}

void stest4() {
	sorter_t *s;
	s = sorter_init();
	int count=0;
  for( int k=1; k<2<<24; k++ ) {
    sorter_push( s, k%1000,k );
		count++;
  }
	printf("%d %d\n", count, 2<<24 );
	sorter_dump( s );
}
*/

int main() {
//	ltest();
//	stest3();
//	stest2();
//	stest4();
}

