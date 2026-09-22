#include "Foundation/Components/ARActionComponent.h"

#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Core/ARLogChannels.h"

UARActionComponent::UARActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARActionComponent::BeginPlay()
{
	Super::BeginPlay();
	StatsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStatsComponent>() : nullptr;
	StaggerComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStaggerComponent>() : nullptr;
	MovementComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARMovementControlComponent>() : nullptr;
}

FARRequestStatus UARActionComponent::CanStartAction(const FARActionRequest& Request) const
{
	FARRequestStatus Status;
	if (!GetOwner())
	{
		Status.Result = EARRequestResult::InvalidOwner;
		return Status;
	}
	if (!Request.ActionTag.IsValid())
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return Status;
	}
	if (const UARStatusEffectComponent* StatusComponent = GetOwner()->FindComponentByClass<UARStatusEffectComponent>())
	{
		if (StatusComponent->BlocksSkillGroups() || (Request.bIsRollAction && StatusComponent->BlocksRoll()))
		{
			Status.Result = EARRequestResult::Blocked;
			return Status;
		}
	}
	if (Request.bIsRollAction && IsRollBlocked())
	{
		Status.Result = EARRequestResult::Blocked;
		return Status;
	}
	Status.Result = EARRequestResult::Success;
	return Status;
}

FARActionHandle UARActionComponent::TryStartAction(const FARActionRequest& Request, FARRequestStatus& Status)
{
	Status = CanStartAction(Request);
	FARActionHandle Handle;
	if (!Status.IsSuccess())
	{
		return Handle;
	}
	Handle.Id = FGuid::NewGuid();
	Handle.Owner = this;
	Handle.Serial = NextActionSerial++;
	FARActiveAction& Action = ActiveActions.AddDefaulted_GetRef();
	Action.Handle = Handle;
	Action.Request = Request;
	if (Request.bBlockBasicMovementWhileActive && MovementComponent)
	{
		Action.MovementLockHandles.Add(MovementComponent->AcquireMovementLock(*FString::Printf(TEXT("Action.%lld"), Handle.Serial), EARMovementLockType::BasicMovementOnly));
	}
	return Handle;
}

bool UARActionComponent::EndAction(FARActionHandle Handle)
{
	const int32 Index = ActiveActions.IndexOfByPredicate([&Handle](const FARActiveAction& Action) { return Action.Handle == Handle; });
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogARAction, Warning, TEXT("EndAction received a stale or foreign handle."));
		return false;
	}
	FARActiveAction Action = MoveTemp(ActiveActions[Index]);
	ActiveActions.RemoveAt(Index);
	CleanupAction(Action);
	OnActionEnded.Broadcast(Handle);
	return true;
}

