#pragma once
#include "vector"
#include "ostream"

struct ExactCoverWithColors;

class WordSearch
{
	int width;
	int height;

	std::vector<std::string> wordList;
	std::vector<std::string> letters;
	std::vector<std::string> positions;

	std::list<std::string> letter_positions;
public:
	WordSearch(int w, int h);

	bool readWordList(const char* pfile_name);

	void makeCoverProblem(ExactCoverWithColors* pproblem);

	void format(std::ostream& stream);
	void print();
	
};


