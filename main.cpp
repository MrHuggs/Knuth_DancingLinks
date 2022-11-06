
#include <iostream>

#include "AlgMPointer.h"
#include "Common.h"
#include "PartridgePuzzle.h"
#include "WordRectangle.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>

#define USE_POINTER_VERSION

#ifndef USE_POINTER_VERSION

bool exact_cover_with_multiplicities_and_colors(const ExactCoverWithMultiplicitiesAndColors& problem, 
						vector<vector<int>>* presults, int max_results, bool non_sharp_preference = false);
void print_exact_cover_with_multiplicities_and_colors_stats();
#endif

class SimpleTester : public ExactCoverWithMultiplicitiesAndColors
{
public:
#ifdef USE_POINTER_VERSION
	bool test()
	{
		vector<vector<int>> results;

		AlgMPointer alg(*this);

		bool b = alg.exactCover(&results, 20);
		assert(b);
		print_solution(results);
		alg.showStats();
		return b;
	}
#else
	bool test()
	{
		vector<vector<int>> results;
		bool b = exact_cover_with_multiplicities_and_colors(*this, &results, 20);
		assert(b);
		print_solution(results);
		print_exact_cover_with_multiplicities_and_colors_stats();
		return b;
	}
#endif
};

class SimpleA : public SimpleTester
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
};

class SimpleAB : public SimpleTester
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

};

class SimpleABPair : public SimpleTester
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

};

///////////////////////////////////////////////////////////////////////////////
class SimpleColoring : public SimpleTester
{
	// Example 49 from Knuth
public:
	SimpleColoring()
	{
		primary_options.resize(3);
		primary_options[0].pValue = "p";
		primary_options[0].u = 1;
		primary_options[0].v = 1;

		primary_options[1].pValue = "q";
		primary_options[1].u = 1;
		primary_options[1].v = 1;

		primary_options[2].pValue = "r";
		primary_options[2].u = 1;
		primary_options[2].v = 1;

		secondary_options.push_back("x");
		secondary_options.push_back("y");

		sequences.resize(5);

		sequences[0].push_back("p");
		sequences[0].push_back("q");
		// This is different from Knuth - we'll just create a unique color now.
		sequences[0].push_back("x:C");	
		sequences[0].push_back("y:A");

		sequences[1].push_back("p");
		sequences[1].push_back("r");
		sequences[1].push_back("x:A");
		sequences[1].push_back("y");

		sequences[2].push_back("p");
		sequences[2].push_back("x:B");

		sequences[3].push_back("q");
		sequences[3].push_back("x:A");

		sequences[4].push_back("r");
		sequences[4].push_back("y:B");

		colors.push_back("A");
		colors.push_back("B");
		colors.push_back("C");
	}
};
///////////////////////////////////////////////////////////////////////////////

void test()
{
#define PREVIOUS 1
#if PREVIOUS
	 {
		SimpleA sa;
		assert(sa.test());
	}
#endif

#if PREVIOUS
	 {
		SimpleA sa(2, 3, 2);
		assert(sa.test());
	}
#endif

#if PREVIOUS
	{
		SimpleA sa(2, 3, 3);
		assert(sa.test());
	}
#endif
#if PREVIOUS
	{

		class SimpleAB sab(1, 1, 1, 1);
		assert(sab.test());
	}
#endif
	
#if PREVIOUS
	{
		class SimpleAB sab(2, 3, 1, 1);
		assert(sab.test());
	}
#endif
	// These are suspect:
#if PREVIOUS
	{
		SimpleA sa(2, 6, 10);
		assert(sa.test());
	}
#endif
#if PREVIOUS
	{
		class SimpleAB sab(2, 3, 2, 3);
		assert(sab.test());
	}
#endif
#if PREVIOUS
	{
		class SimpleABPair sab(3, 6, 3, 3);
		assert(sab.test());
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
void partridge_problem()
{
	PartridgePuzzle puzzle(8);

	ExactCoverWithMultiplicitiesAndColors problem;

	puzzle.generateProblem(&problem);

	vector<vector<int>> results;
	
	// There are over 1000 solution, which would take a long time.
	bool b;
#ifdef USE_POINTER_VERSION
	AlgMPointer alg(problem);
	b = alg.exactCover(&results, 8);
	alg.showStats();
#else
	b = exact_cover_with_multiplicities_and_colors(problem, &results, 8);
	print_exact_cover_with_multiplicities_and_colors_stats();
#endif

	if (b)
	{
		cout << "Found " << results.size() << " solutions:" << endl;
		for (int idx = 0; idx < results.size(); idx++)
		{
			cout << "Solution:" << endl;
			const vector<int>& result = results[idx];
			for (int i = 0; i < result.size(); i++)
			{
				problem.format_sequence(result[i], cout);
			}
			cout << "(end solution)" << endl;
		}
	}
	else
	{
		cout << "Puzzle cannot be solved." << endl;
	}
}
///////////////////////////////////////////////////////////////////////////////
void world_rectangle_problem()
{
	WordRectangle word_rectangle;

	//bool b = word_rectangle.readWords("20k_words.txt");
	bool b = word_rectangle.readWords("test_words.txt");	// A smaller, simpler test case.
	assert(b);
	cout << "Word list read successfully." << endl;

	ExactCoverWithMultiplicitiesAndColors problem;
	word_rectangle.generateProblem(&problem);

	cout << "Problem generated." << endl;

	//problem.print();

	vector<vector<int>> results;

#ifdef USE_POINTER_VERSION
	AlgMPointer alg(problem);
	b = alg.exactCover(&results, 100);
	alg.format();
#else
	b = exact_cover_with_multiplicities_and_colors(problem, &results, 100, 
		true  // We want the non-sharp preference heuristic
		);
	print_exact_cover_with_multiplicities_and_colors_stats();
#endif
	if (b)
	{
		cout << results.size() << " word rectangle(s) found" << endl;

		for (int idx = 0; idx < results.size(); idx++)
		{
			const vector<int> &result = results[idx];

			/*for (int i = 0; i < result.size(); i++)
			{
				problem.format_sequence(result[i], cout);
			}*/
			cout << "Rectangle:" << endl << endl;
			word_rectangle.writeRectangle(problem, result);
			cout << endl;
		}
	}
	else
	{
		cout << "Words cannot be placed." << endl;
	}

}
///////////////////////////////////////////////////////////////////////////////
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_crtBreakAlloc = 0;	// If you need to debug a particular alloc.

	//test();

	partridge_problem();
	//world_rectangle_problem();

	assert(_CrtCheckMemory());
	cout << "Done!\n";
}

