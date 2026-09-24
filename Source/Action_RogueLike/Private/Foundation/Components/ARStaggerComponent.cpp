#include "Foundation/Components/ARStaggerComponent.h"

#include "Engine/World.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"

UARStaggerComponent::UARStaggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UARStaggerComponent::BeginPlay()
{
	Super::BeginPlay();
	StatsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStatsComponent>() : nullptr;
	if (!StatsComponent)
	{
		UE_LOG(LogARFoundation, Error, TEXT("StaggerComponent on %s requires ARStatsComponent."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	StatsComponent->OnFinalStatChanged.AddDynamic(this, &UARStaggerComponent::HandleFinalStatChanged);
	CurrentGroggy = bUseGroggyGauge ? GetMaxGroggy() : 0.0f;
}

void UARStaggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StatsComponent)
	{
		StatsComponent->OnFinalStatChanged.RemoveDynamic(this, &UARStaggerComponent::HandleFinalStatChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UARStaggerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Now = GetNow();

	const bool bSuperArmorBefore = IsSuperArmorActive();
	for (int32 Index = ActiveSuperArmor.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveSuperArmor[Index].ExpireAt >= 0.0 && ActiveSuperArmor[Index].ExpireAt <= Now)
		{
			ActiveSuperArmor.RemoveAt(Index);
		}
	}
	if (bSuperArmorBefore != IsSuperArmorActive())
	{
		OnSuperArmorChanged.Broadcast(GetOwner(), IsSuperArmorActive());
	}

	const bool bStaggeredNow = IsStaggered();
	if (bWasStaggeredLastTick && !bStaggeredNow)
	{
		StaggerImmuneUntil = Now + PostStaggerImmunityDuration;
		OnStaggerStateChangedNative.Broadcast(GetOwner(), false);
		OnStaggerStateChanged.Broadcast(GetOwner(), false);
	}
	bWasStaggeredLastTick = bStaggeredNow;

	if (bUseGroggyGauge && !bGroggyDepletedLocked && CurrentGroggy < GetMaxGroggy())
	{
		const float Delay = FMath::Max(0.0f, StatsComponent->GetFinalStat(EARStatType::GroggyRecoveryDelay));
		if (LastGroggyDamageAt < 0.0 || Now >= LastGroggyDamageAt + Delay)
		{
			const float Rate = FMath::Max(0.0f, StatsComponent->GetFinalStat(EARStatType::GroggyRecoveryPerSecond));
			const float Before = CurrentGroggy;
			CurrentGroggy = FMath::Min(GetMaxGroggy(), CurrentGroggy + Rate * DeltaTime);
			if (!FMath::IsNearlyEqual(Before, CurrentGroggy))
			{
				OnGroggyChanged.Broadcast(GetOwner(), CurrentGroggy, GetMaxGroggy(), CurrentGroggy - Before);
			}
		}
	}
}

FARStaggerResult UARStaggerComponent::ApplyStaggerAndGroggyDamage(const FARStaggerRequest& Request)
{
	FARStaggerResult Result;
	bool bBroadcastStaggered = false;
	if (!StatsComponent || !Request.HitContext.IsUsable() || Request.HitContext.Target != GetOwner())
	{
		return Result;
	}
	Result.bValidRequest = true;

	const float StaggerPowerMultiplier = FMath::Max(0.0f, 1.0f + Request.HitContext.StaggerPowerSnapshot / 100.0f);
	const float RawStagger = Request.Template.BaseStaggerDamage * StaggerPowerMultiplier * FMath::Max(0.0f, Request.Template.StaggerMultiplier);
	Result.FinalStaggerDamage = FMath::Max(0, FMath::FloorToInt(RawStagger));
	Result.bStaggerAttempted = Result.FinalStaggerDamage > 0;
	if (Result.bStaggerAttempted && bCanBeStaggered)
	{
		Result.bBlockedBySuperArmor = IsSuperArmorActive();
		const float Resistance = StatsComponent->GetFinalStat(EARStatType::StaggerResistance);
		if (!Result.bBlockedBySuperArmor && !IsStaggerImmune() && Result.FinalStaggerDamage > Resistance)
		{
			const bool bAlreadyStaggered = IsStaggered();
			StaggeredUntil = FMath::Max(StaggeredUntil, GetNow() + BaseStaggerDuration);
			Result.bStaggered = true;
			bWasStaggeredLastTick = true;
			if (!bAlreadyStaggered)
			{
				OnStaggerStateChangedNative.Broadcast(GetOwner(), true);
				OnStaggerStateChanged.Broadcast(GetOwner(), true);
			}
			bBroadcastStaggered = true;
		}
	}

	const float GroggyAmplification = FMath::Max(0.0f, 1.0f + Request.HitContext.GroggyDamageAmplificationSnapshot / 100.0f);
	const float RawGroggy = Request.Template.BaseGroggyDamage * StaggerPowerMultiplier * GroggyAmplification;
	Result.FinalGroggyDamage = FMath::Max(0, FMath::FloorToInt(RawGroggy));
	if (bUseGroggyGauge && !bGroggyDepletedLocked && Result.FinalGroggyDamage > 0)
	{
		const float Before = CurrentGroggy;
		CurrentGroggy = FMath::Max(0.0f, CurrentGroggy - Result.FinalGroggyDamage);
		LastGroggyDamageAt = GetNow();
		OnGroggyChanged.Broadcast(GetOwner(), CurrentGroggy, GetMaxGroggy(), CurrentGroggy - Before);
		if (Before > 0.0f && CurrentGroggy <= 0.0f)
		{
			bGroggyDepletedLocked = true;
			Result.bGroggyDepleted = true;
			OnGroggyGaugeDepleted.Broadcast(GetOwner());
		}
	}
	Result.CurrentGroggy = CurrentGroggy;
	if (bBroadcastStaggered)
	{
		OnStaggeredNative.Broadcast(GetOwner(), Result);
		OnStaggered.Broadcast(GetOwner(), Result);
	}
	return Result;
}

FARSuperArmorHandle UARStaggerComponent::AddSuperArmor(const FARSuperArmorSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	FARSuperArmorHandle Handle;
	if (!FMath::IsFinite(Spec.Duration) || Spec.Duration == 0.0f)
	{
		return Handle;
	}
	const bool bWasActive = IsSuperArmorActive();
	Handle.Id = FGuid::NewGuid();
	FARActiveSuperArmor& Armor = ActiveSuperArmor.AddDefaulted_GetRef();
	Armor.Handle = Handle;
	Armor.Source = Spec.Source;
	Armor.ExpireAt = Spec.Duration < 0.0f ? -1.0 : GetNow() + Spec.Duration;
	bSuccess = true;
	if (!bWasActive)
	{
		OnSuperArmorChanged.Broadcast(GetOwner(), true);
	}
	return Handle;
}

bool UARStaggerComponent::RemoveSuperArmor(FARSuperArmorHandle Handle)
{
	const bool bWasActive = IsSuperArmorActive();
	const int32 Removed = ActiveSuperArmor.RemoveAll([&Handle](const FARActiveSuperArmor& Armor)
	{
		return Armor.Handle == Handle;
	});
	if (bWasActive && !IsSuperArmorActive())
	{
		OnSuperArmorChanged.Broadcast(GetOwner(), false);
	}
	return Removed > 0;
}

int32 UARStaggerComponent::RemoveSuperArmorBySource(EARModifierSourceCategory Category, FName SourceId)
{
	const bool bWasActive = IsSuperArmorActive();
	const int32 Removed = ActiveSuperArmor.RemoveAll([Category, SourceId](const FARActiveSuperArmor& Armor)
	{
		return (Category == EARModifierSourceCategory::All || Armor.Source.Category == Category)
			&& (SourceId.IsNone() || Armor.Source.SourceId == SourceId);
	});
	if (bWasActive && !IsSuperArmorActive())
	{
		OnSuperArmorChanged.Broadcast(GetOwner(), false);
	}
	return Removed;
}

bool UARStaggerComponent::IsStaggered() const
{
	return StaggeredUntil > GetNow();
}

bool UARStaggerComponent::IsStaggerImmune() const
{
	return StaggerImmuneUntil > GetNow();
}

float UARStaggerComponent::GetMaxGroggy() const
{
	return StatsComponent ? StatsComponent->GetFinalStat(EARStatType::MaxGroggy) : 0.0f;
}

float UARStaggerComponent::ResetGroggyGauge(float NewCurrentValue)
{
	if (!bUseGroggyGauge)
	{
		return 0.0f;
	}
	const float Before = CurrentGroggy;
	CurrentGroggy = NewCurrentValue < 0.0f ? GetMaxGroggy() : FMath::Clamp(NewCurrentValue, 0.0f, GetMaxGroggy());
	bGroggyDepletedLocked = false;
	LastGroggyDamageAt = -1.0;
	OnGroggyChanged.Broadcast(GetOwner(), CurrentGroggy, GetMaxGroggy(), CurrentGroggy - Before);
	return CurrentGroggy;
}

void UARStaggerComponent::HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue)
{
	if (StatType == EARStatType::MaxGroggy && bUseGroggyGauge)
	{
		const float Before = CurrentGroggy;
		CurrentGroggy = FMath::Min(CurrentGroggy, NewValue);
		OnGroggyChanged.Broadcast(GetOwner(), CurrentGroggy, NewValue, CurrentGroggy - Before);
	}
}

double UARStaggerComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
