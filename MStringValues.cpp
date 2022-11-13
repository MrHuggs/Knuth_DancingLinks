//
// Implementation of Algorithm M: Covering with Multiplicities and Colors
// This versin tries to match Knuth as closely as possible.
//
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <map>
#include <chrono>

#include "Common.h"
using namespace std;
///////////////////////////////////////////////////////////////////////////////
// Cell structure to match 7.2.2.1 Table 1:
struct Header
{
	int i;
	const char* pName;
	int llink;
	int rlink;

	int slack;
	int bound;
};

struct Cell
{
	int x;
	union
	{
		int len;
		int top;
	};
	struct
	{
		int ulink;
		int dlink;
		int color;
	};
};

///////////////////////////////////////////////////////////////////////////////
static const ExactCoverWithMultiplicitiesAndColors* pproblem;

static  int nsequences;
static  int nsequence_items;		// Total number of characters in all strings
static  int nprimary_items;
static  int nsecondary_items;

static map<const char*, int, CmpSame>* pitem_indices = nullptr;
static map<const char*, int, CmpSame>*pcolor_indices = nullptr;

static map<int, int> *psequence_map = nullptr;

int max_item_len;
char* pitem_buf;

static int nheaders;
static Header* headers;	
static int ncells;		// Total number of cells:
static Cell* cells;

static int max_depth;
// This stores the index of our choice at each level.
static int* x;
// Array of first tweaks.
static int* ft;
///////////////////////////////////////////////////////////////////////////////
/// For performance timing:
static chrono::steady_clock::time_point start_time;
static chrono::steady_clock::time_point setup_complete;
static chrono::steady_clock::time_point run_complete;
static size_t solution_count;
static long long loop_count;
static long long level_count;
static bool non_sharp_preference;

static void get_counts()
{

	pitem_indices = new map<const char*, int, CmpSame>();
	pcolor_indices = new map<const char*, int, CmpSame>();
	max_item_len = 0;
	
	nprimary_items = (int) pproblem->primary_options.size();
	nsecondary_items = (int) pproblem->secondary_options.size();

	max_depth = 0;

	int item_id = 1; // Id's in Knuth's scheme start at 1.
	for (auto primary : pproblem->primary_options)
	{
		max_item_len = max(max_item_len, (int) strlen(primary.pValue));
		(*pitem_indices)[primary.pValue] = item_id++;
		max_depth += primary.v;
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
	for (int i = 0; i < pproblem->sequences.size(); i++)
	{
		nsequence_items += (int) pproblem->sequences[i].size();
	}

	// Calculate the total number of cells needed. Not that this is different
	// from Algorithm X
	//
	nheaders = 2 + nprimary_items + nsecondary_items;
	ncells =  2 + nprimary_items + nsecondary_items + nsequence_items + nsequences;
}
///////////////////////////////////////////////////////////////////////////////
// Initialized the header & cell data as in Table 1.
//
// Value to stick in unused field. Don't really need this, but it makes things
// print more nicely:
static const int unused = -99;

static void init_cells()
{
	headers = new Header[nheaders];
	pitem_buf = new char[max_item_len + 1];
	psequence_map = new map<int, int>();

	headers[0].i = 0;
	headers[0].pName = "";
	headers[0].llink = nprimary_items;
	headers[0].rlink = 1;
	headers[0].slack = unused;
	headers[0].bound = unused;

	int index;

	// Headers for primary options
	for (int i = 0; i < nprimary_items; i++)
	{
		index = i + 1;
		headers[index].i = index;
		headers[index].llink = index - 1;
		headers[index].rlink = (index + 1) % (nprimary_items + 1);
		headers[index].pName = pproblem->primary_options[i].pValue;
		headers[index].slack = pproblem->primary_options[i].v - pproblem->primary_options[i].u;
		assert(headers[index].slack >= 0);
		headers[index].bound = pproblem->primary_options[i].v;
	}

	// Headers for secondary options:
	for (int i = 0; i < nsecondary_items; i++)
	{
		index = i + nprimary_items + 1;
		headers[index].i = index;
		headers[index].llink = (i == 0) ? (nprimary_items + nsecondary_items + 1) : index - 1;
		headers[index].rlink = index + 1;
		headers[index].pName = pproblem->secondary_options[i];
		headers[index].slack = unused;
		headers[index].bound = unused;
	}

	// Secondary sentinel:
	index = nprimary_items + nsecondary_items + 1;
	headers[index].i = index;
	headers[index].pName = "";
	headers[index].llink = index - 1;
	headers[index].rlink = nprimary_items + 1;
	headers[index].slack = unused;
	headers[index].bound = unused;

	//First line of the cell data:
	cells = new Cell[ncells];

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
		const vector<const char*> &ptr_vec = pproblem->sequences[idx_seq];

		bool first_in_sequence = true;
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
				assert(pitem_indices->find(pc) != pitem_indices->end());
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

			if (first_in_sequence)
			{
				(* psequence_map)[index] = idx_seq;
				first_in_sequence = false;
			}

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
	delete[] cells;
	delete[] pitem_buf;
	delete pitem_indices;
	delete pcolor_indices;
	delete psequence_map;
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
		unhide(p);
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
			hide(q);
		}

		q = cells[q].dlink;
	}
}

