#include "Foundation/Components/ARStaminaComponent.h"

#include "Engine/World.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"

UARStaminaComponent::UARStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UARStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
	StatsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStatsComponent>() : nullptr;
	if (!StatsComponent)
	{
		UE_LOG(LogARFoundation, Error, TEXT("StaminaComponent on %s requires ARStatsComponent."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	StatsComponent->OnFinalStatChanged.AddDynamic(this, &UARStaminaComponent::HandleFinalStatChanged);
	CurrentStamina = bStartFull ? GetMax() : FMath::Clamp(CurrentStamina, 0.0f, GetMax());
}

void UARStaminaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatsComponent)
	{
		StatsComponent->OnFinalStatChanged.RemoveDynamic(this, &UARStaminaComponent::HandleFinalStatChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UARStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!StatsComponent || CurrentStamina >= GetMax())
	{
		return;
	}
	const float Delay = FMath::Max(0.0f, StatsComponent->GetFinalStat(EARStatType::StaminaRecoveryDelay));
	if (LastSuccessfulConsumeAt >= 0.0 && GetNow() < LastSuccessfulConsumeAt + Delay)
	{
		return;
	}
	const float Rate = FMath::Max(0.0f, StatsComponent->GetFinalStat(EARStatType::StaminaRecoveryPerSecond));
	if (Rate > 0.0f)
	{
		FARSourceInfo Source;
		Source.SourceId = TEXT("System.StaminaRegeneration");
		const float Before = CurrentStamina;
		CurrentStamina = FMath::Min(GetMax(), CurrentStamina + Rate * DeltaTime);
		BroadcastChange(CurrentStamina - Before, EARResourceChangeReason::Regeneration, Source);
	}
}

bool UARStaminaComponent::CanAfford(float Amount) const
{
	return FMath::IsFinite(Amount) && Amount >= 0.0f && CurrentStamina + KINDA_SMALL_NUMBER >= Amount;
}

bool UARStaminaComponent::TryConsume(float Amount, const FARSourceInfo& Source, float& NewStamina)
{
	if (!CanAfford(Amount))
	{
		NewStamina = CurrentStamina;
		return false;
	}
	CurrentStamina -= Amount;
	LastSuccessfulConsumeAt = GetNow();
	NewStamina = CurrentStamina;
	BroadcastChange(-Amount, EARResourceChangeReason::Consume, Source);
	return true;
}

float UARStaminaComponent::Restore(float Amount, const FARSourceInfo& Source)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float Before = CurrentStamina;
	CurrentStamina = FMath::Min(GetMax(), CurrentStamina + Amount);
	const float Applied = CurrentStamina - Before;
	if (Applied > 0.0f)
	{
		BroadcastChange(Applied, EARResourceChangeReason::Restore, Source);
	}
	return Applied;
}

float UARStaminaComponent::GetMax() const
{
	return StatsComponent ? StatsComponent->GetFinalStat(EARStatType::MaxStamina) : 0.0f;
}

float UARStaminaComponent::GetRatio() const
{
	const float Maximum = GetMax();
	return Maximum > 0.0f ? CurrentStamina / Maximum : 0.0f;
}

void UARStaminaComponent::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (StatType != EARStatType::MaxStamina)
	{
		return;
	}
	const float Before = CurrentStamina;
	CurrentStamina = FMath::Min(CurrentStamina, NewValue);
	FARSourceInfo Source;
	Source.SourceId = TEXT("System.MaxStaminaChanged");
	BroadcastChange(CurrentStamina - Before, EARResourceChangeReason::MaxChanged, Source);
}

void UARStaminaComponent::BroadcastChange(float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source)
{
	OnResourceChanged.Broadcast(GetOwner(), CurrentStamina, GetMax(), Delta, Reason, Source);
}

double UARStaminaComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

