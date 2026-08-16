#include "catch2/catch_amalgamated.hpp"

#include "classes/frustum.hpp"
#include "raylib.h"
#include "raymath.h"

TEST_CASE("Frustum extracts 6 planes correctly and tests spheres", "[rendering][culling]")
{
	Camera3D camera{};
	camera.position = Vector3{0.0f, 0.0f, 10.0f};
	camera.target = Vector3{0.0f, 0.0f, 0.0f};
	camera.up = Vector3{0.0f, 1.0f, 0.0f};
	camera.fovy = 60.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	Matrix view = GetCameraMatrix(camera);
	Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, 4.0f / 3.0f, 0.01f, 1000.0f);
	Matrix viewProj = MatrixMultiply(view, proj);

	Frustum frustum = Frustum::fromViewProjection(viewProj);

	SECTION("Center target sphere is visible")
	{
		// Center target at (0, 0, 0) is well inside the view frustum
		CHECK(frustum.isSphereInside(Vector3{0.0f, 0.0f, 0.0f}, 1.0f));
	}

	SECTION("Entity directly behind the camera is culled")
	{
		// Camera is at z = 10 looking towards z = 0 (into -Z). Behind camera is +Z (e.g. z = 20)
		CHECK_FALSE(frustum.isSphereInside(Vector3{0.0f, 0.0f, 25.0f}, 2.0f));
	}

	SECTION("Entity far off to the side is culled")
	{
		// Far off in +X at distance 500
		CHECK_FALSE(frustum.isSphereInside(Vector3{500.0f, 0.0f, 0.0f}, 2.0f));
	}

	SECTION("Conservative intersection: sphere straddling the near plane or edge is NOT culled")
	{
		// Camera at z=10, looking at z=0. Position just behind camera at z=11 with radius 3.0 reaches z=8 (inside near frustum)
		CHECK(frustum.isSphereInside(Vector3{0.0f, 0.0f, 11.0f}, 3.0f));
	}

	SECTION("Entity beyond the far plane is culled")
	{
		// Far plane is at ~1000 from camera (z = -990). At z = -1500, it should be culled.
		CHECK_FALSE(frustum.isSphereInside(Vector3{0.0f, 0.0f, -1500.0f}, 5.0f));
	}
}
