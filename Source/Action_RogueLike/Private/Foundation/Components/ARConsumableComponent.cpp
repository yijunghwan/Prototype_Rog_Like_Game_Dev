#include "Foundation/Components/ARConsumableComponent.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Items/ARConsumableDefinition.h"
#include "Foundation/Items/ARConsumableInstance.h"

UARConsumableComponent::UARConsumableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	const int32 SlotIndex = Slots.IndexOfByPredicate([](const UARConsumableInstance* Instance) { return Instance == nullptr; });
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
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
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
	DropRequest.Definition = Slots[SlotIndex]->GetConsumableDefinition();
	DropRequest.SuggestedLocation = GetOwner()->GetActorLocation();
	DropRequest.PreviousSlotIndex = SlotIndex;
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

TArray<FARConsumableSlotSnapshot> UARConsumableComponent::GetConsumableSlots() const
{
	TArray<FARConsumableSlotSnapshot> Result;
	Result.Reserve(Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		FARConsumableSlotSnapshot& Snapshot = Result.AddDefaulted_GetRef();
		Snapshot.SlotIndex = Index;
		Snapshot.bOccupied = Slots[Index] != nullptr;
		if (Slots[Index])
		{
			Snapshot.InstanceId = Slots[Index]->GetInstanceId();
			Snapshot.Definition = Slots[Index]->GetConsumableDefinition();
		}
	}
	return Result;
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
	const int32 DesiredCount = StatsComponent
		? FMath::Max(0, FMath::FloorToInt(StatsComponent->GetFinalStat(EARStatType::MaxConsumableSlots)))
		: 0;
	if (DesiredCount == Slots.Num())
	{
		return;
	}
	if (DesiredCount < Slots.Num())
	{
		for (int32 Index = Slots.Num() - 1; Index >= DesiredCount; --Index)
		{
			if (Slots[Index])
			{
				FARConsumableDropRequest DropRequest;
				DropRequest.Definition = Slots[Index]->GetConsumableDefinition();
				DropRequest.SuggestedLocation = GetOwner()->GetActorLocation();
				DropRequest.PreviousSlotIndex = Index;
				DropRequest.bCausedBySlotReduction = true;
				RemoveSlotInstance(Index, EARConsumableRemovalReason::SlotReduced);
				OnConsumableDropRequested.Broadcast(DropRequest);
			}
		}
	}
	Slots.SetNum(DesiredCount);
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

void UARConsumableComponent::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (Target == GetOwner() && StatType == EARStatType::MaxConsumableSlots)
	{
		SynchronizeSlotCount();
	}
}
