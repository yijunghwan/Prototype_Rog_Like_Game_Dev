#include "Foundation/Combat/ARCombatSubsystem.h"

#include "Foundation/Combat/ARDamageResolver.h"
#include "Foundation/Components/ARCombatSourceComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Interfaces/ARCombatTargetInterface.h"

void UARCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CombatRandomStream.Initialize(FMath::Rand());
}

void UARCombatSubsystem::Tick(float DeltaTime)
{
	struct FARDueDotTick
	{
		FARCombatDamageRequest Request;
		bool bApplyStaggerAndGroggy = false;
		FARStaggerRequestTemplate StaggerTemplate;
		bool bSourceAlreadyValidated = false;
	};

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	TArray<FARDueDotTick> DueTicks;
	for (int32 Index = ActiveDots.Num() - 1; Index >= 0; --Index)
	{
		FARActiveDotInstance& Dot = ActiveDots[Index];
		if (!Dot.Target.IsValid())
		{
			ActiveDots.RemoveAt(Index);
			continue;
		}

		int32 CatchUpCount = 0;
		while (Dot.NextTickAt <= Now && Dot.NextTickAt <= Dot.ExpireAt)
		{
			FARDueDotTick& Due = DueTicks.AddDefaulted_GetRef();
			Due.Request = Dot.Spec.DamageRequest;
			Due.Request.Delivery = EARDamageDelivery::DamageOverTime;
			Due.Request.bApplyEvasion = false;
			Due.Request.Target = Dot.Target.Get();
			Due.bApplyStaggerAndGroggy = Dot.Spec.bApplyStaggerAndGroggyEachTick;
			Due.StaggerTemplate = Dot.Spec.StaggerTemplate;
			if (Dot.Attacker.IsValid())
			{
				Due.Request.Attacker = Dot.Attacker.Get();
				Dot.LastOffensiveSnapshot = CaptureOffensiveSnapshot(Dot.Attacker.Get(), Due.Request.Delivery, Due.Request.Attribute);
			}
			else
			{
				Due.Request.Attacker = nullptr;
				Due.Request.bUseOffensiveSnapshot = true;
				Due.Request.OffensiveSnapshot = Dot.LastOffensiveSnapshot;
				Due.bSourceAlreadyValidated = true;
			}
			Dot.NextTickAt += Dot.Spec.TickInterval;
			++CatchUpCount;
		}
		if (CatchUpCount > CatchUpTickWarningThreshold)
		{
			UE_LOG(LogARCombat, Warning, TEXT("DOT %s caught up %d ticks in one frame."), *Dot.Spec.DotName.ToString(), CatchUpCount);
		}
		if (Now >= Dot.ExpireAt && Dot.NextTickAt > Dot.ExpireAt)
		{
			ActiveDots.RemoveAt(Index);
		}
	}

	for (const FARDueDotTick& Due : DueTicks)
	{
		const FARCombatDamageResult DamageResult = ProcessDamageRequest(Due.Request, Due.bSourceAlreadyValidated);
		if (Due.bApplyStaggerAndGroggy && DamageResult.WasApplied() && DamageResult.HitContext.IsUsable())
		{
			if (UARStaggerComponent* Stagger = DamageResult.HitContext.Target->FindComponentByClass<UARStaggerComponent>())
			{
				FARStaggerRequest StaggerRequest;
				StaggerRequest.HitContext = DamageResult.HitContext;
				StaggerRequest.Template = Due.StaggerTemplate;
				Stagger->ApplyStaggerAndGroggyDamage(StaggerRequest);
			}
		}
	}
}

TStatId UARCombatSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UARCombatSubsystem, STATGROUP_Tickables);
}

