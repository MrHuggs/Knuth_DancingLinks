
#include <iostream>

#include "Common.h"
#include "PartridgePuzzle.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>

bool exact_cover_strings(const ExactCoverWithMultiplicitiesAndColors& problem, vector<int>* presults);

void x_small_problem();


void partridge_problem()
{
	PartridgePuzzle puzzle(2);

	ExactCoverWithMultiplicitiesAndColors problem;

	puzzle.generateProblem(&problem);

	vector<int> results;
	exact_cover_strings(problem, &results);
}


class SimpleA : public ExactCoverWithMultiplicitiesAndColors
{

public:
	SimpleA(int min = 1, int max = 1)
	{
		primary_options.resize(1);
		primary_options[0].pValue = "A";
		primary_options[0].u = min;
		primary_options[0].v = max;

		sequences.resize(min);
		for (int i = 0; i < min; i++)
		{
			sequences[i].push_back("A");
		}
	}
	bool test()
	{
		vector<int> results;
		bool b = exact_cover_strings(*this, &results);
		assert(b);
		return b;
	}
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_crtBreakAlloc = 0;	// If you need to debug a particular alloc.

	//x_small_problem();

#if 0
	{
		SimpleA sa;
		sa.test();
	}
#else
	{
		SimpleA sa(2, 3);
		sa.test();
	}
#endif

	//partridge_problem();

	assert(_CrtCheckMemory());
	cout << "Done!\n";
}

