#include "Common.h"
#include "PartridgePuzzle.h"

using namespace std;

void PartridgePuzzle::generateProblem(ExactCoverWithMultiplicitiesAndColors* pproblem)
{
	int N = n * (n + 1) / 2;

	squareNames.resize(n);
	squareIds.resize(n);
	positionNames.resize(N * N);
	squaresWithColor.resize(N * N * n);

	pproblem->primary_options.resize(n);
	pproblem->secondary_options.resize(N * N);
	pproblem->colors.resize(n);


	char buf[64];
	for (int i = 0; i < n; i++)
	{
		sprintf_s(buf, "#%i", i + 1);
		squareNames[i] = buf;
		squareIds[i] = buf + 1;

		pproblem->primary_options[i].pValue = squareNames[i].c_str();
		pproblem->primary_options[i].u = i + 1;
		pproblem->primary_options[i].v = i + 1;

		pproblem->colors[i] = squareIds[i].c_str();
	}

	for (int row = 0; row < N; row++)
	{
		for (int column = 0; column < N; column++)
		{
			sprintf_s(buf, "(%i,%i)", row, column);

			int idx = row * N + column;
			positionNames[idx] = buf;

			pproblem->secondary_options[idx] = positionNames[idx].c_str();

			for (int i = 0; i < n; i++)
			{
				sprintf_s(buf, "(%i,%i):%i", row, column, i + 1);
				squaresWithColor[idx * n + i] = buf;
			}
		}
	}

	for (int i = 0; i < n; i++)
	{
		for (int row = 0; row < N - i; row++)
		{
			for (int column = 0; column < N - i; column++)
			{
				std::vector<const char*> ptr_vec;
				ptr_vec.push_back(pproblem->primary_options[i].pValue);

				for (int y = 0; y <= i; y++)
				{
					for (int x = 0; x <= i; x++)
					{
						int idx = (row + y)* N + column + x;
						ptr_vec.push_back(squaresWithColor[idx * n + i].c_str());
					}
				}
				pproblem->sequences.emplace_back(ptr_vec);
			}
		}
	}


}