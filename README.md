# Choose Min

This branch implements the chose min heuristic for searching:

When we are at the ax_Choose state, we pick the item that appears in the smallest 
number of sequences.

Aside from the ax_Choose code, the linking for cells had to be changed to a circular list.

# Results

## Very Large Problem

'''
Cover found:
        28 12 8 51 33
        15 47 45 23 4
        42 34 11 3 31
        29 27 13 59
        9 53 52 60
        41 17 10 56
        50 35 21 30
        20 14 57 46 48
        32 1 49 19
        16 54 44 39 22
        26 18 6 36 5
        2 40 58
        43 55 25 37
        38 7 24
Result: 1
Very large problem with strings solution took: 255 microseconds for setup and 55828 microseconds to run and 864003 iterations.
Cover found:
        8 51 33 28 12
        15 47 45 23 4
        11 3 31 42 34
        59 29 27 13
        52 60 9 53
        10 56 41 17
        30 50 35 21
        20 14 57 46 48
        1 49 19 32
        22 16 54 44 39
        18 6 36 5 26
        2 40 58
        25 37 43 55
        24 38 7
Very large problem with pointers solution took: 1112 microseconds for setup and 67187 microseconds to run and 606577 iterations.
Done!
'''
Compare to 3300769/5927513 microseconds before!
                  
