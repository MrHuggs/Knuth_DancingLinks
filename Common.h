//
// Some helper functions and classes to convert character sequences to string
// sequences.

#pragma once


#include <cassert>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>

// Want conditional output that completely compiles away when not needed.
//#define ENABLE_TRACE
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
	ax_PrepareToBranch,
	ax_PossiblyTweak,
	ax_TryX,
	ax_TryAgain,
	ax_Restore,
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
		case ax_PrepareToBranch:	return "PrepareToBranch";
		case ax_PossiblyTweak:		return "PossiblyTweak";
		case ax_TryX:		return "TryX";
		case ax_TryAgain:	return "TryAgain";
		case ax_Restore:	return "Restore";
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

struct ExactCoverWithMultiplicitiesAndColors
{
	
	struct PrimaryOption
	{
		const char* pValue;
		int u;	// Minimum multiplicity
		int v;  // Max multiplicity
	};

	// Description of an exact cover with colors problem.
	std::vector<PrimaryOption> primary_options;
	std::vector<const char*> colors;
	std::vector<const char*> secondary_options;
	std::vector< std::vector<const char*> > sequences;

	void format(std::ostream& stream) const
	{
		stream << "ExactCoverWithMultiplicitiesAndColors problem." << std::endl;

		stream << primary_options.size() << " primary options." << std::endl;
		for (auto opt : primary_options)
		{
			stream << "\t" << opt.u << " <= m <= " << opt.v << ", " << opt.pValue << std::endl;
		}

		stream << colors.size() << " colors." << std::endl;
		for (auto opt : colors)
			stream << "\t" << opt << std::endl;

		stream << secondary_options.size() << " secondary options." << std::endl;
		for (auto opt : secondary_options)
			stream << "\t" << opt << std::endl;

		stream << sequences.size() << " sequences." << std::endl;
		for (int i = 0; i < sequences.size(); i++)
		{
			format_sequence(i, stream);
		}

	}

	void format_sequence(int idx_seq, std::ostream& stream) const
	{
		const std::vector<const char*>& seq = sequences[idx_seq];
		stream << "\t";
		for (auto opt : seq)
			stream << opt << "   ";
		stream << std::endl;
	}

	void print() const
	{
		format(std::cout);
	}
};

