#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Foundation/Characters/ARBaseCharacter.h"
#include "ARPlayerCharacter.generated.h"

class UARCameraFollowComponent;
class UARManaComponent;
class UARStaminaComponent;
class UARLoadoutComponent;
class UARConsumableComponent;
class UARInteractionComponent;
class UARUIManagerComponent;
class UCameraComponent;
class UInputAction;
class USceneComponent;
struct FInputActionValue;
class AARPlayerCharacter;

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARSkillInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> InputAction = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input") FGameplayTag InputTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARAimWorldLocationChangedSignature, AARPlayerCharacter*, Player, FVector, WorldLocation);

UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARPlayerCharacter : public AARBaseCharacter
{
	GENERATED_BODY()

public:
	AARPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category="AR|Player") void SetGameplayInputBlocked(bool bBlocked);
	UFUNCTION(BlueprintPure, Category="AR|Player") bool IsGameplayInputBlocked() const { return bGameplayInputBlocked; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UARStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UARManaComponent* GetManaComponent() const { return ManaComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UARLoadoutComponent* GetLoadoutComponent() const { return LoadoutComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UARConsumableComponent* GetConsumableComponent() const { return ConsumableComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UARInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UARUIManagerComponent* GetUIManagerComponent() const { return UIManagerComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Player") UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	UFUNCTION(BlueprintPure, Category="AR|Player|Aim") FVector GetAimWorldLocation() const { return AimWorldLocation; }
	UFUNCTION(BlueprintPure, Category="AR|Player|Aim") bool HasAimWorldLocation() const { return bHasAimWorldLocation; }
	UFUNCTION(BlueprintCallable, Category="AR|Player|Aim") bool SetAimWorldLocation(FVector WorldLocation);
	UFUNCTION(BlueprintCallable, Category="AR|Player|Aim") void ClearAimWorldLocation();
	UFUNCTION(BlueprintPure, Category="AR|Player|Damage") float GetPostHitInvulnerabilityDuration() const { return PostHitInvulnerabilityDuration; }

	UPROPERTY(BlueprintAssignable, Category="AR|Player|Aim") FARAimWorldLocationChangedSignature OnAimWorldLocationChanged;

protected:
	void HandleMove(const FInputActionValue& Value);
	void HandleRollPressed(const FInputActionValue& Value);
	void HandleSkillPressed(const FInputActionValue& Value, FGameplayTag InputTag);
	void HandleInteractPressed(const FInputActionValue& Value);
	void HandleConsumablePressed(const FInputActionValue& Value, int32 SlotIndex);
	void HandleInventoryPressed(const FInputActionValue& Value);
	void HandleUIBackPressed(const FInputActionValue& Value);
	void UpdateAimFromCursor();
	void UpdateRoll(float DeltaSeconds);
	void FinishRoll(bool bCancel);
	void HandlePlayerDamageApplied(AActor* Target, const FARCombatDamageResult& Result);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputAction> RollAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TArray<FARSkillInputBinding> SkillInputBindings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TArray<TObjectPtr<UInputAction>> ConsumableSlotActions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputAction> InventoryAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputAction> UIBackAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Roll", meta=(ClampMin="0.01")) float RollDuration = 0.35f;
	/** Direct, non-Void damage grants this much normal invulnerability. Zero disables it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Damage", meta=(ClampMin="0.0")) float PostHitInvulnerabilityDuration = 0.35f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARStaminaComponent> StaminaComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARManaComponent> ManaComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARLoadoutComponent> LoadoutComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARConsumableComponent> ConsumableComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARInteractionComponent> InteractionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARUIManagerComponent> UIManagerComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<USceneComponent> CameraAnchor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UCameraComponent> TopDownCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARCameraFollowComponent> CameraFollowComponent;

private:
	FVector LastMoveDirection = FVector::ForwardVector;
	FVector RollDirection = FVector::ZeroVector;
	FARActionHandle ActiveRollHandle;
	FARStatModifierHandle PostHitInvulnerabilityHandle;
	FDelegateHandle PostHitDamageDelegateHandle;
	double RollEndsAt = -1.0;
	FVector AimWorldLocation = FVector::ZeroVector;
	bool bHasAimWorldLocation = false;
	bool bGameplayInputBlocked = false;
};
