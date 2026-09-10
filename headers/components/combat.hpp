#ifndef COMPONENTS_COMBAT_HPP
#define COMPONENTS_COMBAT_HPP

#include "includes.hpp"
#include <array>

namespace combat {

struct HP
{
	float value;
	float maxValue;

	HP(float val): value(val), maxValue(val) {}
};

struct HPRegen
{
	float value;
};

struct EnergyShield
{
	float hp;
	float maxHp;
	float activeDuration;
	float activeTimer = 0.0f;

	EnergyShield(float val, float _activeDuration=3.0f): hp(val), maxHp(val), activeDuration(_activeDuration) {}
};

struct EnergyShieldRegen
{
	float value;
	float regenCd = 2.0f;
};

struct Damage
{
	float value;
};

struct DelayedDamage
{
	float timeRemaining;
	float damage;
};

struct DamageContributors {
	struct DamageContributorEntry {
		entt::entity attacker = entt::null;
		float damage = 0.0f;
		float lastHitGameTime = 0.0f;
	};

	static constexpr size_t MAX_ENTRIES = 3;
	std::array<DamageContributorEntry, MAX_ENTRIES> entries{};
	size_t count = 0;

	float getTotalDamage() const {
		float total = 0.0f;
		for (size_t i = 0; i < count; ++i)
			total += entries[i].damage;
		return total;
	}

	void recordDamage(entt::entity attacker, float amount, float currentGameTime) {
		if (attacker == entt::null || amount <= 0.0f)
			return;
		if (tryUpdateExisting(attacker, amount, currentGameTime))
			return;
		if (tryAppendNew(attacker, amount, currentGameTime))
			return;
		evictLRU(attacker, amount, currentGameTime);
	}

	entt::entity getLastDamageDealer(float currentGameTime, float maxAge = 10.0f, entt::entity exclude = entt::null) const {
		entt::entity best = entt::null;
		float bestTime = -1.0f;
		for (size_t i = 0; i < count; ++i) {
			const auto &entry = entries[i];
			if (entry.attacker == entt::null || entry.attacker == exclude)
				continue;
			if ((currentGameTime - entry.lastHitGameTime) > maxAge)
				continue;
			if (entry.lastHitGameTime <= bestTime)
				continue;
			bestTime = entry.lastHitGameTime;
			best = entry.attacker;
		}
		return best;
	}

private:
	bool tryUpdateExisting(entt::entity attacker, float amount, float currentGameTime) {
		for (size_t i = 0; i < count; ++i) {
			if (entries[i].attacker != attacker)
				continue;
			entries[i].damage += amount;
			entries[i].lastHitGameTime = currentGameTime;
			return true;
		}
		return false;
	}

	bool tryAppendNew(entt::entity attacker, float amount, float currentGameTime) {
		if (count >= MAX_ENTRIES)
			return false;
		entries[count++] = {attacker, amount, currentGameTime};
		return true;
	}

	void evictLRU(entt::entity attacker, float amount, float currentGameTime) {
		size_t lruIdx = 0;
		for (size_t i = 1; i < count; ++i) {
			if (entries[i].lastHitGameTime < entries[lruIdx].lastHitGameTime)
				lruIdx = i;
		}
		entries[lruIdx] = {attacker, amount, currentGameTime};
	}
};

namespace tag {

struct Targetable {};

} // namespace tag

} // namespace combat

namespace tag {
using ::combat::tag::Targetable;
} // namespace tag

using ::combat::HP;
using ::combat::HPRegen;
using ::combat::EnergyShield;
using ::combat::EnergyShieldRegen;
using ::combat::Damage;
using ::combat::DelayedDamage;
using ::combat::DamageContributors;

#endif // COMPONENTS_COMBAT_HPP
