#include "Foundation/Actions/ARAsyncActionDelay.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Foundation/Components/ARActionComponent.h"
#include "TimerManager.h"

UARAsyncActionDelay* UARAsyncActionDelay::ActionDelay(UObject* InWorldContextObject, UARActionComponent* InActionComponent, FARActionHandle InActionHandle, float InDuration)
{
	UARAsyncActionDelay* Node = NewObject<UARAsyncActionDelay>();
	Node->WorldContextObject = InWorldContextObject;
	Node->ActionComponent = InActionComponent;
	Node->ActionHandle = InActionHandle;
	Node->Duration = FMath::Max(0.0f, InDuration);
	Node->RegisterWithGameInstance(InWorldContextObject);
	return Node;
}

void UARAsyncActionDelay::Activate()
{
	if (!ActionComponent || !ActionComponent->IsActionActive(ActionHandle))
	{
		Finish(true);
		return;
	}
	ActionComponent->OnActionEnded.AddDynamic(this, &UARAsyncActionDelay::HandleActionEnded);
	ActionComponent->OnActionCancelled.AddDynamic(this, &UARAsyncActionDelay::HandleActionCancelled);
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		Finish(true);
		return;
	}
	if (Duration <= 0.0f)
	{
		Finish(false);
		return;
	}
	World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &UARAsyncActionDelay::Finish, false), Duration, false);
}

void UARAsyncActionDelay::Finish(bool bWasCancelled)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
	if (ActionComponent)
	{
		ActionComponent->OnActionEnded.RemoveDynamic(this, &UARAsyncActionDelay::HandleActionEnded);
		ActionComponent->OnActionCancelled.RemoveDynamic(this, &UARAsyncActionDelay::HandleActionCancelled);
	}
	if (bWasCancelled)
	{
		Cancelled.Broadcast();
	}
	else
	{
		Completed.Broadcast();
	}
	SetReadyToDestroy();
}

void UARAsyncActionDelay::HandleActionEnded(FARActionHandle EndedHandle)
{
	if (EndedHandle == ActionHandle)
	{
		Finish(true);
	}
}

void UARAsyncActionDelay::HandleActionCancelled(FARActionHandle CancelledHandle, EARActionCancelReason Reason)
{
	if (CancelledHandle == ActionHandle)
	{
		Finish(true);
	}
}
