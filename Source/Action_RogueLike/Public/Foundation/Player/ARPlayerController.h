#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ARPlayerController.generated.h"

class UInputMappingContext;

UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AARPlayerController();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="AR|Input")
	bool ProjectMouseToGameplayPlane(float PlaneZ, FVector& WorldPoint) const;

	UFUNCTION(BlueprintCallable, Category="AR|Input")
	void SetGameplayUIInputMode(bool bUIOpen);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputMappingContext> PlayerMappingContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") int32 MappingPriority = 0;
};
