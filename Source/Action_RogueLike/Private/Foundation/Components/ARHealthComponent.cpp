#include "Foundation/Components/ARHealthComponent.h"

#include "Engine/World.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"

UARHealthComponent::UARHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UARHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	StatsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStatsComponent>() : nullptr;
	if (!StatsComponent)
	{
		UE_LOG(LogARFoundation, Error, TEXT("HealthComponent on %s requires ARStatsComponent."), *GetNameSafe(GetOwner()));
		return;
	}

	StatsComponent->OnFinalStatChanged.AddDynamic(this, &UARHealthComponent::HandleFinalStatChanged);
	CurrentHealth = bStartAtFullHealth ? GetMaxHealth() : FMath::Clamp(CurrentHealth, 0.0f, GetMaxHealth());
}

void UARHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatsComponent)
	{
		StatsComponent->OnFinalStatChanged.RemoveDynamic(this, &UARHealthComponent::HandleFinalStatChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UARHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const double Now = GetNow();
	for (int32 Index = ActiveShields.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveShields[Index].ExpireAt >= 0.0 && ActiveShields[Index].ExpireAt <= Now)
		{
			const FARActiveShield Expired = ActiveShields[Index];
			ActiveShields.RemoveAt(Index);
			OnShieldChanged.Broadcast(GetOwner(), GetCurrentShield(), -Expired.RemainingAmount, Expired.Handle, EARResourceChangeReason::Other);
		}
	}
	RefreshShieldTickState();
}

bool UARHealthComponent::CanAcceptResolvedDamage() const
{
	return bDamageSystemEnabled && !bDead && StatsComponent != nullptr;
}

bool UARHealthComponent::ApplyResolvedDamage(FARCombatDamageResult& InOutResult)
{
	if (!CanAcceptResolvedDamage() || InOutResult.Outcome != EARDamageOutcome::Applied || InOutResult.FinalDamage <= 0)
	{
		return false;
	}

	const float ShieldBefore = GetCurrentShield();
	const float RequestedShieldDamage = FMath::Max(0, InOutResult.ShieldDamage);
	const float ActualShieldDamage = ConsumeShieldLifo(RequestedShieldDamage);
	const float HealthBefore = CurrentHealth;
	const float RequestedHealthDamage = FMath::Max(0, InOutResult.HealthDamage);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - RequestedHealthDamage);
	const float ActualHealthDamage = HealthBefore - CurrentHealth;

	InOutResult.ShieldDamage = FMath::FloorToInt(ActualShieldDamage);
	InOutResult.HealthDamage = FMath::FloorToInt(ActualHealthDamage);
	InOutResult.FinalDamage = InOutResult.ShieldDamage + InOutResult.HealthDamage;
	InOutResult.bKilledTarget = !bDead && CurrentHealth <= 0.0f;

	if (!FMath::IsNearlyEqual(ShieldBefore, GetCurrentShield()))
	{
		OnShieldChanged.Broadcast(GetOwner(), GetCurrentShield(), -ActualShieldDamage, FARShieldHandle(), EARResourceChangeReason::Damage);
	}
	if (!FMath::IsNearlyEqual(HealthBefore, CurrentHealth))
	{
		OnHealthChanged.Broadcast(GetOwner(), CurrentHealth, GetMaxHealth(), -ActualHealthDamage, EARResourceChangeReason::Damage);
	}

	OnDamageApplied.Broadcast(GetOwner(), InOutResult);
	OnDamageAppliedNative.Broadcast(GetOwner(), InOutResult);
	if (InOutResult.bKilledTarget)
	{
		bDead = true;
		OnDeath.Broadcast(GetOwner(), InOutResult);
	}
	return true;
}

void UARHealthComponent::PreviewDamageAllocation(int32 FinalDamage, bool bIgnoreShield, int32& OutShieldDamage, int32& OutHealthDamage) const
{
	const int32 SafeDamage = FMath::Max(0, FinalDamage);
	OutShieldDamage = bIgnoreShield ? 0 : FMath::Min(SafeDamage, FMath::FloorToInt(GetCurrentShield()));
	OutHealthDamage = FMath::Min(SafeDamage - OutShieldDamage, FMath::CeilToInt(CurrentHealth));
}

float UARHealthComponent::RestoreHealth(float Amount, const FARSourceInfo& Source)
{
	if (bDead || !FMath::IsFinite(Amount) || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float Before = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, GetMaxHealth());
	const float Applied = CurrentHealth - Before;
	if (Applied > 0.0f)
	{
		OnHealthChanged.Broadcast(GetOwner(), CurrentHealth, GetMaxHealth(), Applied, EARResourceChangeReason::Restore);
	}
	return Applied;
}

