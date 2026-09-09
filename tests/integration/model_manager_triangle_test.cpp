#include "catch2/catch_amalgamated.hpp"
#include "classes/model_manager.hpp"
#include "raylib.h"

TEST_CASE("ModelManager creates and caches two-sided triangles", "[integration][model_manager]")
{
	ModelManager modelManager;

	Vector3 p1 = { 1.0f, 0.0f, 0.0f };
	Vector3 p2 = { 0.0f, 1.0f, 0.0f };
	Vector3 p3 = { -1.0f, -1.0f, 0.0f };

	t_model_id tri1 = modelManager.createTriangle(p1, p2, p3);
	t_model_id tri2 = modelManager.createTriangle(p1, p2, p3);

	CHECK(tri1 == tri2);
	CHECK(modelManager.isValid(tri1));
	CHECK(modelManager.getModelRadius(tri1) > 0.0f);
}

TEST_CASE("ModelManager polar createTriangle delegates to cartesian overload and caches", "[integration][model_manager]")
{
	ModelManager modelManager;

	t_model_id tri1 = modelManager.createTriangle(2.0f, 0.0f, 2.0f * PI / 3.0f, 4.0f * PI / 3.0f);
	t_model_id tri2 = modelManager.createTriangle(2.0f, 0.0f, 2.0f * PI / 3.0f, 4.0f * PI / 3.0f);

	CHECK(tri1 == tri2);
	CHECK(modelManager.isValid(tri1));
	CHECK(modelManager.getModelRadius(tri1) > 0.0f);
}

TEST_CASE("ModelManager handles degenerate points safely", "[integration][model_manager]")
{
	ModelManager modelManager;

	Vector3 p1 = { 0.0f, 0.0f, 0.0f };
	Vector3 p2 = { 0.0f, 0.0f, 0.0f };
	Vector3 p3 = { 0.0f, 0.0f, 0.0f };

	t_model_id tri = modelManager.createTriangle(p1, p2, p3);
	CHECK(modelManager.isValid(tri));
}
