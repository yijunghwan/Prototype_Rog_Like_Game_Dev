#include "Foundation/Blueprint/ARItemCatalogBlueprintLibrary.h"

#include "Engine/AssetManager.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARConsumableComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "Foundation/Items/ARConsumableInstance.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"

namespace
{
	TArray<UARItemDefinition*> GetCatalogDefinitions()
	{
		TArray<UARItemDefinition*> Result;
		UAssetManager& AssetManager = UAssetManager::Get();
		TArray<FPrimaryAssetId> AssetIds;
		AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("ARItem")), AssetIds);
		for (const FPrimaryAssetId& AssetId : AssetIds)
		{
			const FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(AssetId);
			if (UARItemDefinition* Definition = Cast<UARItemDefinition>(Path.TryLoad()))
			{
				Result.Add(Definition);
			}
		}
		return Result;
	}

	bool MatchesType(const UARItemDefinition* Definition, FGameplayTag TypeFilter)
	{
		return Definition && (!TypeFilter.IsValid() || Definition->ItemTypeTag == TypeFilter);
	}
}

UARItemDefinition* UARItemCatalogBlueprintLibrary::FindItemByKey(FGameplayTag ItemTypeTag, int32 ItemId)
{
	if (!ItemTypeTag.IsValid() || ItemId <= 0) return nullptr;
	UARItemDefinition* Match = nullptr;
	for (UARItemDefinition* Definition : GetCatalogDefinitions())
	{
		if (Definition->ItemTypeTag != ItemTypeTag || Definition->ItemId != ItemId) continue;
		if (Match)
		{
			UE_LOG(LogARItems, Error, TEXT("Duplicate item key (%s, %d): %s and %s"),
				*ItemTypeTag.ToString(), ItemId, *GetNameSafe(Match), *GetNameSafe(Definition));
			return nullptr;
		}
		Match = Definition;
	}
	return Match;
}

bool UARItemCatalogBlueprintLibrary::ValidateItemCatalog(TArray<FString>& Errors)
{
	Errors.Reset();
	TMap<FString, const UARItemDefinition*> SeenKeys;
	for (const UARItemDefinition* Definition : GetCatalogDefinitions())
	{
		const FString AssetName = GetNameSafe(Definition);
		const bool bLoadout = Definition->ItemTypeTag == ARGameplayTags::Item_Type_Weapon
			|| Definition->ItemTypeTag == ARGameplayTags::Item_Type_ActiveRelic
			|| Definition->ItemTypeTag == ARGameplayTags::Item_Type_PassiveRelic;
		const bool bConsumable = Definition->IsConsumable();
		if (!bLoadout && !bConsumable)
		{
			Errors.Add(FString::Printf(TEXT("%s: invalid Item Type Tag"), *AssetName));
			continue;
		}
		if (Definition->ItemId <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: Item Id must be greater than zero"), *AssetName));
		}
		else
		{
			const FString Key = FString::Printf(TEXT("%s:%d"), *Definition->ItemTypeTag.ToString(), Definition->ItemId);
			if (const UARItemDefinition* const* Previous = SeenKeys.Find(Key))
			{
				Errors.Add(FString::Printf(TEXT("%s and %s: duplicate item key %s"), *GetNameSafe(*Previous), *AssetName, *Key));
			}
			else
			{
				SeenKeys.Add(Key, Definition);
			}
		}
		const UClass* RuntimeClass = Definition->RuntimeBehaviorClass.Get();
		if (!RuntimeClass || RuntimeClass->HasAnyClassFlags(CLASS_Abstract)
			|| (bLoadout && !RuntimeClass->IsChildOf(UARLoadoutItemInstance::StaticClass()))
			|| (bConsumable && !RuntimeClass->IsChildOf(UARConsumableInstance::StaticClass())))
		{
			Errors.Add(FString::Printf(TEXT("%s: Runtime Behavior Class is missing, abstract, or incompatible with Item Type Tag"), *AssetName));
		}
	}
	for (const FString& Error : Errors)
	{
		UE_LOG(LogARItems, Error, TEXT("Item catalog validation: %s"), *Error);
	}
	return Errors.IsEmpty();
}

TArray<UARItemDefinition*> UARItemCatalogBlueprintLibrary::FindItemsByAdditionalTag(const FString& AdditionalTag, FGameplayTag TypeFilter)
{
	TArray<UARItemDefinition*> Result;
	if (AdditionalTag.TrimStartAndEnd().IsEmpty()) return Result;
	for (UARItemDefinition* Definition : GetCatalogDefinitions())
	{
		if (MatchesType(Definition, TypeFilter) && Definition->HasAdditionalTag(AdditionalTag))
		{
			Result.Add(Definition);
		}
	}
	return Result;
}

int32 UARItemCatalogBlueprintLibrary::CountOwnedItemsByAdditionalTag(const AARPlayerCharacter* Player,
	const FString& AdditionalTag, FGameplayTag TypeFilter)
{
	if (!Player || AdditionalTag.TrimStartAndEnd().IsEmpty()) return 0;
	int32 Count = 0;
	if (const UARLoadoutComponent* Loadout = Player->GetLoadoutComponent())
	{
		for (const FARLoadoutItemSnapshot& Item : Loadout->GetLoadoutInventory())
		{
			if (MatchesType(Item.Definition, TypeFilter) && Item.Definition->HasAdditionalTag(AdditionalTag)) ++Count;
		}
	}
	if (const UARConsumableComponent* Consumables = Player->GetConsumableComponent())
	{
		for (const FARConsumableSlotSnapshot& Slot : Consumables->GetConsumableSlots())
		{
			if (Slot.bOccupied && MatchesType(Slot.Definition, TypeFilter) && Slot.Definition->HasAdditionalTag(AdditionalTag)) ++Count;
		}
	}
	return Count;
}

FARRequestStatus UARItemCatalogBlueprintLibrary::TryAcquireItem(AARPlayerCharacter* Player, UARItemDefinition* Definition)
{
	FARRequestStatus Status;
	if (!Player || !Definition)
	{
		Status.Result = !Player ? EARRequestResult::InvalidOwner : EARRequestResult::InvalidDefinition;
		return Status;
	}
	if (Definition->IsConsumable())
	{
		return Player->GetConsumableComponent()->TryAcquireConsumable(Definition).Status;
	}
	if (Definition->ItemTypeTag != ARGameplayTags::Item_Type_Weapon
		&& Definition->ItemTypeTag != ARGameplayTags::Item_Type_ActiveRelic
		&& Definition->ItemTypeTag != ARGameplayTags::Item_Type_PassiveRelic)
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return Status;
	}
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	const FARLoadoutAcquisitionResult Request = Loadout->BeginLoadoutAcquisition(Definition);
	if (!Request.Status.IsSuccess()) return Request.Status;
	Loadout->CommitLoadoutAcquisition(Request.Token, Status);
	return Status;
}
