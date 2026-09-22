#include "Foundation/Status/ARStatusEffectDefinition.h"

FPrimaryAssetId UARStatusEffectDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ARStatusEffect"), GetFName());
}

