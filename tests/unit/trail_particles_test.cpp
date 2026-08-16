#include "catch2/catch_amalgamated.hpp"
#include "components.hpp"

TEST_CASE("trail emitters keep counted local spawn locations", "[unit][trail]") {
	SpawnsTrailParticles trail{};
	CHECK(SpawnsTrailParticles::maxSpawnLocations == 8);
	trail.spawnCount = 2;
	trail.spawnLocations[0] = Vector3{1.0f, 0.0f, -2.0f};
	trail.spawnLocations[1] = Vector3{-1.0f, 0.0f, -2.0f};
	trail.radius = 0.3f;
	trail.lifespan = 0.1f;
	trail.color = SKYBLUE;

	CHECK(trail.spawnCount == 2);
	CHECK(trail.spawnLocations[0].x == Catch::Approx(1.0f));
	CHECK(trail.spawnLocations[1].x == Catch::Approx(-1.0f));
	CHECK(trail.radius == Catch::Approx(0.3f));
	CHECK(trail.lifespan == Catch::Approx(0.1f));
	CHECK(trail.color.r == SKYBLUE.r);
}
