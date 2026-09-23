#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "ARStatsBlueprintLibrary.generated.h"

UCLASS()
class ACTION_ROGUELIKE_API UARStatsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static FARStatModifierHandle ApplyStatModifier(AActor* Target, const FARStatModifierSpec& Spec, bool& bSuccess);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static bool RemoveStatModifier(AActor* Target, FARStatModifierHandle Handle);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static int32 RemoveStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static int32 ClearStatModifiers(AActor* Target, EARModifierSourceCategory Category = EARModifierSourceCategory::All);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static bool RemoveOneStatModifierStack(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static float GetFinalStat(AActor* Target, EARStatType StatType);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static FARStatBreakdown GetStatBreakdown(AActor* Target, EARStatType StatType);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static TArray<FARFinalStatView> GetAllFinalStatViews(AActor* Target);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static FARStatModifierQueryResult GetStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId);
};
