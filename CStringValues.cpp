//
// Algorithm modified to support string values (instead of just a char).
//
//
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
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
			int color;
		};
		struct
		{
			int llink;
			int rlink;
		};
	};
};


///////////////////////////////////////////////////////////////////////////////
static const ExactCoverWithColors* pproblem;

static  int nsequences;
static  int nsequence_items;		// Total number of characters in all strings
static  int nprimary_items;
static  int nsecondary_items;

static map<const char*, int, CmpSame>* pitem_indices;
static map<const char*, int, CmpSame>* pcolor_indices;

int max_item_len;
char* pitem_buf;

static int ncells;		// Total number of cells:
static Cell* headers;	// Pointer to the headers (i/name/llink/rlink) in Table 1.
static Cell* cells;
///////////////////////////////////////////////////////////////////////////////
/// For performance timing:
static chrono::steady_clock::time_point start_time;
static chrono::steady_clock::time_point setup_complete;
static chrono::steady_clock::time_point run_complete;
static long long loop_count;

static void get_counts()
{

	pitem_indices = new map<const char*, int, CmpSame>();
	pcolor_indices = new map<const char*, int, CmpSame>();
	max_item_len = 0;
	
	nprimary_items = (int) pproblem->primary_options.size();
	nsecondary_items = (int) pproblem->secondary_options.size();

	int item_id = 1; // Id's in Knuth's scheme start at 1.
	for (auto primary : pproblem->primary_options)
	{
		max_item_len = max(max_item_len, (int) strlen(primary));
		(*pitem_indices)[primary] = item_id++;
	}

	int idx_color = 1;
	for (auto color : pproblem->colors)
	{
		(*pcolor_indices)[color] = idx_color++;
	}

	for (auto secondary : pproblem->secondary_options)
	{
		max_item_len = max(max_item_len, (int)strlen(secondary));
		(*pitem_indices)[secondary] = item_id++;
	}

	nsequences = (int)pproblem->sequences.size();
	nsequence_items = 0;
	for (auto sequence : pproblem->sequences)
	{
		nsequence_items += (int) sequence.size();
	}

	// Calculate the total number of cells needed. Not that this is different
	// from Algorithm X
	//
	ncells = 2 * (2 + nprimary_items + nsecondary_items) + nsequence_items + nsequences;
}
///////////////////////////////////////////////////////////////////////////////
// Initialized the header & cell data as in Table 1:

// Value to stick in unused field. Don't really need this, but it makes things
// print more nicely:
static const int unused = -99;

static void init_cells()
{
	headers = new Cell[ncells];
	pitem_buf = new char[max_item_len + 1];

	headers[0].i = 0;
	headers[0].pName = "";
	headers[0].llink = nprimary_items;
	headers[0].rlink = 1;

	int index;

	// Headers for primary options
	for (int i = 0; i < nprimary_items; i++)
	{
		index = i + 1;
		headers[index].i = index;
		headers[index].llink = index - 1;
		headers[index].rlink = (index + 1) % (nprimary_items + 1);
		headers[index].pName = pproblem->primary_options[i];
	}

	// Headers for secondary options:
	for (int i = 0; i < nsecondary_items; i++)
	{
		index = i + nprimary_items + 1;
		headers[index].i = index;
		headers[index].llink = (i == 0) ? (nprimary_items + nsecondary_items + 1) : index - 1;
		headers[index].rlink = index + 1;
		headers[index].pName = pproblem->secondary_options[i];
	}

	// Secondary sentinel:
	index = nprimary_items + nsecondary_items + 1;
	headers[index].i = index;
	headers[index].pName = "";
	headers[index].llink = index - 1;
	headers[index].rlink = nprimary_items + 1;

	//First line of the cell data:

	cells = headers + nprimary_items + nsecondary_items + 2;

	// Special 0 element:
	cells[0].x = 0;
	cells[0].len = cells[0].ulink = cells[0].dlink = cells[0].color = unused;


	for (int i = 0; i  < nprimary_items; i++)
	{
		index = i + 1;
		cells[index].x = index;
		cells[index].len = 0;
		cells[index].ulink = cells[index].dlink = unused;
		cells[index].color = unused;
		index++;
	}

	for (int i = 0; i < nsecondary_items; i++)
	{
		index = i + nprimary_items + 1;
		cells[index].x = index;
		cells[index].len = 0;
		cells[index].ulink = cells[index].dlink = -1;
		cells[index].color = unused;
		index++;
	}

	// Special sentinel:
	index = nprimary_items + nsecondary_items + 1;
	cells[index].x = index;
	cells[index].len = 0;
	cells[index].ulink = unused;
	cells[index].dlink = -1;
	cells[index].color = 0;
	index++;

	int idx_spacer = -1;
	int prev_first = index;		// First node after the previous spacer.

	

	for (int idx_seq = 0; idx_seq < nsequences; idx_seq++)
	{
		const vector<const char*> ptr_vec = pproblem->sequences[idx_seq];
		for (auto pc : ptr_vec)
		{
			// Look at the sequence item for the separator, which would indicate this item
			// has an assigned color:
			const char* sep = strchr(pc, ':');
			int idx_item, idx_color;
			if (sep)
			{
				int nc = (int) (sep - pc);
				memcpy(pitem_buf, pc, nc);
				pitem_buf[nc] = 0;
				idx_item = (*pitem_indices)[pitem_buf];
				idx_color = (*pcolor_indices)[sep + 1];
			}
			else
			{
				idx_item = (*pitem_indices)[pc];
				idx_color = 0;
			}
			

			cells[index].x = index;
			cells[index].top = idx_item;

			assert(cells[idx_item].x == idx_item);
			if (cells[idx_item].dlink < 0)	// list is empty
			{
				assert(cells[idx_item].len == 0);
				cells[idx_item].dlink = index;
				cells[idx_item].ulink = index;

				cells[index].ulink = idx_item;
				cells[index].dlink = idx_item;
			}
			else
			{
				cells[index].ulink = cells[idx_item].ulink;
				cells[index].dlink = idx_item;

				cells[cells[idx_item].ulink].dlink = index;
				cells[idx_item].ulink = index;
			}

			cells[index].color = idx_color;

			cells[idx_item].len++;

			index++;
			pc++;
		}

		// Add the spacer at the end:
		cells[index].x = index;
		cells[index].top = idx_spacer--;
		cells[index].ulink = prev_first;
		cells[index].dlink = unused;
		cells[index].color = 0;

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
	delete[] pitem_buf;
	delete pitem_indices;
	delete pcolor_indices;
}
///////////////////////////////////////////////////////////////////////////////
// hide/cover/uncover/unhide functions:
static void hide_p(int p)
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
			if (cells[q].color >= 0)
			{
				cells[u].dlink = d;
				cells[d].ulink = u;
				cells[x].len--;
			}

			q++;
		}
	}
}


