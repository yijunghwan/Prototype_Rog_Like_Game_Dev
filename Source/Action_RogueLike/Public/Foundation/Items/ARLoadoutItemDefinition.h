#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Foundation/Items/ARItemTypes.h"
#include "ARLoadoutItemDefinition.generated.h"

class UARLoadoutItemInstance;
class UTexture2D;

UCLASS(Abstract, BlueprintType)
class ACTION_ROGUELIKE_API UARLoadoutItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual EARLoadoutItemKind GetItemKind() const PURE_VIRTUAL(UARLoadoutItemDefinition::GetItemKind, return EARLoadoutItemKind::PassiveRelic;);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FGameplayTag DefinitionTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FGameplayTag ItemTypeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") FText ShortDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display", meta=(MultiLine="true")) FText DetailedDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display") TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats") TArray<FARStatModifierSpec> DefaultStatModifiers;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skills") TArray<FARSkillDefinition> SkillDefinitions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Runtime") TSubclassOf<UARLoadoutItemInstance> RuntimeBehaviorClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI") TArray<FARUIStateDisplayDefinition> UIStateDisplayDefinitions;
};

UCLASS(BlueprintType)
class ACTION_ROGUELIKE_API UARWeaponDefinition : public UARLoadoutItemDefinition
{
	GENERATED_BODY()

public:
	virtual EARLoadoutItemKind GetItemKind() const override { return EARLoadoutItemKind::Weapon; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Evolution") FName EvolutionGroupId = NAME_None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Evolution", meta=(ClampMin="1")) int32 EvolutionStage = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Evolution") TArray<TSoftObjectPtr<UARWeaponDefinition>> NextEvolutionCandidates;
};

UCLASS(BlueprintType)
class ACTION_ROGUELIKE_API UARActiveRelicDefinition : public UARLoadoutItemDefinition
{
	GENERATED_BODY()

public:
	virtual EARLoadoutItemKind GetItemKind() const override { return EARLoadoutItemKind::ActiveRelic; }
};

UCLASS(BlueprintType)
class ACTION_ROGUELIKE_API UARPassiveRelicDefinition : public UARLoadoutItemDefinition
{
	GENERATED_BODY()

public:
	virtual EARLoadoutItemKind GetItemKind() const override { return EARLoadoutItemKind::PassiveRelic; }
};

