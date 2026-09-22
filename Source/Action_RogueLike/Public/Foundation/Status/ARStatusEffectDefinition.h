#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARStatusEffectDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class ACTION_ROGUELIKE_API UARStatusEffectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FGameplayTag DefinitionTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FGameplayTag StatusTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules", meta=(ClampMin="0.0")) float BaseDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bAffectedByTenacity = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bCanBeImmune = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bBlocksBasicMovement = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bBlocksAllMovement = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bBlocksRoll = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bBlocksSkillGroups = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules") bool bCancelActionsOnApply = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rules", meta=(EditCondition="bCancelActionsOnApply")) EARActionCancelReason ActionCancelReason = EARActionCancelReason::Stun;
};
