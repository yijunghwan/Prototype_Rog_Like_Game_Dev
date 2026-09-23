#include "Foundation/Interaction/ARItemPickupActors.h"

#include "Components/SphereComponent.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARConsumableComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Items/ARConsumableDefinition.h"
#include "Foundation/Items/ARLoadoutItemDefinition.h"

namespace
{
	constexpr ECollisionChannel ARInteractableChannel = ECC_GameTraceChannel3;
}

AARItemPickupBase::AARItemPickupBase()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	SetRootComponent(InteractionVolume);
	InteractionVolume->InitSphereRadius(48.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionObjectType(ARInteractableChannel);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionPrompt = NSLOCTEXT("ARInteraction", "PickupPrompt", "획득");
}

bool AARItemPickupBase::CanInteract_Implementation(AARPlayerCharacter* Interactor, FText& FailureReason) const
{
	FailureReason = FText::GetEmpty();
	return IsValid(Interactor);
}

FText AARItemPickupBase::GetInteractionPrompt_Implementation(AARPlayerCharacter* Interactor) const
{
	return InteractionPrompt;
}

int32 AARItemPickupBase::GetInteractionPriority_Implementation(AARPlayerCharacter* Interactor) const
{
	return InteractionPriority;
}

bool AARItemPickupBase::RequiresInteractionLineOfSight_Implementation() const
{
	return bRequiresLineOfSight;
}

bool AARLoadoutItemPickup::CanInteract_Implementation(AARPlayerCharacter* Interactor, FText& FailureReason) const
{
	if (!Super::CanInteract_Implementation(Interactor, FailureReason) || !ItemDefinition)
	{
		FailureReason = NSLOCTEXT("ARInteraction", "InvalidLoadoutPickup", "획득할 아이템 정보가 없습니다.");
		return false;
	}
	return true;
}

void AARLoadoutItemPickup::AssignItemDefinition(const UARLoadoutItemDefinition* Definition)
{
	ItemDefinition = Definition;
	ReceiveItemDefinitionAssigned();
}

FARRequestStatus AARLoadoutItemPickup::Interact_Implementation(AARPlayerCharacter* Interactor)
{
	FARRequestStatus Status;
	if (!Interactor || !Interactor->GetLoadoutComponent())
	{
		Status.Result = EARRequestResult::InvalidOwner;
		return Status;
	}
	const FARLoadoutAcquisitionResult BeginResult = Interactor->GetLoadoutComponent()->BeginLoadoutAcquisition(ItemDefinition);
	if (!BeginResult.Status.IsSuccess())
	{
		return BeginResult.Status;
	}
	if (Interactor->GetLoadoutComponent()->CommitLoadoutAcquisition(BeginResult.Token, Status))
	{
		Destroy();
	}
	return Status;
}

bool AARConsumablePickup::CanInteract_Implementation(AARPlayerCharacter* Interactor, FText& FailureReason) const
{
	if (!Super::CanInteract_Implementation(Interactor, FailureReason) || !ConsumableDefinition)
	{
		FailureReason = NSLOCTEXT("ARInteraction", "InvalidConsumablePickup", "획득할 소모품 정보가 없습니다.");
		return false;
	}
	return true;
}

void AARConsumablePickup::AssignConsumableDefinition(UARConsumableDefinition* Definition)
{
	ConsumableDefinition = Definition;
	ReceiveConsumableDefinitionAssigned();
}

FARRequestStatus AARConsumablePickup::Interact_Implementation(AARPlayerCharacter* Interactor)
{
	FARRequestStatus Status;
	if (!Interactor || !Interactor->GetConsumableComponent())
	{
		Status.Result = EARRequestResult::InvalidOwner;
		return Status;
	}
	const FARConsumableAcquisitionResult Result = Interactor->GetConsumableComponent()->TryAcquireConsumable(ConsumableDefinition);
	if (Result.Status.IsSuccess())
	{
		Destroy();
	}
	return Result.Status;
}
