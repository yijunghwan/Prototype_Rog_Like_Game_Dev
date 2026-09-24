#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "Foundation/Interfaces/ARCombatTargetInterface.h"
#include "Foundation/Combat/ARDamageTypes.h"
#include "Foundation/Combat/ARStaggerTypes.h"
#include "Foundation/Status/ARStatusEffectTypes.h"
#include "ARBaseCharacter.generated.h"

class UARActionComponent;
class UARHealthComponent;
class UARMovementControlComponent;
class UARStaggerComponent;
class UARStatsComponent;
class UARStatusEffectComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARCharacterDeathSignature, AARBaseCharacter*, Character, const FARCombatDamageResult&, KillingDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARAimDirectionChangedSignature, AARBaseCharacter*, Character, FVector, AimDirection);

UCLASS(Abstract, Blueprintable)
class ACTION_ROGUELIKE_API AARBaseCharacter : public APaperCharacter, public IARCombatTargetInterface
{
	GENERATED_BODY()

public:
	AARBaseCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual EARCombatTeam GetCombatTeam() const override { return CombatTeam; }
	virtual bool CanBeCombatTarget() const override;

	UFUNCTION(BlueprintPure, Category="AR|Character") FVector GetAimDirection() const { return AimDirection; }
	UFUNCTION(BlueprintCallable, Category="AR|Character") void SetAimDirection(FVector NewAimDirection);

	UFUNCTION(BlueprintPure, Category="AR|Character") UARStatsComponent* GetStatsComponent() const { return StatsComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Character") UARHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Character") UARStatusEffectComponent* GetStatusEffectComponent() const { return StatusEffectComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Character") UARStaggerComponent* GetStaggerComponent() const { return StaggerComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Character") UARActionComponent* GetActionComponent() const { return ActionComponent; }
	UFUNCTION(BlueprintPure, Category="AR|Character") UARMovementControlComponent* GetMovementControlComponent() const { return MovementControlComponent; }

	UPROPERTY(BlueprintAssignable, Category="AR|Character") FARCharacterDeathSignature OnCharacterDeath;
	UPROPERTY(BlueprintAssignable, Category="AR|Character") FARAimDirectionChangedSignature OnAimDirectionChanged;

protected:
	UFUNCTION() void HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue);
	UFUNCTION() void HandleCharacterDeath(AActor* Target, const FARCombatDamageResult& KillingDamage);
	UFUNCTION() void HandleStaggered(AActor* Target, const FARStaggerResult& Result);
	UFUNCTION() void HandleStaggerStateChanged(AActor* Target, bool bIsStaggered);
	UFUNCTION() void HandleStatusAdded(AActor* Target, const FARStatusEffectView& Status);
	UFUNCTION() void HandleStatusUpdated(AActor* Target, const FARStatusEffectView& Status);
	UFUNCTION() void HandleStatusRemoved(AActor* Target, const FARStatusEffectView& Status);
	void RefreshStatusMovementLock();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Character") EARCombatTeam CombatTeam = EARCombatTeam::Enemy;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARStatusEffectComponent> StatusEffectComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARStaggerComponent> StaggerComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARActionComponent> ActionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Components") TObjectPtr<UARMovementControlComponent> MovementControlComponent;

private:
	UPROPERTY(Transient) FVector AimDirection = FVector::ForwardVector;
	UPROPERTY(Transient) FARMovementLockHandle StatusMovementLock;
	UPROPERTY(Transient) FARMovementLockHandle StaggerMovementLock;
	UPROPERTY(Transient) FARMovementLockHandle DeathMovementLock;
	EARMovementLockType CurrentStatusLockType = EARMovementLockType::BasicMovementOnly;
};
