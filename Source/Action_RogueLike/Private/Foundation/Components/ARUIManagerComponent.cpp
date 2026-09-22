#include "Foundation/Components/ARUIManagerComponent.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Player/ARPlayerController.h"

UARUIManagerComponent::UARUIManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARUIManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerOwner = Cast<AARPlayerCharacter>(GetOwner());
	if (PlayerOwner && PlayerOwner->GetHealthComponent())
	{
		PlayerOwner->GetHealthComponent()->OnDeath.AddDynamic(this, &UARUIManagerComponent::HandleOwnerDeath);
	}
}

void UARUIManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerOwner && PlayerOwner->GetHealthComponent())
	{
		PlayerOwner->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UARUIManagerComponent::HandleOwnerDeath);
	}
	CloseCurrentScreen();
	Super::EndPlay(EndPlayReason);
}

FARRequestStatus UARUIManagerComponent::OpenScreen(EARUIScreen Screen, UObject* ContextObject)
{
	FARRequestStatus Status;
	if (!PlayerOwner)
	{
		Status.Result = EARRequestResult::InvalidOwner;
		return Status;
	}
	if (Screen == EARUIScreen::None)
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return Status;
	}
	if (CurrentScreen != EARUIScreen::None)
	{
		Status.Result = EARRequestResult::Blocked;
		return Status;
	}
	CurrentScreen = Screen;
	CurrentContext = ContextObject;
	PlayerOwner->SetGameplayInputBlocked(true);
	ApplyInputMode(true);
	OnScreenOpenRequested.Broadcast(Screen, ContextObject);
	Status.Result = EARRequestResult::Success;
	return Status;
}

bool UARUIManagerComponent::CloseCurrentScreen()
{
	if (CurrentScreen == EARUIScreen::None)
	{
		return false;
	}
	const EARUIScreen ClosedScreen = CurrentScreen;
	CurrentScreen = EARUIScreen::None;
	CurrentContext = nullptr;
	OnScreenCloseRequested.Broadcast(ClosedScreen);
	if (PlayerOwner)
	{
		PlayerOwner->SetGameplayInputBlocked(false);
	}
	ApplyInputMode(false);
	return true;
}

void UARUIManagerComponent::RequestBack()
{
	if (CurrentScreen != EARUIScreen::None)
	{
		OnScreenBackRequested.Broadcast(CurrentScreen);
	}
}

void UARUIManagerComponent::ApplyInputMode(bool bUIOpen)
{
	if (AARPlayerController* Controller = PlayerOwner ? Cast<AARPlayerController>(PlayerOwner->GetController()) : nullptr)
	{
		Controller->SetGameplayUIInputMode(bUIOpen);
	}
}

void UARUIManagerComponent::HandleOwnerDeath(AActor* Target, const FARCombatDamageResult& KillingDamage)
{
	if (PlayerOwner && PlayerOwner->GetLoadoutComponent())
	{
		PlayerOwner->GetLoadoutComponent()->CancelAllPendingRequests();
	}
	CloseCurrentScreen();
}
