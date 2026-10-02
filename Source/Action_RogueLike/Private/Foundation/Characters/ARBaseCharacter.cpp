#include "Foundation/Characters/ARBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"

AARBaseCharacter::AARBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	StatsComponent = CreateDefaultSubobject<UARStatsComponent>(TEXT("StatsComponent"));
	HealthComponent = CreateDefaultSubobject<UARHealthComponent>(TEXT("HealthComponent"));
	StatusEffectComponent = CreateDefaultSubobject<UARStatusEffectComponent>(TEXT("StatusEffectComponent"));
	StaggerComponent = CreateDefaultSubobject<UARStaggerComponent>(TEXT("StaggerComponent"));
	ActionComponent = CreateDefaultSubobject<UARActionComponent>(TEXT("ActionComponent"));
	MovementControlComponent = CreateDefaultSubobject<UARMovementControlComponent>(TEXT("MovementControlComponent"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->SetPlaneConstraintEnabled(true);
		Movement->SetPlaneConstraintNormal(FVector::UpVector);
		Movement->bSnapToPlaneAtStart = true;
	}
}

void AARBaseCharacter::BeginPlay()
{
	// Actor::BeginPlay dispatches BP BeginPlay after component initialization. Bind
	// first so damage/status/action work performed by that BP also reaches the hooks.
	StatsComponent->OnFinalStatChanged.AddDynamic(this, &AARBaseCharacter::HandleFinalStatChanged);
	HealthComponent->OnDeath.AddDynamic(this, &AARBaseCharacter::HandleCharacterDeath);
	HealthComponent->OnShieldBrokenNative.AddUObject(this, &AARBaseCharacter::HandleShieldBroken);
	HealthComponent->OnDamageAppliedNative.AddUObject(this, &AARBaseCharacter::HandleDamageApplied);
	ActionComponent->OnActionCancelled.AddDynamic(this, &AARBaseCharacter::HandleActionCancelled);
	StaggerComponent->OnGroggyGaugeDepleted.AddDynamic(this, &AARBaseCharacter::HandleGroggyGaugeDepleted);
	StaggerComponent->OnStaggeredNative.AddUObject(this, &AARBaseCharacter::HandleStaggered);
	StaggerComponent->OnStaggerStateChangedNative.AddUObject(this, &AARBaseCharacter::HandleStaggerStateChanged);
	StatusEffectComponent->OnStatusAddedNative.AddUObject(this, &AARBaseCharacter::HandleStatusAdded);
	StatusEffectComponent->OnStatusUpdatedNative.AddUObject(this, &AARBaseCharacter::HandleStatusUpdated);
	StatusEffectComponent->OnStatusRemovedNative.AddUObject(this, &AARBaseCharacter::HandleStatusRemoved);
	Super::BeginPlay();
	HandleFinalStatChanged(this, EARStatType::MoveSpeed, 0.0f, StatsComponent->GetFinalStat(EARStatType::MoveSpeed));
}

void AARBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StatsComponent->OnFinalStatChanged.RemoveDynamic(this, &AARBaseCharacter::HandleFinalStatChanged);
	HealthComponent->OnDeath.RemoveDynamic(this, &AARBaseCharacter::HandleCharacterDeath);
	HealthComponent->OnShieldBrokenNative.RemoveAll(this);
	HealthComponent->OnDamageAppliedNative.RemoveAll(this);
	StaggerComponent->OnGroggyGaugeDepleted.RemoveDynamic(this, &AARBaseCharacter::HandleGroggyGaugeDepleted);
	StaggerComponent->OnStaggeredNative.RemoveAll(this);
	StaggerComponent->OnStaggerStateChangedNative.RemoveAll(this);
	StatusEffectComponent->OnStatusAddedNative.RemoveAll(this);
	StatusEffectComponent->OnStatusUpdatedNative.RemoveAll(this);
	StatusEffectComponent->OnStatusRemovedNative.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
	// Keep cancellation forwarding alive through component EndPlay cleanup.
	ActionComponent->OnActionCancelled.RemoveDynamic(this, &AARBaseCharacter::HandleActionCancelled);
}

void AARBaseCharacter::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (StatType == EARStatType::MoveSpeed && GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, NewValue);
		GetCharacterMovement()->MaxWalkSpeedCrouched = FMath::Max(0.0f, NewValue);
	}
}

bool AARBaseCharacter::CanBeCombatTarget() const
{
	return HealthComponent && !HealthComponent->IsDead();
}

