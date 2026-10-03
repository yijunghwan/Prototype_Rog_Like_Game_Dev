#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Foundation/Time/ARTimeSubsystem.h"

namespace ARLifetimeTimer
{
	void Clear(UWorld* World, TArray<FTimerHandle>& OwnedHandles);
	FTimerHandle StartGrouped(UObject* Owner, FTimerDynamicDelegate Event, float Time, bool bLooping,
		EARTimeGroup TimeGroup, float InitialStartDelay, float InitialStartDelayVariance, bool bMaxOncePerFrame,
		TFunction<bool()> IsLifetimeActive, TArray<FTimerHandle>& OwnedHandles, bool& bSuccess);
}
