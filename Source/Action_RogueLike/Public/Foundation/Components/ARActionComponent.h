#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Actions/ARActionTypes.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Combat/ARStaggerTypes.h"
#include "ARActionComponent.generated.h"

class UARMovementControlComponent;
class UARStaggerComponent;

USTRUCT()
struct FARActiveAction
{
	GENERATED_BODY()

	UPROPERTY() FARActionHandle Handle;
	UPROPERTY() FARActionRequest Request;
	UPROPERTY() TArray<FARStatModifierHandle> StatModifiers;
	UPROPERTY() TArray<FARSuperArmorHandle> SuperArmorHandles;
	UPROPERTY() TArray<FARMovementLockHandle> MovementLockHandles;
	UPROPERTY() TArray<TWeakObjectPtr<AActor>> HitboxActors;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARActionEndedSignature, FARActionHandle, Handle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARActionCancelledSignature, FARActionHandle, Handle, EARActionCancelReason, Reason);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARActionComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="AR|Action")
	FARRequestStatus CanStartAction(const FARActionRequest& Request) const;

	UFUNCTION(BlueprintCallable, Category="AR|Action")
	FARActionHandle TryStartAction(const FARActionRequest& Request, FARRequestStatus& Status);

	UFUNCTION(BlueprintCallable, Category="AR|Action") bool EndAction(FARActionHandle Handle);
	UFUNCTION(BlueprintCallable, Category="AR|Action") bool CancelAction(FARActionHandle Handle, EARActionCancelReason Reason);
	UFUNCTION(BlueprintCallable, Category="AR|Action") int32 CancelActionsByReason(EARActionCancelReason Reason);
	UFUNCTION(BlueprintCallable, Category="AR|Action") int32 CancelRollActions(EARActionCancelReason Reason = EARActionCancelReason::Root);
	UFUNCTION(BlueprintCallable, Category="AR|Action") int32 CancelAllActions(EARActionCancelReason Reason = EARActionCancelReason::Manual);
	UFUNCTION(BlueprintCallable, Category="AR|Action") int32 CancelActionsByItemInstance(FGuid ItemInstanceId, EARActionCancelReason Reason = EARActionCancelReason::ItemRemoved);

	UFUNCTION(BlueprintCallable, Category="AR|Action") bool SetActionCancelRules(FARActionHandle Handle, const FARActionCancelRules& Rules);
	UFUNCTION(BlueprintCallable, Category="AR|Action") bool SetActionRollBlocked(FARActionHandle Handle, bool bBlocked);
	UFUNCTION(BlueprintCallable, Category="AR|Action") bool SetActionBasicMovementBlocked(FARActionHandle Handle, bool bBlocked);
	UFUNCTION(BlueprintCallable, Category="AR|Action") bool RegisterActionHitbox(FARActionHandle Handle, AActor* HitboxActor);
	UFUNCTION(BlueprintCallable, Category="AR|Action") FARStatModifierHandle ApplyActionStatModifier(FARActionHandle Handle, const FARStatModifierSpec& Spec, bool& bSuccess);
	UFUNCTION(BlueprintCallable, Category="AR|Action") FARSuperArmorHandle ApplyActionSuperArmor(FARActionHandle Handle, const FARSuperArmorSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintPure, Category="AR|Action") bool IsActionActive(FARActionHandle Handle) const;
	UFUNCTION(BlueprintPure, Category="AR|Action") bool IsRollBlocked() const;
	UFUNCTION(BlueprintPure, Category="AR|Action") bool IsBasicMovementBlocked() const;
	UFUNCTION(BlueprintPure, Category="AR|Action") int32 GetActiveActionCount() const { return ActiveActions.Num(); }

	UPROPERTY(BlueprintAssignable, Category="AR|Action") FARActionEndedSignature OnActionEnded;
	UPROPERTY(BlueprintAssignable, Category="AR|Action") FARActionCancelledSignature OnActionCancelled;

private:
	FARActiveAction* FindActiveAction(FARActionHandle Handle);
	const FARActiveAction* FindActiveAction(FARActionHandle Handle) const;
	void CleanupAction(FARActiveAction& Action);
	bool ShouldCancelForReason(const FARActiveAction& Action, EARActionCancelReason Reason) const;

	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) TObjectPtr<UARStaggerComponent> StaggerComponent;
	UPROPERTY(Transient) TObjectPtr<UARMovementControlComponent> MovementComponent;
	UPROPERTY(Transient) TArray<FARActiveAction> ActiveActions;
	int64 NextActionSerial = 1;
};
