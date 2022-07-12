// Knuth_7_2_2_1_X.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;


struct Cell
{
	union
	{
		int i;
		int x;
	};
	union
	{
		char name;
		int len;
		int top;
	};
	union
	{
		struct
		{
			int ulink;
			int dlink;
		};
		struct
		{
			int llink;
			int rlink;
		};
	};

};

int nstrings = 0;
int nstring_chars = 0;
int nunique = 0;


int char_indices[128]; // 1 based index of the character, or -1 if not used.

int ncells;
//Cell* headers;
Cell headers[39];
Cell* cells;

void get_counts(const char *pstrings[])
{
	memset(char_indices, 0xff, sizeof(char_indices));
	const char* pc;
	while((pc = pstrings[nstrings]) != nullptr)
	{
		nstrings++;
		while (*pc)
		{
			nstring_chars++;
			if (char_indices[*pc] < 0)
			{
				nunique++;
				char_indices[*pc] = 0;
			}
			pc++;
		}
	}

	// Indices should be assigned in sort order:
	int char_idx = 1;
	int* indices = char_indices;
	while (char_idx <= nunique)
	{
		if (*indices >= 0)
		{
			*indices = char_idx++;
		}
		indices++;
	}
	assert(char_idx == nunique + 1);


	// From page 67:
	// 7 chars in 6 strings with 16 total chars = 8 + 31 = 39 cells
	//                      = 1 + 7 (headers)
	//						+ 1 + 7 (length)
	//                      + 1 + 16  + 6

	ncells = 2 * (1 + nunique)  + 1 + nstring_chars + nstrings;
}

void init_cells(const char* pstrings[])
{
	assert(ncells == 39);
	//headers = new Cell[ncells];

	headers[0].i = 0;
	headers[0].name = -1;
	headers[0].llink = nunique;
	headers[0].rlink = 1;

	int index;
	for (index = 1; index <= nunique; index++)
	{
		headers[index].i = index;
		headers[index].llink = index - 1;
		headers[index].rlink = (index + 1) % (nunique + 1);
	}

	for (int nchar = 0; nchar < 128; nchar++)
	{
		if (char_indices[nchar] >= 0)
			headers[char_indices[nchar]].name = (char)nchar;
	}


	cells = headers + nunique + 1;
	memset(cells, 0xff, sizeof(Cell));
	cells[0].x = 0;

	index = 1;


	int x = 1;
	for (x = 1; x <= nunique; x++)
	{
		cells[index].x = x;
		cells[index].len = 0;
		cells[index].ulink = cells[index].dlink = -1;
		index++;
	}

	// First spacer
	cells[index].x = index;
	cells[index].top = 0;
	cells[index].ulink = cells[index].dlink = -1;
	index++;

	int idx_spacer = -1;
	int prev_first = index;		// First node after the previous spacer.

	for (int idx_str = 0; idx_str < nstrings; idx_str++)
	{
		const char* pc = pstrings[idx_str];
		while (*pc)
		{
			int idx_char = char_indices[*pc];

			cells[index].x = index;
			cells[index].top = idx_char;

			assert(cells[idx_char].x == idx_char);
			if (cells[idx_char].dlink < 0)	// list is empty for this character.
			{
				assert(cells[idx_char].len == 0);
				cells[idx_char].dlink = index;
				cells[idx_char].ulink = index;

				cells[index].ulink = idx_char;
				cells[index].dlink = idx_char;
			}
			else
			{
				cells[index].ulink = cells[idx_char].ulink;
				cells[index].dlink = idx_char;

				cells[cells[idx_char].ulink].dlink = index;
				cells[idx_char].ulink = index;
			}

			cells[idx_char].len++;

			index++;
			pc++;
		}

		// Add the spacer
		cells[index].x = index;
		cells[index].top = idx_spacer--;
		cells[index].ulink = prev_first;

		cells[prev_first - 1].dlink = index - 1;

		prev_first = index + 1;

		index++;
	}

}

void hide(int p)
{
	int q = p + 1;
	while (q != p)
	{
		int x = cells[q].top;
		int u = cells[q].ulink;
		int d = cells[q].dlink;

		if (x <= 0)
		{
			q = u;
		}
		else
		{
			cells[u].dlink = d;
			cells[d].ulink = u;
			cells[x].len--;

			q++;
		}
	}
}

void cover(int i)
{
	int p = cells[i].dlink;

	while (p != i)
	{
		hide(p);
		p = cells[p].dlink;
	}

	int l = headers[i].llink;
	int r = headers[i].rlink;

	headers[l].rlink = r;
	headers[r].llink = l;
}

void unhide(int p)
{
	int q = p - 1;
	
	while (q != p)
	{
		int x = cells[q].top;
		int u = cells[q].ulink;
		int d = cells[q].dlink;

		if (x <= 0)
		{
			q = d;
		}
		else
		{
			cells[u].dlink = q;
			cells[d].ulink = q;
			cells[x].len++;
			q--;
		}
	}
}

void uncover(int i)
{
	int l = headers[i].llink;
	int r = headers[i].rlink;

	headers[l].rlink = i;
	headers[r].llink = i;

	int p = cells[i].ulink;
	while (p != i)
	{
		unhide(p);
		p = cells[p].ulink;
	}
}

void print(ostream &stream)
{
	stream << "Using " << nstrings << " strings with " << nunique << " characters and " << nstring_chars << " total chracters." << endl;
	stream << "Gives " << ncells << " cells." << endl;

	// Code to put out a table that looks like Table 1 in 7.2.2.1:

	int nsep = (nunique + 1) * 5 + 8;
	char* separator = new char[nsep + 2];
	memset(separator, '_', nsep);
	separator[nsep] = '\n';
	separator[nsep + 1] = 0;

	stream << separator;

	stream << setw(8) << "i";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << headers[n].i;
	stream << endl << setw(8) << "NAME";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << headers[n].name;
	stream << endl << setw(8) << "LLINK";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << headers[n].llink;
	stream << endl << setw(8) << "RLINK";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << headers[n].rlink;

	stream << endl;
	stream << separator;

	stream << setw(8) << "x";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << cells[n].x;
	stream << endl << setw(8) << "LEN";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << cells[n].len;
	stream << endl << setw(8) << "ULINK";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << cells[n].ulink;
	stream << endl << setw(8) << "DLINK";
	for (int n = 0; n <= nunique; n++)
		stream << setw(5) << cells[n].dlink;

	stream << endl;
	stream << separator;

	for (int line_start = 2 * (nunique + 1); line_start < ncells; line_start += nunique + 1)
	{
		int line_cell_count = min(nunique + 1, ncells - line_start);

		stream << setw(8) << "x";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << headers[line_start + n].x;
		stream << endl << setw(8) << "TOP";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << headers[line_start + n].top;
		stream << endl << setw(8) << "ULINK";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << headers[line_start + n].ulink;
		stream << endl << setw(8) << "DLINK";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << headers[line_start + n].dlink;

		stream << endl;
		stream << separator;
	}


	delete[] separator;
}

void exact_cover(const char* pstrings[])
{
	get_counts(pstrings);
	init_cells(pstrings);

	print(cout);

	cover(2);
	uncover(2);
	print(cout);
}


#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	const char* strings[] =
	{
		"ce",
		"adg",
		"bcf",
		"adf",
		"bg",
		"deg",
		nullptr
	};
	exact_cover(strings);


    std::cout << "Done!\n";
}
