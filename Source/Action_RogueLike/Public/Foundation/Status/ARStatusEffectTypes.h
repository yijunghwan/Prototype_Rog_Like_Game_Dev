#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARStatusEffectTypes.generated.h"

class UARStatusEffectDefinition;

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatusEffectRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") TObjectPtr<const UARStatusEffectDefinition> Definition = nullptr;
	/** Negative uses the Definition duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") float DurationOverride = -1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status") FARSourceInfo Source;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatusEffectResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") EARRequestResult Result = EARRequestResult::Rejected;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") FARStatusEffectHandle Handle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") FGameplayTag StatusTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") float AppliedDuration = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") bool bRefreshedExisting = false;
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARStatusEffectView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") FARStatusEffectHandle Handle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") FGameplayTag StatusTag;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") TObjectPtr<const UARStatusEffectDefinition> Definition = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") FARSourceInfo Source;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status") float RemainingTime = 0.0f;
};

