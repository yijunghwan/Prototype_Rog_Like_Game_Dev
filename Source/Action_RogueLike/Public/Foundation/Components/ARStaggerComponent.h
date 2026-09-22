#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Combat/ARStaggerTypes.h"
#include "ARStaggerComponent.generated.h"

class UARStatsComponent;

USTRUCT()
struct FARActiveSuperArmor
{
	GENERATED_BODY()

	UPROPERTY() FARSuperArmorHandle Handle;
	UPROPERTY() FARSourceInfo Source;
	UPROPERTY() double ExpireAt = -1.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARStaggeredSignature, AActor*, Target, const FARStaggerResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARStaggerStateChangedSignature, AActor*, Target, bool, bIsStaggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FARGroggyChangedSignature, AActor*, Target, float, Current, float, Maximum, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARGroggyDepletedSignature, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARSuperArmorChangedSignature, AActor*, Target, bool, bIsActive);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARStaggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARStaggerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="AR|Stagger")
	FARStaggerResult ApplyStaggerAndGroggyDamage(const FARStaggerRequest& Request);

	UFUNCTION(BlueprintCallable, Category="AR|Stagger")
	FARSuperArmorHandle AddSuperArmor(const FARSuperArmorSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Stagger")
	bool RemoveSuperArmor(FARSuperArmorHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Stagger")
	int32 RemoveSuperArmorBySource(EARModifierSourceCategory Category, FName SourceId);

	UFUNCTION(BlueprintPure, Category="AR|Stagger")
	bool IsSuperArmorActive() const { return ActiveSuperArmor.Num() > 0; }

	UFUNCTION(BlueprintPure, Category="AR|Stagger")
	bool IsStaggered() const;

	UFUNCTION(BlueprintPure, Category="AR|Stagger")
	bool IsStaggerImmune() const;

	UFUNCTION(BlueprintPure, Category="AR|Stagger")
	float GetCurrentGroggy() const { return CurrentGroggy; }

	UFUNCTION(BlueprintPure, Category="AR|Stagger")
	float GetMaxGroggy() const;

	UFUNCTION(BlueprintCallable, Category="AR|Stagger")
	float ResetGroggyGauge(float NewCurrentValue = -1.0f);

	UPROPERTY(BlueprintAssignable, Category="AR|Stagger") FARStaggeredSignature OnStaggered;
	UPROPERTY(BlueprintAssignable, Category="AR|Stagger") FARStaggerStateChangedSignature OnStaggerStateChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Stagger") FARGroggyChangedSignature OnGroggyChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Stagger") FARGroggyDepletedSignature OnGroggyGaugeDepleted;
	UPROPERTY(BlueprintAssignable, Category="AR|Stagger") FARSuperArmorChangedSignature OnSuperArmorChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stagger") bool bCanBeStaggered = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stagger", meta=(ClampMin="0.0")) float BaseStaggerDuration = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stagger", meta=(ClampMin="0.0")) float PostStaggerImmunityDuration = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Groggy") bool bUseGroggyGauge = false;

private:
	UFUNCTION() void HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue);
	double GetNow() const;

	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) TArray<FARActiveSuperArmor> ActiveSuperArmor;
	UPROPERTY(Transient) float CurrentGroggy = 0.0f;
	double StaggeredUntil = -1.0;
	double StaggerImmuneUntil = -1.0;
	double LastGroggyDamageAt = -1.0;
	bool bGroggyDepletedLocked = false;
	bool bWasStaggeredLastTick = false;
};

