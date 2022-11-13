# Program to read the output of the Partridge problem and plot
# the results squares.


import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import re

def read_file(file_name):
    with open(file_name) as f:
        while(True):
            line = f.readline();

            if line == '':
                print("Start token not found.")
                quit(-1)

            if line.find("Found") != -1:
                break

        lines = f.readlines()

    #print(lines)

    sets = []

    maxval = 1
    squares = None

    for line in lines:

        if line.rfind("Solution:") != -1:
            squares = []
            continue

        if line.rfind("(end solution)") != -1:
            sets.append(squares);
            squares = None
            continue

        m = re.search('.*#(\d+) *\((\d+),(\d+)\)', line)

        if m is None:
            break

        #print(m.group(1), m.group(2), m.group(3))
        square_size = int(m.group(1))
        row = int(m.group(2))
        column = int(m.group(3))

        maxval = max(maxval, row + square_size)
        maxval = max(maxval, column + square_size)

        #print(square_size, row, column)

        squares.append((square_size, (row, column)))

    return maxval, sets

def plot_squares(maxval, sets):
    for squares in sets:
        fig, ax = plt.subplots(figsize =(6,6))

        colors = ['b', 'g', 'r', 'c', 'm', 'y']

        for square_size, (row, column) in squares:
            print(square_size, row, column)
            ax.add_patch(Rectangle((column, row), square_size, square_size,
                                   edgecolor='black',
                                   facecolor=colors[square_size % len(colors)],
                                   fill=True,
                                   lw=4))


        # display plot
        plt.xlim([-1, maxval + 1])
        plt.ylim([-1, maxval + 1])
        plt.axis('off')
    plt.show()

m, squares = read_file(r"Partridge8.txt")
print("Read file successfully.")

plot_squares(m, squares)


