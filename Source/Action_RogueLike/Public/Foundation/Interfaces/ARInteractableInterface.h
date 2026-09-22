#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARInteractableInterface.generated.h"

class AARPlayerCharacter;

UINTERFACE(BlueprintType)
class ACTION_ROGUELIKE_API UARInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class ACTION_ROGUELIKE_API IARInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Interaction")
	bool CanInteract(AARPlayerCharacter* Interactor, FText& FailureReason) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Interaction")
	FText GetInteractionPrompt(AARPlayerCharacter* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Interaction")
	int32 GetInteractionPriority(AARPlayerCharacter* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Interaction")
	bool RequiresInteractionLineOfSight() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AR|Interaction")
	FARRequestStatus Interact(AARPlayerCharacter* Interactor);
};