bool UARActionComponent::CancelAction(FARActionHandle Handle, EARActionCancelReason Reason)
{
	const int32 Index = ActiveActions.IndexOfByPredicate([&Handle](const FARActiveAction& Action) { return Action.Handle == Handle; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	FARActiveAction Action = MoveTemp(ActiveActions[Index]);
	ActiveActions.RemoveAt(Index);
	CleanupAction(Action);
	OnActionCancelled.Broadcast(Handle, Reason);
	return true;
}

int32 UARActionComponent::CancelActionsByReason(EARActionCancelReason Reason)
{
	TArray<FARActionHandle> Handles;
	for (const FARActiveAction& Action : ActiveActions)
	{
		if (ShouldCancelForReason(Action, Reason))
		{
			Handles.Add(Action.Handle);
		}
	}
	for (const FARActionHandle& Handle : Handles)
	{
		CancelAction(Handle, Reason);
	}
	return Handles.Num();
}

int32 UARActionComponent::CancelRollActions(EARActionCancelReason Reason)
{
	TArray<FARActionHandle> Handles;
	for (const FARActiveAction& Action : ActiveActions)
	{
		if (Action.Request.bIsRollAction)
		{
			Handles.Add(Action.Handle);
		}
	}
	for (const FARActionHandle& Handle : Handles)
	{
		CancelAction(Handle, Reason);
	}
	return Handles.Num();
}

int32 UARActionComponent::CancelAllActions(EARActionCancelReason Reason)
{
	TArray<FARActionHandle> Handles;
	for (const FARActiveAction& Action : ActiveActions)
	{
		Handles.Add(Action.Handle);
	}
	for (const FARActionHandle& Handle : Handles)
	{
		CancelAction(Handle, Reason);
	}
	return Handles.Num();
}

int32 UARActionComponent::CancelActionsByItemInstance(FGuid ItemInstanceId, EARActionCancelReason Reason)
{
	TArray<FARActionHandle> Handles;
	for (const FARActiveAction& Action : ActiveActions)
	{
		if (Action.Request.OwningItemInstanceId == ItemInstanceId)
		{
			Handles.Add(Action.Handle);
		}
	}
	for (const FARActionHandle& Handle : Handles)
	{
		CancelAction(Handle, Reason);
	}
	return Handles.Num();
}

bool UARActionComponent::SetActionCancelRules(FARActionHandle Handle, const FARActionCancelRules& Rules)
{
	if (FARActiveAction* Action = FindActiveAction(Handle))
	{
		Action->Request.CancelRules = Rules;
		return true;
	}
	return false;
}

bool UARActionComponent::SetActionRollBlocked(FARActionHandle Handle, bool bBlocked)
{
	if (FARActiveAction* Action = FindActiveAction(Handle))
	{
		Action->Request.bBlockRollWhileActive = bBlocked;
		return true;
	}
	return false;
}

bool UARActionComponent::SetActionBasicMovementBlocked(FARActionHandle Handle, bool bBlocked)
{
	FARActiveAction* Action = FindActiveAction(Handle);
	if (!Action || !MovementComponent || Action->Request.bBlockBasicMovementWhileActive == bBlocked)
	{
		return Action != nullptr;
	}
	Action->Request.bBlockBasicMovementWhileActive = bBlocked;
	if (bBlocked)
	{
		Action->MovementLockHandles.Add(MovementComponent->AcquireMovementLock(*FString::Printf(TEXT("Action.%lld"), Handle.Serial), EARMovementLockType::BasicMovementOnly));
	}
	else
	{
		for (const FARMovementLockHandle& Lock : Action->MovementLockHandles)
		{
			MovementComponent->ReleaseMovementLock(Lock);
		}
		Action->MovementLockHandles.Reset();
	}
	return true;
}

bool UARActionComponent::RegisterActionHitbox(FARActionHandle Handle, AActor* HitboxActor)
{
	if (!IsValid(HitboxActor))
	{
		return false;
	}
	if (FARActiveAction* Action = FindActiveAction(Handle))
	{
		Action->HitboxActors.Add(HitboxActor);
		return true;
	}
	return false;
}

FARStatModifierHandle UARActionComponent::ApplyActionStatModifier(FARActionHandle Handle, const FARStatModifierSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	FARActiveAction* Action = FindActiveAction(Handle);
	if (!Action || !StatsComponent)
	{
		return FARStatModifierHandle();
	}
	FARStatModifierHandle Modifier = StatsComponent->AddStatModifier(Spec, bSuccess);
	if (bSuccess)
	{
		Action->StatModifiers.Add(Modifier);
	}
	return Modifier;
}

FARSuperArmorHandle UARActionComponent::ApplyActionSuperArmor(FARActionHandle Handle, const FARSuperArmorSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	FARActiveAction* Action = FindActiveAction(Handle);
	if (!Action || !StaggerComponent)
	{
		return FARSuperArmorHandle();
	}
	FARSuperArmorHandle Armor = StaggerComponent->AddSuperArmor(Spec, bSuccess);
	if (bSuccess)
	{
		Action->SuperArmorHandles.Add(Armor);
	}
	return Armor;
}

bool UARActionComponent::IsActionActive(FARActionHandle Handle) const
{
	return FindActiveAction(Handle) != nullptr;
}

bool UARActionComponent::IsRollBlocked() const
{
	return ActiveActions.ContainsByPredicate([](const FARActiveAction& Action) { return Action.Request.bBlockRollWhileActive; });
}

bool UARActionComponent::IsBasicMovementBlocked() const
{
	return ActiveActions.ContainsByPredicate([](const FARActiveAction& Action) { return Action.Request.bBlockBasicMovementWhileActive; });
}

FARActiveAction* UARActionComponent::FindActiveAction(FARActionHandle Handle)
{
	return ActiveActions.FindByPredicate([&Handle](const FARActiveAction& Action) { return Action.Handle == Handle; });
}

const FARActiveAction* UARActionComponent::FindActiveAction(FARActionHandle Handle) const
{
	return ActiveActions.FindByPredicate([&Handle](const FARActiveAction& Action) { return Action.Handle == Handle; });
}

void UARActionComponent::CleanupAction(FARActiveAction& Action)
{
	if (StatsComponent)
	{
		for (const FARStatModifierHandle& Modifier : Action.StatModifiers)
		{
			StatsComponent->RemoveStatModifier(Modifier);
		}
	}
	if (StaggerComponent)
	{
		for (const FARSuperArmorHandle& Armor : Action.SuperArmorHandles)
		{
			StaggerComponent->RemoveSuperArmor(Armor);
		}
	}
	if (MovementComponent)
	{
		for (const FARMovementLockHandle& Lock : Action.MovementLockHandles)
		{
			MovementComponent->ReleaseMovementLock(Lock);
		}
	}
	for (const TWeakObjectPtr<AActor>& Hitbox : Action.HitboxActors)
	{
		if (Hitbox.IsValid())
		{
			Hitbox->Destroy();
		}
	}
}

bool UARActionComponent::ShouldCancelForReason(const FARActiveAction& Action, EARActionCancelReason Reason) const
{
	switch (Reason)
	{
	case EARActionCancelReason::Stagger: return Action.Request.CancelRules.bCancelOnStagger;
	case EARActionCancelReason::Stun: return Action.Request.CancelRules.bCancelOnStun;
	case EARActionCancelReason::Roll: return Action.Request.CancelRules.bCancelOnRoll;
	case EARActionCancelReason::BasicMovementInput: return Action.Request.CancelRules.bCancelOnBasicMovementInput;
	case EARActionCancelReason::Root: return Action.Request.bIsRollAction;
	default: return true;
	}
}
