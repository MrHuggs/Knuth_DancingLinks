#include "Common.h"
#include "WordRectangle.h"
#include <fstream>
#include <direct.h>
#include <iostream>
#include <string>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
WordRectangle::WordRectangle()
{

	char* pos = positions;
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			*pos++ = '0' + row;
			*pos++ = '0' + column;
			*pos++ = 0;
		}
	}
	assert(pos - positions == sizeof(positions));

	pos = positions_with_char;
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			for (char c = 'a'; c <= 'z'; c++)
			{
				*pos++ = '0' + row;
				*pos++ = '0' + column;
				*pos++ = ':';
				*pos++ = c;
				*pos++ = 0;
			}
		}
	}
	assert(pos - positions_with_char == sizeof(positions_with_char));

	pos = rowNames;
	for (int row = 0; row < height; row++)
	{
		*pos++ = 'A';
		*pos++ = '0' + row;
		*pos++ = 0;
	}
	assert(pos - rowNames == sizeof(rowNames));

	pos = columnNames;
	for (int column = 0; column < width; column++)
	{
		*pos++ = 'D';
		*pos++ = '0' + column;
		*pos++ = 0;
	}
	assert(pos - columnNames == sizeof(columnNames));


	pos = letters;
	for (char c = 'a'; c <= 'z'; c++)
	{
		*pos++ = c;
		*pos++ = 0;
	}
	assert(pos - letters == sizeof(letters));

	pos = colors;
	for (char c = 'a'; c <= 'z'; c++)
	{
		*pos++ = c;
		*pos++ = 0;
	}
	for (int i = 0; i <= 1; i++)
	{
		*pos++ = '0' + i;
		*pos++ = 0;
	}
	assert(pos - colors == sizeof(colors));

	pos = hash_letters;
	for (char c = 'a'; c <= 'z'; c++)
	{
		*pos++ = '#';
		*pos++ = c;
		*pos++ = 0;
	}
	assert(pos - hash_letters == sizeof(hash_letters));


	pos = letterUsages;
	for (char c = 'a'; c <= 'z'; c++)
	{
		pos += sprintf_s(pos, 4, "%C:0", c) + 1;
		pos += sprintf_s(pos, 4, "%C:1", c) + 1;

	}
	assert(pos - letterUsages == sizeof(letterUsages));

	hash[0] = '#';
	hash[1] = 0;
}
///////////////////////////////////////////////////////////////////////////////
static string trim_string(const string &s)
{
	string result;
	for (auto c : s)
	{
		if (c >= 'a' && c <= 'z')
		{
			result.push_back(c);
		}
	}
	return result;
}
///////////////////////////////////////////////////////////////////////////////
// Create a string with the letters in the input. Essentially, we remove
// duplicates.
static string used_letters(const string& s)
{
	string result;
	for (auto c : s)
	{
		for (int j = 0; ; j++)
		{
			if (j == result.length())
			{
				result.push_back(c);
			}
			if (c == result[j])
			{
				break;
			}
		}
	}
	return result;
}
///////////////////////////////////////////////////////////////////////////////
bool WordRectangle::readWords(const char* pfile_name)
{
	for (;;)
	{
		ifstream infile(pfile_name);

		// Move up the directory tree if you need to:
		if (!infile.is_open())
		{
			char b1[_MAX_PATH], b2[_MAX_PATH];
			_getcwd(b1, sizeof(b1));
			_chdir("..");
			_getcwd(b2, sizeof(b2));

			if (strcmp(b1, b2) == 0)
			{
				cout << "Could not open words file " << pfile_name << endl;
				return false;
			}
			continue;
		}

		string line;
		while (getline(infile, line))
		{
			if (line.find('#') != string::npos)
			{
				continue;
			}
			string word = trim_string(line);

			if (word.length() == 4 && Words4.size() < lim4)
			{
				Words4.emplace_back(word);
				Words4Letters.emplace_back(used_letters(word));
			}
			else if (word.length() == 5 && Words5.size() < lim5)
			{
				Words5.emplace_back(word);
				Words5Letters.emplace_back(used_letters(word));
			}

		}
		return true;
	}
}
///////////////////////////////////////////////////////////////////////////////
void WordRectangle::generateProblem(ExactCoverWithMultiplicitiesAndColors* pproblem)
{
	pproblem->primary_options.resize(width + height + 26 + 1);

	int idx = 0;
	for (int row = 0; row < height; row++)
	{
		pproblem->primary_options[idx].u = 1;
		pproblem->primary_options[idx].v = 1;
		pproblem->primary_options[idx++].pValue = rowNames + row * 3;
	}
	for (int column = 0; column < width; column++)
	{
		pproblem->primary_options[idx].u = 1;
		pproblem->primary_options[idx].v = 1;
		pproblem->primary_options[idx++].pValue = columnNames + column * 3;
	}

	for (char c = 'a'; c <= 'z'; c++)
	{
		pproblem->primary_options[idx].u = 1;
		pproblem->primary_options[idx].v = 1;
		pproblem->primary_options[idx++].pValue = hash_letter(c);
	}

	pproblem->primary_options[idx].u = 1;
	pproblem->primary_options[idx].v = 8;
	pproblem->primary_options[idx].pValue = hash;

	pproblem->secondary_options.resize(width * height + 26);
	idx = 0;
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			pproblem->secondary_options[idx++] = pos(row, column);
		}
	}

	for (char c = 'a'; c <= 'z'; c++)
	{
		pproblem->secondary_options[idx++] = letter(c);
	}

	idx = 0;

	for (int i = 0; i < Words4.size(); i++)
	{
		const string& word = Words4[i];
		const string& letters = Words4Letters[i];

		for (int column = 0; column < width; column++)
		{
			std::vector<const char*> ptr_vec;
			ptr_vec.push_back(columnNames + column * 3);
			for (int row = 0; row < height; row++)
			{
				const char* popt = pos(row, column, word[row]);
				ptr_vec.push_back(popt);
			}

			for (auto c : letters)
			{
				ptr_vec.push_back(letterUsage(c, true));
			}
			pproblem->sequences.emplace_back(ptr_vec);
		}
	}

	// Sequences for the rows, for example:
	// A3 30:p 31:r 32:e 33:s 34:s p:1 r:1 e:1 s:1
	for (int i = 0; i < Words5.size(); i++)
	{
		const string& word = Words5[i];
		const string& letters = Words5Letters[i];

		for (int row = 0; row < height; row++)
		{
			std::vector<const char*> ptr_vec;
			ptr_vec.push_back(rowNames + row * 3);
			for (int column = 0; column < width; column++)
			{
				const char* popt = pos(row, column, word[column]);
				ptr_vec.push_back(popt);
			}
			for (auto c : letters)
			{
				ptr_vec.push_back(letterUsage(c, true));
			}
			pproblem->sequences.emplace_back(ptr_vec);
		}
	}

	// These sequences are for the letter counting:
	// ‘#a a:0’, ‘#a a:1 #’; ‘#b b:0’, ‘#b b:1 #’; . . . ; ‘#z z:0’, ‘#z z:1 #’.
	for (char c = 'a'; c <= 'z'; c++)
	{
		{
			std::vector<const char*> ptr_vec;
			ptr_vec.push_back(hash_letter(c));
			ptr_vec.push_back(letterUsage(c, false));
			pproblem->sequences.emplace_back(ptr_vec);
		}
		{
			std::vector<const char*> ptr_vec;
			ptr_vec.push_back(hash_letter(c));
			ptr_vec.push_back(letterUsage(c, true));
			ptr_vec.push_back(hash);
			pproblem->sequences.emplace_back(ptr_vec);
		}
	}

	pproblem->colors.resize(28);
	for (int i = 0; i < 28; i++)
	{
		pproblem->colors[i] = colors + i * 2;
	}
}
///////////////////////////////////////////////////////////////////////////////
void WordRectangle::writeRectangle(const ExactCoverWithMultiplicitiesAndColors& problem,
	const std::vector<int> results,
	std::ostream& stream,
	int xspacing, int yspacing
)
{

	char* ppad = (char*)alloca(xspacing + 1);
	memset(ppad, ' ', xspacing);
	ppad[xspacing] = 0;

	char* vpad = (char*)alloca(yspacing + 1);
	memset(vpad, '\n', yspacing);
	vpad[yspacing] = 0;

	int horizontal_sequences[height];
	memset(horizontal_sequences, -1, sizeof(horizontal_sequences));

	// Look the the sequences selected in the results. Since we created the sequence,
	// we can realy on the "A0" item being at the front and so forth.
	int found = 0;
	for (int i = 0; i < results.size(); i++)
	{
		auto seq = problem.sequences[results[i]];
		auto pfirst_item = seq[0];

		if (pfirst_item[0] == 'A')
		{
			int row = pfirst_item[1] - '0';

			assert(horizontal_sequences[row] == -1);
			horizontal_sequences[row] = results[i];
			found++;

			if (found == height)
				break;
		}
	}
	assert(found == height);

	for (int i = 0; i < height; i++)
	{
		auto seq = problem.sequences[horizontal_sequences[i]];
		assert(seq.size() > width);

		if (i > 0)
			stream << vpad;

		for (int j = 0; j < width; j++)
		{
			const char* pc = seq[j + 1];	// will look like 30:p or whatever
			char letter = pc[3];

			if (j > 0)
				stream << ppad;
			stream << letter;
		}
		stream << endl;

	}

	
}