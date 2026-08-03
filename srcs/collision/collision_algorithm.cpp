#include "collision_algorithm.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
	constexpr float epsilon = 0.000001f;
	constexpr float twoPi = 6.283185307179586f;

	struct SweepContext
	{
		const CollisionModel &mesh;
		Matrix baseTransform;
		Vector3 meshStart;
		Vector3 meshDisplacement;
		Vector3 sphereStart;
		Vector3 sphereDisplacement;
		float sphereRadius;
		float startDt;
		float endDt;
		std::optional<CollisionHit> earliestHit;
	};

	Vector3 addVector(const Vector3 &left, const Vector3 &right)
	{
		return Vector3{left.x + right.x, left.y + right.y, left.z + right.z};
	}

	Vector3 subtractVector(const Vector3 &left, const Vector3 &right)
	{
		return Vector3{left.x - right.x, left.y - right.y, left.z - right.z};
	}

	Vector3 scaleVector(const Vector3 &value, float scale)
	{
		return Vector3{value.x * scale, value.y * scale, value.z * scale};
	}

	float getAxisValue(const Vector3 &value, int axis)
	{
		if (axis == 0)
			return value.x;
		if (axis == 1)
			return value.y;
		return value.z;
	}

	BoundingBox expandBounds(const BoundingBox &bounds, float amount)
	{
		const Vector3 expansion{amount, amount, amount};
		return BoundingBox{
			subtractVector(bounds.min, expansion),
			addVector(bounds.max, expansion)
		};
	}

	BoundingBox transformBounds(const BoundingBox &bounds, const Matrix &transform)
	{
		const std::array<Vector3, 8> corners{
			Vector3{bounds.min.x, bounds.min.y, bounds.min.z},
			Vector3{bounds.max.x, bounds.min.y, bounds.min.z},
			Vector3{bounds.min.x, bounds.max.y, bounds.min.z},
			Vector3{bounds.max.x, bounds.max.y, bounds.min.z},
			Vector3{bounds.min.x, bounds.min.y, bounds.max.z},
			Vector3{bounds.max.x, bounds.min.y, bounds.max.z},
			Vector3{bounds.min.x, bounds.max.y, bounds.max.z},
			Vector3{bounds.max.x, bounds.max.y, bounds.max.z}
		};
		const Vector3 firstCorner = Vector3Transform(corners[0], transform);
		BoundingBox result{firstCorner, firstCorner};

		for (std::size_t index = 1; index < corners.size(); ++index)
		{
			const Vector3 transformedCorner = Vector3Transform(corners[index], transform);
			result.min = Vector3Min(result.min, transformedCorner);
			result.max = Vector3Max(result.max, transformedCorner);
		}
		return result;
	}

	bool sweptPointIntersectsBounds(
		const Vector3 &start,
		const Vector3 &displacement,
		const BoundingBox &bounds,
		float startDt,
		float endDt
	)
	{
		float intervalStart = startDt;
		float intervalEnd = endDt;

		for (int axis = 0; axis < 3; ++axis)
		{
			const float startValue = getAxisValue(start, axis);
			const float displacementValue = getAxisValue(displacement, axis);
			const float minValue = getAxisValue(bounds.min, axis);
			const float maxValue = getAxisValue(bounds.max, axis);

			if (std::fabs(displacementValue) < epsilon)
			{
				if (startValue < minValue || startValue > maxValue)
					return false;
				continue;
			}

			float axisStart = (minValue - startValue) / displacementValue;
			float axisEnd = (maxValue - startValue) / displacementValue;
			if (axisStart > axisEnd)
				std::swap(axisStart, axisEnd);

			intervalStart = std::max(intervalStart, axisStart);
			intervalEnd = std::min(intervalEnd, axisEnd);
			if (intervalStart > intervalEnd)
				return false;
		}

		return true;
	}

	Matrix getBaseTransform(
		const CollisionModel &mesh,
		const CollisionMeshInstance &instance
	)
	{
		const Matrix scale = MatrixScale(
			instance.scale.x,
			instance.scale.y,
			instance.scale.z
		);
		const Matrix rotation = QuaternionToMatrix(instance.rotation);
		return MatrixMultiply(
			mesh.staticTransform,
			MatrixMultiply(scale, rotation)
		);
	}

	CollisionTriangle transformTriangle(
		const CollisionTriangle &triangle,
		const Matrix &transform
	)
	{
		return CollisionTriangle{
			Vector3Transform(triangle.a, transform),
			Vector3Transform(triangle.b, transform),
			Vector3Transform(triangle.c, transform)
		};
	}

	Vector3 getMeshStart(const CollisionMeshInstance &instance)
	{
		return addVector(
			instance.previousPosition,
			Vector3RotateByQuaternion(instance.translation, instance.rotation)
		);
	}

	Vector3 closestPointOnSegment(
		const Vector3 &point,
		const Vector3 &start,
		const Vector3 &end
	)
	{
		const Vector3 edge = subtractVector(end, start);
		const float edgeLengthSquared = Vector3LengthSqr(edge);
		if (edgeLengthSquared < epsilon)
			return start;

		const float ratio = std::clamp(
			Vector3DotProduct(subtractVector(point, start), edge) / edgeLengthSquared,
			0.0f,
			1.0f
		);
		return addVector(start, scaleVector(edge, ratio));
	}

	Vector3 closestPointOnTriangle(
		const Vector3 &point,
		const CollisionTriangle &triangle
	)
	{
		const Vector3 ab = subtractVector(triangle.b, triangle.a);
		const Vector3 ac = subtractVector(triangle.c, triangle.a);
		const Vector3 ap = subtractVector(point, triangle.a);
		const float d1 = Vector3DotProduct(ab, ap);
		const float d2 = Vector3DotProduct(ac, ap);
		if (d1 <= 0.0f && d2 <= 0.0f)
			return triangle.a;

		const Vector3 bp = subtractVector(point, triangle.b);
		const float d3 = Vector3DotProduct(ab, bp);
		const float d4 = Vector3DotProduct(ac, bp);
		if (d3 >= 0.0f && d4 <= d3)
			return triangle.b;

		const float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
		{
			const float ratio = d1 / (d1 - d3);
			return addVector(triangle.a, scaleVector(ab, ratio));
		}

		const Vector3 cp = subtractVector(point, triangle.c);
		const float d5 = Vector3DotProduct(ab, cp);
		const float d6 = Vector3DotProduct(ac, cp);
		if (d6 >= 0.0f && d5 <= d6)
			return triangle.c;

		const float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
		{
			const float ratio = d2 / (d2 - d6);
			return addVector(triangle.a, scaleVector(ac, ratio));
		}

		const float va = d3 * d6 - d5 * d4;
		if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
		{
			const Vector3 edge = subtractVector(triangle.c, triangle.b);
			const float ratio = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			return addVector(triangle.b, scaleVector(edge, ratio));
		}

		const float denominator = 1.0f / (va + vb + vc);
		const float barycentricB = vb * denominator;
		const float barycentricC = vc * denominator;
		return addVector(
			triangle.a,
			addVector(
				scaleVector(ab, barycentricB),
				scaleVector(ac, barycentricC)
			)
		);
	}

	float getTriangleSolidAngle(
		const Vector3 &point,
		const CollisionTriangle &triangle
	)
	{
		const Vector3 a = subtractVector(triangle.a, point);
		const Vector3 b = subtractVector(triangle.b, point);
		const Vector3 c = subtractVector(triangle.c, point);
		const float lengthA = Vector3Length(a);
		const float lengthB = Vector3Length(b);
		const float lengthC = Vector3Length(c);
		if (lengthA < epsilon || lengthB < epsilon || lengthC < epsilon)
			return 0.0f;

		const float numerator = Vector3DotProduct(
			a,
			Vector3CrossProduct(b, c)
		);
		const float denominator =
			lengthA * lengthB * lengthC
			+ Vector3DotProduct(a, b) * lengthC
			+ Vector3DotProduct(b, c) * lengthA
			+ Vector3DotProduct(c, a) * lengthB;

		return 2.0f * std::atan2(numerator, denominator);
	}

	bool isPointInsideMesh(const SweepContext &context)
	{
		const BoundingBox bounds = transformBounds(
			context.mesh.bounds,
			context.baseTransform
		);
		if (context.sphereStart.x < bounds.min.x || context.sphereStart.x > bounds.max.x ||
			context.sphereStart.y < bounds.min.y || context.sphereStart.y > bounds.max.y ||
			context.sphereStart.z < bounds.min.z || context.sphereStart.z > bounds.max.z)
			return false;

		float solidAngle = 0.0f;
		for (const CollisionTriangle &localTriangle : context.mesh.triangles)
		{
			const CollisionTriangle triangle = transformTriangle(
				localTriangle,
				context.baseTransform
			);
			solidAngle += getTriangleSolidAngle(context.sphereStart, triangle);
		}

		return std::fabs(solidAngle) > twoPi;
	}

	bool pointInsideTriangle(
		const Vector3 &point,
		const CollisionTriangle &triangle,
		const Vector3 &normal
	)
	{
		const Vector3 edgeAB = subtractVector(triangle.b, triangle.a);
		const Vector3 edgeBC = subtractVector(triangle.c, triangle.b);
		const Vector3 edgeCA = subtractVector(triangle.a, triangle.c);
		const Vector3 pointA = subtractVector(point, triangle.a);
		const Vector3 pointB = subtractVector(point, triangle.b);
		const Vector3 pointC = subtractVector(point, triangle.c);

		return Vector3DotProduct(Vector3CrossProduct(edgeAB, pointA), normal) >= -epsilon &&
			Vector3DotProduct(Vector3CrossProduct(edgeBC, pointB), normal) >= -epsilon &&
			Vector3DotProduct(Vector3CrossProduct(edgeCA, pointC), normal) >= -epsilon;
	}

	std::optional<float> getEarliestQuadraticRoot(
		float a,
		float b,
		float c,
		float startDt,
		float endDt
	)
	{
		if (std::fabs(a) < epsilon)
		{
			if (std::fabs(b) < epsilon)
				return std::nullopt;
			const float root = -c / b;
			if (root >= startDt - epsilon && root <= endDt + epsilon)
				return std::clamp(root, startDt, endDt);
			return std::nullopt;
		}

		const float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			return std::nullopt;

		const float sqrtDiscriminant = std::sqrt(discriminant);
		const float denominator = 2.0f * a;
		const float roots[2]{
			(-b - sqrtDiscriminant) / denominator,
			(-b + sqrtDiscriminant) / denominator
		};
		float earliestRoot = std::numeric_limits<float>::infinity();
		for (float root : roots)
		{
			if (root >= startDt - epsilon && root <= endDt + epsilon)
				earliestRoot = std::min(earliestRoot, root);
		}

		if (!std::isfinite(earliestRoot))
			return std::nullopt;
		return std::clamp(earliestRoot, startDt, endDt);
	}

	Vector3 getSafeNormal(const Vector3 &value, const Vector3 &fallback)
	{
		if (Vector3LengthSqr(value) < epsilon)
			return fallback;
		return Vector3Normalize(value);
	}

	void considerHit(
		SweepContext &context,
		float collisionDt,
		const Vector3 &surfacePoint,
		const Vector3 &normal
	)
	{
		if (collisionDt < context.startDt - epsilon || collisionDt > context.endDt + epsilon)
			return;
		if (context.earliestHit && collisionDt >= context.earliestHit->collisionDt - epsilon)
			return;

		context.earliestHit = CollisionHit{
			std::clamp(collisionDt, context.startDt, context.endDt),
			addVector(
				addVector(surfacePoint, context.meshStart),
				scaleVector(context.meshDisplacement, collisionDt)
			),
			getSafeNormal(normal, Vector3{0.0f, 1.0f, 0.0f})
		};
	}

	void testMovingPointAgainstSphere(
		SweepContext &context,
		const Vector3 &center,
		const Vector3 &fallbackNormal
	)
	{
		const Vector3 offset = subtractVector(context.sphereStart, center);
		const float radiusSquared = context.sphereRadius * context.sphereRadius;
		const float a = Vector3LengthSqr(context.sphereDisplacement);
		const float b = 2.0f * Vector3DotProduct(offset, context.sphereDisplacement);
		const float c = Vector3LengthSqr(offset) - radiusSquared;
		const std::optional<float> root = getEarliestQuadraticRoot(
			a,
			b,
			c,
			context.startDt,
			context.endDt
		);
		if (!root)
			return;

		const Vector3 sphereCenter = addVector(
			context.sphereStart,
			scaleVector(context.sphereDisplacement, *root)
		);
		considerHit(
			context,
			*root,
			center,
			getSafeNormal(subtractVector(sphereCenter, center), fallbackNormal)
		);
	}

	void testMovingPointAgainstEdge(
		SweepContext &context,
		const Vector3 &edgeStart,
		const Vector3 &edgeEnd,
		const Vector3 &fallbackNormal
	)
	{
		const Vector3 edge = subtractVector(edgeEnd, edgeStart);
		const float edgeLengthSquared = Vector3LengthSqr(edge);
		if (edgeLengthSquared < epsilon)
		{
			testMovingPointAgainstSphere(context, edgeStart, fallbackNormal);
			return;
		}

		const Vector3 pointOffset = subtractVector(context.sphereStart, edgeStart);
		const float edgeVelocityRatio =
			Vector3DotProduct(context.sphereDisplacement, edge) / edgeLengthSquared;
		const float edgeOffsetRatio =
			Vector3DotProduct(pointOffset, edge) / edgeLengthSquared;
		const Vector3 perpendicularVelocity = subtractVector(
			context.sphereDisplacement,
			scaleVector(edge, edgeVelocityRatio)
		);
		const Vector3 perpendicularOffset = subtractVector(
			pointOffset,
			scaleVector(edge, edgeOffsetRatio)
		);
		const float radiusSquared = context.sphereRadius * context.sphereRadius;
		const float a = Vector3LengthSqr(perpendicularVelocity);
		const float b = 2.0f * Vector3DotProduct(perpendicularOffset, perpendicularVelocity);
		const float c = Vector3LengthSqr(perpendicularOffset) - radiusSquared;
		const std::optional<float> root = getEarliestQuadraticRoot(
			a,
			b,
			c,
			context.startDt,
			context.endDt
		);
		if (!root)
			return;

		const Vector3 sphereCenter = addVector(
			context.sphereStart,
			scaleVector(context.sphereDisplacement, *root)
		);
		const Vector3 closestPoint = closestPointOnSegment(sphereCenter, edgeStart, edgeEnd);
		const float edgeRatio = Vector3DotProduct(
			subtractVector(closestPoint, edgeStart),
			edge
		) / edgeLengthSquared;
		if (edgeRatio < -epsilon || edgeRatio > 1.0f + epsilon)
			return;

		considerHit(
			context,
			*root,
			closestPoint,
			subtractVector(sphereCenter, closestPoint)
		);
	}

	void testTriangle(
		SweepContext &context,
		const CollisionTriangle &localTriangle
	)
	{
		const CollisionTriangle triangle = transformTriangle(
			localTriangle,
			context.baseTransform
		);
		const Vector3 normal = Vector3Normalize(Vector3CrossProduct(
			subtractVector(triangle.b, triangle.a),
			subtractVector(triangle.c, triangle.a)
		));
		if (Vector3LengthSqr(normal) < epsilon)
			return;

		const Vector3 initialClosestPoint = closestPointOnTriangle(context.sphereStart, triangle);
		if (Vector3DistanceSqr(context.sphereStart, initialClosestPoint) <=
			context.sphereRadius * context.sphereRadius + epsilon)
		{
			considerHit(
				context,
				context.startDt,
				initialClosestPoint,
				subtractVector(context.sphereStart, initialClosestPoint)
			);
			return;
		}

		const float initialPlaneDistance = Vector3DotProduct(
			subtractVector(context.sphereStart, triangle.a),
			normal
		);
		const float planeDisplacement = Vector3DotProduct(context.sphereDisplacement, normal);
		for (float side : {-1.0f, 1.0f})
		{
			if (std::fabs(planeDisplacement) < epsilon)
				continue;
			const float targetDistance = side * context.sphereRadius;
			const float collisionDt =
				(targetDistance - initialPlaneDistance) / planeDisplacement;
			if (collisionDt < context.startDt - epsilon || collisionDt > context.endDt + epsilon)
				continue;

			const Vector3 sphereCenter = addVector(
				context.sphereStart,
				scaleVector(context.sphereDisplacement, collisionDt)
			);
			const Vector3 surfacePoint = subtractVector(
				sphereCenter,
				scaleVector(normal, targetDistance)
			);
			if (pointInsideTriangle(surfacePoint, triangle, normal))
				considerHit(context, collisionDt, surfacePoint, scaleVector(normal, side));
		}

		testMovingPointAgainstEdge(context, triangle.a, triangle.b, normal);
		testMovingPointAgainstEdge(context, triangle.b, triangle.c, normal);
		testMovingPointAgainstEdge(context, triangle.c, triangle.a, normal);
		testMovingPointAgainstSphere(context, triangle.a, normal);
		testMovingPointAgainstSphere(context, triangle.b, normal);
		testMovingPointAgainstSphere(context, triangle.c, normal);
	}

	void visitBvhNode(SweepContext &context, int nodeIndex)
	{
		if (nodeIndex < 0)
			return;

		const CollisionBvhNode &node = context.mesh.bvh[static_cast<std::size_t>(nodeIndex)];
		const float nodeEndDt = context.earliestHit
			? context.earliestHit->collisionDt
			: context.endDt;
		const BoundingBox worldBounds = expandBounds(
			transformBounds(node.bounds, context.baseTransform),
			context.sphereRadius
		);
		if (!sweptPointIntersectsBounds(
			context.sphereStart,
			context.sphereDisplacement,
			worldBounds,
			context.startDt,
			nodeEndDt
		))
			return;

		if (node.leftChild < 0 && node.rightChild < 0)
		{
			for (std::size_t offset = 0; offset < node.triangleCount; ++offset)
				testTriangle(context, context.mesh.triangles[node.firstTriangle + offset]);
			return;
		}

		visitBvhNode(context, node.leftChild);
		visitBvhNode(context, node.rightChild);
	}
}

