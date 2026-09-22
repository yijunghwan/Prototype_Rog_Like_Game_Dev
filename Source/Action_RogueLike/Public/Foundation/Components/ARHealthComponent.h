#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Combat/ARDamageTypes.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARHealthComponent.generated.h"

class UARStatsComponent;

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARShieldSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shield", meta=(ClampMin="0.0"))
	float Amount = 0.0f;

	/** Negative means permanent. Zero is rejected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shield")
	float Duration = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shield")
	FARSourceInfo Source;
};

USTRUCT()
struct FARActiveShield
{
	GENERATED_BODY()

	UPROPERTY() FARShieldHandle Handle;
	UPROPERTY() FARSourceInfo Source;
	UPROPERTY() float RemainingAmount = 0.0f;
	UPROPERTY() double AppliedAt = 0.0;
	UPROPERTY() double ExpireAt = -1.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FARHealthChangedSignature, AActor*, Target, float, CurrentHealth, float, MaxHealth, float, Delta, EARResourceChangeReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FARShieldChangedSignature, AActor*, Target, float, CurrentShield, float, Delta, FARShieldHandle, Handle, EARResourceChangeReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARDamageAppliedSignature, AActor*, Target, const FARCombatDamageResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARDeathSignature, AActor*, Target, const FARCombatDamageResult&, KillingDamage);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARHealthComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="AR|Health")
	bool CanAcceptResolvedDamage() const;

	/** C++ combat entry point. The resolver supplies the planned shield/health split. */
	bool ApplyResolvedDamage(FARCombatDamageResult& InOutResult);

	/** Used by the resolver before ApplyResolvedDamage. Does not mutate state. */
	void PreviewDamageAllocation(int32 FinalDamage, bool bIgnoreShield, int32& OutShieldDamage, int32& OutHealthDamage) const;

	UFUNCTION(BlueprintCallable, Category="AR|Health")
	float RestoreHealth(float Amount, const FARSourceInfo& Source);

	UFUNCTION(BlueprintCallable, Category="AR|Health")
	FARShieldHandle ApplyShield(const FARShieldSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Health")
	bool RemoveShield(FARShieldHandle Handle, float& RemovedAmount);

	UFUNCTION(BlueprintCallable, Category="AR|Health")
	int32 RemoveShieldsBySource(EARModifierSourceCategory Category, FName SourceId, EARShieldLifetimeFilter LifetimeFilter, float& RemovedAmount);

	UFUNCTION(BlueprintCallable, Category="AR|Health")
	int32 ClearShields(float& RemovedAmount);

	UFUNCTION(BlueprintPure, Category="AR|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="AR|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category="AR|Health")
	float GetHealthRatio() const;

	UFUNCTION(BlueprintPure, Category="AR|Health")
	float GetCurrentShield() const;

	UFUNCTION(BlueprintPure, Category="AR|Health")
	bool IsDead() const { return bDead; }

	UPROPERTY(BlueprintAssignable, Category="AR|Health") FARHealthChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Health") FARShieldChangedSignature OnShieldChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Health") FARDamageAppliedSignature OnDamageApplied;
	UPROPERTY(BlueprintAssignable, Category="AR|Health") FARDeathSignature OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Health")
	bool bStartAtFullHealth = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Health")
	bool bDamageSystemEnabled = true;

private:
	UFUNCTION()
	void HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue);

	float ConsumeShieldLifo(float Amount);
	void RefreshShieldTickState();
	double GetNow() const;

	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) float CurrentHealth = 0.0f;
	UPROPERTY(Transient) bool bDead = false;
	UPROPERTY(Transient) TArray<FARActiveShield> ActiveShields;
};
