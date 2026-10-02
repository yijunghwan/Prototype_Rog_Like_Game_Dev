#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Foundation/Blueprint/ARCombatBlueprintLibrary.h"
#include "Foundation/Characters/ARBaseEnemy.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"
#include <limits>

namespace ARCrowdControlTests
{
	struct FWorldScope
	{
		UWorld* World;
		FWorldScope()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARCrowdControl_%s"), *FGuid::NewGuid().ToString())), GetTransientPackage());
			World->AddToRoot();
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FWorldScope()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World->RemoveFromRoot();
		}
		AARBaseEnemy* Spawn()
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AARBaseEnemy* Enemy = World->SpawnActor<AARBaseEnemy>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			Enemy->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 100.0f);
			Enemy->DispatchBeginPlay();
			return Enemy;
		}
	};
	struct FResult
	{
		FARStatusEffectHandle Handle;
		bool bSuccess = true;
		EARRequestResult Reason = EARRequestResult::Success;
		float Duration = -1.0f;
	};
	FResult Apply(AActor* Target, EARCrowdControlType Type, float Duration, bool bTenacity = true, const FARSourceInfo& Source = FARSourceInfo())
	{
		FResult Result;
		Result.Handle = UARCombatBlueprintLibrary::ApplyCrowdControl(Target, Type, Duration, bTenacity,
			Result.bSuccess, Result.Reason, Result.Duration, Source);
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCrowdControlRulesTest, "AR.Foundation.Status.CrowdControl.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARCrowdControlRulesTest::RunTest(const FString& Parameters)
{
	using namespace ARCrowdControlTests;
	FWorldScope Scope;
	AARBaseEnemy* Enemy = Scope.Spawn();
	UARActionComponent* Actions = Enemy->GetActionComponent();
	FARRequestStatus Started;
	const FARActionHandle Skill = Actions->TryStartAction(FARActionRequest(), Started);
	TestTrue(TEXT("Skill starts"), Skill.IsValid());
	const FResult Stun = Apply(Enemy, EARCrowdControlType::Stun, 3.0f);
	TestTrue(TEXT("Stun succeeds without a DA"), Stun.bSuccess && Stun.Handle.IsValid());
	TestEqual(TEXT("Stun duration"), Stun.Duration, 3.0f);
	TestFalse(TEXT("Stun cancels default-cancellable skill"), Actions->IsActionActive(Skill));
	TestFalse(TEXT("Stun blocks all movement"), Enemy->GetMovementControlComponent()->CanMoveAtAll());
	TestFalse(TEXT("Stun blocks new skill"), Actions->CanStartAction(FARActionRequest()).IsSuccess());
	TestTrue(TEXT("Remove uses existing status node"), UARCombatBlueprintLibrary::RemoveStatusEffect(Enemy, Stun.Handle));
	TestTrue(TEXT("Stun removal restores movement"), Enemy->GetMovementControlComponent()->CanBasicMove());
	const FARActionHandle Skill2 = Actions->TryStartAction(FARActionRequest(), Started);
	const FResult Root = Apply(Enemy, EARCrowdControlType::Root, 2.0f);
	TestTrue(TEXT("Root succeeds"), Root.bSuccess);
	TestTrue(TEXT("Root preserves skill"), Actions->IsActionActive(Skill2));
	TestTrue(TEXT("Root permits skill"), Actions->CanStartAction(FARActionRequest()).IsSuccess());
	TestFalse(TEXT("Root blocks basic movement"), Enemy->GetMovementControlComponent()->CanBasicMove());
	TestTrue(TEXT("Root leaves action movement possible"), Enemy->GetMovementControlComponent()->CanMoveAtAll());
	FARActionRequest Roll;
	Roll.bIsRollAction = true;
	TestFalse(TEXT("Root blocks new roll"), Actions->CanStartAction(Roll).IsSuccess());
	UARCombatBlueprintLibrary::RemoveStatusEffect(Enemy, Root.Handle);
	Actions->EndAction(Skill2);
	const FARActionHandle RollHandle = Actions->TryStartAction(Roll, Started);
	TestTrue(TEXT("Roll starts before root"), RollHandle.IsValid());
	Apply(Enemy, EARCrowdControlType::Root, 2.0f);
	TestFalse(TEXT("Root cancels an active roll"), Actions->IsActionActive(RollHandle));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCrowdControlDurationTest, "AR.Foundation.Status.CrowdControl.DurationImmunityLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARCrowdControlDurationTest::RunTest(const FString& Parameters)
{
	using namespace ARCrowdControlTests;
	FWorldScope Scope;
	AARBaseEnemy* Enemy = Scope.Spawn();
	UARStatusEffectComponent* Status = Enemy->GetStatusEffectComponent();
	Enemy->GetStatsComponent()->SetBaseStat(EARStatType::Tenacity, 50.0f);
	FARSourceInfo Source;
	Source.SourceId = TEXT("SimpleCCSource");
	const FResult First = Apply(Enemy, EARCrowdControlType::Stun, 4.0f, true, Source);
	TestEqual(TEXT("50 tenacity halves duration"), First.Duration, 2.0f);
	const FResult Longer = Apply(Enemy, EARCrowdControlType::Stun, 3.0f, false, Source);
	TestEqual(TEXT("Disabled tenacity gives full duration"), Longer.Duration, 3.0f);
	TestTrue(TEXT("Refresh retains same handle"), First.Handle == Longer.Handle);
	Apply(Enemy, EARCrowdControlType::Stun, 1.0f);
	float Remaining = 0.0f;
	Status->GetStatusRemainingTime(ARGameplayTags::Status_Stun, Remaining);
	TestEqual(TEXT("Shorter request never shortens status"), Remaining, 3.0f);
	TestEqual(TEXT("Same tag is one status"), Status->GetActiveStatusEffects().Num(), 1);
	const TArray<FARStatusEffectView> Views = Status->GetActiveStatusEffects();
	if (!TestEqual(TEXT("Active status retains its definition"), Views.Num(), 1)) return false;
	TestTrue(TEXT("Transient definition remains valid"), IsValid(Views[0].Definition));
	TestTrue(TEXT("Definition not a saved asset"), Views[0].Definition->HasAnyFlags(RF_Transient));
	TestEqual(TEXT("Source passed through"), Views[0].Source.SourceId, Source.SourceId);
	int32 Removed = 0;
	const FDelegateHandle Subscription = Status->OnStatusRemovedNative.AddLambda([&](AActor*, const FARStatusEffectView& View)
	{
		++Removed;
		TestTrue(TEXT("Removal broadcasts exact handle"), View.Handle == First.Handle);
	});
	// Advance in small frames: UWorld clamps a single large frame's delta.
	for (int32 Frame = 0; Frame < 40; ++Frame)
	{
		++GFrameCounter;
		Scope.World->Tick(LEVELTICK_All, 0.1f);
		Status->TickComponent(0.0f, LEVELTICK_All, nullptr);
	}
	// Expiration uses world time, not a content-owned timer.
	Status->TickComponent(0.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("One expiry notification"), Removed, 1);
	TestTrue(TEXT("Expiry releases movement"), Enemy->GetMovementControlComponent()->CanBasicMove());
	Status->OnStatusRemovedNative.Remove(Subscription);
	bool bAdded = false;
	const FARCCImmunityHandle Immunity = Status->AddCCImmunity(FARCCImmunitySpec(), bAdded);
	TestTrue(TEXT("Immunity added"), bAdded);
	const FResult Blocked = Apply(Enemy, EARCrowdControlType::Stun, 3.0f, false);
	TestTrue(TEXT("Tenacity-off does not bypass immunity"), !Blocked.bSuccess && !Blocked.Handle.IsValid());
	TestEqual(TEXT("Immunity reason"), Blocked.Reason, EARRequestResult::Blocked);
	Status->RemoveCCImmunity(Immunity);
	Status->ImmuneStatusTags.AddTag(ARGameplayTags::Status_Root);
	TestFalse(TEXT("Tag immunity blocks root"), Apply(Enemy, EARCrowdControlType::Root, 2.0f).bSuccess);
	Enemy->GetStatsComponent()->SetBaseStat(EARStatType::Tenacity, 100.0f);
	TestFalse(TEXT("100 tenacity prevents duration"), Apply(Enemy, EARCrowdControlType::Stun, 3.0f).bSuccess);
	TestTrue(TEXT("Tenacity off permits CC at 100 tenacity"), Apply(Enemy, EARCrowdControlType::Stun, 3.0f, false).bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCrowdControlInputTest, "AR.Foundation.Status.CrowdControl.InvalidInputsAndPins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARCrowdControlInputTest::RunTest(const FString& Parameters)
{
	using namespace ARCrowdControlTests;
	FWorldScope Scope;
	AARBaseEnemy* Enemy = Scope.Spawn();
	const FResult Missing = Apply(nullptr, EARCrowdControlType::Stun, 1.0f);
	TestTrue(TEXT("Null target resets all outputs"), !Missing.bSuccess && !Missing.Handle.IsValid() && Missing.Duration == 0.0f);
	TestEqual(TEXT("Null target reason"), Missing.Reason, EARRequestResult::InvalidTarget);
	AActor* PlainActor = Scope.World->SpawnActor<AActor>();
	TestFalse(TEXT("Actor without status component rejected"), Apply(PlainActor, EARCrowdControlType::Stun, 1.0f).bSuccess);
	for (float Duration : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
	{
		const FResult Bad = Apply(Enemy, EARCrowdControlType::Stun, Duration);
		TestTrue(TEXT("Bad duration resets outputs"), !Bad.bSuccess && !Bad.Handle.IsValid() && Bad.Duration == 0.0f);
		TestEqual(TEXT("Bad duration reason"), Bad.Reason, EARRequestResult::InvalidDefinition);
	}
	TestFalse(TEXT("Unknown enum rejected"), Apply(Enemy, static_cast<EARCrowdControlType>(255), 1.0f).bSuccess);
	TestTrue(TEXT("No status created on failure"), Enemy->GetStatusEffectComponent()->GetActiveStatusEffects().IsEmpty());
	UFunction* Function = UARCombatBlueprintLibrary::StaticClass()->FindFunctionByName(TEXT("ApplyCrowdControl"));
	if (!TestNotNull(TEXT("Blueprint node is reflected"), Function)) return false;
	TestTrue(TEXT("Blueprint callable"), Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestEqual(TEXT("Exact search name"), Function->GetMetaData(TEXT("DisplayName")), FString(TEXT("Apply Crowd Control")));
	TestEqual(TEXT("Default tenacity is on"), Function->GetMetaData(TEXT("CPP_Default_bAffectedByTenacity")), FString(TEXT("true")));
	TestEqual(TEXT("Default duration"), Function->GetMetaData(TEXT("CPP_Default_Duration")), FString(TEXT("1.0")));
	for (FName Pin : {TEXT("Target"), TEXT("CCType"), TEXT("Duration"), TEXT("bAffectedByTenacity"), TEXT("bSuccess"), TEXT("ReturnValue")})
	{
		TestNotNull(*FString::Printf(TEXT("Pin %s exists"), *Pin.ToString()), Function->FindPropertyByName(Pin));
	}
	return true;
}

#endif
