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
	// Generated numbers in a range, which is inclusive of the end points.
	RandRange(int min, int max, unsigned int seed)
	{
		applySeed(seed);

		Min = min;
		Range = max - min + 1;

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

		int ntest = (range + 1) * 1000;
		int counts[range + 1] = { 0 };

		for (int i = 0; i < ntest; i++)
		{
			int r = rr();
			assert(r >= min);
			assert(r <= max);
			counts[r - min]++;
		}

		for (int i = min; i <= max; i++)
		{
			std::cout << i << " - " << counts[i - min] << std::endl;
		}
	}
#endif
};
///////////////////////////////////////////////////////////////////////////////
class ProblemGenerator
{
	// Class to generate a difficult exact cover problem.
	// The default values of items/counts/seed produce a problem that takes
	// about 5 seconds to solve.
	//

	int Items;
	int SequencesCount;
	int MinSequenceLength;
	int MaxSequenceLength;

public:

	ProblemGenerator(int items = 60, int sequence_count = 500, int min_sequence_length = 4, int max_sequence_length = 5)
	{
		Items = items;
		SequencesCount = sequence_count;
		MinSequenceLength = min_sequence_length;
		MaxSequenceLength = max_sequence_length;
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
		
	void Generate(unsigned int seed = 0xdeadbeef)
	{

		RandRange rng(0, Items - 1, seed);
		RandRange rng_len(MinSequenceLength, MaxSequenceLength, ~seed);

		Sequences.clear();

		std::vector<int> usage_counts;

		usage_counts.resize(Items);
		int nused = 0;

		for (int i = 0; i < SequencesCount; i++)
		{

			std::vector<const char*> ptr_vec;
			std::unordered_set<int> seen_sequence;

			if (i == SequencesCount - 1 && nused < Items)
			{
				// If we haven't used all the items at least once, make a sequence
				// with all the unused items.
				for (int j = 0; j < Items; j++)
				{
					if (usage_counts[j] == 0)
					{
						char buf[32];
						_itoa_s(j + 1, buf, 32, 10);

						ptr_vec.emplace_back(_strdup(buf));
					}
				}
			}
			else
			{
				int len = rng_len();

				for (int j = 0; j < len; j++)
				{

					int val = rng();

					if (seen_sequence.find(val) != seen_sequence.end())
					{
						continue;
					}
					seen_sequence.insert(val);
					if (usage_counts[val]++ == 0)
					{
						nused++;
					}


					char buf[32];
					_itoa_s(val + 1, buf, 32, 10);

					ptr_vec.emplace_back(_strdup(buf));
				}
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
