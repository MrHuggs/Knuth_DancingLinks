# AlgorithmM

This branch implements Algorithm M: Covering with Multiplicities and Colors.

## Differences

Knuth's algorithm allows a the color of an item to be blank to indicate
it has a unique color (e.g. "x:"). The can be easily handed at problem set
by just making a new color.

These algorithms take their input from an **ExactCoverWithMultiplicitiesAndColors** structure,
as opposed to just a bunch of strings.

# Implementations

## MStringValues.cpp

This implementation attempts to follow Knuth as closely as possible, as explained
in 7.2.2.1. 

Specficially:

- Uses the same header/cell structure as Knuth
- Uses the same names for **cover()**, **uncover()**, etc. as Knuth
- State names in AlgXStates follow Knuth
- Single file implemetation with no classes

There is quite a bit of extraneous code in the file in order to setup cell structure and produce output.
The algorithm itself is implemented in **exact_cover_with_multiplicities_and_colors()**.

## AlgMPointer

This implementation is what I would write C++ today. 

Specifically:

- Algorithm is in a class.
- Data structures uses pointers, instead of indices.

