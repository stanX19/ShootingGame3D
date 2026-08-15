#include "catch2/catch_amalgamated.hpp"
#include "classes/ui/stat_bar.hpp"

TEST_CASE("stat bars clamp values above their maximum", "[unit][ui]")
{
	const ui::StatBar firepower{"FIREPOWER", 32.0f, 8.0f};

	CHECK(firepower.clampedValue() == Catch::Approx(8.0f));
	CHECK(firepower.normalizedValue() == Catch::Approx(1.0f));
}

TEST_CASE("stat bars clamp values below zero", "[unit][ui]")
{
	const ui::StatBar stat{"HP", -10.0f, 100.0f};

	CHECK(stat.clampedValue() == Catch::Approx(0.0f));
	CHECK(stat.normalizedValue() == Catch::Approx(0.0f));
}
