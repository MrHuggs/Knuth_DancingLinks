# AlgorithmM

This branch implements Algorithm M: Covering with multiplicities and colors.

## Differences

Knuth's algorithm allows a the color of an item to be blank to indicate
it has a unique color (e.g. "x:"). The can be easily handed at problem set
by just making a new color.

These algorithms take their input from a ExactCoverWithMultiplicitiesAndColors structure,
as opposed to just a bunch of strings.

# Implementations

## MStringValues

This implementation attempts to follow Knuth as closely as possible, as explained
in 7.2.2.1. 

Uses the same header/cell structure as Knuth
Uses the same names for cover, uncover, etc. as Knuth
State names in AlgXStates follow Knuth
Single file implemetation with no classes

## AlgMPointer

This implementation is what I would write C++ today. 

Algorithm is in a class.
Data structures uses pointers, instead of indices.








Problem 78. [16 ] Show that it’s quite easy to pack the 27 mathematicians’ names of Fig. 71
into a 12 × 15 array, with all names reading correctly from left to right.

w  e  i  e  r  s  t  r  a  s  s  -

m  i  n  k  o  w  s  k  i  -  -  -

s  y  l  v  e  s  t  e  r  n  -  -

f  r  o  b  e  n  i  u  s  -  -  f

k  i  r  c  h  h  o  f  f  -  f  -

s  t  i  e  l  t  j  e  s  o  -  -

b  e  r  t  r  a  n  d  k  -  -  -

g  l  a  i  s  h  e  r  m  i  t  e

-  c  a  t  a  l  a  n  d  a  u  -

r  u  n  g  e  m  e  l  l  i  n  -

j  e  n  s  e  n  e  k  n  o  p  p