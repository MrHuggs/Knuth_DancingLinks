//
// Some helper functions and classes to convert character sequences to string
// sequences.

#pragma once


#include <string>
#include <iostream>
#include <vector>


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
class ConvertedCharProblem
{
public:

	std::vector< std::vector<const char*> > stringPointers;

	ConvertedCharProblem(const char* pstrings[], int max_strings = std::numeric_limits<int>::max())
	{
		const char* pc;
		while ((pc = *pstrings++) != nullptr)
		{
			std::vector<const char*> ptr_vec;

			while (*pc)
			{
				char* pbuf = new char[2];
				pbuf[0] = *pc++;
				pbuf[1] = 0;
				ptr_vec.emplace_back(pbuf);
			}
			stringPointers.emplace_back(ptr_vec);

			if (stringPointers.size() == max_strings)
				break;
		}

	}
	~ConvertedCharProblem()
	{
		for (auto ptr_vec : stringPointers)
		{
			for (auto pc : ptr_vec)
			{
				delete[] pc;
			}
		}
	}

	void Print() const
	{
		print_sequences(stringPointers);
	}
};
//////////////////////////////////////////////////////////////////////////////////
class StringArrayConverter
{
public:


	std::vector< std::vector<const char*> > stringPointers;

	StringArrayConverter(const char* pstrings[], int max_strings = std::numeric_limits<int>::max())
	{
		while (*pstrings != nullptr)
		{
			std::vector<const char*> ptr_vec;

			while (*pstrings)
			{
				ptr_vec.push_back(*pstrings++);

			}
			stringPointers.emplace_back(ptr_vec);

			if (stringPointers.size() == max_strings)
				break;

			pstrings++;
		}
	}

	void Print() const
	{
		print_sequences(stringPointers);
	}
};
