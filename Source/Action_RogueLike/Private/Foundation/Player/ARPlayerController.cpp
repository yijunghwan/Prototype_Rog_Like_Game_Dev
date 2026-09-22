#include "Foundation/Player/ARPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

AARPlayerController::AARPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AARPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (PlayerMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(PlayerMappingContext, MappingPriority);
		}
	}
}

bool AARPlayerController::ProjectMouseToGameplayPlane(float PlaneZ, FVector& WorldPoint) const
{
	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection) || FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}
	const float Distance = (PlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (Distance < 0.0f)
	{
		return false;
	}
	WorldPoint = WorldOrigin + WorldDirection * Distance;
	return true;
}

void AARPlayerController::SetGameplayUIInputMode(bool bUIOpen)
{
	if (bUIOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
	bShowMouseCursor = true;
}
