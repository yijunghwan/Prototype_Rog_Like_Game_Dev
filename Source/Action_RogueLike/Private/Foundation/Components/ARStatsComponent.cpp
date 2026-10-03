#include "Foundation/Components/ARStatsComponent.h"

#include "Engine/World.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Characters/ARPlayerCharacter.h"

UARStatsComponent::FScopedNotifications::FScopedNotifications(UARStatsComponent* InStats) : Stats(InStats)
{
	if (Stats) ++Stats->NotificationScopeDepth;
}

UARStatsComponent::FScopedNotifications::~FScopedNotifications()
{
	if (Stats && --Stats->NotificationScopeDepth == 0)
	{
		Stats->RefreshTickState();
		Stats->FlushNotifications();
	}
}

bool UARStatsComponent::CanMutate() const
{
	// Standalone, unregistered components are also used for stat calculations.
	const AActor* Owner = GetOwner();
	return !bEndingPlay && IsValid(this) && (!Owner || (IsValid(Owner) && !Owner->IsActorBeingDestroyed()));
}

void UARStatsComponent::RefreshTickState()
{
	SetComponentTickEnabled(!bEndingPlay && (!PendingNotifications.IsEmpty() || ActiveModifiers.ContainsByPredicate(
		[](const FARActiveStatModifier& Modifier) { return Modifier.ExpireAt >= 0.0; })));
}

void UARStatsComponent::FlushNotifications()
{
	if (NotificationScopeDepth != 0 || bDispatchingNotifications) return;
	TGuardValue<bool> DispatchGuard(bDispatchingNotifications, true);
	// A content callback that endlessly changes a stat must not recurse or hang a frame.
	constexpr int32 MaxNotificationsPerFlush = 1024;
	int32 Delivered = 0;
	while (!PendingNotifications.IsEmpty() && CanMutate() && Delivered < MaxNotificationsPerFlush)
	{
		// Never retain an array/map reference across a Blueprint callback.
		const FPendingNotification Notification = MoveTemp(PendingNotifications[0]);
		PendingNotifications.RemoveAt(0);
		++Delivered;
		if (Notification.bSource)
		{
			const FARStatModifierQueryResult Query = GetModifiersBySource(Notification.Source.Category, Notification.Source.SourceId);
			OnStatModifiersChanged.Broadcast(GetOwner(), Notification.Source.SourceId, Notification.Source.DisplayName,
				Query.StackCount, Query.LongestRemainingTime);
		}
		else
		{
			const float NewValue = GetFinalStat(Notification.StatType);
			if (!FMath::IsNearlyEqual(Notification.OldValue, NewValue))
				OnFinalStatChanged.Broadcast(GetOwner(), Notification.StatType, Notification.OldValue, NewValue);
		}
	}
	if (!CanMutate()) PendingNotifications.Reset();
	if (!PendingNotifications.IsEmpty() && !bReportedNotificationLoop)
	{
		bReportedNotificationLoop = true;
		UE_LOG(LogARFoundation, Warning, TEXT("Stat notification budget reached on %s. Check self-triggering stat callbacks; remaining notifications are deferred."), *GetNameSafe(GetOwner()));
	}
	if (PendingNotifications.IsEmpty()) bReportedNotificationLoop = false;
	RefreshTickState();
}

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
	const FScopedNotifications Notifications(this);
	if (IsStatSupported(EARStatType::Money))
	{
		float& InitialMoney = BaseStats.FindOrAdd(EARStatType::Money);
		if (!FMath::IsFinite(InitialMoney) || InitialMoney < 0.0f) InitialMoney = 0.0f;
	}
	else BaseStats.Remove(EARStatType::Money);
	RecalculateAll();
}

void UARStatsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	PendingNotifications.Reset();
	ActiveModifiers.Reset();
	SetComponentTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}

void UARStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!CanMutate()) return;
	const FScopedNotifications Notifications(this);

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

	TSet<EARStatType> DirtyStats;
	for (const FARActiveStatModifier& Modifier : Expired)
	{
		if (!Modifier.Spec.bStackOnly) DirtyStats.Add(Modifier.Spec.StatType);
		BroadcastSourceChange(Modifier.Spec.Source);
	}
	for (const EARStatType StatType : DirtyStats) RecalculateStat(StatType);
}

FARStatModifierHandle UARStatsComponent::AddStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	FARStatModifierHandle Handle;
	if (!CanMutate()) return Handle;
	const FScopedNotifications Notifications(this);
	// Input can itself alias an active modifier; take a copy before array growth/removal.
	const FARStatModifierSpec SafeSpec = Spec;

	// Owned item/action paths use this method. Permanent edits must never enter
	// their reversible handle lists (including data-asset default modifiers).
	if (Spec.Operation == EARStatModifierOperation::PermanentFlat || (!Spec.bStackOnly && !IsStatSupported(Spec.StatType)))
	{
		return Handle;
	}

	if (!FMath::IsFinite(Spec.Value) || !FMath::IsFinite(Spec.Duration) || Spec.Duration == 0.0f)
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Rejected invalid stat modifier on %s."), *GetNameSafe(GetOwner()));
		return Handle;
	}
	if (!Spec.bStackOnly && Spec.Operation == EARStatModifierOperation::IndependentDamageReduction && !IsReductionStat(Spec.StatType))
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Independent damage reduction is only valid for reduction stats."));
		return Handle;
	}
	if (Spec.bStackOnly && (Spec.bGuaranteeInvulnerability || Spec.bGuaranteeEvasion))
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Stack-only effects cannot grant invulnerability or evasion."));
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
	if ((Spec.bStackOnly || Spec.HUDDisplay != EARStatEffectDisplay::Hidden) && Spec.Source.SourceId.IsNone())
	{
		UE_LOG(LogARFoundation, Warning, TEXT("Stack and HUD effects require a Source Id on %s."), *GetNameSafe(GetOwner()));
		return Handle;
	}

	const FARActiveStatModifier* GroupMember = nullptr;
	if (Spec.StackGroupHandle.IsValid())
	{
		GroupMember = ActiveModifiers.FindByPredicate([&Spec](const FARActiveStatModifier& Modifier)
		{
			return Modifier.StackGroupId == Spec.StackGroupHandle.Id;
		});
		if (!GroupMember || GroupMember->Spec.Source.Category != Spec.Source.Category
			|| GroupMember->Spec.Source.SourceId != Spec.Source.SourceId)
		{
			UE_LOG(LogARFoundation, Warning, TEXT("Invalid stat effect stack group on %s."), *GetNameSafe(GetOwner()));
			return Handle;
		}
	}
	else if (Spec.bAffectedByTenacity && Spec.Duration > 0.0f
		&& GetFinalStat(EARStatType::Tenacity) >= 100.0f)
	{
		return Handle;
	}

	const FGuid ExistingGroupId = GroupMember ? GroupMember->StackGroupId : FGuid();
	const double ExistingGroupExpiry = GroupMember ? GroupMember->ExpireAt : -1.0;
	const float EffectiveDuration = Spec.bAffectedByTenacity && Spec.Duration > 0.0f
		? Spec.Duration * (1.0f - FMath::Clamp(GetFinalStat(EARStatType::Tenacity), 0.0f, 100.0f) / 100.0f)
		: Spec.Duration;
	Handle.Id = FGuid::NewGuid();
	FARActiveStatModifier& NewModifier = ActiveModifiers.AddDefaulted_GetRef();
	NewModifier.Handle = Handle;
	NewModifier.Spec = SafeSpec;
	NewModifier.AppliedAt = GetNow();
	NewModifier.StackGroupId = GroupMember ? ExistingGroupId : Handle.Id;
	NewModifier.ExpireAt = GroupMember ? ExistingGroupExpiry
		: (EffectiveDuration < 0.0f ? -1.0 : NewModifier.AppliedAt + EffectiveDuration);

	if (!SafeSpec.bStackOnly) RecalculateStat(SafeSpec.StatType);
	BroadcastSourceChange(SafeSpec.Source);
	bSuccess = true;
	return Handle;
}

