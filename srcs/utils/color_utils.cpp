#include "utils.hpp"
#include "shoot_3d.hpp"
#include <cmath>
#include <iostream>

Color colorMix(Color a, Color b, float weightA, float weightB)
{
	float totalWeight = weightA + weightB;
	if (totalWeight <= 0.0f)
		totalWeight = 1.0f; // prevent divide by zero

	float wa = weightA / totalWeight;
	float wb = weightB / totalWeight;

	Color result;
	result.r = (unsigned char)(a.r * wa + b.r * wb);
	result.g = (unsigned char)(a.g * wa + b.g * wb);
	result.b = (unsigned char)(a.b * wa + b.b * wb);
	result.a = (unsigned char)(a.a * wa + b.a * wb);
	return result;
}

Color colorRevert(Color a)
{
	Color result;
	result.r = (unsigned char)(255 - a.r);
	result.g = (unsigned char)(255 - a.g);
	result.b = (unsigned char)(255 - a.b);
	result.a = (unsigned char)(a.a);
	return result;
}