bool UARCombatSubsystem::CanDamageTarget(AActor* Attacker, AActor* Target, EARRequestResult& FailureReason) const
{
	FailureReason = EARRequestResult::Rejected;
	if (!IsValid(Attacker))
	{
		FailureReason = EARRequestResult::InvalidSource;
		return false;
	}
	if (!IsValid(Target) || Attacker == Target)
	{
		FailureReason = EARRequestResult::InvalidTarget;
		return false;
	}

	EARCombatTeam AttackerTeam = EARCombatTeam::Environment;
	EARCombatTeam TargetTeam = EARCombatTeam::Environment;
	bool bAttackerCanBeTarget = false;
	bool bTargetCanBeTarget = false;
	if (!TryGetCombatTeam(Attacker, AttackerTeam, bAttackerCanBeTarget))
	{
		FailureReason = EARRequestResult::InvalidSource;
		return false;
	}
	if (!TryGetCombatTeam(Target, TargetTeam, bTargetCanBeTarget) || !bTargetCanBeTarget || TargetTeam == EARCombatTeam::Environment)
	{
		FailureReason = EARRequestResult::ForbiddenTarget;
		return false;
	}
	if (AttackerTeam == TargetTeam)
	{
		FailureReason = EARRequestResult::SameTeam;
		return false;
	}
	if (!((AttackerTeam == EARCombatTeam::Player && TargetTeam == EARCombatTeam::Enemy)
		|| (AttackerTeam == EARCombatTeam::Enemy && TargetTeam == EARCombatTeam::Player)
		|| AttackerTeam == EARCombatTeam::Environment))
	{
		FailureReason = EARRequestResult::ForbiddenTarget;
		return false;
	}

	const UARHealthComponent* Health = Target->FindComponentByClass<UARHealthComponent>();
	if (!Health)
	{
		FailureReason = EARRequestResult::InvalidTarget;
		return false;
	}
	if (!Health->CanAcceptResolvedDamage())
	{
		FailureReason = Health->IsDead() ? EARRequestResult::Dead : EARRequestResult::Blocked;
		return false;
	}
	FailureReason = EARRequestResult::Success;
	return true;
}

FARCombatDamageResult UARCombatSubsystem::ApplyCombatDamage(const FARCombatDamageRequest& Request)
{
	if (bProcessingDamage)
	{
		FARCombatDamageResult QueuedResult;
		if (QueuedDamageRequests.Num() >= MaxQueuedRequestsPerChain)
		{
			QueuedResult.Outcome = EARDamageOutcome::Blocked;
			QueuedResult.FailureReason = EARRequestResult::Blocked;
			UE_LOG(LogARCombat, Error, TEXT("Damage request chain limit reached. Request from %s was rejected."), *GetNameSafe(Request.Attacker));
			return QueuedResult;
		}
		QueuedDamageRequests.Add(Request);
		QueuedResult.Outcome = EARDamageOutcome::Queued;
		QueuedResult.FailureReason = EARRequestResult::Success;
		return QueuedResult;
	}

	bProcessingDamage = true;
	FARCombatDamageResult RootResult = ProcessDamageRequest(Request);
	int32 ProcessedQueuedRequests = 0;
	while (!QueuedDamageRequests.IsEmpty() && ProcessedQueuedRequests < MaxQueuedRequestsPerChain)
	{
		const FARCombatDamageRequest QueuedRequest = QueuedDamageRequests[0];
		QueuedDamageRequests.RemoveAt(0, 1, EAllowShrinking::No);
		ProcessDamageRequest(QueuedRequest);
		++ProcessedQueuedRequests;
	}
	if (!QueuedDamageRequests.IsEmpty())
	{
		UE_LOG(LogARCombat, Error, TEXT("Discarded %d excessive chained damage requests."), QueuedDamageRequests.Num());
		QueuedDamageRequests.Reset();
	}
	bProcessingDamage = false;
	return RootResult;
}

