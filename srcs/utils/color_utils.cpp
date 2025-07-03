#include "utils.hpp"
#include "shoot_3d.hpp"
#include <cmath>
#include <iostream>

Color colorRevert(Color a)
{
	Color result;
	result.r = (unsigned char)(255 - a.r);
	result.g = (unsigned char)(255 - a.g);
	result.b = (unsigned char)(255 - a.b);
	result.a = (unsigned char)(a.a);
	return result;
}
