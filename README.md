Tool a  minimal text indexing and searching library.

Tool is an intentionally small C library that allows one 
to create and search documents in the form of an inverted 
word index.  Tool supports multiple fields for indexing.  
For example if you are indexing rfc822 email you could
index the email subject as well as the email body 
separate from one another.

Tool does not tokenize data.  The caller is expected to 
pass in tokenized data when indexing. As such Tool
is UTF8 Agnostic.  Tool itself does not tokenize any 
data source by design.

Tool maintains two files on disk, the corpus or term file 
and the run file.  The corpus file is a sorted set of 
individual terms.  The run file is the inverted file itself, 
which maintains the set of document identifiers associated 
with each term. 

Tool maps these files in memory.  This simplifes the 
implementation whileincreasing performance.

Why Tool when you have Lucene?

Luecene is full of awesome, it's the likely choice for most 
applications.  Lucene is written in Java, and as such, 
requires the full java stack and is not easily embeddable 
in the context of non-jvm languages.  This may not be 
ideal or even appropriate in all situations.

Tool was designed to make few asumptions and allow itself to 
be easily embedded in a host scripting language, e.g. Pyhon, 
PHP, Perl, Ruby.  These languages typically have a more robust 
support for non-ascii text sources such as PDF, HTML etc.  
This means that it is somewhat easier to read, parse and 
tokenize these data sources in the host language and call 
into Tool to index the data.