FARDotHandle UARCombatSubsystem::ApplyDamageOverTime(const FARDamageOverTimeSpec& Spec, bool& bSuccess, EARRequestResult& FailureReason)
{
	bSuccess = false;
	FARDotHandle Handle;
	FailureReason = EARRequestResult::Rejected;
	if (!FMath::IsFinite(Spec.Duration) || !FMath::IsFinite(Spec.TickInterval) || Spec.Duration <= 0.0f || Spec.TickInterval <= 0.0f)
	{
		FailureReason = EARRequestResult::InvalidDefinition;
		return Handle;
	}
	if (!CanDamageTarget(Spec.DamageRequest.Attacker, Spec.DamageRequest.Target, FailureReason))
	{
		return Handle;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Spec.StackPolicy == EARDotStackPolicy::RefreshSameName)
	{
		if (Spec.DotName.IsNone())
		{
			FailureReason = EARRequestResult::InvalidDefinition;
			return Handle;
		}
		FARActiveDotInstance* Existing = ActiveDots.FindByPredicate([&Spec](const FARActiveDotInstance& Dot)
		{
			return Dot.Target.Get() == Spec.DamageRequest.Target && Dot.Spec.DotName == Spec.DotName;
		});
		if (Existing)
		{
			if (!IsRefreshDotDefinitionEqual(Existing->Spec, Spec))
			{
				UE_LOG(LogARCombat, Warning, TEXT("Refresh DOT %s has mismatched damage, duration, or interval. Existing DOT was kept."), *Spec.DotName.ToString());
				FailureReason = EARRequestResult::InvalidDefinition;
				return Handle;
			}
			Existing->ExpireAt = Now + Spec.Duration;
			Existing->NextTickAt = Now + Spec.TickInterval;
			bSuccess = true;
			FailureReason = EARRequestResult::Success;
			return Existing->Handle;
		}
	}

	Handle.Id = FGuid::NewGuid();
	FARActiveDotInstance& Dot = ActiveDots.AddDefaulted_GetRef();
	Dot.Handle = Handle;
	Dot.Spec = Spec;
	Dot.Spec.DamageRequest.Delivery = EARDamageDelivery::DamageOverTime;
	Dot.Spec.DamageRequest.bApplyEvasion = false;
	Dot.Attacker = Spec.DamageRequest.Attacker;
	Dot.Target = Spec.DamageRequest.Target;
	Dot.LastOffensiveSnapshot = CaptureOffensiveSnapshot(Dot.Attacker.Get(), EARDamageDelivery::DamageOverTime, Spec.DamageRequest.Attribute);
	Dot.ExpireAt = Now + Spec.Duration;
	Dot.NextTickAt = Now + Spec.TickInterval;
	if (ActiveDots.Num() > ActiveDotWarningThreshold)
	{
		UE_LOG(LogARCombat, Warning, TEXT("Active DOT count is %d (warning threshold %d)."), ActiveDots.Num(), ActiveDotWarningThreshold);
	}
	bSuccess = true;
	FailureReason = EARRequestResult::Success;
	return Handle;
}

bool UARCombatSubsystem::RemoveDamageOverTime(FARDotHandle Handle)
{
	const int32 Removed = ActiveDots.RemoveAll([&Handle](const FARActiveDotInstance& Dot)
	{
		return Dot.Handle == Handle;
	});
	return Removed > 0;
}

int32 UARCombatSubsystem::RemoveDamageOverTimeBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId, FName DotName)
{
	return ActiveDots.RemoveAll([Target, Category, SourceId, DotName](const FARActiveDotInstance& Dot)
	{
		const FARSourceInfo& Source = Dot.Spec.DamageRequest.Source;
		return Dot.Target.Get() == Target
			&& (Category == EARModifierSourceCategory::All || Source.Category == Category)
			&& (SourceId.IsNone() || Source.SourceId == SourceId)
			&& (DotName.IsNone() || Dot.Spec.DotName == DotName);
	});
}

int32 UARCombatSubsystem::RemoveAllDamageOverTimeFromTarget(AActor* Target)
{
	return ActiveDots.RemoveAll([Target](const FARActiveDotInstance& Dot)
	{
		return Dot.Target.Get() == Target;
	});
}

int32 UARCombatSubsystem::GetActiveDamageOverTimeCount(AActor* Target) const
{
	int32 Count = 0;
	for (const FARActiveDotInstance& Dot : ActiveDots)
	{
		if (Dot.Target.Get() == Target)
		{
			++Count;
		}
	}
	return Count;
}

