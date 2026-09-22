#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "Foundation/UI/ARUITypes.h"
#include "ARUIManagerComponent.generated.h"

class AARPlayerCharacter;
struct FARCombatDamageResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARUIScreenOpenRequestedSignature, EARUIScreen, Screen, UObject*, ContextObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARUIScreenEventSignature, EARUIScreen, Screen);

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

	UPROPERTY(BlueprintAssignable, Category="AR|UI") FARUIScreenOpenRequestedSignature OnScreenOpenRequested;
	UPROPERTY(BlueprintAssignable, Category="AR|UI") FARUIScreenEventSignature OnScreenCloseRequested;
	UPROPERTY(BlueprintAssignable, Category="AR|UI") FARUIScreenEventSignature OnScreenBackRequested;

private:
	void ApplyInputMode(bool bUIOpen);

	UFUNCTION() void HandleOwnerDeath(AActor* Target, const FARCombatDamageResult& KillingDamage);

	UPROPERTY(Transient) TObjectPtr<AARPlayerCharacter> PlayerOwner;
	UPROPERTY(Transient) TObjectPtr<UObject> CurrentContext;
	UPROPERTY(Transient) EARUIScreen CurrentScreen = EARUIScreen::None;
};
