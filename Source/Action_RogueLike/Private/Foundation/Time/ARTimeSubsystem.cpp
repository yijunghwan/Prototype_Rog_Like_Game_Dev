#include "Foundation/Time/ARTimeSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Foundation/Characters/ARPlayerCharacter.h"

bool UARTimeSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE || Type == EWorldType::GamePreview;
}
void UARTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PlayerSeconds = GetWorld()->GetTimeSeconds();
	PlayerTimers = MakeUnique<FTimerManager>(GetWorld()->GetGameInstance());
	TickBinding = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UARTimeSubsystem::PreActorTick);
}
void UARTimeSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldPreActorTick.Remove(TickBinding);
	for (const auto& Pair : Players) if (AActor* Player = Pair.Key.Get()) Player->CustomTimeDilation = Pair.Value;
	Players.Reset();
	Events.Reset();
	PlayerTimers.Reset();
	Super::Deinitialize();
}
void UARTimeSubsystem::PreActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (World != GetWorld() || World->bIsTearingDown || World->IsPaused() || TickType == LEVELTICK_TimeOnly) return;
	UpdatePlayers();
	const float Effective = World->GetWorldSettings()->GetEffectiveTimeDilation();
	const float Delta = Effective > SMALL_NUMBER ? DeltaSeconds / Effective * PlayerRate : 0.0f;
	PlayerSeconds += Delta;
	PlayerTimers->Tick(Delta);
	for (auto It = Events.CreateIterator(); It; ++It) if (!DoesTimeGroupTimerExist(It.Key())) It.RemoveCurrent();
}
bool UARTimeSubsystem::SetTimeGroupRate(EARTimeGroup Group, float Rate)
{
	if (!FMath::IsFinite(Rate) || Rate < 0.001f || Rate > 10.0f || GetWorld()->bIsTearingDown) return false;
	if (Group == EARTimeGroup::World) GetWorld()->GetWorldSettings()->SetTimeDilation(Rate);
	else if (Group == EARTimeGroup::Player) PlayerRate = Rate;
	else return false;
	UpdatePlayers();
	return true;
}
float UARTimeSubsystem::GetTimeGroupRate(EARTimeGroup Group) const
{
	return Group == EARTimeGroup::Player ? PlayerRate : GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
}
double UARTimeSubsystem::GetTimeGroupSeconds(EARTimeGroup Group) const
{
	return Group == EARTimeGroup::Player ? PlayerSeconds : GetWorld()->GetTimeSeconds();
}
void UARTimeSubsystem::RegisterPlayer(AActor* Player)
{
	if (IsValid(Player) && Player->GetWorld() == GetWorld() && !Players.Contains(Player)) Players.Add(Player, Player->CustomTimeDilation);
	UpdatePlayers();
}
void UARTimeSubsystem::UnregisterPlayer(AActor* Player)
{
	if (const float* Baseline = Players.Find(Player)) Player->CustomTimeDilation = *Baseline;
	Players.Remove(Player);
}
void UARTimeSubsystem::UpdatePlayers()
{
	const float Effective = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
	for (auto It = Players.CreateIterator(); It; ++It)
	{
		if (AActor* Player = It.Key().Get()) Player->CustomTimeDilation = It.Value() * PlayerRate / FMath::Max(SMALL_NUMBER, Effective);
		else It.RemoveCurrent();
	}
}
FTimerManager& UARTimeSubsystem::GetTimers(EARTimeGroup Group)
{
	return Group == EARTimeGroup::Player ? *PlayerTimers : GetWorld()->GetTimerManager();
}
FTimerManager& UARTimeSubsystem::FindTimers(FTimerHandle Handle) const
{
	// FTimerManager serial numbers are global across managers, so handles cannot collide.
	return PlayerTimers && PlayerTimers->TimerExists(Handle) ? *PlayerTimers : GetWorld()->GetTimerManager();
}
void UARTimeSubsystem::PauseTimeGroupTimer(FTimerHandle Handle) { FindTimers(Handle).PauseTimer(Handle); }
void UARTimeSubsystem::UnpauseTimeGroupTimer(FTimerHandle Handle) { FindTimers(Handle).UnPauseTimer(Handle); }
void UARTimeSubsystem::ClearTimeGroupTimer(FTimerHandle Handle) { FindTimers(Handle).ClearTimer(Handle); Events.Remove(Handle); }
float UARTimeSubsystem::GetTimeGroupTimerRemaining(FTimerHandle Handle) const { return FindTimers(Handle).GetTimerRemaining(Handle); }
bool UARTimeSubsystem::DoesTimeGroupTimerExist(FTimerHandle Handle) const { return FindTimers(Handle).TimerExists(Handle); }
bool UARTimeSubsystem::IsTimeGroupTimerPaused(FTimerHandle Handle) const { return FindTimers(Handle).IsTimerPaused(Handle); }
FTimerHandle UARTimeSubsystem::FindEventTimer(const TArray<FTimerHandle>& Handles, const FTimerDynamicDelegate& Event)
{
	for (FTimerHandle Handle : Handles) if (const FTimerDynamicDelegate* Existing = Events.Find(Handle))
		if (*Existing == Event && DoesTimeGroupTimerExist(Handle)) return Handle;
	return FTimerHandle();
}
void UARTimeSubsystem::RememberEvent(FTimerHandle Handle, const FTimerDynamicDelegate& Event) { Events.Add(Handle, Event); }
double UARTimeSubsystem::Now(const UObject* Context, EARTimeGroup Group)
{
	UWorld* World = Context ? Context->GetWorld() : nullptr;
	if (!World) return 0.0;
	if (const UARTimeSubsystem* Time = World->GetSubsystem<UARTimeSubsystem>()) return Time->GetTimeGroupSeconds(Group);
	return World->GetTimeSeconds();
}
double UARTimeSubsystem::OwnerNow(const AActor* Owner)
{
	return Now(Owner, Cast<AARPlayerCharacter>(Owner) ? EARTimeGroup::Player : EARTimeGroup::World);
}
