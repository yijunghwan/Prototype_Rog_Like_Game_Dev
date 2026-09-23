#include "Foundation/Components/ARUIManagerComponent.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARConsumableComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
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
		PlayerOwner->GetHealthComponent()->OnHealthChanged.AddDynamic(this, &UARUIManagerComponent::HandleHealthChanged);
		PlayerOwner->GetHealthComponent()->OnShieldChanged.AddDynamic(this, &UARUIManagerComponent::HandleShieldChanged);
	}
	if (PlayerOwner && PlayerOwner->GetManaComponent())
	{
		PlayerOwner->GetManaComponent()->OnResourceChanged.AddDynamic(this, &UARUIManagerComponent::HandleManaChanged);
	}
	if (PlayerOwner && PlayerOwner->GetStaminaComponent())
	{
		PlayerOwner->GetStaminaComponent()->OnResourceChanged.AddDynamic(this, &UARUIManagerComponent::HandleStaminaChanged);
	}
	if (PlayerOwner && PlayerOwner->GetLoadoutComponent())
	{
		PlayerOwner->GetLoadoutComponent()->OnLoadoutChanged.AddDynamic(this, &UARUIManagerComponent::HandleLoadoutChanged);
		PlayerOwner->GetLoadoutComponent()->OnRegisteredSkillsChanged.AddDynamic(this, &UARUIManagerComponent::HandleRegisteredSkillsChanged);
		PlayerOwner->GetLoadoutComponent()->OnItemUIStateChanged.AddDynamic(this, &UARUIManagerComponent::HandleItemUIStateChanged);
	}
	if (PlayerOwner && PlayerOwner->GetConsumableComponent())
	{
		PlayerOwner->GetConsumableComponent()->OnConsumableSlotsChanged.AddDynamic(this, &UARUIManagerComponent::HandleConsumableSlotsChanged);
	}
	if (PlayerOwner)
	{
		PlayerOwner->OnAimWorldLocationChanged.AddDynamic(this, &UARUIManagerComponent::HandleAimWorldLocationChanged);
		if (PlayerOwner->GetStatusEffectComponent())
		{
			PlayerOwner->GetStatusEffectComponent()->OnStatusAdded.AddDynamic(this, &UARUIManagerComponent::HandleStatusEffectChanged);
			PlayerOwner->GetStatusEffectComponent()->OnStatusUpdated.AddDynamic(this, &UARUIManagerComponent::HandleStatusEffectChanged);
			PlayerOwner->GetStatusEffectComponent()->OnStatusRemoved.AddDynamic(this, &UARUIManagerComponent::HandleStatusEffectChanged);
		}
	}
}

void UARUIManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PlayerOwner && PlayerOwner->GetHealthComponent())
	{
		PlayerOwner->GetHealthComponent()->OnDeath.RemoveDynamic(this, &UARUIManagerComponent::HandleOwnerDeath);
		PlayerOwner->GetHealthComponent()->OnHealthChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleHealthChanged);
		PlayerOwner->GetHealthComponent()->OnShieldChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleShieldChanged);
	}
	if (PlayerOwner && PlayerOwner->GetManaComponent())
	{
		PlayerOwner->GetManaComponent()->OnResourceChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleManaChanged);
	}
	if (PlayerOwner && PlayerOwner->GetStaminaComponent())
	{
		PlayerOwner->GetStaminaComponent()->OnResourceChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleStaminaChanged);
	}
	if (PlayerOwner && PlayerOwner->GetLoadoutComponent())
	{
		PlayerOwner->GetLoadoutComponent()->OnLoadoutChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleLoadoutChanged);
		PlayerOwner->GetLoadoutComponent()->OnRegisteredSkillsChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleRegisteredSkillsChanged);
		PlayerOwner->GetLoadoutComponent()->OnItemUIStateChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleItemUIStateChanged);
	}
	if (PlayerOwner && PlayerOwner->GetConsumableComponent())
	{
		PlayerOwner->GetConsumableComponent()->OnConsumableSlotsChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleConsumableSlotsChanged);
	}
	if (PlayerOwner)
	{
		PlayerOwner->OnAimWorldLocationChanged.RemoveDynamic(this, &UARUIManagerComponent::HandleAimWorldLocationChanged);
		if (PlayerOwner->GetStatusEffectComponent())
		{
			PlayerOwner->GetStatusEffectComponent()->OnStatusAdded.RemoveDynamic(this, &UARUIManagerComponent::HandleStatusEffectChanged);
			PlayerOwner->GetStatusEffectComponent()->OnStatusUpdated.RemoveDynamic(this, &UARUIManagerComponent::HandleStatusEffectChanged);
			PlayerOwner->GetStatusEffectComponent()->OnStatusRemoved.RemoveDynamic(this, &UARUIManagerComponent::HandleStatusEffectChanged);
		}
	}
	CloseCurrentScreen();
	Super::EndPlay(EndPlayReason);
}

