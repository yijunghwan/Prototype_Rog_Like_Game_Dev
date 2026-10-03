#include "Foundation/Items/ARLoadoutItemInstance.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "Foundation/Actions/ARLifetimeTimer.h"

UWorld* UARLoadoutItemInstance::GetWorld() const
{
	return ItemOwner.IsValid() ? ItemOwner->GetWorld() : nullptr;
}

void UARLoadoutItemInstance::InitializeInstance(AARPlayerCharacter* InOwner, const UARItemDefinition* InDefinition)
{
	ItemOwner = InOwner;
	Definition = InDefinition;
	InstanceId = FGuid::NewGuid();
}

bool UARLoadoutItemInstance::RegisterItem()
{
	if (bRegistered || bUnregistering || !ItemOwner.IsValid() || ItemOwner->IsActorBeingDestroyed() || !Definition)
	{
		return false;
	}
	bRegistered = true;
	const UARStatsComponent::FScopedNotifications Notifications(ItemOwner->GetStatsComponent());
	TMap<FName, FARStatModifierHandle> DefaultGroups;
	for (const FARStatModifierSpec& DefaultSpec : Definition->DefaultStatModifiers)
	{
		FARStatModifierSpec Spec = DefaultSpec;
		Spec.Duration = -1.0f;
		Spec.StackGroupHandle = FARStatModifierHandle();
		if (const FARStatModifierHandle* Existing = DefaultGroups.Find(Spec.Source.SourceId))
		{
			Spec.StackGroupHandle = *Existing;
		}
		bool bSuccess = false;
		const FARStatModifierHandle Handle = ApplyItemStatModifier(Spec, bSuccess);
		if (!bSuccess)
		{
			UE_LOG(LogARItems, Error, TEXT("Failed to apply default modifier for %s; registration rolled back."), *GetNameSafe(Definition));
			RemoveAllOwnItemModifiers();
			bRegistered = false;
			return false;
		}
		if (!Spec.Source.SourceId.IsNone() && !DefaultGroups.Contains(Spec.Source.SourceId))
		{
			DefaultGroups.Add(Spec.Source.SourceId, Handle);
		}
	}
	ReceiveItemRegistered();
	return bRegistered;
}

void UARLoadoutItemInstance::UnregisterItem(EARItemRemovalReason Reason)
{
	if (!bRegistered || bUnregistering)
	{
		return;
	}
	TGuardValue<bool> UnregisterGuard(bUnregistering, true);
	bRegistered = false; // Prevent recursive unregister/new effects/timers during cleanup callbacks.
	ARLifetimeTimer::Clear(GetWorld(), OwnTimerHandles);
	const UARStatsComponent::FScopedNotifications Notifications(ItemOwner.IsValid() ? ItemOwner->GetStatsComponent() : nullptr);
	ReceiveItemUnregistered(Reason);
	if (ItemOwner.IsValid())
	{
		if (UARStaggerComponent* Stagger = ItemOwner->GetStaggerComponent())
		{
			for (const FARSuperArmorHandle& Handle : OwnSuperArmorHandles)
			{
				Stagger->RemoveSuperArmor(Handle);
			}
		}
		if (UARStatusEffectComponent* Status = ItemOwner->GetStatusEffectComponent())
		{
			for (const FARCCImmunityHandle& Handle : OwnCCImmunityHandles)
			{
				Status->RemoveCCImmunity(Handle);
			}
		}
	}
	OwnSuperArmorHandles.Reset();
	OwnCCImmunityHandles.Reset();
	RemoveAllOwnItemModifiers();
	const TMap<FGameplayTag, FARItemUIState> RemovedUIStates = MoveTemp(UIStates);
	UIStates.Reset();
	for (const TPair<FGameplayTag, FARItemUIState>& Pair : RemovedUIStates)
	{
		OnItemUIStateChanged.Broadcast(InstanceId, Pair.Value, true);
	}
}

bool UARLoadoutItemInstance::CanExecuteItemSkill_Implementation(FName SkillId, FGameplayTag& FailureTag) const
{
	FailureTag = FGameplayTag();
	return bRegistered;
}

