#include "spaceship_config.hpp"

#include <fstream>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

#include "json.hpp"
#include "spaceship_design.hpp"

namespace {
	using Json = nlohmann::json;

	const Json& requireObjectMember(const Json& object, const char* key, const std::string& path) {
		if (!object.is_object() || !object.contains(key))
			throw std::invalid_argument("Missing spaceship setting: " + path + "." + key);
		return object.at(key);
	}

	float requireFloat(const Json& object, const char* key, const std::string& path) {
		const Json& value = requireObjectMember(object, key, path);
		if (!value.is_number())
			throw std::invalid_argument("Spaceship setting must be numeric: " + path + "." + key);
		return value.get<float>();
	}

	int requireInt(const Json& object, const char* key, const std::string& path) {
		const Json& value = requireObjectMember(object, key, path);
		if (!value.is_number_integer())
			throw std::invalid_argument("Spaceship setting must be an integer: " + path + "." + key);
		return value.get<int>();
	}

	std::string requireString(const Json& object, const char* key, const std::string& path) {
		const Json& value = requireObjectMember(object, key, path);
		if (!value.is_string())
			throw std::invalid_argument("Spaceship setting must be a string: " + path + "." + key);
		return value.get<std::string>();
	}

	gen_model::spaceship::RgbColor requireColor(
		const Json& object,
		const char* key,
		const std::string& path
	) {
		const Json& value = requireObjectMember(object, key, path);
		if (!value.is_array() || value.size() != 3u)
			throw std::invalid_argument("Spaceship color must contain three channels: " + path + "." + key);
		gen_model::spaceship::RgbColor result;
		std::uint8_t* channels[]{&result.r, &result.g, &result.b};
		for (std::size_t index = 0; index < value.size(); ++index) {
			if (!value[index].is_number_integer())
				throw std::invalid_argument("Spaceship color channel must be an integer: " + path + "." + key);
			const int channel = value[index].get<int>();
			if (channel < 0 || channel > 255)
				throw std::invalid_argument("Spaceship color channel must be between 0 and 255: " + path + "." + key);
			*channels[index] = static_cast<std::uint8_t>(channel);
		}
		return result;
	}

	gen_model::spaceship::PaintPattern parsePaintPattern(
		const std::string& value,
		const std::string& path
	) {
		using Pattern = gen_model::spaceship::PaintPattern;
		if (value == "spine_band") return Pattern::SpineBand;
		if (value == "chevron") return Pattern::Chevron;
		if (value == "wing_bands") return Pattern::WingBands;
		if (value == "blocked") return Pattern::Blocked;
		if (value == "hazard") return Pattern::Hazard;
		throw std::invalid_argument("Unknown spaceship paint pattern: " + path + "=" + value);
	}

	gen_model::spaceship::PropulsionLayout parsePropulsionLayout(
		const std::string& value,
		const std::string& path
	) {
		using Layout = gen_model::spaceship::PropulsionLayout;
		if (value == "auto") return Layout::Auto;
		if (value == "central_cluster") return Layout::CentralCluster;
		if (value == "spine_cluster") return Layout::SpineCluster;
		if (value == "twin_boom") return Layout::TwinBoom;
		if (value == "wing_nacelles") return Layout::WingNacelles;
		if (value == "distributed_aft") return Layout::DistributedAft;
		if (value == "capital_side_blocks") return Layout::CapitalSideBlocks;
		throw std::invalid_argument("Unknown spaceship propulsion layout: " + path + "=" + value);
	}

	gen_model::spaceship::PlacementMode parsePlacementMode(
		const std::string& value,
		const std::string& path
	) {
		if (value == "auto") return gen_model::spaceship::PlacementMode::Auto;
		if (value == "manual") return gen_model::spaceship::PlacementMode::Manual;
		throw std::invalid_argument("Unknown spaceship placement mode: " + path + "=" + value);
	}

