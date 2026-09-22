#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARActionHitboxActor.generated.h"

UENUM(BlueprintType)
enum class EARHitboxHitPolicy : uint8
{
	OncePerTarget,
	EveryAcceptedOverlap
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FARHitboxTargetAcceptedSignature, AActor*, Target);

UCLASS(Abstract, Blueprintable)
class ACTION_ROGUELIKE_API AARActionHitboxActor : public AActor
{
	GENERATED_BODY()

public:
	AARActionHitboxActor();

	UFUNCTION(BlueprintCallable, Category="AR|Hitbox")
	bool InitializeHitbox(FARActionHandle InActionHandle, AActor* InCombatSource, EARHitboxHitPolicy InHitPolicy, float Lifetime);

	UFUNCTION(BlueprintCallable, Category="AR|Hitbox")
	bool TryAcceptTarget(AActor* Candidate);

	UFUNCTION(BlueprintCallable, Category="AR|Hitbox")
	void ResetAcceptedTargets();

	UFUNCTION(BlueprintPure, Category="AR|Hitbox") FARActionHandle GetActionHandle() const { return ActionHandle; }
	UFUNCTION(BlueprintPure, Category="AR|Hitbox") AActor* GetCombatSource() const { return CombatSource.Get(); }
	UFUNCTION(BlueprintPure, Category="AR|Hitbox") bool HasAcceptedTarget(AActor* Target) const;

	UPROPERTY(BlueprintAssignable, Category="AR|Hitbox") FARHitboxTargetAcceptedSignature OnTargetAccepted;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="AR|Hitbox", meta=(DisplayName="On Hitbox Target Accepted"))
	void ReceiveTargetAccepted(AActor* Target);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Hitbox") FARActionHandle ActionHandle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AR|Hitbox") EARHitboxHitPolicy HitPolicy = EARHitboxHitPolicy::OncePerTarget;

private:
	UPROPERTY(Transient) TWeakObjectPtr<AActor> CombatSource;
	UPROPERTY(Transient) TSet<TWeakObjectPtr<AActor>> AcceptedTargets;
	bool bInitialized = false;
};
