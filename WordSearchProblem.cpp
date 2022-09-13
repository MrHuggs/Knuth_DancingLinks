
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <fstream>
#include <sstream>

#include "Common.h"
#include "WordSearchProblem.h"


using namespace std;
///////////////////////////////////////////////////////////////////////////////
WordSearchProblem::WordSearchProblem(int w, int h)
{
    width = w;
	height = h;

    char buf[2];
    buf[1] = 0;
    for (int i = 0; i < 26; i++)
    {
        buf[0] = 'a' + i;
        letters.emplace_back(buf);
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
		{
            char buf[32];
            sprintf_s(buf, "%i_%i", y, x);
            positions.emplace_back(buf);
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
bool WordSearchProblem::readWordList(const char* pfile_name)
{
    ifstream file(pfile_name);
    if (!file.is_open())
    {
        cout << "Failed to open file: " << pfile_name;
        return false;
    }
    std::string line, word;
    while (std::getline(file, line))
    {
        if (line.find("#") != string::npos)
            continue;

        std::istringstream iss(line);
        while (iss >> word)
        {
            wordList.emplace_back(word);
        }
    }

    return true;
}
///////////////////////////////////////////////////////////////////////////////
void WordSearchProblem::makeCoverProblem(ExactCoverWithColors* pproblem)
{
    pproblem->primary_options.resize(wordList.size());
    for (int i = 0; i < wordList.size(); i++)
    {
        pproblem->primary_options[i] = wordList[i].c_str();
    }

    pproblem->colors.resize(letters.size());
    for (int i = 0; i < letters.size(); i++)
    {
        pproblem->colors[i] = letters[i].c_str();
    }

    pproblem->secondary_options.resize(positions.size());
    for (int i = 0; i < positions.size(); i++)
    {
        pproblem->secondary_options[i] = positions[i].c_str();
    }

    // Want a sequence for every word starting at every position in every
    // legal orientation.
    for (int i = 0; i < wordList.size(); i++)
    {
        int l = (int) wordList[i].length();
        assert(l <= width || l <= height); // Otherwise it could never fit.
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                for (int ydir = -1; ydir <= 1; ydir++)
                {
                    int final_y = y + ydir * (l - 1);
                    if (final_y < 0 || final_y >= height)
                        continue;

                    for (int xdir = -1; xdir <= 1; xdir++)
                    {
                        if (xdir == 0 && ydir == 0)
                            continue;

                        int final_x = x + xdir * (l - 1);
						if (final_x < 0 || final_x >= width)
                        continue;

                        std::vector<const char*> ptr_vec;
                        ptr_vec.push_back(pproblem->primary_options[i]);
                        for (int j = 0; j < l; j++)
                        {
                            int cx = x + xdir * j;
                            int cy = y + ydir * j;
                            int idx = cy * width + cx;
                            const char* pos_text = pproblem->secondary_options[idx];

                            char buf[32];
                            char c = (pproblem->primary_options[i])[j];
                            sprintf_s(buf, "%s:%c", pos_text, c);
                            letter_positions.push_back(buf);

                            ptr_vec.push_back(letter_positions.back().c_str());
                        }
                        pproblem->sequences.emplace_back(ptr_vec);
                    }
                }
            }
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
bool WordSearchProblem::decodeSequenceItem(const char* pitem, int* prow, int* pcolumn, char* pc)
{
    char buf[32];
    char c;
    int i = 0;
    while ((c = *pitem++) != '_')
    {
        if (c == 0)
            return false;
        buf[i++] = c;
    }
    buf[i] = 0;
    *prow = atoi(buf);

    i = 0;
    while ((c = *pitem++) != ':')
    {
        if (c == 0)
            return false;
        buf[i++] = c;
    }
    buf[i] = 0;
    *pcolumn = atoi(buf);


    *pc = *pitem;

    return true;
}
///////////////////////////////////////////////////////////////////////////////
WordSearch::WordSearch(int w, int h)
{
    width = w;
    height = h;
    int nc = w * h;
    pLetters = new char[w * h];
    memset(pLetters, 0xff, nc);
}
///////////////////////////////////////////////////////////////////////////////
WordSearch::~WordSearch()
{
    delete[] pLetters;
}
///////////////////////////////////////////////////////////////////////////////
char WordSearch::letter(int row, int column) const
{
    assert(column >= 0 && column  < width);
    assert(row >= 0 && row < height);
    return pLetters[column + row * width];
}
///////////////////////////////////////////////////////////////////////////////
void WordSearch::setLetter(int row, int column, char c)
{
    assert(column >= 0 && column < width);
    assert(row >= 0 && row < height);
    pLetters[column + row * width] = c;
}
///////////////////////////////////////////////////////////////////////////////
void WordSearch::applySolution(const ExactCoverWithColors& problem, const std::vector<int> results)
{
    for (auto idx_seq : results)
    {
        auto sequence = problem.sequences[idx_seq];
        assert(sequence.size() > 1);



        int row, column;
        char c;

        for (auto pc : sequence)
        {
            if (WordSearchProblem::decodeSequenceItem(pc, &row, &column, &c))
            {
                setLetter(row, column, c);
            }
        }
    }
}
///////////////////////////////////////////////////////////////////////////////
void WordSearch::format(std::ostream& stream, int xspacing, int yspacing)
{
    char* ppad = (char *)alloca(xspacing + 1);
    memset(ppad, ' ', xspacing);
	ppad[xspacing] = 0;

    char* vpad = (char*)alloca(yspacing + 1);
    memset(vpad, '\n', yspacing);
    vpad[yspacing] = 0;

    for (int row = 0; row < height; row++)
    {
	    for (int column = 0; column < width; column++)
	    {
            char c = letter(row, column);

            if (c < 0)
                c = '-';
            stream << c;
            if (column < width - 1)
                stream << ppad;

	    }
        stream << endl;

        if (row < height - 1)
            stream << vpad;
    }

}
///////////////////////////////////////////////////////////////////////////////
void WordSearch::print()
{
    format(cout, 2, 1);
}