	gen_model::spaceship::WeaponCoverage parseWeaponCoverage(
		const std::string& value,
		const std::string& path
	) {
		using Coverage = gen_model::spaceship::WeaponCoverage;
		if (value == "forward") return Coverage::Forward;
		if (value == "broadside") return Coverage::Broadside;
		if (value == "omnidirectional") return Coverage::Omnidirectional;
		throw std::invalid_argument("Unknown spaceship weapon coverage: " + path + "=" + value);
	}

	gen_model::spaceship::LayoutSymmetry parseLayoutSymmetry(
		const std::string& value,
		const std::string& path
	) {
		using Symmetry = gen_model::spaceship::LayoutSymmetry;
		if (value == "bilateral") return Symmetry::Bilateral;
		if (value == "radial") return Symmetry::Radial;
		if (value == "none") return Symmetry::None;
		throw std::invalid_argument("Unknown spaceship layout symmetry: " + path + "=" + value);
	}

	gen_model::spaceship::BatteryStyle parseBatteryStyle(
		const std::string& value,
		const std::string& path
	) {
		using Style = gen_model::spaceship::BatteryStyle;
		if (value == "auto") return Style::Auto;
		if (value == "integrated") return Style::Integrated;
		if (value == "external") return Style::External;
		throw std::invalid_argument("Unknown spaceship battery style: " + path + "=" + value);
	}

	gen_model::spaceship::DesignSettings parseDesign(
		const Json& ship,
		const std::string& path
	) {
		gen_model::spaceship::DesignSettings result;
		if (!ship.contains("design"))
			return result;
		const Json& value = ship.at("design");
		if (!value.is_object())
			throw std::invalid_argument("Spaceship setting must be an object: " + path + ".design");
		const std::string designPath = path + ".design";
		result.propulsionLayout = parsePropulsionLayout(
			requireString(value, "propulsionLayout", designPath),
			designPath + ".propulsionLayout"
		);
		result.propulsionPlacement = parsePlacementMode(
			requireString(value, "propulsionPlacement", designPath),
			designPath + ".propulsionPlacement"
		);
		result.targetAcceleration = requireFloat(value, "targetAcceleration", designPath);
		result.endurance = requireFloat(value, "endurance", designPath);
		result.armorMassScale = requireFloat(value, "armorMassScale", designPath);
		result.engineTechnology = requireFloat(value, "engineTechnology", designPath);
		result.moduleClearance = requireFloat(value, "moduleClearance", designPath);
		result.hardSurfaceBias = requireFloat(value, "hardSurfaceBias", designPath);
		if (!value.contains("weaponLayout"))
			return result;
		const Json& weapon = value.at("weaponLayout");
		if (!weapon.is_object())
			throw std::invalid_argument("Spaceship setting must be an object: " + designPath + ".weaponLayout");
		const std::string weaponPath = designPath + ".weaponLayout";
		result.weaponLayout.placement = parsePlacementMode(
			requireString(weapon, "placement", weaponPath), weaponPath + ".placement"
		);
		result.weaponLayout.coverage = parseWeaponCoverage(
			requireString(weapon, "coverage", weaponPath), weaponPath + ".coverage"
		);
		result.weaponLayout.symmetry = parseLayoutSymmetry(
			requireString(weapon, "symmetry", weaponPath), weaponPath + ".symmetry"
		);
		result.weaponLayout.batteryStyle = parseBatteryStyle(
			requireString(weapon, "batteryStyle", weaponPath), weaponPath + ".batteryStyle"
		);
		if (weapon.contains("turretCount")) {
			const int turretCount = requireInt(weapon, "turretCount", weaponPath);
			if (turretCount <= 0)
				throw std::invalid_argument(weaponPath + ".turretCount must be positive");
			result.weaponLayout.turretCount = static_cast<std::size_t>(turretCount);
		}
		result.weaponLayout.minimumSeparationScale = requireFloat(
			weapon, "minimumSeparationScale", weaponPath
		);
		return result;
	}

	gen_model::gen_types::Point3 requirePoint(const Json& object, const char* key, const std::string& path) {
		const Json& point = requireObjectMember(object, key, path);
		const std::string pointPath = path + "." + key;
		return {
			requireFloat(point, "x", pointPath),
			requireFloat(point, "y", pointPath),
			requireFloat(point, "z", pointPath)
		};
	}

