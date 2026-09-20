#ifndef COMPONENTS_SCORE_HPP
#define COMPONENTS_SCORE_HPP

#include "includes.hpp"

namespace score {

struct KilledScore
{
	int value;
};

struct Score
{
	int value = 0;
};

} // namespace score

using ::score::KilledScore;
using ::score::Score;

#endif // COMPONENTS_SCORE_HPP
