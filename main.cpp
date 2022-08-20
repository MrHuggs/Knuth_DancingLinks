
#include <iostream>
#include "ProblemGenerator.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cassert>

void unit_test();
void small_problems();
void large_problem();

void x_small_problems();
void x_large_problem();
void x_very_large_problem();
bool exact_cover_strings(const vector< vector<const char*> >& sequences);

void ptr_small_problems();
void ptr_large_problem();
void ptr_very_large_problem();

///////////////////////////////////////////////////////////////////////////////
// Test strings where each sequence is a bunch of characters. This is the form
// Knuth gives his exampes:
const char* small_char_problems[] =
{
	"a", nullptr,
	"a", "b", nullptr,
	"a","ab","c", nullptr,
	"ce", "adg", "bcf",	"adf", "bg", "deg", nullptr,
	"147", "14", "457", "356", "2367", "27", nullptr,
	nullptr
};

const char* large_char_problem[] = {
	//"ce", "adg", "bcf",	"adf", "bg", "deg", nullptr,

	"svoxm", "shown", "ogipc", "hxmtq", "djxtf", "elroa", "hzytp", "gtnc", "zgwfc", "dvzwf", "oayuc", "rzgyt",
	"bgxtq", "jlxan", "oaqic", "jzxoq", "sdogw", "loafp", "dhlfc", "sdbkw", "drknu", "jogwt", "zvpe", "ehxgp",
	"sdev", "koznf", "dkvap", "jtqfp", "hjryu", "dejot", "sojf", "sekgw", "twrn", "ejvyt", "hjbpc", "lynqc",
	"dboaf", "hwad", "sejvp", "quev", "gwlk", "lzayc", "zgwym", "segyi", "mtkc", "jxfai", "shkyt", "odpc",
	"zlkc", "rtkn", "sjlnc", "boytu", "ohiv", "hbvwf", "egynt", "dvxaq", "jlwtq", "skgnp", "sjvzx", "dbgfp", "girp",
	"tuge", "stqip", "dehwq", "htvx", "hlkqc", "sfqui", "sqgy", "dhwmu", "bwuy", "vnquc", "moaf", "zbgv", "hwun",
	"drkxc", "xgyfp", "ehvnf", "wup", "seyip", "cbrx", "vxtui", "djrnf", "syvc", "ehrwy", "ehgxa", "dvzgu",
	"rxaqu", "ofev", "hten", "sganm", "zbwi", "zbuv", "rzwyt", "gwvh", "zxbc", "zxatu", "beh", "hrnfc", "qavc",
	"jlvwi", "slzxw", "vwmtp", "brmtc", "xqlc", "xqle", "sbzot", "sbzyc", "qikn", "sotq", "dblri", "jlaqu", "rgtp",
	"oqa", "sdhni", "bkgai", "mgit", "dhown", "gxytc", "zjnc", "djhmp", "jlrmf", "djlzg", "bkoqf", "sbhoq", "ognup",
	"jqtn", "oatup", "lvxwy", "oxgk", "minc", "vgin", "bjrnp", "euamf", "owytu", "sbomq", "jgupc", "sqvx", "hanmi",
	"bjkmc", "sayfc", "sehrg", "bowaq", "sqpx", "atfh", "zsrn", "dbvgm", "hlogt", "djoxg", "vozym", "duvc", "elrka",
	"dekzn", "rogmq", "ozek", "evznc", "kgwip", "owv", "sjknq", "bwqip", "juie", "dqan", "jqul", "zsfn", "dekou",
	"hiej", "mjuv",
	nullptr
};

///////////////////////////////////////////////////////////////////////////////
const char* vary_large_char_problem[] = {
#include "sequence.txt"
	nullptr,	// Terminator
};
///////////////////////////////////////////////////////////////////////////////
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	// Various combinations of problems you can run by uncommenting.

	// These run the original algorithm, which uses characters as items, and so
	// is limited in how many items you can have:
	//unit_test();
	//small_problems();
	//large_problem();


	// Routines with an x_ prefix run the original algorithm modified to use character
	// strings as items. The number of items can be unbounded.
	//x_small_problems();
	x_large_problem();
	//x_very_large_problem();

	// Routines with the a ptr_ prefix run an algorithm implemented with pointers, instead
	// of indices. This is closer to what a moder C++ programm might write, allthough
	// it turns out to be slower.
	//ptr_small_problems();
	ptr_large_problem();
	//ptr_very_large_problem();

	assert(_CrtCheckMemory());
	cout << "Done!\n";
}

