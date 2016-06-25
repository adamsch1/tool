Use sharc, utility for density compression library.
Use unix metaphor stdin stdout redireciton pipe etc.  

A term/doc entries "pairs" are an unsigned 64 bit int, uint64_t.
The upper 32 bits are the term, the lower 32 bits are the doc id.

The output of indexing should be a stream of these pairs that are 
sorted with inverted the con utilty which sorts in memory as manhy pairs as 
possible and writes them to disk.  THe merge utility merges these sorted
files together.

The Inv utility which should be a library simply mmaps the final sorted file
and performs a binary search.  This is log(N).  Update it's now a 
interpolation sort log (log(n)) it is faster from my tests.

My little 4 core laptop can perform millions of bsearches a second agains
a 6 gb mmap file.

The tradeoff is that this encoding while simple and allows for simple 
algoriths [ the utilities themselves are not very big and fancy ] give up
space effeciency.  Each term in a document will occupy 8 bytes in the 
inverted index.  Using a variable int encoding which is popular in the literature
and in other implementations you can approach 1 byte per term.  


The forward index is a bunch of documents.  Each document is a fixed sized struct.
The variable length bits, like the url of the document or the list of terms integers
are stored in a second heap file.  The document has byte offsets in it's structure
that point to the location of these variable sized entries.  

Documents are sorted in the forward index by document id.  Given they are fixed size
the library performs an interpolation search to quickly locate the file.


TODO:

Finish fetch.cc

It pulls down web pages, parses them with gumbo which is a html5 parser.  Extracts the 
text using ICU and it's word break functions.  All I need to do is to conver the terms 
to their numeric values then write the data to the forward index.

Then at indexing time or later with a separate utility feed the document stream to the 
con utility which actually constructs the inverted index. 
