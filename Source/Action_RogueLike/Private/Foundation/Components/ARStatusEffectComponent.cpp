#include "Foundation/Components/ARStatusEffectComponent.h"

#include "Engine/World.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"

UARStatusEffectComponent::UARStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UARStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();
	StatsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UARStatsComponent>() : nullptr;
	if (!StatsComponent)
	{
		UE_LOG(LogARFoundation, Error, TEXT("StatusEffectComponent on %s requires ARStatsComponent."), *GetNameSafe(GetOwner()));
	}
}

void UARStatusEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Now = GetNow();
	TArray<FARStatusEffectView> RemovedViews;
	for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveEffects[Index].ExpireAt <= Now)
		{
			FARStatusEffectView View = MakeView(ActiveEffects[Index]);
			View.RemainingTime = 0.0f;
			RemovedViews.Add(View);
			ActiveEffects.RemoveAt(Index);
		}
	}
	for (const FARStatusEffectView& View : RemovedViews)
	{
		OnStatusRemovedNative.Broadcast(GetOwner(), View);
		OnStatusRemoved.Broadcast(GetOwner(), View);
	}
	RefreshTickState();
}

FARStatusEffectResult UARStatusEffectComponent::ApplyStatusEffect(const FARStatusEffectRequest& Request)
{
	FARStatusEffectResult Result;
	if (!StatsComponent || !IsValid(Request.Definition) || !Request.Definition->StatusTag.IsValid())
	{
		Result.Result = EARRequestResult::InvalidDefinition;
		return Result;
	}
	if (Request.Definition->bCanBeImmune && ImmuneStatusTags.HasTag(Request.Definition->StatusTag))
	{
		Result.Result = EARRequestResult::Blocked;
		return Result;
	}

	const float RequestedDuration = Request.DurationOverride < 0.0f ? Request.Definition->BaseDuration : Request.DurationOverride;
	if (!FMath::IsFinite(RequestedDuration) || RequestedDuration <= 0.0f)
	{
		Result.Result = EARRequestResult::InvalidDefinition;
		return Result;
	}
	const float Tenacity = Request.Definition->bAffectedByTenacity
		? FMath::Clamp(StatsComponent->GetFinalStat(EARStatType::Tenacity), 0.0f, 100.0f)
		: 0.0f;
	const float AppliedDuration = RequestedDuration * (1.0f - Tenacity / 100.0f);
	if (AppliedDuration <= KINDA_SMALL_NUMBER)
	{
		Result.Result = EARRequestResult::Blocked;
		return Result;
	}

	const double Now = GetNow();
	FARActiveStatusEffect* Existing = ActiveEffects.FindByPredicate([&Request](const FARActiveStatusEffect& Effect)
	{
		return Effect.Definition && Effect.Definition->StatusTag == Request.Definition->StatusTag;
	});
	if (Existing)
	{
		Result.Handle = Existing->Handle;
		Result.StatusTag = Request.Definition->StatusTag;
		Result.AppliedDuration = AppliedDuration;
		Result.bRefreshedExisting = true;
		Result.Result = EARRequestResult::Success;
		if (Existing->ExpireAt < Now + AppliedDuration)
		{
			Existing->ExpireAt = Now + AppliedDuration;
			Existing->Definition = Request.Definition;
			Existing->Source = Request.Source;
			OnStatusUpdatedNative.Broadcast(GetOwner(), MakeView(*Existing));
			OnStatusUpdated.Broadcast(GetOwner(), MakeView(*Existing));
		}
		RefreshTickState();
		return Result;
	}

	FARActiveStatusEffect& Effect = ActiveEffects.AddDefaulted_GetRef();
	Effect.Handle.Id = FGuid::NewGuid();
	Effect.Definition = Request.Definition;
	Effect.Source = Request.Source;
	Effect.ExpireAt = Now + AppliedDuration;
	Result.Handle = Effect.Handle;
	Result.StatusTag = Request.Definition->StatusTag;
	Result.AppliedDuration = AppliedDuration;
	Result.Result = EARRequestResult::Success;
	OnStatusAddedNative.Broadcast(GetOwner(), MakeView(Effect));
	OnStatusAdded.Broadcast(GetOwner(), MakeView(Effect));
	RefreshTickState();
	return Result;
}

