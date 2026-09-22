#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARCameraFollowComponent.generated.h"

class USceneComponent;

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARCameraFollowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARCameraFollowComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetCameraAnchor(USceneComponent* InCameraAnchor);

	UFUNCTION(BlueprintCallable, Category="AR|Camera") void SnapToTarget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AR|Camera", meta=(ClampMin="0.0")) float BaseFollowSpeed = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AR|Camera", meta=(ClampMin="0.0")) float AccelerationStartDistance = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AR|Camera", meta=(ClampMin="0.0")) float DistanceSpeedMultiplier = 0.025f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AR|Camera", meta=(ClampMin="0.0")) float MaximumFollowSpeed = 18.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AR|Camera", meta=(ClampMin="0.0")) float SnapDistance = 1200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AR|Camera") float CameraHeight = 2000.0f;

private:
	UPROPERTY(Transient) TObjectPtr<USceneComponent> CameraAnchor;
};

