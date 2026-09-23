#include "Foundation/Components/ARStatsComponent.h"

#include "Engine/World.h"
#include "Foundation/Core/ARLogChannels.h"

UARStatsComponent::UARStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	BaseStats.Add(EARStatType::MaxHealth, 100.0f);
	BaseStats.Add(EARStatType::MoveSpeed, 600.0f);
	BaseStats.Add(EARStatType::AttackSpeed, 100.0f);
	BaseStats.Add(EARStatType::CriticalDamage, 150.0f);
	BaseStats.Add(EARStatType::MaxStamina, 100.0f);
	BaseStats.Add(EARStatType::StaminaRecoveryPerSecond, 20.0f);
	BaseStats.Add(EARStatType::StaminaRecoveryDelay, 1.0f);
	BaseStats.Add(EARStatType::RollStaminaCost, 20.0f);
	BaseStats.Add(EARStatType::RollDistance, 350.0f);
	BaseStats.Add(EARStatType::RollEvasionDuration, 0.25f);
	BaseStats.Add(EARStatType::MaxMana, 100.0f);
	BaseStats.Add(EARStatType::MaxConsumableSlots, 3.0f);
}

void UARStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	RecalculateAll();
}

void UARStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const double Now = GetNow();
	TArray<FARActiveStatModifier> Expired;
	for (int32 Index = ActiveModifiers.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveModifiers[Index].ExpireAt >= 0.0 && ActiveModifiers[Index].ExpireAt <= Now)
		{
			Expired.Add(ActiveModifiers[Index]);
			ActiveModifiers.RemoveAt(Index);
		}
	}

	for (const FARActiveStatModifier& Modifier : Expired)
	{
		RecalculateStat(Modifier.Spec.StatType);
		BroadcastSourceChange(Modifier.Spec.Source);
	}

	const bool bHasTimedModifier = ActiveModifiers.ContainsByPredicate([](const FARActiveStatModifier& Modifier)
	{
		return Modifier.ExpireAt >= 0.0;
	});
	SetComponentTickEnabled(bHasTimedModifier);
}

FARStatModifierHandle UARStatsComponent::AddStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	FARStatModifierHandle Handle;

	if (!FMath::IsFinite(Spec.Value) || !FMath::IsFinite(Spec.Duration) || Spec.Duration == 0.0f)
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Rejected invalid stat modifier on %s."), *GetNameSafe(GetOwner()));
		return Handle;
	}
	if (Spec.Operation == EARStatModifierOperation::IndependentDamageReduction && !IsReductionStat(Spec.StatType))
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Independent damage reduction is only valid for reduction stats."));
		return Handle;
	}
	if (Spec.bGuaranteeInvulnerability && Spec.StatType != EARStatType::OverallDamageReduction)
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Invulnerability guarantee requires OverallDamageReduction."));
		return Handle;
	}
	if (Spec.bGuaranteeEvasion && Spec.StatType != EARStatType::Evasion)
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Evasion guarantee requires Evasion."));
		return Handle;
	}

	Handle.Id = FGuid::NewGuid();
	FARActiveStatModifier& NewModifier = ActiveModifiers.AddDefaulted_GetRef();
	NewModifier.Handle = Handle;
	NewModifier.Spec = Spec;
	NewModifier.AppliedAt = GetNow();
	NewModifier.ExpireAt = Spec.Duration < 0.0f ? -1.0 : NewModifier.AppliedAt + Spec.Duration;

	RecalculateStat(Spec.StatType);
	BroadcastSourceChange(Spec.Source);
	if (NewModifier.ExpireAt >= 0.0)
	{
		SetComponentTickEnabled(true);
	}
	bSuccess = true;
	return Handle;
}