bool UARStatsComponent::RemoveStatModifier(FARStatModifierHandle Handle)
{
	if (!CanMutate()) return false;
	const FScopedNotifications Notifications(this);
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
	if (!Removed.Spec.bStackOnly) RecalculateStat(Removed.Spec.StatType);
	BroadcastSourceChange(Removed.Spec.Source);
	return true;
}

int32 UARStatsComponent::RemoveModifiers(EARModifierSourceCategory Category, FName SourceId)
{
	if (!CanMutate()) return 0;
	const FScopedNotifications Notifications(this);
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
			if (!Modifier.Spec.bStackOnly) DirtyStats.Add(Modifier.Spec.StatType);
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
	RemovedHandle = FARStatModifierHandle();
	int32 RemovedCount = 0;
	TArray<FARStatModifierHandle> RemovedHandles;
	const bool bRemoved = RemoveModifierStacks(Category, SourceId, 1, RemovedCount, RemovedHandles, Policy, true);
	if (bRemoved && !RemovedHandles.IsEmpty()) RemovedHandle = RemovedHandles[0];
	return bRemoved;
}

bool UARStatsComponent::RemoveModifierStacks(EARModifierSourceCategory Category, FName SourceId, int32 Count, int32& RemovedCount, TArray<FARStatModifierHandle>& RemovedHandles, EARModifierStackRemovalPolicy Policy, bool bRequireFullCount)
{
	RemovedCount = 0;
	RemovedHandles.Reset();
	if (!CanMutate()) return false;
	const FScopedNotifications Notifications(this);
	if (Count <= 0 || SourceId.IsNone()
		|| (Policy != EARModifierStackRemovalPolicy::Newest && Policy != EARModifierStackRemovalPolicy::Oldest)) return false;

	struct FStackCandidate { FGuid Id; double AppliedAt; int32 Order; };
	TArray<FStackCandidate> Candidates;
	TMap<FGuid, int32> CandidateIndexes;
	const double Now = GetNow();
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if ((Category != EARModifierSourceCategory::All && Modifier.Spec.Source.Category != Category)
			|| Modifier.Spec.Source.SourceId != SourceId || (Modifier.ExpireAt >= 0.0 && Modifier.ExpireAt <= Now)) continue;
		if (const int32* Existing = CandidateIndexes.Find(Modifier.StackGroupId))
		{
			Candidates[*Existing].AppliedAt = FMath::Min(Candidates[*Existing].AppliedAt, Modifier.AppliedAt);
		}
		else
		{
			const int32 Order = Candidates.Num();
			CandidateIndexes.Add(Modifier.StackGroupId, Order);
			Candidates.Add({ Modifier.StackGroupId, Modifier.AppliedAt, Order });
		}
	}
	if (Candidates.IsEmpty() || (bRequireFullCount && Candidates.Num() < Count)) return false;
	Candidates.Sort([Policy](const FStackCandidate& A, const FStackCandidate& B)
	{
		if (A.AppliedAt == B.AppliedAt) return Policy == EARModifierStackRemovalPolicy::Newest ? A.Order > B.Order : A.Order < B.Order;
		return Policy == EARModifierStackRemovalPolicy::Newest ? A.AppliedAt > B.AppliedAt : A.AppliedAt < B.AppliedAt;
	});
	TSet<FGuid> SelectedGroups;
	for (int32 Index = 0; Index < FMath::Min(Count, Candidates.Num()); ++Index) SelectedGroups.Add(Candidates[Index].Id);
	TSet<EARStatType> DirtyStats;
	TArray<FARSourceInfo> ChangedSources;
	// Remove the complete batch before recalculation/notifications so callbacks cannot observe a half-consumed cost.
	for (int32 Index = ActiveModifiers.Num() - 1; Index >= 0; --Index)
	{
		const FARActiveStatModifier& Modifier = ActiveModifiers[Index];
		if (!SelectedGroups.Contains(Modifier.StackGroupId)) continue;
		RemovedHandles.Add(Modifier.Handle);
		if (!Modifier.Spec.bStackOnly) DirtyStats.Add(Modifier.Spec.StatType);
		if (!ChangedSources.ContainsByPredicate([&Modifier](const FARSourceInfo& Source)
			{ return Source.Category == Modifier.Spec.Source.Category && Source.SourceId == Modifier.Spec.Source.SourceId; })) ChangedSources.Add(Modifier.Spec.Source);
		ActiveModifiers.RemoveAt(Index);
	}
	RemovedCount = SelectedGroups.Num();
	for (const EARStatType StatType : DirtyStats) RecalculateStat(StatType);
	for (const FARSourceInfo& Source : ChangedSources) BroadcastSourceChange(Source);
	return true;
}

