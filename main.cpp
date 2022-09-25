
#include <iostream>

#include "Common.h"
#include "PartridgePuzzle.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>

bool exact_cover_with_multiplicities_and_colors(const ExactCoverWithMultiplicitiesAndColors& problem, vector<int>* presults);
void print_exact_cover_with_multiplicities_and_colors_times();

class SimpleA : public ExactCoverWithMultiplicitiesAndColors
{

public:
	SimpleA(int min = 1, int max = 1, int seq = -1)
	{
		primary_options.resize(1);
		primary_options[0].pValue = "A";
		primary_options[0].u = min;
		primary_options[0].v = max;

		if (seq < 0)
			seq = min;
		sequences.resize(seq);
		for (int i = 0; i < seq; i++)
		{
			sequences[i].push_back("A");
		}
	}
	bool test()
	{
		vector<int> results;
		bool b = exact_cover_with_multiplicities_and_colors(*this, &results);
		assert(b);
		return b;
	}
};

class SimpleAB : public ExactCoverWithMultiplicitiesAndColors
{

public:
	SimpleAB(int mina = 1, int maxa = 1, int minb = 1, int maxb = 1)
	{
		primary_options.resize(2);
		primary_options[0].pValue = "A";
		primary_options[0].u = mina;
		primary_options[0].v = maxa;
		primary_options[1].pValue = "B";
		primary_options[1].u = minb;
		primary_options[1].v = maxb;

		sequences.resize(maxa + maxb);

		for (int i = 0; i < maxa; i++)
		{
			sequences[i].push_back("A");
		}
		for (int i = maxa; i < maxa + maxb; i++)
		{
			sequences[i].push_back("B");
		}
	}
	bool test()
	{
		vector<int> results;
		bool b = exact_cover_with_multiplicities_and_colors(*this, &results);
		assert(b);
		return b;
	}
};

class SimpleABPair : public ExactCoverWithMultiplicitiesAndColors
{

public:
	SimpleABPair(int mina = 1, int maxa = 1, int minb = 1, int maxb = 1)
	{
		primary_options.resize(2);
		primary_options[0].pValue = "A";
		primary_options[0].u = mina;
		primary_options[0].v = maxa;
		primary_options[1].pValue = "B";
		primary_options[1].u = minb;
		primary_options[1].v = maxb;

		int na = max(0, maxa - maxb);
		sequences.resize(na + maxb * 2);

		int idx = 0;
		for (int i = 0; i < maxb * 2; i++)
		{
			sequences[idx].push_back("A");
			sequences[idx++].push_back("B");
		}
		for (int i = 0; i < na; i++)
		{
			sequences[idx++].push_back("A");
		}
	}
	bool test()
	{
		vector<int> results;
		bool b = exact_cover_with_multiplicities_and_colors(*this, &results);
		assert(b);
		return b;
	}
};

void test()
{
	{
		SimpleA sa;
		assert(sa.test());
	}
	{
		SimpleA sa(2, 3, 2);
		assert(sa.test());
	}
	{
		SimpleA sa(2, 3, 3);
		assert(sa.test());
	}
	{

		class SimpleAB sab(1, 1, 1, 1);
		assert(sab.test());
	}
	{

		class SimpleAB sab(2, 3, 1, 1);
		assert(sab.test());
	}
	// These are suspect:
	{
		SimpleA sa(2, 6, 10);
		assert(sa.test());
	}
	{

		class SimpleAB sab(2, 3, 2, 3);
		assert(sab.test());
	}
	{

		class SimpleABPair sab(3, 6, 3, 3);
		assert(sab.test());
	}
}
///////////////////////////////////////////////////////////////////////////////
void partridge_problem()
{
	PartridgePuzzle puzzle(8);

	ExactCoverWithMultiplicitiesAndColors problem;

	puzzle.generateProblem(&problem);

	vector<int> results;
	bool b = exact_cover_with_multiplicities_and_colors(problem, &results);
	if (b)
	{
		cout << "Puzzle can be solved:" << endl;
		for (int i = 0; i < results.size(); i++)
		{
			problem.format_sequence(results[i], cout);
		}
	}
	else
	{
		cout << "Puzzle cannot be solved." << endl;
	}
	print_exact_cover_with_multiplicities_and_colors_times();
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_crtBreakAlloc = 0;	// If you need to debug a particular alloc.

	//test();

	partridge_problem();

	assert(_CrtCheckMemory());
	cout << "Done!\n";
}