bool UARStatsComponent::RemoveStatModifier(FARStatModifierHandle Handle)
{
	const int32 Index = ActiveModifiers.IndexOfByPredicate([&Handle](const FARActiveStatModifier& Modifier)
	{
		return Modifier.Handle == Handle;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}

	const FARActiveStatModifier Removed = ActiveModifiers[Index];
	ActiveModifiers.RemoveAt(Index);
	RecalculateStat(Removed.Spec.StatType);
	BroadcastSourceChange(Removed.Spec.Source);
	return true;
}

int32 UARStatsComponent::RemoveModifiers(EARModifierSourceCategory Category, FName SourceId)
{
	TSet<EARStatType> DirtyStats;
	TArray<FARSourceInfo> ChangedSources;
	int32 RemovedCount = 0;
	for (int32 Index = ActiveModifiers.Num() - 1; Index >= 0; --Index)
	{
		const FARActiveStatModifier& Modifier = ActiveModifiers[Index];
		const bool bCategoryMatches = Category == EARModifierSourceCategory::All || Modifier.Spec.Source.Category == Category;
		const bool bIdMatches = SourceId.IsNone() || Modifier.Spec.Source.SourceId == SourceId;
		if (bCategoryMatches && bIdMatches)
		{
			DirtyStats.Add(Modifier.Spec.StatType);
			ChangedSources.Add(Modifier.Spec.Source);
			ActiveModifiers.RemoveAt(Index);
			++RemovedCount;
		}
	}
	for (const EARStatType StatType : DirtyStats)
	{
		RecalculateStat(StatType);
	}
	for (const FARSourceInfo& Source : ChangedSources)
	{
		BroadcastSourceChange(Source);
	}
	return RemovedCount;
}

int32 UARStatsComponent::ClearModifiers(EARModifierSourceCategory Category)
{
	return RemoveModifiers(Category, NAME_None);
}

bool UARStatsComponent::RemoveOneModifierStack(EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle)
{
	int32 SelectedIndex = INDEX_NONE;
	double SelectedTime = Policy == EARModifierStackRemovalPolicy::Newest ? -DBL_MAX : DBL_MAX;
	for (int32 Index = 0; Index < ActiveModifiers.Num(); ++Index)
	{
		const FARActiveStatModifier& Modifier = ActiveModifiers[Index];
		if ((Category == EARModifierSourceCategory::All || Modifier.Spec.Source.Category == Category) && Modifier.Spec.Source.SourceId == SourceId)
		{
			const bool bSelect = Policy == EARModifierStackRemovalPolicy::Newest ? Modifier.AppliedAt > SelectedTime : Modifier.AppliedAt < SelectedTime;
			if (bSelect)
			{
				SelectedIndex = Index;
				SelectedTime = Modifier.AppliedAt;
			}
		}
	}
	if (SelectedIndex == INDEX_NONE)
	{
		return false;
	}
	RemovedHandle = ActiveModifiers[SelectedIndex].Handle;
	return RemoveStatModifier(RemovedHandle);
}

float UARStatsComponent::GetFinalStat(EARStatType StatType) const
{
	if (const float* Value = CachedFinalStats.Find(StatType))
	{
		return *Value;
	}
	return ApplySafetyRules(StatType, CalculateGeneralStat(StatType));
}

float UARStatsComponent::GetBaseStat(EARStatType StatType) const
{
	return BaseStats.FindRef(StatType);
}

FARStatBreakdown UARStatsComponent::GetStatBreakdown(EARStatType StatType) const
{
	FARStatBreakdown Breakdown;
	CalculateGeneralStat(StatType, &Breakdown);
	Breakdown.FinalValue = GetFinalStat(StatType);
	return Breakdown;
}

TArray<FARFinalStatView> UARStatsComponent::GetAllFinalStatViews() const
{
	TArray<FARFinalStatView> Result;
	Result.Reserve(static_cast<int32>(EARStatType::Count));
	const UEnum* StatEnum = StaticEnum<EARStatType>();
	for (int32 Value = 0; Value < static_cast<int32>(EARStatType::Count); ++Value)
	{
		FARFinalStatView& View = Result.AddDefaulted_GetRef();
		View.StatType = static_cast<EARStatType>(Value);
		View.DisplayName = StatEnum ? StatEnum->GetDisplayNameTextByValue(Value) : FText::GetEmpty();
		View.Breakdown = GetStatBreakdown(View.StatType);
	}
	return Result;
}

FARStatModifierQueryResult UARStatsComponent::GetModifiersBySource(EARModifierSourceCategory Category, FName SourceId) const
{
	FARStatModifierQueryResult Result;
	const double Now = GetNow();
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if ((Category == EARModifierSourceCategory::All || Modifier.Spec.Source.Category == Category) && Modifier.Spec.Source.SourceId == SourceId)
		{
			Result.bExists = true;
			++Result.StackCount;
			if (Modifier.ExpireAt < 0.0)
			{
				Result.bHasPermanent = true;
				Result.LongestRemainingTime = -1.0f;
			}
			else if (!Result.bHasPermanent)
			{
				Result.LongestRemainingTime = FMath::Max(Result.LongestRemainingTime, static_cast<float>(Modifier.ExpireAt - Now));
			}
		}
	}
	return Result;
}

