#include "Foundation/Blueprint/ARStatsBlueprintLibrary.h"

namespace
{
	UARStatsComponent* FindStats(AActor* Target)
	{
		return Target ? Target->FindComponentByClass<UARStatsComponent>() : nullptr;
	}
}

FARStatModifierHandle UARStatsBlueprintLibrary::ApplyStatModifier(AActor* Target, const FARStatModifierSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	if (UARStatsComponent* Stats = FindStats(Target)) return Stats->AddStatModifier(Spec, bSuccess);
	return FARStatModifierHandle();
}

bool UARStatsBlueprintLibrary::RemoveStatModifier(AActor* Target, FARStatModifierHandle Handle)
{
	if (UARStatsComponent* Stats = FindStats(Target)) return Stats->RemoveStatModifier(Handle);
	return false;
}

int32 UARStatsBlueprintLibrary::RemoveStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId)
{
	if (UARStatsComponent* Stats = FindStats(Target)) return Stats->RemoveModifiers(Category, SourceId);
	return 0;
}

int32 UARStatsBlueprintLibrary::ClearStatModifiers(AActor* Target, EARModifierSourceCategory Category)
{
	if (UARStatsComponent* Stats = FindStats(Target)) return Stats->ClearModifiers(Category);
	return 0;
}

bool UARStatsBlueprintLibrary::RemoveOneStatModifierStack(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle)
{
	if (UARStatsComponent* Stats = FindStats(Target)) return Stats->RemoveOneModifierStack(Category, SourceId, Policy, RemovedHandle);
	return false;
}

float UARStatsBlueprintLibrary::GetFinalStat(AActor* Target, EARStatType StatType)
{
	if (const UARStatsComponent* Stats = FindStats(Target)) return Stats->GetFinalStat(StatType);
	return 0.0f;
}

FARStatBreakdown UARStatsBlueprintLibrary::GetStatBreakdown(AActor* Target, EARStatType StatType)
{
	if (const UARStatsComponent* Stats = FindStats(Target)) return Stats->GetStatBreakdown(StatType);
	return FARStatBreakdown();
}

FARStatModifierQueryResult UARStatsBlueprintLibrary::GetStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId)
{
	if (const UARStatsComponent* Stats = FindStats(Target)) return Stats->GetModifiersBySource(Category, SourceId);
	return FARStatModifierQueryResult();
}

