
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <unordered_set>
#include <chrono>

#include "Converters.h"
#include "AlgXPointer.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////////

AlgXPointer::~AlgXPointer()
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
ItemHeader* AlgXPointer::getItem(const char* pc)
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

	pitem->pNextActive = pFirstActiveItem;

	if (pFirstActiveItem)
	{
		pFirstActiveItem->pPrevActive = pitem;
	}

	pFirstActiveItem = pitem;

	return pitem;
}

///////////////////////////////////////////////////////////////////////////////
AlgXPointer::AlgXPointer(const std::vector< std::vector<const char*> >& sequences)
{
	auto start_time= std::chrono::high_resolution_clock::now();

	memset(this, 0, sizeof(this));

	for (int i = 0; i < sequences.size(); i++)
	{
		const vector<const char*>& seq = sequences[i];

		XCell* pprev = nullptr;

		for (const char* pc : seq)
		{
			ItemHeader* pitem = getItem(pc);

			XCell* pcell = new XCell;

			pcell->pDown = pitem->pTopCell;

			if (pitem->pTopCell)
			{
				pitem->pTopCell->pUp = pcell;
			}
			pitem->pTopCell = pcell;

			pcell->pLeft = pprev;

			if (pprev)
			{
				pprev->pRight = pcell;
			}
			pprev = pcell;

			pcell->pTop = pitem;
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	setupTime =  (long) std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

}

///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::unlinkCellVertically(XCell* pcell)
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
}
///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::relinkCellVertically(XCell* pcell)
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
}
///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::cover(ItemHeader* pitem)
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
void AlgXPointer::coverSeqItems(XCell* pcell)
{
	for (XCell* pleft = pcell->pLeft; pleft; pleft = pleft->pLeft)
	{
		cover(pleft->pTop);
	}

	for (XCell* pright = pcell->pRight; pright; pright = pright->pRight)
	{
		cover(pright->pTop);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::hide(XCell* pcell)
{
	XCell* left = pcell->pLeft;
	while (left)
	{
		unlinkCellVertically(left);
		left = left->pLeft;
	}

	XCell* right = pcell->pRight;
	while (right)
	{
		unlinkCellVertically(right);
		right = right->pRight;
	}
}

///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::unhide(XCell* pcell)
{
	XCell* left = pcell->pLeft;
	while (left)
	{
		relinkCellVertically(left);
		left = left->pLeft;
	}

	XCell* right = pcell->pRight;
	while (right)
	{
		relinkCellVertically(right);
		right = right->pRight;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::uncover(ItemHeader* pitem)
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
void AlgXPointer::uncoverSeqItems(XCell* pcell)
{
	for (XCell* pleft = pcell->pLeft; pleft; pleft = pleft->pLeft)
	{
		uncover(pleft->pTop);
	}

	for (XCell* pright = pcell->pRight; pright; pright = pright->pRight)
	{
		uncover(pright->pTop);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgXPointer::format(ostream& stream)
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
	//assert(idx == nunique_items);

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
	stream << separator;

	unordered_set<XCell*> seen_first_cells;
	XCell** pseq_cells = (XCell**)alloca(nunique_items * sizeof(XCell*));


	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		for (XCell *pcell = pitem->pTopCell; pcell; pcell = pcell->pDown)
		{
			// Backup to the first cell in the sequence:
			XCell* pfirst = pcell;
			while (pfirst->pLeft)
				pfirst = pfirst->pLeft;

			// See if we have already output it. Skip if we have, else record it.
			auto iter = seen_first_cells.find(pfirst);
			if (iter != seen_first_cells.end())
				continue;

			seen_first_cells.insert(pfirst);

			memset(pseq_cells, 0, nunique_items * sizeof(XCell*));

			// Record all the items used for this sequence:
			stream << setw(8) << "";
			for (XCell* poutput_cell = pfirst; poutput_cell; poutput_cell = poutput_cell->pRight)
			{
				int index = item_indexes[poutput_cell->pTop];
				pseq_cells[index] = poutput_cell;
			}

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
void AlgXPointer::print()
{
	format(cout);
}
///////////////////////////////////////////////////////////////////////////////
bool AlgXPointer::exactCover()
{
	assert(_CrtCheckMemory());
	AlgXStates state = ax_Initialize;

	bool rval = false;

	auto start_time = std::chrono::high_resolution_clock::now();

	//print();		// If you want to see Table 1.

	int l;

	// This stores the index of our choice at each level.
	size_t nunique_items = itemHeaders.size();
	XCell** px = (XCell**) alloca(nunique_items * sizeof(XCell*));

	for (;;)
	{
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
				state = ax_ChooseAndCover;
			break;

		case ax_ChooseAndCover:
		{
			auto pcur_item = pFirstActiveItem;

			if (pcur_item->pTopCell == nullptr)
			{
				// No sequences remain that could cover this item.
				state = ax_LeaveLevel;
				break;
			}
			cover(pcur_item);
			px[l] = pcur_item->pTopCell;

			state = ax_TryX;

#ifdef TRACE_PICKS
			cout << l << " - " << pcur_item->pName << endl;
#endif

			break;
		}

		case ax_TryX:
			assert(px[l]);

			coverSeqItems(px[l]);

#ifdef TRACE_PICKS
			cout << l << " try ";
			px[l]->format(cout);
			cout << endl;
			print();
#endif
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
#ifdef TRACE_PICKS
			cout << "Backtrack" << endl;
#endif
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
			
				for (int lout = 0; lout < l; lout++)
				{
					XCell* pcell = px[lout];

					while (pcell->pLeft)
						pcell = pcell->pLeft;

					do
					{
						cout << pcell->pTop->pName << " ";
						pcell = pcell->pRight;
					} while (pcell);
					cout << endl;
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
bool AlgXPointer::testUncoverCover()
{

	ostringstream  before, after;
	format(before);

	bool pass = true;
	ItemHeader* prev = nullptr;
	for (auto iter = itemHeaders.begin(); iter != itemHeaders.end(); iter++)
	{
		ItemHeader* pitem = iter->second;

		cover(pitem);

		if (prev)
		{
			cover(prev);
			uncover(prev);
		}
		uncover(pitem);
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
			AlgXPointer alg(converted.stringPointers);
			assert(alg.testUncoverCover());
		}

		{
			AlgXPointer alg(converted.stringPointers);
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

	AlgXPointer alg(converted.stringPointers);

	bool b = alg.exactCover();

	assert(b);


	cout << "Large problem with pointers solution took: " << alg.setupTime << " microseconds for setup and " <<
		alg.runTime << " microseconds to run.\n";
}
///////////////////////////////////////////////////////////////////////////////

void ptr_very_large_problem()
{
	StringArrayConverter converted(vary_large_char_problem);

	AlgXPointer alg(converted.stringPointers);

	bool b = alg.exactCover();

	assert(b);


	cout << "Very large problem with pointers solution took: " << alg.setupTime << " microseconds for setup and " <<
		alg.runTime << " microseconds to run.\n";
}