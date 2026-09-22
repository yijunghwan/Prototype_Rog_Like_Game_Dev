#include "Foundation/Combat/ARDamageResolver.h"

FARCombatDamageResult FARDamageResolver::Resolve(
	const FARCombatDamageRequest& Request,
	const FAROffensiveStatSnapshot& Offense,
	const FARDefensiveStatSnapshot& Defense,
	FRandomStream& RandomStream)
{
	FARCombatDamageResult Result;
	Result.FailureReason = EARRequestResult::Rejected;

	const bool bVoid = Request.Attribute == EARDamageAttribute::Void;
	if (!FMath::IsFinite(Request.BaseDamage)
		|| !FMath::IsFinite(Request.AttackPowerCoefficient)
		|| !FMath::IsFinite(Request.SpellPowerCoefficient))
	{
		return Result;
	}

	if (!Request.bGuaranteedHit && Request.bApplyEvasion)
	{
		const float EvasionChance = Defense.bGuaranteedEvasion ? 100.0f : FMath::Clamp(Defense.Evasion, 0.0f, 100.0f);
		if (RandomStream.FRandRange(0.0f, 100.0f) < EvasionChance)
		{
			Result.Outcome = EARDamageOutcome::Evaded;
			Result.FailureReason = EARRequestResult::Success;
			return Result;
		}
	}

	if (!bVoid && Defense.bGuaranteedInvulnerability)
	{
		Result.Outcome = EARDamageOutcome::Blocked;
		Result.FailureReason = EARRequestResult::Blocked;
		return Result;
	}

	const float BaseDamage = Request.BaseDamage
		+ Offense.AttackPower * Request.AttackPowerCoefficient
		+ Offense.SpellPower * Request.SpellPowerCoefficient;
	if (BaseDamage <= 0.0f)
	{
		Result.Outcome = EARDamageOutcome::Blocked;
		Result.FailureReason = EARRequestResult::Blocked;
		return Result;
	}

	float Damage = BaseDamage;
	if (Request.bApplyAmplification)
	{
		const float TotalAmplification = Offense.OverallAmplification + Offense.DeliveryAmplification + Offense.AttributeAmplification;
		Damage *= FMath::Max(0.0f, 1.0f + TotalAmplification / 100.0f);
	}

	if (Request.bCanCrit)
	{
		const float CriticalChance = FMath::Clamp(Offense.CriticalChance, 0.0f, 100.0f);
		Result.bCritical = RandomStream.FRandRange(0.0f, 100.0f) < CriticalChance;
		if (Result.bCritical)
		{
			Damage *= FMath::Max(0.0f, Offense.CriticalDamage / 100.0f);
		}
	}

	if (!bVoid)
	{
		if (!Request.bIgnoreDefense)
		{
			const float Penetration = FMath::Clamp(Offense.DefensePenetration, 0.0f, 100.0f);
			const float EffectiveDefense = FMath::Max(0.0f, Defense.AttributeDefense) * (1.0f - Penetration / 100.0f);
			Damage *= 100.0f / (100.0f + EffectiveDefense);
		}
		if (Request.bApplyAmplification)
		{
			Damage *= FMath::Max(0.0f, 1.0f + Defense.AttributeDamageTakenIncrease / 100.0f);
		}
		Damage *= Defense.OverallRemainingDamageMultiplier;
		Damage *= Defense.AttributeRemainingDamageMultiplier;
	}

	if (Damage <= 0.0f)
	{
		Result.Outcome = EARDamageOutcome::Blocked;
		Result.FailureReason = EARRequestResult::Blocked;
		return Result;
	}

	Result.FinalDamage = FMath::Max(1, FMath::FloorToInt(Damage));
	Result.Outcome = EARDamageOutcome::Applied;
	Result.FailureReason = EARRequestResult::Success;
	return Result;
}