	gen_model::gen_types::Point3 normalizedPoint(
		gen_model::gen_types::Point3 point,
		const std::string& path
	) {
		const float lengthSquared = gen_model::gen_types::dot(point, point);
		if (!std::isfinite(lengthSquared) || lengthSquared <= 0.00001f)
			throw std::invalid_argument("Spaceship facingDirection must be a finite nonzero vector: " + path);
		return point * (1.0f / std::sqrt(lengthSquared));
	}

	float defaultFloat(const Json& object, const char* key, float fallback, const std::string& path) {
		if (!object.contains(key))
			return fallback;
		if (!object.at(key).is_number())
			throw std::invalid_argument("Spaceship setting must be numeric: " + path + "." + key);
		return object.at(key).get<float>();
	}

	gen_model::spaceship::MountAttachmentSettings parseMountAttachment(
		const Json& ship,
		const Json& defaults,
		const std::string& shipPath
	) {
		const Json baseline = requireObjectMember(defaults, "mountAttachment", "defaults");
		const Json* values = &baseline;
		std::string valuePath = "defaults.mountAttachment";
		if (ship.contains("mountAttachment")) {
			if (!ship.at("mountAttachment").is_object())
				throw std::invalid_argument(
					"Spaceship setting must be an object: " + shipPath + ".mountAttachment"
				);
			values = &ship.at("mountAttachment");
			valuePath = shipPath + ".mountAttachment";
		}
		const auto value = [&](const char* key) {
			return defaultFloat(
				*values,
				key,
				requireFloat(baseline, key, "defaults.mountAttachment"),
				valuePath
			);
		};
		return {
			value("preferredTakeoffAngleDegrees"),
			value("minimumTakeoffAngleDegrees"),
			value("maximumTakeoffAngleDegrees"),
			value("directBlisterGapScale"),
			value("distanceWeight"),
			value("angleWeight"),
			value("blisterRadiusScale")
		};
	}

	gen_model::spaceship::MountSettings parseMount(
		const Json& value,
		const Json& defaults,
		const std::string& path
	) {
		gen_model::spaceship::MountSettings mount;
		mount.id = requireString(value, "id", path);
		mount.position = requirePoint(value, "position", path);
		mount.forward = requirePoint(value, "forward", path);
		mount.requestedFacing = normalizedPoint(mount.forward, path + ".forward");
		mount.supportRoot = requirePoint(value, "supportRoot", path);
		mount.turretRadius = defaultFloat(
			value, "turretRadius", requireFloat(defaults, "turretRadius", "defaults"), path
		);
		mount.barrelRadius = defaultFloat(
			value, "barrelRadius", requireFloat(defaults, "barrelRadius", "defaults"), path
		);
		mount.barrelLength = defaultFloat(
			value, "barrelLength", requireFloat(defaults, "barrelLength", "defaults"), path
		);
		mount.traverseHalfAngleDegrees = defaultFloat(
			value,
			"traverseHalfAngleDegrees",
			requireFloat(defaults, "traverseHalfAngleDegrees", "defaults"),
			path
		);
		mount.supportWidth = requireFloat(value, "supportWidth", path);
		mount.supportHeight = requireFloat(value, "supportHeight", path);
		mount.socketHeight = requireFloat(value, "socketHeight", path);
		return mount;
	}

