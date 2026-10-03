#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARStatsComponent.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EARStatEffectDisplay : uint8
{
	Hidden,
	Buff,
	Debuff
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatModifierSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat", meta=(EditCondition="!bStackOnly", EditConditionHides, HideEditConditionToggle))
	EARStatType StatType = EARStatType::AttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat", meta=(EditCondition="!bStackOnly", EditConditionHides, HideEditConditionToggle))
	EARStatModifierOperation Operation = EARStatModifierOperation::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat", meta=(EditCondition="!bStackOnly", EditConditionHides, HideEditConditionToggle))
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat", meta=(EditCondition="Operation != EARStatModifierOperation::PermanentFlat", EditConditionHides))
	float Duration = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stat", meta=(EditCondition="Operation != EARStatModifierOperation::PermanentFlat", EditConditionHides))
	FARSourceInfo Source;

	/** Records one stack without changing or recalculating a stat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stack") bool bStackOnly = false;
	/** Pass the first application handle to attach another stat change to the same stack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stack") FARStatModifierHandle StackGroupHandle;
	/** Only affects finite durations, using the target's tenacity at application time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Duration", meta=(EditCondition="Operation != EARStatModifierOperation::PermanentFlat && Duration > 0.0", EditConditionHides)) bool bAffectedByTenacity = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(EditCondition="Operation != EARStatModifierOperation::PermanentFlat", EditConditionHides)) EARStatEffectDisplay HUDDisplay = EARStatEffectDisplay::Hidden;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(EditCondition="Operation != EARStatModifierOperation::PermanentFlat && HUDDisplay != EARStatEffectDisplay::Hidden")) FText HUDName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(EditCondition="Operation != EARStatModifierOperation::PermanentFlat && HUDDisplay != EARStatEffectDisplay::Hidden")) TSoftObjectPtr<UTexture2D> HUDIcon;

	// Keep an already-checked guarantee visible so legacy/changed specs can be corrected.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guarantee", meta=(EditCondition="bGuaranteeInvulnerability || (!bStackOnly && Operation != EARStatModifierOperation::PermanentFlat && StatType == EARStatType::OverallDamageReduction)", EditConditionHides, HideEditConditionToggle, ToolTip="Only valid for non-stack-only, non-permanent Overall Damage Reduction. Clear before switching modes."))
	bool bGuaranteeInvulnerability = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guarantee", meta=(EditCondition="bGuaranteeEvasion || (!bStackOnly && Operation != EARStatModifierOperation::PermanentFlat && StatType == EARStatType::Evasion)", EditConditionHides, HideEditConditionToggle, ToolTip="Only valid for non-stack-only, non-permanent Evasion. Clear before switching modes."))
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

/** One visible source, with one stack per application group (not per stat change). */
USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatEffectView
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARModifierSourceCategory Category = EARModifierSourceCategory::Other;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName SourceId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EARStatEffectDisplay Display = EARStatEffectDisplay::Hidden;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 StackCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHasPermanent = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float LongestRemainingTime = 0.0f;
};

USTRUCT()
struct FARActiveStatModifier
{
	GENERATED_BODY()

	UPROPERTY() FARStatModifierHandle Handle;
	UPROPERTY() FARStatModifierSpec Spec;
	UPROPERTY() FGuid StackGroupId;
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Native ownership transactions defer notifications until their handles are recorded.
	 * Nested BP mutations are immediate, but their notifications are drained without recursion. */
	class ACTION_ROGUELIKE_API FScopedNotifications
	{
	public:
		explicit FScopedNotifications(UARStatsComponent* InStats);
		~FScopedNotifications();
		FScopedNotifications(const FScopedNotifications&) = delete;
		FScopedNotifications& operator=(const FScopedNotifications&) = delete;
	private:
		UARStatsComponent* Stats;
	};

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	FARStatModifierHandle AddStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	bool RemoveStatModifier(FARStatModifierHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	int32 RemoveModifiers(EARModifierSourceCategory Category, FName SourceId);

	UFUNCTION(BlueprintCallable, Category="AR|Stats")
	int32 ClearModifiers(EARModifierSourceCategory Category = EARModifierSourceCategory::All);

	/** Count is in stack groups, not individual stat changes. Invalid/insufficient full requests remove nothing. */
	UFUNCTION(BlueprintCallable, Category="AR|Stats", meta=(AdvancedDisplay="RemovedHandles,Policy,bRequireFullCount", CPP_Default_Count="1"))
	bool RemoveModifierStacks(EARModifierSourceCategory Category, FName SourceId, int32 Count, int32& RemovedCount, TArray<FARStatModifierHandle>& RemovedHandles, EARModifierStackRemovalPolicy Policy = EARModifierStackRemovalPolicy::Oldest, bool bRequireFullCount = true);

	UFUNCTION(BlueprintCallable, Category="AR|Stats", meta=(BlueprintInternalUseOnly="true", DeprecatedFunction, DeprecationMessage="Use Remove Modifier Stacks with Count=1."))
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
	TArray<FARStatEffectView> GetVisibleStatEffects() const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	bool GetModifierRemainingTime(FARStatModifierHandle Handle, bool& bIsPermanent, float& RemainingSeconds) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	float GetDamageRemainingMultiplier(EARStatType ReductionStat) const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	bool HasGuaranteedInvulnerability() const;

	UFUNCTION(BlueprintPure, Category="AR|Stats")
	bool HasGuaranteedEvasion() const;

	void SetBaseStat(EARStatType StatType, float Value);
	/** No effect handle or source record. Negative Money balances and non-finite results are rejected atomically. */
	bool ApplyPermanentFlat(EARStatType StatType, float Delta);

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
	bool IsStatSupported(EARStatType StatType) const;
	void FlushNotifications();
	void RefreshTickState();
	bool CanMutate() const;
	struct FPendingNotification
	{
		bool bSource = false;
		EARStatType StatType = EARStatType::AttackPower;
		float OldValue = 0.0f;
		FARSourceInfo Source;
	};
	TArray<FPendingNotification> PendingNotifications;
	int32 NotificationScopeDepth = 0;
	bool bDispatchingNotifications = false;
	bool bEndingPlay = false;
	bool bReportedNotificationLoop = false;

	UPROPERTY(Transient)
	TArray<FARActiveStatModifier> ActiveModifiers;

	UPROPERTY(Transient)
	TMap<EARStatType, float> CachedFinalStats;
};
