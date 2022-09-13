#pragma once
#include "vector"
#include "ostream"

struct ExactCoverWithColors;

class WordSearchProblem
{
	int width;
	int height;

	std::vector<std::string> wordList;
	std::vector<std::string> letters;
	std::vector<std::string> positions;

	std::list<std::string> letter_positions;
public:
	WordSearchProblem(int w, int h);

	bool readWordList(const char* pfile_name);

	void makeCoverProblem(ExactCoverWithColors* pproblem);

	void format(std::ostream& stream);
	void print();

	static bool decodeSequenceItem(const char* pitem, int* prow, int* pcolumn, char* pc);
	
};

class WordSearch
{
	int width;
	int height;
	char* pLetters;

	char letter(int row, int column) const;
	void setLetter(int row, int column, char c);
public:

	WordSearch(int w, int h);
	~WordSearch();

	void applySolution(const ExactCoverWithColors& problem, const std::vector<int> results);

	void format(std::ostream& stream, int xspacing = 0, int yspacing = 0);
	void print();
};