	gen_model::spaceship::MountSettings automaticMount(
		const Json& defaults,
		const Json* capability,
		const std::string& path,
		std::size_t index,
		std::size_t count,
		gen_model::spaceship::WeaponCoverage coverage
	) {
		gen_model::spaceship::MountSettings mount;
		mount.id = "mount_" + std::to_string(index);
		mount.turretRadius = requireFloat(defaults, "turretRadius", "defaults");
		mount.barrelRadius = requireFloat(defaults, "barrelRadius", "defaults");
		mount.barrelLength = requireFloat(defaults, "barrelLength", "defaults");
		if (coverage == gen_model::spaceship::WeaponCoverage::Forward)
			mount.traverseHalfAngleDegrees = count <= 2u ? 20.0f : (count <= 4u ? 5.0f : 12.0f);
		else
			mount.traverseHalfAngleDegrees = count >= 16u ? 10.0f : 12.0f;
		if (capability != nullptr) {
			if (!capability->is_object())
				throw std::invalid_argument("Spaceship mount capability must be an object: " + path);
			mount.turretRadius = defaultFloat(*capability, "turretRadius", mount.turretRadius, path);
			mount.barrelRadius = defaultFloat(*capability, "barrelRadius", mount.barrelRadius, path);
			mount.barrelLength = defaultFloat(*capability, "barrelLength", mount.barrelLength, path);
			mount.traverseHalfAngleDegrees = defaultFloat(
				*capability, "traverseHalfAngleDegrees", mount.traverseHalfAngleDegrees, path
			);
		}
		mount.supportWidth = mount.turretRadius * (count >= 6u ? 3.0f : 2.4f);
		mount.supportHeight = mount.turretRadius * (count >= 6u ? 2.4f : 2.0f);
		mount.socketHeight = mount.turretRadius * 0.88f;
		if (capability != nullptr) {
			mount.supportWidth = defaultFloat(*capability, "supportWidth", mount.supportWidth, path);
			mount.supportHeight = defaultFloat(*capability, "supportHeight", mount.supportHeight, path);
			mount.socketHeight = defaultFloat(*capability, "socketHeight", mount.socketHeight, path);
			if (capability->contains("facingDirection")) {
				const Json& facing = capability->at("facingDirection");
				if (facing.is_null())
					mount.requestedFacing.reset();
				else
					mount.requestedFacing = normalizedPoint(
						requirePoint(*capability, "facingDirection", path),
						path + ".facingDirection"
					);
			}
		}
		mount.forward = mount.requestedFacing.value_or(
			gen_model::gen_types::Point3{0.0f, 0.0f, 1.0f}
		);
		return mount;
	}

