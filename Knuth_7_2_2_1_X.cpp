// Knuth_7_2_2_1_X.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Cell structure to match 7.2.2.1 Table 1:
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
///////////////////////////////////////////////////////////////////////////////
int nstrings;			// Number of strings passed in
int nstring_chars;		// Total number of characters in all strings
int nunique;			// Number of unique characters.

// Looking table to allow finding the index of a character. Contains the 1 based index of the character,
// or -1 if the character is not not used.
int char_indices[128]; 

int ncells;		// Total number of cells:
Cell* headers;	// Pointer to the headers (i/name/llink/rlink) in Table 1.
Cell* cells;
///////////////////////////////////////////////////////////////////////////////
// First step of analysis: Find out how many strings & unique characters there are:
void get_counts(const char *pstrings[])
{
	nstrings = nstring_chars = nunique = 0;
	const char* pc;

	// Reset the indices array. First step is marking characters that are used:
	memset(char_indices, 0xff, sizeof(char_indices));

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

	// Populate the reverse looking table:
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


	// Calculate the total number of cells needed.
	//
	// An example from page 67:
	// 7 chars in 6 strings with 16 total chars = 8 + 31 = 39 cells
	//                      = 1 + 7 (headers)
	//						+ 1 + 7 (length)
	//                      + 1 + 16  + 6
	ncells = 2 * (1 + nunique)  + 1 + nstring_chars + nstrings;
}
///////////////////////////////////////////////////////////////////////////////
// Initialized the header & cell data as in Table 1:
void init_cells(const char* pstrings[])
{
	headers = new Cell[ncells];

	headers[0].i = 0;
	headers[0].name = -1;
	headers[0].llink = nunique;
	headers[0].rlink = 1;

	// Fill out the headers, starting with the special 0 elements:
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

	//First line of the cell data:

	cells = headers + nunique + 1;
	memset(cells, 0xff, sizeof(Cell));

	// Special 0 element:
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

	// Remaining lines, starting with the special spacer:
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

		// Add the spacer at the end:
		cells[index].x = index;
		cells[index].top = idx_spacer--;
		cells[index].ulink = prev_first;

		cells[prev_first - 1].dlink = index - 1;

		prev_first = index + 1;

		index++;
	}

}
///////////////////////////////////////////////////////////////////////////////
// Free any allocated memory:
void destroy_cells()
{
	delete[] headers;
}
///////////////////////////////////////////////////////////////////////////////
// hide/cover/uncover/unhide functions:
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
///////////////////////////////////////////////////////////////////////////////
// Helper to output a table that looks like Table 1 in 7.2.2.1:
void format(ostream &stream)
{
	stream << "Using " << nstrings << " strings with " << nunique << " characters and " << nstring_chars << " total chracters." << endl;
	stream << "Gives " << ncells << " cells." << endl;


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

void print()
{
	ostringstream  s;
	format(s);
	cout << s.str();
}
///////////////////////////////////////////////////////////////////////////////
// Helper to diff to strings. Useful for the comparing the output of fomat
// above.
bool print_diff(string s1, string s2)
{
	const char* p1 = s1.c_str();
	const char* p2 = s2.c_str();

	string result;
	bool equal = true;
	while (*p1 && *p2)
	{
		if (*p1 == *p2)
			result.push_back(*p1);
		else
		{
			result.push_back('*');
			equal = false;
		}
		p1++;
		p2++;
	}
	while (*p1)
	{
		equal = false;
		result.push_back(*p1++);
	}
	while (*p1)
	{
		equal = false;
		result.push_back(*p2++);
	}

	if (!equal)
	{
		cout << "String differ\n";
		cout << result;
	}
	else
	{
		//cout << "String the same!\n";
	}

	return equal;
}
///////////////////////////////////////////////////////////////////////////////
// States of the algorithm. Corresponds to X1-X8.
// Note that some of these could be combined.
enum AlgXStates
{
	ax_Initialize,
	ax_EnterLevel,
	ax_Choose,
	ax_Cover,
	ax_TryX,
	ax_TryAgain,
	ax_Backtrack,
	ax_LeaveLevel,
	ax_Cleanup,
};
///////////////////////////////////////////////////////////////////////////////
//
// Main function: print the exact cover of a null-terminated array of string.
//
bool exact_cover(const char* pstrings[])
{
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;

	bool rval = false;

	get_counts(pstrings);
	init_cells(pstrings);

	//print();		// If you want to see Table 1.

	int i, p, l;

	// This stores the index of our choice at each level.
	int *x = new int[nstrings];

	for (;;)
	{
		switch (state)
		{
		case ax_Initialize:
			l = 0;
			state = ax_EnterLevel;
			break;

		case ax_EnterLevel:
			if (headers[0].rlink == 0)
			{
				rval = true;
				state = ax_Cleanup;
			}
			else
				state = ax_Choose;
			break;

		case ax_Choose:
			i = headers[0].rlink;
			state = ax_Cover;
			break;

		case ax_Cover:
			cover(i);
			x[l] = cells[i].dlink;
			state = ax_TryX;
			break;

		case ax_TryX:
			if (x[l] == i)
			{
				state = ax_Backtrack;
				break;
			}

			p = x[l] + 1;
			while (p != x[l])
			{
				int j = cells[p].top;
				if (j <= 0)
					p = cells[p].ulink;
				else
				{
					cover(j);
					p++;
				}
			}
			l++;
			state = ax_EnterLevel;
			break;

		case ax_TryAgain:
			p = x[l] - 1;

			while (p != x[l])
			{
				int j = cells[p].top;
				if (j <= 0)
					p = cells[p].dlink;
				else
				{
					uncover(j);
					p--;
				}
			}

			i = cells[x[l]].top;
			x[l] = cells[x[l]].dlink;
			state = ax_TryX;
			break;
		case ax_Backtrack:
			uncover(i);
			state = ax_LeaveLevel;
			break;
		case ax_LeaveLevel:
			if (l == 0)
			{
				state = ax_Cleanup;
			}
			else
			{
				l--;
				state = ax_TryAgain;
			}
			break;

		case ax_Cleanup:
			if (rval == true)
			{
				// To output the actual strings:
				for (int lout = 0; lout < l; lout++)
				{
					// c will be the cell index of the first character in the string we chose.
					// that was chosen. Output the character corresponding to that cell, and move to the
					// next character (which has index+1). Continue until we hit the next spacer
					// which will have top <= 0.
					int c = x[lout];

					while (cells[c].top > 0)
					{
						cout << headers[cells[c].top].name << " ";
						c++;
					}
					cout << endl;
				}
			}
			else
				cout << "FAILED!\n";

			assert(_CrtCheckMemory());

			delete[] x;
			destroy_cells();

			return rval;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
// Unit test for cover/uncover.
bool test_cover_uncover(const char* pstrings[])
{
	get_counts(pstrings);
	init_cells(pstrings);

	ostringstream  before, after;
	format(before);

	bool pass = true;
	for (int i = 1; i <= nunique; i++)
	{
		cover(i);
		uncover(i);
		format(after);
		if (!print_diff(before.str(), after.str()))
		{
			if (pass == true)
			{
				pass = false;
				cout << before.str();
			}
			cout << after.str();
		}
	}
	return pass;
}

void unit_test()
{
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
	test_cover_uncover(strings);
}
///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	unit_test();

	bool b;

	const char * strings[] =
	{
		"a", nullptr,
		"a", "b", nullptr,
		"a","ab","c", nullptr,
		"ce", "adg", "bcf",	"adf", "bg", "deg", nullptr,
		"147", "14", "457", "356", "2367", "27", nullptr,
		nullptr
	};
	const char** pstrings = strings;
	while (*pstrings)
	{
		b = exact_cover(pstrings);
		assert(b);
		while (*(pstrings++));
	}
	cout << b << endl;


    cout << "Done!\n";
}
