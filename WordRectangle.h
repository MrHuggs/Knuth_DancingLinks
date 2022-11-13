#pragma once


struct ExactCoverWithMultiplicitiesAndColors;

class WordRectangle
{
	static const int width = 5;
	static const int height = 4;

	static const int lim5 = 2000;
	static const int lim4 = 1000;

	std::vector<std::string> Words4;
	std::vector<std::string> Words4Letters;
	std::vector<std::string> Words5;
	std::vector<std::string> Words5Letters;

	char letters[26 * 2];
	char hash_letters[26 * 3];
	char colors[26 * 2 + 2 * 2];

	char letterUsages[26 * 8];
	char rowNames[height * 3];
	char columnNames[width * 3];
	char positions[width * height * 3];
	char positions_with_char[width * height * 26 * 5];
	char hash[2];

	const char* letter(char c)
	{
		return letters + (c - 'a') * 2;
	}

	const char *hash_letter(char c)
	{
		return hash_letters + (c - 'a') * 3;
	}

	const char *pos(int row, int column) const 
	{
		int idx = row * width + column;
		return positions + idx * 3;
	}

	const char* pos(int row, int column, char c) const
	{
		int idx = (row * width + column) * 26 + c - 'a';
		return positions_with_char + idx * 5;
	}


	const char *letterUsage(char letter, bool yes_or_no)
	{
		int idx = (letter - 'a') * 8;
		if (yes_or_no)
			idx += 4;

		return letterUsages + idx;
	}

public:

	WordRectangle();

	bool readWords(const char *pfile_name);

	void generateProblem(ExactCoverWithMultiplicitiesAndColors* pproblem);

	// Once we have a solution, write the result:
	void writeRectangle(const ExactCoverWithMultiplicitiesAndColors& problem,
					const std::vector<int> results,
					std::ostream& stream = std::cout,
					int xspacing = 1, int yspacing = 1
					);
};