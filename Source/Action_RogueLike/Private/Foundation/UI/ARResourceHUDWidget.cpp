#include "Foundation/UI/ARResourceHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARUIManagerComponent.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"

void UARResourceHUDWidget::BuildDefaultLayout(UWidgetTree* Tree)
{
	if (!Tree || Tree->RootWidget) return;
	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
	Tree->RootWidget = Canvas;
	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResourcePanel"));
	Panel->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.92f));
	Panel->SetPadding(FMargin(16.0f));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetPosition(FVector2D(24.0f, 24.0f));
	PanelSlot->SetSize(FVector2D(320.0f, 178.0f));
	UVerticalBox* Rows = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResourceRows"));
	Panel->SetContent(Rows);

	auto AddRow = [Tree, Rows](const TCHAR* BarName, const TCHAR* ValueName, const TCHAR* Label, FLinearColor Color)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetHeightOverride(38.0f);
		UVerticalBoxSlot* RowSlot = Rows->AddChildToVerticalBox(Size);
		RowSlot->SetPadding(FMargin(0, 0, 0, 8));
		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>();
		Size->SetContent(Overlay);
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(BarName));
		Bar->SetFillColorAndOpacity(Color);
		Bar->SetPercent(0.0f);
		UOverlaySlot* BarSlot = Overlay->AddChildToOverlay(Bar);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
		UTextBlock* Name = Tree->ConstructWidget<UTextBlock>();
		Name->SetText(FText::FromString(Label));
		Name->SetFontSize(14);
		Name->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		UOverlaySlot* NameSlot = Overlay->AddChildToOverlay(Name);
		NameSlot->SetPadding(FMargin(10, 0));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(ValueName));
		Value->SetText(FText::FromString(TEXT("0 / 0")));
		Value->SetFontSize(14);
		Value->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		UOverlaySlot* ValueSlot = Overlay->AddChildToOverlay(Value);
		ValueSlot->SetHorizontalAlignment(HAlign_Right);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		ValueSlot->SetPadding(FMargin(0, 0, 10, 0));
	};
	AddRow(TEXT("HealthBar"), TEXT("HealthValue"), TEXT("HP"), FLinearColor(0.8f, 0.08f, 0.12f));
	AddRow(TEXT("StaminaBar"), TEXT("StaminaValue"), TEXT("STAMINA"), FLinearColor(0.1f, 0.65f, 0.25f));
	AddRow(TEXT("ManaBar"), TEXT("ManaValue"), TEXT("MANA"), FLinearColor(0.1f, 0.35f, 0.9f));
}

TSharedRef<SWidget> UARResourceHUDWidget::RebuildWidget()
{
	BuildDefaultLayout(WidgetTree);
	HealthBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("HealthBar")));
	ShieldBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("ShieldBar")));
	// Keep existing Designer assets intact. Insert the shield layer before Slate builds the overlay.
	if (HealthBar && !ShieldBar)
	{
		if (UOverlay* HealthOverlay = Cast<UOverlay>(HealthBar->GetParent()))
		{
			ShieldBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ShieldBar"));
			ShieldBar->SetWidgetStyle(HealthBar->GetWidgetStyle());
			ShieldBar->SetFillColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f));
			ShieldBar->SetBarFillType(HealthBar->GetBarFillType());
			ShieldBar->SetBarFillStyle(HealthBar->GetBarFillStyle());
			ShieldBar->SetBorderPadding(HealthBar->GetBorderPadding());
			ShieldBar->SetPercent(0.0f);
			ShieldBar->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* ShieldSlot = Cast<UOverlaySlot>(HealthOverlay->InsertChildAt(
				HealthOverlay->GetChildIndex(HealthBar), ShieldBar));
			ShieldSlot->SetHorizontalAlignment(HAlign_Fill);
			ShieldSlot->SetVerticalAlignment(VAlign_Fill);
			if (const UOverlaySlot* HealthSlot = Cast<UOverlaySlot>(HealthBar->Slot))
			{
				ShieldSlot->SetPadding(HealthSlot->GetPadding());
			}
		}
	}
	if (HealthBar && ShieldBar)
	{
		// The red fill must not hide the gray layer with an opaque background.
		FProgressBarStyle HealthStyle = HealthBar->GetWidgetStyle();
		HealthStyle.BackgroundImage.TintColor = FSlateColor(FLinearColor::Transparent);
		HealthBar->SetWidgetStyle(HealthStyle);
	}
	StaminaBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("StaminaBar")));
	ManaBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("ManaBar")));
	HealthValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HealthValue")));
	StaminaValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StaminaValue")));
	ManaValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ManaValue")));
	StatEffectIcons = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("StatEffectIcons")));
	StatEffectPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("StatEffectPanel")));
	if (!StatEffectIcons && StatEffectPanel)
	{
		StatEffectIcons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatEffectIcons"));
		StatEffectPanel->SetContent(StatEffectIcons);
	}
	if (!StatEffectIcons)
	{
		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			StatEffectPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StatEffectPanel"));
			StatEffectPanel->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.92f));
			StatEffectPanel->SetPadding(FMargin(8.0f));
			StatEffectIcons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatEffectIcons"));
			StatEffectPanel->SetContent(StatEffectIcons);
			UCanvasPanelSlot* EffectSlot = Canvas->AddChildToCanvas(StatEffectPanel);
			EffectSlot->SetPosition(FVector2D(24.0f, 220.0f));
			EffectSlot->SetAutoSize(true);
		}
	}
	TimedLabels.Reset();
	CachedEffectKeys.Reset();
	return Super::RebuildWidget();
}

void UARResourceHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ObservePawn(GetOwningPlayerPawn());
}

void UARResourceHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	for (FTimedLabel& Label : TimedLabels)
	{
		if (UTextBlock* Text = Label.Text.Get())
		{
			const int32 Seconds = FMath::CeilToInt(FMath::Max(0.0, Label.EndsAt - Now));
			if (Label.LastShown != Seconds)
			{
				Text->SetText(FText::AsNumber(Seconds));
				Label.LastShown = Seconds;
			}
		}
	}
}

void UARResourceHUDWidget::Disconnect()
{
	if (ObservedUI) ObservedUI->OnHUDSnapshotChangedNative.RemoveAll(this);
	ObservedUI = nullptr;
}

void UARResourceHUDWidget::ObservePawn(APawn* Pawn)
{
	Disconnect();
	AARPlayerCharacter* Player = Cast<AARPlayerCharacter>(Pawn);
	ObservedUI = Player ? Player->GetUIManagerComponent() : nullptr;
	if (ObservedUI)
	{
		ObservedUI->OnHUDSnapshotChangedNative.AddUObject(this, &UARResourceHUDWidget::ApplySnapshot);
		ApplySnapshot(ObservedUI->GetHUDSnapshot());
	}
	else ApplySnapshot(FARPlayerHUDSnapshot());
	SetVisibility(Player ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UARResourceHUDWidget::ApplySnapshot(const FARPlayerHUDSnapshot& Snapshot)
{
	auto Update = [](UProgressBar* Bar, UTextBlock* Value, const FARHUDResourceValue& Resource)
	{
		if (Bar) Bar->SetPercent(FMath::Clamp(Resource.Ratio, 0.0f, 1.0f));
		if (Value)
		{
			FNumberFormattingOptions Format;
			Format.MaximumFractionalDigits = 0;
			Value->SetText(FText::Format(NSLOCTEXT("ARHUD", "ResourceValue", "{0} / {1}"),
				FText::AsNumber(Resource.Current, &Format), FText::AsNumber(Resource.Maximum, &Format)));
		}
	};
	Update(HealthBar, HealthValue, Snapshot.Health);
	const float CurrentHealth = FMath::Max(0.0f, Snapshot.Health.Current);
	const float CurrentShield = FMath::Max(0.0f, Snapshot.Shield);
	if (ShieldBar)
	{
		// Show shield immediately after HP. Expand the visual scale only if their sum exceeds max HP.
		const float DisplayMaximum = FMath::Max(Snapshot.Health.Maximum, CurrentHealth + CurrentShield);
		ShieldBar->SetPercent(DisplayMaximum > 0.0f
			? FMath::Clamp((CurrentHealth + CurrentShield) / DisplayMaximum, 0.0f, 1.0f) : 0.0f);
		if (HealthBar) HealthBar->SetPercent(DisplayMaximum > 0.0f
			? FMath::Clamp(CurrentHealth / DisplayMaximum, 0.0f, 1.0f) : 0.0f);
	}
	if (HealthValue && CurrentShield > 0.0f)
	{
		FNumberFormattingOptions Format;
		Format.MaximumFractionalDigits = 0;
		HealthValue->SetText(FText::Format(NSLOCTEXT("ARHUD", "HealthWithShield", "{0} / {1}  + {2}"),
			FText::AsNumber(CurrentHealth, &Format), FText::AsNumber(Snapshot.Health.Maximum, &Format),
			FText::AsNumber(CurrentShield, &Format)));
	}
	if (HealthValue) HealthValue->SetToolTipText(FText::Format(
		NSLOCTEXT("ARHUD", "HealthShieldTooltip", "Health: {0} / {1}\nShield: {2}"),
		FText::AsNumber(CurrentHealth), FText::AsNumber(Snapshot.Health.Maximum), FText::AsNumber(CurrentShield)));
	Update(StaminaBar, StaminaValue, Snapshot.Stamina);
	Update(ManaBar, ManaValue, Snapshot.Mana);

	if (!StatEffectIcons) return;
	TArray<FString> EffectKeys;
	TArray<float> TimedRemaining;
	for (const FARStatEffectView& Effect : Snapshot.StatEffects)
	{
		EffectKeys.Add(FString::Printf(TEXT("S:%d:%s:%d:%d:%d:%s:%s"), static_cast<int32>(Effect.Category),
			*Effect.SourceId.ToString(), Effect.StackCount, Effect.bHasPermanent ? 1 : 0, static_cast<int32>(Effect.Display),
			*Effect.DisplayName.ToString(), *Effect.Icon.ToSoftObjectPath().ToString()));
		if (!Effect.bHasPermanent) TimedRemaining.Add(Effect.LongestRemainingTime);
	}
	for (const FARStatusEffectView& Effect : Snapshot.StatusEffects)
	{
		if (!Effect.Definition) continue;
		EffectKeys.Add(FString::Printf(TEXT("C:%s"), *Effect.Handle.Id.ToString(EGuidFormats::Digits)));
		TimedRemaining.Add(Effect.RemainingTime);
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (EffectKeys == CachedEffectKeys && TimedRemaining.Num() == TimedLabels.Num())
	{
		for (int32 Index = 0; Index < TimedLabels.Num(); ++Index)
		{
			TimedLabels[Index].EndsAt = Now + TimedRemaining[Index];
		}
		return;
	}
	CachedEffectKeys = MoveTemp(EffectKeys);
	StatEffectIcons->ClearChildren();
	TimedLabels.Reset();
	auto AddEffect = [this, Now](const FText& Name, TSoftObjectPtr<UTexture2D> Icon, int32 StackCount, float Remaining, bool bPermanent, FLinearColor Tint)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* ColumnSlot = StatEffectIcons->AddChildToHorizontalBox(Column);
		ColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(Tint);
		Frame->SetPadding(FMargin(2.0f));
		Frame->SetToolTipText(Name);
		Column->AddChildToVerticalBox(Frame);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(36.0f);
		Size->SetHeightOverride(36.0f);
		Frame->SetContent(Size);
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
		Size->SetContent(Overlay);
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		if (UTexture2D* Texture = Icon.LoadSynchronous()) Image->SetBrushFromTexture(Texture);
		Overlay->AddChildToOverlay(Image);
		if (StackCount > 1)
		{
			UTextBlock* StackText = WidgetTree->ConstructWidget<UTextBlock>();
			StackText->SetText(FText::AsNumber(StackCount));
			StackText->SetFontSize(14);
			StackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			UOverlaySlot* StackSlot = Overlay->AddChildToOverlay(StackText);
			StackSlot->SetHorizontalAlignment(HAlign_Right);
			StackSlot->SetVerticalAlignment(VAlign_Bottom);
		}
		if (!bPermanent)
		{
			UTextBlock* TimeText = WidgetTree->ConstructWidget<UTextBlock>();
			TimeText->SetFontSize(11);
			TimeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Column->AddChildToVerticalBox(TimeText);
			FTimedLabel& Label = TimedLabels.AddDefaulted_GetRef();
			Label.Text = TimeText;
			Label.EndsAt = Now + Remaining;
			Label.LastShown = FMath::CeilToInt(Remaining);
			TimeText->SetText(FText::AsNumber(Label.LastShown));
		}
	};
	for (const FARStatEffectView& Effect : Snapshot.StatEffects)
	{
		AddEffect(Effect.DisplayName, Effect.Icon, Effect.StackCount, Effect.LongestRemainingTime,
			Effect.bHasPermanent, Effect.Display == EARStatEffectDisplay::Debuff
				? FLinearColor(0.55f, 0.08f, 0.1f) : FLinearColor(0.05f, 0.45f, 0.2f));
	}
	for (const FARStatusEffectView& Effect : Snapshot.StatusEffects)
	{
		if (Effect.Definition)
		{
			AddEffect(Effect.Definition->DisplayName, Effect.Definition->Icon, 1, Effect.RemainingTime,
				false, FLinearColor(0.55f, 0.08f, 0.1f));
		}
	}
	if (StatEffectPanel) StatEffectPanel->SetVisibility(StatEffectIcons->GetChildrenCount() == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	else StatEffectIcons->SetVisibility(StatEffectIcons->GetChildrenCount() == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UARResourceHUDWidget::NativeDestruct()
{
	Disconnect();
	Super::NativeDestruct();
}
