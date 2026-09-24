#include "Foundation/Characters/ARPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "InputActionValue.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARCameraFollowComponent.h"
#include "Foundation/Components/ARConsumableComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARInteractionComponent.h"
#include "Foundation/Components/ARUIManagerComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
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
	GetActionComponent()->OnActionEnded.AddDynamic(this, &AARPlayerCharacter::HandleRollActionEnded);
	GetActionComponent()->OnActionCancelled.AddDynamic(this, &AARPlayerCharacter::HandleRollActionCancelled);
	PostHitDamageDelegateHandle = GetHealthComponent()->OnDamageAppliedNative.AddUObject(
		this, &AARPlayerCharacter::HandlePlayerDamageApplied);
	CameraFollowComponent->SetCameraAnchor(CameraAnchor);
	CameraFollowComponent->SnapToTarget();
}

void AARPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishRoll(true);
	GetActionComponent()->OnActionEnded.RemoveDynamic(this, &AARPlayerCharacter::HandleRollActionEnded);
	GetActionComponent()->OnActionCancelled.RemoveDynamic(this, &AARPlayerCharacter::HandleRollActionCancelled);
	if (GetHealthComponent())
	{
		GetHealthComponent()->OnDamageAppliedNative.Remove(PostHitDamageDelegateHandle);
	}
	if (PostHitInvulnerabilityHandle.IsValid() && GetStatsComponent())
	{
		GetStatsComponent()->RemoveStatModifier(PostHitInvulnerabilityHandle);
		PostHitInvulnerabilityHandle = FARStatModifierHandle();
	}
	Super::EndPlay(EndPlayReason);
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
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &AARPlayerCharacter::HandleMoveReleased);
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AARPlayerCharacter::HandleMoveReleased);
		}
		if (RollAction)
		{
			EnhancedInput->BindAction(RollAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleRollPressed);
		}
		if (MouseRollAction && MouseRollAction != RollAction)
		{
			EnhancedInput->BindAction(MouseRollAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleMouseRollPressed);
		}
		if (InteractAction)
		{
			EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AARPlayerCharacter::HandleInteractPressed);
		}
		for (const FARSkillInputBinding& Binding : SkillInputBindings)
		{
			if (Binding.InputAction && Binding.InputTag.IsValid())
			{
				EnhancedInput->BindAction(Binding.InputAction, ETriggerEvent::Started, this,
					&AARPlayerCharacter::HandleSkillPressed, Binding.InputTag);
			}
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

bool AARPlayerCharacter::SetAimWorldLocation(FVector WorldLocation)
{
	if (WorldLocation.ContainsNaN())
	{
		return false;
	}
	WorldLocation.Z = GetActorLocation().Z;
	const FVector Direction = WorldLocation - GetActorLocation();
	if (Direction.IsNearlyZero())
	{
		return false;
	}
	SetAimDirection(Direction);
	const bool bChanged = !bHasAimWorldLocation || !WorldLocation.Equals(AimWorldLocation, 0.01f);
	AimWorldLocation = WorldLocation;
	bHasAimWorldLocation = true;
	if (bChanged)
	{
		OnAimWorldLocationChanged.Broadcast(this, AimWorldLocation);
	}
	return true;
}

void AARPlayerCharacter::ClearAimWorldLocation()
{
	bHasAimWorldLocation = false;
	AimWorldLocation = FVector::ZeroVector;
	OnAimWorldLocationChanged.Broadcast(this, AimWorldLocation);
}

void AARPlayerCharacter::SetGameplayInputBlocked(bool bBlocked)
{
	bGameplayInputBlocked = bBlocked;
	if (bBlocked)
	{
		GetMovementControlComponent()->StopMovementImmediately();
	}
}

bool AARPlayerCharacter::IsGameplayInputBlocked() const
{
	return bGameplayInputBlocked || (UIManagerComponent && UIManagerComponent->IsScreenOpen());
}

void AARPlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	CurrentMoveInput = Value.Get<FVector2D>();
	if (IsGameplayInputBlocked() || ActiveRollHandle.IsValid()) return;
	const FVector Direction(CurrentMoveInput.Y, CurrentMoveInput.X, 0.0f);
	if (!Direction.IsNearlyZero()
		&& GetMovementControlComponent()->RequestBasicMove(Direction.GetSafeNormal(), FMath::Clamp(Direction.Size(), 0.0f, 1.0f)))
	{
		GetActionComponent()->CancelActionsByReason(EARActionCancelReason::BasicMovementInput);
	}
}

