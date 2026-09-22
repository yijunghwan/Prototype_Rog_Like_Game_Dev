#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Foundation/Actions/ARActionTypes.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARItemTypes.generated.h"

class UTexture2D;
class UARLoadoutItemDefinition;

UENUM(BlueprintType)
enum class EARLoadoutItemKind : uint8
{
	Weapon,
	ActiveRelic,
	PassiveRelic
};

UENUM(BlueprintType)
enum class EARItemRemovalReason : uint8
{
	Replaced,
	Discarded,
	Evolved,
	OwnerDestroyed,
	Manual
};

UENUM(BlueprintType)
enum class EARItemUIStateDisplayType : uint8
{
	Gauge,
	Number,
	SmallStack
};

UENUM(BlueprintType)
enum class EARItemUIStatePlacement : uint8
{
	HUD,
	Details,
	Both
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARUIStateDisplayDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag StateId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EARItemUIStateDisplayType DisplayType = EARItemUIStateDisplayType::Number;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="6", EditCondition="DisplayType==EARItemUIStateDisplayType::SmallStack")) int32 MaxDisplaySlots = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bShowNumberAlongside = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EARItemUIStatePlacement Placement = EARItemUIStatePlacement::Both;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARItemUIState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGameplayTag StateId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CurrentValue = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumValue = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARItemUIStateDisplayType EffectiveDisplayType = EARItemUIStateDisplayType::Number;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARSkillDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FName SkillId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") FGameplayTag InputTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") int32 InputPriority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cost") FARResourceCost ResourceCost;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cooldown", meta=(ClampMin="0.0")) float BaseCooldown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cooldown", meta=(ClampMin="0.0")) float MinimumCooldown = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Action") FARActionRequest ActionRequest;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display") FText SkillDisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display", meta=(MultiLine="true")) FText SkillDescription;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display") TSoftObjectPtr<UTexture2D> SkillIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display") bool bShowOnHUD = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display") int32 HUDSortOrder = 0;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARRegisteredSkillUIData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARRegisteredSkillHandle RegisteredHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid ItemInstanceId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName SkillId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGameplayTag InputTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARResourceCost ResourceCost;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CooldownRemaining = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CooldownTotal = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bReady = true;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARLoadoutItemSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<const UARLoadoutItemDefinition> Definition = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARLoadoutItemKind Kind = EARLoadoutItemKind::PassiveRelic;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FARItemUIState> UIStates;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARLoadoutAcquisitionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARRequestStatus Status;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARAcquisitionToken Token;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bWillReplaceWeapon = false;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARLoadoutDropRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<const UARLoadoutItemDefinition> Definition = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector SuggestedLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARWeaponEvolutionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARRequestStatus Status;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FAREvolutionToken Token;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<class UARWeaponDefinition>> Candidates;
};
