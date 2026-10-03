#include "Foundation/Actions/ARLifetimeTimer.h"
#include "Engine/World.h"

void ARLifetimeTimer::Clear(UWorld* World, TArray<FTimerHandle>& OwnedHandles)
{
	const TArray<FTimerHandle> Handles = MoveTemp(OwnedHandles);
	OwnedHandles.Reset();
	if (World)
	{
		for (FTimerHandle Handle : Handles)
		{
			if (UARTimeSubsystem* Time = World->GetSubsystem<UARTimeSubsystem>()) Time->ClearTimeGroupTimer(Handle);
			else World->GetTimerManager().ClearTimer(Handle);
		}
	}
}

FTimerHandle ARLifetimeTimer::StartGrouped(UObject* Owner, FTimerDynamicDelegate Event, float Time, bool bLooping,
	EARTimeGroup TimeGroup, float InitialStartDelay, float Variance, bool bMaxOncePerFrame,
	TFunction<bool()> IsLifetimeActive, TArray<FTimerHandle>& OwnedHandles, bool& bSuccess)
{
	bSuccess = false;
	UWorld* World = IsValid(Owner) ? Owner->GetWorld() : nullptr;
	UARTimeSubsystem* Clock = World ? World->GetSubsystem<UARTimeSubsystem>() : nullptr;
	if (!Clock || World->bIsTearingDown || !Event.IsBound() || !IsLifetimeActive()
		|| !FMath::IsFinite(Time) || !FMath::IsFinite(InitialStartDelay) || !FMath::IsFinite(Variance)
		|| (TimeGroup != EARTimeGroup::World && TimeGroup != EARTimeGroup::Player)) return FTimerHandle();
	const UObject* CallbackObject = Event.GetUObject();
	if (!IsValid(CallbackObject) || (CallbackObject->GetWorld() && CallbackObject->GetWorld() != World)) return FTimerHandle();
	const float FirstDelay = Time > 0.0f ? Time + InitialStartDelay + FMath::RandRange(-Variance, Variance) : 0.0f;
	if (!FMath::IsFinite(FirstDelay)) return FTimerHandle();
	OwnedHandles.RemoveAll([Clock](FTimerHandle Handle) { return !Clock->DoesTimeGroupTimerExist(Handle); });
	FTimerHandle Existing = Clock->FindEventTimer(OwnedHandles, Event);
	// Re-registration is scoped to this item or this action, not all delegates in the world.
	if (Existing.IsValid()) { Clock->ClearTimeGroupTimer(Existing); OwnedHandles.Remove(Existing); }
	if (Time <= 0.0f) return FTimerHandle(); // Unreal clears the existing timer for non-positive rates.
	const TSharedRef<FTimerHandle> Handle = MakeShared<FTimerHandle>();
	const TWeakObjectPtr<UARTimeSubsystem> WeakClock(Clock);
	FTimerDelegate Callback = FTimerDelegate::CreateWeakLambda(Owner,
		[Event, IsLifetimeActive = MoveTemp(IsLifetimeActive), Handle, WeakClock]()
		{
			if (!Event.IsBound() || !IsLifetimeActive())
			{
				if (UARTimeSubsystem* Current = WeakClock.Get()) Current->ClearTimeGroupTimer(*Handle);
				return;
			}
			Event.ExecuteIfBound();
		});
	FTimerManagerTimerParameters Parameters;
	Parameters.bLoop = bLooping;
	Parameters.bMaxOncePerFrame = bMaxOncePerFrame;
	Parameters.FirstDelay = FirstDelay;
	FTimerManager& Timers = Clock->GetTimers(TimeGroup);
	Timers.SetTimer(*Handle, Callback, Time, Parameters);
	bSuccess = Timers.TimerExists(*Handle);
	if (bSuccess) { OwnedHandles.Add(*Handle); Clock->RememberEvent(*Handle, Event); }
	return *Handle;
}
