#include "Foundation/UI/ARResourceHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARUIManagerComponent.h"

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
	StaminaBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("StaminaBar")));
	ManaBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("ManaBar")));
	HealthValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HealthValue")));
	StaminaValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("StaminaValue")));
	ManaValue = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ManaValue")));
	return Super::RebuildWidget();
}

void UARResourceHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ObservePawn(GetOwningPlayerPawn());
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
	Update(StaminaBar, StaminaValue, Snapshot.Stamina);
	Update(ManaBar, ManaValue, Snapshot.Mana);
}

void UARResourceHUDWidget::NativeDestruct()
{
	Disconnect();
	Super::NativeDestruct();
}
