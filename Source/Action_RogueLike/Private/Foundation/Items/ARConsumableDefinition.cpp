#include "Foundation/Items/ARConsumableDefinition.h"

FPrimaryAssetId UARConsumableDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ARConsumable"), GetFName());
}
