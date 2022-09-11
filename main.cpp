
#include <iostream>
#include "ProblemGenerator.h"
#include "WordSearch.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>

void x_small_problem();

///////////////////////////////////////////////////////////////////////////////
// Figure 49, page 87 of 7.2.2.1:


/*const char* knuth_sample[] = {
	"p", "q", "x", "y:A", nullptr,
	"p", "r", "x:A", "y", nullptr,
	"p", "x:B", nullptr,
	"q", "x:A", nullptr,
	"r", "y:B", nullptr,
	nullptr,
};*/

const char* knuth_sample[] = {
	"aargh",   "0_0:a",   "0_1:a",   "0_2:r",   "0_3:g",   "0_4:h", nullptr,
	"arg",   "0_0:a",   "0_1:r",   "0_2:g", nullptr,
	"arg",   "0_1:a",   "0_2:r",   "0_3:g", nullptr,
	"arg",   "0_2:a",   "0_3:r",   "0_4:g", nullptr,
	nullptr
};

bool exact_cover_strings(const ExactCoverWithColors& problem);

void make_word_search()
{
	WordSearch word_search(5, 5);
	bool b = word_search.readWordList("short_list.txt");
	assert(b);

	ExactCoverWithColors problem;
	word_search.makeCoverProblem(&problem);

	b = exact_cover_strings(problem);
	cout << "Exact cover returned: " << b << endl;
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);


	// Various combinations of problems you can run by uncommenting.


	// Routines with an c_ prefix run the original algorithm modified to use character
	// strings as items. The number of items can be unbounded.
	//x_small_problem();
	//x_large_problem();


	// Routines with the a ptr_ prefix run an algorithm implemented with pointers, instead
	// of indices. This is closer to what a modern C++ programmer might write, although
	// it turns out to be slower.
	//ptr_small_problems();
	//ptr_large_problem();
	//ptr_very_large_problem();

	make_word_search();

	assert(_CrtCheckMemory());
	cout << "Done!\n";
}

