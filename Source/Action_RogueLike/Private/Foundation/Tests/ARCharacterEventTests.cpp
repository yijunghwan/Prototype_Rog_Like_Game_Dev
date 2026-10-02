#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Script.h"
#include "Foundation/Blueprint/ARCombatBlueprintLibrary.h"
#include "Foundation/Blueprint/ARResourceBlueprintLibrary.h"
#include "Foundation/Characters/ARBaseEnemy.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"
#include <limits>

namespace ARCharacterEventTests
{
	struct FWorldScope
	{
		FWorldScope()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARCharacterEvents_%s"), *FGuid::NewGuid().ToString())), GetTransientPackage());
			World->AddToRoot();
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FWorldScope()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World->RemoveFromRoot();
		}
		UWorld* World = nullptr;
	};

	struct FHookRecord
	{
		FName Name;
		TWeakObjectPtr<AARBaseCharacter> Character;
		FARActionHandle ActionHandle;
		FARStatusEffectHandle StatusHandle;
		float PreviousShield = 0.0f;
		EARResourceChangeReason ResourceReason = EARResourceChangeReason::Other;
		EARActionCancelReason CancelReason = EARActionCancelReason::None;
		bool bCleanedUp = false;
	};

	struct FHookRecorder;
	static FHookRecorder* ActiveRecorder = nullptr;
	// Only the test temporarily replaces BP event thunks. No test assets or gameplay
	// subclass are saved, and all reflection flags/function pointers are restored.
	struct FHookRecorder
	{
		struct FSlot { UFunction* Function; EFunctionFlags Flags; FNativeFuncPtr Native; };
		TArray<FSlot> Slots;
		TArray<FHookRecord> Records;
		FHookRecorder()
		{
			check(!ActiveRecorder);
			ActiveRecorder = this;
			Hook(TEXT("ReceiveCharacterDeath"), &Death);
			Hook(TEXT("ReceiveGroggyGaugeDepleted"), &Groggy);
			Hook(TEXT("ReceiveShieldBroken"), &Shield);
			Hook(TEXT("ReceiveDamageApplied"), &Damage);
			Hook(TEXT("ReceiveActionCancelled"), &Cancelled);
			Hook(TEXT("ReceiveStatusRemoved"), &StatusRemoved);
		}
		~FHookRecorder()
		{
			for (const FSlot& Slot : Slots)
			{
				Slot.Function->FunctionFlags = Slot.Flags;
				Slot.Function->SetNativeFunc(Slot.Native);
			}
			ActiveRecorder = nullptr;
		}
		void Hook(FName Name, FNativeFuncPtr Native)
		{
			UFunction* Function = AARBaseCharacter::StaticClass()->FindFunctionByName(Name);
			check(Function && Function->HasAnyFunctionFlags(FUNC_BlueprintEvent));
			Slots.Add({Function, Function->FunctionFlags, Function->GetNativeFunc()});
			Function->FunctionFlags |= FUNC_Native;
			Function->SetNativeFunc(Native);
		}
		static FHookRecord& Add(UObject* Context, FName Name)
		{
			FHookRecord& Record = ActiveRecorder->Records.AddDefaulted_GetRef();
			Record.Name = Name;
			Record.Character = CastChecked<AARBaseCharacter>(Context);
			return Record;
		}
		int32 Count(FName Name) const
		{
			return Records.FilterByPredicate([Name](const FHookRecord& R) { return R.Name == Name; }).Num();
		}
		static void Death(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_STRUCT_REF(FARCombatDamageResult, KillingDamage);
			P_FINISH;
			FHookRecord& R = Add(Context, TEXT("Death"));
			AARBaseCharacter* C = R.Character.Get();
			R.bCleanedUp = C->GetHealthComponent()->IsDead()
				&& C->GetActionComponent()->GetActiveActionCount() == 0
				&& !C->GetMovementControlComponent()->CanMoveAtAll()
				&& C->GetStatusEffectComponent()->GetActiveStatusEffects().IsEmpty()
				&& KillingDamage.bKilledTarget;
		}
		static void Groggy(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_FINISH;
			FHookRecord& R = Add(Context, TEXT("Groggy"));
			R.bCleanedUp = R.Character->GetStaggerComponent()->GetCurrentGroggy() == 0.0f;
		}
		static void Shield(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_PROPERTY(FFloatProperty, PreviousShield);
			P_GET_ENUM(EARResourceChangeReason, Reason);
			P_FINISH;
			FHookRecord& R = Add(Context, TEXT("Shield"));
			R.PreviousShield = PreviousShield;
			R.ResourceReason = Reason;
		}
		static void Damage(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_STRUCT_REF(FARCombatDamageResult, DamageResult);
			P_FINISH;
			FHookRecord& R = Add(Context, TEXT("Damage"));
			R.bCleanedUp = DamageResult.WasApplied();
		}
		static void Cancelled(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_STRUCT(FARActionHandle, ActionHandle);
			P_GET_ENUM(EARActionCancelReason, Reason);
			P_FINISH;
			FHookRecord& R = Add(Context, TEXT("Cancel"));
			R.ActionHandle = ActionHandle;
			R.CancelReason = Reason;
			R.bCleanedUp = !R.Character->GetActionComponent()->IsActionActive(ActionHandle);
		}
		static void StatusRemoved(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_STRUCT_REF(FARStatusEffectView, Status);
			P_FINISH;
			FHookRecord& R = Add(Context, TEXT("Status"));
			R.StatusHandle = Status.Handle;
			R.bCleanedUp = R.Character->GetMovementControlComponent()->CanBasicMove();
		}
	};

	AARBaseEnemy* SpawnEnemy(UWorld* World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AARBaseEnemy* Enemy = World->SpawnActor<AARBaseEnemy>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		Enemy->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 100.0f);
		Enemy->GetStatsComponent()->SetBaseStat(EARStatType::MaxGroggy, 100.0f);
		Enemy->GetStaggerComponent()->bUseGroggyGauge = true;
		Enemy->DispatchBeginPlay();
		return Enemy;
	}

	FARCombatDamageResult Resolved(int32 Shield, int32 Health)
	{
		FARCombatDamageResult R;
		R.Outcome = EARDamageOutcome::Applied;
		R.FailureReason = EARRequestResult::Success;
		R.FinalDamage = Shield + Health;
		R.ShieldDamage = Shield;
		R.HealthDamage = Health;
		return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARShieldBrokenEventsTest, "AR.Foundation.Health.ShieldBrokenEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARShieldBrokenEventsTest::RunTest(const FString& Parameters)
{
	using namespace ARCharacterEventTests;
	FWorldScope TestWorld;
	AARBaseEnemy* Enemy = SpawnEnemy(TestWorld.World);
	UARHealthComponent* Health = Enemy->GetHealthComponent();
	TArray<EARResourceChangeReason> Reasons;
	TArray<float> Previous;
	const FDelegateHandle Listener = Health->OnShieldBrokenNative.AddLambda([&](AActor* Target, float Before, EARResourceChangeReason Reason)
	{
		TestEqual(TEXT("Break target is the owner"), Target, static_cast<AActor*>(Enemy));
		Previous.Add(Before);
		Reasons.Add(Reason);
	});
	TestNotNull(TEXT("Health exposes a BP-assignable shield break delegate"), Health->GetClass()->FindPropertyByName(TEXT("OnShieldBroken")));
	bool bSuccess = false;
	auto AddShield = [&](float Amount, float Duration = -1.0f)
	{
		FARShieldSpec Spec; Spec.Amount = Amount; Spec.Duration = Duration;
		return Health->ApplyShield(Spec, bSuccess);
	};
	float Removed = 0.0f;
	Health->ClearShields(Removed);
	TestEqual(TEXT("Clearing an empty shield does not break"), Reasons.Num(), 0);
	const FARShieldHandle Older = AddShield(30.0f);
	AddShield(20.0f);
	FARCombatDamageResult Partial = Resolved(25, 0);
	Health->ApplyResolvedDamage(Partial);
	TestEqual(TEXT("Breaking one layer with total remaining is not a total break"), Reasons.Num(), 0);
	TestEqual(TEXT("Expected shield remains"), Health->GetCurrentShield(), 25.0f);
	Health->RemoveShield(Older, Removed);
	TestEqual(TEXT("Manual removal of last layer breaks once"), Reasons.Num(), 1);
	TestEqual(TEXT("Manual break is Other"), Reasons.Last(), EARResourceChangeReason::Other);
	TestEqual(TEXT("Before payload is total immediately before removal"), Previous.Last(), 25.0f);
	Health->RemoveShield(Older, Removed);
	TestEqual(TEXT("Stale removal does not duplicate break"), Reasons.Num(), 1);
	AddShield(5.0f); AddShield(5.0f);
	Health->ClearShields(Removed);
	TestEqual(TEXT("Batch clear breaks once, not per layer"), Reasons.Num(), 2);
	AddShield(10.0f);
	FARCombatDamageResult Exact = Resolved(10, 0);
	Health->ApplyResolvedDamage(Exact);
	TestEqual(TEXT("Exact shield depletion breaks"), Reasons.Num(), 3);
	TestEqual(TEXT("Damage reason is retained"), Reasons.Last(), EARResourceChangeReason::Damage);
	AddShield(10.0f);
	FARCombatDamageResult Bypass = Resolved(0, 5);
	Health->ApplyResolvedDamage(Bypass);
	TestEqual(TEXT("Bypass leaves shield intact"), Health->GetCurrentShield(), 10.0f);
	TestEqual(TEXT("Bypass does not break"), Reasons.Num(), 3);
	Health->ClearShields(Removed);
	const int32 BeforeExpiry = Reasons.Num();
	AddShield(7.0f, 0.1f);
	TestWorld.World->Tick(LEVELTICK_All, 0.2f);
	Health->TickComponent(0.2f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Expired total is zero"), Health->GetCurrentShield(), 0.0f);
	TestEqual(TEXT("Time expiry is not a break"), Reasons.Num(), BeforeExpiry);
	Health->ClearShields(Removed);
	TestEqual(TEXT("Manual clear after expiry is not a break"), Reasons.Num(), BeforeExpiry);
	AddShield(1.0f, 0.1f); AddShield(9.0f);
	TestWorld.World->Tick(LEVELTICK_All, 0.2f);
	Health->TickComponent(0.2f, LEVELTICK_All, nullptr);
	FARCombatDamageResult Spill = Resolved(9, 1);
	Health->ApplyResolvedDamage(Spill);
	TestEqual(TEXT("Damage to remaining shield after an expiry still breaks"), Reasons.Num(), BeforeExpiry + 1);
	TestEqual(TEXT("Spill damage also changes health"), Spill.HealthDamage, 1);
	AddShield(3.0f);
	bool bReentered = false;
	const FDelegateHandle Reentrant = Health->OnShieldBrokenNative.AddLambda([&](AActor*, float, EARResourceChangeReason)
	{
		if (!bReentered) { bReentered = true; AddShield(4.0f); }
	});
	FARCombatDamageResult ReentrantHit = Resolved(3, 0);
	Health->ApplyResolvedDamage(ReentrantHit);
	TestTrue(TEXT("Break observer can add a new shield"), bReentered);
	TestEqual(TEXT("New shield survives original operation"), Health->GetCurrentShield(), 4.0f);
	Health->OnShieldBrokenNative.Remove(Reentrant);
	Health->OnShieldBrokenNative.Remove(Listener);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCharacterDirectEventsTest, "AR.Foundation.Character.DirectEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARCharacterDirectEventsTest::RunTest(const FString& Parameters)
{
	using namespace ARCharacterEventTests;
	FWorldScope TestWorld;
	TGuardValue<bool> ScriptGuard(GAllowActorScriptExecutionInEditor, true);
	FHookRecorder Recorder;
	AARBaseEnemy* Enemy = SpawnEnemy(TestWorld.World);
	TestTrue(TEXT("BeginPlay has initialized health"), Enemy->GetHealthComponent()->GetCurrentHealth() == 100.0f);
	FARShieldSpec Shield; Shield.Amount = 10.0f;
	bool bSuccess = false;
	Enemy->GetHealthComponent()->ApplyShield(Shield, bSuccess);
	FARCombatDamageResult Hit = Resolved(10, 0);
	Enemy->GetHealthComponent()->ApplyResolvedDamage(Hit);
	TestEqual(TEXT("Direct shield event forwards once without BP Bind"), Recorder.Count(TEXT("Shield")), 1);
	TestEqual(TEXT("Direct damage event forwards once"), Recorder.Count(TEXT("Damage")), 1);
	TestEqual(TEXT("Damage shield event precedes applied damage hook"), Recorder.Records[0].Name, FName(TEXT("Shield")));
	TestEqual(TEXT("Shield hook carries damage reason"), Recorder.Records[0].ResourceReason, EARResourceChangeReason::Damage);
	FARRequestStatus StartStatus;
	const FARActionHandle Handle = Enemy->GetActionComponent()->TryStartAction(FARActionRequest(), StartStatus);
	Enemy->GetActionComponent()->CancelAction(Handle, EARActionCancelReason::Manual);
	TestTrue(TEXT("Cancel hook forwards correct action"), Recorder.Records.Last().ActionHandle == Handle);
	TestTrue(TEXT("Cancelled hook sees action already removed"), Recorder.Records.Last().bCleanedUp);
	const FARActionHandle Normal = Enemy->GetActionComponent()->TryStartAction(FARActionRequest(), StartStatus);
	Enemy->GetActionComponent()->EndAction(Normal);
	TestEqual(TEXT("Normal end is not cancellation"), Recorder.Count(TEXT("Cancel")), 1);
	FARCombatDamageResult GroggyHit;
	GroggyHit.Outcome = EARDamageOutcome::Applied;
	GroggyHit.HitContext.HitId = FGuid::NewGuid();
	GroggyHit.HitContext.Target = Enemy;
	GroggyHit.HitContext.bValidHit = true;
	UARCombatBlueprintLibrary::ApplyStaggerAndGroggyDamageFromResult(GroggyHit, 0.0f, 1.0f, 100.0f, FARSourceInfo(), NAME_None);
	TestEqual(TEXT("Groggy hook forwards real depletion"), Recorder.Count(TEXT("Groggy")), 1);
	TestTrue(TEXT("Groggy hook sees exhausted gauge"), Recorder.Records.Last().bCleanedUp);
	UARCombatBlueprintLibrary::ApplyStaggerAndGroggyDamageFromResult(GroggyHit, 0.0f, 1.0f, 1.0f, FARSourceInfo(), NAME_None);
	TestEqual(TEXT("Locked zero gauge does not repeat event"), Recorder.Count(TEXT("Groggy")), 1);
	UARStatusEffectDefinition* Definition = NewObject<UARStatusEffectDefinition>();
	Definition->StatusTag = ARGameplayTags::Status_Stun;
	Definition->BaseDuration = 1.0f;
	Definition->bBlocksAllMovement = true;
	Definition->bAffectedByTenacity = false;
	FARStatusEffectRequest Request; Request.Definition = Definition;
	const FARStatusEffectResult Status = Enemy->GetStatusEffectComponent()->ApplyStatusEffect(Request);
	TestFalse(TEXT("Status blocks movement"), Enemy->GetMovementControlComponent()->CanBasicMove());
	Enemy->GetStatusEffectComponent()->RemoveStatusEffect(Status.Handle);
	TestEqual(TEXT("Status removal hook forwards once"), Recorder.Count(TEXT("Status")), 1);
	TestEqual(TEXT("Removed status forwards exact handle"), Recorder.Records.Last().StatusHandle.Id, Status.Handle.Id);
	TestTrue(TEXT("Removed hook runs after movement-lock refresh"), Recorder.Records.Last().bCleanedUp);
	Enemy->GetActionComponent()->TryStartAction(FARActionRequest(), StartStatus);
	Enemy->GetStatusEffectComponent()->ApplyStatusEffect(Request);
	FARCombatDamageResult Lethal = Resolved(0, 100);
	Enemy->GetHealthComponent()->ApplyResolvedDamage(Lethal);
	TestEqual(TEXT("Death hook forwarded once"), Recorder.Count(TEXT("Death")), 1);
	TestTrue(TEXT("Death hook runs after shared cleanup"), Recorder.Records.Last().bCleanedUp);
	Enemy->GetHealthComponent()->ApplyResolvedDamage(Lethal);
	TestEqual(TEXT("Repeated dead damage does not repeat death"), Recorder.Count(TEXT("Death")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCurrentResourceRestoreTest, "AR.Foundation.Resource.RestoreCurrentValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARCurrentResourceRestoreTest::RunTest(const FString& Parameters)
{
	using namespace ARCharacterEventTests;
	FWorldScope TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 100.0f);
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxMana, 100.0f);
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxStamina, 100.0f);
	Player->DispatchBeginPlay();
	FARCombatDamageResult Damage = Resolved(0, 30);
	Player->GetHealthComponent()->ApplyResolvedDamage(Damage);
	FARSourceInfo Source;
	float Current = 0.0f;
	Player->GetManaComponent()->TryConsume(30.0f, Source, Current);
	Player->GetStaminaComponent()->TryConsume(30.0f, Source, Current);
	TestEqual(TEXT("Restore Health returns actual clamped addition"), UARResourceBlueprintLibrary::RestoreHealth(Player, 40.0f, Source), 30.0f);
	TestEqual(TEXT("Restore Mana returns actual clamped addition"), UARResourceBlueprintLibrary::RestoreMana(Player, 40.0f, Source), 30.0f);
	TestEqual(TEXT("Restore Stamina returns actual clamped addition"), UARResourceBlueprintLibrary::RestoreStamina(Player, 40.0f, Source), 30.0f);
	TestEqual(TEXT("Maximum is not increased by restoration"), Player->GetStatsComponent()->GetFinalStat(EARStatType::MaxMana), 100.0f);
	TestEqual(TEXT("Full health restores zero"), UARResourceBlueprintLibrary::RestoreHealth(Player, 10.0f, Source), 0.0f);
	TestEqual(TEXT("Negative mana restore is rejected"), UARResourceBlueprintLibrary::RestoreMana(Player, -1.0f, Source), 0.0f);
	TestEqual(TEXT("Nonfinite stamina restore is rejected"), UARResourceBlueprintLibrary::RestoreStamina(Player, std::numeric_limits<float>::infinity(), Source), 0.0f);
	AARBaseEnemy* Enemy = SpawnEnemy(TestWorld.World);
	TestEqual(TEXT("Enemy without mana component restores zero"), UARResourceBlueprintLibrary::RestoreMana(Enemy, 10.0f, Source), 0.0f);
	TestEqual(TEXT("Enemy without stamina component restores zero"), UARResourceBlueprintLibrary::RestoreStamina(Enemy, 10.0f, Source), 0.0f);
	FARCombatDamageResult Lethal = Resolved(0, 100);
	Player->GetHealthComponent()->ApplyResolvedDamage(Lethal);
	TestEqual(TEXT("Health restore does not revive"), UARResourceBlueprintLibrary::RestoreHealth(Player, 100.0f, Source), 0.0f);
	TestEqual(TEXT("Missing target restores zero"), UARResourceBlueprintLibrary::RestoreHealth(nullptr, 10.0f, Source), 0.0f);
	return true;
}

#endif
