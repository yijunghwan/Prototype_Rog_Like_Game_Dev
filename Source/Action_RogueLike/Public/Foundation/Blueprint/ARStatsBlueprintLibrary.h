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
	/** Permanent Flat changes the base value without recording a modifier. On success its Return Value is intentionally an invalid handle: check Success, not handle validity. Duration/Source/HUD/tenacity/stack group are ignored in that mode; Stack Only and guarantees are rejected. Money is player-only; insufficient base balance rejects a deduction. */
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static FARStatModifierHandle ApplyStatModifier(AActor* Target, const FARStatModifierSpec& Spec, bool& bSuccess);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static bool RemoveStatModifier(AActor* Target, FARStatModifierHandle Handle);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static int32 RemoveStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId);
	UFUNCTION(BlueprintCallable, Category="AR|Stats") static int32 ClearStatModifiers(AActor* Target, EARModifierSourceCategory Category = EARModifierSourceCategory::All);
	/** Removes Count application groups. Require Full Count rejects insufficient stacks without consuming any. */
	UFUNCTION(BlueprintCallable, Category="AR|Stats", meta=(AdvancedDisplay="Policy,bRequireFullCount", CPP_Default_Count="1"))
	static bool RemoveStatModifierStacks(AActor* Target, EARModifierSourceCategory Category, FName SourceId, int32 Count, int32& RemovedCount, EARModifierStackRemovalPolicy Policy = EARModifierStackRemovalPolicy::Oldest, bool bRequireFullCount = true);
	/** Load-only compatibility for saved graphs; use Remove Stat Modifier Stacks for new content. */
	UFUNCTION(BlueprintCallable, Category="AR|Stats", meta=(BlueprintInternalUseOnly="true", DeprecatedFunction, DeprecationMessage="Use Remove Stat Modifier Stacks with Count=1."))
	static bool RemoveOneStatModifierStack(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static float GetFinalStat(AActor* Target, EARStatType StatType);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static FARStatBreakdown GetStatBreakdown(AActor* Target, EARStatType StatType);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static TArray<FARFinalStatView> GetAllFinalStatViews(AActor* Target);
	UFUNCTION(BlueprintPure, Category="AR|Stats") static FARStatModifierQueryResult GetStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId);
};