FARCombatDamageResult UARCombatSubsystem::ProcessDamageRequest(const FARCombatDamageRequest& Request, bool bSourceAlreadyValidated)
{
	FARCombatDamageResult Result;
	EARRequestResult FailureReason = EARRequestResult::Rejected;
	if (!bSourceAlreadyValidated && !CanDamageTarget(Request.Attacker, Request.Target, FailureReason))
	{
		Result.FailureReason = FailureReason;
		return Result;
	}
	if (bSourceAlreadyValidated)
	{
		if (!IsValid(Request.Target))
		{
			Result.FailureReason = EARRequestResult::InvalidTarget;
			return Result;
		}
		const UARHealthComponent* ExistingHealth = Request.Target->FindComponentByClass<UARHealthComponent>();
		if (!ExistingHealth || !ExistingHealth->CanAcceptResolvedDamage())
		{
			Result.FailureReason = ExistingHealth && ExistingHealth->IsDead() ? EARRequestResult::Dead : EARRequestResult::Blocked;
			return Result;
		}
	}

	UARHealthComponent* TargetHealth = Request.Target->FindComponentByClass<UARHealthComponent>();
	const UARStatsComponent* TargetStats = Request.Target->FindComponentByClass<UARStatsComponent>();
	if (!TargetHealth || !TargetStats)
	{
		Result.FailureReason = EARRequestResult::InvalidTarget;
		return Result;
	}

	const FAROffensiveStatSnapshot Offense = Request.bUseOffensiveSnapshot
		? Request.OffensiveSnapshot
		: CaptureOffensiveSnapshot(Request.Attacker, Request.Delivery, Request.Attribute);

	FARDefensiveStatSnapshot Defense;
	Defense.Evasion = TargetStats->GetFinalStat(EARStatType::Evasion);
	Defense.bGuaranteedEvasion = TargetStats->HasGuaranteedEvasion();
	Defense.bGuaranteedInvulnerability = TargetStats->HasGuaranteedInvulnerability();
	if (Request.Attribute == EARDamageAttribute::Physical)
	{
		Defense.AttributeDefense = TargetStats->GetFinalStat(EARStatType::PhysicalDefense);
		Defense.AttributeDamageTakenIncrease = TargetStats->GetFinalStat(EARStatType::PhysicalDamageTakenIncrease);
		Defense.AttributeRemainingDamageMultiplier = TargetStats->GetDamageRemainingMultiplier(EARStatType::PhysicalDamageReduction);
	}
	else if (Request.Attribute == EARDamageAttribute::Fire)
	{
		Defense.AttributeDefense = TargetStats->GetFinalStat(EARStatType::FireDefense);
		Defense.AttributeDamageTakenIncrease = TargetStats->GetFinalStat(EARStatType::FireDamageTakenIncrease);
		Defense.AttributeRemainingDamageMultiplier = TargetStats->GetDamageRemainingMultiplier(EARStatType::FireDamageReduction);
	}
	else if (Request.Attribute == EARDamageAttribute::Magic)
	{
		Defense.AttributeDefense = TargetStats->GetFinalStat(EARStatType::MagicDefense);
		Defense.AttributeDamageTakenIncrease = TargetStats->GetFinalStat(EARStatType::MagicDamageTakenIncrease);
		Defense.AttributeRemainingDamageMultiplier = TargetStats->GetDamageRemainingMultiplier(EARStatType::MagicDamageReduction);
	}
	Defense.OverallRemainingDamageMultiplier = TargetStats->GetDamageRemainingMultiplier(EARStatType::OverallDamageReduction);

	Result = FARDamageResolver::Resolve(Request, Offense, Defense, CombatRandomStream);
	Result.HitContext.HitId = FGuid::NewGuid();
	Result.HitContext.Attacker = Request.Attacker;
	Result.HitContext.Target = Request.Target;
	Result.HitContext.Delivery = Request.Delivery;
	Result.HitContext.Attribute = Request.Attribute;
	Result.HitContext.StaggerPowerSnapshot = Offense.StaggerPower;
	Result.HitContext.GroggyDamageAmplificationSnapshot = Offense.GroggyDamageAmplification;

	if (Result.Outcome == EARDamageOutcome::Evaded)
	{
		OnDamageEvaded.Broadcast(Request.Target, Result);
		return Result;
	}
	if (Result.Outcome == EARDamageOutcome::Blocked)
	{
		OnDamageBlocked.Broadcast(Request.Target, Result);
		return Result;
	}
	if (Result.Outcome != EARDamageOutcome::Applied)
	{
		return Result;
	}

	TargetHealth->PreviewDamageAllocation(Result.FinalDamage, Request.bIgnoreShield, Result.ShieldDamage, Result.HealthDamage);
	Result.HitContext.bValidHit = true;
	Result.bOnHitEffectsTriggered = Request.bApplyOnHitEffects && Request.Delivery == EARDamageDelivery::Direct;
	TargetHealth->ApplyResolvedDamage(Result);
	OnDamageReceived.Broadcast(Request.Target, Result);
	if (Result.bOnHitEffectsTriggered)
	{
		OnDamageHit.Broadcast(Request.Attacker, Result);
	}
	ApplyAbsorption(Request, Result, Offense);
	return Result;
}