	gen_model::spaceship::Settings parseShip(
		const std::string& id,
		const Json& value,
		const Json& defaults
	) {
		const std::string path = "ships." + id;
		gen_model::spaceship::Settings settings;
		settings.id = id;
		settings.modelPath = requireString(value, "modelPath", path);
		settings.seed = static_cast<std::uint32_t>(requireInt(value, "seed", path));
		settings.design = parseDesign(value, path);
		settings.mountAttachment = parseMountAttachment(value, defaults, path);

		const Json& dimensions = requireObjectMember(value, "dimensions", path);
		settings.dimensions = {
			requireFloat(dimensions, "width", path + ".dimensions"),
			requireFloat(dimensions, "height", path + ".dimensions"),
			requireFloat(dimensions, "length", path + ".dimensions")
		};

		const Json& hull = requireObjectMember(value, "hull", path);
		settings.hull = {
			requireFloat(hull, "width", path + ".hull"),
			requireFloat(hull, "height", path + ".hull"),
			requireFloat(hull, "length", path + ".hull"),
			requireFloat(hull, "crown", path + ".hull"),
			requireFloat(hull, "keel", path + ".hull"),
			requireFloat(hull, "noseSharpness", path + ".hull"),
			requireFloat(hull, "rearTaper", path + ".hull")
		};

		const Json& wings = requireObjectMember(value, "wings", path);
		settings.wings = {
			requireFloat(wings, "halfSpan", path + ".wings"),
			requireFloat(wings, "rootFrontZ", path + ".wings"),
			requireFloat(wings, "rootRearZ", path + ".wings"),
			requireFloat(wings, "tipFrontZ", path + ".wings"),
			requireFloat(wings, "tipRearZ", path + ".wings"),
			requireFloat(wings, "rootX", path + ".wings"),
			requireFloat(wings, "topY", path + ".wings"),
			requireFloat(wings, "bottomY", path + ".wings"),
			requireFloat(wings, "shoulderWidth", path + ".wings")
		};

		const Json& cockpit = requireObjectMember(value, "cockpit", path);
		settings.cockpit.center = requirePoint(cockpit, "center", path + ".cockpit");
		settings.cockpit.size = requirePoint(cockpit, "size", path + ".cockpit");
		settings.cockpit.browDepth = requireFloat(cockpit, "browDepth", path + ".cockpit");

		const Json& layout = requireObjectMember(value, "layout", path);
		settings.layout = {
			requireString(layout, "archetype", path + ".layout"),
			requireInt(layout, "crew", path + ".layout"),
			requireFloat(layout, "primarySpineWidth", path + ".layout"),
			requireFloat(layout, "reactorRadius", path + ".layout"),
			requireFloat(layout, "serviceBayLength", path + ".layout"),
			requireFloat(layout, "radiatorScale", path + ".layout"),
			requireFloat(layout, "weaponDeckCantDegrees", path + ".layout")
		};

		const Json& wear = requireObjectMember(value, "wear", path);
		settings.wear = {
			requireFloat(wear, "paintLoss", path + ".wear"),
			requireFloat(wear, "oxidation", path + ".wear"),
			requireFloat(wear, "heatStaining", path + ".wear"),
			requireInt(wear, "repairPanels", path + ".wear")
		};

		const Json& material = requireObjectMember(value, "material", path);
		const std::string materialPath = path + ".material";
		settings.material = {
			requireColor(material, "armorBase", materialPath),
			requireColor(material, "armorSecondary", materialPath),
			requireColor(material, "accent", materialPath),
			requireColor(material, "structure", materialPath),
			requireColor(material, "canopy", materialPath),
			requireColor(material, "engine", materialPath),
			parsePaintPattern(
				requireString(material, "pattern", materialPath),
				materialPath + ".pattern"
			),
			requireFloat(material, "secondaryCoverage", materialPath),
			requireFloat(material, "accentCoverage", materialPath),
			requireFloat(material, "normalStrength", materialPath),
			requireFloat(material, "detailScale", materialPath)
		};

		const Json& texture = requireObjectMember(value, "texture", path);
		settings.textureWidth = requireInt(texture, "width", path + ".texture");
		settings.textureHeight = requireInt(texture, "height", path + ".texture");
		const Json& topology = requireObjectMember(value, "topology", path);
		settings.cylinderSegments = requireInt(topology, "cylinderSegments", path + ".topology");
		settings.armorDepth = requireFloat(value, "armorDepth", path);

		const Json& engines = requireObjectMember(value, "engines", path);
		if (!engines.is_array() || engines.empty())
			throw std::invalid_argument(path + ".engines must be a non-empty array");
		settings.engines.reserve(engines.size());
		for (std::size_t index = 0; index < engines.size(); ++index) {
			const Json& engine = engines[index];
			const std::string enginePath = path + ".engines[" + std::to_string(index) + "]";
			settings.engines.push_back({
				requirePoint(engine, "center", enginePath),
				requireFloat(engine, "radius", enginePath),
				requireFloat(engine, "length", enginePath),
				requireFloat(engine, "nozzleDepth", enginePath)
			});
		}

		const std::size_t automaticCount = settings.design.weaponLayout.turretCount;
		if (settings.design.weaponLayout.placement == gen_model::spaceship::PlacementMode::Auto
			&& automaticCount > 0u) {
			const Json* capabilities = nullptr;
			const Json& design = requireObjectMember(value, "design", path);
			const std::string designPath = path + ".design";
			const Json& weapon = requireObjectMember(design, "weaponLayout", designPath);
			if (weapon.contains("capabilities")) {
				const Json& configured = weapon.at("capabilities");
				if (!configured.is_array() || configured.size() != automaticCount)
					throw std::invalid_argument(
						path + ".design.weaponLayout.capabilities must contain one entry per automatic turret"
					);
				capabilities = &configured;
			}
			settings.mounts.reserve(automaticCount);
			for (std::size_t index = 0; index < automaticCount; ++index) {
				const std::string mountPath = path + ".design.weaponLayout.capabilities["
					+ std::to_string(index) + "]";
				settings.mounts.push_back(automaticMount(
					defaults,
					capabilities == nullptr ? nullptr : &(*capabilities)[index],
					mountPath,
					index,
					automaticCount,
					settings.design.weaponLayout.coverage
				));
			}
		} else {
			const Json& mounts = requireObjectMember(value, "mounts", path);
			if (!mounts.is_array() || mounts.empty())
				throw std::invalid_argument(path + ".mounts must be a non-empty array");
			settings.mounts.reserve(mounts.size());
			for (std::size_t index = 0; index < mounts.size(); ++index) {
				settings.mounts.push_back(parseMount(
					mounts[index], defaults, path + ".mounts[" + std::to_string(index) + "]"
				));
			}
		}
		if (settings.layout.weaponDeckCantDegrees != 0.0f
			&& settings.design.weaponLayout.placement == gen_model::spaceship::PlacementMode::Manual) {
			constexpr float degreesToRadians = 3.14159265358979323846f / 180.0f;
			const float angle = settings.layout.weaponDeckCantDegrees * degreesToRadians;
			for (auto& mount : settings.mounts) {
				const float horizontalLength = std::sqrt(
					mount.forward.x * mount.forward.x + mount.forward.z * mount.forward.z
				);
				if (horizontalLength <= 0.00001f)
					throw std::invalid_argument(path + " weapon mount cannot cant a vertical forward vector");
				const float deckSide = mount.position.y < 0.0f ? -1.0f : 1.0f;
				mount.forward = {
					mount.forward.x / horizontalLength * std::cos(angle),
					deckSide * std::sin(angle),
					mount.forward.z / horizontalLength * std::cos(angle)
				};
				mount.requestedFacing = normalizedPoint(mount.forward, path + ".mount.forward");
			}
		}
		return settings;
	}

