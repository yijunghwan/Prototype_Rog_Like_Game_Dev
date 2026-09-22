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

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static FARSuperArmorHandle ApplySuperArmor(AActor* Target, const FARSuperArmorSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static bool RemoveSuperArmor(AActor* Target, FARSuperArmorHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static FARStatusEffectResult ApplyStatusEffect(AActor* Target, const FARStatusEffectRequest& Request);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	static bool RemoveStatusEffect(AActor* Target, FARStatusEffectHandle Handle);
};

