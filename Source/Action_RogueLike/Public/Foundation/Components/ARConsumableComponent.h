#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Items/ARConsumableTypes.h"
#include "ARConsumableComponent.generated.h"

class AARPlayerCharacter;
class UARConsumableDefinition;
class UARConsumableInstance;
class UARStatsComponent;
class AARConsumablePickup;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARConsumableSlotsChangedSignature, const TArray<FARConsumableSlotSnapshot>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARConsumableDropRequestedSignature, const FARConsumableDropRequest&, DropRequest);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARConsumableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARConsumableComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="AR|Consumable")
	FARConsumableAcquisitionResult TryAcquireConsumable(UARConsumableDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="AR|Consumable")
	FARRequestStatus TryUseConsumableSlot(int32 SlotIndex, UARConsumableDefinition*& UsedDefinition);

	UFUNCTION(BlueprintCallable, Category="AR|Consumable")
	FARRequestStatus DropConsumableSlot(int32 SlotIndex, FARConsumableDropRequest& DropRequest);

	UFUNCTION(BlueprintCallable, Category="AR|Consumable")
	void SetBaseMaxConsumableSlots(int32 NewSlotCount);

	UFUNCTION(BlueprintCallable, Category="AR|Consumable")
	void RetryPendingOverflowDrops();

	UFUNCTION(BlueprintPure, Category="AR|Consumable")
	TArray<FARConsumableSlotSnapshot> GetConsumableSlots() const;

	UFUNCTION(BlueprintPure, Category="AR|Consumable")
	int32 GetMaxConsumableSlots() const { return DesiredSlotCount; }

	UPROPERTY(BlueprintAssignable, Category="AR|Consumable") FARConsumableSlotsChangedSignature OnConsumableSlotsChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Consumable") FARConsumableDropRequestedSignature OnConsumableDropRequested;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Consumable|Drop") TSubclassOf<AARConsumablePickup> DroppedConsumablePickupClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Consumable|Drop", meta=(ClampMin="0.0")) float DropForwardDistance = 96.0f;

private:
	bool ValidateDefinition(const UARConsumableDefinition* Definition, FARRequestStatus& Status) const;
	void SynchronizeSlotCount();
	void BroadcastSlotsChanged();
	void RemoveSlotInstance(int32 SlotIndex, EARConsumableRemovalReason Reason);
	bool TrySpawnDropForSlot(int32 SlotIndex, bool bCausedBySlotReduction, FARConsumableDropRequest& DropRequest);

	UFUNCTION() void HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue);

	UPROPERTY(Transient) TObjectPtr<AARPlayerCharacter> PlayerOwner;
	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) TArray<TObjectPtr<UARConsumableInstance>> Slots;
	int32 DesiredSlotCount = 0;
};
