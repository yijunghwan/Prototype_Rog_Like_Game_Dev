#include "Foundation/Actions/ARLifetimeTimer.h"
#include "Engine/World.h"

FTimerHandle ARLifetimeTimer::Start(UObject* Owner, FTimerDynamicDelegate Event, float Time, bool bLooping,
	float InitialStartDelay, bool bMaxOncePerFrame, TFunction<bool()> IsLifetimeActive,
	TArray<FTimerHandle>& OwnedHandles, bool& bSuccess)
{
	bSuccess = false;
	UWorld* World = IsValid(Owner) ? Owner->GetWorld() : nullptr;
	if (!World || World->bIsTearingDown || !Event.IsBound() || !IsLifetimeActive()
		|| !FMath::IsFinite(Time) || Time <= 0.0f || !FMath::IsFinite(InitialStartDelay)
		|| (InitialStartDelay < 0.0f && InitialStartDelay != -1.0f)) return FTimerHandle();
	const UObject* CallbackObject = Event.GetUObject();
	if (!IsValid(CallbackObject) || (CallbackObject->GetWorld() && CallbackObject->GetWorld() != World)) return FTimerHandle();

	FTimerManager& Timers = World->GetTimerManager();
	OwnedHandles.RemoveAll([&Timers](FTimerHandle Handle) { return !Timers.TimerExists(Handle); });
	const TSharedRef<FTimerHandle> Handle = MakeShared<FTimerHandle>();
	const TWeakObjectPtr<UWorld> WeakWorld(World);
	// Neither the delegate nor the lifetime predicate holds its owner alive.
	FTimerDelegate Callback = FTimerDelegate::CreateWeakLambda(Owner,
		[Event, IsLifetimeActive = MoveTemp(IsLifetimeActive), Handle, WeakWorld]()
		{
			if (!Event.IsBound() || !IsLifetimeActive())
			{
				if (UWorld* CurrentWorld = WeakWorld.Get()) CurrentWorld->GetTimerManager().ClearTimer(*Handle);
				return;
			}
			Event.ExecuteIfBound(); // Can end/cancel/unregister and clear this executing timer.
		});
	FTimerManagerTimerParameters Parameters;
	Parameters.bLoop = bLooping;
	Parameters.bMaxOncePerFrame = bMaxOncePerFrame;
	Parameters.FirstDelay = InitialStartDelay;
	Timers.SetTimer(*Handle, Callback, Time, Parameters);
	bSuccess = Timers.TimerExists(*Handle);
	if (bSuccess) OwnedHandles.Add(*Handle);
	return *Handle;
}

void ARLifetimeTimer::Clear(UWorld* World, TArray<FTimerHandle>& OwnedHandles)
{
	const TArray<FTimerHandle> Handles = MoveTemp(OwnedHandles);
	OwnedHandles.Reset();
	if (World)
	{
		for (FTimerHandle Handle : Handles) World->GetTimerManager().ClearTimer(Handle);
	}
}
