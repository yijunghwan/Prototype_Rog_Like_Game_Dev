#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Foundation/Interfaces/ARInteractableInterface.h"
#include "ARItemPickupActors.generated.h"

class USphereComponent;
class UARLoadoutItemDefinition;
class UARConsumableDefinition;

UCLASS(Abstract, Blueprintable)
class ACTION_ROGUELIKE_API AARItemPickupBase : public AActor, public IARInteractableInterface
{
	GENERATED_BODY()

public:
	AARItemPickupBase();

	virtual bool CanInteract_Implementation(AARPlayerCharacter* Interactor, FText& FailureReason) const override;
	virtual FText GetInteractionPrompt_Implementation(AARPlayerCharacter* Interactor) const override;
	virtual int32 GetInteractionPriority_Implementation(AARPlayerCharacter* Interactor) const override;
	virtual bool RequiresInteractionLineOfSight_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Pickup") TObjectPtr<USphereComponent> InteractionVolume;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Pickup") FText InteractionPrompt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Pickup") int32 InteractionPriority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Pickup") bool bRequiresLineOfSight = false;
};

UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARLoadoutItemPickup : public AARItemPickupBase
{
	GENERATED_BODY()

public:
	virtual bool CanInteract_Implementation(AARPlayerCharacter* Interactor, FText& FailureReason) const override;
	virtual FARRequestStatus Interact_Implementation(AARPlayerCharacter* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Pickup") TObjectPtr<UARLoadoutItemDefinition> ItemDefinition;
};

UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARConsumablePickup : public AARItemPickupBase
{
	GENERATED_BODY()

public:
	virtual bool CanInteract_Implementation(AARPlayerCharacter* Interactor, FText& FailureReason) const override;
	virtual FARRequestStatus Interact_Implementation(AARPlayerCharacter* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Pickup") TObjectPtr<UARConsumableDefinition> ConsumableDefinition;
};