static void cover_p(int i)
{
	int p = cells[i].dlink;

	while (p != i)
	{
		hide_p(p);
		p = cells[p].dlink;
	}

	int l = headers[i].llink;
	int r = headers[i].rlink;

	headers[l].rlink = r;
	headers[r].llink = l;
}

static void unhide_p(int p)
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
			if (cells[q].color >= 0)
			{
				cells[u].dlink = q;
				cells[d].ulink = q;
				cells[x].len++;
			}
			q--;
		}
	}
}

static void uncover_p(int i)
{
	int l = headers[i].llink;
	int r = headers[i].rlink;

	headers[l].rlink = i;
	headers[r].llink = i;

	int p = cells[i].ulink;
	while (p != i)
	{
		unhide_p(p);
		p = cells[p].ulink;
	}
}

void purify(int p)
{
	int c = cells[p].color;
	int i = cells[p].top;

	cells[i].color = c;
	int q = cells[i].dlink;

	while (q != i)
	{
		if (cells[q].color == c)
		{
			cells[q].color = -1;
		}
		else
		{
			hide_p(q);
		}

		q = cells[q].dlink;
	}
}

void commit(int p, int j)
{
	int c = cells[p].color;
	if (c == 0)
	{
		cover_p(j);
	}
	else if (c > 0)
	{
		purify(p);
	}
}

void unpurify(int p)
{
	int c = cells[p].color;
	int i = cells[p].top;
	int q = cells[i].ulink;

	while (q != i)
	{
		if (cells[q].color < 0)
		{
			cells[q].color = c;
		}
		else
		{
			unhide_p(q);
		}

		q = cells[q].ulink;
	}
}

