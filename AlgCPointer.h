#pragma once

// Pointer based version of Knuth's Algorithm X.
// Some care is taken to make the same choices Knuth would make so
// performance can be compared.

#include <vector>
#include <map>

class ItemHeader;
class XCellHeader;
class XCell;


class XCell;
enum AlgXStates;

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

	int Count;
};

class XCell
{
public:
	XCell()
	{
		memset(this, 0, sizeof(*this));
	}

	ItemHeader* pTop;

	// Next and previous cells in this sequence. Forms a loop.
	XCell* pLeft;
	XCell* pRight;

	// Previous and next sequences containing this item. Null terminated.
	XCell* pUp;		
	XCell* pDown;

	const char *format()
	{
		const int bufsize = 256;
		static char buf[bufsize];	// Yes, this is not thread safe.

		buf[0] = 0;
		size_t curlen = 0;

		XCell* pcell = this;
		do
		{
			auto n = strlen(pcell->pTop->pName);

			if (curlen + n + 2 > bufsize)
			{
				assert(false);
				return "Error: out of buffer space! ";
			}

			memcpy(buf + curlen, pcell->pTop->pName, n);
			curlen += n;
			buf[curlen] = ' ';
			curlen++;

			pcell = pcell->pRight;
		} while (pcell != this);

		buf[curlen] = 0;
		assert(curlen < bufsize);

		return buf;
	}
};


class AlgCPointer
{
	ItemHeader* pFirstActiveItem;

	struct CmpSame
	{
		bool operator()(const char* p1, const char* p2) const
		{
			return strcmp(p1, p2) < 0;
		}
	};
	// All of the item headers, including ones that might not be currently
	// active.
	std::map<const char*, ItemHeader*, CmpSame> itemHeaders;

	ItemHeader* getItem(const char* pc);
	void sortItems();

	void unlinkCellVertically(XCell* pcell);
	void relinkCellVertically(XCell* pcell);

	void cover(ItemHeader* pitem);

	void coverSeqItems(XCell* pcell);

	void hide(XCell* pcell);
	void unhide(XCell* pcell);
	void uncover(ItemHeader* pitem);
	void uncoverSeqItems(XCell* pcell);


#ifdef NDEBUG
	void assertValid() const {}
#else
	void assertValid() const;
#endif

public:
	AlgCPointer(const std::vector< std::vector<const char*> >& sequences);
	~AlgCPointer();

	void format(std::ostream& stream);
	void print();

	bool testUncoverCover();


	bool exactCover();

	long setupTime;
	long runTime;
	long long loopCount;
};
