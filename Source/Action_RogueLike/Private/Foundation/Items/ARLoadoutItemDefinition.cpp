#include "Foundation/Items/ARLoadoutItemDefinition.h"

FPrimaryAssetId UARLoadoutItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ARLoadoutItem"), GetFName());
}

