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

struct ExactCoverWithMultiplicitiesAndColors;

enum AlgXStates;

class ItemHeader
{
public:
	ItemHeader()
	{
		memset(this, 0, sizeof(*this));
	}

	const char* pName;

	// Linked list of active headers. An primary item can be active, inactive  and covered, or inactive and uncovered.
	ItemHeader* pPrevActive;
	ItemHeader* pNextActive;

	MCell* pTopCell;

	// If this is a primary item, max/min numbers allowed, else -1 for a secondary item:
	int Min;
	int Max;
	// How many sequences used this item:
	int StartingCount;

	bool isPrimary() const { return Max >= 0; }

	// How many times this item has been assigned in the current partial solution:
	int Current;

	int remaining() const { return StartingCount - Current; }

	// The branching factor is how many choices we have for the next sequence containing
	// this item.
	
	int branchingFactor() const
	{
		assert(Current <= Max);
		int needed = Min - Current;
		int branching_factor = remaining() - needed + 1;

		return branching_factor;
	}

	int slack() const
	{
		return Max - Current;
	}

	// For a secondary item, the currently active color.
	const char* pColor;	// or null if no color has been assigned.
};


class MCell
{
public:
	MCell()
	{
		memset(this, 0, sizeof(*this));
	}

	ItemHeader* pTop;

	const char* pColor; // or nullptr if not colored.

	// Next and previous cells in this sequence. Forms a circularly linked list.
	MCell* pLeft;
	MCell* pRight;

	// Previous and next sequences containing this item. Null terminated.
	MCell* pUp;		
	MCell* pDown;

	void format(std::ostream& stream) const
	{
		const MCell* pstart = this;
		// Start from a consistent point in the sequence. If the cells are block allocated in
		// order (and they are), this will start from the first cell:
		while (pstart->pLeft < this)
			pstart = pstart->pLeft;


		const MCell* pcell = pstart;
		do
		{
			if (pcell != pstart)
				stream << " ";

			stream << pcell->pTop->pName;

			if (pcell->pTop->pColor)
			{
				stream << ":" << pcell->pTop->pColor;
			}

			pcell = pcell->pRight;
		} while (pcell != pstart);
	}

	const char *format() const
	{
		std::ostringstream buf;
		format(buf);
		static char cbuf[1024];
		auto l = std::min(buf.str().length(), sizeof(cbuf) - 1);
		memcpy(cbuf, buf.str().c_str(), l);
		cbuf[l] = 0;

		return cbuf;
	}

};


class AlgMPointer
{
	ItemHeader* pFirstActiveItem;
	
	size_t TotalItems;
	ItemHeader* pHeaders;

	int MaxItems;

	size_t TotalCells;
	MCell* pCells;

	std::map<MCell*, int> SequenceMap;

	const ExactCoverWithMultiplicitiesAndColors& Problem;

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

	int CurLevel;
	struct LevelState
	{
		AgActions Action;
		ItemHeader* pItem;
		MCell*	pCurCell;
		MCell* pStartingCell;
		int TryCellCount;

#ifndef NDEBUG
		unsigned int EntryCheckSum;
#endif

	};
	LevelState* pLevelState;

	ItemHeader* getItem(const char* pc, size_t len);

	// Unlink/relink a single cell in the corresponding list of items:
	void unlinkCellVertically(MCell* pcell);
	void relinkCellVertically(MCell* pcell);

	void unlinkItem(ItemHeader* pitem);
	void cover(ItemHeader* pitem);		// cover is called when an item is being chosen.
	void sequenceUsed(MCell* pcell);   
	void deactivateOrCover(ItemHeader* pitem);


	void tweak(MCell* pcell);
	void untweak_all();

	void hide(MCell* pcell);	// hide/unhide removes the sequence containing pcell.
	void unhide(MCell* pcell);
	void relinkItem(ItemHeader* pitem);
	void uncover(ItemHeader* pitem);	
	void sequenceReleased(MCell* pcell);
	void reactivateOrUncover(ItemHeader* pitem);
	void recordSolution(std::vector<std::vector<int>>* presults);

	void checkValid() const;
	unsigned long checksum();
public:
	AlgMPointer(const ExactCoverWithMultiplicitiesAndColors& problem);
	~AlgMPointer();

	void format(std::ostream& stream) const;
	void print();

	bool testUncoverCover();

	bool exactCover(std::vector<std::vector<int>>* presults, int max_results = 1);

	long setupTime;
	long runTime;
	long long loopCount;
};

