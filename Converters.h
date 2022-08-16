#pragma once

#include <string>
#include <iostream>
#include <vector>
///////////////////////////////////////////////////////////////////////////////
// Helper to diff to strings. Useful for the comparing the output of format
// above.
inline bool print_diff(std::string s1, std::string s2)
{
	const char* p1 = s1.c_str();
	const char* p2 = s2.c_str();

	std::string result;
	bool equal = true;
	while (*p1 && *p2)
	{
		if (*p1 == *p2)
			result.push_back(*p1);
		else
		{
			result.push_back('*');
			equal = false;
		}
		p1++;
		p2++;
	}
	while (*p1)
	{
		equal = false;
		result.push_back(*p1++);
	}
	while (*p1)
	{
		equal = false;
		result.push_back(*p2++);
	}

	if (!equal)
	{
		std::cout << "String differ\n";
		std::cout << result;
	}
	else
	{
		//cout << "String the same!\n";
	}

	return equal;
}
///////////////////////////////////////////////////////////////////////////////
class ConvertedCharProblem
{
public:

	std::vector< std::vector<const char*> > stringPointers;

	ConvertedCharProblem(const char* pstrings[], int max_strings = std::numeric_limits<int>::max())
	{
		const char* pc;
		while ((pc = *pstrings++) != nullptr)
		{
			std::vector<const char*> ptr_vec;

			while (*pc)
			{
				char* pbuf = new char[2];
				pbuf[0] = *pc++;
				pbuf[1] = 0;
				ptr_vec.emplace_back(pbuf);
			}
			stringPointers.emplace_back(ptr_vec);

			if (stringPointers.size() == max_strings)
				break;
		}

	}
	~ConvertedCharProblem()
	{
		for (auto ptr_vec : stringPointers)
		{
			for (auto pc : ptr_vec)
			{
				delete[] pc;
			}
		}
	}
};
//////////////////////////////////////////////////////////////////////////////////
class StringArrayConverter
{
public:


	std::vector< std::vector<const char*> > stringPointers;

	StringArrayConverter(const char* pstrings[], int max_strings = std::numeric_limits<int>::max())
	{
		while (*pstrings != nullptr)
		{
			std::vector<const char*> ptr_vec;

			while (*pstrings)
			{
				ptr_vec.push_back(*pstrings++);

			}
			stringPointers.emplace_back(ptr_vec);

			if (stringPointers.size() == max_strings)
				break;

			pstrings++;
		}
	}

};
