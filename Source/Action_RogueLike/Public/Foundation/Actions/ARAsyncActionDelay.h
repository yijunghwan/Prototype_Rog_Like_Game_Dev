#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARAsyncActionDelay.generated.h"

class UARActionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FARActionDelayOutputSignature);

UCLASS()
class ACTION_ROGUELIKE_API UARAsyncActionDelay : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="AR|Action", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Action Delay"))
	static UARAsyncActionDelay* ActionDelay(UObject* WorldContextObject, UARActionComponent* ActionComponent, FARActionHandle ActionHandle, float Duration);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable) FARActionDelayOutputSignature Completed;
	UPROPERTY(BlueprintAssignable) FARActionDelayOutputSignature Cancelled;

private:
	void Finish(bool bWasCancelled);

	UFUNCTION() void HandleActionEnded(FARActionHandle EndedHandle);
	UFUNCTION() void HandleActionCancelled(FARActionHandle CancelledHandle, EARActionCancelReason Reason);

	UPROPERTY() TObjectPtr<UObject> WorldContextObject;
	UPROPERTY() TObjectPtr<UARActionComponent> ActionComponent;
	UPROPERTY() FARActionHandle ActionHandle;
	float Duration = 0.0f;
	FTimerHandle TimerHandle;
	bool bFinished = false;
};