void AARBaseCharacter::SetAimDirection(FVector NewAimDirection)
{
	NewAimDirection.Z = 0.0f;
	if (NewAimDirection.IsNearlyZero())
	{
		return;
	}
	const FVector Normalized = NewAimDirection.GetSafeNormal();
	if (!Normalized.Equals(AimDirection, KINDA_SMALL_NUMBER))
	{
		AimDirection = Normalized;
		OnAimDirectionChanged.Broadcast(this, AimDirection);
	}
}

void AARBaseCharacter::HandleCharacterDeath(AActor* Target, const FARCombatDamageResult& KillingDamage)
{
	ActionComponent->CancelAllActions(EARActionCancelReason::Death);
	MovementControlComponent->StopMovementImmediately();
	if (!DeathMovementLock.IsValid())
	{
		DeathMovementLock = MovementControlComponent->AcquireMovementLock(TEXT("State.Death"), EARMovementLockType::AllMovement);
	}
	StatusEffectComponent->ClearAllStatusEffects();
	ReceiveCharacterDeath(KillingDamage);
	OnCharacterDeath.Broadcast(this, KillingDamage);
}

void AARBaseCharacter::HandleGroggyGaugeDepleted(AActor* Target)
{
	ReceiveGroggyGaugeDepleted();
}

void AARBaseCharacter::HandleShieldBroken(AActor* Target, float PreviousShield, EARResourceChangeReason Reason)
{
	ReceiveShieldBroken(PreviousShield, Reason);
}

void AARBaseCharacter::HandleDamageApplied(AActor* Target, const FARCombatDamageResult& DamageResult)
{
	ReceiveDamageApplied(DamageResult);
}

void AARBaseCharacter::HandleActionCancelled(FARActionHandle ActionHandle, EARActionCancelReason Reason)
{
	ReceiveActionCancelled(ActionHandle, Reason);
}

void AARBaseCharacter::HandleStaggered(AActor* Target, const FARStaggerResult& Result)
{
	MovementControlComponent->StopMovementImmediately();
	ActionComponent->CancelActionsByReason(EARActionCancelReason::Stagger);
}

void AARBaseCharacter::HandleStaggerStateChanged(AActor* Target, bool bIsStaggered)
{
	if (bIsStaggered && !StaggerMovementLock.IsValid())
	{
		StaggerMovementLock = MovementControlComponent->AcquireMovementLock(TEXT("State.Stagger"), EARMovementLockType::AllMovement);
	}
	else if (!bIsStaggered && StaggerMovementLock.IsValid())
	{
		MovementControlComponent->ReleaseMovementLock(StaggerMovementLock);
		StaggerMovementLock = FARMovementLockHandle();
	}
}

void AARBaseCharacter::HandleStatusAdded(AActor* Target, const FARStatusEffectView& Status)
{
	if (Status.Definition)
	{
		if (Status.Definition->bCancelActionsOnApply)
		{
			ActionComponent->CancelActionsByReason(Status.Definition->ActionCancelReason);
		}
		else if (Status.Definition->bBlocksRoll)
		{
			ActionComponent->CancelRollActions(EARActionCancelReason::Root);
		}
	}
	RefreshStatusMovementLock();
}

void AARBaseCharacter::HandleStatusUpdated(AActor* Target, const FARStatusEffectView& Status)
{
	RefreshStatusMovementLock();
}

void AARBaseCharacter::HandleStatusRemoved(AActor* Target, const FARStatusEffectView& Status)
{
	RefreshStatusMovementLock();
	ReceiveStatusRemoved(Status);
}

void AARBaseCharacter::RefreshStatusMovementLock()
{
	const bool bBlocksAll = StatusEffectComponent->BlocksAllMovement();
	const bool bBlocksBasic = StatusEffectComponent->BlocksBasicMovement();
	const bool bNeedsLock = bBlocksAll || bBlocksBasic;
	const EARMovementLockType DesiredType = bBlocksAll ? EARMovementLockType::AllMovement : EARMovementLockType::BasicMovementOnly;

	if (StatusMovementLock.IsValid() && (!bNeedsLock || DesiredType != CurrentStatusLockType))
	{
		MovementControlComponent->ReleaseMovementLock(StatusMovementLock);
		StatusMovementLock = FARMovementLockHandle();
	}
	if (bNeedsLock && !StatusMovementLock.IsValid())
	{
		CurrentStatusLockType = DesiredType;
		StatusMovementLock = MovementControlComponent->AcquireMovementLock(TEXT("State.StatusEffect"), DesiredType);
		MovementControlComponent->StopMovementImmediately();
	}
}
