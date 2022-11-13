		#pragma once

// Pointer based version of Knuth's Algorithm X.
// Some care is taken to make the same choices Knuth would make so
// performance can be compared.

#include <vector>
#include <map>
#include <cassert>
#include <sstream>
#include "AlgMPointer.h"

class ItemHeader;
class XCellHeader;
class MCell;
class AlgMPointer;

struct ExactCoverWithMultiplicitiesAndColors;
enum AlgXStates;

///////////////////////////////////////////////////////////////////////////////
class ItemHeader
{
	// Header for both primary and secondary items. Secondary items can be
	// used 0 more times and will have a color assigned to them.
public:
	ItemHeader()
	{
		memset(this, 0, sizeof(*this));
	}

	const char* pName;

	// Linked list of active headers. A primary item can be active, inactive and covered,
	// or inactive and uncovered.
	ItemHeader* pPrevActive;
	ItemHeader* pNextActive;

	MCell* pTopCell;

	// If this is a primary item, max/min numbers allowed, else -1 for a secondary item:
	int Min;
	int Max;

	bool isPrimary() const { return Max >= 0; }

	// How many times this item has been assigned in the current partial solution:
	int UsedCount;

	// How many sequences are available that use this item:
	int AvailableSequences;

	// The branching factor is how many choices we have for the next sequence containing
	// this item.
	int branchingFactor() const
	{
		assert(UsedCount <= Max);
		int needed = Min - UsedCount;
		int branching_factor = AvailableSequences - needed + 1;

		return branching_factor;
	}

	int slack() const
	{
		return Max - UsedCount;
	}

	// For a secondary item, the currently active color:
	const char* pColor;	// or null if no color has been assigned.
};
///////////////////////////////////////////////////////////////////////////////
class MCell
{
	// Represents an items used in a sequence:
public:
	MCell()
	{
		memset(this, 0, sizeof(*this));
	}

	// Previous and next sequences containing this item. Null terminated.
	MCell* pUp;
	MCell* pDown;

	ItemHeader* pTop;

	const char* pColor; // Will match one of the pointers in the input problem, or nullptr if not colored.

	// Next and previous cells in this sequence. Forms a circularly linked list.
	MCell* pLeft;
	MCell* pRight;


	void format(std::ostream& stream) const;
	const char* format() const;

};
///////////////////////////////////////////////////////////////////////////////
enum AgActions
{
	ag_Init,
	ag_EnterLevel,
	ag_TryX,
	ag_Tweak,
	ag_NextX,
	ag_TweakNext,
	ag_Restore,
	ag_LeaveLevel,
	ag_Done,
};
///////////////////////////////////////////////////////////////////////////////
struct LevelState
{
	AgActions Action;
	ItemHeader* pItem;
	MCell* pCurCell;
	MCell* pStartingCell;
	int TryCellCount;
};
///////////////////////////////////////////////////////////////////////////////
// The goal of dancing links is to be able to descend the search tree and
// then quickly backtrack. The backtracking code is supposed to restore
// the items & cells to their previous state exactly. The checksum code
// is used to make sure this works properly.
class AlgMChecksum
{
	ItemHeader* pHeaders;
	MCell* pCells;
public:
	AlgMChecksum();
	~AlgMChecksum();

	void init(const AlgMPointer& alg);

	void checksum(const AlgMPointer &alg);

	bool compare(const AlgMChecksum& other, const AlgMPointer& alg) const;
};
///////////////////////////////////////////////////////////////////////////////
class AlgMPointer
{
	friend class AlgMChecksum;

	ItemHeader* pFirstActiveItem;

	size_t TotalItems;
	ItemHeader* pHeaders;

	int MaxItems;

	size_t TotalCells;
	MCell* pCells;

	std::map<MCell*, int> SequenceMap;

	const ExactCoverWithMultiplicitiesAndColors& Problem;

	static inline const char* ActionName(AgActions action)
	{
		switch (action)
		{
			case ag_Init:				return "Init";
			case ag_EnterLevel:			return "EnterLevel";
			case ag_TryX:				return "TryX";
			case ag_Tweak:				return "Tweak";
			case ag_NextX:				return "NextX";
			case ag_TweakNext:			return "TweakNext";
			case ag_Restore:			return "Restore";
			case ag_LeaveLevel:			return "LeaveLevel";
			case ag_Done:				return "Done";

			default:			assert(0); return "Error";
		}
	}

	// This stores the search state as we go down the tree:
	int CurLevel;
	LevelState* pLevelState;

	// Heuristic that can be used with item selection:
	bool NonSharpPreference;

#ifndef NDEBUG
	AlgMChecksum* pChecksums;
	AlgMChecksum tempChecksum;
#endif

	ItemHeader* getItem(const char* pc, size_t len);
	// We 
	const char* getColor(const char* pc);

	// Unlink/relink a single cell in the corresponding list of items:
	void unlinkCellVertically(MCell* pcell);
	void relinkCellVertically(MCell* pcell);

	void unlinkItem(ItemHeader* pitem);
	void cover(ItemHeader* pitem);		// cover is called when an item is being chosen.
	void sequenceUsed(MCell* pcell);   
	void setcolor(MCell* pcell);
	void deactivateOrCover(ItemHeader* pitem);


	void tweak(MCell* pcell);
	void untweak_all();

	void hide(MCell* pcell);	// hide/unhide removes the sequence containing pcell.
	void unhide(MCell* pcell);
	void relinkItem(ItemHeader* pitem);
	void uncover(ItemHeader* pitem);	
	void sequenceReleased(MCell* pcell);
	void clearColor(MCell* pcell);
	void reactivateOrUncover(ItemHeader* pitem);
	void recordSolution(std::vector<std::vector<int>>* presults);

	bool isLinked(const ItemHeader* pitem) const;
	void assertValid() const;
	void testChecksum();
public:
	AlgMPointer(const ExactCoverWithMultiplicitiesAndColors& problem);
	~AlgMPointer();

	void setHeuristic(bool b) { NonSharpPreference = b; }
	void format(std::ostream& stream = std::cout) const;
	void print() { format(); }		// Just so we can call from the debugger if we want.

	bool exactCover(std::vector<std::vector<int>>* presults, int max_results = 1);

	// Metrics for stats:
	size_t Solutions;
	long setupTime;
	long runTime;
	long long loopCount;
	long long levelCount;

	void showStats(std::ostream& stream = std::cout) const;

#ifndef NDEBUG
	bool testUncoverCover();
#endif

};

