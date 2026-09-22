#pragma once

#include "CoreMinimal.h"
#include "Foundation/Combat/ARDamageTypes.h"

struct FARDefensiveStatSnapshot
{
	float Evasion = 0.0f;
	bool bGuaranteedEvasion = false;
	bool bGuaranteedInvulnerability = false;
	float AttributeDefense = 0.0f;
	float AttributeDamageTakenIncrease = 0.0f;
	float OverallRemainingDamageMultiplier = 1.0f;
	float AttributeRemainingDamageMultiplier = 1.0f;
};

/** Pure numeric combat formula. Actor validity, teams, state mutation, and events belong to UARCombatSubsystem. */
class FARDamageResolver
{
public:
	static FARCombatDamageResult Resolve(
		const FARCombatDamageRequest& Request,
		const FAROffensiveStatSnapshot& Offense,
		const FARDefensiveStatSnapshot& Defense,
		FRandomStream& RandomStream);
};