bool UARStatsComponent::GetModifierRemainingTime(FARStatModifierHandle Handle, bool& bIsPermanent, float& RemainingSeconds) const
{
	const FARActiveStatModifier* Modifier = ActiveModifiers.FindByPredicate([&Handle](const FARActiveStatModifier& Candidate)
	{
		return Candidate.Handle == Handle;
	});
	if (!Modifier)
	{
		bIsPermanent = false;
		RemainingSeconds = 0.0f;
		return false;
	}
	bIsPermanent = Modifier->ExpireAt < 0.0;
	RemainingSeconds = bIsPermanent ? -1.0f : FMath::Max(0.0f, static_cast<float>(Modifier->ExpireAt - GetNow()));
	return true;
}

float UARStatsComponent::GetDamageRemainingMultiplier(EARStatType ReductionStat) const
{
	if (!IsReductionStat(ReductionStat))
	{
		return 1.0f;
	}

	float Flat = BaseStats.FindRef(ReductionStat);
	float AdditivePercent = 0.0f;
	float Multiplicative = 1.0f;
	TArray<float> IndependentValues;
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if (Modifier.Spec.StatType != ReductionStat)
		{
			continue;
		}
		switch (Modifier.Spec.Operation)
		{
		case EARStatModifierOperation::Flat: Flat += Modifier.Spec.Value; break;
		case EARStatModifierOperation::AdditivePercent: AdditivePercent += Modifier.Spec.Value; break;
		case EARStatModifierOperation::Multiplicative: Multiplicative *= Modifier.Spec.Value; break;
		case EARStatModifierOperation::IndependentDamageReduction: IndependentValues.Add(Modifier.Spec.Value); break;
		default: break;
		}
	}
	const float BaseReduction = Flat * (1.0f + AdditivePercent / 100.0f) * Multiplicative;
	float Remaining = FMath::Max(0.0f, 1.0f - BaseReduction / 100.0f);
	for (const float IndependentValue : IndependentValues)
	{
		Remaining *= FMath::Max(0.0f, 1.0f - IndependentValue / 100.0f);
	}
	return Remaining;
}

bool UARStatsComponent::HasGuaranteedInvulnerability() const
{
	return ActiveModifiers.ContainsByPredicate([](const FARActiveStatModifier& Modifier)
	{
		return Modifier.Spec.bGuaranteeInvulnerability;
	});
}

bool UARStatsComponent::HasGuaranteedEvasion() const
{
	return ActiveModifiers.ContainsByPredicate([](const FARActiveStatModifier& Modifier)
	{
		return Modifier.Spec.bGuaranteeEvasion;
	});
}

void UARStatsComponent::SetBaseStat(EARStatType StatType, float Value)
{
	BaseStats.FindOrAdd(StatType) = Value;
	RecalculateStat(StatType);
}

void UARStatsComponent::RecalculateAll()
{
	for (uint8 Index = 0; Index < static_cast<uint8>(EARStatType::Count); ++Index)
	{
		RecalculateStat(static_cast<EARStatType>(Index));
	}
}

