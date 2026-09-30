#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARWorldItemDropSubsystem.generated.h"

class AARConsumablePickup;
class AARLoadoutItemPickup;
class UARItemDefinition;

UCLASS()
class ACTION_ROGUELIKE_API UARWorldItemDropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="AR|Pickup")
	bool TrySpawnLoadoutPickup(const UARItemDefinition* Definition, FVector DesiredLocation,
		TSubclassOf<AARLoadoutItemPickup> PickupClass, AActor* DropOwner, AARLoadoutItemPickup*& SpawnedPickup);

	UFUNCTION(BlueprintCallable, Category="AR|Pickup")
	bool TrySpawnConsumablePickup(UARItemDefinition* Definition, FVector DesiredLocation,
		TSubclassOf<AARConsumablePickup> PickupClass, AActor* DropOwner, AARConsumablePickup*& SpawnedPickup);
};