float UARStatsComponent::GetFinalStat(EARStatType StatType) const
{
	if (!IsStatSupported(StatType)) return 0.0f;
	if (const float* Value = CachedFinalStats.Find(StatType))
	{
		return *Value;
	}
	return ApplySafetyRules(StatType, CalculateGeneralStat(StatType));
}

float UARStatsComponent::GetBaseStat(EARStatType StatType) const
{
	if (!IsStatSupported(StatType)) return 0.0f;
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
	// Enumeration declaration order groups related stats without renumbering persisted enum values.
	for (int32 Index = 0; StatEnum && Index < StatEnum->NumEnums(); ++Index)
	{
		const int64 Value = StatEnum->GetValueByIndex(Index);
		if (Value < 0 || Value >= static_cast<int64>(EARStatType::Count)
			|| !IsStatSupported(static_cast<EARStatType>(Value))) continue;
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
	TSet<FGuid> Groups;
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if ((Category == EARModifierSourceCategory::All || Modifier.Spec.Source.Category == Category) && Modifier.Spec.Source.SourceId == SourceId)
		{
			Result.bExists = true;
			Groups.Add(Modifier.StackGroupId);
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
	Result.StackCount = Groups.Num();
	return Result;
}

TArray<FARStatEffectView> UARStatsComponent::GetVisibleStatEffects() const
{
	TArray<FARStatEffectView> Views;
	const double Now = GetNow();
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if (Modifier.Spec.HUDDisplay == EARStatEffectDisplay::Hidden) continue;
		const bool bKnownSource = Views.ContainsByPredicate([&Modifier](const FARStatEffectView& View)
		{
			return View.Category == Modifier.Spec.Source.Category && View.SourceId == Modifier.Spec.Source.SourceId;
		});
		if (bKnownSource) continue;
		FARStatEffectView& View = Views.AddDefaulted_GetRef();
		View.Category = Modifier.Spec.Source.Category;
		View.SourceId = Modifier.Spec.Source.SourceId;
		View.Display = Modifier.Spec.HUDDisplay;
		View.DisplayName = Modifier.Spec.HUDName.IsEmpty() ? Modifier.Spec.Source.DisplayName : Modifier.Spec.HUDName;
		View.Icon = Modifier.Spec.HUDIcon;
	}
	for (FARStatEffectView& View : Views)
	{
		TSet<FGuid> Groups;
		for (const FARActiveStatModifier& Modifier : ActiveModifiers)
		{
			if (Modifier.Spec.Source.Category != View.Category || Modifier.Spec.Source.SourceId != View.SourceId) continue;
			Groups.Add(Modifier.StackGroupId);
			if (Modifier.ExpireAt < 0.0)
			{
				View.bHasPermanent = true;
				View.LongestRemainingTime = -1.0f;
			}
			else if (!View.bHasPermanent)
			{
				View.LongestRemainingTime = FMath::Max(View.LongestRemainingTime, static_cast<float>(Modifier.ExpireAt - Now));
			}
		}
		View.StackCount = Groups.Num();
	}
	Views.Sort([](const FARStatEffectView& A, const FARStatEffectView& B)
	{
		return A.SourceId.LexicalLess(B.SourceId);
	});
	return Views;
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
		if (Modifier.Spec.bStackOnly || Modifier.Spec.StatType != ReductionStat)
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
		return !Modifier.Spec.bStackOnly && Modifier.Spec.bGuaranteeInvulnerability;
	});
}