std::optional<CollisionHit> sweepSphereAgainstMesh(
	const CollisionModel &mesh,
	const CollisionMeshInstance &meshInstance,
	const Vector3 &spherePreviousPosition,
	const Vector3 &sphereDisplacement,
	float sphereRadius,
	const CollisionInterval &broadPhaseInterval
)
{
	if (!willCollide(broadPhaseInterval, 1.0f))
		return std::nullopt;
	if (mesh.bvh.empty() || sphereRadius < 0.0f)
		return std::nullopt;

	const float startDt = std::max(broadPhaseInterval.collisionStartDt, 0.0f);
	const float endDt = std::min(broadPhaseInterval.collisionEndDt, 1.0f);
	if (startDt > endDt)
		return std::nullopt;

	const Vector3 meshStart = getMeshStart(meshInstance);
	const Vector3 meshDisplacement = subtractVector(
		meshInstance.currentPosition,
		meshInstance.previousPosition
	);
	SweepContext context{
		mesh,
		getBaseTransform(mesh, meshInstance),
		meshStart,
		meshDisplacement,
		subtractVector(spherePreviousPosition, meshStart),
		subtractVector(sphereDisplacement, meshDisplacement),
		sphereRadius,
		startDt,
		endDt,
		std::nullopt
	};

	if (isPointInsideMesh(context))
	{
		// Containment in a closed mesh is an initial collision.
		// A hole remains outside because its winding number is zero.
		considerHit(
			context,
			context.startDt,
			context.sphereStart,
			Vector3{0.0f, 1.0f, 0.0f}
		);
		return context.earliestHit;
	}

	visitBvhNode(context, 0);
	return context.earliestHit;
}
