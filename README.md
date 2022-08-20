# Knuth 7.2.2.1 Algorithm X

This is a C++ implementation of Algorithm X in The Art of Computer Programming Volume 4.
In the version I have, this is page 67.

The idea is to follow Knuth as closely as possible, so the style is more C-like.

There are several implementations which are slightly different:

* Knuth_7_2_2_1_X.cpp - This is as close to the book as I can come. It uses characters as the items, and so is limitted in how many items it can have.
* XStringValues.cpp - This is like the book version, but with strings as items. This allows the number of items to be very large.
* AlgXPointer.cpp - This is a version written to link the nodes with pointers. Much closer to what a C++ programmer would write and I think easier to read. But, it turns out, signficantly slower.

# Results

## Large Problem

Running all three versions agains the "large" problem (166 strings, all 26 characters, 769 total characters) gives:
'''
Knuth_7_2_2_1_X.exe
        a f p l o
        b w i z
        c d r k x
        e n h t
        g y s q
        j u v m
Large problem solution took: 12 microseconds for setup and 190 microseconds to run and 4475 iterations.
Cover found:
        l o a f p
        z b w i
        d r k x c
        h t e n
        s q g y
        m j u v
Large problem with strings solution took: 78 microseconds for setup and 191 microseconds to run and 4475 iterations.
Cover found:
        l o a f p
        z b w i
        d r k x c
        h t e n
        s q g y
        m j u v
Large problem with pointers solution took: 139 microseconds for setup and 350 microseconds to run and 3045 iterations.
Done!
'''
Notice:

* The string and character versions are very close and have the same number of state changes.
* The pointers version is almost 2x slower. The nuber of state changes is lower because some cases are handled as
one (e.g. choose and cover).

## Very Large Problem

Running the string and pointer version on a "very large" problem gives:
```
Knuth_7_2_2_1_X.exe
Cover found:
        60 37 4 33 1
        22 43 10 31 48
        11 12 28
        13 41 9 19 20
        14 36 53 54
        15 38 16 27
        39 51 24 17 30
        44 58 18 26
        2 35 23 56
        21 32 49 47
        42 45 50 25
        8 29 5 55 57
        7 3 46 40
        6 52 34 59
Result: 1
Very large problem with strings solution took: 251 microseconds for setup and 3300769 microseconds to run.
Cover found:
        60 37 4 33 1
        22 43 10 31 48
        11 12 28
        13 41 9 19 20
        14 36 53 54
        15 38 16 27
        39 51 24 17 30
        44 58 18 26
        2 35 23 56
        21 32 49 47
        42 45 50 25
        8 29 5 55 57
        7 3 46 40
        6 52 34 59
Very large problem with pointers solution took: 428 microseconds for setup and 5927513 microseconds to run.
Done!

```
The scaling factor between the string and pointer versions remains about the same.

# Performance Comparison

Profiling with VTune shows the pointer version is much more memory bound, although the effect doesn't seem 
large enough to explain the performance difference.

## Very large problem, Strings version - VTune results

| "Function / Call Stack" | "CPU Time" | "Memory Bound" | "Loads"      | "Stores"    | "LLC Miss Count" | "Average Latency (cycles)" 
|-------------------------|------------|----------------|--------------|-------------|------------------|----------------------------
| "hide"                  | "0.922393" | "0"            | "1099832994" | "715021450" | "0"              | "7.04965"                  
| "unhide"                | "0.881464" | "0.0160775"    | "1150534515" | "665619968" | "0"              | "7.17829"                  
| "exact_cover_strings"   | "0.534069" | "0.105657"     | "694220826"  | "392611778" | "0"              | "7.10976"                  
| "cover"                 | "0.516101" | "0.0576591"    | "643519305"  | "267808034" | "0"              | "7"                        
| "uncover"               | "0.340407" | "0.068438"     | "565516965"  | "93602808"  | "0"              | "7.28846"                  
                                                                                                                                     


## Very large problem, Pointer version - VTune results

| Function / Call Stack             | CPU Time | Memory Bound | Loads     | Stores    | LLC Miss Count | Average Latency (cycles) 
|-----------------------------------|----------|--------------|-----------|-----------|----------------|--------------
| AlgXPointer::unlinkCellVertically | 0.746699 | 0.00581701   | 629218876 | 205406162 | 0              | 6.72131                  
| AlgXPointer::unlinkCellVertically | 0.687802 | 0.0787244    | 474514235 | 213206396 | 0              | 6.74138                  
| AlgXPointer::hide                 | 0.575996 | 0.155881     | 469314079 | 0         | 0              | 6.84091                  
| AlgXPointer::relinkCellVertically | 0.439235 | 0.0424649    | 156004680 | 163804914 | 0              | 6.61538                  
| AlgXPointer::unhide               | 0.433245 | 0            | 351010530 | 0         | 0              | 6.78788                  
| AlgXPointer::cover                | 0.419269 | 0.0789176    | 462813884 | 71502145  | 0              | 6.78723                  
| AlgXPointer::relinkCellVertically | 0.401301 | 0.0338767    | 175505265 | 120903627 | 0              | 6.69231                  
| AlgXPointer::unhide               | 0.311457 | 0.102153     | 223606708 | 0         | 0              | 6.86364                  
| AlgXPointer::relinkCellVertically | 0.302473 | 0.0869816    | 118303549 | 91002730  | 0              | 7.78571                  
| AlgXPointer::relinkCellVertically | 0.293489 | 0            | 141704251 | 92302769  | 0              | 6.44828                  
| AlgXPointer::exactCover           | 0.282508 | 0.0012048    | 210606318 | 98802964  | 0              | 6.94595                  
| AlgXPointer::uncover              | 0.255555 | 0.0453461    | 196305889 | 58501755  | 0              | 6.83333                  
| AlgXPointer::uncoverSeqItems      | 0.238584 | 0.100362     | 161204836 | 45501365  | 0              | 6.875                    
| AlgXPointer::coverSeqItems        | 0.194661 | 0.0928951    | 75402262  | 62401872  | 0              | 6.91667                  
| AlgXPointer::uncover              | 0.18967  | 0.017018     | 96202886  | 36401092  | 0              | 7                        
