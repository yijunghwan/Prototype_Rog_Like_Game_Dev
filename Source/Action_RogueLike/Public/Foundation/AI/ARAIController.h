#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ARAIController.generated.h"

class UARMovementControlComponent;
struct FAIMoveRequest;
struct FPathFollowingRequestResult;

/** Routes ordinary enemy path following through the shared CC movement rules. */
UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual FPathFollowingRequestResult MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath = nullptr) override;

	/** Starts a path-following request. The return value reports request acceptance, not arrival. */
	UFUNCTION(BlueprintCallable, Category="AR|AI|Movement", meta=(DisplayName="AR AI Move To Actor"))
	EPathFollowingRequestResult::Type ARMoveToActor(AActor* Goal, float AcceptanceRadius = -1.0f);

	UFUNCTION(BlueprintCallable, Category="AR|AI|Movement", meta=(DisplayName="AR AI Move To Location"))
	EPathFollowingRequestResult::Type ARMoveToLocation(FVector Destination, float AcceptanceRadius = -1.0f);

	UFUNCTION(BlueprintPure, Category="AR|AI|Movement")
	bool CanRequestBasicMove() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleMovementLockChanged(AActor* Target, bool bCanBasicMove, bool bCanMoveAtAll);

	void UnbindMovementControl();

	UPROPERTY(Transient)
	TObjectPtr<UARMovementControlComponent> MovementControlComponent;
};
