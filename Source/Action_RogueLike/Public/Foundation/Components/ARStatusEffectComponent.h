#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Foundation/Status/ARStatusEffectTypes.h"
#include "ARStatusEffectComponent.generated.h"

class UARStatsComponent;

USTRUCT()
struct FARActiveStatusEffect
{
	GENERATED_BODY()

	UPROPERTY() FARStatusEffectHandle Handle;
	UPROPERTY() TObjectPtr<const UARStatusEffectDefinition> Definition = nullptr;
	UPROPERTY() FARSourceInfo Source;
	UPROPERTY() double ExpireAt = 0.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARStatusEffectChangedSignature, AActor*, Target, const FARStatusEffectView&, Status);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARStatusEffectComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="AR|Status")
	FARStatusEffectResult ApplyStatusEffect(const FARStatusEffectRequest& Request);

	UFUNCTION(BlueprintCallable, Category="AR|Status")
	bool RemoveStatusEffect(FARStatusEffectHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Status")
	int32 RemoveStatusEffectsBySource(EARModifierSourceCategory Category, FName SourceId);

	UFUNCTION(BlueprintCallable, Category="AR|Status")
	int32 ClearAllStatusEffects();

	UFUNCTION(BlueprintPure, Category="AR|Status")
	bool HasStatus(FGameplayTag StatusTag) const;

	UFUNCTION(BlueprintPure, Category="AR|Status")
	bool GetStatusRemainingTime(FGameplayTag StatusTag, float& RemainingTime) const;

	UFUNCTION(BlueprintPure, Category="AR|Status")
	TArray<FARStatusEffectView> GetActiveStatusEffects() const;

	UFUNCTION(BlueprintPure, Category="AR|Status") bool BlocksBasicMovement() const;
	UFUNCTION(BlueprintPure, Category="AR|Status") bool BlocksAllMovement() const;
	UFUNCTION(BlueprintPure, Category="AR|Status") bool BlocksRoll() const;
	UFUNCTION(BlueprintPure, Category="AR|Status") bool BlocksSkillGroups() const;

	UPROPERTY(BlueprintAssignable, Category="AR|Status") FARStatusEffectChangedSignature OnStatusAdded;
	UPROPERTY(BlueprintAssignable, Category="AR|Status") FARStatusEffectChangedSignature OnStatusUpdated;
	UPROPERTY(BlueprintAssignable, Category="AR|Status") FARStatusEffectChangedSignature OnStatusRemoved;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Status") FGameplayTagContainer ImmuneStatusTags;

private:
	FARStatusEffectView MakeView(const FARActiveStatusEffect& Effect) const;
	void RefreshTickState();
	double GetNow() const;

	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) TArray<FARActiveStatusEffect> ActiveEffects;
};