FARShieldHandle UARHealthComponent::ApplyShield(const FARShieldSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	FARShieldHandle Handle;
	if (!FMath::IsFinite(Spec.Amount) || !FMath::IsFinite(Spec.Duration) || Spec.Amount <= 0.0f || Spec.Duration == 0.0f)
	{
		return Handle;
	}

	Handle.Id = FGuid::NewGuid();
	FARActiveShield& Shield = ActiveShields.AddDefaulted_GetRef();
	Shield.Handle = Handle;
	Shield.Source = Spec.Source;
	Shield.RemainingAmount = Spec.Amount;
	Shield.AppliedAt = GetNow();
	Shield.ExpireAt = Spec.Duration < 0.0f ? -1.0 : Shield.AppliedAt + Spec.Duration;
	OnShieldChanged.Broadcast(GetOwner(), GetCurrentShield(), Spec.Amount, Handle, EARResourceChangeReason::Restore);
	RefreshShieldTickState();
	bSuccess = true;
	return Handle;
}

bool UARHealthComponent::RemoveShield(FARShieldHandle Handle, float& RemovedAmount)
{
	RemovedAmount = 0.0f;
	const int32 Index = ActiveShields.IndexOfByPredicate([&Handle](const FARActiveShield& Shield)
	{
		return Shield.Handle == Handle;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}
	RemovedAmount = ActiveShields[Index].RemainingAmount;
	ActiveShields.RemoveAt(Index);
	OnShieldChanged.Broadcast(GetOwner(), GetCurrentShield(), -RemovedAmount, Handle, EARResourceChangeReason::Other);
	RefreshShieldTickState();
	return true;
}

int32 UARHealthComponent::RemoveShieldsBySource(EARModifierSourceCategory Category, FName SourceId, EARShieldLifetimeFilter LifetimeFilter, float& RemovedAmount)
{
	RemovedAmount = 0.0f;
	int32 RemovedCount = 0;
	for (int32 Index = ActiveShields.Num() - 1; Index >= 0; --Index)
	{
		const FARActiveShield& Shield = ActiveShields[Index];
		const bool bCategoryMatches = Category == EARModifierSourceCategory::All || Shield.Source.Category == Category;
		const bool bIdMatches = SourceId.IsNone() || Shield.Source.SourceId == SourceId;
		const bool bLifetimeMatches = LifetimeFilter == EARShieldLifetimeFilter::All
			|| (LifetimeFilter == EARShieldLifetimeFilter::TimedOnly && Shield.ExpireAt >= 0.0)
			|| (LifetimeFilter == EARShieldLifetimeFilter::PermanentOnly && Shield.ExpireAt < 0.0);
		if (bCategoryMatches && bIdMatches && bLifetimeMatches)
		{
			RemovedAmount += Shield.RemainingAmount;
			ActiveShields.RemoveAt(Index);
			++RemovedCount;
		}
	}
	if (RemovedCount > 0)
	{
		OnShieldChanged.Broadcast(GetOwner(), GetCurrentShield(), -RemovedAmount, FARShieldHandle(), EARResourceChangeReason::Other);
	}
	RefreshShieldTickState();
	return RemovedCount;
}

int32 UARHealthComponent::ClearShields(float& RemovedAmount)
{
	return RemoveShieldsBySource(EARModifierSourceCategory::All, NAME_None, EARShieldLifetimeFilter::All, RemovedAmount);
}

float UARHealthComponent::GetMaxHealth() const
{
	return StatsComponent ? StatsComponent->GetFinalStat(EARStatType::MaxHealth) : 0.0f;
}

float UARHealthComponent::GetHealthRatio() const
{
	const float MaxHealth = GetMaxHealth();
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

float UARHealthComponent::GetCurrentShield() const
{
	float Total = 0.0f;
	for (const FARActiveShield& Shield : ActiveShields)
	{
		Total += Shield.RemainingAmount;
	}
	return Total;
}

void UARHealthComponent::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (StatType != EARStatType::MaxHealth)
	{
		return;
	}
	const float Before = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth, NewValue);
	OnHealthChanged.Broadcast(GetOwner(), CurrentHealth, NewValue, CurrentHealth - Before, EARResourceChangeReason::MaxChanged);
}

float UARHealthComponent::ConsumeShieldLifo(float Amount)
{
	float RemainingDamage = FMath::Max(0.0f, Amount);
	float Consumed = 0.0f;
	for (int32 Index = ActiveShields.Num() - 1; Index >= 0 && RemainingDamage > 0.0f; --Index)
	{
		FARActiveShield& Shield = ActiveShields[Index];
		const float ThisDamage = FMath::Min(Shield.RemainingAmount, RemainingDamage);
		Shield.RemainingAmount -= ThisDamage;
		RemainingDamage -= ThisDamage;
		Consumed += ThisDamage;
		if (Shield.RemainingAmount <= KINDA_SMALL_NUMBER)
		{
			ActiveShields.RemoveAt(Index);
		}
	}
	RefreshShieldTickState();
	return Consumed;
}

void UARHealthComponent::RefreshShieldTickState()
{
	const bool bHasTimedShield = ActiveShields.ContainsByPredicate([](const FARActiveShield& Shield)
	{
		return Shield.ExpireAt >= 0.0;
	});
	SetComponentTickEnabled(bHasTimedShield);
}

double UARHealthComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
