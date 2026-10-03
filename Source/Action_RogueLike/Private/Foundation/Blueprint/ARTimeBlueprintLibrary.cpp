#include "Foundation/Blueprint/ARTimeBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	UARTimeSubsystem* ClockFor(const UObject* Context)
	{
		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
		return World ? World->GetSubsystem<UARTimeSubsystem>() : nullptr;
	}
}

bool UARTimeBlueprintLibrary::SetTimeGroupRate(const UObject* WorldContextObject, EARTimeGroup TimeGroup, float Rate)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	return Clock && Clock->SetTimeGroupRate(TimeGroup, Rate);
}

float UARTimeBlueprintLibrary::GetTimeGroupRate(const UObject* WorldContextObject, EARTimeGroup TimeGroup)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	return Clock ? Clock->GetTimeGroupRate(TimeGroup) : 1.0f;
}

double UARTimeBlueprintLibrary::GetTimeGroupSeconds(const UObject* WorldContextObject, EARTimeGroup TimeGroup)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	return Clock ? Clock->GetTimeGroupSeconds(TimeGroup) : 0.0;
}

void UARTimeBlueprintLibrary::PauseTimeGroupTimer(const UObject* WorldContextObject, FTimerHandle Handle)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	if (Clock) Clock->PauseTimeGroupTimer(Handle);
}

void UARTimeBlueprintLibrary::UnpauseTimeGroupTimer(const UObject* WorldContextObject, FTimerHandle Handle)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	if (Clock) Clock->UnpauseTimeGroupTimer(Handle);
}

void UARTimeBlueprintLibrary::ClearTimeGroupTimer(const UObject* WorldContextObject, FTimerHandle Handle)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	if (Clock) Clock->ClearTimeGroupTimer(Handle);
}

float UARTimeBlueprintLibrary::GetTimeGroupTimerRemaining(const UObject* WorldContextObject, FTimerHandle Handle)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	return Clock ? Clock->GetTimeGroupTimerRemaining(Handle) : -1.0f;
}

bool UARTimeBlueprintLibrary::DoesTimeGroupTimerExist(const UObject* WorldContextObject, FTimerHandle Handle)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	return Clock && Clock->DoesTimeGroupTimerExist(Handle);
}

bool UARTimeBlueprintLibrary::IsTimeGroupTimerPaused(const UObject* WorldContextObject, FTimerHandle Handle)
{
	UARTimeSubsystem* Clock = ClockFor(WorldContextObject);
	return Clock && Clock->IsTimeGroupTimerPaused(Handle);
}
