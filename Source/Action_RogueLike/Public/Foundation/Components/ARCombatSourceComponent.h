#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARCombatSourceComponent.generated.h"

/** Gives traps, hazards, and other world actors a valid Environment combat identity. */
UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARCombatSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="AR|Combat")
	EARCombatTeam GetCombatTeam() const { return EARCombatTeam::Environment; }

	UFUNCTION(BlueprintPure, Category="AR|Combat")
	const FARSourceInfo& GetSourceInfo() const { return SourceInfo; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Combat")
	FARSourceInfo SourceInfo;
};

