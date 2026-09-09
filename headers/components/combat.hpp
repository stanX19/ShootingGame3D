#pragma once

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