FARPlayerHUDSnapshot UARUIManagerComponent::GetHUDSnapshot() const
{
	FARPlayerHUDSnapshot Snapshot;
	if (!PlayerOwner)
	{
		return Snapshot;
	}
	if (const UARHealthComponent* Health = PlayerOwner->GetHealthComponent())
	{
		Snapshot.Health.Current = Health->GetCurrentHealth();
		Snapshot.Health.Maximum = Health->GetMaxHealth();
		Snapshot.Health.Ratio = Snapshot.Health.Maximum > 0.0f ? Snapshot.Health.Current / Snapshot.Health.Maximum : 0.0f;
		Snapshot.Shield = Health->GetCurrentShield();
	}
	if (const UARManaComponent* Mana = PlayerOwner->GetManaComponent())
	{
		Snapshot.Mana.Current = Mana->GetCurrent();
		Snapshot.Mana.Maximum = Mana->GetMax();
		Snapshot.Mana.Ratio = Mana->GetRatio();
	}
	if (const UARStaminaComponent* Stamina = PlayerOwner->GetStaminaComponent())
	{
		Snapshot.Stamina.Current = Stamina->GetCurrent();
		Snapshot.Stamina.Maximum = Stamina->GetMax();
		Snapshot.Stamina.Ratio = Stamina->GetRatio();
	}
	Snapshot.AimDirection = PlayerOwner->GetAimDirection();
	Snapshot.bHasAimWorldLocation = PlayerOwner->HasAimWorldLocation();
	Snapshot.AimWorldLocation = PlayerOwner->GetAimWorldLocation();
	if (const UARLoadoutComponent* Loadout = PlayerOwner->GetLoadoutComponent())
	{
		for (const FARLoadoutItemSnapshot& Item : Loadout->GetLoadoutInventory())
		{
			if (Item.Kind == EARLoadoutItemKind::Weapon)
			{
				Snapshot.bHasEquippedWeapon = true;
				Snapshot.EquippedWeapon = Item;
			}
			else if (Item.Kind == EARLoadoutItemKind::ActiveRelic)
			{
				Snapshot.ActiveRelics.Add(Item);
			}
		}
		Snapshot.Skills = Loadout->GetRegisteredSkillUIData();
	}
	if (const UARConsumableComponent* Consumables = PlayerOwner->GetConsumableComponent())
	{
		Snapshot.Consumables = Consumables->GetConsumableSlots();
	}
	if (const UARStatusEffectComponent* StatusEffects = PlayerOwner->GetStatusEffectComponent())
	{
		Snapshot.StatusEffects = StatusEffects->GetActiveStatusEffects();
	}
	return Snapshot;
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

void UARUIManagerComponent::BroadcastHUDSnapshot()
{
	OnHUDSnapshotChanged.Broadcast(GetHUDSnapshot());
}

void UARUIManagerComponent::HandleHealthChanged(AActor* Target, float Current, float Maximum, float Delta, EARResourceChangeReason Reason)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleShieldChanged(AActor* Target, float Current, float Delta, FARShieldHandle Handle, EARResourceChangeReason Reason)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleManaChanged(AActor* Target, float Current, float Maximum, float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleStaminaChanged(AActor* Target, float Current, float Maximum, float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleLoadoutChanged(int32 Revision)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleRegisteredSkillsChanged()
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleItemUIStateChanged(FGuid ItemInstanceId, const FARItemUIState& State, bool bRemoved)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleConsumableSlotsChanged(const TArray<FARConsumableSlotSnapshot>& Slots)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleAimWorldLocationChanged(AARPlayerCharacter* Player, FVector WorldLocation)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleStatusEffectChanged(AActor* Target, const FARStatusEffectView& Status)
{
	BroadcastHUDSnapshot();
}

void UARUIManagerComponent::HandleOwnerDeath(AActor* Target, const FARCombatDamageResult& KillingDamage)
{
	if (PlayerOwner && PlayerOwner->GetLoadoutComponent())
	{
		PlayerOwner->GetLoadoutComponent()->CancelAllPendingRequests();
	}
	CloseCurrentScreen();
}
