//
// Some helper functions and classes to convert character sequences to string
// sequences.

#pragma once


#include <cassert>
#include <functional>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_set>

// Want conditional output that completely compiles away when not needed.
#define ENABLE_TRACE
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
	ax_RecordSolution,
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
		case ax_RecordSolution:	return "RecordSolution";
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

	struct CmpSame
	{
		bool operator()(const char* p1, const char* p2) const
		{
			return strcmp(p1, p2) < 0;
		}
	};
	
	void assertValid() const
	{

		std::map<const char*, int, CmpSame> primaries_set;		// All we really need is an unordered set...
		std::map<const char*, int, CmpSame> secondaries_set;	
		std::map<const char*, int, CmpSame> colors_set;

		// Check for duplicates and improper reuse:
		size_t max_len = 0;
		for (int i = 0; i < primary_options.size(); i++)
		{
			const PrimaryOption &o = primary_options[i];
			assert(primaries_set.find(o.pValue) == primaries_set.end());
			assert(o.u <= o.v);
			primaries_set[o.pValue] = 0;
			max_len = std::max(max_len, strlen(o.pValue));
		}

		for (auto pc : secondary_options)
		{
			assert(primaries_set.find(pc) == primaries_set.end());
			assert(secondaries_set.find(pc) == secondaries_set.end());
			secondaries_set[pc] = 0;
			max_len = std::max(max_len, strlen(pc));
		}

		for (auto pc : colors)
		{
			assert(colors_set.find(pc) == colors_set.end());
			colors_set[pc] = 0;
		}

		char* buf = (char*)_alloca(max_len + 1);
		for (int i = 0; i < sequences.size(); i++)
		{
			const std::vector<const char*> &seq = sequences[i];

			for (auto pc : seq)
			{
				const char* sep = strchr(pc, ':');
				if (sep)
				{
					int nc = (int)(sep - pc);
					assert(nc <= max_len);
					memcpy(buf, pc, nc);
					buf[nc] = 0;

					assert(secondaries_set.find(pc) != secondaries_set.end());
					assert(colors_set.find(sep + 1) != colors_set.end());
				}
				else
				{
					assert(primaries_set.find(pc) != primaries_set.end());
				}
			}
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

	void format_solution(std::vector<std::vector<int>> &results, std::ostream& stream) const
	{
		stream << results.size() << " solutions found:" << std::endl;

		for (int i = 0; i < results.size(); i++)
		{
			stream << "Solution " << i << std::endl;
			for (int j = 0; j < results[i].size(); j++)
			{
				format_sequence(results[i][j], stream);
			}
		}
	}
	void print_solution(std::vector<std::vector<int>>& results)
	{
		format_solution(results, std::cout);
	}
};

