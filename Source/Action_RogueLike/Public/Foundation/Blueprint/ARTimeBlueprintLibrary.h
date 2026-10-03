#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Foundation/Time/ARTimeSubsystem.h"
#include "ARTimeBlueprintLibrary.generated.h"

UCLASS()
class ACTION_ROGUELIKE_API UARTimeBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static bool SetTimeGroupRate(const UObject* WorldContextObject, EARTimeGroup TimeGroup, float Rate);
	UFUNCTION(BlueprintPure, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static float GetTimeGroupRate(const UObject* WorldContextObject, EARTimeGroup TimeGroup);
	UFUNCTION(BlueprintPure, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static double GetTimeGroupSeconds(const UObject* WorldContextObject, EARTimeGroup TimeGroup);
	UFUNCTION(BlueprintCallable, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static void PauseTimeGroupTimer(const UObject* WorldContextObject, FTimerHandle Handle);
	UFUNCTION(BlueprintCallable, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static void UnpauseTimeGroupTimer(const UObject* WorldContextObject, FTimerHandle Handle);
	UFUNCTION(BlueprintCallable, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static void ClearTimeGroupTimer(const UObject* WorldContextObject, FTimerHandle Handle);
	UFUNCTION(BlueprintPure, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static float GetTimeGroupTimerRemaining(const UObject* WorldContextObject, FTimerHandle Handle);
	UFUNCTION(BlueprintPure, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static bool DoesTimeGroupTimerExist(const UObject* WorldContextObject, FTimerHandle Handle);
	UFUNCTION(BlueprintPure, Category="AR|Time", meta=(WorldContext="WorldContextObject"))
	static bool IsTimeGroupTimerPaused(const UObject* WorldContextObject, FTimerHandle Handle);
};
