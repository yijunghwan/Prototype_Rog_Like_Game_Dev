#include "Foundation/UI/ARCreateResourceHUDCommandlet.h"

#if WITH_EDITOR
#include "Blueprint/WidgetTree.h"
#include "Foundation/UI/ARResourceHUDWidget.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#endif

UARCreateResourceHUDCommandlet::UARCreateResourceHUDCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UARCreateResourceHUDCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	const FString PackageName = TEXT("/Game/Game/Tests/UI/WBP_TestResourceHUD");
	if (FPackageName::DoesPackageExist(PackageName))
	{
		UE_LOG(LogTemp, Display, TEXT("Resource HUD asset already exists. Keeping Designer edits."));
		return 0;
	}
	UPackage* Package = CreatePackage(*PackageName);
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		UARResourceHUDWidget::StaticClass(), Package, TEXT("WBP_TestResourceHUD"), BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	if (!Blueprint || !Blueprint->WidgetTree) return 1;
	UARResourceHUDWidget::BuildDefaultLayout(Blueprint->WidgetTree);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass) return 1;
	Package->MarkPackageDirty();
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const bool bSaved = UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
	UE_LOG(LogTemp, Display, TEXT("Resource HUD asset saved: %s (%s)"), *Filename, bSaved ? TEXT("success") : TEXT("failed"));
	return bSaved ? 0 : 1;
#else
	return 1;
#endif
}
