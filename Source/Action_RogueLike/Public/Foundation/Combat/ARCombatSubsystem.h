#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Foundation/Combat/ARDamageTypes.h"
#include "Foundation/Combat/ARDotTypes.h"
#include "ARCombatSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARCombatResultEventSignature, AActor*, Subject, const FARCombatDamageResult&, Result);

struct FARActiveDotInstance
{
	FARDotHandle Handle;
	FARDamageOverTimeSpec Spec;
	TWeakObjectPtr<AActor> Attacker;
	TWeakObjectPtr<AActor> Target;
	FAROffensiveStatSnapshot LastOffensiveSnapshot;
	double ExpireAt = 0.0;
	double NextTickAt = 0.0;
};

UCLASS()
class ACTION_ROGUELIKE_API UARCombatSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintPure, Category="AR|Combat")
	bool CanDamageTarget(AActor* Attacker, AActor* Target, EARRequestResult& FailureReason) const;

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	FARCombatDamageResult ApplyCombatDamage(const FARCombatDamageRequest& Request);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	FARDotHandle ApplyDamageOverTime(const FARDamageOverTimeSpec& Spec, bool& bSuccess, EARRequestResult& FailureReason);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	bool RemoveDamageOverTime(FARDotHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	int32 RemoveDamageOverTimeBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId, FName DotName = NAME_None);

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	int32 RemoveAllDamageOverTimeFromTarget(AActor* Target);

	UFUNCTION(BlueprintPure, Category="AR|Combat")
	int32 GetActiveDamageOverTimeCount(AActor* Target) const;

	UPROPERTY(BlueprintAssignable, Category="AR|Combat") FARCombatResultEventSignature OnDamageReceived;
	UPROPERTY(BlueprintAssignable, Category="AR|Combat") FARCombatResultEventSignature OnDamageHit;
	UPROPERTY(BlueprintAssignable, Category="AR|Combat") FARCombatResultEventSignature OnDamageEvaded;
	UPROPERTY(BlueprintAssignable, Category="AR|Combat") FARCombatResultEventSignature OnDamageBlocked;

private:
	FARCombatDamageResult ProcessDamageRequest(const FARCombatDamageRequest& Request, bool bSourceAlreadyValidated = false);
	FAROffensiveStatSnapshot CaptureOffensiveSnapshot(const AActor* Attacker, EARDamageDelivery Delivery, EARDamageAttribute Attribute) const;
	bool TryGetCombatTeam(const AActor* Actor, EARCombatTeam& OutTeam, bool& bOutCanBeTarget) const;
	void ApplyAbsorption(const FARCombatDamageRequest& Request, const FARCombatDamageResult& Result, const FAROffensiveStatSnapshot& Offense);
	bool IsRefreshDotDefinitionEqual(const FARDamageOverTimeSpec& A, const FARDamageOverTimeSpec& B) const;

	UPROPERTY(Transient) TArray<FARCombatDamageRequest> QueuedDamageRequests;
	TArray<FARActiveDotInstance> ActiveDots;
	TMap<TWeakObjectPtr<AActor>, double> AbsorptionRemainders;
	FRandomStream CombatRandomStream;
	bool bProcessingDamage = false;

	UPROPERTY(EditAnywhere, Category="AR|Combat", meta=(ClampMin="1"))
	int32 MaxQueuedRequestsPerChain = 256;

	UPROPERTY(EditAnywhere, Category="AR|Combat", meta=(ClampMin="1"))
	int32 ActiveDotWarningThreshold = 512;

	UPROPERTY(EditAnywhere, Category="AR|Combat", meta=(ClampMin="1"))
	int32 CatchUpTickWarningThreshold = 32;
};
