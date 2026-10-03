#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "ARTimeSubsystem.generated.h"

UENUM(BlueprintType)
enum class EARTimeGroup : uint8
{
	World,
	Player
};

/** Pause-aware gameplay clocks. World uses Unreal time; Player has a separate timer manager. */
UCLASS()
class ACTION_ROGUELIKE_API UARTimeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	bool SetTimeGroupRate(EARTimeGroup TimeGroup, float Rate);
	float GetTimeGroupRate(EARTimeGroup TimeGroup) const;
	double GetTimeGroupSeconds(EARTimeGroup TimeGroup) const;
	void PauseTimeGroupTimer(FTimerHandle Handle);
	void UnpauseTimeGroupTimer(FTimerHandle Handle);
	void ClearTimeGroupTimer(FTimerHandle Handle);
	float GetTimeGroupTimerRemaining(FTimerHandle Handle) const;
	bool DoesTimeGroupTimerExist(FTimerHandle Handle) const;
	bool IsTimeGroupTimerPaused(FTimerHandle Handle) const;
	FTimerManager& GetTimers(EARTimeGroup Group);
	FTimerManager& FindTimers(FTimerHandle Handle) const;
	FTimerHandle FindEventTimer(const TArray<FTimerHandle>& Handles, const FTimerDynamicDelegate& Event);
	void RememberEvent(FTimerHandle Handle, const FTimerDynamicDelegate& Event);
	void RegisterPlayer(AActor* Player);
	void UnregisterPlayer(AActor* Player);
	static double Now(const UObject* Context, EARTimeGroup Group);
	static double OwnerNow(const AActor* Owner);
private:
	void PreActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void UpdatePlayers();
	TUniquePtr<FTimerManager> PlayerTimers;
	TMap<FTimerHandle, FTimerDynamicDelegate> Events;
	TMap<TWeakObjectPtr<AActor>, float> Players;
	FDelegateHandle TickBinding;
	double PlayerSeconds = 0.0;
	float PlayerRate = 1.0f;
};
