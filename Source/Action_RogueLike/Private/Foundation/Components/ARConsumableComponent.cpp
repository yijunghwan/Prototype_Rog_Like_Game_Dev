#include "Foundation/Components/ARConsumableComponent.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Interaction/ARItemPickupActors.h"
#include "Foundation/Interaction/ARWorldItemDropSubsystem.h"
#include "Foundation/Items/ARConsumableDefinition.h"
#include "Foundation/Items/ARConsumableInstance.h"

UARConsumableComponent::UARConsumableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DroppedConsumablePickupClass = AARConsumablePickup::StaticClass();
}

void UARConsumableComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerOwner = Cast<AARPlayerCharacter>(GetOwner());
	if (!PlayerOwner)
	{
		UE_LOG(LogARItems, Error, TEXT("ConsumableComponent requires AARPlayerCharacter owner."));
		return;
	}
	StatsComponent = PlayerOwner->GetStatsComponent();
	if (StatsComponent)
	{
		StatsComponent->OnFinalStatChanged.AddDynamic(this, &UARConsumableComponent::HandleFinalStatChanged);
	}
	SynchronizeSlotCount();
}

void UARConsumableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatsComponent)
	{
		StatsComponent->OnFinalStatChanged.RemoveDynamic(this, &UARConsumableComponent::HandleFinalStatChanged);
	}
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		RemoveSlotInstance(Index, EARConsumableRemovalReason::OwnerDestroyed);
	}
	Slots.Reset();
	Super::EndPlay(EndPlayReason);
}

FARConsumableAcquisitionResult UARConsumableComponent::TryAcquireConsumable(UARConsumableDefinition* Definition)
{
	FARConsumableAcquisitionResult Result;
	if (!ValidateDefinition(Definition, Result.Status))
	{
		return Result;
	}
	int32 SlotIndex = INDEX_NONE;
	for (int32 Index = 0; Index < FMath::Min(DesiredSlotCount, Slots.Num()); ++Index)
	{
		if (!Slots[Index])
		{
			SlotIndex = Index;
			break;
		}
	}
	if (SlotIndex == INDEX_NONE)
	{
		Result.Status.Result = EARRequestResult::SlotFull;
		return Result;
	}
	UARConsumableInstance* Instance = NewObject<UARConsumableInstance>(this, Definition->RuntimeBehaviorClass);
	if (!Instance)
	{
		Result.Status.Result = EARRequestResult::Rejected;
		return Result;
	}
	Instance->InitializeInstance(PlayerOwner, Definition);
	Instance->RegisterInSlot(SlotIndex);
	if (!Instance->IsRegistered())
	{
		Result.Status.Result = EARRequestResult::Rejected;
		return Result;
	}
	Slots[SlotIndex] = Instance;
	Result.SlotIndex = SlotIndex;
	Result.InstanceId = Instance->GetInstanceId();
	Result.Status.Result = EARRequestResult::Success;
	BroadcastSlotsChanged();
	return Result;
}

FARRequestStatus UARConsumableComponent::TryUseConsumableSlot(int32 SlotIndex, UARConsumableDefinition*& UsedDefinition)
{
	FARRequestStatus Status;
	UsedDefinition = nullptr;
	if (!PlayerOwner || PlayerOwner->IsGameplayInputBlocked())
	{
		Status.Result = EARRequestResult::Blocked;
		return Status;
	}
	if (!Slots.IsValidIndex(SlotIndex) || SlotIndex >= DesiredSlotCount || !Slots[SlotIndex])
	{
		Status.Result = EARRequestResult::InvalidHandle;
		return Status;
	}
	UARConsumableInstance* Instance = Slots[SlotIndex];
	FGameplayTag FailureTag;
	if (!Instance->CanUseConsumable(FailureTag))
	{
		Status.Result = EARRequestResult::Rejected;
		return Status;
	}
	UsedDefinition = Instance->GetConsumableDefinition();
	Instance->ExecuteConsumableUse();
	RemoveSlotInstance(SlotIndex, EARConsumableRemovalReason::Used);
	Status.Result = EARRequestResult::Success;
	BroadcastSlotsChanged();
	return Status;
}

FARRequestStatus UARConsumableComponent::DropConsumableSlot(int32 SlotIndex, FARConsumableDropRequest& DropRequest)
{
	FARRequestStatus Status;
	DropRequest = FARConsumableDropRequest();
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		Status.Result = EARRequestResult::InvalidHandle;
		return Status;
	}
	if (!TrySpawnDropForSlot(SlotIndex, false, DropRequest))
	{
		Status.Result = EARRequestResult::Rejected;
		return Status;
	}
	RemoveSlotInstance(SlotIndex, EARConsumableRemovalReason::Dropped);
	Status.Result = EARRequestResult::Success;
	BroadcastSlotsChanged();
	return Status;
}

void UARConsumableComponent::SetBaseMaxConsumableSlots(int32 NewSlotCount)
{
	if (StatsComponent)
	{
		StatsComponent->SetBaseStat(EARStatType::MaxConsumableSlots, FMath::Max(0, NewSlotCount));
	}
}

void UARConsumableComponent::RetryPendingOverflowDrops()
{
	SynchronizeSlotCount();
}

