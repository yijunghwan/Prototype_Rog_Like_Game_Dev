#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARInteractionComponent.generated.h"

class AARPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARInteractionCandidateChangedSignature, AActor*, Candidate, FText, Prompt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FARInteractionCompletedSignature, AActor*, Target, FARRequestStatus, Status);

UCLASS(ClassGroup=(ARFoundation), meta=(BlueprintSpawnableComponent))
class ACTION_ROGUELIKE_API UARInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARInteractionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="AR|Interaction")
	FARRequestStatus TryInteract();

	UFUNCTION(BlueprintCallable, Category="AR|Interaction")
	void RefreshInteractionCandidate();

	UFUNCTION(BlueprintPure, Category="AR|Interaction") AActor* GetCurrentCandidate() const { return CurrentCandidate.Get(); }
	UFUNCTION(BlueprintPure, Category="AR|Interaction") FText GetCurrentPrompt() const { return CurrentPrompt; }

	UPROPERTY(BlueprintAssignable, Category="AR|Interaction") FARInteractionCandidateChangedSignature OnInteractionCandidateChanged;
	UPROPERTY(BlueprintAssignable, Category="AR|Interaction") FARInteractionCompletedSignature OnInteractionCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Interaction", meta=(ClampMin="1.0")) float InteractionRadius = 160.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Interaction", meta=(ClampMin="0.02")) float ScanInterval = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AR|Interaction") TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

private:
	bool HasLineOfSightTo(const AActor* Candidate) const;
	void SetCurrentCandidate(AActor* Candidate);

	UPROPERTY(Transient) TObjectPtr<AARPlayerCharacter> PlayerOwner;
	UPROPERTY(Transient) TWeakObjectPtr<AActor> CurrentCandidate;
	UPROPERTY(Transient) FText CurrentPrompt;
	float ScanAccumulator = 0.0f;
};
