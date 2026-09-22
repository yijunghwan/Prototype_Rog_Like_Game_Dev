#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARCombatTargetInterface.generated.h"

UINTERFACE(BlueprintType)
class ACTION_ROGUELIKE_API UARCombatTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class ACTION_ROGUELIKE_API IARCombatTargetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Combat")
	EARCombatTeam GetCombatTeam() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Combat")
	bool CanBeCombatTarget() const;
};

