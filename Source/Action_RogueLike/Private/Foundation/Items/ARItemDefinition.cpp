#include "Foundation/Items/ARItemDefinition.h"

#include "Foundation/Core/ARGameplayTags.h"

FPrimaryAssetId UARItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ARItem"), GetFName());
}

EARLoadoutItemKind UARItemDefinition::GetItemKind() const
{
	if (ItemTypeTag == ARGameplayTags::Item_Type_Weapon) return EARLoadoutItemKind::Weapon;
	if (ItemTypeTag == ARGameplayTags::Item_Type_ActiveRelic) return EARLoadoutItemKind::ActiveRelic;
	return EARLoadoutItemKind::PassiveRelic;
}

bool UARItemDefinition::IsConsumable() const
{
	return ItemTypeTag == ARGameplayTags::Item_Type_Consumable;
}

bool UARItemDefinition::HasAdditionalTag(const FString& Tag) const
{
	const FString Requested = Tag.TrimStartAndEnd();
	return !Requested.IsEmpty() && AdditionalTags.ContainsByPredicate([&Requested](const FString& Candidate)
	{
		return Candidate.TrimStartAndEnd().Equals(Requested, ESearchCase::IgnoreCase);
	});
}
