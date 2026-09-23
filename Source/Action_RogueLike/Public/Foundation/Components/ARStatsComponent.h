#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARStatsComponent.generated.h"

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatModifierSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat")
	EARStatType StatType = EARStatType::AttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat")
	EARStatModifierOperation Operation = EARStatModifierOperation::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat")
	float Duration = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat")
	FARSourceInfo Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guarantee")
	bool bGuaranteeInvulnerability = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guarantee")
	bool bGuaranteeEvasion = false;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatBreakdown
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") float BaseValue = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") float FlatTotal = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") float AdditivePercentTotal = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") float MultiplicativeProduct = 1.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") float FinalValue = 0.0f;
};

/** Ordered, read-only stat data used by character-sheet and inventory widgets. */
USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARFinalStatView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") EARStatType StatType = EARStatType::MaxHealth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") FText DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") FARStatBreakdown Breakdown;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatModifierQueryResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") bool bExists = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") int32 StackCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") bool bHasPermanent = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stat") float LongestRemainingTime = 0.0f;
};

USTRUCT()
struct FARActiveStatModifier
{
	GENERATED_BODY()

	UPROPERTY() FARStatModifierHandle Handle;
	UPROPERTY() FARStatModifierSpec Spec;
	UPROPERTY() double AppliedAt = 0.0;
	UPROPERTY() double ExpireAt = -1.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FARFinalStatChangedSignature, AActor*, Target, EARStatType, StatType, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FARStatModifiersChangedSignature, AActor*, Target, FName, SourceId, FText, DisplayName, int32, StackCount, float, LongestRemainingTime);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARStatsComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	FARStatModifierHandle AddStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	bool RemoveStatModifier(FARStatModifierHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	int32 RemoveModifiers(EARModifierSourceCategory Category, FName SourceId);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	int32 ClearModifiers(EARModifierSourceCategory Category = EARModifierSourceCategory::All);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	bool RemoveOneModifierStack(EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle);

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	float GetFinalStat(EARStatType StatType) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	float GetBaseStat(EARStatType StatType) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	FARStatBreakdown GetStatBreakdown(EARStatType StatType) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	TArray<FARFinalStatView> GetAllFinalStatViews() const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	FARStatModifierQueryResult GetModifiersBySource(EARModifierSourceCategory Category, FName SourceId) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	bool GetModifierRemainingTime(FARStatModifierHandle Handle, bool& bIsPermanent, float& RemainingSeconds) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	float GetDamageRemainingMultiplier(EARStatType ReductionStat) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	bool HasGuaranteedInvulnerability() const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	bool HasGuaranteedEvasion() const;

	void SetBaseStat(EARStatType StatType, float Value);

	UPROPERTY(BlueprintAssignable, Category="AR|Stats")
	FARFinalStatChangedSignature OnFinalStatChanged;

	UPROPERTY(BlueprintAssignable, Category="AR|Stats")
	FARStatModifiersChangedSignature OnStatModifiersChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stats|Base")
	TMap<EARStatType, float> BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stats|Caps", meta=(ClampMin="0", ClampMax="100"))
	float MaxEvasion = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stats|Caps", meta=(ClampMin="0", ClampMax="100"))
	float MaxCriticalChance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stats|Caps", meta=(ClampMin="0", ClampMax="100"))
	float MaxTenacity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Stats|Caps", meta=(ClampMin="0", ClampMax="100"))
	float MaxCooldownReduction = 40.0f;

private:
	void RecalculateAll();
	void RecalculateStat(EARStatType StatType);
	float CalculateGeneralStat(EARStatType StatType, FARStatBreakdown* OutBreakdown = nullptr) const;
	float ApplySafetyRules(EARStatType StatType, float Value) const;
	void BroadcastSourceChange(const FARSourceInfo& Source);
	double GetNow() const;
	static bool IsReductionStat(EARStatType StatType);

	UPROPERTY(Transient)
	TArray<FARActiveStatModifier> ActiveModifiers;

	UPROPERTY(Transient)
	TMap<EARStatType, float> CachedFinalStats;
};