FAROffensiveStatSnapshot UARCombatSubsystem::CaptureOffensiveSnapshot(const AActor* Attacker, EARDamageDelivery Delivery, EARDamageAttribute Attribute) const
{
	FAROffensiveStatSnapshot Snapshot;
	const UARStatsComponent* Stats = Attacker ? Attacker->FindComponentByClass<UARStatsComponent>() : nullptr;
	if (!Stats)
	{
		return Snapshot;
	}
	Snapshot.AttackPower = Stats->GetFinalStat(EARStatType::AttackPower);
	Snapshot.SpellPower = Stats->GetFinalStat(EARStatType::SpellPower);
	Snapshot.StaggerPower = Stats->GetFinalStat(EARStatType::StaggerPower);
	Snapshot.GroggyDamageAmplification = Stats->GetFinalStat(EARStatType::GroggyDamageAmplification);
	Snapshot.CriticalChance = Stats->GetFinalStat(EARStatType::CriticalChance);
	Snapshot.CriticalDamage = Stats->GetFinalStat(EARStatType::CriticalDamage);
	Snapshot.DefensePenetration = Stats->GetFinalStat(EARStatType::DefensePenetration);
	Snapshot.Absorption = Stats->GetFinalStat(EARStatType::Absorption);
	Snapshot.OverallAmplification = Stats->GetFinalStat(EARStatType::OverallDamageAmplification);
	Snapshot.DeliveryAmplification = Stats->GetFinalStat(Delivery == EARDamageDelivery::Direct
		? EARStatType::DirectDamageAmplification
		: EARStatType::DamageOverTimeAmplification);
	if (Attribute == EARDamageAttribute::Physical)
	{
		Snapshot.AttributeAmplification = Stats->GetFinalStat(EARStatType::PhysicalDamageAmplification);
	}
	else if (Attribute == EARDamageAttribute::Fire)
	{
		Snapshot.AttributeAmplification = Stats->GetFinalStat(EARStatType::FireDamageAmplification);
	}
	else if (Attribute == EARDamageAttribute::Magic)
	{
		Snapshot.AttributeAmplification = Stats->GetFinalStat(EARStatType::MagicDamageAmplification);
	}
	return Snapshot;
}

bool UARCombatSubsystem::TryGetCombatTeam(const AActor* Actor, EARCombatTeam& OutTeam, bool& bOutCanBeTarget) const
{
	bOutCanBeTarget = false;
	if (!IsValid(Actor))
	{
		return false;
	}
	if (const IARCombatTargetInterface* CombatTarget = Cast<IARCombatTargetInterface>(Actor))
	{
		OutTeam = CombatTarget->GetCombatTeam();
		bOutCanBeTarget = CombatTarget->CanBeCombatTarget();
		return true;
	}
	if (const UARCombatSourceComponent* Source = Actor->FindComponentByClass<UARCombatSourceComponent>())
	{
		OutTeam = Source->GetCombatTeam();
		return true;
	}
	return false;
}

void UARCombatSubsystem::ApplyAbsorption(const FARCombatDamageRequest& Request, const FARCombatDamageResult& Result, const FAROffensiveStatSnapshot& Offense)
{
	if (!Request.bApplyAbsorption || Result.FinalDamage <= 0 || Offense.Absorption <= 0.0f || !IsValid(Request.Attacker))
	{
		return;
	}
	UARHealthComponent* AttackerHealth = Request.Attacker->FindComponentByClass<UARHealthComponent>();
	if (!AttackerHealth || AttackerHealth->IsDead())
	{
		return;
	}
	double& Remainder = AbsorptionRemainders.FindOrAdd(Request.Attacker);
	Remainder += static_cast<double>(Result.FinalDamage) * static_cast<double>(Offense.Absorption) / 1000.0;
	const int32 WholeHealing = FMath::FloorToInt(Remainder);
	if (WholeHealing > 0)
	{
		FARSourceInfo Source = Request.Source;
		AttackerHealth->RestoreHealth(static_cast<float>(WholeHealing), Source);
		Remainder -= WholeHealing;
	}
}

bool UARCombatSubsystem::IsRefreshDotDefinitionEqual(const FARDamageOverTimeSpec& A, const FARDamageOverTimeSpec& B) const
{
	return FMath::IsNearlyEqual(A.Duration, B.Duration)
		&& FMath::IsNearlyEqual(A.TickInterval, B.TickInterval)
		&& FMath::IsNearlyEqual(A.DamageRequest.BaseDamage, B.DamageRequest.BaseDamage)
		&& FMath::IsNearlyEqual(A.DamageRequest.AttackPowerCoefficient, B.DamageRequest.AttackPowerCoefficient)
		&& FMath::IsNearlyEqual(A.DamageRequest.SpellPowerCoefficient, B.DamageRequest.SpellPowerCoefficient)
		&& A.DamageRequest.Attribute == B.DamageRequest.Attribute
		&& A.DamageRequest.bCanCrit == B.DamageRequest.bCanCrit
		&& A.DamageRequest.bApplyAmplification == B.DamageRequest.bApplyAmplification
		&& A.DamageRequest.bIgnoreDefense == B.DamageRequest.bIgnoreDefense
		&& A.DamageRequest.bIgnoreShield == B.DamageRequest.bIgnoreShield;
}
