
#include <iostream>
#include "ProblemGenerator.h"
#include "WordSearchProblem.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>

void x_small_problem();

///////////////////////////////////////////////////////////////////////////////
// Figure 49, page 87 of 7.2.2.1:
const char* knuth_sample[] = {
	"p", "q", "x", "y:A", nullptr,
	"p", "r", "x:A", "y", nullptr,
	"p", "x:B", nullptr,
	"q", "x:A", nullptr,
	"r", "y:B", nullptr,
	nullptr,
};


bool exact_cover_strings(const ExactCoverWithColors& problem, vector<int>* presults);

void mathematicians_problem()
{
	int w = 12;
	int h = 15;
	WordSearchProblem word_search_problem(w, h);
	bool b = word_search_problem.readWordList("mathematicians.txt");
	assert(b);

	ExactCoverWithColors problem;
	word_search_problem.makeCoverProblem(&problem);

	vector<int> results;
	b = exact_cover_strings(problem, &results);
	cout << "Exact cover returned: " << b << endl;

	WordSearch word_search(w, h);
	word_search.applySolution(problem, results);
	word_search.print();
}
///////////////////////////////////////////////////////////////////////////////
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	x_small_problem();

	mathematicians_problem();

	assert(_CrtCheckMemory());
	cout << "Done!\n";
}

