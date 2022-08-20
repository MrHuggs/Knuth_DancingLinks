#pragma once

// Helper class to generated sequences for testing.

#include <random>
#include <cassert>
#include <unordered_set>
#include "Common.h"
///////////////////////////////////////////////////////////////////////////////
class RandRange
{
//#define TEST_RAND_RANGE
#ifndef TEST_RAND_RANGE
	std::mt19937_64 gen64;

	const unsigned long long MaxRand = gen64.max();
	const unsigned long long rng() { return gen64(); }
	void applySeed(unsigned int seed) { gen64.seed(seed); }
#else
	unsigned long long Next = 0;

	const unsigned long long MaxRand = 255;
	const unsigned long long rng() { return Next++ % MaxRand; }
	void applySeed(unsigned int seed) { Next = seed; }

#endif

	int Min;
	int Range;
	unsigned long long Upper;
public:
	RandRange(int min, int max, unsigned int seed)
	{
		applySeed(seed);

		Min = min;
		Range = max - min;

		Upper = (MaxRand / Range) * Range;
	}

	int operator()()
	{
		unsigned long long r;
		do
		{
			r = rng();
		} while (r >=  Upper);

		return (int)(r % Range) + Min;
	}

#ifdef TEST_RAND_RANGE
	static void Test()
	{
		const int min = 10;
		const int max = 20;
		const int range = max - min;
		RandRange rr(min, max, 24);

		int ntest = range * 1000;
		int counts[range] = { 0 };

		for (int i = 0; i < ntest; i++)
		{
			int r = rr();
			assert(r >= min);
			assert(r < max);
			counts[r - min]++;
		}

		for (int i = 0; i < range; i++)
		{
			std::cout << counts[i] << std::endl;
		}
	}
#endif
};
///////////////////////////////////////////////////////////////////////////////
class ProblemGenerator
{
	int Items;
	int SequencesCount;
	int SequenceLength;

public:

	ProblemGenerator(int items, int sequence_count, int sequence_length)
	{
		Items = items;
		SequencesCount = sequence_count;
		SequenceLength = sequence_length;
	}

	~ProblemGenerator()
	{
		Clear();
	}

	
	std::vector< std::vector<const char*> > Sequences;

	void Clear()
	{
		for (auto ptr_vec : Sequences)
		{
			for (auto pc : ptr_vec)
			{
				free((void *) pc);	// Because allocated with strdup.
			}
		}
	}
		
	void Generate(unsigned int seed)
	{

		RandRange rng(0, Items - 1, seed);


		Sequences.clear();

		for (int i = 0; i < SequencesCount; i++)
		{
			std::vector<const char*> ptr_vec;

			std::unordered_set<int> seen_sequence;


			for (int j = 0; j < SequenceLength; j++)
			{

				int val = rng();

				if (seen_sequence.find(val) != seen_sequence.end())
				{
					continue;
				}
				seen_sequence.insert(val);

				char buf[32];
				_itoa_s(val, buf, 32, 10);
						
				ptr_vec.emplace_back(_strdup(buf));
			}


			Sequences.emplace_back(ptr_vec);
		}
		
	}

	void Print() const
	{
		print_sequences(Sequences);
	}

};
///////////////////////////////////////////////////////////////////////////////
