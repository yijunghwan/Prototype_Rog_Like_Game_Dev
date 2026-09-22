#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Foundation/Combat/ARDamageResolver.h"
#include "Foundation/Components/ARStatsComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARDamageFormulaTest,
	"AR.Foundation.Combat.DamageFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARDamageFormulaTest::RunTest(const FString& Parameters)
{
	FARCombatDamageRequest Request;
	Request.BaseDamage = 100.0f;
	Request.AttackPowerCoefficient = 1.0f;
	Request.bCanCrit = false;

	FAROffensiveStatSnapshot Offense;
	Offense.AttackPower = 50.0f;
	Offense.OverallAmplification = 10.0f;
	Offense.DeliveryAmplification = 20.0f;
	Offense.AttributeAmplification = 30.0f;
	Offense.DefensePenetration = 50.0f;

	FARDefensiveStatSnapshot Defense;
	Defense.AttributeDefense = 100.0f;
	Defense.AttributeDamageTakenIncrease = 25.0f;
	Defense.OverallRemainingDamageMultiplier = 0.5f;
	Defense.AttributeRemainingDamageMultiplier = 0.5f;

	FRandomStream Random(12345);
	const FARCombatDamageResult Result = FARDamageResolver::Resolve(Request, Offense, Defense, Random);
	TestEqual(TEXT("Formula order produces expected final damage"), Result.FinalDamage, 50);
	TestEqual(TEXT("Damage is applied"), Result.Outcome, EARDamageOutcome::Applied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARVoidDamageRulesTest,
	"AR.Foundation.Combat.VoidRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARVoidDamageRulesTest::RunTest(const FString& Parameters)
{
	FARCombatDamageRequest Request;
	Request.Attribute = EARDamageAttribute::Void;
	Request.BaseDamage = 100.0f;
	Request.bCanCrit = false;

	FAROffensiveStatSnapshot Offense;
	Offense.OverallAmplification = 50.0f;
	FARDefensiveStatSnapshot Defense;
	Defense.AttributeDefense = 9999.0f;
	Defense.AttributeDamageTakenIncrease = 999.0f;
	Defense.OverallRemainingDamageMultiplier = 0.0f;
	Defense.AttributeRemainingDamageMultiplier = 0.0f;
	Defense.bGuaranteedInvulnerability = true;

	FRandomStream Random(7);
	const FARCombatDamageResult Result = FARDamageResolver::Resolve(Request, Offense, Defense, Random);
	TestEqual(TEXT("Void ignores defense, reductions, vulnerability, and normal invulnerability"), Result.FinalDamage, 150);
	TestEqual(TEXT("Void damage is applied"), Result.Outcome, EARDamageOutcome::Applied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAREvasionAndMinimumDamageTest,
	"AR.Foundation.Combat.EvasionAndMinimum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAREvasionAndMinimumDamageTest::RunTest(const FString& Parameters)
{
	FARCombatDamageRequest Request;
	Request.BaseDamage = 0.1f;
	Request.bCanCrit = false;
	FAROffensiveStatSnapshot Offense;
	FARDefensiveStatSnapshot Defense;
	FRandomStream Random(1);

	FARCombatDamageResult Result = FARDamageResolver::Resolve(Request, Offense, Defense, Random);
	TestEqual(TEXT("Positive damage is floored with minimum one"), Result.FinalDamage, 1);

	Defense.bGuaranteedEvasion = true;
	Result = FARDamageResolver::Resolve(Request, Offense, Defense, Random);
	TestEqual(TEXT("Guaranteed evasion wins when hit is not guaranteed"), Result.Outcome, EARDamageOutcome::Evaded);

	Request.bGuaranteedHit = true;
	Result = FARDamageResolver::Resolve(Request, Offense, Defense, Random);
	TestEqual(TEXT("Guaranteed hit bypasses guaranteed evasion"), Result.Outcome, EARDamageOutcome::Applied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatModifierFormulaTest,
	"AR.Foundation.Stats.ModifierFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARStatModifierFormulaTest::RunTest(const FString& Parameters)
{
	UARStatsComponent* Stats = NewObject<UARStatsComponent>();
	Stats->SetBaseStat(EARStatType::AttackPower, 100.0f);
	bool bSuccess = false;

	FARStatModifierSpec Flat;
	Flat.StatType = EARStatType::AttackPower;
	Flat.Operation = EARStatModifierOperation::Flat;
	Flat.Value = 20.0f;
	Flat.Duration = -1.0f;
	Stats->AddStatModifier(Flat, bSuccess);
	TestTrue(TEXT("Flat modifier accepted"), bSuccess);

	FARStatModifierSpec Additive = Flat;
	Additive.Operation = EARStatModifierOperation::AdditivePercent;
	Additive.Value = 50.0f;
	Stats->AddStatModifier(Additive, bSuccess);

	FARStatModifierSpec Multiplicative = Flat;
	Multiplicative.Operation = EARStatModifierOperation::Multiplicative;
	Multiplicative.Value = 2.0f;
	Stats->AddStatModifier(Multiplicative, bSuccess);
	TestEqual(TEXT("(Base + Flat) * additive * multiplicative"), Stats->GetFinalStat(EARStatType::AttackPower), 360.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARIndependentReductionTest,
	"AR.Foundation.Stats.IndependentReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARIndependentReductionTest::RunTest(const FString& Parameters)
{
	UARStatsComponent* Stats = NewObject<UARStatsComponent>();
	bool bSuccess = false;
	FARStatModifierSpec Reduction;
	Reduction.StatType = EARStatType::OverallDamageReduction;
	Reduction.Operation = EARStatModifierOperation::IndependentDamageReduction;
	Reduction.Value = 50.0f;
	Reduction.Duration = -1.0f;
	Stats->AddStatModifier(Reduction, bSuccess);
	Stats->AddStatModifier(Reduction, bSuccess);
	TestEqual(TEXT("Two independent 50 percent reductions leave 25 percent damage"), Stats->GetDamageRemainingMultiplier(EARStatType::OverallDamageReduction), 0.25f);
	TestEqual(TEXT("Displayed final reduction is 75 percent"), Stats->GetFinalStat(EARStatType::OverallDamageReduction), 75.0f);
	return true;
}

#endif

