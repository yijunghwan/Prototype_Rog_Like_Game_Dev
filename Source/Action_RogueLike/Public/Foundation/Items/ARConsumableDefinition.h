#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ARConsumableDefinition.generated.h"

class UARConsumableInstance;
class UTexture2D;

UCLASS(BlueprintType)
class ACTION_ROGUELIKE_API UARConsumableDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FGameplayTag DefinitionTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FGameplayTag ItemTypeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText ShortDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display", meta=(MultiLine="true")) FText DetailedDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Runtime") TSubclassOf<UARConsumableInstance> RuntimeBehaviorClass;
};
