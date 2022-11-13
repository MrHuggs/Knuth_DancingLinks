//
// Algorithm modified to support string values (instead of just a char).
//
//
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <map>
#include <chrono>

#include "Common.h"
#include "ProblemGenerator.h"
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
		const char* pName;
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
// Special comparator so we can make a map of string pointers eliminating
// duplicates:
struct CmpSame
{
	bool operator()(const char* p1, const char* p2) const
	{
		return strcmp(p1, p2) < 0;
	}
};
///////////////////////////////////////////////////////////////////////////////
static  int nsequences;
static  int nsequence_items;		// Total number of characters in all strings
static  int nunique_items;			// Number of unique characters.

static map<const char*, int, CmpSame>* pitem_indices;
static map<int ,const char*>* pindex_items;

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

static void get_counts(const vector< vector<const char *> > &sequences)
{
	nsequences = (int) sequences.size();
	nsequence_items = 0;

	pitem_indices = new map<const char*, int, CmpSame>();
	pindex_items = new map<int, const char*>();

	nunique_items = 0;

	// We will record the item names, sort them, and then assign id's in sort order.
	// This is not really needed, but matches what Knuth_7_2_2_1_X does.
	vector<const char*> item_names;

	for (int i= 0; i < nsequences; i++)
	{
		const vector<const char*>& seq = sequences[i];
		nsequence_items += (int) seq.size();

		for (const char *pc : seq)
		{
			if (pitem_indices->find(pc) == pitem_indices->end())
			{
				nunique_items++;
				item_names.push_back(pc);
				(*pitem_indices)[pc] = 0;	// We'll figure out id's later
			}
		}
	}

	sort(item_names.begin(), item_names.end(), [](const char* pa, const char* pb) { return strcmp(pa, pb) < 0; });

	for (int i = 0; i <item_names.size(); i++)
	{
		int id = i + 1;	// Id's in Knuth's scheme start at 1.
		(*pitem_indices)[item_names[i]] = id;
		(*pindex_items)[id] = item_names[i];
	}

	// Calculate the total number of cells needed.
	//
	// An example from page 67:
	// 7 chars in 6 strings with 16 total chars = 8 + 31 = 39 cells
	//                      = 1 + 7 (headers)
	//						+ 1 + 7 (length)
	//                      + 1 + 16  + 6
	ncells = 2 * (1 + nunique_items) + 1 + nsequence_items + nsequences;
}
///////////////////////////////////////////////////////////////////////////////
// Initialized the header & cell data as in Table 1:
static void init_cells(const vector< vector<const char*> >& sequences)
{
	headers = new Cell[ncells];

	headers[0].i = 0;
	headers[0].pName = "";
	headers[0].llink = nunique_items;
	headers[0].rlink = 1;

	// Fill out the headers, starting with the special 0 elements:
	int index;
	for (index = 1; index <= nunique_items; index++)
	{
		headers[index].i = index;
		headers[index].llink = index - 1;
		headers[index].rlink = (index + 1) % (nunique_items + 1);
		headers[index].pName = (*pindex_items)[index];

	}

	//First line of the cell data:
	cells = headers + nunique_items + 1;
	memset(cells, 0xff, sizeof(Cell));

	// Special 0 element:
	cells[0].x = 0;

	index = 1;
	int x = 1;
	for (x = 1; x <= nunique_items; x++)
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

	for (int idx_seq = 0; idx_seq < nsequences; idx_seq++)
	{
		const vector<const char*> ptr_vec = sequences[idx_seq];
		for (auto pc : ptr_vec)
		{
			int idx_char = (*pitem_indices)[pc];

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
	delete pitem_indices;
	delete pindex_items;
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
static void format(ostream& stream)
{
	stream << "Using " << nsequences << " sequences with " << nunique_items << " unique strings and " << nsequence_items << " total strings." << endl;
	stream << "Gives " << ncells << " cells." << endl;


	int nsep = (nunique_items + 1) * 5 + 8;
	char* separator = new char[nsep + 2];
	memset(separator, '_', nsep);
	separator[nsep] = '\n';
	separator[nsep + 1] = 0;

	stream << separator;

	stream << setw(8) << "i";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << headers[n].i;
	stream << endl << setw(8) << "NAME";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << headers[n].pName;
	stream << endl << setw(8) << "LLINK";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << headers[n].llink;
	stream << endl << setw(8) << "RLINK";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << headers[n].rlink;

	stream << endl;
	stream << separator;

	stream << setw(8) << "x";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << cells[n].x;
	stream << endl << setw(8) << "LEN";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << cells[n].len;
	stream << endl << setw(8) << "ULINK";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << cells[n].ulink;
	stream << endl << setw(8) << "DLINK";
	for (int n = 0; n <= nunique_items; n++)
		stream << setw(5) << cells[n].dlink;

	stream << endl;
	stream << separator;

	for (int line_start = 2 * (nunique_items + 1); line_start < ncells; line_start += nunique_items + 1)
	{
		int line_cell_count = min(nunique_items + 1, ncells - line_start);

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
	format(cout);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Given the id of a cell in a sequence, output the whole sequence.
static const char *format_sequence(int q)
{
	// Fast forward to the separator:
	do
	{
		q++;
	} while (cells[q].top > 0);

	// Start of the sequence is the separator's ulink:
	q = cells[q].ulink;

	const int bufsize = 256;
	static char buf[bufsize];

	buf[0] = 0;
	size_t curlen = 0;

	do
	{
		auto n = strlen(headers[cells[q].top].pName);

		if (curlen + n + 2 > bufsize)
		{
			assert(false);
			return "Error: out of buffer space! ";
		}

		memcpy(buf + curlen, headers[cells[q].top].pName, n);
		curlen += n;

		buf[curlen] = ' ';
		curlen++;

		q++;

	} while (cells[q].top > 0);

	buf[curlen] = 0;
	return buf;
}
///////////////////////////////////////////////////////////////////////////////
//
// Main function: print the exact cover of a null-terminated array of string.
//
bool exact_cover_strings(const vector< vector<const char*> >& sequences)
{
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;

	bool rval = false;

	start_time = std::chrono::high_resolution_clock::now();
	get_counts(sequences);
	init_cells(sequences);
	setup_complete = std::chrono::high_resolution_clock::now();
	loop_count = 0;

	//print();		// If you want to see Table 1.

	int i, p, l = -1;

	// This stores the index of our choice at each level.
	int* x = new int[nunique_items];

	for (;;)
	{
		loop_count++;
		TRACE("%lli:%i - %s\n", loop_count, l, StateName(state));

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

			TRACE("\t\t%i - covering: %s\n", l, headers[i].pName);
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

			TRACE("\t\t%i - trying: %s\n", l, format_sequence(p));

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
				cout << "Cover found:" << endl;
				// To output the actual strings:
				for (int lout = 0; lout < l; lout++)
				{
					// c will be the cell index of a character in the string we chose.
					int c = x[lout];
					cout << "\t" << format_sequence(c) << endl;
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



bool exact_cover_strings(const char* pstrings[])
{
	bool rval;
	assert(_CrtCheckMemory());

	{
		ConvertedCharProblem converted(pstrings);
		rval = exact_cover_strings(converted.stringPointers);
	}

	assert(_CrtCheckMemory());

	return rval;
}

///////////////////////////////////////////////////////////////////////////////
const char* small_char_problems[];
const char* large_char_problem[];
const char* vary_large_char_problem[];

void x_small_problems()
{

	bool b;

	const char** pstrings = small_char_problems;
	while (*pstrings)
	{
		b = exact_cover_strings(pstrings);
		assert(b);
		while (*(pstrings++));
	}
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void x_large_problem()
{
	ConvertedCharProblem converted(large_char_problem);
	//converted.Print();

	bool b = exact_cover_strings(converted.stringPointers);

	assert(b);

	auto setup_dt = std::chrono::duration_cast<std::chrono::microseconds>(setup_complete - start_time);
	auto run_dt = std::chrono::duration_cast<std::chrono::microseconds>(run_complete - setup_complete);

	cout << "Large problem with strings solution took: " << setup_dt.count() << " microseconds for setup and " <<
		run_dt.count() << " microseconds to run and " << loop_count << " iterations.\n";
}
///////////////////////////////////////////////////////////////////////////////
void x_very_large_problem()
{
	ProblemGenerator generator;
	generator.Generate();

	bool b = exact_cover_strings(generator.Sequences);

	cout << "Result: " << b << endl;

	auto setup_dt = std::chrono::duration_cast<std::chrono::microseconds>(setup_complete - start_time);
	auto run_dt = std::chrono::duration_cast<std::chrono::microseconds>(run_complete - setup_complete);

	cout << "Very large problem with strings solution took: " << setup_dt.count() << " microseconds for setup and " <<
		run_dt.count() << " microseconds to run.\n";
}