bool UARStatusEffectComponent::RemoveStatusEffect(FARStatusEffectHandle Handle)
{
	const int32 Index = ActiveEffects.IndexOfByPredicate([&Handle](const FARActiveStatusEffect& Effect)
	{
		return Effect.Handle == Handle;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}
	FARStatusEffectView View = MakeView(ActiveEffects[Index]);
	ActiveEffects.RemoveAt(Index);
	OnStatusRemovedNative.Broadcast(GetOwner(), View);
	OnStatusRemoved.Broadcast(GetOwner(), View);
	RefreshTickState();
	return true;
}

int32 UARStatusEffectComponent::RemoveStatusEffectsBySource(EARModifierSourceCategory Category, FName SourceId)
{
	TArray<FARStatusEffectView> RemovedViews;
	for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
	{
		const FARSourceInfo& Source = ActiveEffects[Index].Source;
		if ((Category == EARModifierSourceCategory::All || Source.Category == Category)
			&& (SourceId.IsNone() || Source.SourceId == SourceId))
		{
			RemovedViews.Add(MakeView(ActiveEffects[Index]));
			ActiveEffects.RemoveAt(Index);
		}
	}
	for (const FARStatusEffectView& View : RemovedViews)
	{
		OnStatusRemovedNative.Broadcast(GetOwner(), View);
		OnStatusRemoved.Broadcast(GetOwner(), View);
	}
	RefreshTickState();
	return RemovedViews.Num();
}

int32 UARStatusEffectComponent::ClearAllStatusEffects()
{
	return RemoveStatusEffectsBySource(EARModifierSourceCategory::All, NAME_None);
}

bool UARStatusEffectComponent::HasStatus(FGameplayTag StatusTag) const
{
	return ActiveEffects.ContainsByPredicate([StatusTag](const FARActiveStatusEffect& Effect)
	{
		return Effect.Definition && Effect.Definition->StatusTag == StatusTag;
	});
}

bool UARStatusEffectComponent::GetStatusRemainingTime(FGameplayTag StatusTag, float& RemainingTime) const
{
	const FARActiveStatusEffect* Effect = ActiveEffects.FindByPredicate([StatusTag](const FARActiveStatusEffect& Candidate)
	{
		return Candidate.Definition && Candidate.Definition->StatusTag == StatusTag;
	});
	RemainingTime = Effect ? FMath::Max(0.0f, static_cast<float>(Effect->ExpireAt - GetNow())) : 0.0f;
	return Effect != nullptr;
}

TArray<FARStatusEffectView> UARStatusEffectComponent::GetActiveStatusEffects() const
{
	TArray<FARStatusEffectView> Views;
	Views.Reserve(ActiveEffects.Num());
	for (const FARActiveStatusEffect& Effect : ActiveEffects)
	{
		Views.Add(MakeView(Effect));
	}
	return Views;
}

bool UARStatusEffectComponent::BlocksBasicMovement() const
{
	return ActiveEffects.ContainsByPredicate([](const FARActiveStatusEffect& Effect)
	{
		return Effect.Definition && Effect.Definition->bBlocksBasicMovement;
	});
}

bool UARStatusEffectComponent::BlocksAllMovement() const
{
	return ActiveEffects.ContainsByPredicate([](const FARActiveStatusEffect& Effect)
	{
		return Effect.Definition && Effect.Definition->bBlocksAllMovement;
	});
}

bool UARStatusEffectComponent::BlocksRoll() const
{
	return ActiveEffects.ContainsByPredicate([](const FARActiveStatusEffect& Effect)
	{
		return Effect.Definition && Effect.Definition->bBlocksRoll;
	});
}

bool UARStatusEffectComponent::BlocksSkillGroups() const
{
	return ActiveEffects.ContainsByPredicate([](const FARActiveStatusEffect& Effect)
	{
		return Effect.Definition && Effect.Definition->bBlocksSkillGroups;
	});
}

FARStatusEffectView UARStatusEffectComponent::MakeView(const FARActiveStatusEffect& Effect) const
{
	FARStatusEffectView View;
	View.Handle = Effect.Handle;
	View.Definition = Effect.Definition;
	View.StatusTag = Effect.Definition ? Effect.Definition->StatusTag : FGameplayTag();
	View.Source = Effect.Source;
	View.RemainingTime = FMath::Max(0.0f, static_cast<float>(Effect.ExpireAt - GetNow()));
	return View;
}

void UARStatusEffectComponent::RefreshTickState()
{
	SetComponentTickEnabled(!ActiveEffects.IsEmpty());
}

double UARStatusEffectComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}
