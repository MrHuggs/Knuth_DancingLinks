//
// Some helper functions and classes to convert character sequences to string
// sequences.

#pragma once


#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_set>

// Want conditional output that completely compiles away when not needed.
#ifdef ENABLE_TRACE
#include <stdio.h>
#define TRACE printf

#else

#define TRACE(...) ((void) 0)

#endif

///////////////////////////////////////////////////////////////////////////////
// States of the algorithm. Corresponds to X1-X8.
// Note that some of these could be combined.
enum AlgXStates
{
	ax_Initialize,
	ax_EnterLevel,
	ax_Choose,
	ax_Cover,
	ax_TryX,
	ax_TryAgain,
	ax_Backtrack,
	ax_LeaveLevel,
	ax_Cleanup,
};
inline const char* StateName(AlgXStates state)
{
	switch (state)
	{
		case ax_Initialize: return "Initialize";
		case ax_EnterLevel: return "EnterLevel";
		case ax_Choose:		return "Choose";
		case ax_Cover:		return "Cover";
		case ax_TryX:		return "TryX";
		case ax_TryAgain:	return "TryAgain";
		case ax_Backtrack:	return "Backtrack";
		case ax_LeaveLevel:	return "LeaveLevel";
		case ax_Cleanup:	return "Cleanup";
		default:			assert(0); return "Error";
	}
}
///////////////////////////////////////////////////////////////////////////////
inline void print_sequences(const std::vector< std::vector<const char*> >& sequences)
{
	for (auto ptr_vec : sequences)
	{
		for (auto pc : ptr_vec)
		{
			std::cout << pc << ", ";
		}
		std::cout << std::endl;
	}
}
///////////////////////////////////////////////////////////////////////////////
// Helper to diff to strings. Useful for the comparing the output of format
// above.
inline bool print_diff(std::string s1, std::string s2)
{
	const char* p1 = s1.c_str();
	const char* p2 = s2.c_str();

	std::string result;
	bool equal = true;
	while (*p1 && *p2)
	{
		if (*p1 == *p2)
			result.push_back(*p1);
		else
		{
			result.push_back('*');
			equal = false;
		}
		p1++;
		p2++;
	}
	while (*p1)
	{
		equal = false;
		result.push_back(*p1++);
	}
	while (*p1)
	{
		equal = false;
		result.push_back(*p2++);
	}

	if (!equal)
	{
		std::cout << "String differ\n";
		std::cout << result;
	}
	else
	{
		//cout << "String the same!\n";
	}

	return equal;
}
///////////////////////////////////////////////////////////////////////////////
// Special comparator so we can make a map of string pointers eliminating
// duplicates:
struct CmpSame
{
	bool operator()(const char* p1, const char* p2) const
	{
		return strcmp(p1, p2) < 0;
	}
};

struct ExactCoverWithColors
{
	// Description of an exact cover with colors problem.
	std::vector<const char*> primary_options;
	std::vector<const char*> colors;
	std::vector<const char*> secondary_options;
	std::vector< std::vector<const char*> > sequences;

	void format(std::ostream& stream) const
	{
		stream << "ExactCoverWithColors problem." << std::endl;

		stream << primary_options.size() << " primary options." << std::endl;
		for (auto opt : primary_options)
			stream << "\t" << opt << std::endl;

		stream << colors.size() << " colors." << std::endl;
		for (auto opt : colors)
			stream << "\t" << opt << std::endl;

		stream << secondary_options.size() << " secondary options." << std::endl;
		for (auto opt : secondary_options)
			stream << "\t" << opt << std::endl;

		stream << sequences.size() << " sequences." << std::endl;
		for (auto seq : sequences)
		{
			stream << "\t";
			for (auto opt : seq)
				stream << opt << "   ";
			stream << std::endl;
		}

	}
	void print() const
	{
		format(std::cout);
	}
};

//////////////////////////////////////////////////////////////////////////////////
class StringArrayConverterWithColors
{

	static void sortVector(std::vector<const char*> &vec)
	{
		sort(vec.begin(), vec.end(), [](const char*& a, const char*& b) { return strcmp(a, b) < 0;	});

	}
public:

	ExactCoverWithColors Problem;

	StringArrayConverterWithColors(const char* pstrings[], int max_strings = std::numeric_limits<int>::max())
	{
		// Tracks items we have see and if they are secondary:
		std::map<const char*, bool, CmpSame> items;
		std::map<const char*, int, CmpSame> colors;		// Using a map for expediency, but really just need a set.

		while (*pstrings != nullptr)
		{
			std::vector<const char*> ptr_vec;

			while (*pstrings)
			{
				auto sep = strchr(*pstrings, ':');

				if (sep != nullptr)
				{
					// And item with a separator, so the item must be secondary:
					auto found_color = colors.find(sep + 1);
					if (found_color == colors.end())
					{
						auto pc = _strdup(sep + 1);
						colors[pc] = 1;
					}

					int nc = (int) (sep - *pstrings);
					char* pitem = (char*) alloca(nc + 1);
					memcpy(pitem, *pstrings, nc);
					pitem[nc] = 0;

					auto found_item = items.find(pitem);
					if (found_item == items.end())
					{
						auto pc = _strdup(pitem);
						items[pc] = true;		// Mark as secondary
					}
					else
					{
						found_item->second = true;
					}
				}
				else
				{
					auto found_item = items.find(*pstrings);
					if (found_item == items.end())
					{
						// Haven't seen the item before, so we'll add it, assuming it is not
						// secondary:
						items[_strdup (*pstrings)] = false;
					}
				}

				ptr_vec.push_back(*pstrings++);
			}
			Problem.sequences.emplace_back(ptr_vec);

			if (Problem.sequences.size() == max_strings)
				break;

			pstrings++;
		}
		for (auto it : items)
		{
			if (it.second)
			{
				Problem.secondary_options.emplace_back(it.first);
			}
			else
			{
				Problem.primary_options.emplace_back(it.first);
			}
		}
		for (auto it : colors)
			Problem.colors.emplace_back(it.first);

		sortVector(Problem.primary_options);
		sortVector(Problem.colors);
		sortVector(Problem.secondary_options);
	}

	~StringArrayConverterWithColors()
	{
		// All the placeholder strings were allocated with strdup; hence free with free().
		for (auto pc : Problem.primary_options)
		{
			free((void*)pc);
		}
		for (auto pc : Problem.secondary_options)
		{
			free((void*)pc);
		}
		for (auto pc : Problem.colors)
		{
			free((void*)pc);
		}
	}



	void Print() const
	{
		std::cout << "ExactCoverWithColors problem with " << Problem.primary_options.size() << " primary options:" << std::endl;
		printVector(Problem.primary_options);
		std::cout << Problem.secondary_options.size() << " secondary options:" << std::endl;
		printVector(Problem.secondary_options);
		std::cout << Problem.colors.size() << " colors:" << std::endl;
		printVector(Problem.colors);

		std::cout << Problem.sequences.size() << " Sequences:" << std::endl;
		for (auto seq : Problem.sequences)
		{
			printVector(seq, 32);
		}
	}
private:
	static void printVector(const std::vector<const char *>& v, int items_per_line = 16)
	{
		int nc = 0;

		for (const char* pc : v)
		{
			if (nc == items_per_line)
			{
				std::cout << std::endl;
				nc = 0;
			}
			else if (nc > 0)
				std::cout << " ";

			nc++;
			std::cout << pc;
		}
		std::cout << std::endl;
	}
};
