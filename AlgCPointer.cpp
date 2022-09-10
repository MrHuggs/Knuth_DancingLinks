
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <unordered_set>
#include <chrono>

#include "Common.h"
#include "ProblemGenerator.h"
#include "AlgCPointer.h"

using namespace std;
///////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
void AlgCPointer::assertValid() const
{
	ItemHeader* pprev = nullptr;
	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		assert(pitem->pPrevActive == pprev);
		if (pprev)
		{
			assert(strcmp(pprev->pName, pitem->pName) < 0);
			assert(pprev->pNextActive == pitem);
		}
		pprev = pitem;
	}

	for (auto iter = itemHeaders.begin(); iter != itemHeaders.end(); iter++)
	{
		const char* pc = iter->first;
		ItemHeader* pitem = iter->second;

		assert(strcmp(pc, pitem->pName) == 0);

		int count = 0;
		XCell* prev = nullptr;
		for (XCell* pcell = pitem->pTopCell; pcell; pcell = pcell->pDown)
		{
			assert(pcell->pUp == prev);

			if (pcell->pLeft)
			{
				assert(pcell->pLeft->pRight == pcell);
			}
			if (pcell->pRight)
			{
				assert(pcell->pRight->pLeft == pcell);
			}

			count++;
			prev = pcell;
		}
		assert(pitem->Count == count);
	}

}
#endif
///////////////////////////////////////////////////////////////////////////////
AlgCPointer::~AlgCPointer()
{
	for (auto iter = itemHeaders.begin(); iter != itemHeaders.end(); iter++)
	{
		ItemHeader* pitem = iter->second;
		for (XCell* pcell = pitem->pTopCell; pcell; )
		{
			auto pnext_cell = pcell->pDown;
			delete pcell;
			pcell = pnext_cell;
		}
		delete pitem;
	}
}
///////////////////////////////////////////////////////////////////////////////
ItemHeader* AlgCPointer::getItem(const char* pc)
{
	ItemHeader* pitem;
	auto it = itemHeaders.find(pc);

	if (it != itemHeaders.end())
	{
		assert(strcmp(it->first, pc) == 0);

		return it->second;
	}

	pitem = new ItemHeader;
	pitem->pName = pc;

	itemHeaders[pc] = pitem;
	
	return pitem;
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::sortItems()
{
	// Sort the items to match the order in the original AlgorithmX. In general,
	// this should not make any difference, but is useful for comparing
	// performance.

	vector<ItemHeader*> headers;
	for (auto iter = itemHeaders.begin(); iter != itemHeaders.end(); iter++)
	{
		ItemHeader* pitem = iter->second;
		headers.push_back(pitem);
	}

	sort(headers.begin(), headers.end(), [](const ItemHeader* pa, const ItemHeader* pb) 
		{ return strcmp(pa->pName, pb->pName) < 0; });


	pFirstActiveItem = headers[0];

	for (int i = 0; i < headers.size() - 1; i++)
	{
		headers[i]->pNextActive = headers[i + 1];
		headers[i + 1]->pPrevActive = headers[i];
	}

}
///////////////////////////////////////////////////////////////////////////////
AlgCPointer::AlgCPointer(const std::vector< std::vector<const char*> >& sequences)
{
	auto start_time= std::chrono::high_resolution_clock::now();

	memset(this, 0, sizeof(this));

	for (int i = 0; i < sequences.size(); i++)
	{
		const vector<const char*>& seq = sequences[i];

		XCell* pprev = nullptr;

		vector<XCell*> cells;

		for (const char* pc : seq)
		{
			ItemHeader* pitem = getItem(pc);

			XCell* pcell = new XCell;
			
#if LINK_TOP
			// It would be easiest to just link the new cell at the top of the list.
			// However, to match AlgorithmX more closely, we should link at the bottom. This
			// will produce the same sequence of choices.
			pcell->pDown = pitem->pTopCell;

			if (pitem->pTopCell)
			{
				pitem->pTopCell->pUp = pcell;
			}
			pitem->pTopCell = pcell;
#else
			if (pitem->pTopCell)
			{
				XCell* pcur_bottom = pitem->pTopCell->pUp;

				pcur_bottom->pDown = pcell;
				pcell->pUp = pcur_bottom;

				// Temporarily store the bottom cell as the up pointer in the top:
				pitem->pTopCell->pUp = pcell;
			}
			else
			{
				pitem->pTopCell = pcell;
				pcell->pUp = pcell;
			}
#endif
			pitem->Count++;

			cells.push_back(pcell);

			pcell->pTop = pitem;
		}

		int ncells = (int) cells.size();
		for (int j = 0; j < ncells; j++)
		{
			int next = (j + 1) % ncells;
			int prev = (j + ncells - 1) % ncells;
			cells[j]->pRight = cells[next];
			cells[j]->pLeft = cells[prev];
		}
	}


#ifndef LINK_TOP
	// Properly set the top item's up pointer to null. We had been using to to
	// store the bottom item, but we don't need the bottom any more.
	for (auto iter = itemHeaders.begin(); iter != itemHeaders.end(); iter++)
	{
		ItemHeader* pitem = iter->second;
		pitem->pTopCell->pUp = nullptr;
	}
#endif

	sortItems();

	auto end_time = std::chrono::high_resolution_clock::now();
	setupTime =  (long) std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	assertValid();
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::unlinkCellVertically(XCell* pcell)
{
	if (pcell->pUp)
	{
		pcell->pUp->pDown = pcell->pDown;
	}
	else
	{
		pcell->pTop->pTopCell = pcell->pDown;
	}

	if (pcell->pDown)
	{
		pcell->pDown->pUp = pcell->pUp;
	}
	pcell->pTop->Count--;
	assert(pcell->pTop->Count >= 0);
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::relinkCellVertically(XCell* pcell)
{
	if (pcell->pUp)
	{
		pcell->pUp->pDown = pcell;
	}
	else
	{
		pcell->pTop->pTopCell = pcell;
	}

	if (pcell->pDown)
	{
		pcell->pDown->pUp = pcell;
	}

	pcell->pTop->Count++;
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::cover(ItemHeader* pitem)
{
	if (pitem == pFirstActiveItem)
	{
		pFirstActiveItem = pitem->pNextActive;
		assert(pitem->pPrevActive == nullptr);
	}
	else
	{
		assert(pitem->pPrevActive);
		pitem->pPrevActive->pNextActive = pitem->pNextActive;
	}

	if (pitem->pNextActive)
		pitem->pNextActive->pPrevActive = pitem->pPrevActive;

	XCell* pcell = pitem->pTopCell;

	while (pcell)
	{
		hide(pcell);
		pcell = pcell->pDown;
	};

}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::coverSeqItems(XCell* pcell)
{
	for (XCell* pright = pcell->pRight; pright != pcell; pright = pright->pRight)
	{
		cover(pright->pTop);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::hide(XCell* pcell)
{

	XCell* right = pcell->pRight;
	while (right != pcell)
	{
		unlinkCellVertically(right);
		right = right->pRight;
	}
}

///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::unhide(XCell* pcell)
{
	XCell* left = pcell->pLeft;
	while (left != pcell)
	{
		relinkCellVertically(left);
		left = left->pLeft;
	}

}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::uncover(ItemHeader* pitem)
{
	if (pitem->pPrevActive == nullptr)
	{
		pFirstActiveItem = pitem;
	}
	else
	{
		pitem->pPrevActive->pNextActive = pitem;
	}

	if (pitem->pNextActive)
		pitem->pNextActive->pPrevActive = pitem;

	XCell* pcell = pitem->pTopCell;

	while (pcell)
	{
		unhide(pcell);
		pcell = pcell->pDown;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::uncoverSeqItems(XCell* pcell)
{
	for (XCell* pleft = pcell->pLeft; pleft != pcell; pleft = pleft->pLeft)
	{
		uncover(pleft->pTop);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::format(ostream& stream)
{
	size_t nunique_items = itemHeaders.size();

	size_t nsep = (nunique_items + 1) * 5 + 8;
	char* separator = (char*) alloca(nsep + 2);
	memset(separator, '_', nsep);
	separator[nsep] = '\n';
	separator[nsep + 1] = 0;

	map<ItemHeader*, int> item_indexes;
	int idx = 0;
	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		item_indexes[pitem] = idx++;
	}

	stream << separator;

	idx = 1;
	stream << setw(8) << "Index";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << idx++;
	}
	stream << endl;

	stream << setw(8) << "Item";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->pName;
	}
	stream << endl;
	stream << setw(8) << "Count";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->Count;
	}
	stream << endl;
	stream << separator;

	unordered_set<XCell*> seen_cells;
	XCell** pseq_cells = (XCell**)alloca(nunique_items * sizeof(XCell*));


	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		for (XCell *pcell = pitem->pTopCell; pcell; pcell = pcell->pDown)
		{
			XCell* pfirst = pcell;

			// See if we have already output this cell. Skip if we have. If we haven't
			// already seen it, iterate through the circularly linked list, record
			// each entry, and output it.
			auto iter = seen_cells.find(pfirst);
			if (iter != seen_cells.end())
				continue;

			memset(pseq_cells, 0, nunique_items * sizeof(XCell*));

			// Record all the items used for this sequence:
			stream << setw(8) << "";

			XCell* poutput_cell = pfirst;
			do
			{
				seen_cells.insert(poutput_cell);

				int index = item_indexes[poutput_cell->pTop];
				pseq_cells[index] = poutput_cell;

				poutput_cell = poutput_cell->pRight;
				
			} while (poutput_cell != pfirst);

			// Write the items:
			for (int i = 0; i < nunique_items; i++)
			{
				if (pseq_cells[i])
					stream << setw(5) << pseq_cells[i]->pTop->pName;
				else
					stream << setw(5) << "";
			}
			stream << endl;
		}
	}
	stream << separator;
}
///////////////////////////////////////////////////////////////////////////////
void AlgCPointer::print()
{
	format(cout);
}
///////////////////////////////////////////////////////////////////////////////
bool AlgCPointer::exactCover()
{
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;

	bool rval = false;

	auto start_time = std::chrono::high_resolution_clock::now();

	//print();

	int l = -1;

	// This stores the index of our choice at each level.
	size_t nunique_items = itemHeaders.size();
	XCell** px = (XCell**) alloca(nunique_items * sizeof(XCell*));

	loopCount = 0;

	for (;;)
	{
		loopCount++;
		TRACE("%lli:%i - %s\n", loopCount, l, StateName(state));
		assertValid();
		//print();


		switch (state)
		{
		case ax_Initialize:
			l = 0;
			state = ax_EnterLevel;
			break;

		case ax_EnterLevel:
			if (pFirstActiveItem == nullptr)
			{
				rval = true;
				state = ax_Cleanup;
			}
			else
				state = ax_Choose;
			break;

		case ax_Choose:	// Choose and cover are combined for this version.
		{
			auto pcur_item = pFirstActiveItem;

#define CHOOSE_MIN
#ifdef CHOOSE_MIN
			int min_count = pcur_item->Count;

			for (auto pnext = pcur_item->pNextActive; pnext; pnext = pnext->pNextActive)
			{
				if (pnext->Count < min_count)
				{
					pcur_item = pnext;
					min_count = pcur_item->Count;
				}
			}

#endif
			if (pcur_item->pTopCell == nullptr)
			{
				assert(pcur_item->Count == 0);
				// No sequences remain that could cover this item.
				state = ax_LeaveLevel;
				break;
			}
			cover(pcur_item);
			px[l] = pcur_item->pTopCell;

			state = ax_TryX;

			TRACE("\t\t%i - covering: %s\n", l, pcur_item->pName);

			break;
		}

		case ax_TryX:
			assert(px[l]);

			coverSeqItems(px[l]);

			TRACE("\t\t%i - trying: %s\n", l, px[l]->format());

			l++;
			state = ax_EnterLevel;
			break;

		case ax_TryAgain:

			uncoverSeqItems(px[l]);

			if (px[l]->pDown == nullptr)
			{
				state = ax_Backtrack;
			}
			else
			{
				px[l] = px[l]->pDown;
				state = ax_TryX;
			}
			break;
		case ax_Backtrack:
			uncover(px[l]->pTop);
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

			auto end_time = std::chrono::high_resolution_clock::now();
			runTime = (long) std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

			if (rval == true)
			{
				// To output the actual strings:

				cout << "Cover found:" << endl;
				for (int lout = 0; lout < l; lout++)
				{
					XCell* pcell = px[lout];

					cout << "\t" << pcell->format() << endl;
				}

				// Relink all the covered/hidden items. At the very least, need this
				// for proper cleanup.
				l--;
				while (l >= 0)
				{
					uncoverSeqItems(px[l]);
					uncover(px[l--]->pTop);
				}
			}
			else
				cout << "FAILED!\n";

			assert(_CrtCheckMemory());

			return rval;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
bool AlgCPointer::testUncoverCover()
{

	ostringstream  before, after;
	format(before);

	bool pass = true;
	ItemHeader* prev = nullptr;
	for (auto iter = itemHeaders.begin(); iter != itemHeaders.end(); iter++)
	{
		ItemHeader* pitem = iter->second;

		//print();
		cover(pitem);

		if (prev)
		{
			cover(prev);
			uncover(prev);
		}
		//print();
		uncover(pitem);
		//print();
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
		prev = pitem;
	}
	return pass;
}
///////////////////////////////////////////////////////////////////////////////
const char* small_char_problems[];
const char* large_char_problem[];
const char* vary_large_char_problem[];

void ptr_small_problems()
{

	const char** pstrings = small_char_problems;
	while (*pstrings)
	{
		ConvertedCharProblem converted(pstrings);

		{
			AlgCPointer alg(converted.stringPointers);
			assert(alg.testUncoverCover());
		}

		{
			AlgCPointer alg(converted.stringPointers);
			bool b = alg.exactCover();
			assert(b);
		}

		while (*(pstrings++));
	}
}
///////////////////////////////////////////////////////////////////////////////
void ptr_large_problem()
{
	ConvertedCharProblem converted(large_char_problem);

	AlgCPointer alg(converted.stringPointers);

	bool b = alg.exactCover();

	assert(b);


	cout << "Large problem with pointers solution took: " << alg.setupTime << " microseconds for setup and " <<
		alg.runTime << " microseconds to run and " << alg.loopCount	<< " iterations.\n";
}
///////////////////////////////////////////////////////////////////////////////

void ptr_very_large_problem()
{
	ProblemGenerator generator;
	generator.Generate();

	AlgCPointer alg(generator.Sequences);

	bool b = alg.exactCover();

	assert(b);

	cout << "Very large problem with pointers solution took: " << alg.setupTime << " microseconds for setup and " <<
		alg.runTime << " microseconds to run and " << alg.loopCount << " iterations.\n";

}