void AARPlayerCharacter::HandleMoveReleased(const FInputActionValue& Value)
{
	CurrentMoveInput = FVector2D::ZeroVector;
}

void AARPlayerCharacter::HandleRollPressed(const FInputActionValue& Value)
{
	TryStartRoll();
}

void AARPlayerCharacter::HandleMouseRollPressed(const FInputActionValue& Value)
{
	TryStartRoll(true);
}

float AARPlayerCharacter::GetRollCooldownRemaining() const
{
	return GetWorld() ? FMath::Max(0.0, RollCooldownEndsAt - GetWorld()->GetTimeSeconds()) : 0.0f;
}

bool AARPlayerCharacter::TryStartRoll(bool bForceMouseDirection)
{
	if (IsGameplayInputBlocked() || ActiveRollHandle.IsValid() || !GetWorld()
		|| !GetMovementControlComponent()->CanMoveAtAll() || GetRollCooldownRemaining() > 0.0f) return false;
	const float Distance = GetStatsComponent()->GetFinalStat(EARStatType::RollDistance);
	if (!FMath::IsFinite(RollDuration) || RollDuration <= 0.0f || !FMath::IsFinite(Distance) || Distance <= 0.0f) return false;

	UpdateAimFromCursor();
	FVector2D MoveInput = CurrentMoveInput;
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (const UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput); Input && MoveAction)
		{
			// Read this frame's evaluated value, not the previous movement callback.
			MoveInput = Input->GetActionValue(MoveAction).Get<FVector2D>();
		}
	}
	const bool bMouseOnly = bForceMouseDirection || RollDirectionMode == EARRollDirectionMode::MouseOnly;
	FVector Direction = !bMouseOnly && !MoveInput.IsNearlyZero()
		? FVector(MoveInput.Y, MoveInput.X, 0.0f) : GetAimDirection();
	Direction.Z = 0.0f;
	Direction = Direction.GetSafeNormal();
	if (Direction.IsNearlyZero()) return false;

	const float Cost = GetStatsComponent()->GetFinalStat(EARStatType::RollStaminaCost);
	if (!StaminaComponent->CanAfford(Cost)) return false;

	FARActionRequest Request;
	Request.ActionTag = ARGameplayTags::Action_Roll;
	Request.bIsRollAction = true;
	Request.bBlockBasicMovementWhileActive = true;
	Request.CancelRules.bCancelOnStagger = true;
	Request.CancelRules.bCancelOnStun = true;
	FARRequestStatus Precheck = GetActionComponent()->CanStartAction(Request);
	if (!Precheck.IsSuccess()) return false;
	GetActionComponent()->CancelActionsByReason(EARActionCancelReason::Roll);
	ActiveRollHandle = GetActionComponent()->TryStartAction(Request, Precheck);
	if (!ActiveRollHandle.IsValid()) return false;

	FARSourceInfo Source;
	Source.Category = EARModifierSourceCategory::Other;
	Source.SourceId = TEXT("Action.Roll");
	float NewStamina = 0.0f;
	if (!StaminaComponent->TryConsume(Cost, Source, NewStamina))
	{
		FinishRoll(true);
		return false;
	}

	RollDirection = Direction;
	ConsumeMovementInputVector();
	TSharedPtr<FRootMotionSource_ConstantForce> Motion = MakeShared<FRootMotionSource_ConstantForce>();
	Motion->InstanceName = TEXT("AR.PlayerRoll");
	Motion->Priority = 1000;
	Motion->AccumulateMode = ERootMotionAccumulateMode::Override;
	Motion->Duration = RollDuration;
	Motion->Force = Direction * (Distance / RollDuration);
	Motion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Motion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	RollRootMotionId = GetCharacterMovement()->ApplyRootMotionSource(Motion);

	FARStatModifierSpec EvasionGuarantee;
	EvasionGuarantee.StatType = EARStatType::Evasion;
	EvasionGuarantee.Operation = EARStatModifierOperation::Flat;
	EvasionGuarantee.Duration = GetStatsComponent()->GetFinalStat(EARStatType::RollEvasionDuration);
	EvasionGuarantee.Source = Source;
	EvasionGuarantee.bGuaranteeEvasion = true;
	bool bApplied = false;
	GetActionComponent()->ApplyActionStatModifier(ActiveRollHandle, EvasionGuarantee, bApplied);
	return true;
}

