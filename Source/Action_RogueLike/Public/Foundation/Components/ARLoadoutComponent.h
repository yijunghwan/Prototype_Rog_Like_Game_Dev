#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Items/ARItemTypes.h"
#include "ARLoadoutComponent.generated.h"

class AARPlayerCharacter;
class UARActionComponent;
class UARManaComponent;
class UARStaminaComponent;
class UARStatsComponent;
class UARLoadoutItemDefinition;
class UARLoadoutItemInstance;
class UARWeaponDefinition;
class AARLoadoutItemPickup;

USTRUCT()
struct FARRegisteredSkillRecord
{
	GENERATED_BODY()

	UPROPERTY() FARRegisteredSkillHandle Handle;
	UPROPERTY() TObjectPtr<UARLoadoutItemInstance> Instance = nullptr;
	UPROPERTY() FARSkillDefinition Definition;
	UPROPERTY() double CooldownEndsAt = 0.0;
	UPROPERTY() float CooldownTotal = 0.0f;
};

struct FARPendingAcquisition
{
	TWeakObjectPtr<const UARLoadoutItemDefinition> Definition;
	int32 Revision = 0;
};

struct FARPendingEvolution
{
	int32 Revision = 0;
	FGuid WeaponInstanceId;
	TArray<FSoftObjectPath> CandidatePaths;
};

struct FARActiveSkillGroupRecord
{
	FGameplayTag InputTag;
	TArray<FARActionHandle> ActionHandles;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARLoadoutChangedSignature, int32, Revision);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FARRegisteredSkillsChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FARLoadoutItemUIStateChangedSignature, FGuid, ItemInstanceId, const FARItemUIState&, State, bool, bRemoved);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARLoadoutComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	FARLoadoutAcquisitionResult BeginLoadoutAcquisition(const UARLoadoutItemDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	UARLoadoutItemInstance* CommitLoadoutAcquisition(FARAcquisitionToken Token, FARRequestStatus& Status);

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	bool CancelLoadoutAcquisition(FARAcquisitionToken Token);

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	bool CancelWeaponEvolution(FAREvolutionToken Token);

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	void CancelAllPendingRequests();

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	bool DiscardLoadoutItem(FGuid InstanceId, FARLoadoutDropRequest& DropRequest, FARRequestStatus& Status);

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	FARRequestStatus HandleSkillInput(FGameplayTag InputTag, FARSkillGroupHandle& GroupHandle);

	UFUNCTION(BlueprintPure, Category="AR|Loadout")
	TArray<FARRegisteredSkillUIData> GetRegisteredSkillUIData() const;

	UFUNCTION(BlueprintPure, Category="AR|Loadout")
	bool GetSkillCooldownState(FARRegisteredSkillHandle Handle, bool& bReady, float& Remaining, float& Total, float& Ratio) const;

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	bool ResetSkillCooldown(FARRegisteredSkillHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	bool ModifySkillCooldown(FARRegisteredSkillHandle Handle, float DeltaSeconds, float& NewRemaining);

	UFUNCTION(BlueprintPure, Category="AR|Loadout")
	TArray<FARLoadoutItemSnapshot> GetLoadoutInventory() const;

	UFUNCTION(BlueprintPure, Category="AR|Loadout|UI")
	bool GetLoadoutItemDisplayData(FGuid InstanceId, FARLoadoutItemDisplayData& DisplayData) const;

	UFUNCTION(BlueprintPure, Category="AR|Loadout")
	UARLoadoutItemInstance* GetEquippedWeapon() const { return EquippedWeapon; }

	UFUNCTION(BlueprintPure, Category="AR|Loadout")
	const TArray<UARLoadoutItemInstance*>& GetActiveRelics() const { return ActiveRelics; }

	UFUNCTION(BlueprintPure, Category="AR|Loadout")
	const TArray<UARLoadoutItemInstance*>& GetPassiveRelics() const { return PassiveRelics; }

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	FARWeaponEvolutionResult RequestWeaponEvolution();

	UFUNCTION(BlueprintCallable, Category="AR|Loadout")
	UARLoadoutItemInstance* CommitWeaponEvolution(FAREvolutionToken Token, const UARWeaponDefinition* Candidate, FARRequestStatus& Status);

	UPROPERTY(BlueprintAssignable, Category="AR|Loadout") FARLoadoutChangedSignature OnLoadoutChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Loadout") FARRegisteredSkillsChangedSignature OnRegisteredSkillsChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Loadout") FARLoadoutItemUIStateChangedSignature OnItemUIStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Loadout", meta=(ClampMin="1")) int32 MaxActiveRelics = 2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Loadout|Drop") TSubclassOf<AARLoadoutItemPickup> DroppedItemPickupClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Loadout|Drop", meta=(ClampMin="0.0")) float DropForwardDistance = 96.0f;

private:
	bool ValidateDefinition(const UARLoadoutItemDefinition* Definition, FARRequestStatus& Status) const;
	UARLoadoutItemInstance* CreateAndRegisterInstance(const UARLoadoutItemDefinition* Definition, FARRequestStatus& Status);
	void UnregisterAndReleaseInstance(UARLoadoutItemInstance* Instance, EARItemRemovalReason Reason);
	void RegisterSkills(UARLoadoutItemInstance* Instance);
	void UnregisterSkills(UARLoadoutItemInstance* Instance);
	FARRegisteredSkillRecord* FindSkill(FARRegisteredSkillHandle Handle);
	const FARRegisteredSkillRecord* FindSkill(FARRegisteredSkillHandle Handle) const;
	const UARLoadoutItemInstance* FindItemInstance(FGuid InstanceId) const;
	FARLoadoutItemSnapshot MakeSnapshot(const UARLoadoutItemInstance* Instance) const;
	void NotifyLoadoutChanged();
	double GetNow() const;

	UFUNCTION() void HandleItemUIStateChanged(FGuid ItemInstanceId, const FARItemUIState& State, bool bRemoved);
	UFUNCTION() void HandleActionEnded(FARActionHandle Handle);
	UFUNCTION() void HandleActionCancelled(FARActionHandle Handle, EARActionCancelReason Reason);
	void RemoveActionFromSkillGroups(FARActionHandle Handle);

	UPROPERTY(Transient) TObjectPtr<AARPlayerCharacter> PlayerOwner;
	UPROPERTY(Transient) TObjectPtr<UARStatsComponent> StatsComponent;
	UPROPERTY(Transient) TObjectPtr<UARActionComponent> ActionComponent;
	UPROPERTY(Transient) TObjectPtr<UARManaComponent> ManaComponent;
	UPROPERTY(Transient) TObjectPtr<UARStaminaComponent> StaminaComponent;
	UPROPERTY(Transient) TObjectPtr<UARLoadoutItemInstance> EquippedWeapon;
	UPROPERTY(Transient) TArray<TObjectPtr<UARLoadoutItemInstance>> ActiveRelics;
	UPROPERTY(Transient) TArray<TObjectPtr<UARLoadoutItemInstance>> PassiveRelics;
	UPROPERTY(Transient) TArray<FARRegisteredSkillRecord> RegisteredSkills;
	TMap<FGuid, FARPendingAcquisition> PendingAcquisitions;
	TMap<FGuid, FARPendingEvolution> PendingEvolutions;
	TMap<FGuid, FARActiveSkillGroupRecord> ActiveSkillGroups;
	int32 LoadoutRevision = 0;
};