FARStatModifierHandle UARLoadoutItemInstance::ApplyItemStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	if (!bRegistered || !ItemOwner.IsValid())
	{
		return FARStatModifierHandle();
	}
	UARStatsComponent* Stats = ItemOwner->GetStatsComponent();
	if (!Stats)
	{
		return FARStatModifierHandle();
	}
	const UARStatsComponent::FScopedNotifications Notifications(Stats);
	FARStatModifierSpec OwnedSpec = Spec;
	OwnedSpec.Source = MakeOwnedSource(Spec.Source);
	// Item ownership is tracked by OwnModifierHandles; keep an explicit gameplay Source Id queryable.
	if (!Spec.Source.SourceId.IsNone()) OwnedSpec.Source.SourceId = Spec.Source.SourceId;
	FARStatModifierHandle Handle = Stats->AddStatModifier(OwnedSpec, bSuccess);
	if (bSuccess)
	{
		OwnModifierHandles.Add(Handle);
	}
	return Handle;
}

bool UARLoadoutItemInstance::RemoveOwnItemModifier(FARStatModifierHandle Handle)
{
	if (!OwnModifierHandles.Contains(Handle) || !ItemOwner.IsValid())
	{
		return false;
	}
	// Detach first, so nested cleanup never iterates the same mutable handle list.
	OwnModifierHandles.Remove(Handle);
	const bool bRemoved = ItemOwner->GetStatsComponent()->RemoveStatModifier(Handle);
	return bRemoved;
}

int32 UARLoadoutItemInstance::RemoveAllOwnItemModifiers()
{
	int32 Removed = 0;
	const TArray<FARStatModifierHandle> Handles = MoveTemp(OwnModifierHandles);
	OwnModifierHandles.Reset();
	const UARStatsComponent::FScopedNotifications Notifications(ItemOwner.IsValid() ? ItemOwner->GetStatsComponent() : nullptr);
	if (ItemOwner.IsValid())
	{
		for (const FARStatModifierHandle& Handle : Handles)
		{
			Removed += ItemOwner->GetStatsComponent()->RemoveStatModifier(Handle) ? 1 : 0;
		}
	}
	return Removed;
}

FTimerHandle UARLoadoutItemInstance::SetItemTimerByEvent(FTimerDynamicDelegate Event, float Time, bool bLooping,
	bool& bSuccess, float InitialStartDelay, bool bMaxOncePerFrame)
{
	bSuccess = false;
	if (!bRegistered || bUnregistering || !ItemOwner.IsValid() || ItemOwner->IsActorBeingDestroyed()) return FTimerHandle();
	const TWeakObjectPtr<UARLoadoutItemInstance> WeakOwner(this);
	return ARLifetimeTimer::Start(this, Event, Time, bLooping, InitialStartDelay, bMaxOncePerFrame,
		[WeakOwner]()
		{
			const AARPlayerCharacter* Owner = WeakOwner.IsValid() ? WeakOwner->GetItemOwner() : nullptr;
			return WeakOwner.IsValid() && WeakOwner->IsRegistered() && IsValid(Owner) && !Owner->IsActorBeingDestroyed();
		}, OwnTimerHandles, bSuccess);
}

FARSuperArmorHandle UARLoadoutItemInstance::ApplyItemSuperArmor(const FARSuperArmorSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	if (!bRegistered || !ItemOwner.IsValid() || !ItemOwner->GetStaggerComponent())
	{
		return FARSuperArmorHandle();
	}
	FARSuperArmorSpec OwnedSpec = Spec;
	OwnedSpec.Source = MakeOwnedSource(Spec.Source);
	UARStaggerComponent* Stagger = ItemOwner->GetStaggerComponent();
	OwnSuperArmorHandles.RemoveAll([Stagger](FARSuperArmorHandle Handle)
	{
		return !Stagger->IsSuperArmorHandleActive(Handle);
	});
	const FARSuperArmorHandle Handle = Stagger->AddSuperArmor(OwnedSpec, bSuccess);
	if (bSuccess)
	{
		OwnSuperArmorHandles.Add(Handle);
	}
	return Handle;
}

bool UARLoadoutItemInstance::RemoveOwnItemSuperArmor(FARSuperArmorHandle Handle)
{
	if (!OwnSuperArmorHandles.Contains(Handle) || !ItemOwner.IsValid() || !ItemOwner->GetStaggerComponent())
	{
		return false;
	}
	const bool bRemoved = ItemOwner->GetStaggerComponent()->RemoveSuperArmor(Handle);
	OwnSuperArmorHandles.Remove(Handle);
	return bRemoved;
}

