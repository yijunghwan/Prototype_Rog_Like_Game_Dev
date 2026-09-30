#include "Foundation/Player/ARPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Foundation/UI/ARResourceHUDWidget.h"

AARPlayerController::AARPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	ResourceHUDClass = TSoftClassPtr<UARResourceHUDWidget>(FSoftObjectPath(TEXT("/Game/Game/Tests/UI/WBP_TestResourceHUD.WBP_TestResourceHUD_C")));
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
	RefreshResourceHUD();
}

void AARPlayerController::RefreshResourceHUD()
{
	if (!IsLocalController() || !HasActorBegunPlay() || !bShowResourceHUD || !GetLocalPlayer()) return;
	if (!ResourceHUD)
	{
		UClass* Class = ResourceHUDClass.IsNull() ? nullptr : ResourceHUDClass.LoadSynchronous();
		ResourceHUD = CreateWidget<UARResourceHUDWidget>(this, Class ? Class : UARResourceHUDWidget::StaticClass());
		if (ResourceHUD) ResourceHUD->AddToViewport(10);
	}
	if (ResourceHUD) ResourceHUD->ObservePawn(GetPawn());
}

void AARPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RefreshResourceHUD();
}

void AARPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshResourceHUD();
}

void AARPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	if (ResourceHUD) ResourceHUD->ObservePawn(nullptr);
}

void AARPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ResourceHUD)
	{
		ResourceHUD->ObservePawn(nullptr);
		ResourceHUD->RemoveFromParent();
		ResourceHUD = nullptr;
	}
	Super::EndPlay(EndPlayReason);
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
