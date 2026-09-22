#include "Foundation/Characters/ARPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARCameraFollowComponent.h"
#include "Foundation/Components/ARConsumableComponent.h"
#include "Foundation/Components/ARInteractionComponent.h"
#include "Foundation/Components/ARUIManagerComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Player/ARPlayerController.h"

AARPlayerCharacter::AARPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	CombatTeam = EARCombatTeam::Player;
	StaminaComponent = CreateDefaultSubobject<UARStaminaComponent>(TEXT("StaminaComponent"));
	ManaComponent = CreateDefaultSubobject<UARManaComponent>(TEXT("ManaComponent"));
	LoadoutComponent = CreateDefaultSubobject<UARLoadoutComponent>(TEXT("LoadoutComponent"));
	ConsumableComponent = CreateDefaultSubobject<UARConsumableComponent>(TEXT("ConsumableComponent"));
	InteractionComponent = CreateDefaultSubobject<UARInteractionComponent>(TEXT("InteractionComponent"));
	UIManagerComponent = CreateDefaultSubobject<UARUIManagerComponent>(TEXT("UIManagerComponent"));
	CameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CameraAnchor"));
	CameraAnchor->SetupAttachment(RootComponent);
	CameraAnchor->SetUsingAbsoluteLocation(true);
	CameraAnchor->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraAnchor);
	TopDownCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TopDownCamera->OrthoWidth = 1536.0f;
	CameraFollowComponent = CreateDefaultSubobject<UARCameraFollowComponent>(TEXT("CameraFollowComponent"));
}

void AARPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CameraFollowComponent->SetCameraAnchor(CameraAnchor);
	CameraFollowComponent->SnapToTarget();
}

void AARPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAimFromCursor();
	UpdateRoll(DeltaSeconds);
}

void AARPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AARPlayerCharacter::HandleMove);
		}
		if (RollAction)
		{
			EnhancedInput->BindAction(RollAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleRollPressed);
		}
		if (InteractAction)
		{
			EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleInteractPressed);
		}
		if (InventoryAction)
		{
			EnhancedInput->BindAction(InventoryAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleInventoryPressed);
		}
		if (UIBackAction)
		{
			EnhancedInput->BindAction(UIBackAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleUIBackPressed);
		}
		for (int32 SlotIndex = 0; SlotIndex < ConsumableSlotActions.Num(); ++SlotIndex)
		{
			if (ConsumableSlotActions[SlotIndex])
			{
				EnhancedInput->BindAction(ConsumableSlotActions[SlotIndex], ETriggerEvent::Started, this, &AARPlayerCharacter::HandleConsumablePressed, SlotIndex);
			}
		}
	}
}

void AARPlayerCharacter::SetGameplayInputBlocked(bool bBlocked)
{
	bGameplayInputBlocked = bBlocked;
	if (bBlocked)
	{
		GetMovementControlComponent()->StopMovementImmediately();
	}
}

void AARPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	if (bGameplayInputBlocked || ActiveRollHandle.IsValid())
	{
		return;
	}
	const FVector2D Input = Value.Get<FVector2D>();
	const FVector Direction(Input.Y, Input.X, 0.0f);
	if (!Direction.IsNearlyZero())
	{
		LastMoveDirection = Direction.GetSafeNormal();
		GetMovementControlComponent()->RequestBasicMove(LastMoveDirection, FMath::Clamp(Direction.Size(), 0.0f, 1.0f));
		GetActionComponent()->CancelActionsByReason(EARActionCancelReason::BasicMovementInput);
	}
}

