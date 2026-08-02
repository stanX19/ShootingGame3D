#include "catch2/catch_amalgamated.hpp"

#include "utils.hpp"

#include <cmath>

TEST_CASE("calculateCollisionTime returns the first future collision", "[collision]")
{
	float collisionTime = calculateCollisionTime(
		Vector3{-2.0f, 0.0f, 0.0f},
		Vector3{4.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(collisionTime == Catch::Approx(0.25f));
}

TEST_CASE("calculateCollisionTime preserves past-collision results", "[collision]")
{
	float collisionTime = calculateCollisionTime(
		Vector3{2.0f, 0.0f, 0.0f},
		Vector3{1.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(collisionTime == Catch::Approx(-1.0f));
}

TEST_CASE("calculateCollisionTime reports overlapping spheres at zero", "[collision]")
{
	float collisionTime = calculateCollisionTime(
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(collisionTime == Catch::Approx(0.0f));
}

TEST_CASE("calculateCollisionTime reports tangent contact", "[collision]")
{
	float collisionTime = calculateCollisionTime(
		Vector3{-1.0f, 1.0f, 0.0f},
		Vector3{2.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(collisionTime == Catch::Approx(0.5f));
}

TEST_CASE("calculateCollisionTime rejects a missed collision", "[collision]")
{
	float collisionTime = calculateCollisionTime(
		Vector3{-2.0f, 2.0f, 0.0f},
		Vector3{4.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(collisionTime == Catch::Approx(-1.0f));
}

TEST_CASE("willCollide scalar overload checks inclusive time bounds", "[collision]")
{
	REQUIRE(willCollide(0.0f, 1.0f));
	REQUIRE(willCollide(1.0f, 1.0f));
	REQUIRE_FALSE(willCollide(-0.001f, 1.0f));
	REQUIRE_FALSE(willCollide(1.001f, 1.0f));
}

TEST_CASE("willCollide vector overload checks the complete motion interval", "[collision]")
{
	REQUIRE(willCollide(
		Vector3{-2.0f, 0.0f, 0.0f},
		Vector3{4.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f,
		1.0f
	));
	REQUIRE(willCollide(
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		1.0f,
		1.0f
	));
	REQUIRE_FALSE(willCollide(
		Vector3{-2.0f, 0.0f, 0.0f},
		Vector3{0.5f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f,
		1.0f
	));
	REQUIRE_FALSE(willCollide(
		Vector3{2.0f, 0.0f, 0.0f},
		Vector3{1.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f,
		1.0f
	));
}

TEST_CASE("calculateCollisionInterval exposes both collision endpoints", "[collision]")
{
	std::optional<CollisionInterval> interval = calculateCollisionInterval(
		Vector3{-2.0f, 0.0f, 0.0f},
		Vector3{4.0f, 0.0f, 0.0f},
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(interval);
	REQUIRE(interval->collisionStartDt == Catch::Approx(0.25f));
	REQUIRE(interval->collisionEndDt == Catch::Approx(0.75f));
}

TEST_CASE("calculateCollisionInterval represents stationary overlap", "[collision]")
{
	std::optional<CollisionInterval> interval = calculateCollisionInterval(
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		Vector3Zeros,
		1.0f
	);

	REQUIRE(interval);
	REQUIRE(interval->collisionStartDt == Catch::Approx(0.0f));
	REQUIRE(std::isinf(interval->collisionEndDt));
}
