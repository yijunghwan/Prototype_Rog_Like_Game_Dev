#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ARItemInstance.generated.h"

/** Common runtime class selector; the owning inventory chooses the concrete lifecycle. */
UCLASS(Abstract, Blueprintable, BlueprintType)
class ACTION_ROGUELIKE_API UARItemInstance : public UObject
{
	GENERATED_BODY()
};
