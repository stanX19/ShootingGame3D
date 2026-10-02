#include "draw_utils.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

bool draw_utils::isInFrontOfCamera(const Vector3 &entityPos, const Camera3D &camera)
{
	const Vector3 cameraToEntity = entityPos - camera.position;
	const Vector3 forward = camera.target - camera.position;
	return Vector3DotProduct(cameraToEntity, forward) > 0;
}

struct RibbonPoint {
	Vector3 left;
	Vector3 right;
	Color color;
};

static void emitRoundedTipFan(
	const Vector3 &center,
	const Vector3 &prevPStart,
	const Vector3 &headSide,
	const Vector3 &headForward,
	float radius,
	Color color
) {
	constexpr int CAP_SEGMENTS = 6;
	Vector3 prevP = prevPStart;
	for (int s = 1; s <= CAP_SEGMENTS; ++s) {
		const float angle = (s * PI) / CAP_SEGMENTS;
		const Vector3 curP = center + (headSide * std::cos(angle) + headForward * std::sin(angle)) * radius;
		rlColor4ub(color.r, color.g, color.b, color.a);
		rlVertex3f(center.x, center.y, center.z);
		rlColor4ub(color.r, color.g, color.b, color.a);
		rlVertex3f(prevP.x, prevP.y, prevP.z);
		rlColor4ub(color.r, color.g, color.b, color.a);
		rlVertex3f(curP.x, curP.y, curP.z);
		prevP = curP;
	}
}

static void emitRibbonQuads(const RibbonPoint *pts, std::size_t n) {
	for (std::size_t i = 0; i < n - 1; ++i) {
		rlColor4ub(pts[i].color.r, pts[i].color.g, pts[i].color.b, pts[i].color.a);
		rlVertex3f(pts[i].left.x, pts[i].left.y, pts[i].left.z);
		rlColor4ub(pts[i].color.r, pts[i].color.g, pts[i].color.b, pts[i].color.a);
		rlVertex3f(pts[i].right.x, pts[i].right.y, pts[i].right.z);
		rlColor4ub(pts[i + 1].color.r, pts[i + 1].color.g, pts[i + 1].color.b, pts[i + 1].color.a);
		rlVertex3f(pts[i + 1].right.x, pts[i + 1].right.y, pts[i + 1].right.z);

		rlColor4ub(pts[i].color.r, pts[i].color.g, pts[i].color.b, pts[i].color.a);
		rlVertex3f(pts[i].left.x, pts[i].left.y, pts[i].left.z);
		rlColor4ub(pts[i + 1].color.r, pts[i + 1].color.g, pts[i + 1].color.b, pts[i + 1].color.a);
		rlVertex3f(pts[i + 1].right.x, pts[i + 1].right.y, pts[i + 1].right.z);
		rlColor4ub(pts[i + 1].color.r, pts[i + 1].color.g, pts[i + 1].color.b, pts[i + 1].color.a);
		rlVertex3f(pts[i + 1].left.x, pts[i + 1].left.y, pts[i + 1].left.z);
	}
}

static void emitRibbonTriangles(
	const trail::Node *nodes,
	std::size_t count,
	float startWidth,
	float endWidth,
	Color baseColor,
	const Camera3D &camera
) {
	if (count < 2) {
		return;
	}
	const std::size_t n = std::min(count, effect::HasMultiTrail::MAX_NODES);
	RibbonPoint pts[effect::HasMultiTrail::MAX_NODES];

	Vector3 headForward = Vector3{0.0f, 0.0f, 1.0f};
	Vector3 headSide = Vector3{1.0f, 0.0f, 0.0f};

	for (std::size_t i = 0; i < n; ++i) {
		const auto &node = nodes[i];
		Vector3 forward;
		if (i == 0) {
			forward = Vector3Subtract(node.pos, nodes[1].pos);
		} else if (i == n - 1) {
			forward = Vector3Subtract(nodes[n - 2].pos, node.pos);
		} else {
			forward = Vector3Subtract(nodes[i - 1].pos, nodes[i + 1].pos);
		}

		const float fwdLenSq = Vector3LengthSqr(forward);
		Vector3 toCam = Vector3Subtract(camera.position, node.pos);
		Vector3 side;
		if (fwdLenSq > 1e-6f) {
			side = Vector3CrossProduct(forward, toCam);
			if (Vector3LengthSqr(side) < 1e-4f) {
				side = Vector3CrossProduct(forward, camera.up);
			}
		}
		if (Vector3LengthSqr(side) < 1e-6f) {
			side = Vector3CrossProduct(Vector3Subtract(camera.target, camera.position), camera.up);
		}
		const float sideLen = Vector3Length(side);
		side = (sideLen > 1e-5f) ? Vector3Scale(side, 1.0f / sideLen) : Vector3{1.0f, 0.0f, 0.0f};

		if (i == 0) {
			const float fLen = std::sqrt(fwdLenSq);
			headForward = (fLen > 1e-5f) ? Vector3Scale(forward, 1.0f / fLen) : Vector3{0.0f, 0.0f, 1.0f};
			headSide = side;
		}

		const float t = static_cast<float>(i) / static_cast<float>(n - 1);
		const float halfWidth = (startWidth * (1.0f - t) + endWidth * t) * 0.5f;
		const Vector3 offset = Vector3Scale(side, halfWidth);

		const unsigned char alpha = static_cast<unsigned char>(std::clamp(baseColor.a * node.alpha, 0.0f, 255.0f));
		pts[i].color = Color{baseColor.r, baseColor.g, baseColor.b, alpha};
		pts[i].left = Vector3Add(node.pos, offset);
		pts[i].right = Vector3Subtract(node.pos, offset);
	}

	if (startWidth > 0.001f && n >= 2) {
		emitRoundedTipFan(nodes[0].pos, pts[0].left, headSide, headForward, startWidth * 0.5f, pts[0].color);
	}

	emitRibbonQuads(pts, n);
}

void draw_utils::drawSimpleTrail(const effect::HasSimpleTrail &trail, const Camera3D &camera) {
	emitRibbonTriangles(trail.nodes, trail.count, trail.width, trail.endWidth, trail.color, camera);
}

void draw_utils::drawMultiTrailEmitter(
	const effect::HasMultiTrail::Emitter &emitter,
	const effect::HasMultiTrail &trail,
	const Camera3D &camera
) {
	emitRibbonTriangles(emitter.nodes, emitter.count, emitter.width, trail.endWidth, trail.color, camera);
}
