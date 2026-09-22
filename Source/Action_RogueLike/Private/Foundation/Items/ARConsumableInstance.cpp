#include "Foundation/Items/ARConsumableInstance.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Items/ARConsumableDefinition.h"

UWorld* UARConsumableInstance::GetWorld() const
{
	return ConsumableOwner.IsValid() ? ConsumableOwner->GetWorld() : nullptr;
}

void UARConsumableInstance::InitializeInstance(AARPlayerCharacter* InOwner, UARConsumableDefinition* InDefinition)
{
	ConsumableOwner = InOwner;
	Definition = InDefinition;
	InstanceId = FGuid::NewGuid();
}

void UARConsumableInstance::RegisterInSlot(int32 InSlotIndex)
{
	if (bRegistered || !ConsumableOwner.IsValid() || !Definition || InSlotIndex < 0)
	{
		return;
	}
	SlotIndex = InSlotIndex;
	bRegistered = true;
	ReceiveConsumableRegistered();
}

void UARConsumableInstance::UnregisterFromSlot(EARConsumableRemovalReason Reason)
{
	if (!bRegistered)
	{
		return;
	}
	ReceiveConsumableUnregistered(Reason);
	bRegistered = false;
	SlotIndex = INDEX_NONE;
}

bool UARConsumableInstance::CanUseConsumable_Implementation(FGameplayTag& FailureTag) const
{
	FailureTag = FGameplayTag();
	return bRegistered && ConsumableOwner.IsValid();
}
