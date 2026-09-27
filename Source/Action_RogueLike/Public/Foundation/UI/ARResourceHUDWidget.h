#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Foundation/UI/ARUITypes.h"
#include "ARResourceHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UARUIManagerComponent;
class APawn;

/** Presentation only. Designer children keep these widget names to retain the data connection. */
UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API UARResourceHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ObservePawn(APawn* Pawn);
	void ApplySnapshot(const FARPlayerHUDSnapshot& Snapshot);
	static void BuildDefaultLayout(UWidgetTree* Tree);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ManaBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> HealthValue;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StaminaValue;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ManaValue;

private:
	void Disconnect();
	UPROPERTY(Transient) TObjectPtr<UARUIManagerComponent> ObservedUI;
};
