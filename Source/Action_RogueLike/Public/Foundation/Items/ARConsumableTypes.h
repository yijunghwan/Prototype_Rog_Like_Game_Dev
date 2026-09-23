#pragma once

#include "CoreMinimal.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARConsumableTypes.generated.h"

class UARConsumableDefinition;
class AARConsumablePickup;
class UTexture2D;

UENUM(BlueprintType)
enum class EARConsumableRemovalReason : uint8
{
	Used,
	Dropped,
	SlotReduced,
	OwnerDestroyed,
	Manual
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARConsumableSlotSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bOccupied = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UARConsumableDefinition> Definition = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bOverflowSlot = false;
};

/** UI-safe static and runtime identity data for an owned slot or an unowned shop candidate. */
USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARConsumableDisplayData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<const UARConsumableDefinition> Definition = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGameplayTag DefinitionTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGameplayTag ItemTypeTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText ShortDescription;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText DetailedDescription;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARConsumableAcquisitionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARRequestStatus Status;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid InstanceId;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARConsumableDropRequest
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UARConsumableDefinition> Definition = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector SuggestedLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 PreviousSlotIndex = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCausedBySlotReduction = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AARConsumablePickup> SpawnedPickup = nullptr;
};