	Json pointJson(gen_model::gen_types::Point3 point) {
		return {{"x", point.x}, {"y", point.y}, {"z", point.z}};
	}

	Json engineJson(const gen_model::spaceship::EngineSettings& engine) {
		return {
			{"center", pointJson(engine.center)},
			{"radius", engine.radius},
			{"length", engine.length},
			{"nozzleDepth", engine.nozzleDepth}
		};
	}

	Json mountJson(const gen_model::spaceship::MountSettings& mount) {
		return {
			{"id", mount.id},
			{"facingDirection", mount.requestedFacing.has_value()
				? pointJson(*mount.requestedFacing)
				: Json(nullptr)},
			{"position", pointJson(mount.position)},
			{"forward", pointJson(mount.forward)},
			{"supportRoot", pointJson(mount.supportRoot)},
			{"turretRadius", mount.turretRadius},
			{"barrelRadius", mount.barrelRadius},
			{"barrelLength", mount.barrelLength},
			{"traverseHalfAngleDegrees", mount.traverseHalfAngleDegrees},
			{"supportWidth", mount.supportWidth},
			{"supportHeight", mount.supportHeight},
			{"socketHeight", mount.socketHeight}
		};
	}
}

std::vector<gen_model::spaceship::Settings> gen_model::spaceship::loadCatalog(std::istream& input) {
	Json catalog;
	try {
		input >> catalog;
	} catch (const Json::parse_error& error) {
		throw std::invalid_argument(std::string("Invalid spaceship JSON: ") + error.what());
	}
	if (catalog.value("schemaVersion", 0) != 1)
		throw std::invalid_argument("spaceships.json schemaVersion must be 1");
	const Json& defaults = catalog.at("defaults");
	const Json& ships = catalog.at("ships");
	if (!ships.is_object() || ships.empty())
		throw std::invalid_argument("spaceships.json must define at least one ship");
	std::vector<Settings> result;
	result.reserve(ships.size());
	for (auto iterator = ships.begin(); iterator != ships.end(); ++iterator)
		result.push_back(parseShip(iterator.key(), iterator.value(), defaults));
	return result;
}