bool UARStatsComponent::HasGuaranteedEvasion() const
{
	return ActiveModifiers.ContainsByPredicate([](const FARActiveStatModifier& Modifier)
	{
		return !Modifier.Spec.bStackOnly && Modifier.Spec.bGuaranteeEvasion;
	});
}

void UARStatsComponent::SetBaseStat(EARStatType StatType, float Value)
{
	if (!CanMutate() || !IsStatSupported(StatType) || !FMath::IsFinite(Value)) return;
	const FScopedNotifications Notifications(this);
	if (StatType == EARStatType::Money && (!FMath::IsFinite(Value) || Value < 0.0f)) return;
	BaseStats.FindOrAdd(StatType) = Value;
	RecalculateStat(StatType);
}

bool UARStatsComponent::ApplyPermanentFlat(EARStatType StatType, float Delta)
{
	if (!CanMutate() || !IsStatSupported(StatType) || !FMath::IsFinite(Delta)) return false;
	const FScopedNotifications Notifications(this);
	const float NewBase = GetBaseStat(StatType) + Delta;
	if (!FMath::IsFinite(NewBase) || (StatType == EARStatType::Money && NewBase < 0.0f)) return false;
	// Keep raw base semantics consistent with Flat. Existing final-value safety
	// rules and percentage/multiplier effects still run via RecalculateStat.
	SetBaseStat(StatType, NewBase);
	return true;
}

bool UARStatsComponent::IsStatSupported(EARStatType StatType) const
{
	return static_cast<uint8>(StatType) < static_cast<uint8>(EARStatType::Count)
		&& (StatType != EARStatType::Money || (IsValid(GetOwner()) && GetOwner()->IsA<AARPlayerCharacter>()));
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
	if (!IsStatSupported(StatType)) return;
	const float OldValue = CachedFinalStats.FindRef(StatType);
	const float NewValue = IsReductionStat(StatType)
		? (1.0f - GetDamageRemainingMultiplier(StatType)) * 100.0f
		: ApplySafetyRules(StatType, CalculateGeneralStat(StatType));
	CachedFinalStats.FindOrAdd(StatType) = NewValue;
	if (!FMath::IsNearlyEqual(OldValue, NewValue))
	{
		if (!PendingNotifications.ContainsByPredicate([StatType](const FPendingNotification& N) { return !N.bSource && N.StatType == StatType; }))
		{
			FPendingNotification& Notification = PendingNotifications.AddDefaulted_GetRef();
			Notification.StatType = StatType;
			Notification.OldValue = OldValue;
		}
	}
}

float UARStatsComponent::CalculateGeneralStat(EARStatType StatType, FARStatBreakdown* OutBreakdown) const
{
	if (!IsStatSupported(StatType)) return 0.0f;
	const float BaseValue = BaseStats.FindRef(StatType);
	float FlatTotal = 0.0f;
	float AdditiveTotal = 0.0f;
	float MultiplicativeProduct = 1.0f;
	for (const FARActiveStatModifier& Modifier : ActiveModifiers)
	{
		if (Modifier.Spec.bStackOnly || Modifier.Spec.StatType != StatType)
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
	case EARStatType::OverallDamageTakenIncrease:
	case EARStatType::Money:
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
	if (!PendingNotifications.ContainsByPredicate([&Source](const FPendingNotification& N)
		{ return N.bSource && N.Source.Category == Source.Category && N.Source.SourceId == Source.SourceId; }))
	{
		FPendingNotification& Notification = PendingNotifications.AddDefaulted_GetRef();
		Notification.bSource = true;
		Notification.Source = Source;
	}
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
