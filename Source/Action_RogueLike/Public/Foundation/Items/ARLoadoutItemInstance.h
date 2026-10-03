#pragma once

#include "CoreMinimal.h"
#include "Foundation/Items/ARItemInstance.h"
#include "Foundation/Items/ARItemTypes.h"
#include "Foundation/Combat/ARStaggerTypes.h"
#include "Foundation/Status/ARStatusEffectTypes.h"
#include "TimerManager.h"
#include "ARLoadoutItemInstance.generated.h"

class AARPlayerCharacter;
class UARItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FARItemUIStateChangedSignature, FGuid, ItemInstanceId, const FARItemUIState&, State, bool, bRemoved);

/** Concrete base is valid for data-only weapons/relics that only use Definition stat modifiers. */
UCLASS(Blueprintable, BlueprintType)
class ACTION_ROGUELIKE_API UARLoadoutItemInstance : public UARItemInstance
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	void InitializeInstance(AARPlayerCharacter* InOwner, const UARItemDefinition* InDefinition);
	bool RegisterItem();
	void UnregisterItem(EARItemRemovalReason Reason);

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item", meta=(DisplayName="On Item Registered"))
	void ReceiveItemRegistered();

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item", meta=(DisplayName="On Item Unregistered"))
	void ReceiveItemUnregistered(EARItemRemovalReason Reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="AR|Item")
	bool CanExecuteItemSkill(FName SkillId, FGameplayTag& FailureTag) const;

	/** Content Blueprint implements the actual skill after the native input/cost transaction. */
	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item")
	void ExecuteItemSkill(FName SkillId, FARActionHandle ActionHandle);

	/** Only this instance's executed skills are notified, after native action cleanup. Not a normal-end event. */
	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item|Skill", meta=(DisplayName="On Item Skill Cancelled"))
	void ReceiveItemSkillCancelled(FName SkillId, FARActionHandle ActionHandle, EARActionCancelReason Reason);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	FARStatModifierHandle ApplyItemStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	bool RemoveOwnItemModifier(FARStatModifierHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	int32 RemoveAllOwnItemModifiers();

	/** Clears on item unregistration (including drop, replacement and owner EndPlay).
	 * Not action-owned: a skill cancellation alone does not stop this timer.
	 * Time>0. Initial Start Delay=-1 uses Time; bLooping=false performs one delayed call.
	 * Each call creates an independent timer. Existing engine handle nodes can pause/unpause/clear it. */
	UFUNCTION(BlueprintCallable, Category="AR|Item|Timer", meta=(DisplayName="Set Item Timer by Event", AdvancedDisplay="InitialStartDelay,bMaxOncePerFrame"))
	FTimerHandle SetItemTimerByEvent(UPARAM(DisplayName="Event") FTimerDynamicDelegate Event, float Time,
		bool bLooping, bool& bSuccess, float InitialStartDelay = -1.0f, bool bMaxOncePerFrame = true);

	/** Item-owned guarantees are removed automatically when this runtime instance is unregistered. */
	UFUNCTION(BlueprintCallable, Category="AR|Item|Guarantees")
	FARSuperArmorHandle ApplyItemSuperArmor(const FARSuperArmorSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Item|Guarantees")
	bool RemoveOwnItemSuperArmor(FARSuperArmorHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Item|Guarantees")
	FARCCImmunityHandle ApplyItemCCImmunity(const FARCCImmunitySpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Item|Guarantees")
	bool RemoveOwnItemCCImmunity(FARCCImmunityHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	bool SetItemUIState(FGameplayTag StateId, float CurrentValue, float MaximumValue);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	bool RemoveItemUIState(FGameplayTag StateId);

	UFUNCTION(BlueprintPure, Category="AR|Item") AARPlayerCharacter* GetItemOwner() const { return ItemOwner.Get(); }
	UFUNCTION(BlueprintPure, Category="AR|Item") const UARItemDefinition* GetItemDefinition() const { return Definition; }
	UFUNCTION(BlueprintPure, Category="AR|Item") FGuid GetInstanceId() const { return InstanceId; }
	UFUNCTION(BlueprintPure, Category="AR|Item") bool IsRegistered() const { return bRegistered; }
	UFUNCTION(BlueprintPure, Category="AR|Item") TArray<FARItemUIState> GetItemUIStates() const;

	UPROPERTY(BlueprintAssignable, Category="AR|Item") FARItemUIStateChangedSignature OnItemUIStateChanged;

private:
	FARSourceInfo MakeOwnedSource(const FARSourceInfo& RequestedSource) const;

	UPROPERTY(Transient) TWeakObjectPtr<AARPlayerCharacter> ItemOwner;
	UPROPERTY(Transient) TObjectPtr<const UARItemDefinition> Definition = nullptr;
	UPROPERTY(Transient) FGuid InstanceId;
	UPROPERTY(Transient) TArray<FARStatModifierHandle> OwnModifierHandles;
	UPROPERTY(Transient) TArray<FARSuperArmorHandle> OwnSuperArmorHandles;
	UPROPERTY(Transient) TArray<FARCCImmunityHandle> OwnCCImmunityHandles;
	UPROPERTY(Transient) TMap<FGameplayTag, FARItemUIState> UIStates;
	bool bRegistered = false;
	bool bUnregistering = false;
	TArray<FTimerHandle> OwnTimerHandles;
};
