
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <unordered_set>
#include <chrono>

#include "Common.h"
#include "ProblemGenerator.h"
#include "AlgMPointer.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////////

AlgMPointer::~AlgMPointer()
{
	delete[] pHeaders;
	delete[] pCells;
	delete[] pLevelState;
}
///////////////////////////////////////////////////////////////////////////////
ItemHeader* AlgMPointer::getItem(const char* pc, size_t len)
{
	for (int i = 0; i < TotalItems; i++)
	{
		if (memcmp(pc, pHeaders[i].pName, len) == 0 && pHeaders[i].pName[len] == 0)
		{
			return pHeaders + i;
		}
	}
	assert(false);
	return nullptr;
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::checkValid() const
{
	ItemHeader* pprev = nullptr;
	for (ItemHeader *pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		assert(pitem->pPrevActive == pprev);
		pprev = pitem;
	}
	for (int i = 0; i < TotalItems; i++)
	{
		ItemHeader* pitem = pHeaders + i;

		int count = 0;
		for (MCell *pcell = pitem->pTopCell; pcell != nullptr; pcell = pcell->pDown)
		{
			count++;
		}
		assert(pitem->Current + count == pitem->StartingCount);

		if (pitem->pTopCell)
		{
			assert(pitem->pTopCell->pTop == pitem);
		}

		if (pitem->isPrimary())
		{

			assert(pitem->Current >= 0);
			assert(pitem->Current <= pitem->Max);

			bool islinked = false;
			if (pitem == pFirstActiveItem)
			{
				islinked = true;
			}
			else if (pitem->pPrevActive != nullptr && pitem->pPrevActive->pNextActive == pitem)
			{
				islinked = true;
			}
			else if (pitem->pNextActive != nullptr && pitem->pNextActive->pPrevActive == pitem)
			{
				islinked = true;
			}

			if (islinked)
			{
				assert(pitem->Current < pitem->Min);
			}
			
		}

	}
}
///////////////////////////////////////////////////////////////////////////////
#include "util/crc.h"
unsigned long AlgMPointer::checksum()
{
	static bool binit = false;
	if (!binit)
	{
		binit = true;
		crcInit();
	}

	crc parts[2];
	parts[0] = crcFast((unsigned char *) pHeaders, (int) TotalItems * sizeof(ItemHeader));
	parts[1] = crcFast((unsigned char*)pCells, (int) TotalCells * sizeof(MCell));

	crc results = crcFast((unsigned char*)parts, sizeof(parts));

	return results;
}
///////////////////////////////////////////////////////////////////////////////
AlgMPointer::AlgMPointer(const ExactCoverWithMultiplicitiesAndColors& problem) : Problem(problem)
{
	auto start_time= std::chrono::high_resolution_clock::now();

	TotalItems = (Problem.primary_options.size() + Problem.secondary_options.size());
	pHeaders = new ItemHeader[TotalItems];

	ItemHeader *pheader = pHeaders;
	ItemHeader* prev = nullptr;
	MaxItems = 0;
	 
	for (int i = 0; i < Problem.primary_options.size(); i++)
	{
		pheader->pName = Problem.primary_options[i].pValue;
		pheader->Max = Problem.primary_options[i].v;
		pheader->Min = Problem.primary_options[i].u;

		MaxItems += pheader->Max;
		pheader->pPrevActive = prev;
		if (prev)
			prev->pNextActive = pheader;
		prev = pheader;
		pheader++;
	}

	for (int i = 0; i < Problem.secondary_options.size(); i++)
	{
		pheader->pName = Problem.secondary_options[i];
		pheader->Min = pheader->Max = -1;

		pheader->pPrevActive = prev;
		if (prev)
			prev->pNextActive = pheader;
		prev = pheader;

		pheader++;
	}

	pFirstActiveItem = pHeaders;

	TotalCells = 0;
	for (int i = 0; i < Problem.sequences.size(); i++)
	{
		TotalCells += Problem.sequences[i].size();
	}

	pCells = new MCell[TotalCells];

	MCell *pcell = pCells;
	
	for (int i = 0; i < Problem.sequences.size(); i++)
	{
		const vector<const char*>& seq = Problem.sequences[i];

		MCell* pprev = nullptr;
		MCell* pfirst = pcell;

		SequenceMap[pcell] = i;	// Save for lookup when we find a solution.


		for (const char* pc : seq)
		{
			const char* sep = strchr(pc, ':');
			size_t len;
			if (sep)
			{
				len = sep - pc;
				pcell->pColor = sep + 1;
			}
			else
			{
				len = strlen(pc);
				pcell->pColor = nullptr;
			}

			ItemHeader* pitem = getItem(pc, len);
			
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
				MCell* pexisting = pitem->pTopCell;
				while (pexisting->pDown)
				{
					pexisting = pexisting->pDown;
				}
				pexisting->pDown = pcell;
				pcell->pUp = pexisting;
			}
			else 
				pitem->pTopCell = pcell;
#endif

			pcell->pTop = pitem;
			pitem->StartingCount++;

			if (pprev)
			{
				pprev->pRight = pcell;
			}
			pcell->pLeft = pprev;

			pprev = pcell;

			pcell++;
		}

		pprev->pRight = pfirst;	// Complete the circular linking.
		pfirst->pLeft = pprev;

	}

	CurLevel = 0;
	// Longest possible solution is max items, and we could go 1 level deeper:
	pLevelState = new LevelState[MaxItems + 1];

	auto end_time = std::chrono::high_resolution_clock::now();
	setupTime =  (long) std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	assert(_CrtCheckMemory());
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::unlinkCellVertically(MCell* pcell)
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
void AlgMPointer::relinkCellVertically(MCell* pcell)
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
void AlgMPointer::unlinkItem(ItemHeader *pitem)
{
#ifndef NDEBUG
	for (ItemHeader* plinked = pFirstActiveItem; plinked != pitem; plinked = plinked->pNextActive)
	{
		assert(plinked);	// Should be currently linked.
	}
#endif
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
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::cover(ItemHeader* pitem)
{
	unlinkItem(pitem);

	MCell* pcell = pitem->pTopCell;

	while (pcell)
	{
		hide(pcell);
		pcell = pcell->pDown;
	};
	
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::sequenceUsed(MCell* pcell)
{
	for (MCell* pright = pcell->pRight; pright != pcell; pright = pright->pRight)
	{
		pright->pTop->Current++;
		deactivateOrCover(pright->pTop);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::deactivateOrCover(ItemHeader* pitem)
{
	if (pitem->Current == pitem->Max)
	{
		cover(pitem);
	}
	else if (pitem->Current >= pitem->Min)
	{
		// The items is no longer active - we don't need any more sequences that reference it,
		// but we don't cover it.
		unlinkItem(pitem);
	}
	else
	{
		assert(pitem->Current < pitem->Min);
		/// Don nothing - this item needs to be used again.
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::tweak(MCell* pcell)
{
	// All sequences above this cell should lave already been tweaked.
	assert(pcell->pTop->pTopCell == pcell);

	hide(pcell);
	pcell->pTop->pTopCell = pcell->pDown;

	if (pcell->pDown)
	{
		pcell->pDown->pUp = nullptr;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::untweak_all()
{
	ItemHeader* pitem = pLevelState[CurLevel].pItem;
	pitem->pTopCell = pLevelState[CurLevel].pStartingCell;

	MCell* pprev = pitem->pTopCell;
	MCell* prelink = pprev->pDown;
	while (prelink && prelink->pUp == nullptr)
	{
		prelink->pUp = pprev;
		pprev = prelink;
		prelink = prelink->pDown;
	}

	while (pprev)
	{
		unhide(pprev);
		pprev = pprev->pUp;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::hide(MCell* pcell)
{

	MCell* right = pcell->pRight;
	while (right != pcell)
	{
		unlinkCellVertically(right);
		right = right->pRight;
	}
}

///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::unhide(MCell* pcell)
{
	MCell* left = pcell->pLeft;
	while (left != pcell)
	{
		relinkCellVertically(left);
		left = left->pLeft;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::relinkItem(ItemHeader* pitem)
{
#ifndef NDEBUG
	for (ItemHeader *plinked = pFirstActiveItem; plinked; plinked = plinked->pNextActive)
	{
		assert(plinked != pitem);	// Should not be currently linked.
	}
#endif

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
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::uncover(ItemHeader* pitem)
{
	relinkItem(pitem);

	MCell* pcell = pitem->pTopCell;

	while (pcell)
	{
		unhide(pcell);
		pcell = pcell->pDown;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::sequenceReleased(MCell* pcell)
{
	for (MCell* pleft = pcell->pLeft; pleft != pcell; pleft = pleft->pLeft)
	{
		pleft->pTop->Current--;
		reactivateOrUncover(pleft->pTop);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::reactivateOrUncover(ItemHeader* pitem)
{
	if (pitem->Current == pitem->Max - 1)
	{
		uncover(pitem);
	}
	else if (pitem->Current == pitem->Min - 1)
	{
		relinkItem(pitem);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::format(ostream& stream) const
{

	size_t nsep = (TotalItems + 1) * 5 + 8;
	char* separator = (char*) alloca(nsep + 2);
	memset(separator, '_', nsep);
	separator[nsep] = '\n';
	separator[nsep + 1] = 0;

	int *item_indexes = (int *) _alloca(TotalCells * sizeof(int));
	int idx = 0;
	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		item_indexes[pitem - pHeaders] = idx++;
	}

	stream << separator;

	idx = 1;
	const int lable_len = 10;
	stream << setw(lable_len) << "Index";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << idx++;
	}
	stream << endl;

	stream << setw(lable_len) << "Item";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->pName;
	}
	stream << endl;

	stream << setw(lable_len) << "Min";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->Min;
	}
	stream << endl;

	stream << setw(lable_len) << "Max";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->Max;
	}
	stream << endl;

	stream << setw(lable_len) << "ICount";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->StartingCount;
	}
	stream << endl;

	stream << setw(lable_len) << "Current";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << pitem->Current;
	}
	stream << endl;

	stream << setw(lable_len) << "Color";
	for (auto pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		stream << setw(5) << ((pitem->pColor) ? pitem->pColor : "");
	}
	stream << endl;


	stream << separator;

	unordered_set<MCell*> seen_cells;
	MCell** pseq_cells = (MCell**)alloca(TotalItems * sizeof(MCell*));


	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		for (MCell* pcell = pitem->pTopCell; pcell; pcell = pcell->pDown)
		{
			MCell* pfirst = pcell;

			// See if we have already output this cell. Skip if we have. If we haven't
			// already seen it, iterate through the circularly linked list, record
			// each entry, and output it.
			auto iter = seen_cells.find(pfirst);
			if (iter != seen_cells.end())
				continue;

			memset(pseq_cells, 0, TotalItems * sizeof(MCell*));

			// Record all the items used for this sequence:
			stream << setw(lable_len) << "";

			MCell* poutput_cell = pfirst;
			do
			{
				seen_cells.insert(poutput_cell);

				int index = item_indexes[poutput_cell->pTop - pHeaders];
				pseq_cells[index] = poutput_cell;

				poutput_cell = poutput_cell->pRight;

			} while (poutput_cell != pfirst);

			// Write the items:
			for (int i = 0; i < TotalItems; i++)
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
void AlgMPointer::print()
{
	format(cout);
}

///////////////////////////////////////////////////////////////////////////////
bool AlgMPointer::exactCover(std::vector<std::vector<int>>* presults, int max_results)
{
	assert(max_results >= 1);
	assert(presults->size() == 0);
	assert(_CrtCheckMemory());


	auto start_time = std::chrono::high_resolution_clock::now();

	CurLevel = 0;
	pLevelState[0].Action = ag_Init;


	loopCount = 0;
	print();

	for (;;)
	{
		loopCount++;

		assert(_CrtCheckMemory());

		LevelState& state = pLevelState[CurLevel];
		TRACE("%lli:%i - %s\n", loopCount, CurLevel, ActionName(state.Action));

		switch (state.Action)
		{
			case ag_Init:
			{
				assert(CurLevel == 0);
				state.Action = ag_EnterLevel;
				break;
			}
			case ag_EnterLevel:
			{
				if (pFirstActiveItem == nullptr)
				{
					recordSolution(presults);

					if (presults->size() <= max_results)
					{
						// We should continue searching:
						state.Action = ag_LeaveLevel;
					}
					else
					{
						state.Action = ag_Done;
					}
					continue;
				}
#ifndef NDEBUG
				state.EntryCheckSum = checksum();
#endif

				int smallest_branch_factor = std::numeric_limits<int>::max();
				ItemHeader* pbest = nullptr;

				for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
				{
					int branching_factor = pitem->branchingFactor();
					if (branching_factor < smallest_branch_factor)
					{
						smallest_branch_factor = branching_factor;
						pbest = pitem;
					}
				}

				if (smallest_branch_factor <= 0)
				{
					state.Action = ag_LeaveLevel;
					continue;
				}

				pbest->Current++;
				deactivateOrCover(pbest);

				state.pItem = pbest;
				state.pCurCell = pbest->pTopCell;

				// There are two different cases: The usage we are adding finishes this item, which causes
				// it to be covered. Or the item will still be active. If the item is still active, we only
				// consider as many sequences as we have to before branching again.
				if (pbest->Current == pbest->Max)
				{
					state.Action = ag_TryX;
					state.TryCellCount = pbest->remaining() + 1;
				}
				else
				{

					state.Action = ag_Tweak;
					state.pStartingCell = state.pCurCell;	      // Only matters for tweaking
					state.TryCellCount = smallest_branch_factor;
				}
				break;
			}
			case ag_TryX:
			{
				assert(state.pCurCell);
				state.TryCellCount--;

				// If we selected the current cell, all the items it references get used:
				sequenceUsed(state.pCurCell);

				TRACE("\t\t%i - trying: %s\n", CurLevel, state.pCurCell->format());

				CurLevel++;
				pLevelState[CurLevel].Action = ag_EnterLevel;
				break;
			}
			case ag_NextX:
			{
				sequenceReleased(state.pCurCell);


				if (state.TryCellCount == 0)
				{
					state.Action = ag_Restore;
				}
				else
				{
					state.pCurCell = state.pCurCell->pDown;
					assert(state.pCurCell); // Else TryCellCount should have hit 0.
					state.Action = ag_TryX;
				}
				break;
			}
			case ag_Tweak:
			{
				state.TryCellCount--;
				tweak(state.pCurCell);
				CurLevel++;
				pLevelState[CurLevel].Action = ag_EnterLevel;
				break;
			}

			case ag_TweakNext:
			{

				if (state.TryCellCount == 0)
				{
					untweak_all();
					state.Action = ag_Restore;
				}
				else
				{
					state.pCurCell = state.pCurCell->pDown;
					assert(state.pCurCell); // Else TryCellCount should have hit 0.
					state.Action = ag_Tweak;
				}
				break;
			}
			case ag_Restore:
			{
				state.pItem->Current--;
				reactivateOrUncover(state.pItem);

#ifndef NDEBUG
				auto cur_checksum = checksum();
				assert(state.EntryCheckSum == checksum());
#endif

				state.Action = ag_LeaveLevel;
				break;
			}

			case ag_LeaveLevel:
			{
				if (CurLevel == 0)
				{
					state.Action = ag_Done;
				}
				else
				{

					CurLevel--;
					if (pLevelState[CurLevel].Action == ag_TryX)
					{
						pLevelState[CurLevel].Action = ag_NextX;
					}
					else
					{
						assert(pLevelState[CurLevel].Action == ag_Tweak);
						pLevelState[CurLevel].Action = ag_TweakNext;
					}
				}
				break;
			}
			case ag_Done:
			{
				auto end_time = std::chrono::high_resolution_clock::now();
				runTime = (long)std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

				assert(_CrtCheckMemory());
				return presults->size() != 0;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::recordSolution(std::vector<std::vector<int>>* presults)
{
	vector<int> result;
	result.resize(CurLevel);
	for (int lout = 0; lout < CurLevel; lout++)
	{
		// c will be the cell index of a character in the string we chose.
		MCell *pcell = pLevelState[lout].pCurCell;

		//TRACE("\t%s\n", format_sequence(c));

		for (;;)
		{
			auto iter = SequenceMap.find(pcell);
			if (iter != SequenceMap.end())
			{
				int idx_seq = iter->second;
				result[lout] = idx_seq;
				break;
			}
			pcell = pcell->pLeft;
		}

	}
	presults->emplace_back(result);
}
///////////////////////////////////////////////////////////////////////////////
bool AlgMPointer::testUncoverCover()
{

	ostringstream  before, after;
	format(before);

	bool pass = true;
	ItemHeader* prev = nullptr;
	for (int i = 0; i < TotalItems; i++)
	{
		ItemHeader* pitem = pHeaders + i;

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

/*
	cout << "Large problem with pointers solution took: " << alg.setupTime << " microseconds for setup and " <<
		alg.runTime << " microseconds to run and " << alg.loopCount	<< " iterations.\n";
		*/
