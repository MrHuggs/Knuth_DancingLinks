#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <chrono>

using namespace std;

// From main.cpp:
const char* small_char_problems[];
const char* large_char_problem[];

#include "Common.h"
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
static int nstrings;			// Number of strings passed in
static int nstring_chars;		// Total number of characters in all strings
static int nunique;			// Number of unique characters.

// Looking table to allow finding the index of a character. Contains the 1 based index of the character,
// or -1 if the character is not not used.
static int char_indices[128];

static int ncells;		// Total number of cells:
static Cell* headers;	// Pointer to the headers (i/name/llink/rlink) in Table 1.
static Cell* cells;
///////////////////////////////////////////////////////////////////////////////
/// For performance timing:
static chrono::steady_clock::time_point start_time;
static chrono::steady_clock::time_point setup_complete;
static chrono::steady_clock::time_point run_complete;
static long long loop_count;
///////////////////////////////////////////////////////////////////////////////
// First step of analysis: Find out how many strings & unique characters there are:
static void get_counts(const char *pstrings[])
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

	// Populate the reverse lookup table:
	// Indices will be assigned in sort order:
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
static void init_cells(const char* pstrings[])
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
static void destroy_cells()
{
	delete[] headers;
}
///////////////////////////////////////////////////////////////////////////////
// hide/cover/uncover/unhide functions:
static void hide(int p)
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


static void cover(int i)
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

static void unhide(int p)
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

static void uncover(int i)
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
static void format(ostream &stream)
{
	stream << "Using " << nstrings << " strings with " << nunique << " characters and " << nstring_chars << " total characters." << endl;
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

// An immediate version of format, which you could call inside the debugger.
static void print()
{
	ostringstream  s;
	format(s);
	std::string str_result = s.str();

	// Outputting very large strings inside the debugger sometimes has bad results,
	// so output a line at a time.

	istringstream istream(str_result);
	std::string line;
	while (std::getline(istream, line))
	{
		const char* pc = line.c_str();
		puts(pc);
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// Main function: print the exact cover of a null-terminated array of string.
//
bool exact_cover(const char* pstrings[])
{
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;

	bool rval = false;

	start_time = std::chrono::high_resolution_clock::now();
	get_counts(pstrings);
	init_cells(pstrings);
	setup_complete = std::chrono::high_resolution_clock::now();
	loop_count = 0;

#ifdef TRACE_PICKS
	print();		// If you want to see Table 1.
#endif

	int i, p, l;

	// This stores the index of our choice at each level. We need no more than
	// the number of unique characters.
	int *x = new int[nunique];

	for (;;)
	{
		loop_count++;
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

			run_complete = std::chrono::high_resolution_clock::now();

			if (rval == true)
			{
				// To output the actual strings:
				for (int lout = 0; lout < l; lout++)
				{
					// c will be the cell index of a character in the string we chose.
					// Loop through the characters in the string as we do in hide.
					int c = x[lout];
					int q = c;
					cout << "\t";
					do
					{
						cout << headers[cells[q].top].name << " ";
						
						q++;
						if (cells[q].top <= 0)
						{
							q = cells[q].ulink;		// We hit the spacer. ulink is start of the string.
						}

					} while (q != c);
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

void small_problems()
{

	bool b;

	const char** pstrings = small_char_problems;
	while (*pstrings)
	{
		b = exact_cover(pstrings);
		assert(b);
		while (*(pstrings++));
	}
	cout << b << endl;

}

void large_problem()
{

	auto start = std::chrono::high_resolution_clock::now();

	bool b = exact_cover(large_char_problem);

	auto finish = std::chrono::high_resolution_clock::now();
	assert(b);


	auto setup_dt = std::chrono::duration_cast<std::chrono::microseconds>(setup_complete - start_time);
	auto run_dt = std::chrono::duration_cast<std::chrono::microseconds>(run_complete - setup_complete);

	cout << "Large problem solution took: " << setup_dt.count() << " microseconds for setup and " <<
		run_dt.count() << " microseconds to run and " << loop_count << " iterations.\n";
}
