#pragma once

#include "CoreMinimal.h"
#include "ARUITypes.generated.h"

UENUM(BlueprintType)
enum class EARUIScreen : uint8
{
	None,
	Inventory,
	EvolutionSelection,
	Shop,
	Dialogue,
	Menu,
	Custom
};
