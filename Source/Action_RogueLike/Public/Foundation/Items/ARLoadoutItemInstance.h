#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Foundation/Items/ARItemTypes.h"
#include "ARLoadoutItemInstance.generated.h"

class AARPlayerCharacter;
class UARLoadoutItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FARItemUIStateChangedSignature, FGuid, ItemInstanceId, const FARItemUIState&, State, bool, bRemoved);

/** Concrete base is valid for data-only weapons/relics that only use Definition stat modifiers. */
UCLASS(Blueprintable, BlueprintType)
class ACTION_ROGUELIKE_API UARLoadoutItemInstance : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	void InitializeInstance(AARPlayerCharacter* InOwner, const UARLoadoutItemDefinition* InDefinition);
	bool RegisterItem();
	void UnregisterItem(EARItemRemovalReason Reason);

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item", meta=(DisplayName="On Item Registered"))
	void ReceiveItemRegistered();

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item", meta=(DisplayName="On Item Unregistered"))
	void ReceiveItemUnregistered(EARItemRemovalReason Reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="AR|Item")
	bool CanExecuteItemSkill(FName SkillId, FGameplayTag& FailureTag) const;

	UFUNCTION(BlueprintImplementableEvent, Category="AR|Item")
	void ExecuteItemSkill(FName SkillId, FARActionHandle ActionHandle);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	FARStatModifierHandle ApplyItemStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	bool RemoveOwnItemModifier(FARStatModifierHandle Handle);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	int32 RemoveAllOwnItemModifiers();

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	bool SetItemUIState(FGameplayTag StateId, float CurrentValue, float MaximumValue);

	UFUNCTION(BlueprintCallable, Category="AR|Item")
	bool RemoveItemUIState(FGameplayTag StateId);

	UFUNCTION(BlueprintPure, Category="AR|Item") AARPlayerCharacter* GetItemOwner() const { return ItemOwner.Get(); }
	UFUNCTION(BlueprintPure, Category="AR|Item") const UARLoadoutItemDefinition* GetItemDefinition() const { return Definition; }
	UFUNCTION(BlueprintPure, Category="AR|Item") FGuid GetInstanceId() const { return InstanceId; }
	UFUNCTION(BlueprintPure, Category="AR|Item") bool IsRegistered() const { return bRegistered; }
	UFUNCTION(BlueprintPure, Category="AR|Item") TArray<FARItemUIState> GetItemUIStates() const;

	UPROPERTY(BlueprintAssignable, Category="AR|Item") FARItemUIStateChangedSignature OnItemUIStateChanged;

private:
	FARSourceInfo MakeOwnedSource(const FARSourceInfo& RequestedSource) const;

	UPROPERTY(Transient) TWeakObjectPtr<AARPlayerCharacter> ItemOwner;
	UPROPERTY(Transient) TObjectPtr<const UARLoadoutItemDefinition> Definition = nullptr;
	UPROPERTY(Transient) FGuid InstanceId;
	UPROPERTY(Transient) TArray<FARStatModifierHandle> OwnModifierHandles;
	UPROPERTY(Transient) TMap<FGameplayTag, FARItemUIState> UIStates;
	bool bRegistered = false;
};