void uncommit(int p, int j)
{
	int c = cells[p].color;
	if (c == 0)
	{
		uncover_p(j);
	}
	else if (c > 0)
	{
		unpurify(p);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper to output a table that looks like Table 2 in 7.2.2.1:
static void format(ostream& stream)
{
	stream << "Using " << nsequences << " sequences with " << nprimary_items << " primary strings, " << nsecondary_items << " secondary strings and " << nsequence_items << " total strings." << endl;
	stream << "Gives " << ncells << " cells." << endl;

	int nunique_items = nprimary_items + nsecondary_items;

	int nsep = (nunique_items + 2) * 5 + 8;
	char* separator = new char[nsep + 2];
	memset(separator, '_', nsep);
	separator[nsep] = '\n';
	separator[nsep + 1] = 0;

	stream << separator;

	stream << setw(8) << "i";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << headers[n].i;
	stream << endl << setw(8) << "NAME";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << headers[n].pName;
	stream << endl << setw(8) << "LLINK";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << headers[n].llink;
	stream << endl << setw(8) << "RLINK";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << headers[n].rlink;

	stream << endl;
	stream << separator;

	stream << setw(8) << "x";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << cells[n].x;
	stream << endl << setw(8) << "LEN";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << cells[n].len;
	stream << endl << setw(8) << "ULINK";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << cells[n].ulink;
	stream << endl << setw(8) << "DLINK";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << cells[n].dlink;
	stream << endl << setw(8) << "COLOR";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << cells[n].color;

	stream << endl;
	stream << separator;

	for (int line_start = 2 * (nunique_items + 2); line_start < ncells; line_start += nunique_items + 2)
	{
		int line_cell_count = min(nunique_items + 2, ncells - line_start);

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
		stream << endl << setw(8) << "COLOR";
		for (int n = 0; n < line_cell_count; n++)
		{
			int color = headers[line_start + n].color;
			if (color <= 0)
				stream << setw(5) << headers[line_start + n].color;
			else
				stream << setw(5) << pproblem->colors[headers[line_start + n].color - 1];
		}

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
		int idx_item = cells[q].top;
		const char* pitem_name;
		if (idx_item <= nprimary_items)
		{
			pitem_name = pproblem->primary_options[idx_item - 1];
		}
		else
		{
			pitem_name = pproblem->secondary_options[idx_item - nprimary_items - 1];
		}

		auto n = strlen(pitem_name);

		int nc;
		const char* pcolor_name = "";
		int color = cells[cells[q].top].color;
		if (color > 0)
		{
			pcolor_name = pproblem->colors[color - 1];
		}
		nc = (int)strlen(pcolor_name);


		if (curlen + n + nc + 2 > bufsize)
		{
			assert(false);
			return "Error: out of buffer space! ";
		}

		memcpy(buf + curlen, pitem_name, n);
		curlen += n;

		if (nc > 0)
		{
			buf[curlen++] = ':';
			memcpy(buf + curlen, pcolor_name, nc);
			curlen += nc;
		}

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
bool exact_cover_strings(const ExactCoverWithColors& problem)
{
	//problem.print();
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;

	pproblem = &problem;

	bool rval = false;

	start_time = std::chrono::high_resolution_clock::now();
	get_counts();
	init_cells();
	setup_complete = std::chrono::high_resolution_clock::now();
	loop_count = 0;

	//print();		// If you want to see Table 1.
	int i, p, l = -1;

	// This stores the index of our choice at each level.
	int* x = new int[nprimary_items + nsecondary_items];

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
		{
			i = headers[0].rlink;

			// A good heuristic for picking the next choice is to pick the item
			// present in the smallest number of sequences. In a few tests, this
			// Seems to make a huge difference.
#define CHOOSE_MIN
#ifdef CHOOSE_MIN
			int lbest = cells[i].len;
			int inext = headers[i].rlink;

			while (inext != 0)
			{
				if (cells[inext].len < lbest)
				{
					i = inext;
					lbest = cells[i].len;
				}
				inext = headers[inext].rlink;
			}
#endif

			state = ax_Cover;
			break;
		}

		case ax_Cover:
			cover_p(i);
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
					commit(p, j);
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
					uncommit(p, j);
					p--;
				}
			}

			i = cells[x[l]].top;
			x[l] = cells[x[l]].dlink;
			state = ax_TryX;
			break;
		case ax_Backtrack:
			uncover_p(i);
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
				//print();
				// To output the actual strings:
				for (int lout = 0; lout < l; lout++)
				{
					// c will be the cell index of a character in the string we chose.
					int c = x[lout];
					cout << "\t" << format_sequence(c) << endl;
				}
			}
			else
			{
				cout << "FAILED!\n";
				//print();
			}

			assert(_CrtCheckMemory());

			delete[] x;
			destroy_cells();

			return rval;
		}
	}
}


/*
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
}*/

///////////////////////////////////////////////////////////////////////////////
const char* knuth_sample[];

void x_small_problem()
{
	StringArrayConverterWithColors converted(knuth_sample);

	bool b = exact_cover_strings(converted.Problem);


}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/*
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
*/
///////////////////////////////////////////////////////////////////////////////
/*
void x_very_large_problem()
{
	ProblemGenerator generator;
	generator.Generate();

	bool b = exact_cover_strings(generator.Sequences);

	cout << "Result: " << b << endl;

	auto setup_dt = std::chrono::duration_cast<std::chrono::microseconds>(setup_complete - start_time);
	auto run_dt = std::chrono::duration_cast<std::chrono::microseconds>(run_complete - setup_complete);

	cout << "Very large problem with strings solution took: " << setup_dt.count() << " microseconds for setup and " <<
		run_dt.count() << " microseconds to run and " << loop_count << " iterations.\n";
}
*/


