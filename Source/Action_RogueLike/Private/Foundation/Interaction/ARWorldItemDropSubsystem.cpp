#include "Foundation/Interaction/ARWorldItemDropSubsystem.h"

#include "Engine/World.h"
#include "Foundation/Interaction/ARItemPickupActors.h"
#include "Foundation/Items/ARConsumableDefinition.h"
#include "Foundation/Items/ARLoadoutItemDefinition.h"
#include "Kismet/GameplayStatics.h"

bool UARWorldItemDropSubsystem::TrySpawnLoadoutPickup(const UARLoadoutItemDefinition* Definition, FVector DesiredLocation,
	TSubclassOf<AARLoadoutItemPickup> PickupClass, AActor* DropOwner, AARLoadoutItemPickup*& SpawnedPickup)
{
	SpawnedPickup = nullptr;
	UWorld* World = GetWorld();
	if (!World || !Definition)
	{
		return false;
	}
	UClass* SpawnClass = PickupClass ? PickupClass.Get() : AARLoadoutItemPickup::StaticClass();
	const FTransform SpawnTransform(FRotator::ZeroRotator, DesiredLocation);
	AARLoadoutItemPickup* Pickup = World->SpawnActorDeferred<AARLoadoutItemPickup>(
		SpawnClass, SpawnTransform, DropOwner, Cast<APawn>(DropOwner),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Pickup)
	{
		return false;
	}
	Pickup->AssignItemDefinition(Definition);
	UGameplayStatics::FinishSpawningActor(Pickup, SpawnTransform);
	if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed())
	{
		return false;
	}
	SpawnedPickup = Pickup;
	return true;
}

bool UARWorldItemDropSubsystem::TrySpawnConsumablePickup(UARConsumableDefinition* Definition, FVector DesiredLocation,
	TSubclassOf<AARConsumablePickup> PickupClass, AActor* DropOwner, AARConsumablePickup*& SpawnedPickup)
{
	SpawnedPickup = nullptr;
	UWorld* World = GetWorld();
	if (!World || !Definition)
	{
		return false;
	}
	UClass* SpawnClass = PickupClass ? PickupClass.Get() : AARConsumablePickup::StaticClass();
	const FTransform SpawnTransform(FRotator::ZeroRotator, DesiredLocation);
	AARConsumablePickup* Pickup = World->SpawnActorDeferred<AARConsumablePickup>(
		SpawnClass, SpawnTransform, DropOwner, Cast<APawn>(DropOwner),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Pickup)
	{
		return false;
	}
	Pickup->AssignConsumableDefinition(Definition);
	UGameplayStatics::FinishSpawningActor(Pickup, SpawnTransform);
	if (!IsValid(Pickup) || Pickup->IsActorBeingDestroyed())
	{
		return false;
	}
	SpawnedPickup = Pickup;
	return true;
}
