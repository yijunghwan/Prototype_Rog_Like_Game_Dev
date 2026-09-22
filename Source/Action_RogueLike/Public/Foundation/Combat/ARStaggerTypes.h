#pragma once

#include "CoreMinimal.h"
#include "Foundation/Combat/ARDamageTypes.h"
#include "ARStaggerTypes.generated.h"

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStaggerRequestTemplate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float BaseStaggerDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float StaggerMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float BaseGroggyDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARSourceInfo Source;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EffectName = NAME_None;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStaggerRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARDamageHitContext HitContext;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARStaggerRequestTemplate Template;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStaggerResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bValidRequest = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bStaggerAttempted = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bStaggered = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bBlockedBySuperArmor = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bGroggyDepleted = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 FinalStaggerDamage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 FinalGroggyDamage = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CurrentGroggy = 0.0f;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARSuperArmorSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARSourceInfo Source;
};

