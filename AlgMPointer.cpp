
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
//
// AlgMChecksum
//
///////////////////////////////////////////////////////////////////////////////
#ifdef SMALL_CHECKSUM
#include "util/crc.h"

void AlgMChecksum::init(const AlgMPointer& alg)
{
	static bool binit = false;
	if (!binit)
	{
		binit = true;
		crcInit();
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMChecksum::checksum(const AlgMPointer& alg)
{

	crc parts[2];
	parts[0] = crcFast((unsigned char*)alg.pHeaders, (int)alg.TotalItems * sizeof(ItemHeader));
	parts[1] = crcFast((unsigned char*)alg.pCells, (int)alg.TotalCells * sizeof(MCell));

	EntryCheckSum = crcFast((unsigned char*)parts, sizeof(parts));

}
///////////////////////////////////////////////////////////////////////////////
bool AlgMChecksum::compare(const AlgMChecksum& other, const AlgMPointer& alg) const
{
	return EntryCheckSum == other.EntryCheckSum;
}
///////////////////////////////////////////////////////////////////////////////
#else
///////////////////////////////////////////////////////////////////////////////
AlgMChecksum::AlgMChecksum()
{
	memset(this, 0, sizeof(*this));
}
///////////////////////////////////////////////////////////////////////////////
AlgMChecksum::~AlgMChecksum()
{
	delete[] pHeaders;
	delete[] pCells;
}
///////////////////////////////////////////////////////////////////////////////
void AlgMChecksum::init(const AlgMPointer& alg)
{
	pHeaders = new ItemHeader[alg.TotalItems];
	pCells = new MCell[alg.TotalCells];
}
///////////////////////////////////////////////////////////////////////////////
void AlgMChecksum::checksum(const AlgMPointer &alg)
{
	memcpy(pHeaders, alg.pHeaders, alg.TotalItems * sizeof(ItemHeader));
	memcpy(pCells, alg.pCells, alg.TotalCells * sizeof(MCell));
	
}
///////////////////////////////////////////////////////////////////////////////
template<class T>
static bool check_and_report (int idx, const char *pname, const T & t1, const T& t2)
{
	if (t1 == t2)
		return true;

	cout << "index " << idx << " " << pname << " differs." << endl;
	return false;
}

bool AlgMChecksum::compare(const AlgMChecksum& other, const AlgMPointer& alg) const
{
	bool same = true;
	for (int i = 0; i < alg.TotalItems; i++)
	{

#define CHECK_HEADER(field) same = same && check_and_report(i, #field, pHeaders[i].field, other.pHeaders[i].field);
			CHECK_HEADER(pName)
			CHECK_HEADER(pPrevActive)
			CHECK_HEADER(pNextActive)
			CHECK_HEADER(pTopCell)
			CHECK_HEADER(Min)
			CHECK_HEADER(Max)
			CHECK_HEADER(UsedCount)
			CHECK_HEADER(AvailableSequences)
			CHECK_HEADER(pColor)
	}

	for (int i = 0; i < alg.TotalCells; i++)
	{
#define CHECK_CELL(field) same = same && check_and_report(i, #field, pCells[i].field, other.pCells[i].field);
		CHECK_CELL(pTop)
			CHECK_CELL(pColor)
			CHECK_CELL(pLeft)
			CHECK_CELL(pRight)
			CHECK_CELL(pUp)
			CHECK_CELL(pDown)
	}
	return same;
}
#endif
///////////////////////////////////////////////////////////////////////////////
//
// AlgMPointer
//
///////////////////////////////////////////////////////////////////////////////
AlgMPointer::~AlgMPointer()
{
	delete[] pHeaders;
	delete[] pCells;
	delete[] pLevelState;

#ifndef NDEBUG
	delete[] pChecksums;
#endif
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
const char* AlgMPointer::getColor(const char* pc)
{
	for (auto pproblem_color : Problem.colors)
	{
		if (strcmp(pc, pproblem_color) == 0)
			return pproblem_color;
	}
	assert(false);
	return nullptr;
}
///////////////////////////////////////////////////////////////////////////////
bool AlgMPointer::isLinked(const ItemHeader* pitem) const
{

	for (ItemHeader* plinked = pFirstActiveItem; plinked; plinked = plinked->pNextActive)
	{
		if (pitem == plinked)
			return true;
	}

	return false;
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::assertValid() const
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
		assert(count == pitem->AvailableSequences);

		if (pitem->pTopCell)
		{
			assert(pitem->pTopCell->pTop == pitem);
		}

		if (pitem->isPrimary())
		{

			assert(pitem->UsedCount >= 0);
			assert(pitem->UsedCount <= pitem->Max);

			bool islinked = isLinked(pitem);

			if (islinked)
			{
				assert(pitem->UsedCount < pitem->Min);
			}
			
		}

	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::testChecksum()
{
#ifndef NDEBUG
	tempChecksum.checksum(*this);
#endif


	assert(pChecksums[CurLevel].compare(tempChecksum, *this));
}
///////////////////////////////////////////////////////////////////////////////
AlgMPointer::AlgMPointer(const ExactCoverWithMultiplicitiesAndColors& problem) : Problem(problem)
{
	Problem.assertValid();
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
	pFirstActiveItem = pHeaders;

	prev = nullptr;
	for (int i = 0; i < Problem.secondary_options.size(); i++)
	{
		pheader->pName = Problem.secondary_options[i];
		pheader->Min = pheader->Max = -1;

		pheader->pPrevActive = prev;
		if (prev)
		{
			prev->pNextActive = pheader;
		}
		else
		{
			pFirstSecondaryItem = pheader;
		}
		prev = pheader;

		pheader++;
	}


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
				pcell->pColor = getColor(sep + 1);
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
			pitem->AvailableSequences++;

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

	NonSharpPreference = false;

#ifndef NDEBUG
	pChecksums = new AlgMChecksum[MaxItems + 1];
	for (int i = 0; i < MaxItems + 1; i++)
		pChecksums[i].init(*this);

	tempChecksum.init(*this);
#endif

	auto end_time = std::chrono::high_resolution_clock::now();
	setupTime =  (long) std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

	assert(_CrtCheckMemory());
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::unlinkCellVertically(MCell* pcell)
{
	pcell->pTop->AvailableSequences--;
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
	pcell->pTop->AvailableSequences++;
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::unlinkItem(ItemHeader *pitem)
{
	assert(isLinked(pitem));

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
		pright->pTop->UsedCount++;

		if (pright->pColor)
		{
			setcolor(pright);
		}
		else
		{
			deactivateOrCover(pright->pTop);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::setcolor(MCell* pcell)
{
	assert(pcell->pTop->pColor == nullptr);  // Should not have been assigned yet
	pcell->pTop->pColor = pcell->pColor;

	for (MCell *plinked = pcell->pTop->pTopCell; plinked; plinked = plinked->pDown)
	{
		assert(plinked->pColor != nullptr);
		if (plinked->pColor == pcell->pColor)
		{
			// This cell matches the chosen cell's color. We can continue using the
			// corresponding sequence, but set the color to null so we won't have to
			// check in the future.
			plinked->pColor = nullptr;
		}
		else
		{
			// Uses some other color. Hide the corresponding sequence, but leave it
			// linked horizontally so that we can find it if we restore.
			hide(plinked);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::deactivateOrCover(ItemHeader* pitem)
{
	if (!pitem->isPrimary())
	{
		return;
	}

	if (pitem->UsedCount == pitem->Min)
	{
		// The item is no longer active - we don't need any more sequences that reference it,
		// but we don't cover it either because it could be used again.
		unlinkItem(pitem);
	}
	else if (pitem->UsedCount > pitem->Min)
	{
		// Should have already been unlinked.
		assert(!isLinked(pitem));
	}
	else
	{
		assert(isLinked(pitem));
	}

	if (pitem->UsedCount == pitem->Max)
	{
		cover(pitem);
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::tweak(MCell* pcell)
{
	// All sequences above this cell should lave already been tweaked.
	assert(pcell->pTop->pTopCell == pcell);

	hide(pcell);
	pcell->pTop->pTopCell = pcell->pDown;
	pcell->pTop->AvailableSequences--;

	// Unhook this cell from the one below:
	if (pcell->pDown)
	{
		pcell->pDown->pUp = nullptr;
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::untweak_all()
{
	ItemHeader* pitem = pLevelState[CurLevel].pItem;

	MCell *pcell = pLevelState[CurLevel].pStartingCell;

	pitem->pTopCell = pcell;
	assert(pcell->pDown == nullptr || pcell->pDown->pUp == nullptr);

	for (;;)
	{
		unhide(pcell);
		pitem->AvailableSequences++;

		if (pcell->pDown)
		{
			pcell->pDown->pUp = pcell;
		}

		if (pcell == pLevelState[CurLevel].pCurCell)
		{
			break;
		}

		pcell = pcell->pDown;
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
		pleft->pTop->UsedCount--;

		if (pleft->pColor)
		{
			clearColor(pleft);
		}
		else
		{
			reactivateOrUncover(pleft->pTop);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::clearColor(MCell* pcell)
{
	assert(pcell->pTop->pColor == pcell->pColor);
	pcell->pTop->pColor = nullptr;

	// Note that this is not exactly the reverse of the setColor order.
	for (MCell* plinked = pcell->pTop->pTopCell; plinked; plinked = plinked->pDown)
	{
		if (plinked->pColor == nullptr)
		{
			// This cell matches the chosen cell's color. We can continue using the
			// corresponding sequence, but set the color to null so we won't have to
			// check in the future.
			plinked->pColor = pcell->pColor;
		}
		else
		{
			assert(plinked->pColor != pcell->pColor);
			// Uses some other color. Hide the corresponding sequence, but leave it
			// linked horizontally so that we can find it if we restore.
			unhide(plinked);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
void AlgMPointer::reactivateOrUncover(ItemHeader* pitem)
{
	if (!pitem->isPrimary())
	{
		return;
	}

	if (pitem->UsedCount == pitem->Max - 1)
	{
		uncover(pitem);
	}

	if (pitem->UsedCount == pitem->Min - 1)
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

	int *item_indexes = (int *) _alloca(TotalItems * sizeof(int));
	ItemHeader** pitems = (ItemHeader**)_alloca(TotalItems * sizeof(ItemHeader *));
	int idx = 0;
	for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
	{
		pitems[idx] = pitem;
		item_indexes[pitem - pHeaders] = idx++;
	}

	// Secondary items don't get deactivated, so this will be all of them:
	for (ItemHeader* pitem = pFirstSecondaryItem; pitem; pitem = pitem->pNextActive)
	{
		pitems[idx] = pitem;
		item_indexes[pitem - pHeaders] = idx++;
	}


	stream << separator;

	const int label_len = 10;
	stream << setw(label_len) << "Index";
	for (int i = 0; i < idx; i++)
	{
		stream << setw(5) << item_indexes[pitems[i]-pHeaders];
	}
	stream << endl;

	stream << setw(label_len) << "Item";
	for (int i = 0; i < idx; i++)
	{
		auto pitem = pitems[i];
		stream << setw(5) << pitem->pName;
	}
	stream << endl;

	stream << setw(label_len) << "Min";
	for (int i = 0; i < idx; i++)
	{
		auto pitem = pitems[i];
		stream << setw(5) << pitem->Min;
	}
	stream << endl;

	stream << setw(label_len) << "Max";
	for (int i = 0; i < idx; i++)
	{
		auto pitem = pitems[i];
		stream << setw(5) << pitem->Max;
	}
	stream << endl;

	stream << setw(label_len) << "Available";
	for (int i = 0; i < idx; i++)
	{
		auto pitem = pitems[i];
		stream << setw(5) << pitem->AvailableSequences;
	}
	stream << endl;

	stream << setw(label_len) << "UsedCount";
	for (int i = 0; i < idx; i++)
	{
		auto pitem = pitems[i];
		stream << setw(5) << pitem->UsedCount;
	}
	stream << endl;

	stream << setw(label_len) << "Color";
	for (int i = 0; i < idx; i++)
	{
		auto pitem = pitems[i];
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
			stream << setw(label_len) << "";

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
				auto pcell = pseq_cells[i];
				if (pcell)
				{
					string s = pcell->pTop->pName;
					const char* pcolor = pcell->pColor ? pcell->pColor : pcell->pTop->pColor;
					if (pcolor)
					{
						s += ':';
						s += pcolor;
					}
					stream << setw(5) << s;
				}
				else
					stream << setw(5) << "";
			}
			stream << endl;
		}
	}
	stream << separator;
}
///////////////////////////////////////////////////////////////////////////////
bool AlgMPointer::exactCover(std::vector<std::vector<int>>* presults, int max_results)
{
	assert(max_results >= 1);
	assert(presults->size() == 0);
	assert(_CrtCheckMemory());

	if (TotalItems < 50)
	{
		Problem.print();
		print();
	}
#ifndef NDEBUG
	assert(testUncoverCover());
#endif

	auto start_time = std::chrono::high_resolution_clock::now();

	CurLevel = 0;
	pLevelState[0].Action = ag_Init;

	Solutions = 0;
	loopCount = levelCount = 0;

	for (;;)
	{
		loopCount++;
		static int targ_loop = -1;
		if (targ_loop == loopCount)
			format();

		assertValid();
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
				levelCount++;
				if (pFirstActiveItem == nullptr)
				{
					recordSolution(presults);

					if (presults->size() < max_results)
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
				pChecksums[CurLevel].checksum(*this);
#endif

				int smallest_branch_factor = std::numeric_limits<int>::max();
				ItemHeader* pbest = nullptr;

				for (ItemHeader* pitem = pFirstActiveItem; pitem; pitem = pitem->pNextActive)
				{
					int branching_factor = pitem->branchingFactor();

					// This implements the non-sharp preference heuristic.
					// This is needed for the word rectangle problem, but not in general:
					if (NonSharpPreference)
					{
						if (branching_factor > 1 && pitem->pName[0] == '#')
						{
							branching_factor += 10000;
						}
					}

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

				pbest->UsedCount++;
				deactivateOrCover(pbest);

				state.pItem = pbest;
				state.pCurCell = pbest->pTopCell;

				// There are two different cases: The usage we are adding finishes this item, which causes
				// it to be covered. Or the item will still be active. If the item is still active, we only
				// consider as many sequences as we have to before branching again.
				if (pbest->UsedCount == pbest->Max)
				{
					state.Action = ag_TryX;
					state.TryCellCount = pbest->AvailableSequences;
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

				TRACE("\t\t%i - trying: %s\n", CurLevel, state.pCurCell->format());

				// If we selected the current cell, all the items it references get used:
				sequenceUsed(state.pCurCell);

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
				state.pItem->UsedCount--;
				reactivateOrUncover(state.pItem);

#ifndef NDEBUG
				assertValid();
				testChecksum();
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
				Solutions = presults->size();
				return Solutions != 0;
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
void AlgMPointer::showStats(std::ostream& stream) const
{
	cout << "Pointer based Exact cover with multiplicities and colors found " << Solutions << " solutions." << endl;

	if (NonSharpPreference)
		cout << "\tThe non-sharp preference heuristic was used." << endl;
	cout << "\tTime used (microseconds): " << setupTime << " for setup and " <<
		runTime << " to run." << endl;

	cout << "\tLoop ran " << loopCount << " times with " << levelCount << " level transitions." << endl;
}
///////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
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
#endif

