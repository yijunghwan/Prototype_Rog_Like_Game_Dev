#include "Foundation/Blueprint/ARResourceBlueprintLibrary.h"

#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"

float UARResourceBlueprintLibrary::RestoreHealth(AActor* Target, float Amount, const FARSourceInfo& Source)
{
	UARHealthComponent* Component = Target ? Target->FindComponentByClass<UARHealthComponent>() : nullptr;
	return Component ? Component->RestoreHealth(Amount, Source) : 0.0f;
}

float UARResourceBlueprintLibrary::RestoreMana(AActor* Target, float Amount, const FARSourceInfo& Source)
{
	UARManaComponent* Component = Target ? Target->FindComponentByClass<UARManaComponent>() : nullptr;
	return Component ? Component->Restore(Amount, Source) : 0.0f;
}

float UARResourceBlueprintLibrary::RestoreStamina(AActor* Target, float Amount, const FARSourceInfo& Source)
{
	UARStaminaComponent* Component = Target ? Target->FindComponentByClass<UARStaminaComponent>() : nullptr;
	return Component ? Component->Restore(Amount, Source) : 0.0f;
}

bool UARResourceBlueprintLibrary::TryConsumeMana(AActor* Target, float Amount, const FARSourceInfo& Source, float& NewMana)
{
	UARManaComponent* Component = Target ? Target->FindComponentByClass<UARManaComponent>() : nullptr;
	return Component && Component->TryConsume(Amount, Source, NewMana);
}

bool UARResourceBlueprintLibrary::TryConsumeStamina(AActor* Target, float Amount, const FARSourceInfo& Source, float& NewStamina)
{
	UARStaminaComponent* Component = Target ? Target->FindComponentByClass<UARStaminaComponent>() : nullptr;
	return Component && Component->TryConsume(Amount, Source, NewStamina);
}

bool UARResourceBlueprintLibrary::CanAffordResources(AActor* Target, const FARResourceCost& Cost, EARResourceType& MissingResource)
{
	if (!FMath::IsFinite(Cost.Mana) || !FMath::IsFinite(Cost.Stamina) || Cost.Mana < 0.0f || Cost.Stamina < 0.0f)
	{
		MissingResource = Cost.Mana < 0.0f || !FMath::IsFinite(Cost.Mana) ? EARResourceType::Mana : EARResourceType::Stamina;
		return false;
	}
	const UARManaComponent* Mana = Target ? Target->FindComponentByClass<UARManaComponent>() : nullptr;
	const UARStaminaComponent* Stamina = Target ? Target->FindComponentByClass<UARStaminaComponent>() : nullptr;
	if (Cost.Mana > 0.0f && (!Mana || !Mana->CanAfford(Cost.Mana)))
	{
		MissingResource = EARResourceType::Mana;
		return false;
	}
	if (Cost.Stamina > 0.0f && (!Stamina || !Stamina->CanAfford(Cost.Stamina)))
	{
		MissingResource = EARResourceType::Stamina;
		return false;
	}
	MissingResource = EARResourceType::Health;
	return true;
}

bool UARResourceBlueprintLibrary::TryConsumeResources(AActor* Target, const FARResourceCost& Cost, const FARSourceInfo& Source, float& NewMana, float& NewStamina, EARResourceType& MissingResource)
{
	UARManaComponent* Mana = Target ? Target->FindComponentByClass<UARManaComponent>() : nullptr;
	UARStaminaComponent* Stamina = Target ? Target->FindComponentByClass<UARStaminaComponent>() : nullptr;
	NewMana = Mana ? Mana->GetCurrent() : 0.0f;
	NewStamina = Stamina ? Stamina->GetCurrent() : 0.0f;
	if (!CanAffordResources(Target, Cost, MissingResource))
	{
		return false;
	}
	if (Cost.Mana > 0.0f && !Mana->TryConsume(Cost.Mana, Source, NewMana))
	{
		MissingResource = EARResourceType::Mana;
		return false;
	}
	if (Cost.Stamina > 0.0f && !Stamina->TryConsume(Cost.Stamina, Source, NewStamina))
	{
		MissingResource = EARResourceType::Stamina;
		return false;
	}
	return true;
}

FARShieldHandle UARResourceBlueprintLibrary::ApplyShield(AActor* Target, const FARShieldSpec& Spec, bool& bSuccess, float& NewTotalShield)
{
	bSuccess = false;
	NewTotalShield = 0.0f;
	if (UARHealthComponent* Health = Target ? Target->FindComponentByClass<UARHealthComponent>() : nullptr)
	{
		FARShieldHandle Handle = Health->ApplyShield(Spec, bSuccess);
		NewTotalShield = Health->GetCurrentShield();
		return Handle;
	}
	return FARShieldHandle();
}

bool UARResourceBlueprintLibrary::RemoveShield(AActor* Target, FARShieldHandle Handle, float& RemovedAmount, float& NewTotalShield)
{
	RemovedAmount = 0.0f;
	NewTotalShield = 0.0f;
	if (UARHealthComponent* Health = Target ? Target->FindComponentByClass<UARHealthComponent>() : nullptr)
	{
		const bool bRemoved = Health->RemoveShield(Handle, RemovedAmount);
		NewTotalShield = Health->GetCurrentShield();
		return bRemoved;
	}
	return false;
}

int32 UARResourceBlueprintLibrary::RemoveShieldsBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARShieldLifetimeFilter LifetimeFilter, float& RemovedAmount, float& NewTotalShield)
{
	RemovedAmount = 0.0f;
	NewTotalShield = 0.0f;
	if (UARHealthComponent* Health = Target ? Target->FindComponentByClass<UARHealthComponent>() : nullptr)
	{
		const int32 Removed = Health->RemoveShieldsBySource(Category, SourceId, LifetimeFilter, RemovedAmount);
		NewTotalShield = Health->GetCurrentShield();
		return Removed;
	}
	return 0;
}

bool UARResourceBlueprintLibrary::GetCurrentResource(AActor* Target, EARResourceType ResourceType, float& Current, float& Maximum, float& Ratio)
{
	Current = 0.0f;
	Maximum = 0.0f;
	Ratio = 0.0f;
	if (!Target)
	{
		return false;
	}
	if (ResourceType == EARResourceType::Health || ResourceType == EARResourceType::Shield)
	{
		const UARHealthComponent* Health = Target->FindComponentByClass<UARHealthComponent>();
		if (!Health) return false;
		Current = ResourceType == EARResourceType::Health ? Health->GetCurrentHealth() : Health->GetCurrentShield();
		Maximum = ResourceType == EARResourceType::Health ? Health->GetMaxHealth() : Health->GetCurrentShield();
	}
	else if (ResourceType == EARResourceType::Mana)
	{
		const UARManaComponent* Mana = Target->FindComponentByClass<UARManaComponent>();
		if (!Mana) return false;
		Current = Mana->GetCurrent(); Maximum = Mana->GetMax();
	}
	else if (ResourceType == EARResourceType::Stamina)
	{
		const UARStaminaComponent* Stamina = Target->FindComponentByClass<UARStaminaComponent>();
		if (!Stamina) return false;
		Current = Stamina->GetCurrent(); Maximum = Stamina->GetMax();
	}
	else if (ResourceType == EARResourceType::Groggy)
	{
		const UARStaggerComponent* Stagger = Target->FindComponentByClass<UARStaggerComponent>();
		if (!Stagger) return false;
		Current = Stagger->GetCurrentGroggy(); Maximum = Stagger->GetMaxGroggy();
	}
	Ratio = Maximum > 0.0f ? Current / Maximum : 0.0f;
	return true;
}
