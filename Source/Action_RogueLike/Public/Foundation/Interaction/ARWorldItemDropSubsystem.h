#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARWorldItemDropSubsystem.generated.h"

class AARConsumablePickup;
class AARLoadoutItemPickup;
class UARConsumableDefinition;
class UARLoadoutItemDefinition;

UCLASS()
class ACTION_ROGUELIKE_API UARWorldItemDropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="AR|Pickup")
	bool TrySpawnLoadoutPickup(const UARLoadoutItemDefinition* Definition, FVector DesiredLocation,
		TSubclassOf<AARLoadoutItemPickup> PickupClass, AActor* DropOwner, AARLoadoutItemPickup*& SpawnedPickup);

	UFUNCTION(BlueprintCallable, Category="AR|Pickup")
	bool TrySpawnConsumablePickup(UARConsumableDefinition* Definition, FVector DesiredLocation,
		TSubclassOf<AARConsumablePickup> PickupClass, AActor* DropOwner, AARConsumablePickup*& SpawnedPickup);
};
