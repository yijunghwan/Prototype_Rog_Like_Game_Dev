#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "Foundation/UI/ARUITypes.h"
#include "ARUIManagerComponent.generated.h"

class AARPlayerCharacter;
class AARBaseCharacter;
struct FARCombatDamageResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARUIScreenOpenRequestedSignature, EARUIScreen, Screen, UObject*, ContextObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARUIScreenEventSignature, EARUIScreen, Screen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARHUDSnapshotChangedSignature, const FARPlayerHUDSnapshot&, Snapshot);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARUIManagerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="AR|UI")
	FARRequestStatus OpenScreen(EARUIScreen Screen, UObject* ContextObject = nullptr);

	UFUNCTION(BlueprintCallable, Category="AR|UI")
	bool CloseCurrentScreen();

	UFUNCTION(BlueprintCallable, Category="AR|UI")
	void RequestBack();

	UFUNCTION(BlueprintPure, Category="AR|UI") bool IsScreenOpen() const { return CurrentScreen != EARUIScreen::None; }
	UFUNCTION(BlueprintPure, Category="AR|UI") EARUIScreen GetCurrentScreen() const { return CurrentScreen; }
	UFUNCTION(BlueprintPure, Category="AR|UI") UObject* GetCurrentContext() const { return CurrentContext; }
	UFUNCTION(BlueprintPure, Category="AR|UI|HUD") FARPlayerHUDSnapshot GetHUDSnapshot() const;

	UPROPERTY(BlueprintAssignable, Category="AR|UI") FARUIScreenOpenRequestedSignature OnScreenOpenRequested;
	UPROPERTY(BlueprintAssignable, Category="AR|UI") FARUIScreenEventSignature OnScreenCloseRequested;
	UPROPERTY(BlueprintAssignable, Category="AR|UI") FARUIScreenEventSignature OnScreenBackRequested;
	UPROPERTY(BlueprintAssignable, Category="AR|UI|HUD") FARHUDSnapshotChangedSignature OnHUDSnapshotChanged;

private:
	void ApplyInputMode(bool bUIOpen);
	void BroadcastHUDSnapshot();

	UFUNCTION() void HandleOwnerDeath(AActor* Target, const FARCombatDamageResult& KillingDamage);
	UFUNCTION() void HandleHealthChanged(AActor* Target, float Current, float Maximum, float Delta, EARResourceChangeReason Reason);
	UFUNCTION() void HandleShieldChanged(AActor* Target, float Current, float Delta, FARShieldHandle Handle, EARResourceChangeReason Reason);
	UFUNCTION() void HandleManaChanged(AActor* Target, float Current, float Maximum, float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source);
	UFUNCTION() void HandleStaminaChanged(AActor* Target, float Current, float Maximum, float Delta, EARResourceChangeReason Reason, const FARSourceInfo& Source);
	UFUNCTION() void HandleLoadoutChanged(int32 Revision);
	UFUNCTION() void HandleRegisteredSkillsChanged();
	UFUNCTION() void HandleItemUIStateChanged(FGuid ItemInstanceId, const FARItemUIState& State, bool bRemoved);
	UFUNCTION() void HandleConsumableSlotsChanged(const TArray<FARConsumableSlotSnapshot>& Slots);
	UFUNCTION() void HandleAimDirectionChanged(AARBaseCharacter* Character, FVector AimDirection);

	UPROPERTY(Transient) TObjectPtr<AARPlayerCharacter> PlayerOwner;
	UPROPERTY(Transient) TObjectPtr<UObject> CurrentContext;
	UPROPERTY(Transient) EARUIScreen CurrentScreen = EARUIScreen::None;
};