void AARPlayerCharacter::HandleRollPressed(const FInputActionValue& Value)
{
	if (bGameplayInputBlocked || ActiveRollHandle.IsValid())
	{
		return;
	}
	const float Cost = GetStatsComponent()->GetFinalStat(EARStatType::RollStaminaCost);
	if (!StaminaComponent->CanAfford(Cost))
	{
		return;
	}

	FARActionRequest Request;
	Request.ActionTag = ARGameplayTags::Action_Roll;
	Request.bIsRollAction = true;
	Request.bBlockBasicMovementWhileActive = true;
	Request.CancelRules.bCancelOnStagger = true;
	Request.CancelRules.bCancelOnStun = true;
	FARRequestStatus Precheck = GetActionComponent()->CanStartAction(Request);
	if (!Precheck.IsSuccess())
	{
		return;
	}
	GetActionComponent()->CancelActionsByReason(EARActionCancelReason::Roll);
	ActiveRollHandle = GetActionComponent()->TryStartAction(Request, Precheck);
	if (!ActiveRollHandle.IsValid())
	{
		return;
	}

	FARSourceInfo Source;
	Source.Category = EARModifierSourceCategory::Other;
	Source.SourceId = TEXT("Action.Roll");
	float NewStamina = 0.0f;
	if (!StaminaComponent->TryConsume(Cost, Source, NewStamina))
	{
		FinishRoll(true);
		return;
	}

	RollDirection = LastMoveDirection.IsNearlyZero() ? GetAimDirection() : LastMoveDirection;
	RollDirection.Z = 0.0f;
	RollDirection.Normalize();
	RollEndsAt = GetWorld()->GetTimeSeconds() + RollDuration;

	FARStatModifierSpec EvasionGuarantee;
	EvasionGuarantee.StatType = EARStatType::Evasion;
	EvasionGuarantee.Operation = EARStatModifierOperation::Flat;
	EvasionGuarantee.Duration = GetStatsComponent()->GetFinalStat(EARStatType::RollEvasionDuration);
	EvasionGuarantee.Source = Source;
	EvasionGuarantee.bGuaranteeEvasion = true;
	bool bApplied = false;
	GetActionComponent()->ApplyActionStatModifier(ActiveRollHandle, EvasionGuarantee, bApplied);
}

void AARPlayerCharacter::HandleInteractPressed(const FInputActionValue& Value)
{
	if (!bGameplayInputBlocked && InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AARPlayerCharacter::HandleConsumablePressed(const FInputActionValue& Value, int32 SlotIndex)
{
	if (!bGameplayInputBlocked && ConsumableComponent)
	{
		UARConsumableDefinition* UsedDefinition = nullptr;
		ConsumableComponent->TryUseConsumableSlot(SlotIndex, UsedDefinition);
	}
}

void AARPlayerCharacter::HandleInventoryPressed(const FInputActionValue& Value)
{
	if (!UIManagerComponent)
	{
		return;
	}
	if (UIManagerComponent->GetCurrentScreen() == EARUIScreen::Inventory)
	{
		UIManagerComponent->RequestBack();
	}
	else if (!UIManagerComponent->IsScreenOpen())
	{
		UIManagerComponent->OpenScreen(EARUIScreen::Inventory, this);
	}
}

void AARPlayerCharacter::HandleUIBackPressed(const FInputActionValue& Value)
{
	if (UIManagerComponent && UIManagerComponent->IsScreenOpen())
	{
		UIManagerComponent->RequestBack();
	}
}

void AARPlayerCharacter::UpdateAimFromCursor()
{
	if (const AARPlayerController* PlayerController = Cast<AARPlayerController>(GetController()))
	{
		FVector CursorPoint;
		if (PlayerController->ProjectMouseToGameplayPlane(GetActorLocation().Z, CursorPoint))
		{
			SetAimDirection(CursorPoint - GetActorLocation());
		}
	}
}

void AARPlayerCharacter::UpdateRoll(float DeltaSeconds)
{
	if (!ActiveRollHandle.IsValid())
	{
		return;
	}
	if (!GetActionComponent()->IsActionActive(ActiveRollHandle))
	{
		ActiveRollHandle = FARActionHandle();
		RollEndsAt = -1.0;
		return;
	}
	if (!GetWorld() || GetWorld()->GetTimeSeconds() >= RollEndsAt)
	{
		FinishRoll(false);
		return;
	}
	const float Distance = GetStatsComponent()->GetFinalStat(EARStatType::RollDistance);
	GetMovementControlComponent()->RequestActionVelocity(ActiveRollHandle, RollDirection, Distance / FMath::Max(RollDuration, 0.01f));
}

void AARPlayerCharacter::FinishRoll(bool bCancel)
{
	if (!ActiveRollHandle.IsValid())
	{
		return;
	}
	if (bCancel)
	{
		GetActionComponent()->CancelAction(ActiveRollHandle, EARActionCancelReason::Manual);
	}
	else
	{
		GetActionComponent()->EndAction(ActiveRollHandle);
	}
	GetMovementControlComponent()->StopMovementImmediately();
	ActiveRollHandle = FARActionHandle();
	RollEndsAt = -1.0;
}
