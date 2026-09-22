#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Foundation/Items/ARConsumableTypes.h"
#include "ARConsumableInstance.generated.h"

class AARPlayerCharacter;
class UARConsumableDefinition;

UCLASS(Abstract, Blueprintable, BlueprintType)
class ACTION_ROGUELIKE_API UARConsumableInstance : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	void InitializeInstance(AARPlayerCharacter* InOwner, UARConsumableDefinition* InDefinition);
	void RegisterInSlot(int32 InSlotIndex);
	void UnregisterFromSlot(EARConsumableRemovalReason Reason);

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Consumable", meta=(DisplayName="On Consumable Registered"))
	void ReceiveConsumableRegistered();

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Consumable", meta=(DisplayName="On Consumable Unregistered"))
	void ReceiveConsumableUnregistered(EARConsumableRemovalReason Reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="AR|Consumable")
	bool CanUseConsumable(FGameplayTag& FailureTag) const;

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Consumable")
	void ExecuteConsumableUse();

	UFUNCTION(BlueprintPure, Category="AR|Consumable") AARPlayerCharacter* GetConsumableOwner() const { return ConsumableOwner.Get(); }
	UFUNCTION(BlueprintPure, Category="AR|Consumable") UARConsumableDefinition* GetConsumableDefinition() const { return Definition; }
	UFUNCTION(BlueprintPure, Category="AR|Consumable") FGuid GetInstanceId() const { return InstanceId; }
	UFUNCTION(BlueprintPure, Category="AR|Consumable") int32 GetSlotIndex() const { return SlotIndex; }
	UFUNCTION(BlueprintPure, Category="AR|Consumable") bool IsRegistered() const { return bRegistered; }

private:
	UPROPERTY(Transient) TWeakObjectPtr<AARPlayerCharacter> ConsumableOwner;
	UPROPERTY(Transient) TObjectPtr<UARConsumableDefinition> Definition = nullptr;
	UPROPERTY(Transient) FGuid InstanceId;
	UPROPERTY(Transient) int32 SlotIndex = INDEX_NONE;
	bool bRegistered = false;
};
