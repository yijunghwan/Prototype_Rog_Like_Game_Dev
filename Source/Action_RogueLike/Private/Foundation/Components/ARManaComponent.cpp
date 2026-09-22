#include "Foundation/Components/ARManaComponent.h"

#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"

UARManaComponent::UARManaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UARManaComponent::BeginPlay()
{
	Super::BeginPlay();
	StatsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStatsComponent>() : nullptr;
	if (!StatsComponent)
	{
		UE_LOG(LogARFoundation, Error, TEXT("ManaComponent on %s requires ARStatsComponent."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	StatsComponent->OnFinalStatChanged.AddDynamic(this, &UARManaComponent::HandleFinalStatChanged);
	CurrentMana = bStartFull ? GetMax() : FMath::Clamp(CurrentMana, 0.0f, GetMax());
}

void UARManaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatsComponent)
	{
		StatsComponent->OnFinalStatChanged.RemoveDynamic(this, &UARManaComponent::HandleFinalStatChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UARManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!StatsComponent || CurrentMana >= GetMax())
	{
		return;
	}
	const float Rate = FMath::Max(0.0f, StatsComponent->GetFinalStat(EARStatType::ManaRecoveryPerSecond));
	if (Rate > 0.0f)
	{
		FARSourceInfo Source;
		Source.SourceId = TEXT("System.ManaRegeneration");
		const float Before = CurrentMana;
		CurrentMana = FMath::Min(GetMax(), CurrentMana + Rate * DeltaTime);
		BroadcastChange(CurrentMana - Before, EARResourceChangeReason::Regeneration, Source);
	}
}

bool UARManaComponent::CanAfford(float Amount) const
{
	return FMath::IsFinite(Amount) && Amount >= 0.0f && CurrentMana + KINDA_SMALL_NUMBER >= Amount;
}

bool UARManaComponent::TryConsume(float Amount, const FARSourceInfo& Source, float& NewMana)
{
	if (!CanAfford(Amount))
	{
		NewMana = CurrentMana;
		return false;
	}
	CurrentMana -= Amount;
	NewMana = CurrentMana;
	BroadcastChange(-Amount, EARResourceChangeReason::Consume, Source);
	return true;
}

float UARManaComponent::Restore(float Amount, const FARSourceInfo& Source)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float Before = CurrentMana;
	CurrentMana = FMath::Min(GetMax(), CurrentMana + Amount);
	const float Applied = CurrentMana - Before;
	if (Applied > 0.0f)
	{
		BroadcastChange(Applied, EARResourceChangeReason::Restore, Source);
	}
	return Applied;
}

float UARManaComponent::GetMax() const
{
	return StatsComponent ? StatsComponent->GetFinalStat(EARStatType::MaxMana) : 0.0f;
}

float UARManaComponent::GetRatio() const
{
	const float Maximum = GetMax();
	return Maximum > 0.0f ? CurrentMana / Maximum : 0.0f;
}

void UARManaComponent::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (StatType != EARStatType::MaxMana)
	{
		return;
	}
	const float Before = CurrentMana;
	CurrentMana = FMath::Min(CurrentMana, NewValue);
	FARSourceInfo Source;
	Source.SourceId = TEXT("System.MaxManaChanged");
	BroadcastChange(CurrentMana - Before, EARResourceChangeReason::MaxChanged, Source);
}

void UARManaComponent::BroadcastChange(float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source)
{
	OnResourceChanged.Broadcast(GetOwner(), CurrentMana, GetMax(), Delta, Reason, Source);
}

