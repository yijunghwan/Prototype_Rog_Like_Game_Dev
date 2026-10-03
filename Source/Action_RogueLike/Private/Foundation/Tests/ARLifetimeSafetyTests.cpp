#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "GameplayTagsManager.h"
#include "UObject/Script.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Characters/ARBaseEnemy.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARUIManagerComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Combat/ARCombatSubsystem.h"
#include "Foundation/Blueprint/ARCombatBlueprintLibrary.h"
#include <limits>

namespace ARLifetimeSafetyTests
{
	struct FWorldScope
	{
		UWorld* World;
		FWorldScope()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARLifetime_%s"), *FGuid::NewGuid().ToString())), GetTransientPackage());
			World->AddToRoot();
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FWorldScope()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World->RemoveFromRoot();
		}
		template<class T> T* Spawn()
		{
			FActorSpawnParameters Parameters;
			Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			T* Actor = World->SpawnActor<T>(FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
			Actor->DispatchBeginPlay();
			return Actor;
		}
		void Advance(float Seconds)
		{
			for (float Remaining = Seconds; Remaining > KINDA_SMALL_NUMBER;)
			{
				const float Step = FMath::Min(0.01f, Remaining);
				++GFrameCounter; // Synthetic worlds otherwise share the automation runner's frame.
				World->Tick(LEVELTICK_All, Step);
				Remaining -= Step;
			}
		}
	};

	// Temporary reflected thunks exercise the actual dynamic-delegate path without
	// adding test-only Blueprint nodes/classes or saving any gameplay assets.
	struct FCallbacks;
	static FCallbacks* ActiveCallbacks = nullptr;
	struct FCallbacks
	{
		struct FSlot { UFunction* Function; EFunctionFlags Flags; FNativeFuncPtr Native; };
		TArray<FSlot> Slots;
		TFunction<void(EARStatType, float, float)> OnStat;
		TFunction<void(FName, int32)> OnSource;
		TFunction<void(UARLoadoutItemInstance*)> OnTimer;
		int32 Depth = 0;
		int32 MaxDepth = 0;
		FCallbacks()
		{
			check(!ActiveCallbacks);
			ActiveCallbacks = this;
			Hook(UARManaComponent::StaticClass(), TEXT("HandleFinalStatChanged"), &Stat);
			Hook(UARUIManagerComponent::StaticClass(), TEXT("HandleStatModifiersChanged"), &Source);
			Hook(UARLoadoutItemInstance::StaticClass(), TEXT("ReceiveItemRegistered"), &Timer);
		}
		~FCallbacks()
		{
			for (const FSlot& Slot : Slots)
			{
				Slot.Function->FunctionFlags = Slot.Flags;
				Slot.Function->SetNativeFunc(Slot.Native);
			}
			ActiveCallbacks = nullptr;
		}
		void Hook(UClass* Class, FName Name, FNativeFuncPtr Native)
		{
			UFunction* Function = Class->FindFunctionByName(Name);
			check(Function);
			Slots.Add({Function, Function->FunctionFlags, Function->GetNativeFunc()});
			Function->FunctionFlags |= FUNC_Native;
			Function->SetNativeFunc(Native);
		}
		static void Stat(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_OBJECT(AActor, Target);
			P_GET_ENUM(EARStatType, StatType);
			P_GET_PROPERTY(FFloatProperty, OldValue);
			P_GET_PROPERTY(FFloatProperty, NewValue);
			P_FINISH;
			++ActiveCallbacks->Depth;
			ActiveCallbacks->MaxDepth = FMath::Max(ActiveCallbacks->MaxDepth, ActiveCallbacks->Depth);
			if (ActiveCallbacks->OnStat) ActiveCallbacks->OnStat(static_cast<EARStatType>(StatType), OldValue, NewValue);
			--ActiveCallbacks->Depth;
		}
		static void Source(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_OBJECT(AActor, Target);
			P_GET_PROPERTY(FNameProperty, SourceId);
			P_GET_PROPERTY(FTextProperty, DisplayName);
			P_GET_PROPERTY(FIntProperty, StackCount);
			P_GET_PROPERTY(FFloatProperty, LongestRemainingTime);
			P_FINISH;
			if (ActiveCallbacks->OnSource) ActiveCallbacks->OnSource(SourceId, StackCount);
		}
		static void Timer(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_FINISH;
			if (ActiveCallbacks->OnTimer) ActiveCallbacks->OnTimer(CastChecked<UARLoadoutItemInstance>(Context));
		}
		void Bind(UARStatsComponent* Stats)
		{
			UARManaComponent* Probe = NewObject<UARManaComponent>(Stats->GetOwner());
			Stats->GetOwner()->AddInstanceComponent(Probe);
			FScriptDelegate StatDelegate;
			StatDelegate.BindUFunction(Probe, TEXT("HandleFinalStatChanged"));
			Stats->OnFinalStatChanged.Clear();
			Stats->OnFinalStatChanged.Add(StatDelegate);
			UARUIManagerComponent* SourceProbe = NewObject<UARUIManagerComponent>(Stats->GetOwner());
			Stats->GetOwner()->AddInstanceComponent(SourceProbe);
			FScriptDelegate SourceDelegate;
			SourceDelegate.BindUFunction(SourceProbe, TEXT("HandleStatModifiersChanged"));
			Stats->OnStatModifiersChanged.Clear();
			Stats->OnStatModifiersChanged.Add(SourceDelegate);
		}
	};

