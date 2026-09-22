#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARStaminaComponent.generated.h"

class UARStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FARStaminaChangedSignature, AActor*, Target, float, Current, float, Maximum, float, Delta, EARResourceChangeReason, Reason, const FARSourceInfo&, Source);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARStaminaComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="AR|Stamina") bool CanAfford(float Amount) const;
	UFUNCTION(BlueprintCallable, Category="AR|Stamina") bool TryConsume(float Amount, const FARSourceInfo& Source, float& NewStamina);
	UFUNCTION(BlueprintCallable, Category="AR|Stamina") float Restore(float Amount, const FARSourceInfo& Source);
	UFUNCTION(BlueprintPure, Category="AR|Stamina") float GetCurrent() const { return CurrentStamina; }
	UFUNCTION(BlueprintPure, Category="AR|Stamina") float GetMax() const;
	UFUNCTION(BlueprintPure, Category="AR|Stamina") float GetRatio() const;

	UPROPERTY(BlueprintAssignable, Category="AR|Stamina") FARStaminaChangedSignature OnResourceChanged;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stamina") bool bStartFull = true;

private:
	UFUNCTION() void HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue);
	void BroadcastChange(float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source);
	double GetNow() const;

	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) float CurrentStamina = 0.0f;
	double LastSuccessfulConsumeAt = -1.0;
};

