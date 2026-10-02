#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Foundation/Items/ARItemTypes.h"
#include "ARItemDefinition.generated.h"

class UARItemInstance;
class UTexture2D;

/** The one authorable data-asset type for weapons, relics, and consumables. */
UCLASS(BlueprintType)
class ACTION_ROGUELIKE_API UARItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Only call after confirming this is a weapon or relic; consumables have no loadout kind. */
	EARLoadoutItemKind GetItemKind() const;

	UFUNCTION(BlueprintPure, Category="AR|Item")
	bool IsConsumable() const;

	UFUNCTION(BlueprintPure, Category="AR|Item")
	bool HasAdditionalTag(const FString& Tag) const;

	/** The lookup key is (ItemTypeTag, ItemId); 0 is reserved for unassigned content. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity", meta=(Categories="Item.Type")) FGameplayTag ItemTypeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity", meta=(ClampMin="1")) int32 ItemId = 0;
	/** Free-form family labels. A label may be shared by many item definitions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") TArray<FString> AdditionalTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText ShortDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display", meta=(MultiLine="true")) FText DetailedDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats") TArray<FARStatModifierSpec> DefaultStatModifiers;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skills") TArray<FARSkillDefinition> SkillDefinitions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Runtime") TSubclassOf<UARItemInstance> RuntimeBehaviorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI") TArray<FARUIStateDisplayDefinition> UIStateDisplayDefinitions;

	/** Used only by weapons; other types must leave these fields empty/default. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Evolution") FName EvolutionGroupId = NAME_None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Evolution", meta=(ClampMin="1")) int32 EvolutionStage = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Evolution") TArray<TSoftObjectPtr<UARItemDefinition>> NextEvolutionCandidates;
};
