#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ARPlayerController.generated.h"

class UInputMappingContext;
class UARResourceHUDWidget;

UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AARPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category="AR|Input")
	bool ProjectMouseToGameplayPlane(float PlaneZ, FVector& WorldPoint) const;

	UFUNCTION(BlueprintCallable, Category="AR|Input")
	void SetGameplayUIInputMode(bool bUIOpen);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") TObjectPtr<UInputMappingContext> PlayerMappingContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|Input") int32 MappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|UI") bool bShowResourceHUD = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AR|UI") TSoftClassPtr<UARResourceHUDWidget> ResourceHUDClass;

private:
	void RefreshResourceHUD();
	UPROPERTY(Transient) TObjectPtr<UARResourceHUDWidget> ResourceHUD;
};
