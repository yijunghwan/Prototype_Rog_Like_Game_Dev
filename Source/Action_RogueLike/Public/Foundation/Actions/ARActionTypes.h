#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARActionTypes.generated.h"

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARActionCancelRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCancelOnStagger = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCancelOnStun = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCancelOnRoll = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCancelOnBasicMovementInput = false;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARActionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag ActionTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARActionCancelRules CancelRules;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBlockRollWhileActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBlockBasicMovementWhileActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsRollAction = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FARSkillGroupHandle SkillGroupHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid OwningItemInstanceId;
};
