// See:
// https://www.mathpuzzle.com/partridge.html

struct ExactCoverWithMultiplicitiesAndColors;

class PartridgePuzzle
{
	int n;
	std::vector<std::string> squareNames;
	std::vector<std::string> positionNames;
	std::vector<std::string> squareIds;

	std::vector<std::string> squaresWithColor;


public:

	PartridgePuzzle(int _n) : n(_n) {}

	void generateProblem(ExactCoverWithMultiplicitiesAndColors* pproblem);
};