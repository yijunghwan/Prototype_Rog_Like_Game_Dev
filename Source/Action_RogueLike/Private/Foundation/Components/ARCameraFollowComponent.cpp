#include "Foundation/Components/ARCameraFollowComponent.h"

#include "Components/SceneComponent.h"

UARCameraFollowComponent::UARCameraFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UARCameraFollowComponent::BeginPlay()
{
	Super::BeginPlay();
	SnapToTarget();
}

void UARCameraFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!CameraAnchor || !GetOwner())
	{
		return;
	}
	const FVector Target = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, CameraHeight);
	const FVector Current = CameraAnchor->GetComponentLocation();
	const float Distance2D = FVector::Dist2D(Current, Target);
	if (Distance2D >= SnapDistance && SnapDistance > 0.0f)
	{
		CameraAnchor->SetWorldLocation(Target);
		return;
	}
	const float ExtraDistance = FMath::Max(0.0f, Distance2D - AccelerationStartDistance);
	const float FollowSpeed = FMath::Clamp(BaseFollowSpeed + ExtraDistance * DistanceSpeedMultiplier, BaseFollowSpeed, MaximumFollowSpeed);
	CameraAnchor->SetWorldLocation(FMath::VInterpTo(Current, Target, DeltaTime, FollowSpeed));
}

void UARCameraFollowComponent::SetCameraAnchor(USceneComponent* InCameraAnchor)
{
	CameraAnchor = InCameraAnchor;
}

void UARCameraFollowComponent::SnapToTarget()
{
	if (CameraAnchor && GetOwner())
	{
		CameraAnchor->SetWorldLocation(GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, CameraHeight));
	}
}