FARCCImmunityHandle UARLoadoutItemInstance::ApplyItemCCImmunity(const FARCCImmunitySpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	if (!bRegistered || !ItemOwner.IsValid() || !ItemOwner->GetStatusEffectComponent())
	{
		return FARCCImmunityHandle();
	}
	FARCCImmunitySpec OwnedSpec = Spec;
	OwnedSpec.Source = MakeOwnedSource(Spec.Source);
	UARStatusEffectComponent* Status = ItemOwner->GetStatusEffectComponent();
	OwnCCImmunityHandles.RemoveAll([Status](FARCCImmunityHandle Handle)
	{
		return !Status->IsCCImmunityHandleActive(Handle);
	});
	const FARCCImmunityHandle Handle = Status->AddCCImmunity(OwnedSpec, bSuccess);
	if (bSuccess)
	{
		OwnCCImmunityHandles.Add(Handle);
	}
	return Handle;
}

bool UARLoadoutItemInstance::RemoveOwnItemCCImmunity(FARCCImmunityHandle Handle)
{
	if (!OwnCCImmunityHandles.Contains(Handle) || !ItemOwner.IsValid() || !ItemOwner->GetStatusEffectComponent())
	{
		return false;
	}
	const bool bRemoved = ItemOwner->GetStatusEffectComponent()->RemoveCCImmunity(Handle);
	OwnCCImmunityHandles.Remove(Handle);
	return bRemoved;
}

bool UARLoadoutItemInstance::SetItemUIState(FGameplayTag StateId, float CurrentValue, float MaximumValue)
{
	if (!bRegistered || !Definition || !StateId.IsValid() || !FMath::IsFinite(CurrentValue) || !FMath::IsFinite(MaximumValue))
	{
		return false;
	}
	const FARUIStateDisplayDefinition* Display = Definition->UIStateDisplayDefinitions.FindByPredicate([StateId](const FARUIStateDisplayDefinition& Candidate)
	{
		return Candidate.StateId == StateId;
	});
	if (!Display)
	{
		UE_LOG(LogARItems, Warning, TEXT("Item %s tried to update unregistered UI state %s."), *GetNameSafe(Definition), *StateId.ToString());
		return false;
	}
	FARItemUIState& State = UIStates.FindOrAdd(StateId);
	State.StateId = StateId;
	State.CurrentValue = CurrentValue;
	State.MaximumValue = MaximumValue;
	State.EffectiveDisplayType = Display->DisplayType;
	if (Display->DisplayType == EARItemUIStateDisplayType::SmallStack && (Display->MaxDisplaySlots > 6 || MaximumValue > 6.0f))
	{
		State.EffectiveDisplayType = EARItemUIStateDisplayType::Number;
		UE_LOG(LogARItems, Warning, TEXT("SmallStack state %s exceeded six slots and was changed to Number."), *StateId.ToString());
	}
	const FARItemUIState StateSnapshot = State;
	OnItemUIStateChanged.Broadcast(InstanceId, StateSnapshot, false);
	return true;
}

bool UARLoadoutItemInstance::RemoveItemUIState(FGameplayTag StateId)
{
	FARItemUIState Removed;
	if (!UIStates.RemoveAndCopyValue(StateId, Removed))
	{
		return false;
	}
	OnItemUIStateChanged.Broadcast(InstanceId, Removed, true);
	return true;
}

TArray<FARItemUIState> UARLoadoutItemInstance::GetItemUIStates() const
{
	TArray<FARItemUIState> Result;
	UIStates.GenerateValueArray(Result);
	return Result;
}

FARSourceInfo UARLoadoutItemInstance::MakeOwnedSource(const FARSourceInfo& RequestedSource) const
{
	FARSourceInfo Source = RequestedSource;
	Source.Category = Definition && Definition->GetItemKind() == EARLoadoutItemKind::Weapon
		? EARModifierSourceCategory::Weapon
		: EARModifierSourceCategory::Relic;
	Source.SourceId = FName(*InstanceId.ToString(EGuidFormats::Digits));
	if (Source.DisplayName.IsEmpty() && Definition)
	{
		Source.DisplayName = Definition->DisplayName;
	}
	return Source;
}
