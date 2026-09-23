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
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &AARBaseCharacter::HandleCharacterDeath);
	StaggerComponent->OnStaggered.AddDynamic(this, &AARBaseCharacter::HandleStaggered);
	StaggerComponent->OnStaggerStateChanged.AddDynamic(this, &AARBaseCharacter::HandleStaggerStateChanged);
	StatusEffectComponent->OnStatusAdded.AddDynamic(this, &AARBaseCharacter::HandleStatusAdded);
	StatusEffectComponent->OnStatusUpdated.AddDynamic(this, &AARBaseCharacter::HandleStatusUpdated);
	StatusEffectComponent->OnStatusRemoved.AddDynamic(this, &AARBaseCharacter::HandleStatusRemoved);
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
	OnCharacterDeath.Broadcast(this, KillingDamage);
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
