#include "Foundation/Items/ARLoadoutItemInstance.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Items/ARLoadoutItemDefinition.h"

UWorld* UARLoadoutItemInstance::GetWorld() const
{
	return ItemOwner.IsValid() ? ItemOwner->GetWorld() : nullptr;
}

void UARLoadoutItemInstance::InitializeInstance(AARPlayerCharacter* InOwner, const UARLoadoutItemDefinition* InDefinition)
{
	ItemOwner = InOwner;
	Definition = InDefinition;
	InstanceId = FGuid::NewGuid();
}

bool UARLoadoutItemInstance::RegisterItem()
{
	if (bRegistered || !ItemOwner.IsValid() || !Definition)
	{
		return false;
	}
	bRegistered = true;
	for (const FARStatModifierSpec& DefaultSpec : Definition->DefaultStatModifiers)
	{
		FARStatModifierSpec Spec = DefaultSpec;
		Spec.Duration = -1.0f;
		bool bSuccess = false;
		ApplyItemStatModifier(Spec, bSuccess);
		if (!bSuccess)
		{
			UE_LOG(LogARItems, Error, TEXT("Failed to apply default modifier for %s; registration rolled back."), *GetNameSafe(Definition));
			RemoveAllOwnItemModifiers();
			bRegistered = false;
			return false;
		}
	}
	ReceiveItemRegistered();
	return true;
}

void UARLoadoutItemInstance::UnregisterItem(EARItemRemovalReason Reason)
{
	if (!bRegistered)
	{
		return;
	}
	ReceiveItemUnregistered(Reason);
	RemoveAllOwnItemModifiers();
	for (const TPair<FGameplayTag, FARItemUIState>& Pair : UIStates)
	{
		OnItemUIStateChanged.Broadcast(InstanceId, Pair.Value, true);
	}
	UIStates.Reset();
	bRegistered = false;
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
	FARStatModifierSpec OwnedSpec = Spec;
	OwnedSpec.Source = MakeOwnedSource(Spec.Source);
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
	const bool bRemoved = ItemOwner->GetStatsComponent()->RemoveStatModifier(Handle);
	if (bRemoved)
	{
		OwnModifierHandles.Remove(Handle);
	}
	return bRemoved;
}

int32 UARLoadoutItemInstance::RemoveAllOwnItemModifiers()
{
	int32 Removed = 0;
	if (ItemOwner.IsValid())
	{
		for (const FARStatModifierHandle& Handle : OwnModifierHandles)
		{
			Removed += ItemOwner->GetStatsComponent()->RemoveStatModifier(Handle) ? 1 : 0;
		}
	}
	OwnModifierHandles.Reset();
	return Removed;
}

bool UARLoadoutItemInstance::SetItemUIState(FGameplayTag StateId, float CurrentValue, float MaximumValue)
{
	if (!Definition || !StateId.IsValid() || !FMath::IsFinite(CurrentValue) || !FMath::IsFinite(MaximumValue))
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
	OnItemUIStateChanged.Broadcast(InstanceId, State, false);
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
