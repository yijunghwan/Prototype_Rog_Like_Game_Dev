#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ARFoundationTypes.generated.h"

UENUM(BlueprintType)
enum class EARCombatTeam : uint8
{
	Player,
	Enemy,
	Environment
};

UENUM(BlueprintType)
enum class EARDamageDelivery : uint8
{
	Direct,
	DamageOverTime
};

UENUM(BlueprintType)
enum class EARDamageAttribute : uint8
{
	Physical,
	Fire,
	Magic,
	Void
};

UENUM(BlueprintType)
enum class EARModifierSourceCategory : uint8
{
	Weapon,
	Relic,
	Buff,
	Other,
	All
};

UENUM(BlueprintType)
enum class EARStatModifierOperation : uint8
{
	Flat,
	AdditivePercent,
	Multiplicative,
	IndependentDamageReduction
};

UENUM(BlueprintType)
enum class EARStatType : uint8
{
	MaxHealth,
	RecoveryPower,
	PhysicalDefense,
	FireDefense,
	MagicDefense,
	OverallDamageReduction,
	PhysicalDamageReduction,
	FireDamageReduction,
	MagicDamageReduction,
	Evasion,
	AttackPower,
	SpellPower,
	StaggerPower,
	MoveSpeed,
	AttackSpeed,
	Range,
	CriticalChance,
	CriticalDamage,
	DefensePenetration,
	Absorption,
	OverallDamageAmplification,
	DirectDamageAmplification,
	DamageOverTimeAmplification,
	PhysicalDamageAmplification,
	FireDamageAmplification,
	MagicDamageAmplification,
	GroggyDamageAmplification,
	PhysicalDamageTakenIncrease,
	FireDamageTakenIncrease,
	MagicDamageTakenIncrease,
	Tenacity,
	StaggerResistance,
	MaxGroggy,
	GroggyRecoveryDelay,
	GroggyRecoveryPerSecond,
	Luck,
	MaxStamina,
	StaminaRecoveryPerSecond,
	StaminaRecoveryDelay,
	RollStaminaCost,
	RollDistance,
	RollEvasionDuration,
	MaxMana,
	ManaRecoveryPerSecond,
	MaxConsumableSlots,
	CooldownReduction,
	Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EARResourceType : uint8
{
	Health,
	Shield,
	Stamina,
	Mana,
	Groggy,
	/** Used by affordability nodes when no resource is missing. */
	None
};

UENUM(BlueprintType)
enum class EARResourceChangeReason : uint8
{
	Damage,
	Restore,
	Consume,
	Regeneration,
	MaxChanged,
	Other
};

UENUM(BlueprintType)
enum class EARActionCancelReason : uint8
{
	None,
	Stagger,
	Stun,
	Root,
	Roll,
	BasicMovementInput,
	ItemRemoved,
	Death,
	Manual
};

UENUM(BlueprintType)
enum class EARMovementLockType : uint8
{
	BasicMovementOnly,
	AllMovement
};

UENUM(BlueprintType)
enum class EARRequestResult : uint8
{
	Success,
	InvalidOwner,
	InvalidSource,
	InvalidTarget,
	ForbiddenTarget,
	SameTeam,
	Dead,
	Blocked,
	Cooldown,
	NotEnoughResource,
	InvalidHandle,
	InvalidDefinition,
	SlotFull,
	StaleRequest,
	Rejected
};

UENUM(BlueprintType)
enum class EARDotStackPolicy : uint8
{
	Independent,
	RefreshSameName
};

UENUM(BlueprintType)
enum class EARModifierStackRemovalPolicy : uint8
{
	Newest,
	Oldest
};

UENUM(BlueprintType)
enum class EARShieldLifetimeFilter : uint8
{
	All,
	TimedOnly,
	PermanentOnly
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARSourceInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source")
	EARModifierSourceCategory Category = EARModifierSourceCategory::Other;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source")
	FName SourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source")
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatModifierHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARStatModifierHandle& A, const FARStatModifierHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARStatModifierHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARShieldHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARShieldHandle& A, const FARShieldHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARShieldHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatusEffectHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARStatusEffectHandle& A, const FARStatusEffectHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARStatusEffectHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARSuperArmorHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARSuperArmorHandle& A, const FARSuperArmorHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARSuperArmorHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARMovementLockHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARMovementLockHandle& A, const FARMovementLockHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARMovementLockHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARActionHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") TWeakObjectPtr<UObject> Owner;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") int64 Serial = 0;
	bool IsValid() const { return Id.IsValid() && Owner.IsValid() && Serial > 0; }
	friend bool operator==(const FARActionHandle& A, const FARActionHandle& B) { return A.Id == B.Id && A.Owner == B.Owner && A.Serial == B.Serial; }
	friend uint32 GetTypeHash(const FARActionHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARSkillGroupHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARSkillGroupHandle& A, const FARSkillGroupHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARSkillGroupHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARRegisteredSkillHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARRegisteredSkillHandle& A, const FARRegisteredSkillHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARRegisteredSkillHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARDotHandle
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARDotHandle& A, const FARDotHandle& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARDotHandle& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARAcquisitionToken
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FARAcquisitionToken& A, const FARAcquisitionToken& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FARAcquisitionToken& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FAREvolutionToken
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Handle") FGuid Id;
	bool IsValid() const { return Id.IsValid(); }
	friend bool operator==(const FAREvolutionToken& A, const FAREvolutionToken& B) { return A.Id == B.Id; }
	friend uint32 GetTypeHash(const FAREvolutionToken& Handle) { return GetTypeHash(Handle.Id); }
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARResourceCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ClampMin="0"))
	float Mana = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource", meta=(ClampMin="0"))
	float Stamina = 0.0f;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARRequestStatus
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Request")
	EARRequestResult Result = EARRequestResult::Rejected;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Request")
	FText Message;

	bool IsSuccess() const { return Result == EARRequestResult::Success; }
};
