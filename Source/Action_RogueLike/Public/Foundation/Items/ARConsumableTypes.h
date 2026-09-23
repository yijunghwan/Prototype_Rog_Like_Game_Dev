#pragma once

#include "CoreMinimal.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARConsumableTypes.generated.h"

class UARConsumableDefinition;
class AARConsumablePickup;

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
