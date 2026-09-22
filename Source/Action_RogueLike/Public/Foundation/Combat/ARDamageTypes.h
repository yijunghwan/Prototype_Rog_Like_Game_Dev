#pragma once

#include "CoreMinimal.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARDamageTypes.generated.h"

UENUM(BlueprintType)
enum class EARDamageOutcome : uint8
{
	Invalid,
	Queued,
	Applied,
	Evaded,
	Blocked
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FAROffensiveStatSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AttackPower = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SpellPower = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StaggerPower = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float GroggyDamageAmplification = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CriticalChance = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CriticalDamage = 150.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DefensePenetration = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Absorption = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float OverallAmplification = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DeliveryAmplification = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AttributeAmplification = 0.0f;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARDamageHitContext
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid HitId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Attacker = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARDamageDelivery Delivery = EARDamageDelivery::Direct;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARDamageAttribute Attribute = EARDamageAttribute::Physical;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bValidHit = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StaggerPowerSnapshot = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float GroggyDamageAmplificationSnapshot = 0.0f;

	bool IsUsable() const { return bValidHit && HitId.IsValid() && ::IsValid(Target.Get()); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARCombatDamageRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Attacker = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EARDamageDelivery Delivery = EARDamageDelivery::Direct;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EARDamageAttribute Attribute = EARDamageAttribute::Physical;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float BaseDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackPowerCoefficient = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpellPowerCoefficient = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bGuaranteedHit = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyEvasion = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanCrit = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyAmplification = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIgnoreDefense = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIgnoreShield = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyAbsorption = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyOnHitEffects = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARSourceInfo Source;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DamageName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag AttackTag;

	bool bUseOffensiveSnapshot = false;
	FAROffensiveStatSnapshot OffensiveSnapshot;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARCombatDamageResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARDamageOutcome Outcome = EARDamageOutcome::Invalid;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARRequestResult FailureReason = EARRequestResult::Rejected;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCritical = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bKilledTarget = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bOnHitEffectsTriggered = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 FinalDamage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ShieldDamage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 HealthDamage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARDamageHitContext HitContext;

	bool WasApplied() const { return Outcome == EARDamageOutcome::Applied; }
};
