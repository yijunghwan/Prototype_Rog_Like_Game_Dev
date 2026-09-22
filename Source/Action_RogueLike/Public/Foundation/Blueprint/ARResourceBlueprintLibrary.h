#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "ARResourceBlueprintLibrary.generated.h"

UCLASS()
class ACTION_ROGUELIKE_API UARResourceBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static float RestoreHealth(AActor* Target, float Amount, const FARSourceInfo& Source);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static float RestoreMana(AActor* Target, float Amount, const FARSourceInfo& Source);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static float RestoreStamina(AActor* Target, float Amount, const FARSourceInfo& Source);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static bool TryConsumeMana(AActor* Target, float Amount, const FARSourceInfo& Source, float& NewMana);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static bool TryConsumeStamina(AActor* Target, float Amount, const FARSourceInfo& Source, float& NewStamina);
	UFUNCTION(BlueprintPure, Category="AR|Resource") static bool CanAffordResources(AActor* Target, const FARResourceCost& Cost, EARResourceType& MissingResource);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static bool TryConsumeResources(AActor* Target, const FARResourceCost& Cost, const FARSourceInfo& Source, float& NewMana, float& NewStamina, EARResourceType& MissingResource);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static FARShieldHandle ApplyShield(AActor* Target, const FARShieldSpec& Spec, bool& bSuccess, float& NewTotalShield);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static bool RemoveShield(AActor* Target, FARShieldHandle Handle, float& RemovedAmount, float& NewTotalShield);
	UFUNCTION(BlueprintCallable, Category="AR|Resource") static int32 RemoveShieldsBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARShieldLifetimeFilter LifetimeFilter, float& RemovedAmount, float& NewTotalShield);
	UFUNCTION(BlueprintPure, Category="AR|Resource") static bool GetCurrentResource(AActor* Target, EARResourceType ResourceType, float& Current, float& Maximum, float& Ratio);
};

