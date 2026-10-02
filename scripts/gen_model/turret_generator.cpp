#include "turret_generator.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {
	using namespace gen_model::gen_types;

	struct UvRect {
		float u0 = 0.0f;
		float v0 = 0.0f;
		float u1 = 1.0f;
		float v1 = 1.0f;
	};

	constexpr UvRect UV_BASE{0.05f, 0.05f, 0.45f, 0.45f};
	constexpr UvRect UV_BODY{0.55f, 0.05f, 0.95f, 0.45f};
	constexpr UvRect UV_BARREL{0.05f, 0.55f, 0.45f, 0.72f};
	constexpr UvRect UV_DETAIL{0.55f, 0.55f, 0.95f, 0.95f};
	constexpr UvRect UV_BORE{0.05f, 0.82f, 0.20f, 0.95f};
	constexpr UvRect UV_CONDUIT{0.24f, 0.78f, 0.46f, 0.95f};

	float vectorLength(Point3 p) {
		return std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
	}

	class MeshBuilder {
	public:
		MeshData mesh;

		int addVertex(Point3 pos, Point2 uv, Point3 norm) {
			const int index = static_cast<int>(mesh.positions.size());
			mesh.positions.push_back(pos);
			mesh.texcoords.push_back(uv);
			mesh.normals.push_back(normalize(norm));
			return index;
		}

		void addTriangle(int i0, int i1, int i2) {
			mesh.triangles.push_back(Triangle{
				std::array<int, 3>{i0, i1, i2},
				std::array<int, 3>{i0, i1, i2},
				std::array<int, 3>{i0, i1, i2}
			});
		}

		void addQuad(Point3 p0, Point3 p1, Point3 p2, Point3 p3, UvRect uv, Point3 normal) {
			const int i0 = addVertex(p0, Point2{uv.u0, uv.v0}, normal);
			const int i1 = addVertex(p1, Point2{uv.u1, uv.v0}, normal);
			const int i2 = addVertex(p2, Point2{uv.u1, uv.v1}, normal);
			const int i3 = addVertex(p3, Point2{uv.u0, uv.v1}, normal);

			addTriangle(i0, i1, i2);
			addTriangle(i0, i2, i3);
		}

		void addBox(Point3 center, Point3 halfSize, UvRect uv) {
			const float minX = center.x - halfSize.x;
			const float maxX = center.x + halfSize.x;
			const float minY = center.y - halfSize.y;
			const float maxY = center.y + halfSize.y;
			const float minZ = center.z - halfSize.z;
			const float maxZ = center.z + halfSize.z;

			// Front (+Z)
			addQuad({minX, minY, maxZ}, {maxX, minY, maxZ}, {maxX, maxY, maxZ}, {minX, maxY, maxZ}, uv, {0, 0, 1});
			// Back (-Z)
			addQuad({maxX, minY, minZ}, {minX, minY, minZ}, {minX, maxY, minZ}, {maxX, maxY, minZ}, uv, {0, 0, -1});
			// Top (+Y)
			addQuad({minX, maxY, maxZ}, {maxX, maxY, maxZ}, {maxX, maxY, minZ}, {minX, maxY, minZ}, uv, {0, 1, 0});
			// Bottom (-Y)
			addQuad({minX, minY, minZ}, {maxX, minY, minZ}, {maxX, minY, maxZ}, {minX, minY, maxZ}, uv, {0, -1, 0});
			// Right (+X)
			addQuad({maxX, minY, maxZ}, {maxX, minY, minZ}, {maxX, maxY, minZ}, {maxX, maxY, maxZ}, uv, {1, 0, 0});
			// Left (-X)
			addQuad({minX, minY, minZ}, {minX, minY, maxZ}, {minX, maxY, maxZ}, {minX, maxY, minZ}, uv, {-1, 0, 0});
		}

		void addCylinder(Point3 bottom, Point3 top, float rBottom, float rTop, int segments, UvRect uv, bool capBottom = true, bool capTop = true) {
			if (segments < 3)
				return;
			const Point3 axis = top - bottom;
			const float height = vectorLength(axis);
			if (height < 1e-6f)
				return;
			const Point3 dir = normalize(axis);
			const Point3 helper = (std::abs(dir.y) < 0.9f) ? Point3{0.0f, 1.0f, 0.0f} : Point3{1.0f, 0.0f, 0.0f};
			const Point3 uAxis = normalize(cross(dir, helper));
			const Point3 vAxis = normalize(cross(dir, uAxis));

			std::vector<int> bIndices;
			std::vector<int> tIndices;
			bIndices.reserve(segments);
			tIndices.reserve(segments);

			for (int i = 0; i < segments; ++i) {
				const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
				const float cosA = std::cos(angle);
				const float sinA = std::sin(angle);

				const Point3 radial = uAxis * cosA + vAxis * sinA;
				const Point3 pBottom = bottom + radial * rBottom;
				const Point3 pTop = top + radial * rTop;

				const float u = uv.u0 + (uv.u1 - uv.u0) * (static_cast<float>(i) / static_cast<float>(segments));
				bIndices.push_back(addVertex(pBottom, Point2{u, uv.v0}, radial));
				tIndices.push_back(addVertex(pTop, Point2{u, uv.v1}, radial));
			}

			for (int i = 0; i < segments; ++i) {
				const int next = (i + 1) % segments;
				addTriangle(bIndices[i], bIndices[next], tIndices[next]);
				addTriangle(bIndices[i], tIndices[next], tIndices[i]);
			}

			if (capBottom) {
				const int centerIdx = addVertex(bottom, Point2{(uv.u0 + uv.u1) * 0.5f, uv.v0}, dir * -1.0f);
				for (int i = 0; i < segments; ++i) {
					const int next = (i + 1) % segments;
					addTriangle(centerIdx, bIndices[next], bIndices[i]);
				}
			}

			if (capTop) {
				const int centerIdx = addVertex(top, Point2{(uv.u0 + uv.u1) * 0.5f, uv.v1}, dir);
				for (int i = 0; i < segments; ++i) {
					const int next = (i + 1) % segments;
					addTriangle(centerIdx, tIndices[i], tIndices[next]);
				}
			}
		}

		void addMuzzleBore(Point3 muzzleEnd, Point3 dir, float rOuter, float rBore, float boreDepth, int segments, UvRect rimUv, UvRect boreUv) {
			if (segments < 3 || rBore <= 0.0f || rBore >= rOuter || boreDepth <= 0.0f)
				return;
			const Point3 axis = normalize(dir);
			const Point3 helper = (std::abs(axis.y) < 0.9f) ? Point3{0.0f, 1.0f, 0.0f} : Point3{1.0f, 0.0f, 0.0f};
			const Point3 uAxis = normalize(cross(axis, helper));
			const Point3 vAxis = normalize(cross(axis, uAxis));

			std::vector<int> rimOuterIndices;
			std::vector<int> rimInnerIndices;
			std::vector<int> boreWallFrontIndices;
			std::vector<int> boreWallBackIndices;
			rimOuterIndices.reserve(segments);
			rimInnerIndices.reserve(segments);
			boreWallFrontIndices.reserve(segments);
			boreWallBackIndices.reserve(segments);

			const Point3 boreBottom = muzzleEnd - axis * boreDepth;

			for (int i = 0; i < segments; ++i) {
				const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
				const float cosA = std::cos(angle);
				const float sinA = std::sin(angle);
				const Point3 radial = uAxis * cosA + vAxis * sinA;
				const float uFraction = static_cast<float>(i) / static_cast<float>(segments);

				// Annular crown rim (Z = muzzleEnd, normal = +axis)
				const Point3 pRimOuter = muzzleEnd + radial * rOuter;
				const Point3 pRimInner = muzzleEnd + radial * rBore;
				const float rimU = rimUv.u0 + (rimUv.u1 - rimUv.u0) * uFraction;
				rimOuterIndices.push_back(addVertex(pRimOuter, Point2{rimU, rimUv.v1}, axis));
				rimInnerIndices.push_back(addVertex(pRimInner, Point2{rimU, rimUv.v0}, axis));

				// Interior bore tube (normal = -radial pointing inward)
				const Point3 pBoreFront = muzzleEnd + radial * rBore;
				const Point3 pBoreBack = boreBottom + radial * rBore;
				const float boreU = boreUv.u0 + (boreUv.u1 - boreUv.u0) * uFraction;
				boreWallFrontIndices.push_back(addVertex(pBoreFront, Point2{boreU, boreUv.v0}, radial * -1.0f));
				boreWallBackIndices.push_back(addVertex(pBoreBack, Point2{boreU, boreUv.v1}, radial * -1.0f));
			}

			// Triangulate annular crown rim
			for (int i = 0; i < segments; ++i) {
				const int next = (i + 1) % segments;
				addTriangle(rimOuterIndices[i], rimOuterIndices[next], rimInnerIndices[next]);
				addTriangle(rimOuterIndices[i], rimInnerIndices[next], rimInnerIndices[i]);
			}

			// Triangulate interior bore wall (inward winding)
			for (int i = 0; i < segments; ++i) {
				const int next = (i + 1) % segments;
				addTriangle(boreWallBackIndices[i], boreWallFrontIndices[next], boreWallBackIndices[next]);
				addTriangle(boreWallBackIndices[i], boreWallFrontIndices[i], boreWallFrontIndices[next]);
			}

			// Triangulate bore bottom cap (normal = +axis)
			const int centerIdx = addVertex(boreBottom, Point2{(boreUv.u0 + boreUv.u1) * 0.5f, boreUv.v1}, axis);
			for (int i = 0; i < segments; ++i) {
				const int next = (i + 1) % segments;
				addTriangle(centerIdx, boreWallBackIndices[i], boreWallBackIndices[next]);
			}
		}

		void addSphere(Point3 center, float radius, int rings, int sectors, UvRect uv) {
			if (rings < 3 || sectors < 3 || radius <= 0.0f)
				return;
			const int baseIndex = static_cast<int>(mesh.positions.size());
			for (int r = 0; r <= rings; ++r) {
				const float phi = std::numbers::pi_v<float> * static_cast<float>(r) / static_cast<float>(rings);
				const float sinPhi = std::sin(phi);
				const float cosPhi = std::cos(phi);
				const float v = uv.v0 + (uv.v1 - uv.v0) * (static_cast<float>(r) / static_cast<float>(rings));
				for (int s = 0; s <= sectors; ++s) {
					const float theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(s) / static_cast<float>(sectors);
					const float sinTheta = std::sin(theta);
					const float cosTheta = std::cos(theta);
					const float u = uv.u0 + (uv.u1 - uv.u0) * (static_cast<float>(s) / static_cast<float>(sectors));
					const Point3 normal{sinPhi * sinTheta, cosPhi, sinPhi * cosTheta};
					const Point3 pos = center + normal * radius;
					addVertex(pos, Point2{u, v}, normal);
				}
			}
			for (int r = 0; r < rings; ++r) {
				for (int s = 0; s < sectors; ++s) {
					const int cur = baseIndex + r * (sectors + 1) + s;
					const int next = cur + (sectors + 1);
					if (r == 0) {
						addTriangle(cur, next, next + 1);
					} else if (r == rings - 1) {
						addTriangle(cur, next, cur + 1);
					} else {
						addTriangle(cur, next, cur + 1);
						addTriangle(cur + 1, next, next + 1);
					}
				}
			}
		}
	};

	void buildPivotBallWithDetails(MeshBuilder& builder, float sphereRadius) {
		// Rule 1 & 2 Contract: Main armor pivot sphere at origin (0, 0, 0), radius 1.0f
		builder.addSphere(Point3{0.0f, 0.0f, 0.0f}, sphereRadius, 10, 14, UV_BASE);

		const float gap = 0.055f;

		// Side Trunnion Armor Cheeks (Port & Starboard): Cradles the sphere at |X| >= sphereRadius + gap
		const float cheekW = sphereRadius * 0.16f;
		const float cheekH = sphereRadius * 0.42f;
		const float cheekL = sphereRadius * 0.72f;
		const float cheekX = sphereRadius + gap + cheekW * 0.5f;
		const Point3 cheekLeftCenter{-cheekX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 cheekRightCenter{cheekX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		builder.addBox(cheekLeftCenter, Point3{cheekW * 0.5f, cheekH * 0.5f, cheekL * 0.5f}, UV_BODY);
		builder.addBox(cheekRightCenter, Point3{cheekW * 0.5f, cheekH * 0.5f, cheekL * 0.5f}, UV_BODY);

		// Heavy circular trunnion pivot hub caps on outer faces of cheeks
		const float hubR = sphereRadius * 0.32f;
		const float hubInnerX = cheekX + cheekW * 0.5f + gap;
		const float hubOuterX = hubInnerX + sphereRadius * 0.09f;
		const Point3 hubLeftInner{-hubInnerX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 hubLeftOuter{-hubOuterX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 hubRightInner{hubInnerX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 hubRightOuter{hubOuterX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		builder.addCylinder(hubLeftInner, hubLeftOuter, hubR, hubR, 12, UV_DETAIL, false, true);
		builder.addCylinder(hubRightInner, hubRightOuter, hubR, hubR, 12, UV_DETAIL, false, true);

		// Glowing neon indicator hubs on trunnions
		const float neonInnerX = hubOuterX + gap;
		const float neonOuterX = neonInnerX + sphereRadius * 0.02f;
		const Point3 neonHubL1{-neonInnerX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 neonHubL2{-neonOuterX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 neonHubR1{neonInnerX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		const Point3 neonHubR2{neonOuterX, sphereRadius * 0.06f, -sphereRadius * 0.04f};
		builder.addCylinder(neonHubL1, neonHubL2, hubR * 0.55f, hubR * 0.55f, 10, UV_CONDUIT, false, true);
		builder.addCylinder(neonHubR1, neonHubR2, hubR * 0.55f, hubR * 0.55f, 10, UV_CONDUIT, false, true);

		// BackOfGun (-Z): Substantial rear breech & generator housing at Z <= -sphereRadius - gap
		const float breechW = sphereRadius * 1.12f;
		const float breechH = sphereRadius * 0.86f;
		const float breechL = sphereRadius * 0.80f;
		const float breechZ = -sphereRadius - gap - breechL * 0.5f;
		const Point3 breechCenter{0.0f, sphereRadius * 0.10f, breechZ};
		builder.addBox(breechCenter, Point3{breechW * 0.5f, breechH * 0.5f, breechL * 0.5f}, UV_BODY);

		// Dual rear battery/capacitor sponsons flanking breech
		const float sponsonW = sphereRadius * 0.22f;
		const float sponsonH = sphereRadius * 0.68f;
		const float sponsonL = sphereRadius * 0.74f;
		const float sponX = breechW * 0.5f + gap + sponsonW * 0.5f;
		const Point3 sponLeftCenter{-sponX, sphereRadius * 0.10f, breechZ};
		const Point3 sponRightCenter{sponX, sphereRadius * 0.10f, breechZ};
		builder.addBox(sponLeftCenter, Point3{sponsonW * 0.5f, sponsonH * 0.5f, sponsonL * 0.5f}, UV_DETAIL);
		builder.addBox(sponRightCenter, Point3{sponsonW * 0.5f, sponsonH * 0.5f, sponsonL * 0.5f}, UV_DETAIL);

		// Top radiator heat sink ribs with illuminated conduit channels on breech
		const float ribY = breechCenter.y + breechH * 0.5f + gap + sphereRadius * 0.05f;
		const Point3 ribCenter{0.0f, ribY, breechCenter.z};
		builder.addBox(ribCenter, Point3{breechW * 0.35f, sphereRadius * 0.05f, breechL * 0.38f}, UV_CONDUIT);

		// Twin rear exhaust dump nozzles
		const float exhaustR = sphereRadius * 0.17f;
		const float exhaustStartZ = breechCenter.z - breechL * 0.5f - gap;
		const float exhaustEndZ = exhaustStartZ - sphereRadius * 0.25f;
		const Point3 exLStart{-breechW * 0.26f, sphereRadius * 0.10f, exhaustStartZ};
		const Point3 exLEnd{-breechW * 0.26f, sphereRadius * 0.10f, exhaustEndZ};
		const Point3 exRStart{breechW * 0.26f, sphereRadius * 0.10f, exhaustStartZ};
		const Point3 exREnd{breechW * 0.26f, sphereRadius * 0.10f, exhaustEndZ};
		builder.addCylinder(exLEnd, exLStart, exhaustR, exhaustR, 10, UV_DETAIL, true, false);
		builder.addCylinder(exREnd, exRStart, exhaustR, exhaustR, 10, UV_DETAIL, true, false);
	}

	void buildBasicShooter(MeshBuilder& builder, const gen_model::turret::Settings& settings, gen_model::turret::GenerationReport& report) {
		const float bRad = settings.baseRadius;
		const float gap = 0.055f;

		buildPivotBallWithDetails(builder, bRad);

		// FrontOfGun (+Z): Chunky armored mantlet receiver cowl starting at Z = bRad + gap
		const float mantletW = bRad * 1.24f;
		const float mantletH = bRad * 0.98f;
		const float mantletL = bRad * 0.62f;
		const float mantletZ = bRad + gap + mantletL * 0.5f;
		const Point3 mantletCenter{0.0f, bRad * 0.06f, mantletZ};
		builder.addBox(mantletCenter, Point3{mantletW * 0.5f, mantletH * 0.5f, mantletL * 0.5f}, UV_BODY);

		// Heavy trunnion bushing collar starting at mantlet front face
		const float barrelR = settings.barrelRadius;
		const float barrelLen = settings.barrelLength;
		const float collarStartZ = mantletZ + mantletL * 0.5f + gap;
		const float collarLen = bRad * 0.28f;
		const float collarEndZ = collarStartZ + collarLen;
		builder.addCylinder(Point3{0.0f, 0.0f, collarStartZ}, Point3{0.0f, 0.0f, collarEndZ}, barrelR * 1.70f, barrelR * 1.55f, 14, UV_DETAIL, false, true);

		// Stepped shroud enclosing the base of the elongated barrel
		const float shroudLen = barrelLen * 0.28f;
		const float shroudStartZ = collarEndZ + gap;
		const float shroudEndZ = shroudStartZ + shroudLen;
		const Point3 shroudCenter{0.0f, 0.0f, shroudStartZ + shroudLen * 0.5f};
		builder.addBox(shroudCenter, Point3{barrelR * 1.35f, barrelR * 1.15f, shroudLen * 0.5f}, UV_BODY);

		// Longitudinal energy rails running along top & bottom of shroud
		const float railY = barrelR * 1.15f + gap + barrelR * 0.07f;
		const Point3 railTopCenter{0.0f, railY, shroudCenter.z};
		const Point3 railBottomCenter{0.0f, -railY, shroudCenter.z};
		builder.addBox(railTopCenter, Point3{barrelR * 0.26f, barrelR * 0.07f, shroudLen * 0.48f}, UV_CONDUIT);
		builder.addBox(railBottomCenter, Point3{barrelR * 0.26f, barrelR * 0.07f, shroudLen * 0.48f}, UV_CONDUIT);

		// Dual heavy hydraulic recoil pistons flanking the shroud
		const float pistonR = barrelR * 0.30f;
		const float pistonOffset = barrelR * 1.70f + gap + pistonR;
		const Point3 pLeftStart{-pistonOffset, 0.0f, collarStartZ};
		const Point3 pLeftEnd{-pistonOffset, 0.0f, shroudStartZ + shroudLen * 0.85f};
		const Point3 pRightStart{pistonOffset, 0.0f, collarStartZ};
		const Point3 pRightEnd{pistonOffset, 0.0f, shroudStartZ + shroudLen * 0.85f};
		builder.addCylinder(pLeftStart, pLeftEnd, pistonR, pistonR, 10, UV_DETAIL, true, true);
		builder.addCylinder(pRightStart, pRightEnd, pistonR, pistonR, 10, UV_DETAIL, true, true);

		// Sleek elongated main barrel tube (centered at X=0, Y=0)
		const float barrelCoreStart = shroudEndZ + gap;
		const float barrelEndZ = collarStartZ + barrelLen;
		const float brakeLen = barrelLen * 0.10f;
		const Point3 barrelCoreStartPt{0.0f, 0.0f, barrelCoreStart};
		const Point3 barrelCoreEndPt{0.0f, 0.0f, barrelEndZ - brakeLen - gap};
		builder.addCylinder(barrelCoreStartPt, barrelCoreEndPt, barrelR, barrelR, 14, UV_BARREL, false, false);

		// Cylindrical heavy muzzle brake with glowing emitter ring & prominent deep bore
		const Point3 brakeStartPt{0.0f, 0.0f, barrelEndZ - brakeLen};
		const Point3 brakeEndPt{0.0f, 0.0f, barrelEndZ};
		builder.addCylinder(brakeStartPt, brakeEndPt, barrelR * 1.40f, barrelR * 1.36f, 14, UV_DETAIL, false, false);

		// Illuminated conduit ring encircling muzzle brake (radius clearance >= gap + 0.01f)
		const Point3 emitterRingStart{0.0f, 0.0f, barrelEndZ - brakeLen * 0.70f};
		const Point3 emitterRingEnd{0.0f, 0.0f, barrelEndZ - brakeLen * 0.30f};
		const float emitterRingR = barrelR * 1.40f + gap + 0.01f;
		builder.addCylinder(emitterRingStart, emitterRingEnd, emitterRingR, emitterRingR, 14, UV_CONDUIT, false, false);

		builder.addMuzzleBore(brakeEndPt, Point3{0.0f, 0.0f, 1.0f}, barrelR * 1.36f, barrelR * 0.72f, barrelLen * 0.25f, 14, UV_DETAIL, UV_BORE);

		report.pivotPosition = Point3{0.0f, 0.0f, 0.0f};
		report.muzzlePosition = Point3{0.0f, 0.0f, barrelEndZ};
	}

	void buildHeavyMgRifle(MeshBuilder& builder, const gen_model::turret::Settings& settings, gen_model::turret::GenerationReport& report) {
		const float bRad = settings.baseRadius;
		const float gap = 0.055f;

		buildPivotBallWithDetails(builder, bRad);

		// FrontOfGun (+Z): Heavy precision rifle receiver block starting at Z = bRad + gap
		const float recW = bRad * 1.10f;
		const float recH = bRad * 0.92f;
		const float recL = bRad * 0.88f;
		const float recZ = bRad + gap + recL * 0.5f;
		const Point3 recCenter{0.0f, bRad * 0.08f, recZ};
		builder.addBox(recCenter, Point3{recW * 0.5f, recH * 0.5f, recL * 0.5f}, UV_BODY);

		// Port-side rotary ammunition drum
		const float drumRadius = bRad * 0.52f;
		const float drumThickness = bRad * 0.32f;
		const float chuteLen = bRad * 0.16f;
		const float drumX = -recW * 0.5f - gap - chuteLen - gap - drumThickness * 0.5f;
		const Point3 drumCenter{drumX, bRad * 0.04f, recCenter.z - recL * 0.10f};
		const Point3 drumA{drumCenter.x - drumThickness * 0.5f, drumCenter.y, drumCenter.z};
		const Point3 drumB{drumCenter.x + drumThickness * 0.5f, drumCenter.y, drumCenter.z};
		builder.addCylinder(drumA, drumB, drumRadius, drumRadius, 14, UV_DETAIL, true, true);

		// Ammo feed chute bridging drum to receiver
		const float chuteCenterX = -recW * 0.5f - gap - chuteLen * 0.5f;
		const Point3 chuteCenter{chuteCenterX, bRad * 0.04f, recCenter.z - recL * 0.05f};
		builder.addBox(chuteCenter, Point3{(chuteLen - 2.0f * gap) * 0.5f, bRad * 0.18f, bRad * 0.22f}, UV_DETAIL);

		// Starboard auxiliary power cell
		const float cellX = recW * 0.5f + gap + bRad * 0.14f;
		const Point3 cellCenter{cellX, bRad * 0.08f, recCenter.z};
		builder.addBox(cellCenter, Point3{bRad * 0.14f, bRad * 0.32f, bRad * 0.35f}, UV_BODY);
		const Point3 cellNeonCenter{cellCenter.x + bRad * 0.14f + gap + bRad * 0.02f, cellCenter.y, cellCenter.z};
		builder.addBox(cellNeonCenter, Point3{bRad * 0.02f, bRad * 0.22f, bRad * 0.25f}, UV_CONDUIT);

		// Firing port collar centered at X=0, Y=0
		const float barrelR = settings.barrelRadius;
		const float barrelLen = settings.barrelLength;
		const float barrelStartZ = recZ + recL * 0.5f + gap;
		const float portEndZ = barrelStartZ + bRad * 0.22f;
		builder.addCylinder(Point3{0.0f, 0.0f, barrelStartZ}, Point3{0.0f, 0.0f, portEndZ}, barrelR * 1.70f, barrelR * 1.55f, 14, UV_DETAIL, false, true);

		// Heavy perforated cylindrical thermal cooling shroud (covers first section of barrel)
		const float shroudLen = barrelLen * 0.44f;
		const float shroudR = barrelR * 1.62f;
		const Point3 shroudStart{0.0f, 0.0f, portEndZ + gap};
		const Point3 shroudEnd{0.0f, 0.0f, portEndZ + gap + shroudLen};
		builder.addCylinder(shroudStart, shroudEnd, shroudR, shroudR * 0.94f, 14, UV_BODY, false, true);

		// Longitudinal cyan energy conduit rails running atop and below cooling shroud
		const float spineY = shroudR + gap + barrelR * 0.08f;
		const Point3 spineCenter{0.0f, spineY, portEndZ + gap + shroudLen * 0.5f};
		const Point3 bellyCenter{0.0f, -spineY, portEndZ + gap + shroudLen * 0.5f};
		builder.addBox(spineCenter, Point3{barrelR * 0.24f, barrelR * 0.08f, shroudLen * 0.46f}, UV_CONDUIT);
		builder.addBox(bellyCenter, Point3{barrelR * 0.24f, barrelR * 0.08f, shroudLen * 0.46f}, UV_CONDUIT);

		// Long exposed fluted sniper barrel extending out of shroud
		const float barrelEndZ = barrelStartZ + barrelLen;
		const Point3 innerBarrelStart{0.0f, 0.0f, shroudEnd.z + gap};
		const Point3 innerBarrelEnd{0.0f, 0.0f, barrelEndZ - barrelLen * 0.08f - gap};
		builder.addCylinder(innerBarrelStart, innerBarrelEnd, barrelR, barrelR, 14, UV_BARREL, false, false);

		// Stepped cylindrical double-baffle muzzle brake (unobscured, unclipped)
		const Point3 brakeStart{0.0f, 0.0f, barrelEndZ - barrelLen * 0.08f};
		const Point3 brakeMid{0.0f, 0.0f, barrelEndZ - barrelLen * 0.04f};
		const Point3 brakeEnd{0.0f, 0.0f, barrelEndZ};
		builder.addCylinder(brakeStart, brakeMid, barrelR * 1.40f, barrelR * 1.36f, 14, UV_DETAIL, false, false);
		builder.addCylinder(brakeMid, brakeEnd, barrelR * 1.36f, barrelR * 1.32f, 14, UV_DETAIL, false, false);

		// Neon cyan accent ring between brake baffles (radius clearance >= gap + 0.01f)
		const float mgRingR = barrelR * 1.40f + gap + 0.01f;
		builder.addCylinder(brakeMid - Point3{0, 0, barrelLen * 0.01f}, brakeMid + Point3{0, 0, barrelLen * 0.01f}, mgRingR, mgRingR, 14, UV_CONDUIT, false, false);

		// Prominent, 100% visible deep circular bore cavity
		builder.addMuzzleBore(brakeEnd, Point3{0.0f, 0.0f, 1.0f}, barrelR * 1.32f, barrelR * 0.72f, barrelLen * 0.22f, 14, UV_DETAIL, UV_BORE);

		report.pivotPosition = Point3{0.0f, 0.0f, 0.0f};
		report.muzzlePosition = Point3{0.0f, 0.0f, barrelEndZ};
	}

	void buildLazerDeletor(MeshBuilder& builder, const gen_model::turret::Settings& settings, gen_model::turret::GenerationReport& report) {
		const float bRad = settings.baseRadius;
		const float gap = 0.055f;

		buildPivotBallWithDetails(builder, bRad);

		// FrontOfGun (+Z): Heavy particle beam receiver block starting at Z = bRad + gap
		const float recW = bRad * 1.20f;
		const float recH = bRad * 0.96f;
		const float recL = bRad * 0.86f;
		const float recZ = bRad + gap + recL * 0.5f;
		const Point3 recCenter{0.0f, bRad * 0.06f, recZ};
		builder.addBox(recCenter, Point3{recW * 0.5f, recH * 0.5f, recL * 0.5f}, UV_BODY);

		// Dual heavy beam capacitor banks flanking the receiver (Port & Starboard)
		const float capW = bRad * 0.28f;
		const float capH = bRad * 0.76f;
		const float capL = bRad * 0.88f;
		const float capX = recW * 0.5f + gap + capW * 0.5f;
		const Point3 capLeftCenter{-capX, bRad * 0.06f, recCenter.z - recL * 0.05f};
		const Point3 capRightCenter{capX, bRad * 0.06f, recCenter.z - recL * 0.05f};
		builder.addBox(capLeftCenter, Point3{capW * 0.5f, capH * 0.5f, capL * 0.5f}, UV_DETAIL);
		builder.addBox(capRightCenter, Point3{capW * 0.5f, capH * 0.5f, capL * 0.5f}, UV_DETAIL);

		// Glowing green neon capacitor indicator strips
		const float capNeonX = capX + capW * 0.5f + gap + bRad * 0.02f;
		const Point3 capLeftNeon{-capNeonX, bRad * 0.06f, capLeftCenter.z};
		const Point3 capRightNeon{capNeonX, bRad * 0.06f, capRightCenter.z};
		builder.addBox(capLeftNeon, Point3{bRad * 0.02f, bRad * 0.28f, bRad * 0.36f}, UV_CONDUIT);
		builder.addBox(capRightNeon, Point3{bRad * 0.02f, bRad * 0.28f, bRad * 0.36f}, UV_CONDUIT);

		// Continuous Uninterrupted High-Velocity Passage (NO SEGMENTED RINGS)
		const float barrelR = settings.barrelRadius;
		const float barrelLen = settings.barrelLength;
		const float barrelStartZ = recZ + recL * 0.5f + gap;
		const float barrelEndZ = barrelStartZ + barrelLen;

		// Central cylindrical beam acceleration core pipe
		const Point3 coreStart{0.0f, 0.0f, barrelStartZ};
		const Point3 coreEnd{0.0f, 0.0f, barrelEndZ - barrelLen * 0.06f - gap};
		builder.addCylinder(coreStart, coreEnd, barrelR, barrelR, 14, UV_BARREL, false, false);

		// Upper Continuous Longitudinal Acceleration Rail (uninterrupted from breech to tip)
		const float railSpan = barrelLen * 0.92f;
		const float railArmY = barrelR + gap + barrelR * 0.16f;
		const Point3 railTopArmCenter{0.0f, railArmY, barrelStartZ + railSpan * 0.5f};
		builder.addBox(railTopArmCenter, Point3{barrelR * 0.34f, barrelR * 0.16f, railSpan * 0.5f}, UV_BODY);
		const float railNeonY = railArmY + barrelR * 0.16f + gap + barrelR * 0.05f;
		const Point3 railTopNeonCenter{0.0f, railNeonY, barrelStartZ + railSpan * 0.5f};
		builder.addBox(railTopNeonCenter, Point3{barrelR * 0.16f, barrelR * 0.05f, railSpan * 0.49f}, UV_CONDUIT);

		// Lower Continuous Longitudinal Acceleration Rail (uninterrupted from breech to tip)
		const Point3 railBottomArmCenter{0.0f, -railArmY, barrelStartZ + railSpan * 0.5f};
		builder.addBox(railBottomArmCenter, Point3{barrelR * 0.34f, barrelR * 0.16f, railSpan * 0.5f}, UV_BODY);
		const Point3 railBottomNeonCenter{0.0f, -railNeonY, barrelStartZ + railSpan * 0.5f};
		builder.addBox(railBottomNeonCenter, Point3{barrelR * 0.16f, barrelR * 0.05f, railSpan * 0.49f}, UV_CONDUIT);

		// Longitudinal Side Heat-Sink Ventilation Fins (flanking acceleration core)
		const float finX = barrelR + gap + barrelR * 0.06f;
		const Point3 finLeftCenter{-finX, 0.0f, barrelStartZ + railSpan * 0.5f};
		const Point3 finRightCenter{finX, 0.0f, barrelStartZ + railSpan * 0.5f};
		builder.addBox(finLeftCenter, Point3{barrelR * 0.06f, barrelR * 0.45f, railSpan * 0.46f}, UV_DETAIL);
		builder.addBox(finRightCenter, Point3{barrelR * 0.06f, barrelR * 0.45f, railSpan * 0.46f}, UV_DETAIL);

		// Precision Magnetic Beam Collimator Emitter Head at Muzzle
		const Point3 headStart{0.0f, 0.0f, barrelEndZ - barrelLen * 0.06f};
		const Point3 headEnd{0.0f, 0.0f, barrelEndZ};
		builder.addCylinder(headStart, headEnd, barrelR * 1.35f, barrelR * 1.25f, 14, UV_DETAIL, false, false);

		// Glowing emerald green aperture ring (radius clearance >= gap + 0.01f)
		const Point3 apRingStart{0.0f, 0.0f, barrelEndZ - barrelLen * 0.04f};
		const Point3 apRingEnd{0.0f, 0.0f, barrelEndZ - barrelLen * 0.015f};
		const float apRingR = barrelR * 1.35f + gap + 0.01f;
		builder.addCylinder(apRingStart, apRingEnd, apRingR, apRingR, 14, UV_CONDUIT, false, false);

		// Deep hollow energy emitter bore cavity
		builder.addMuzzleBore(headEnd, Point3{0.0f, 0.0f, 1.0f}, barrelR * 1.25f, barrelR * 0.75f, barrelLen * 0.22f, 14, UV_CONDUIT, UV_BORE);

		report.pivotPosition = Point3{0.0f, 0.0f, 0.0f};
		report.muzzlePosition = Point3{0.0f, 0.0f, barrelEndZ};
	}

	void buildLazerShotgun(MeshBuilder& builder, const gen_model::turret::Settings& settings, gen_model::turret::GenerationReport& report) {
		const float bRad = settings.baseRadius;
		const float gap = 0.055f;

		buildPivotBallWithDetails(builder, bRad);

		// FrontOfGun (+Z): Heavy angular shotgun receiver starting at Z = bRad + gap
		const float recW = bRad * 1.25f;
		const float recH = bRad * 0.90f;
		const float recL = bRad * 0.75f;
		const float recZ = bRad + gap + recL * 0.5f;
		const Point3 recCenter{0.0f, bRad * 0.04f, recZ};
		builder.addBox(recCenter, Point3{recW * 0.5f, recH * 0.5f, recL * 0.5f}, UV_BODY);

		// Dual lateral plasma expansion chambers (Port & Starboard)
		const float expR = bRad * 0.36f;
		const float expLen = bRad * 0.30f;
		const float expX = recW * 0.5f + gap + expLen * 0.5f;
		const Point3 expLeftCenter{-expX, bRad * 0.04f, recCenter.z};
		const Point3 expRightCenter{expX, bRad * 0.04f, recCenter.z};
		const Point3 expLeftA{expLeftCenter.x - expLen * 0.5f, expLeftCenter.y, expLeftCenter.z};
		const Point3 expLeftB{expLeftCenter.x + expLen * 0.5f, expLeftCenter.y, expLeftCenter.z};
		const Point3 expRightA{expRightCenter.x - expLen * 0.5f, expRightCenter.y, expRightCenter.z};
		const Point3 expRightB{expRightCenter.x + expLen * 0.5f, expRightCenter.y, expRightCenter.z};
		builder.addCylinder(expLeftA, expLeftB, expR, expR, 12, UV_DETAIL, true, true);
		builder.addCylinder(expRightA, expRightB, expR, expR, 12, UV_DETAIL, true, true);

		// Glowing green expansion indicator rings on outer cylinder caps
		const Point3 expLNeon1{expLeftA.x - gap - bRad * 0.03f, expLeftA.y, expLeftA.z};
		const Point3 expLNeon2{expLeftA.x - gap, expLeftA.y, expLeftA.z};
		const Point3 expRNeon1{expRightB.x + gap, expRightB.y, expRightB.z};
		const Point3 expRNeon2{expRightB.x + gap + bRad * 0.03f, expRightB.y, expRightB.z};
		builder.addCylinder(expLNeon1, expLNeon2, expR * 0.65f, expR * 0.65f, 10, UV_CONDUIT, true, true);
		builder.addCylinder(expRNeon1, expRNeon2, expR * 0.65f, expR * 0.65f, 10, UV_CONDUIT, true, true);

		// 5-Barrel 3D Conical Diverging Cluster
		const float barrelLen = settings.barrelLength;
		const float barrelStartZ = recZ + recL * 0.5f + gap + 0.015f;
		const float barrelEndZ = barrelStartZ + barrelLen;

		// Central Primary Projector Barrel (centered at X=0, Y=0)
		const float centerR = 0.30f;
		const Point3 centerStart{0.0f, 0.0f, barrelStartZ};
		const Point3 centerEnd{0.0f, 0.0f, barrelEndZ - barrelLen * 0.06f - gap};
		builder.addCylinder(centerStart, centerEnd, centerR, centerR, 12, UV_BARREL, false, false);

		// 4 Satellite Diverging Projector Barrels (Top, Bottom, Port, Starboard)
		// Clearance between center barrel (radius 0.30) and satellite (radius 0.22) >= 0.055f:
		// rStart >= 0.30 + 0.22 + 0.055 + 0.01 = 0.585f
		const float satR = 0.22f;
		const float rStart = 0.585f;
		const float rEnd = 0.79f;

		const Point3 satStarts[4] = {
			Point3{0.0f, rStart, barrelStartZ},
			Point3{0.0f, -rStart, barrelStartZ},
			Point3{-rStart, 0.0f, barrelStartZ},
			Point3{rStart, 0.0f, barrelStartZ}
		};

		const Point3 satEnds[4] = {
			Point3{0.0f, rEnd, barrelEndZ - barrelLen * 0.06f - gap - 0.03f},
			Point3{0.0f, -rEnd, barrelEndZ - barrelLen * 0.06f - gap - 0.03f},
			Point3{-rEnd, 0.0f, barrelEndZ - barrelLen * 0.06f - gap - 0.03f},
			Point3{rEnd, 0.0f, barrelEndZ - barrelLen * 0.06f - gap - 0.03f}
		};

		for (int i = 0; i < 4; ++i) {
			builder.addCylinder(satStarts[i], satEnds[i], satR, satR, 12, UV_BARREL, false, false);
		}

		// 5 Distinct Flared Muzzle Chokes with 100% Visible Deep Bores
		const float chokeLen = barrelLen * 0.06f;
		const float boreDepthCenter = chokeLen * 0.75f;
		const float boreDepthSat = chokeLen * 0.75f;

		const Point3 cChokeStart{0.0f, 0.0f, barrelEndZ - chokeLen};
		const Point3 cChokeEnd{0.0f, 0.0f, barrelEndZ};
		builder.addCylinder(cChokeStart, cChokeEnd, centerR * 1.25f, centerR * 1.20f, 12, UV_DETAIL, false, false);
		builder.addMuzzleBore(cChokeEnd, Point3{0.0f, 0.0f, 1.0f}, centerR * 1.20f, centerR * 0.72f, boreDepthCenter, 12, UV_CONDUIT, UV_BORE);

		const Point3 muzzleFinalCenters[4] = {
			Point3{0.0f, rEnd, barrelEndZ},
			Point3{0.0f, -rEnd, barrelEndZ},
			Point3{-rEnd, 0.0f, barrelEndZ},
			Point3{rEnd, 0.0f, barrelEndZ}
		};

		for (int i = 0; i < 4; ++i) {
			const Point3 chokeStart = Point3{satEnds[i].x, satEnds[i].y, barrelEndZ - chokeLen};
			const Point3 chokeEnd = muzzleFinalCenters[i];
			builder.addCylinder(chokeStart, chokeEnd, satR * 1.28f, satR * 1.22f, 10, UV_DETAIL, false, false);
			builder.addMuzzleBore(chokeEnd, Point3{0.0f, 0.0f, 1.0f}, satR * 1.22f, satR * 0.70f, boreDepthSat, 10, UV_CONDUIT, UV_BORE);
		}

		report.pivotPosition = Point3{0.0f, 0.0f, 0.0f};
		report.muzzlePosition = Point3{0.0f, 0.0f, barrelEndZ};
	}

	TextureData generateAlbedoTexture(int width, int height, gen_model::turret::Archetype archetype) {
		TextureData tex;
		tex.width = width;
		tex.height = height;
		tex.rgba.resize(width * height * 4);

		// Energy neon color selection based on weapon class
		const bool isLaser = (archetype == gen_model::turret::Archetype::LazerDeletor ||
		                      archetype == gen_model::turret::Archetype::LazerShotgun);

		// Core neon color: Vivid Emerald Green for lasers (#00ff66), Electric Cyan for kinetic (#00f0ff)
		const std::uint8_t neonCoreR = isLaser ? 0   : 0;
		const std::uint8_t neonCoreG = isLaser ? 255 : 240;
		const std::uint8_t neonCoreB = isLaser ? 102 : 255;

		// Secondary outer neon glow: Green (#4ade80) vs Cyan (#38bdf8)
		const std::uint8_t neonGlowR = isLaser ? 74  : 56;
		const std::uint8_t neonGlowG = isLaser ? 222 : 189;
		const std::uint8_t neonGlowB = isLaser ? 128 : 248;

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int index = (y * width + x) * 4;
				const float u = static_cast<float>(x) / static_cast<float>(width);
				const float v = static_cast<float>(y) / static_cast<float>(height);

				// Base default: Pearl white ceramic armor (#e6edf3)
				std::uint8_t r = 230;
				std::uint8_t g = 237;
				std::uint8_t b = 243;

				if (u < 0.5f && v < 0.5f) {
					// UV_BASE: Light slate pivot sphere (#cbd5e1 with fine circumferential panel lines)
					r = 203;
					g = 213;
					b = 225;
					if ((x % 32 == 0) || (y % 32 == 0)) {
						// Subtle darker armor panel seam
						r = 148;
						g = 163;
						b = 184;
					}
				} else if (u >= 0.5f && v < 0.5f) {
					// UV_BODY: Crisp white ceramic mantle/receiver (#f1f5f9) with chamfer edge shading
					r = 241;
					g = 245;
					b = 249;
					if (u > 0.90f || v > 0.42f) {
						r = 210;
						g = 220;
						b = 230;
					}
				} else if (u < 0.5f && v >= 0.5f) {
					// Lower-left quadrant: contains UV_BARREL, UV_BORE, and UV_CONDUIT
					if (u >= UV_CONDUIT.u0 && u <= UV_CONDUIT.u1 && v >= UV_CONDUIT.v0 && v <= UV_CONDUIT.v1) {
						// Illuminated energy conduit line: intense neon center with soft outer falloff
						const float cv = (v - UV_CONDUIT.v0) / (UV_CONDUIT.v1 - UV_CONDUIT.v0);
						const float distToCenter = std::abs(cv - 0.5f) * 2.0f;
						if (distToCenter < 0.35f) {
							// High-intensity core
							r = neonCoreR;
							g = neonCoreG;
							b = neonCoreB;
						} else {
							// Soft illuminated halo
							const float t = (distToCenter - 0.35f) / 0.65f;
							r = static_cast<std::uint8_t>((1.0f - t) * neonGlowR + t * 24);
							g = static_cast<std::uint8_t>((1.0f - t) * neonGlowG + t * 32);
							b = static_cast<std::uint8_t>((1.0f - t) * neonGlowB + t * 45);
						}
					} else if (u < 0.22f && v > 0.80f) {
						// Deep bore shadow void: pitch black (#080a0e)
						r = 8;
						g = 10;
						b = 14;
					} else {
						// Barrel pipe steel: deep graphite slate (#334155)
						r = 51;
						g = 65;
						b = 85;
						if (y % 16 < 3) {
							// Longitudinal barrel flute groove
							r = 30;
							g = 41;
							b = 59;
						}
					}
				} else {
					// UV_DETAIL (u >= 0.5f && v >= 0.5f): Dark mechanical chassis (#1e293b)
					r = 30;
					g = 41;
					b = 59;

					// Heat sink vent slats & optical details
					if (u > 0.82f && v > 0.82f) {
						// Optical sensor / emitter focal point: glowing neon
						r = neonCoreR;
						g = neonCoreG;
						b = neonCoreB;
					} else if ((x % 16 < 4) && v < 0.80f) {
						// Heat radiator grill slots
						r = 15;
						g = 23;
						b = 42;
					}
				}

				tex.rgba[index] = r;
				tex.rgba[index + 1] = g;
				tex.rgba[index + 2] = b;
				tex.rgba[index + 3] = 255;
			}
		}
		return tex;
	}

	TextureData generateNormalMap(int width, int height) {
		TextureData norm;
		norm.width = width;
		norm.height = height;
		norm.rgba.resize(width * height * 4);

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int index = (y * width + x) * 4;
				const float u = static_cast<float>(x) / static_cast<float>(width);
				const float v = static_cast<float>(y) / static_cast<float>(height);

				// Neutral normal in tangent space: (128, 128, 255)
				std::uint8_t nx = 128;
				std::uint8_t ny = 128;
				std::uint8_t nz = 255;

				if (u >= UV_CONDUIT.u0 && u <= UV_CONDUIT.u1 && v >= UV_CONDUIT.v0 && v <= UV_CONDUIT.v1) {
					// Recessed conduit channel edges
					const float cv = (v - UV_CONDUIT.v0) / (UV_CONDUIT.v1 - UV_CONDUIT.v0);
					if (cv < 0.15f) {
						ny = 90; // Slanted wall facing +Y
					} else if (cv > 0.85f) {
						ny = 166; // Slanted wall facing -Y
					}
				} else if ((x % 64 == 0 || y % 64 == 0) && !(u < 0.22f && v > 0.80f)) {
					// Panel seam relief
					nx = 110;
					ny = 110;
					nz = 240;
				}

				norm.rgba[index] = nx;
				norm.rgba[index + 1] = ny;
				norm.rgba[index + 2] = nz;
				norm.rgba[index + 3] = 255;
			}
		}
		return norm;
	}
}

namespace gen_model::turret {

Settings defaultBasicShooter() {
	Settings s;
	s.id = "basic_shooter";
	s.archetype = Archetype::BasicShooter;
	s.seed = 1001u;
	s.baseRadius = 1.0f;
	s.baseHeight = 0.10f;
	s.barrelLength = 5.20f;
	s.barrelRadius = 0.42f;
	s.textureWidth = 512;
	s.textureHeight = 512;
	return s;
}

Settings defaultHeavyMgRifle() {
	Settings s;
	s.id = "heavy_mg_rifle";
	s.archetype = Archetype::HeavyMgRifle;
	s.seed = 2002u;
	s.baseRadius = 1.0f;
	s.baseHeight = 0.10f;
	s.barrelLength = 6.80f;
	s.barrelRadius = 0.46f;
	s.textureWidth = 512;
	s.textureHeight = 512;
	return s;
}

Settings defaultLazerDeletor() {
	Settings s;
	s.id = "lazer_deletor";
	s.archetype = Archetype::LazerDeletor;
	s.seed = 3003u;
	s.baseRadius = 1.0f;
	s.baseHeight = 0.10f;
	s.barrelLength = 7.00f;
	s.barrelRadius = 0.45f;
	s.textureWidth = 512;
	s.textureHeight = 512;
	return s;
}

Settings defaultLazerShotgun() {
	Settings s;
	s.id = "lazer_shotgun";
	s.archetype = Archetype::LazerShotgun;
	s.seed = 4004u;
	s.baseRadius = 1.0f;
	s.baseHeight = 0.10f;
	s.barrelLength = 4.50f;
	s.barrelRadius = 0.48f;
	s.textureWidth = 512;
	s.textureHeight = 512;
	return s;
}

GeneratedTurret generate(const Settings& settings) {
	if (settings.baseRadius <= 0.0f || settings.baseHeight <= 0.0f)
		throw std::invalid_argument("Turret base dimensions must be positive");
	if (settings.barrelLength <= 0.0f || settings.barrelRadius <= 0.0f)
		throw std::invalid_argument("Turret barrel dimensions must be positive");

	MeshBuilder builder;
	GenerationReport report;
	report.id = settings.id;
	report.archetype = settings.archetype;

	if (settings.archetype == Archetype::BasicShooter) {
		buildBasicShooter(builder, settings, report);
	} else if (settings.archetype == Archetype::HeavyMgRifle) {
		buildHeavyMgRifle(builder, settings, report);
	} else if (settings.archetype == Archetype::LazerDeletor) {
		buildLazerDeletor(builder, settings, report);
	} else if (settings.archetype == Archetype::LazerShotgun) {
		buildLazerShotgun(builder, settings, report);
	}

	float maxRadiusSq = 0.0f;
	float maxHeight = 0.0f;
	for (const auto& p : builder.mesh.positions) {
		const float radSq = p.x * p.x + p.z * p.z;
		if (radSq > maxRadiusSq)
			maxRadiusSq = radSq;
		if (p.y > maxHeight)
			maxHeight = p.y;
	}
	report.boundingRadius = std::sqrt(maxRadiusSq);
	report.boundingHeight = maxHeight;
	report.vertexCount = builder.mesh.positions.size();
	report.triangleCount = builder.mesh.triangles.size();

	GeneratedTurret result;
	result.asset.mesh = std::move(builder.mesh);
	result.asset.texture = generateAlbedoTexture(settings.textureWidth, settings.textureHeight, settings.archetype);
	result.asset.normalMap = generateNormalMap(settings.textureWidth, settings.textureHeight);
	result.report = report;
	return result;
}

} // namespace gen_model::turret

