#include "Foundation/Components/ARMovementControlComponent.h"

#include "GameFramework/Character.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Core/ARLogChannels.h"

UARMovementControlComponent::UARMovementControlComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARMovementControlComponent::BeginPlay()
{
	Super::BeginPlay();
	CharacterOwner = Cast<ACharacter>(GetOwner());
	CharacterMovement = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	ActionComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARActionComponent>() : nullptr;
	if (!CharacterOwner || !CharacterMovement)
	{
		UE_LOG(LogARFoundation, Error, TEXT("MovementControlComponent requires an ACharacter owner: %s"), *GetNameSafe(GetOwner()));
	}
}

bool UARMovementControlComponent::RequestBasicMove(FVector Direction, float Scale)
{
	if (!CharacterOwner || !CanBasicMove() || Direction.IsNearlyZero() || FMath::IsNearlyZero(Scale))
	{
		return false;
	}
	Direction.Z = 0.0f;
	CharacterOwner->AddMovementInput(Direction.GetSafeNormal(), Scale);
	return true;
}

bool UARMovementControlComponent::RequestActionMove(FARActionHandle ActionHandle, FVector Direction, float Scale)
{
	if (!IsActiveOwnedAction(ActionHandle) || !CharacterOwner || !CanMoveAtAll() || Direction.IsNearlyZero() || FMath::IsNearlyZero(Scale))
	{
		return false;
	}
	Direction.Z = 0.0f;
	CharacterOwner->AddMovementInput(Direction.GetSafeNormal(), Scale, true);
	return true;
}

bool UARMovementControlComponent::RequestActionVelocity(FARActionHandle ActionHandle, FVector Direction, float Speed)
{
	if (!IsActiveOwnedAction(ActionHandle) || !CharacterMovement || !CanMoveAtAll() || Direction.IsNearlyZero() || !FMath::IsFinite(Speed) || Speed <= 0.0f)
	{
		return false;
	}
	Direction.Z = 0.0f;
	CharacterMovement->Velocity = Direction.GetSafeNormal() * Speed;
	return true;
}

bool UARMovementControlComponent::IsActiveOwnedAction(FARActionHandle ActionHandle) const
{
	return ActionComponent
		&& ActionHandle.IsValid()
		&& ActionHandle.Owner == ActionComponent
		&& ActionComponent->IsActionActive(ActionHandle);
}

FARMovementLockHandle UARMovementControlComponent::AcquireMovementLock(FName SourceId, EARMovementLockType LockType)
{
	const bool bCouldMoveAtAll = CanMoveAtAll();
	FARMovementLockHandle Handle;
	Handle.Id = FGuid::NewGuid();
	FARActiveMovementLock& Lock = ActiveLocks.AddDefaulted_GetRef();
	Lock.Handle = Handle;
	Lock.SourceId = SourceId;
	Lock.LockType = LockType;
	RefreshMovementMode(bCouldMoveAtAll);
	OnMovementLockChangedNative.Broadcast(GetOwner(), CanBasicMove(), CanMoveAtAll());
	OnMovementLockChanged.Broadcast(GetOwner(), CanBasicMove(), CanMoveAtAll());
	return Handle;
}

bool UARMovementControlComponent::ReleaseMovementLock(FARMovementLockHandle Handle)
{
	const bool bCouldMoveAtAll = CanMoveAtAll();
	const int32 Removed = ActiveLocks.RemoveAll([&Handle](const FARActiveMovementLock& Lock)
	{
		return Lock.Handle == Handle;
	});
	if (Removed > 0)
	{
		RefreshMovementMode(bCouldMoveAtAll);
		OnMovementLockChangedNative.Broadcast(GetOwner(), CanBasicMove(), CanMoveAtAll());
		OnMovementLockChanged.Broadcast(GetOwner(), CanBasicMove(), CanMoveAtAll());
	}
	return Removed > 0;
}

int32 UARMovementControlComponent::ReleaseMovementLocksBySource(FName SourceId)
{
	const bool bCouldMoveAtAll = CanMoveAtAll();
	const int32 Removed = ActiveLocks.RemoveAll([SourceId](const FARActiveMovementLock& Lock)
	{
		return SourceId.IsNone() || Lock.SourceId == SourceId;
	});
	if (Removed > 0)
	{
		RefreshMovementMode(bCouldMoveAtAll);
		OnMovementLockChangedNative.Broadcast(GetOwner(), CanBasicMove(), CanMoveAtAll());
		OnMovementLockChanged.Broadcast(GetOwner(), CanBasicMove(), CanMoveAtAll());
	}
	return Removed;
}

bool UARMovementControlComponent::CanBasicMove() const
{
	return ActiveLocks.IsEmpty();
}

bool UARMovementControlComponent::CanMoveAtAll() const
{
	return !ActiveLocks.ContainsByPredicate([](const FARActiveMovementLock& Lock)
	{
		return Lock.LockType == EARMovementLockType::AllMovement;
	});
}

void UARMovementControlComponent::StopMovementImmediately()
{
	if (CharacterMovement)
	{
		CharacterMovement->StopMovementImmediately();
	}
}

void UARMovementControlComponent::RefreshMovementMode(bool bCouldMoveAtAll)
{
	if (!CharacterMovement)
	{
		return;
	}
	const bool bCanMoveNow = CanMoveAtAll();
	if (bCouldMoveAtAll && !bCanMoveNow)
	{
		SavedMovementMode = CharacterMovement->MovementMode;
		SavedCustomMovementMode = CharacterMovement->CustomMovementMode;
		CharacterMovement->StopMovementImmediately();
		CharacterMovement->DisableMovement();
	}
	else if (!bCouldMoveAtAll && bCanMoveNow)
	{
		const EMovementMode RestoreMode = SavedMovementMode == MOVE_None ? MOVE_Walking : SavedMovementMode;
		CharacterMovement->SetMovementMode(RestoreMode, SavedCustomMovementMode);
	}
}
