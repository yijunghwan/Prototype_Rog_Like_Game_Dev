#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARManaComponent.generated.h"

class UARStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FARManaChangedSignature, AActor*, Target, float, Current, float, Maximum, float, Delta, EARResourceChangeReason, Reason, const FARSourceInfo&, Source);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARManaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARManaComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="AR|Mana") bool CanAfford(float Amount) const;
	UFUNCTION(BlueprintCallable, Category="AR|Mana") bool TryConsume(float Amount, const FARSourceInfo& Source, float& NewMana);
	UFUNCTION(BlueprintCallable, Category="AR|Mana") float Restore(float Amount, const FARSourceInfo& Source);
	UFUNCTION(BlueprintPure, Category="AR|Mana") float GetCurrent() const { return CurrentMana; }
	UFUNCTION(BlueprintPure, Category="AR|Mana") float GetMax() const;
	UFUNCTION(BlueprintPure, Category="AR|Mana") float GetRatio() const;

	UPROPERTY(BlueprintAssignable, Category="AR|Mana") FARManaChangedSignature OnResourceChanged;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Mana") bool bStartFull = true;

private:
	UFUNCTION() void HandleFinalStatChanged(AActor* Target, EARStatType StatType, float OldValue, float NewValue);
	void BroadcastChange(float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source);

	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) float CurrentMana = 0.0f;
};

