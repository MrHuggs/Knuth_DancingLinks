//
// Some helper functions and classes to convert character sequences to string
// sequences.

#pragma once


#include <cassert>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_set>

// Want conditional output that completely compiles away when not needed.
#ifndef NDEBUG
#define ENABLE_TRACE
#endif

#ifdef ENABLE_TRACE
#include <stdio.h>
extern bool EnableTrace;

#define TRACE if (EnableTrace) printf

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
const char* StateName(AlgXStates state);
///////////////////////////////////////////////////////////////////////////////
void print_sequences(const std::vector< std::vector<const char*> >& sequences);
///////////////////////////////////////////////////////////////////////////////
// Helper to diff to strings. Useful for the comparing the output of format
// above.
bool print_diff(std::string s1, std::string s2);
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

	void format(std::ostream& stream) const;
	
	void assertValid() const;

	void format_sequence(int idx_seq, std::ostream& stream) const;

	void print() const
	{
		format(std::cout);
	}

	void format_solution(std::vector<std::vector<int>>& results, std::ostream& stream) const;

	void print_solution(std::vector<std::vector<int>>& results)
	{
		format_solution(results, std::cout);
	}
};

