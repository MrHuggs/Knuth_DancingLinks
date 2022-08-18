# Knuth 7.2.2.1 Algorithm X

This is a C++ implementation of Algorithm X in The Art of Computer Programming Volume 4.
In the version I have, this is page 67.

The idea is to follow Knuth as closely as possible, so the style is more C-like.

There are several implementations which are slightly different:

* Knuth_7_2_2_1_X.cpp - This is as close to the book as I can come. It uses characters as the items, and so is limitted in how many items it can have.
* XStringValues.cpp - This is like the book version, but with strings as items. This allows the number of items to be very large.
* AlgXPointer.cpp - This is a version written to link the nodes with pointers. Much closer to what a C++ programmer would write and I think easier to read. But, it turns out, signficantly slower.



