#pragma once

#include "CoreMinimal.h"
#include "Foundation/Combat/ARDamageTypes.h"
#include "Foundation/Combat/ARStaggerTypes.h"
#include "ARDotTypes.generated.h"

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARDamageOverTimeSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARCombatDamageRequest DamageRequest;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) float Duration = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001")) float TickInterval = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DotName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EARDotStackPolicy StackPolicy = EARDotStackPolicy::Independent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bApplyStaggerAndGroggyEachTick = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bApplyStaggerAndGroggyEachTick")) FARStaggerRequestTemplate StaggerTemplate;
};