void commit(int p, int j)
{
	int c = cells[p].color;
	if (c == 0)
	{
		cover(j);
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
			unhide(q);
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

void tweak(int x, int p)
{
	hide(x);					// Hide sequence x, and unlink the sequences before x from 
	int d = cells[x].dlink;     // the list of sequences for item p.
	cells[p].dlink = d;

	cells[d].ulink = p;
	cells[p].len--;
}

void tweak_p(int x, int p)		// tweak'
{
	int d = cells[x].dlink;
	cells[p].dlink = d;

	cells[d].ulink = p;
	cells[p].len--;
}

void untweak(int l)
{
	int a = ft[l];
	int p = (a <= nprimary_items + nsecondary_items) ? a : cells[a].top;
	int x = a;
	int y = p;
	int z = cells[p].dlink;
	cells[p].dlink = x;
	int k = 0;


	while (x != z)
	{
		cells[x].ulink = y;
		k++;
		unhide(x);
		y = x;
		x = cells[x].dlink;
	}

	cells[z].ulink = y;
	cells[p].len += k;

}

void untweak_p(int l) // untweak'
{
	int a = ft[l];
	int p = (a <= nprimary_items + nsecondary_items) ? a : cells[a].top;
	int x = a;
	int y = p;
	int z = cells[p].dlink;
	cells[p].dlink = x;
	int k = 0;


	while (x != z)
	{
		cells[x].ulink = y;
		k++;
		y = x;
		x = cells[x].dlink;
	}

	cells[z].ulink = y;
	cells[p].len += k;

	uncover_p(p);
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
	stream << endl << setw(8) << "SLACK";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << headers[n].slack;
	stream << endl << setw(8) << "BOUND";
	for (int n = 0; n <= nunique_items + 1; n++)
		stream << setw(5) << headers[n].bound;

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

	for (int line_start = nunique_items + 2; line_start < ncells; line_start += nunique_items + 2)
	{
		int line_cell_count = min(nunique_items + 2, ncells - line_start);

		stream << setw(8) << "x";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << cells[line_start + n].x;
		stream << endl << setw(8) << "TOP";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << cells[line_start + n].top;
		stream << endl << setw(8) << "ULINK";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << cells[line_start + n].ulink;
		stream << endl << setw(8) << "DLINK";
		for (int n = 0; n < line_cell_count; n++)
			stream << setw(5) << cells[line_start + n].dlink;
		stream << endl << setw(8) << "COLOR";
		for (int n = 0; n < line_cell_count; n++)
		{
			int color = cells[line_start + n].color;
			if (color <= 0)
				stream << setw(5) << cells[line_start + n].color;
			else
				stream << setw(5) << pproblem->colors[cells[line_start + n].color - 1];
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
static int sequence_start(int q)
{
	// Fast forward to the separator:
	do
	{
		q++;
	} while (cells[q].top > 0);
	// Start of the sequence is the separator's ulink:
	q = cells[q].ulink;

	return q;
}
///////////////////////////////////////////////////////////////////////////////
// Given the id of a cell in a sequence, output the whole sequence.
static const char *format_sequence(int q)
{
	q = sequence_start(q);

	assert(psequence_map->find(q) != psequence_map->end());
	int idx_seq = (* psequence_map)[q];

	const int bufsize = 2048;
	static char buf[bufsize];

	size_t curlen = sprintf_s(buf, "%4i: ", idx_seq);

	do
	{
		int idx_item = cells[q].top;
		const char* pitem_name;
		if (idx_item <= nprimary_items)
		{
			pitem_name = pproblem->primary_options[idx_item - 1].pValue;
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
			nc = (int)strlen(pcolor_name) + 1;
		}
		else
			nc = 0;

		size_t tc = n + nc + 2;

		if (curlen + tc > bufsize)
		{
			assert(false);
			return "Error: out of buffer space! ";
		}

#ifndef NDEBUG
		auto base = buf + curlen;
#endif

		memcpy(buf + curlen, pitem_name, n);
		curlen += n;

		if (nc > 0)
		{
			buf[curlen] = ':';
			memcpy(buf + curlen + 1, pcolor_name, nc - 1);
			curlen += nc;
		}

		buf[curlen] = ' ';
		curlen++;

#ifndef NDEBUG
		buf[curlen] = 0;
		size_t nused = strlen(base) + 1;
		assert(nused == tc);
#endif

		q++;

	} while (cells[q].top > 0);

	buf[curlen] = 0;
	return buf;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool exact_cover_with_multiplicities_and_colors(const ExactCoverWithMultiplicitiesAndColors& problem, 
					vector<vector<int>> *presults, int max_results = 1, bool _non_sharp_preference = false)
{
	//problem.print();
	problem.assertValid();
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;
	non_sharp_preference = _non_sharp_preference;

	pproblem = &problem;

	start_time = std::chrono::high_resolution_clock::now();
	get_counts();
	init_cells();
	setup_complete = std::chrono::high_resolution_clock::now();

	solution_count = 0;
	level_count = loop_count = 0;

	//print();		// If you want to see Table 1.
	int i, p, l = -1;

	x = new int[max_depth];
	ft = new int[max_depth];

	for (;;)
	{
		assert(l <= max_depth);
		loop_count++;
		TRACE("%lli:%i - %s\n", loop_count, l, StateName(state));

		switch (state)
		{
		case ax_Initialize:
			l = 0;
			state = ax_EnterLevel;
			break;

		case ax_EnterLevel:
			level_count++;
			if (headers[0].rlink == 0)
			{
				state = ax_RecordSolution;
			}
			else
			{
				state = ax_Choose;
			}
			break;

		case ax_Choose:
		{

			int smallest_branch_factor = std::numeric_limits<int>::max();
			int idx_smallest_branch;

			for (int j = headers[0].rlink; j !=0; j = headers[j].rlink)
			{
				int branch_factor = cells[j].len - (headers[j].bound - headers[j].slack) + 1;

				// This implements the non-sharp preference heuristic.
				// This is needed for the word rectangle problem, but not in general:
				if (non_sharp_preference)
				{
					if (branch_factor > 1 && headers[j].pName[0] == '#')
					{
						branch_factor += 10000;
					}
				}

				if (branch_factor < smallest_branch_factor)
				{
					smallest_branch_factor = branch_factor;
					idx_smallest_branch = j;
				}
			}

			if (smallest_branch_factor <= 0)
			{
				state = ax_LeaveLevel;
			}
			else
			{
				i = idx_smallest_branch;
				state = ax_PrepareToBranch;
			}
			break;
		}

		case ax_PrepareToBranch:
			x[l] = cells[i].dlink;
			headers[i].bound--;

			if (headers[i].bound == 0)
			{
				cover(i);
			}

			if (headers[i].bound != 0 || headers[i].slack != 0)
			{
				ft[l] = x[l];
			}

			state = ax_PossiblyTweak;
			break;

		case ax_PossiblyTweak:
			if (headers[i].bound == 0 && headers[i].slack == 0)  // In this case, we are like algorithm C
			{
				if (x[l] != i)
				{
					state = ax_TryX;
				}
				else
				{
					state = ax_Restore;
				}
				continue;
			}
			if (cells[i].len < headers[i].bound - headers[i].slack) // Not enough items remain
			{
				state = ax_Restore;
				continue;
			}
			if (x[l] != i)
			{
				if (headers[i].bound != 0)
				{
					tweak(x[l], i);
				}
				else
				{
					tweak_p(x[l], i);
				}
			}
			else
			{
				if (headers[i].bound != 0)
				{
					p = headers[i].llink;
					int q = headers[i].rlink;
					headers[p].rlink = q;
					headers[q].llink = p;
				}
			}
			state = ax_TryX;
			break;


		case ax_TryX:

			if (x[l] != i)
			{
				p = x[l] + 1;
				while (p != x[l])
				{
					int j = cells[p].top;
					if (j <= 0)
					{
						p = cells[p].ulink;
					}
					else
					{
						if (j <= nprimary_items)
						{
							headers[j].bound--;
							if (headers[j].bound == 0)
							{
								cover(j);
							}
						}
						else
						{
							commit(p, j);
						}
						p++;
					}
				}

				TRACE("\t\t%i - trying: %s\n", l, format_sequence(p));
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
					if (j <= nprimary_items)
					{
						headers[j].bound++;
						if (headers[j].bound == 1)
						{
							uncover_p(j);
						}
					}
					else
					{
						uncommit(p, j);
					}
					p--;
				}
			}

			x[l] = cells[x[l]].dlink;
			state = ax_PossiblyTweak;
			break;

		case ax_Restore:
			if (headers[i].bound == 0 && headers[i].slack == 0)
			{
				uncover_p(i);
			}
			else
			{
				if (headers[i].bound != 0)
				{
					untweak(l);
				}
				else
				{
					untweak_p(l);
				}
			}
			headers[i].bound++;

			state = ax_LeaveLevel;
			break;

		case ax_LeaveLevel:
			if (l == 0)
			{
				state = ax_Cleanup;
				continue;
			}

			l--;

			if (x[l] <= nprimary_items + nsecondary_items)
			{
				i = x[l];
				p = headers[i].llink;
				int q = headers[i].rlink;
				headers[p].rlink = i;
				headers[q].llink = i;
				state = ax_Restore;
				continue;
			}
			i = cells[x[l]].top;
			state = ax_TryAgain;
			break;

		case ax_RecordSolution:
		{
			assert(l != 0);
			TRACE("Cover found:\n");
			//print();
			vector<int> result;
			result.resize(l);
			for (int lout = 0; lout < l; lout++)
			{
				// c will be the cell index of a character in the string we chose.
				int c = x[lout];

				if (c <= nprimary_items + nsecondary_items)
				{
					continue;
				}

				TRACE("\t%s\n", format_sequence(c));

				int seq_start = sequence_start(c);

				assert(psequence_map->find(seq_start) != psequence_map->end());
				int idx_seq = (*psequence_map)[seq_start];
				result[lout] = idx_seq;
			}
			presults->emplace_back(result);

			if (presults->size() >= max_results)
			{
				state = ax_Cleanup;
			}
			else
			{
				state = ax_LeaveLevel;
			}
			break;
		}

		case ax_Cleanup:

			run_complete = std::chrono::high_resolution_clock::now();

			assert(_CrtCheckMemory());

			destroy_cells();

			delete[] ft;
			delete[] x;

			solution_count = presults->size();
			return solution_count != 0;
		}
	}
}

void print_exact_cover_with_multiplicities_and_colors_stats()
{
	auto setup_dt = std::chrono::duration_cast<std::chrono::microseconds>(setup_complete - start_time);
	auto run_dt = std::chrono::duration_cast<std::chrono::microseconds>(run_complete - setup_complete);

	cout << "Exact cover with multiplicities and colors found " << solution_count << " solutions." << endl;

	if (non_sharp_preference)
		cout << "\tThe non-sharp preference heuristic was used." << endl;

	cout << "\tTime used (microseconds): " << setup_dt.count() << " for setup and " <<
		run_dt.count() << " to run." << endl;

	cout << "\tLoop ran " << loop_count << " times with " << level_count << " level transitions." << endl;
}



