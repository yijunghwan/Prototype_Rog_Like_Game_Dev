#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"

namespace ARLifetimeTimer
{
	FTimerHandle Start(UObject* Owner, FTimerDynamicDelegate Event, float Time, bool bLooping,
		float InitialStartDelay, bool bMaxOncePerFrame, TFunction<bool()> IsLifetimeActive,
		TArray<FTimerHandle>& OwnedHandles, bool& bSuccess);
	void Clear(UWorld* World, TArray<FTimerHandle>& OwnedHandles);
}
