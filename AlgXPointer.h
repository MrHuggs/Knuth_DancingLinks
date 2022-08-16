#pragma once

// Pointer based versin of Knuth's Algorithm X.

#include <vector>
#include <map>

class ItemHeader;
class XCellHeader;
class XCell;


class XCell;

class ItemHeader
{
public:
	ItemHeader()
	{
		memset(this, 0, sizeof(*this));
	}

	const char* pName;

	// Linked list of active headers:
	ItemHeader* pPrevActive;
	ItemHeader* pNextActive;

	XCell* pTopCell;
};

class XCell
{
public:
	XCell()
	{
		memset(this, 0, sizeof(*this));
	}

	ItemHeader* pTop;

	// Next and previous cells in this sequence. Null terminated in both directions.
	XCell* pLeft;
	XCell* pRight;

	// Previous and next sequences containing this item. Null terminated.
	XCell* pUp;		
	XCell* pDown;

	void format(std::ostream& stream)
	{
		XCell* pcell = this;
		while (pcell->pLeft)
			pcell = pcell->pLeft;

		while (pcell)
		{
			stream << pcell->pTop->pName;
			pcell = pcell->pRight;
		}
	}
};


class AlgXPointer
{
	ItemHeader* pFirstActiveItem;


	struct CmpSame
	{
		bool operator()(const char* p1, const char* p2) const
		{
			return strcmp(p1, p2) < 0;
		}
	};
	std::map<const char*, ItemHeader*, CmpSame> itemHeaders;

	ItemHeader* getItem(const char* pc);

	void unlinkCellVertically(XCell* pcell);
	void relinkCellVertically(XCell* pcell);

	void cover(ItemHeader* pitem);

	void coverSeqItems(XCell* pcell);

	void hide(XCell* pcell);
	void unhide(XCell* pcell);
	void uncover(ItemHeader* pitem);
	void uncoverSeqItems(XCell* pcell);

	enum AlgXStates
	{
		ax_Initialize,
		ax_EnterLevel,
		ax_ChooseAndCover,
		ax_TryX,
		ax_TryAgain,
		ax_Backtrack,
		ax_LeaveLevel,
		ax_Cleanup,
	};

public:
	AlgXPointer(const std::vector< std::vector<const char*> >& sequences);
	~AlgXPointer();

	void format(std::ostream& stream);
	void print();

	bool testUncoverCover();


	bool exactCover();

	long setupTime;
	long runTime;
};

