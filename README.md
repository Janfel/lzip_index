# lzip_index

This library implements an index for multi-member `lzip` archives.

For archives with fixed-size input blocks, such as those created by `plzip`, this index allows random access. For other archives, this index allows `O(log n)` access to the compressed data. Accessing the nth archive member is always possible in constant time.

The `lzip_indexer` program demonstrates the use of this library. Note that the input file to `lzip_indexer` must allow seeking. Archives with trailing data are not supported.
