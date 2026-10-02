#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Foundation/UI/ARUITypes.h"
#include "ARResourceHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UHorizontalBox;
class UBorder;
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> HealthBar;
	/** Gray combined HP+shield fill behind HealthBar; added to legacy health overlays at runtime. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ShieldBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ManaBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> HealthValue;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StaminaValue;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ManaValue;
	/** Optional designer container; a default icon strip is added under resources on CanvasPanel roots. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UHorizontalBox> StatEffectIcons;

private:
	void Disconnect();
	UPROPERTY(Transient) TObjectPtr<UARUIManagerComponent> ObservedUI;
	UPROPERTY(Transient) TObjectPtr<UBorder> StatEffectPanel;
	struct FTimedLabel
	{
		TWeakObjectPtr<UTextBlock> Text;
		double EndsAt = 0.0;
		int32 LastShown = -1;
	};
	TArray<FTimedLabel> TimedLabels;
	TArray<FString> CachedEffectKeys;
};