TArray<FARConsumableSlotSnapshot> UARConsumableComponent::GetConsumableSlots() const
{
	TArray<FARConsumableSlotSnapshot> Result;
	Result.Reserve(Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		FARConsumableSlotSnapshot& Snapshot = Result.AddDefaulted_GetRef();
		Snapshot.SlotIndex = Index;
		Snapshot.bOccupied = Slots[Index] != nullptr;
		Snapshot.bOverflowSlot = Index >= DesiredSlotCount;
		if (Slots[Index])
		{
			Snapshot.InstanceId = Slots[Index]->GetInstanceId();
			Snapshot.Definition = Slots[Index]->GetConsumableDefinition();
		}
	}
	return Result;
}

bool UARConsumableComponent::GetConsumableSlotDisplayData(int32 SlotIndex, FARConsumableDisplayData& DisplayData) const
{
	DisplayData = FARConsumableDisplayData();
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex]
		|| !GetConsumableDefinitionDisplayData(Slots[SlotIndex]->GetConsumableDefinition(), DisplayData))
	{
		return false;
	}
	DisplayData.SlotIndex = SlotIndex;
	DisplayData.InstanceId = Slots[SlotIndex]->GetInstanceId();
	return true;
}

bool UARConsumableComponent::GetConsumableDefinitionDisplayData(const UARConsumableDefinition* Definition, FARConsumableDisplayData& DisplayData) const
{
	DisplayData = FARConsumableDisplayData();
	if (!Definition)
	{
		return false;
	}
	DisplayData.Definition = Definition;
	DisplayData.DefinitionTag = Definition->DefinitionTag;
	DisplayData.ItemTypeTag = Definition->ItemTypeTag;
	DisplayData.DisplayName = Definition->DisplayName;
	DisplayData.ShortDescription = Definition->ShortDescription;
	DisplayData.DetailedDescription = Definition->DetailedDescription;
	DisplayData.Icon = Definition->Icon;
	return true;
}

bool UARConsumableComponent::ValidateDefinition(const UARConsumableDefinition* Definition, FARRequestStatus& Status) const
{
	if (!PlayerOwner)
	{
		Status.Result = EARRequestResult::InvalidOwner;
		return false;
	}
	if (!Definition || !Definition->DefinitionTag.IsValid() || !Definition->RuntimeBehaviorClass
		|| Definition->RuntimeBehaviorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return false;
	}
	Status.Result = EARRequestResult::Success;
	return true;
}

void UARConsumableComponent::SynchronizeSlotCount()
{
	DesiredSlotCount = StatsComponent
		? FMath::Max(0, FMath::FloorToInt(StatsComponent->GetFinalStat(EARStatType::MaxConsumableSlots)))
		: 0;
	if (DesiredSlotCount == Slots.Num())
	{
		return;
	}
	if (DesiredSlotCount < Slots.Num())
	{
		int32 RequiredStorageCount = DesiredSlotCount;
		for (int32 Index = Slots.Num() - 1; Index >= DesiredSlotCount; --Index)
		{
			if (Slots[Index])
			{
				FARConsumableDropRequest DropRequest;
				if (TrySpawnDropForSlot(Index, true, DropRequest))
				{
					RemoveSlotInstance(Index, EARConsumableRemovalReason::SlotReduced);
					OnConsumableDropRequested.Broadcast(DropRequest);
				}
				else
				{
					RequiredStorageCount = FMath::Max(RequiredStorageCount, Index + 1);
					UE_LOG(LogARItems, Error, TEXT("Consumable overflow drop failed at slot %d. Item retained for retry."), Index);
				}
			}
		}
		Slots.SetNum(RequiredStorageCount);
	}
	else
	{
		Slots.SetNum(DesiredSlotCount);
	}
	BroadcastSlotsChanged();
}

void UARConsumableComponent::BroadcastSlotsChanged()
{
	OnConsumableSlotsChanged.Broadcast(GetConsumableSlots());
}

void UARConsumableComponent::RemoveSlotInstance(int32 SlotIndex, EARConsumableRemovalReason Reason)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return;
	}
	Slots[SlotIndex]->UnregisterFromSlot(Reason);
	Slots[SlotIndex] = nullptr;
}

bool UARConsumableComponent::TrySpawnDropForSlot(int32 SlotIndex, bool bCausedBySlotReduction, FARConsumableDropRequest& DropRequest)
{
	DropRequest = FARConsumableDropRequest();
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex] || !GetOwner())
	{
		return false;
	}
	DropRequest.Definition = Slots[SlotIndex]->GetConsumableDefinition();
	DropRequest.SuggestedLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * DropForwardDistance;
	DropRequest.PreviousSlotIndex = SlotIndex;
	DropRequest.bCausedBySlotReduction = bCausedBySlotReduction;
	UARWorldItemDropSubsystem* DropSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UARWorldItemDropSubsystem>() : nullptr;
	AARConsumablePickup* SpawnedPickup = nullptr;
	if (!DropSubsystem || !DropSubsystem->TrySpawnConsumablePickup(
		DropRequest.Definition, DropRequest.SuggestedLocation, DroppedConsumablePickupClass, GetOwner(), SpawnedPickup))
	{
		return false;
	}
	DropRequest.SpawnedPickup = SpawnedPickup;
	DropRequest.SuggestedLocation = SpawnedPickup->GetActorLocation();
	return true;
}

void UARConsumableComponent::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (Target == GetOwner() && StatType == EARStatType::MaxConsumableSlots)
	{
		SynchronizeSlotCount();
	}
}
