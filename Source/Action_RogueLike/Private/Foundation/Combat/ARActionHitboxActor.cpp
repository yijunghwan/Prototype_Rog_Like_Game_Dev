#include "Foundation/Combat/ARActionHitboxActor.h"

#include "Components/SceneComponent.h"
#include "Foundation/Components/ARActionComponent.h"

AARActionHitboxActor::AARActionHitboxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	SetActorEnableCollision(false);
}

bool AARActionHitboxActor::InitializeHitbox(FARActionHandle InActionHandle, AActor* InCombatSource, EARHitboxHitPolicy InHitPolicy, float Lifetime)
{
	if (bInitialized || !IsValid(InCombatSource) || !InActionHandle.IsValid())
	{
		return false;
	}
	UARActionComponent* ActionComponent = InCombatSource->FindComponentByClass<UARActionComponent>();
	if (!ActionComponent || !ActionComponent->IsActionActive(InActionHandle) || !ActionComponent->RegisterActionHitbox(InActionHandle, this))
	{
		return false;
	}
	ActionHandle = InActionHandle;
	CombatSource = InCombatSource;
	HitPolicy = InHitPolicy;
	bInitialized = true;
	if (Lifetime > 0.0f)
	{
		SetLifeSpan(Lifetime);
	}
	return true;
}

bool AARActionHitboxActor::TryAcceptTarget(AActor* Candidate)
{
	if (!bInitialized || !IsValid(Candidate) || Candidate == CombatSource.Get())
	{
		return false;
	}
	if (HitPolicy == EARHitboxHitPolicy::OncePerTarget && AcceptedTargets.Contains(Candidate))
	{
		return false;
	}
	AcceptedTargets.Add(Candidate);
	OnTargetAccepted.Broadcast(Candidate);
	ReceiveTargetAccepted(Candidate);
	return true;
}

void AARActionHitboxActor::ResetAcceptedTargets()
{
	AcceptedTargets.Reset();
}

bool AARActionHitboxActor::HasAcceptedTarget(AActor* Target) const
{
	return Target && AcceptedTargets.Contains(Target);
}