	FARStatModifierSpec Modifier(FName Source = TEXT("LifetimeTest"), float Value = 1.0f)
	{
		FARStatModifierSpec Spec;
		Spec.StatType = EARStatType::AttackPower;
		Spec.Value = Value;
		Spec.Source.SourceId = Source;
		return Spec;
	}
	UARLoadoutItemInstance* Item(AARPlayerCharacter* Player, bool bViaLoadout = false)
	{
		UARItemDefinition* Definition = NewObject<UARItemDefinition>(Player);
		Definition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
		Definition->ItemId = 999;
		Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
		if (bViaLoadout)
		{
			const FARLoadoutAcquisitionResult Acquisition = Player->GetLoadoutComponent()->BeginLoadoutAcquisition(Definition);
			FARRequestStatus Status;
			return Player->GetLoadoutComponent()->CommitLoadoutAcquisition(Acquisition.Token, Status);
		}
		UARLoadoutItemInstance* Instance = NewObject<UARLoadoutItemInstance>(Player);
		Instance->InitializeInstance(Player, Definition);
		Instance->RegisterItem();
		return Instance;
	}
	FTimerDynamicDelegate Event(UARLoadoutItemInstance* Instance)
	{
		FTimerDynamicDelegate Delegate;
		Delegate.BindUFunction(Instance, TEXT("ReceiveItemRegistered"));
		return Delegate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatReentrantMutationTest, "AR.Foundation.Safety.Stats.ReentrantMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARStatReentrantMutationTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARStatsComponent* Stats = Player->GetStatsComponent();
	FCallbacks Callbacks;
	Callbacks.Bind(Stats);
	bool bMutated = false;
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		if (bMutated || Type != EARStatType::AttackPower) return;
		bMutated = true;
		TestEqual(TEXT("Callback removes the in-flight addition"), Stats->ClearModifiers(), 1);
		for (int32 Index = 0; Index < 256; ++Index)
		{
			bool bAdded;
			Stats->AddStatModifier(Modifier(TEXT("Replacement")), bAdded);
			TestTrue(TEXT("Nested growth accepted"), bAdded);
		}
	};
	Callbacks.OnSource = [&](FName Id, int32 Count)
	{
		TestEqual(TEXT("Source notification observes the current cache"), Stats->GetFinalStat(EARStatType::AttackPower), 256.0f);
		TestEqual(TEXT("Source notification carries current group count"), Count, Stats->GetModifiersBySource(EARModifierSourceCategory::All, Id).StackCount);
	};
	bool bSuccess;
	const FARStatModifierHandle Original = Stats->AddStatModifier(Modifier(), bSuccess);
	TestTrue(TEXT("Initial addition succeeded before callback removal"), bSuccess);
	bool bPermanent; float Remaining;
	TestFalse(TEXT("Callback-removed handle is no longer active"), Stats->GetModifierRemainingTime(Original, bPermanent, Remaining));
	TestEqual(TEXT("Replacement survived array reallocation"), Stats->GetFinalStat(EARStatType::AttackPower), 256.0f);
	TestEqual(TEXT("No recursive stat notification stack"), Callbacks.MaxDepth, 1);
	Callbacks.OnSource = nullptr;
	Callbacks.OnStat = nullptr;
	TestEqual(TEXT("All replacement records removable"), Stats->ClearModifiers(), 256);
	TestEqual(TEXT("Cache restored"), Stats->GetFinalStat(EARStatType::AttackPower), 0.0f);
	TestFalse(TEXT("No timed records or pending events leave tick running"), Stats->IsComponentTickEnabled());
	Stats->SetBaseStat(EARStatType::AttackPower, std::numeric_limits<float>::infinity());
	TestEqual(TEXT("Non-finite base rejected"), Stats->GetBaseStat(EARStatType::AttackPower), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatBatchAndLoopTest, "AR.Foundation.Safety.Stats.AtomicBatchAndLoopBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARStatBatchAndLoopTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	UARStatsComponent* Stats = Scope.Spawn<AARPlayerCharacter>()->GetStatsComponent();
	FCallbacks Callbacks;
	Callbacks.Bind(Stats);
	bool bSuccess;
	FARStatModifierSpec Spec = Modifier();
	Spec.StackGroupHandle = Stats->AddStatModifier(Spec, bSuccess);
	Spec.StatType = EARStatType::SpellPower;
	Stats->AddStatModifier(Spec, bSuccess);
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		TestEqual(TEXT("Whole batch attack cache committed before events"), Stats->GetFinalStat(EARStatType::AttackPower), 0.0f);
		TestEqual(TEXT("Whole batch spell cache committed before events"), Stats->GetFinalStat(EARStatType::SpellPower), 0.0f);
	};
	int32 Removed; TArray<FARStatModifierHandle> Handles;
	TestTrue(TEXT("One group consumed atomically"), Stats->RemoveModifierStacks(EARModifierSourceCategory::All, TEXT("LifetimeTest"), 1, Removed, Handles));
	TestEqual(TEXT("Both members removed"), Handles.Num(), 2);
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		if (Type == EARStatType::AttackPower && New < 1100.0f) Stats->SetBaseStat(Type, New + 1.0f);
	};
	AddExpectedError(TEXT("Stat notification budget reached"), EAutomationExpectedErrorFlags::Contains, 1);
	Stats->SetBaseStat(EARStatType::AttackPower, 1.0f);
	TestTrue(TEXT("Long callback chain is deferred instead of hanging"), Stats->IsComponentTickEnabled());
	for (int32 Index = 0; Index < 3; ++Index) Stats->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Deferred notifications finish finite chain"), Stats->GetFinalStat(EARStatType::AttackPower), 1100.0f);
	TestEqual(TEXT("No deep recursion"), Callbacks.MaxDepth, 1);
	TestFalse(TEXT("Deferred-only tick stops after drainage"), Stats->IsComponentTickEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatOwnedReentrancyTest, "AR.Foundation.Safety.Stats.OwnedEffectReentrancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARStatOwnedReentrancyTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARStatsComponent* Stats = Player->GetStatsComponent();
	UARActionComponent* Actions = Player->GetActionComponent();
	UARLoadoutItemInstance* Instance = Item(Player);
	FCallbacks Callbacks;
	Callbacks.Bind(Stats);
	FARRequestStatus Status;
	const FARActionHandle Action = Actions->TryStartAction(FARActionRequest(), Status);
	bool bChanged = false;
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		if (bChanged || New <= 0) return;
		bChanged = true;
		TestTrue(TEXT("Can cancel while adding its effect"), Actions->CancelAction(Action, EARActionCancelReason::Stun));
		for (int32 Index = 0; Index < 128; ++Index) Actions->TryStartAction(FARActionRequest(), Status);
	};
	bool bSuccess;
	Actions->ApplyActionStatModifier(Action, Modifier(), bSuccess);
	TestEqual(TEXT("Cancelled in-flight action has no leaked modifier"), Stats->GetFinalStat(EARStatType::AttackPower), 0.0f);
	TestEqual(TEXT("New actions survive reallocation"), Actions->GetActiveActionCount(), 128);
	Actions->CancelAllActions();
	bChanged = false;
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		if (bChanged || New <= 0) return;
		bChanged = true;
		Instance->UnregisterItem(EARItemRemovalReason::Discarded);
		Instance->UnregisterItem(EARItemRemovalReason::Discarded);
	};
	Instance->ApplyItemStatModifier(Modifier(), bSuccess);
	TestFalse(TEXT("Item can be unregistered during addition"), Instance->IsRegistered());
	TestEqual(TEXT("Item-owned in-flight modifier cleaned up"), Stats->GetFinalStat(EARStatType::AttackPower), 0.0f);
	TestEqual(TEXT("No remaining modifier handles"), Stats->ClearModifiers(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARManagedTimerTest, "AR.Foundation.Safety.Timers.ActionAndItemLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARManagedTimerTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARLoadoutItemInstance* Instance = Item(Player, true);
	if (!TestNotNull(TEXT("Real loadout acquisition"), Instance)) return false;
	UARActionComponent* Actions = Player->GetActionComponent();
	FTimerManager& Timers = Scope.World->GetTimerManager();
	FCallbacks Callbacks;
	int32 Calls = 0;
	Callbacks.OnTimer = [&](UARLoadoutItemInstance*) { ++Calls; };
	bool bSuccess;
	const FTimerHandle Once = Instance->SetItemTimerByEvent(Event(Instance), 0.02f, false, bSuccess);
	TestTrue(TEXT("One-shot item delay scheduled"), bSuccess);
	Scope.Advance(0.08f);
	TestEqual(TEXT("One-shot only calls once"), Calls, 1);
	TestFalse(TEXT("Completed one-shot not left in manager"), Timers.TimerExists(Once));
	const FTimerHandle Loop = Instance->SetItemTimerByEvent(Event(Instance), 0.02f, true, bSuccess);
	Scope.Advance(0.08f);
	TestTrue(TEXT("Loop repeats"), Calls > 2);
	Timers.PauseTimer(Loop);
	const int32 PausedCalls = Calls;
	Scope.Advance(0.08f);
	TestEqual(TEXT("Engine pause works"), Calls, PausedCalls);
	Timers.UnPauseTimer(Loop);
	Scope.Advance(0.04f);
	TestTrue(TEXT("Engine unpause works"), Calls > PausedCalls);
	FTimerHandle Clearable = Loop;
	Timers.ClearTimer(Clearable);
	const int32 ClearedCalls = Calls;
	Scope.Advance(0.08f);
	TestEqual(TEXT("Manual clear stops calls"), Calls, ClearedCalls);
	FARRequestStatus Status;
	FARActionHandle Action = Actions->TryStartAction(FARActionRequest(), Status);
	const FTimerHandle ActionLoop = Actions->SetActionTimerByEvent(Action, Event(Instance), 0.02f, true, bSuccess);
	TestTrue(TEXT("Action timer scheduled"), bSuccess);
	Actions->EndAction(Action);
	TestFalse(TEXT("Normal end clears timer immediately"), Timers.TimerExists(ActionLoop));
	Action = Actions->TryStartAction(FARActionRequest(), Status);
	const FTimerHandle CancelledLoop = Actions->SetActionTimerByEvent(Action, Event(Instance), 0.02f, true, bSuccess);
	bool bCCSuccess; EARRequestResult Failure; float Duration;
	UARCombatBlueprintLibrary::ApplyCrowdControl(Player, EARCrowdControlType::Stun, 0.1f, true, bCCSuccess, Failure, Duration, FARSourceInfo());
	TestTrue(TEXT("Stun applied"), bCCSuccess);
	TestFalse(TEXT("CC cancellation clears timer immediately"), Timers.TimerExists(CancelledLoop));
	Player->GetStatusEffectComponent()->ClearAllStatusEffects();
	Action = Actions->TryStartAction(FARActionRequest(), Status);
	Callbacks.OnTimer = [&](UARLoadoutItemInstance*) { ++Calls; Actions->EndAction(Action); };
	const FTimerHandle SelfEnding = Actions->SetActionTimerByEvent(Action, Event(Instance), 0.02f, true, bSuccess);
	const int32 BeforeSelf = Calls;
	Scope.Advance(0.08f);
	TestEqual(TEXT("Callback may end its own action; no second call"), Calls, BeforeSelf + 1);
	TestFalse(TEXT("Executing timer cleared safely"), Timers.TimerExists(SelfEnding));
	Callbacks.OnTimer = [&](UARLoadoutItemInstance* I) { ++Calls; I->UnregisterItem(EARItemRemovalReason::Discarded); };
	const FTimerHandle SelfUnregister = Instance->SetItemTimerByEvent(Event(Instance), 0.02f, true, bSuccess);
	const int32 BeforeUnregister = Calls;
	Scope.Advance(0.08f);
	TestEqual(TEXT("Callback may unregister its item; no second call"), Calls, BeforeUnregister + 1);
	TestFalse(TEXT("Unregistration clears executing timer"), Timers.TimerExists(SelfUnregister));
	TestFalse(TEXT("Unregistered item cannot create timer"), Instance->SetItemTimerByEvent(Event(Instance), 0.02f, true, bSuccess).IsValid());
	TestFalse(TEXT("Creation failure reported"), bSuccess);
	Instance->RegisterItem();
	Callbacks.OnTimer = nullptr;
	const FTimerHandle OwnerTimer = Instance->SetItemTimerByEvent(Event(Instance), 0.02f, true, bSuccess);
	Player->Destroy();
	TestFalse(TEXT("Owner EndPlay clears loadout item timer"), Timers.TimerExists(OwnerTimer));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatExpiryDestructionTest, "AR.Foundation.Safety.Stats.ExpiryInventoryAndDestruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARStatExpiryDestructionTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARStatsComponent* Stats = Player->GetStatsComponent();
	FCallbacks Callbacks;
	Callbacks.Bind(Stats);
	UARItemDefinition* Definition = NewObject<UARItemDefinition>(Player);
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
	Definition->ItemId = 1000;
	Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
	Definition->DefaultStatModifiers.Add(Modifier());
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		TestEqual(TEXT("Inventory ownership committed before default-effect event"), Player->GetLoadoutComponent()->GetPassiveRelics().Num(), 1);
	};
	const FARLoadoutAcquisitionResult Acquisition = Player->GetLoadoutComponent()->BeginLoadoutAcquisition(Definition);
	FARRequestStatus Status;
	UARLoadoutItemInstance* Instance = Player->GetLoadoutComponent()->CommitLoadoutAcquisition(Acquisition.Token, Status);
	TestNotNull(TEXT("Registered item acquired"), Instance);
	Callbacks.OnStat = nullptr;
	Instance->RemoveAllOwnItemModifiers();
	bool bSuccess;
	FARStatModifierSpec Spec = Modifier(TEXT("Expiry"));
	Spec.Duration = 0.02f;
	Spec.StackGroupHandle = Stats->AddStatModifier(Spec, bSuccess);
	Spec.StatType = EARStatType::SpellPower;
	Stats->AddStatModifier(Spec, bSuccess);
	bool bReapplied = false;
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New)
	{
		if (bReapplied || New != 0.0f) return;
		TestEqual(TEXT("Expiry batch has updated both caches"), Stats->GetFinalStat(EARStatType::SpellPower), 0.0f);
		TestEqual(TEXT("Expiry batch has updated attack cache"), Stats->GetFinalStat(EARStatType::AttackPower), 0.0f);
		bReapplied = true;
		FARStatModifierSpec Replacement = Modifier(TEXT("AfterExpiry"));
		Replacement.Duration = 5.0f;
		Stats->AddStatModifier(Replacement, bSuccess);
	};
	Scope.Advance(0.05f);
	Stats->TickComponent(0.05f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Expired callback reapplication ran"), bReapplied);
	TestTrue(TEXT("New timed effect retains ticking"), Stats->IsComponentTickEnabled());
	TestEqual(TEXT("New record not confused with expired group"), Stats->GetModifiersBySource(EARModifierSourceCategory::All, TEXT("AfterExpiry")).StackCount, 1);
	Callbacks.OnStat = nullptr;
	Stats->ClearModifiers();
	int32 SourceCalls = 0;
	Callbacks.OnSource = [&](FName Id, int32 Count) { ++SourceCalls; };
	Callbacks.OnStat = [&](EARStatType Type, float Old, float New) { Player->Destroy(); };
	Stats->AddStatModifier(Modifier(), bSuccess);
	TestTrue(TEXT("Owner destroyed from notification safely"), Player->IsActorBeingDestroyed());
	TestEqual(TEXT("No later queued source callback after destruction"), SourceCalls, 0);
	Stats->AddStatModifier(Modifier(), bSuccess);
	TestFalse(TEXT("Destroyed owner rejects further modifications"), bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARManagedTimerValidationTest, "AR.Foundation.Safety.Timers.ValidationAndIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARManagedTimerValidationTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	for (const TCHAR* Key : { TEXT("Q"), TEXT("W"), TEXT("E"), TEXT("R"), TEXT("A"), TEXT("S"), TEXT("D"), TEXT("F"), TEXT("G"), TEXT("Z"), TEXT("X"), TEXT("C") })
	{
		const FName Tag(*FString::Printf(TEXT("Input.Skill.%s"), Key));
		TestTrue(TEXT("Requested physical-key tag is registered"), UGameplayTagsManager::Get().RequestGameplayTag(Tag, false).IsValid());
	}
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARLoadoutItemInstance* Instance = Item(Player);
	FCallbacks Callbacks;
	int32 Calls = 0;
	Callbacks.OnTimer = [&](UARLoadoutItemInstance*) { ++Calls; };
	bool bSuccess;
	TestFalse(TEXT("Zero interval rejected"), Instance->SetItemTimerByEvent(Event(Instance), 0.0f, true, bSuccess).IsValid());
	TestFalse(TEXT("Unbound callback rejected"), Instance->SetItemTimerByEvent(FTimerDynamicDelegate(), 1.0f, false, bSuccess).IsValid());
	TestFalse(TEXT("NaN interval rejected"), Instance->SetItemTimerByEvent(Event(Instance), std::numeric_limits<float>::quiet_NaN(), true, bSuccess).IsValid());
	TestFalse(TEXT("Unsupported negative delay rejected"), Instance->SetItemTimerByEvent(Event(Instance), 1.0f, false, bSuccess, -2.0f).IsValid());
	FARRequestStatus Status;
	UARActionComponent* Actions = Player->GetActionComponent();
	const FARActionHandle A = Actions->TryStartAction(FARActionRequest(), Status);
	const FARActionHandle B = Actions->TryStartAction(FARActionRequest(), Status);
	const FTimerHandle TimerA = Actions->SetActionTimerByEvent(A, Event(Instance), 0.01f, true, bSuccess);
	const FTimerHandle TimerB = Actions->SetActionTimerByEvent(B, Event(Instance), 0.01f, true, bSuccess);
	TestTrue(TEXT("Separate calls produce independent handles"), TimerA != TimerB);
	Actions->CancelAction(A, EARActionCancelReason::Manual);
	TestFalse(TEXT("Only A is cleared"), Scope.World->GetTimerManager().TimerExists(TimerA));
	TestTrue(TEXT("B remains"), Scope.World->GetTimerManager().TimerExists(TimerB));
	Scope.Advance(0.04f);
	TestTrue(TEXT("Other action keeps firing"), Calls > 0);
	Actions->EndAction(B);
	const FTimerHandle Delayed = Instance->SetItemTimerByEvent(Event(Instance), 0.01f, false, bSuccess, 0.1f);
	const int32 Before = Calls;
	Scope.Advance(0.05f);
	TestEqual(TEXT("Initial delay separate from interval"), Calls, Before);
	Scope.Advance(0.08f);
	TestEqual(TEXT("Initial delay fires once"), Calls, Before + 1);
	const FTimerHandle MaxOnce = Instance->SetItemTimerByEvent(Event(Instance), 0.0001f, true, bSuccess);
	const int32 BeforeMaxOnce = Calls;
	Scope.Advance(0.01f);
	TestEqual(TEXT("Default limits catch-up to one call per frame"), Calls, BeforeMaxOnce + 1);
	Instance->UnregisterItem(EARItemRemovalReason::Discarded);
	TestFalse(TEXT("Paused/completed/active ownership cleanup is safe"), Scope.World->GetTimerManager().TimerExists(MaxOnce));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCombatLifetimeStressTest, "AR.Foundation.Stress.CombatLifetimeCycles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARCombatLifetimeStressTest::RunTest(const FString& Parameters)
{
	using namespace ARLifetimeSafetyTests;
	FWorldScope Scope;
	const double Start = FPlatformTime::Seconds();
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARCombatSubsystem* Combat = Scope.World->GetSubsystem<UARCombatSubsystem>();
	TArray<AARBaseEnemy*> Enemies;
	for (int32 Index = 0; Index < 32; ++Index) Enemies.Add(Scope.Spawn<AARBaseEnemy>());
	int32 AppliedHits = 0;
	int32 RejectedHits = 0;
	int32 RemovedModifiers = 0;
	for (int32 Cycle = 0; Cycle < 20; ++Cycle)
	{
		UARLoadoutItemInstance* Instance = Item(Player);
		bool bSuccess;
		const FTimerHandle ItemTimer = Instance->SetItemTimerByEvent(Event(Instance), 0.02f, true, bSuccess);
		TestTrue(TEXT("Stress item timer scheduled"), bSuccess);
		Instance->ApplyItemStatModifier(Modifier(TEXT("StressItem")), bSuccess);
		for (AARBaseEnemy* Enemy : Enemies)
		{
			UARStatsComponent* Stats = Enemy->GetStatsComponent();
			{
				const UARStatsComponent::FScopedNotifications Batch(Stats);
				for (int32 Buff = 0; Buff < 16; ++Buff)
				{
					FARStatModifierSpec Spec = Modifier(TEXT("StressTimed"));
					Spec.Duration = 0.05f;
					Stats->AddStatModifier(Spec, bSuccess);
				}
			}
			FARShieldSpec Shield; Shield.Amount = 10.0f;
			Enemy->GetHealthComponent()->ApplyShield(Shield, bSuccess);
			FARRequestStatus Status;
			const FARActionHandle Action = Enemy->GetActionComponent()->TryStartAction(FARActionRequest(), Status);
			const FTimerHandle ActionTimer = Enemy->GetActionComponent()->SetActionTimerByEvent(Action, Event(Instance), 0.02f, true, bSuccess);
			TestTrue(TEXT("Stress action timer scheduled"), bSuccess);
			Enemy->GetActionComponent()->ApplyActionStatModifier(Action, Modifier(TEXT("StressAction")), bSuccess);
			AActor* Hitbox = Scope.World->SpawnActor<AActor>();
			Enemy->GetActionComponent()->RegisterActionHitbox(Action, Hitbox);
			FARCombatDamageRequest Request;
			Request.Attacker = Player;
			Request.Target = Enemy;
			Request.BaseDamage = 1.0f;
			Request.bCanCrit = false;
			Request.bGuaranteedHit = true;
			Request.DamageName = TEXT("StressHit");
			const FARCombatDamageResult Hit = Combat->ApplyCombatDamage(Request);
			AppliedHits += Hit.WasApplied();
			RejectedHits += !Combat->ApplyCombatDamage(Request).WasApplied();
			FARDamageOverTimeSpec Dot;
			Dot.DamageRequest = Request;
			Dot.DamageRequest.DamageName = NAME_None;
			Dot.Duration = 0.04f;
			Dot.TickInterval = 0.02f;
			Dot.DotName = TEXT("StressDot");
			EARRequestResult Failure;
			Combat->ApplyDamageOverTime(Dot, bSuccess, Failure);
			float AppliedDuration;
			UARCombatBlueprintLibrary::ApplyCrowdControl(Enemy, EARCrowdControlType::Stun, 0.04f, true,
				bSuccess, Failure, AppliedDuration, FARSourceInfo());
			TestEqual(TEXT("CC leaves no active actions"), Enemy->GetActionComponent()->GetActiveActionCount(), 0);
			TestFalse(TEXT("Stress action timer cleared"), Scope.World->GetTimerManager().TimerExists(ActionTimer));
			TestTrue(TEXT("Registered hitbox destroyed on cancellation"), Hitbox->IsActorBeingDestroyed());
		}
		Scope.Advance(0.25f);
		Combat->Tick(0.25f);
		for (AARBaseEnemy* Enemy : Enemies)
		{
			Enemy->GetStatsComponent()->TickComponent(0.25f, LEVELTICK_All, nullptr);
			Enemy->GetStatusEffectComponent()->TickComponent(0.25f, LEVELTICK_All, nullptr);
			RemovedModifiers += Enemy->GetStatsComponent()->ClearModifiers();
			TestEqual(TEXT("Timed buffs expire without residual stats"), Enemy->GetStatsComponent()->GetFinalStat(EARStatType::AttackPower), 0.0f);
			TestEqual(TEXT("DOTs expire"), Combat->GetActiveDamageOverTimeCount(Enemy), 0);
			TestTrue(TEXT("CC expires"), Enemy->GetStatusEffectComponent()->GetActiveStatusEffects().IsEmpty());
			float RemovedShield;
			Enemy->GetHealthComponent()->ClearShields(RemovedShield);
		}
		Instance->UnregisterItem(EARItemRemovalReason::Discarded);
		TestFalse(TEXT("Stress item timer cleared"), Scope.World->GetTimerManager().TimerExists(ItemTimer));
		TestEqual(TEXT("Repeated item removal does not accumulate effects"), Player->GetStatsComponent()->GetFinalStat(EARStatType::AttackPower), 0.0f);
	}
	TestEqual(TEXT("All first named hits applied"), AppliedHits, 640);
	TestEqual(TEXT("All immediate duplicate named hits ignored"), RejectedHits, 640);
	TestEqual(TEXT("No expired modifier records left to manually remove"), RemovedModifiers, 0);
	AddInfo(FString::Printf(TEXT("Synthetic 32-enemy/20-cycle test: 10,240 timed modifiers, 640 actions/hitboxes/DOTs/CC; %.3f seconds. This is not rendered FPS or a shipping benchmark."), FPlatformTime::Seconds() - Start));
	for (AARBaseEnemy* Enemy : Enemies) Enemy->Destroy();
	Player->Destroy();
	return true;
}

#endif
