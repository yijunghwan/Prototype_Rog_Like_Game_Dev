#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARCombatTargetInterface.generated.h"

/** Combat targets must inherit the native character foundation so team and life-state rules cannot be bypassed in BP. */
UINTERFACE(BlueprintType, meta=(CannotImplementInterfaceInBlueprint))
class ACTION_ROGUELIKE_API UARCombatTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class ACTION_ROGUELIKE_API IARCombatTargetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	virtual EARCombatTeam GetCombatTeam() const = 0;

	UFUNCTION(BlueprintCallable, Category="AR|Combat")
	virtual bool CanBeCombatTarget() const = 0;
};
