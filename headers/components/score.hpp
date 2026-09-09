#pragma once

#include "includes.hpp"

struct ScoreParent
{
	entt::entity parent = entt::null;
};

struct KilledScore
{
	int value;
};

struct Score
{
	int value = 0;
};
