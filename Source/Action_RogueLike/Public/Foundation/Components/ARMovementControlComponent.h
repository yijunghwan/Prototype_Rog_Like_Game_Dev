#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARMovementControlComponent.generated.h"

class ACharacter;
class UARActionComponent;

USTRUCT()
struct FARActiveMovementLock
{
	GENERATED_BODY()

	UPROPERTY() FARMovementLockHandle Handle;
	UPROPERTY() FName SourceId = NAME_None;
	UPROPERTY() EARMovementLockType LockType = EARMovementLockType::BasicMovementOnly;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FARMovementLockChangedSignature, AActor*, Target, bool, bCanBasicMove, bool, bCanMoveAtAll);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARMovementControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARMovementControlComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	bool RequestBasicMove(FVector Direction, float Scale = 1.0f);

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	bool RequestActionMove(FARActionHandle ActionHandle, FVector Direction, float Scale = 1.0f);

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	bool RequestActionVelocity(FARActionHandle ActionHandle, FVector Direction, float Speed);

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	FARMovementLockHandle AcquireMovementLock(FName SourceId, EARMovementLockType LockType);

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	bool ReleaseMovementLock(FARMovementLockHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	int32 ReleaseMovementLocksBySource(FName SourceId);

	UFUNCTION(BlueprintPure, Category="AR|Movement")
	bool CanBasicMove() const;

	UFUNCTION(BlueprintPure, Category="AR|Movement")
	bool CanMoveAtAll() const;

	UFUNCTION(BlueprintCallable, Category="AR|Movement")
	void StopMovementImmediately();

	UPROPERTY(BlueprintAssignable, Category="AR|Movement") FARMovementLockChangedSignature OnMovementLockChanged;

private:
	bool IsActiveOwnedAction(FARActionHandle ActionHandle) const;
	void RefreshMovementMode(bool bCouldMoveAtAll);

	UPROPERTY(Transient) TObjectPtr<ACharacter> CharacterOwner;
	UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> CharacterMovement;
	UPROPERTY(Transient) TObjectPtr<UARActionComponent> ActionComponent;
	UPROPERTY(Transient) TArray<FARActiveMovementLock> ActiveLocks;
	EMovementMode SavedMovementMode = MOVE_Walking;
	uint8 SavedCustomMovementMode = 0;
};
