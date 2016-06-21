Use sharc, utility for density compression library.
Use unix metaphor stdin stdout redireciton pipe etc.  

A term/doc entries "pairs" are an unsigned 64 bit int, uint64_t.
The upper 32 bits are the term, the lower 32 bits are the doc id.

The output of indexing should be a stream of these pairs that are 
sorted with the con utilty which sorts in memory as manhy pairs as 
possible and writes them to disk.  THe merge utility merges these sorted
files together.

THe Inv utility which should be a library simply mmaps the final sorted file
and performs a binary search.  This is log(N).  

My little 4 core laptop can perform millions of bsearches a second agains
a 6 gb mmap file.

The tradeoff is that this encoding while simple and allows for simple 
algoriths [ the utilities themselves are not very big and fancy ] give up
space effeciency.  Each term in a document will occupy 8 bytes in the 
inverted index.  Using a variable int encoding which is popular in the literature
and in other implementations you can approach 1 byte per term.  



