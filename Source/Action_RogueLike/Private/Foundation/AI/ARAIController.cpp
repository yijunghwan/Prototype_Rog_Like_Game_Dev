#include "Foundation/AI/ARAIController.h"

#include "Foundation/Components/ARMovementControlComponent.h"
#include "Navigation/PathFollowingComponent.h"

void AARAIController::OnPossess(APawn* InPawn)
{
	UnbindMovementControl();
	Super::OnPossess(InPawn);
	MovementControlComponent = InPawn ? InPawn->FindComponentByClass<UARMovementControlComponent>() : nullptr;
	if (MovementControlComponent)
	{
		MovementControlComponent->OnMovementLockChangedNative.AddUObject(this, &AARAIController::HandleMovementLockChanged);
		if (!MovementControlComponent->CanBasicMove())
		{
			StopMovement();
		}
	}
}

void AARAIController::OnUnPossess()
{
	UnbindMovementControl();
	Super::OnUnPossess();
}

void AARAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMovementControl();
	Super::EndPlay(EndPlayReason);
}

void AARAIController::UnbindMovementControl()
{
	if (MovementControlComponent)
	{
		MovementControlComponent->OnMovementLockChangedNative.RemoveAll(this);
		MovementControlComponent = nullptr;
	}
}

bool AARAIController::CanRequestBasicMove() const
{
	return IsValid(GetPawn()) && IsValid(MovementControlComponent) && MovementControlComponent->CanBasicMove();
}

FPathFollowingRequestResult AARAIController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	if (!CanRequestBasicMove())
	{
		return FPathFollowingRequestResult();
	}
	return Super::MoveTo(MoveRequest, OutPath);
}

EPathFollowingRequestResult::Type AARAIController::ARMoveToActor(AActor* Goal, float AcceptanceRadius)
{
	return CanRequestBasicMove() && IsValid(Goal)
		? MoveToActor(Goal, AcceptanceRadius)
		: EPathFollowingRequestResult::Failed;
}

EPathFollowingRequestResult::Type AARAIController::ARMoveToLocation(FVector Destination, float AcceptanceRadius)
{
	return CanRequestBasicMove() && !Destination.ContainsNaN()
		? MoveToLocation(Destination, AcceptanceRadius)
		: EPathFollowingRequestResult::Failed;
}

void AARAIController::HandleMovementLockChanged(AActor* Target, bool bCanBasicMove, bool bCanMoveAtAll)
{
	if (Target == GetPawn() && !bCanBasicMove)
	{
		StopMovement();
		MovementControlComponent->StopMovementImmediately();
	}
}