void UARStatsComponent::RecalculateStat(EARStatType StatType)
{
	const float OldValue = CachedFinalStats.FindRef(StatType);
	const float NewValue = IsReductionStat(StatType)
		? (1.0f - GetDamageRemainingMultiplier(StatType)) * 100.0f
		: ApplySafetyRules(StatType, CalculateGeneralStat(StatType));
	CachedFinalStats.FindOrAdd(StatType) = NewValue;
	if (!FMath::IsNearlyEqual(OldValue, NewValue))
	{
		OnFinalStatChanged.Broadcast(GetOwner(), StatType, OldValue, NewValue);
	}
}

float UARStatsComponent::CalculateGeneralStat(EARStatType StatType, FARStatBreakdown* OutBreakdown) const
{
	const float BaseValue = BaseStats.FindRef(StatType);
	float FlatTotal = 0.0f;
	float AdditiveTotal = 0.0f;
	float MultiplicativeProduct = 1.0f;
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if (Modifier.Spec.StatType != StatType)
		{
			continue;
		}
		switch (Modifier.Spec.Operation)
		{
		case EARStatModifierOperation::Flat: FlatTotal += Modifier.Spec.Value; break;
		case EARStatModifierOperation::AdditivePercent: AdditiveTotal += Modifier.Spec.Value; break;
		case EARStatModifierOperation::Multiplicative: MultiplicativeProduct *= Modifier.Spec.Value; break;
		default: break;
		}
	}
	const float Result = (BaseValue + FlatTotal) * (1.0f + AdditiveTotal / 100.0f) * MultiplicativeProduct;
	if (OutBreakdown)
	{
		OutBreakdown->BaseValue = BaseValue;
		OutBreakdown->FlatTotal = FlatTotal;
		OutBreakdown->AdditivePercentTotal = AdditiveTotal;
		OutBreakdown->MultiplicativeProduct = MultiplicativeProduct;
	}
	return Result;
}

float UARStatsComponent::ApplySafetyRules(EARStatType StatType, float Value) const
{
	if (!FMath::IsFinite(Value))
	{
		return 0.0f;
	}
	switch (StatType)
	{
	case EARStatType::MaxHealth: return FMath::Max(1.0f, Value);
	case EARStatType::MaxStamina:
	case EARStatType::MaxMana:
	case EARStatType::MaxGroggy:
	case EARStatType::PhysicalDefense:
	case EARStatType::FireDefense:
	case EARStatType::MagicDefense:
	case EARStatType::PhysicalDamageTakenIncrease:
	case EARStatType::FireDamageTakenIncrease:
	case EARStatType::MagicDamageTakenIncrease:
	case EARStatType::MoveSpeed:
	case EARStatType::CriticalDamage:
	case EARStatType::RollStaminaCost:
		return FMath::Max(0.0f, Value);
	case EARStatType::AttackSpeed: return FMath::Max(1.0f, Value);
	case EARStatType::Evasion: return FMath::Clamp(Value, 0.0f, FMath::Min(MaxEvasion, 100.0f));
	case EARStatType::CriticalChance: return FMath::Clamp(Value, 0.0f, FMath::Min(MaxCriticalChance, 100.0f));
	case EARStatType::Tenacity: return FMath::Clamp(Value, 0.0f, FMath::Min(MaxTenacity, 100.0f));
	case EARStatType::CooldownReduction: return FMath::Clamp(Value, 0.0f, FMath::Min(MaxCooldownReduction, 100.0f));
	case EARStatType::MaxConsumableSlots: return FMath::FloorToFloat(FMath::Max(0.0f, Value));
	default: return Value;
	}
}

void UARStatsComponent::BroadcastSourceChange(const FARSourceInfo& Source)
{
	const FARStatModifierQueryResult Query = GetModifiersBySource(Source.Category, Source.SourceId);
	OnStatModifiersChanged.Broadcast(GetOwner(), Source.SourceId, Source.DisplayName, Query.StackCount, Query.LongestRemainingTime);
}

double UARStatsComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

bool UARStatsComponent::IsReductionStat(EARStatType StatType)
{
	return StatType == EARStatType::OverallDamageReduction
		|| StatType == EARStatType::PhysicalDamageReduction
		|| StatType == EARStatType::FireDamageReduction
		|| StatType == EARStatType::MagicDamageReduction;
}
