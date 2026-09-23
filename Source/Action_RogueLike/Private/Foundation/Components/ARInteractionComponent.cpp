#include "Foundation/Components/ARInteractionComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Interfaces/ARInteractableInterface.h"

namespace
{
	constexpr ECollisionChannel ARInteractionTraceChannel = ECC_GameTraceChannel3;
}

UARInteractionComponent::UARInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UARInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerOwner = Cast<AARPlayerCharacter>(GetOwner());
	RefreshInteractionCandidate();
}

void UARInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ScanAccumulator += DeltaTime;
	if (ScanAccumulator >= ScanInterval)
	{
		ScanAccumulator = 0.0f;
		RefreshInteractionCandidate();
	}
}

FARRequestStatus UARInteractionComponent::TryInteract()
{
	FARRequestStatus Status;
	AActor* Candidate = CurrentCandidate.Get();
	if (!PlayerOwner || PlayerOwner->IsGameplayInputBlocked())
	{
		Status.Result = EARRequestResult::Blocked;
	}
	else if (!Candidate || !Candidate->GetClass()->ImplementsInterface(UARInteractableInterface::StaticClass()))
	{
		Status.Result = EARRequestResult::InvalidTarget;
	}
	else
	{
		FText FailureReason;
		if (!IARInteractableInterface::Execute_CanInteract(Candidate, PlayerOwner, FailureReason))
		{
			Status.Result = EARRequestResult::Rejected;
			Status.Message = FailureReason;
		}
		else if (IARInteractableInterface::Execute_RequiresInteractionLineOfSight(Candidate) && !HasLineOfSightTo(Candidate))
		{
			Status.Result = EARRequestResult::Blocked;
		}
		else
		{
			Status = IARInteractableInterface::Execute_Interact(Candidate, PlayerOwner);
		}
	}
	OnInteractionCompleted.Broadcast(Candidate, Status);
	RefreshInteractionCandidate();
	return Status;
}

void UARInteractionComponent::RefreshInteractionCandidate()
{
	if (!PlayerOwner || !GetWorld())
	{
		SetCurrentCandidate(nullptr);
		return;
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ARInteractionTraceChannel);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ARInteractionScan), false, PlayerOwner);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		PlayerOwner->GetActorLocation(),
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(InteractionRadius),
		QueryParams);

	AActor* Best = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* Candidate = Result.GetActor();
		if (!IsValid(Candidate) || !Candidate->GetClass()->ImplementsInterface(UARInteractableInterface::StaticClass()))
		{
			continue;
		}
		FText FailureReason;
		if (!IARInteractableInterface::Execute_CanInteract(Candidate, PlayerOwner, FailureReason))
		{
			continue;
		}
		if (IARInteractableInterface::Execute_RequiresInteractionLineOfSight(Candidate) && !HasLineOfSightTo(Candidate))
		{
			continue;
		}
		const int32 Priority = IARInteractableInterface::Execute_GetInteractionPriority(Candidate, PlayerOwner);
		const float DistanceSq = FVector::DistSquared2D(PlayerOwner->GetActorLocation(), Candidate->GetActorLocation());
		if (Priority > BestPriority || (Priority == BestPriority && DistanceSq < BestDistanceSq))
		{
			Best = Candidate;
			BestPriority = Priority;
			BestDistanceSq = DistanceSq;
		}
	}
	SetCurrentCandidate(Best);
}

bool UARInteractionComponent::HasLineOfSightTo(const AActor* Candidate) const
{
	if (!PlayerOwner || !Candidate || !GetWorld())
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ARInteractionLineOfSight), true, PlayerOwner);
	Params.AddIgnoredActor(Candidate);
	return !GetWorld()->LineTraceSingleByChannel(Hit, PlayerOwner->GetActorLocation(), Candidate->GetActorLocation(), LineOfSightChannel, Params);
}

void UARInteractionComponent::SetCurrentCandidate(AActor* Candidate)
{
	if (CurrentCandidate.Get() == Candidate)
	{
		if (Candidate)
		{
			CurrentPrompt = IARInteractableInterface::Execute_GetInteractionPrompt(Candidate, PlayerOwner);
		}
		return;
	}
	CurrentCandidate = Candidate;
	CurrentPrompt = Candidate
		? IARInteractableInterface::Execute_GetInteractionPrompt(Candidate, PlayerOwner)
		: FText::GetEmpty();
	OnInteractionCandidateChanged.Broadcast(Candidate, CurrentPrompt);
}