std::vector<gen_model::spaceship::Settings> gen_model::spaceship::loadCatalog(
	const std::filesystem::path& path
) {
	std::ifstream input(path);
	if (!input)
		throw std::runtime_error("Unable to open spaceship catalog: " + path.string());
	return loadCatalog(input);
}

void gen_model::spaceship::writeGenerationReport(
	const gen_model::spaceship::GeneratedShip& ship,
	const std::filesystem::path& path
) {
	if (!path.parent_path().empty())
		std::filesystem::create_directories(path.parent_path());
	Json mounts = Json::array();
	for (const MountReport& mount : ship.mounts) {
		mounts.push_back({
			{"id", mount.id},
			{"structurallyConnected", mount.structurallyConnected},
			{"supportThickness", mount.supportThickness},
			{"minimumClearance", mount.minimumClearance},
			{"parentCollisionClearance", mount.parentCollisionClearance},
			{"attachmentPoint", pointJson(mount.attachmentPoint)},
			{"resolvedForward", pointJson(mount.resolvedForward)},
			{"facingErrorDegrees", mount.facingErrorDegrees},
			{"takeoffAngleDegrees", mount.takeoffAngleDegrees},
			{"supportLength", mount.supportLength},
			{"directBlister", mount.directBlister}
		});
	}
	Json resolvedEngines = Json::array();
	for (std::size_t index = 0; index < ship.resolvedEngines.size(); ++index) {
		Json engine = engineJson(ship.resolvedEngines[index]);
		if (index < ship.resolvedNozzleCells.size())
			engine["nozzleCells"] = ship.resolvedNozzleCells[index];
		resolvedEngines.push_back(std::move(engine));
	}
	Json resolvedMounts = Json::array();
	for (const auto& mount : ship.resolvedMounts)
		resolvedMounts.push_back(mountJson(mount));
	const Json report{
		{"settingsFingerprint", ship.settingsFingerprint},
		{"design", {
			{"selectedLayout", design::layoutName(ship.design.selectedLayout)},
			{"massProxy", ship.design.massProxy},
			{"weaponBurden", ship.design.weaponBurden},
			{"magazineVolume", ship.design.magazineVolume},
			{"fuelVolume", ship.design.fuelVolume},
			{"engineVolume", ship.design.engineVolume},
			{"reactorVolume", ship.design.reactorVolume},
			{"requiredThrust", ship.design.requiredThrust},
			{"availableThrust", ship.design.availableThrust},
			{"massCenter", pointJson(ship.design.massCenter)},
			{"thrustCenter", pointJson(ship.design.thrustCenter)}
		}},
		{"systems", {
			{"fuelHousings", ship.systemDetails.fuelHousings},
			{"feedTrunks", ship.systemDetails.feedTrunks},
			{"reactorShields", ship.systemDetails.reactorShields},
			{"radiatorPanels", ship.systemDetails.radiatorPanels},
			{"serviceAccessPanels", ship.systemDetails.serviceAccessPanels}
		}},
		{"material", {
			{"normalSlopeMean", ship.materialDetails.normalSlopeMean},
			{"normalSlopeP95", ship.materialDetails.normalSlopeP95},
			{"normalSlopeMaximum", ship.materialDetails.normalSlopeMaximum},
			{"secondaryCoverage", ship.materialDetails.secondaryCoverage},
			{"accentCoverage", ship.materialDetails.accentCoverage},
			{"thermalCoverage", ship.materialDetails.thermalCoverage}
		}},
		{"bounds", {
			{"minimum", pointJson(ship.bounds.minimum)},
			{"maximum", pointJson(ship.bounds.maximum)},
			{"maximumRadius", ship.bounds.maximumRadius}
		}},
		{"vertices", ship.asset.mesh.positions.size()},
		{"triangles", ship.asset.mesh.triangles.size()},
		{"mounts", mounts},
		{"resolvedEngines", resolvedEngines},
		{"resolvedMounts", resolvedMounts}
	};
	std::ofstream output(path);
	if (!output)
		throw std::runtime_error("Unable to write spaceship report: " + path.string());
	output << report.dump(2) << '\n';
}