void AARPlayerCharacter::HandleInteractPressed(const FInputActionValue& Value)
{
	if (!IsGameplayInputBlocked() && InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AARPlayerCharacter::HandleSkillPressed(const FInputActionValue& Value, FGameplayTag InputTag)
{
	if (!IsGameplayInputBlocked() && LoadoutComponent && InputTag.IsValid())
	{
		FARSkillGroupHandle GroupHandle;
		LoadoutComponent->HandleSkillInput(InputTag, GroupHandle);
	}
}

void AARPlayerCharacter::HandleConsumablePressed(const FInputActionValue& Value, int32 SlotIndex)
{
	if (!IsGameplayInputBlocked() && ConsumableComponent)
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
			SetAimWorldLocation(CursorPoint);
		}
	}
}

void AARPlayerCharacter::HandlePlayerDamageApplied(AActor* Target, const FARCombatDamageResult& Result)
{
	if (Target != this || Result.FinalDamage <= 0 || Result.bKilledTarget || PostHitInvulnerabilityDuration <= 0.0f
		|| Result.HitContext.Delivery != EARDamageDelivery::Direct || Result.HitContext.Attribute == EARDamageAttribute::Void)
	{
		return;
	}

	UARStatsComponent* Stats = GetStatsComponent();
	if (!Stats)
	{
		return;
	}
	if (PostHitInvulnerabilityHandle.IsValid())
	{
		Stats->RemoveStatModifier(PostHitInvulnerabilityHandle);
	}

	FARStatModifierSpec Spec;
	Spec.StatType = EARStatType::OverallDamageReduction;
	Spec.Operation = EARStatModifierOperation::Flat;
	Spec.Value = 0.0f;
	Spec.Duration = PostHitInvulnerabilityDuration;
	Spec.Source.Category = EARModifierSourceCategory::Other;
	Spec.Source.SourceId = TEXT("System.PlayerPostHitInvulnerability");
	Spec.Source.DisplayName = NSLOCTEXT("ARPlayer", "PostHitInvulnerability", "피격 후 무적");
	Spec.bGuaranteeInvulnerability = true;
	bool bApplied = false;
	PostHitInvulnerabilityHandle = Stats->AddStatModifier(Spec, bApplied);
	if (!bApplied)
	{
		PostHitInvulnerabilityHandle = FARStatModifierHandle();
	}
}

void AARPlayerCharacter::UpdateRoll(float DeltaSeconds)
{
	if (!ActiveRollHandle.IsValid()) return;
	if (!GetActionComponent()->IsActionActive(ActiveRollHandle))
	{
		ClearRollMovement();
		return;
	}
	if (IsGameplayInputBlocked() || !GetMovementControlComponent()->CanMoveAtAll())
	{
		FinishRoll(true);
		return;
	}
	const TSharedPtr<FRootMotionSource> Motion = GetCharacterMovement()->GetRootMotionSourceByID(RollRootMotionId);
	if (!Motion.IsValid() || Motion->Status.HasFlag(ERootMotionSourceStatusFlags::Finished)
		|| Motion->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval))
	{
		FinishRoll(false);
	}
}

void AARPlayerCharacter::FinishRoll(bool bCancel)
{
	const FARActionHandle Handle = ActiveRollHandle;
	ClearRollMovement();
	if (!Handle.IsValid()) return;
	if (bCancel) GetActionComponent()->CancelAction(Handle, EARActionCancelReason::Manual);
	else GetActionComponent()->EndAction(Handle);
}

void AARPlayerCharacter::ClearRollMovement()
{
	if (RollRootMotionId != 0)
	{
		RollCooldownEndsAt = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, RollCooldown) : -1.0;
		GetCharacterMovement()->RemoveRootMotionSourceByID(RollRootMotionId);
		RollRootMotionId = 0;
	}
	if (ActiveRollHandle.IsValid()) GetMovementControlComponent()->StopMovementImmediately();
	ActiveRollHandle = FARActionHandle();
}

void AARPlayerCharacter::HandleRollActionEnded(FARActionHandle Handle)
{
	if (Handle == ActiveRollHandle) ClearRollMovement();
}

void AARPlayerCharacter::HandleRollActionCancelled(FARActionHandle Handle, EARActionCancelReason Reason)
{
	if (Handle == ActiveRollHandle) ClearRollMovement();
}
