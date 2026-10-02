#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Foundation/Combat/ARDamageTypes.h"
#include "Foundation/Combat/ARDotTypes.h"
#include "Foundation/Combat/ARStaggerTypes.h"
#include "Foundation/Status/ARStatusEffectTypes.h"
#include "ARCombatBlueprintLibrary.generated.h"

UCLASS()
class ACTION_ROGUELIKE_API UARCombatBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="AR|Combat", meta=(WorldContext="WorldContextObject"))
	static bool CanDamageTarget(const UObject* WorldContextObject, AActor* Attacker, AActor* Target, EARRequestResult& FailureReason);

	UFUNCTION(BlueprintCallable, Category="AR|Combat", meta=(WorldContext="WorldContextObject"))
	static FARCombatDamageResult ApplyCombatDamage(const UObject* WorldContextObject, const FARCombatDamageRequest& Request);

	UFUNCTION(BlueprintCallable, Category="AR|Combat", meta=(WorldContext="WorldContextObject"))
	static FARDotHandle ApplyDamageOverTime(const UObject* WorldContextObject, const FARDamageOverTimeSpec& Spec, bool& bSuccess, EARRequestResult& FailureReason);

	UFUNCTION(BlueprintCallable, Category="AR|Combat", meta=(WorldContext="WorldContextObject"))
	static bool RemoveDamageOverTime(const UObject* WorldContextObject, FARDotHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static FARStaggerResult ApplyStaggerAndGroggyDamage(const FARStaggerRequest& Request);

	/** Connect Apply Combat Damage's Return Value directly. Only an Applied result with a usable hit is processed; other outcomes do nothing. Does not deal HP damage or deduplicate repeated calls. */
	UFUNCTION(BlueprintCallable, Category="AR|Combat", meta=(DisplayName="Apply Stagger And Groggy Damage From Result", AutoCreateRefTerm="Source", AdvancedDisplay="Source,EffectName", CPP_Default_BaseStaggerDamage="0.0", CPP_Default_StaggerMultiplier="1.0", CPP_Default_BaseGroggyDamage="0.0", CPP_Default_EffectName="None"))
	static FARStaggerResult ApplyStaggerAndGroggyDamageFromResult(const FARCombatDamageResult& DamageResult, float BaseStaggerDamage, float StaggerMultiplier, float BaseGroggyDamage, const FARSourceInfo& Source, FName EffectName);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static FARSuperArmorHandle ApplySuperArmor(AActor* Target, const FARSuperArmorSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static bool RemoveSuperArmor(AActor* Target, FARSuperArmorHandle Handle);

	/** Guarantees that new crowd-control statuses have zero effective duration. */
	UFUNCTION(BlueprintCallable, Category="AR|Combat|CC Immunity")
	static FARCCImmunityHandle ApplyCCImmunity(AActor* Target, const FARCCImmunitySpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Combat|CC Immunity")
	static bool RemoveCCImmunity(AActor* Target, FARCCImmunityHandle Handle);

	/** DA-free stun/root. Duration is seconds (>0); tenacity is enabled by default. Respects CC immunity and same-tag longest-duration refresh. Stun requests action cancellation; Root cancels rolls only. Return Value is the status handle, usable with Remove Status Effect and On Status Removed. Applied Duration is this request's reduced duration, not the existing status's remaining time. */
	UFUNCTION(BlueprintCallable, Category="AR|Combat|CC", meta=(DisplayName="Apply Crowd Control", Keywords="CC Stun Root", AutoCreateRefTerm="Source", AdvancedDisplay="Source,FailureReason,AppliedDuration", CPP_Default_CCType="Stun", CPP_Default_Duration="1.0", CPP_Default_bAffectedByTenacity="true"))
	static FARStatusEffectHandle ApplyCrowdControl(AActor* Target, EARCrowdControlType CCType, float Duration, bool bAffectedByTenacity, bool& bSuccess, EARRequestResult& FailureReason, float& AppliedDuration, const FARSourceInfo& Source);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static FARStatusEffectResult ApplyStatusEffect(AActor* Target, const FARStatusEffectRequest& Request);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static bool RemoveStatusEffect(AActor* Target, FARStatusEffectHandle Handle);
};
