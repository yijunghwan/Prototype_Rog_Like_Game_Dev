#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "InputActionValue.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UObject/Script.h"
#include "Foundation/UI/ARResourceHUDWidget.h"
#include "Foundation/Actions/ARActionTypes.h"
#include "Foundation/AI/ARAIController.h"
#include "Foundation/Blueprint/ARResourceBlueprintLibrary.h"
#include "Foundation/Blueprint/ARStatsBlueprintLibrary.h"
#include "Foundation/Blueprint/ARItemCatalogBlueprintLibrary.h"
#include "Foundation/Blueprint/ARCombatBlueprintLibrary.h"
#include <limits>
#include "Foundation/Characters/ARBaseEnemy.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Combat/ARCombatSubsystem.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARConsumableComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Components/ARLoadoutComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARMovementControlComponent.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Components/ARUIManagerComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "Foundation/Items/ARItemInstance.h"
#include "Foundation/Items/ARConsumableInstance.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include "Foundation/Interaction/ARItemPickupActors.h"
#include "Foundation/Interfaces/ARCombatTargetInterface.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"

namespace ARFoundationTests
{
	struct FItemSkillEventRecord
	{
		TWeakObjectPtr<UARLoadoutItemInstance> Instance;
		FName SkillId;
		FARActionHandle Handle;
		EARActionCancelReason Reason = EARActionCancelReason::None;
	};

	// Test-only native thunks observe the real reflected BP events without creating or saving test assets.
	struct FScopedItemSkillEventRecorder;
	static FScopedItemSkillEventRecorder* ActiveItemSkillRecorder = nullptr;
	struct FScopedItemSkillEventRecorder
	{
		FScopedItemSkillEventRecorder()
		{
			check(!ActiveItemSkillRecorder);
			ExecuteFunction = UARLoadoutItemInstance::StaticClass()->FindFunctionByName(TEXT("ExecuteItemSkill"));
			CancelFunction = UARLoadoutItemInstance::StaticClass()->FindFunctionByName(TEXT("ReceiveItemSkillCancelled"));
			check(ExecuteFunction && CancelFunction);
			ExecuteFlags = ExecuteFunction->FunctionFlags;
			CancelFlags = CancelFunction->FunctionFlags;
			ExecuteNative = ExecuteFunction->GetNativeFunc();
			CancelNative = CancelFunction->GetNativeFunc();
			ActiveItemSkillRecorder = this;
			ExecuteFunction->FunctionFlags |= FUNC_Native;
			CancelFunction->FunctionFlags |= FUNC_Native;
			ExecuteFunction->SetNativeFunc(&RecordExecute);
			CancelFunction->SetNativeFunc(&RecordCancel);
		}

		~FScopedItemSkillEventRecorder()
		{
			ExecuteFunction->SetNativeFunc(ExecuteNative);
			CancelFunction->SetNativeFunc(CancelNative);
			ExecuteFunction->FunctionFlags = ExecuteFlags;
			CancelFunction->FunctionFlags = CancelFlags;
			ActiveItemSkillRecorder = nullptr;
		}

		static void RecordExecute(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_PROPERTY(FNameProperty, SkillId);
			P_GET_STRUCT(FARActionHandle, ActionHandle);
			P_FINISH;
			UARLoadoutItemInstance* Instance = CastChecked<UARLoadoutItemInstance>(Context);
			FItemSkillEventRecord Record;
			Record.Instance = Instance;
			Record.SkillId = SkillId;
			Record.Handle = ActionHandle;
			ActiveItemSkillRecorder->Executions.Add(Record);
			if (ActiveItemSkillRecorder->OnExecute) ActiveItemSkillRecorder->OnExecute(Record);
		}

		static void RecordCancel(UObject* Context, FFrame& Stack, RESULT_DECL)
		{
			P_GET_PROPERTY(FNameProperty, SkillId);
			P_GET_STRUCT(FARActionHandle, ActionHandle);
			P_GET_ENUM(EARActionCancelReason, Reason);
			P_FINISH;
			FItemSkillEventRecord Record;
			Record.Instance = CastChecked<UARLoadoutItemInstance>(Context);
			Record.SkillId = SkillId;
			Record.Handle = ActionHandle;
			Record.Reason = Reason;
			ActiveItemSkillRecorder->Cancellations.Add(Record);
			if (ActiveItemSkillRecorder->OnCancel) ActiveItemSkillRecorder->OnCancel(Record);
		}

		TArray<FItemSkillEventRecord> Executions;
		TArray<FItemSkillEventRecord> Cancellations;
		TFunction<void(const FItemSkillEventRecord&)> OnExecute;
		TFunction<void(const FItemSkillEventRecord&)> OnCancel;
		UFunction* ExecuteFunction = nullptr;
		UFunction* CancelFunction = nullptr;
		EFunctionFlags ExecuteFlags = FUNC_None;
		EFunctionFlags CancelFlags = FUNC_None;
		FNativeFuncPtr ExecuteNative = nullptr;
		FNativeFuncPtr CancelNative = nullptr;
	};

	struct FScopedTestWorld
	{
		FScopedTestWorld()
		{
			const FName WorldName(*FString::Printf(TEXT("ARAutomationWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
			World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
			if (World)
			{
				World->AddToRoot();
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FScopedTestWorld()
		{
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
				World->RemoveFromRoot();
			}
		}

		UWorld* World = nullptr;
	};

	AActor* MakeActorWithComponents(UWorld* World, UARStatsComponent*& Stats, UARActionComponent*& Action,
		UARStaggerComponent*& Stagger, UARStatusEffectComponent*& Status)
	{
		AActor* Actor = World ? World->SpawnActor<AActor>() : nullptr;
		if (!Actor)
		{
			return nullptr;
		}
		Stats = NewObject<UARStatsComponent>(Actor, TEXT("TestStats"));
		Action = NewObject<UARActionComponent>(Actor, TEXT("TestAction"));
		Stagger = NewObject<UARStaggerComponent>(Actor, TEXT("TestStagger"));
		Status = NewObject<UARStatusEffectComponent>(Actor, TEXT("TestStatus"));
		Actor->AddInstanceComponent(Stats);
		Actor->AddInstanceComponent(Action);
		Actor->AddInstanceComponent(Stagger);
		Actor->AddInstanceComponent(Status);
		Stats->RegisterComponent();
		Action->RegisterComponent();
		Stagger->RegisterComponent();
		Status->RegisterComponent();
		Stats->BeginPlay();
		Action->BeginPlay();
		Stagger->BeginPlay();
		Status->BeginPlay();
		return Actor;
	}

	void BeginPlayerSkillSystems(AARPlayerCharacter* Player)
	{
		Player->GetStatsComponent()->BeginPlay();
		Player->GetManaComponent()->BeginPlay();
		Player->GetStaminaComponent()->BeginPlay();
		Player->GetActionComponent()->BeginPlay();
		Player->GetLoadoutComponent()->BeginPlay();
	}

	FARSkillDefinition MakeSkill(FName SkillId, FGameplayTag InputTag, int32 Priority, float ManaCost)
	{
		FARSkillDefinition Skill;
		Skill.SkillId = SkillId;
		Skill.InputTag = InputTag;
		Skill.InputPriority = Priority;
		Skill.ResourceCost.Mana = ManaCost;
		return Skill;
	}

	UARItemDefinition* MakeItem(FGameplayTag TypeTag, TSubclassOf<UARItemInstance> RuntimeClass, FName Name = NAME_None)
	{
		static int32 NextTestItemId = 1;
		UARItemDefinition* Definition = NewObject<UARItemDefinition>(GetTransientPackage(), Name);
		Definition->ItemTypeTag = TypeTag;
		Definition->ItemId = NextTestItemId++;
		Definition->RuntimeBehaviorClass = RuntimeClass;
		return Definition;
	}

	void BeginCombatActor(AARBaseCharacter* Character, float MaxHealth)
	{
		Character->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, MaxHealth);
		Character->GetStatsComponent()->BeginPlay();
		Character->GetHealthComponent()->BeginPlay();
	}

	FARDamageOverTimeSpec MakeDotSpec(AActor* Attacker, AActor* Target, FName DotName, EARDotStackPolicy StackPolicy)
	{
		FARDamageOverTimeSpec Spec;
		Spec.DamageRequest.Attacker = Attacker;
		Spec.DamageRequest.Target = Target;
		Spec.DamageRequest.BaseDamage = 10.0f;
		Spec.DamageRequest.bCanCrit = false;
		Spec.DamageRequest.bApplyAmplification = false;
		Spec.DamageRequest.bIgnoreDefense = true;
		Spec.Duration = 1.0f;
		Spec.TickInterval = 0.25f;
		Spec.DotName = DotName;
		Spec.StackPolicy = StackPolicy;
		return Spec;
	}

	void AdvanceWorld(UWorld* World, float DeltaSeconds)
	{
		float Remaining = DeltaSeconds;
		while (Remaining > KINDA_SMALL_NUMBER)
		{
			const float Step = FMath::Min(0.25f, Remaining);
			World->Tick(LEVELTICK_All, Step);
			Remaining -= Step;
		}
	}

	void AdvanceDotTime(UWorld* World, UARCombatSubsystem* Combat, float DeltaSeconds)
	{
		// UWorld clamps a single large test tick. Advance its clock in safe steps, then
		// tick the DOT scheduler once so all elapsed intervals must be caught up together.
		float Remaining = DeltaSeconds;
		while (Remaining > KINDA_SMALL_NUMBER)
		{
			const float Step = FMath::Min(0.25f, Remaining);
			World->Tick(LEVELTICK_All, Step);
			Remaining -= Step;
		}
		Combat->Tick(DeltaSeconds);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARActionOwnedCleanupTest,
	"AR.Foundation.Action.OwnedCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARActionOwnedCleanupTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	TestNotNull(TEXT("Test world created"), TestWorld.World);
	TestNotNull(TEXT("Test actor created"), ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status));
	Stats->SetBaseStat(EARStatType::AttackPower, 100.0f);

	FARActionRequest Request;
	Request.CancelRules.bCancelOnStagger = false;
	FARRequestStatus StartStatus;
	const FARActionHandle Handle = Action->TryStartAction(Request, StartStatus);
	TestTrue(TEXT("Action starts"), StartStatus.IsSuccess() && Handle.IsValid());

	FARStatModifierSpec Modifier;
	Modifier.StatType = EARStatType::AttackPower;
	Modifier.Operation = EARStatModifierOperation::Flat;
	Modifier.Value = 50.0f;
	Modifier.Duration = -1.0f;
	bool bApplied = false;
	Action->ApplyActionStatModifier(Handle, Modifier, bApplied);
	TestTrue(TEXT("Action-owned modifier applies"), bApplied);
	TestEqual(TEXT("Modifier affects stat while action is active"), Stats->GetFinalStat(EARStatType::AttackPower), 150.0f);
	TestEqual(TEXT("Disallowed stagger cancellation leaves action active"), Action->CancelActionsByReason(EARActionCancelReason::Stagger), 0);
	TestTrue(TEXT("Action remains active"), Action->IsActionActive(Handle));
	TestTrue(TEXT("Manual cancellation succeeds"), Action->CancelAction(Handle, EARActionCancelReason::Manual));
	TestEqual(TEXT("Action cleanup removes owned modifier"), Stats->GetFinalStat(EARStatType::AttackPower), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARActionTagFreeLifecycleTest,
	"AR.Foundation.Action.TagFreeLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARActionTagFreeLifecycleTest::RunTest(const FString& Parameters)
{
	TestNull(TEXT("Action Tag is no longer an authored or Blueprint field"),
		FARActionRequest::StaticStruct()->FindPropertyByName(TEXT("ActionTag")));
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	if (!TestNotNull(TEXT("Test actor created"),
		ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status))) return false;

	FARActionRequest First;
	First.CancelRules.bCancelOnStagger = false;
	FARActionRequest Second;
	FARRequestStatus StartStatus;
	TestTrue(TEXT("Tag-free action passes the precheck"), Action->CanStartAction(First).IsSuccess());
	const FARActionHandle FirstHandle = Action->TryStartAction(First, StartStatus);
	TestTrue(TEXT("First tag-free action starts"), StartStatus.IsSuccess() && FirstHandle.IsValid());
	const FARActionHandle SecondHandle = Action->TryStartAction(Second, StartStatus);
	TestTrue(TEXT("Second tag-free action starts independently"), StartStatus.IsSuccess() && SecondHandle.IsValid());
	TestFalse(TEXT("Handles distinguish the two actions without a tag"), FirstHandle == SecondHandle);
	TestEqual(TEXT("Cancellation still follows each action's own rules"),
		Action->CancelActionsByReason(EARActionCancelReason::Stagger), 1);
	TestTrue(TEXT("First action survives stagger cancellation"), Action->IsActionActive(FirstHandle));
	TestFalse(TEXT("Second action is cancelled"), Action->IsActionActive(SecondHandle));
	TestTrue(TEXT("Remaining action ends by handle"), Action->EndAction(FirstHandle));
	TestEqual(TEXT("No actions remain"), Action->GetActiveActionCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStaggerGroggyRulesTest,
	"AR.Foundation.Combat.StaggerGroggyRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARStaggerGroggyRulesTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	AActor* Target = ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status);
	TestNotNull(TEXT("Test actor created"), Target);
	Stats->SetBaseStat(EARStatType::StaggerResistance, 10.0f);
	Stats->SetBaseStat(EARStatType::MaxGroggy, 100.0f);
	Stagger->bUseGroggyGauge = true;
	Stagger->ResetGroggyGauge();

	FARStaggerRequest Request;
	Request.HitContext.HitId = FGuid::NewGuid();
	Request.HitContext.Target = Target;
	Request.HitContext.bValidHit = true;
	Request.Template.BaseStaggerDamage = 10.0f;
	Request.Template.BaseGroggyDamage = 40.0f;
	FARStaggerResult Result = Stagger->ApplyStaggerAndGroggyDamage(Request);
	TestFalse(TEXT("Stagger damage equal to resistance does not stagger"), Result.bStaggered);
	TestEqual(TEXT("Groggy damage is independent from stagger success"), Result.CurrentGroggy, 60.0f);

	Request.HitContext.HitId = FGuid::NewGuid();
	Request.Template.BaseStaggerDamage = 11.0f;
	Request.Template.BaseGroggyDamage = 60.0f;
	Result = Stagger->ApplyStaggerAndGroggyDamage(Request);
	TestTrue(TEXT("Stagger damage above resistance staggers"), Result.bStaggered);
	TestTrue(TEXT("Groggy reaches zero and emits depletion result"), Result.bGroggyDepleted);
	TestEqual(TEXT("Groggy is clamped to zero"), Result.CurrentGroggy, 0.0f);

	bool bArmorAdded = false;
	FARSuperArmorSpec Armor;
	Armor.Duration = -1.0f;
	const FARSuperArmorHandle ArmorHandle = Stagger->AddSuperArmor(Armor, bArmorAdded);
	Stagger->ResetGroggyGauge();
	Request.HitContext.HitId = FGuid::NewGuid();
	Result = Stagger->ApplyStaggerAndGroggyDamage(Request);
	TestTrue(TEXT("Super armor is added"), bArmorAdded && ArmorHandle.IsValid());
	TestTrue(TEXT("Super armor blocks stagger"), Result.bBlockedBySuperArmor && !Result.bStaggered);
	TestEqual(TEXT("Super armor does not block groggy damage"), Result.CurrentGroggy, 40.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStaggerFromDamageResultTest,
	"AR.Foundation.Combat.StaggerFromDamageResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARStaggerFromDamageResultTest::RunTest(const FString& Parameters)
{
	const UFunction* Function = UARCombatBlueprintLibrary::StaticClass()->FindFunctionByName(TEXT("ApplyStaggerAndGroggyDamageFromResult"));
	if (!TestNotNull(TEXT("Result-based node is reflected for Blueprint"), Function)) return false;
	TestTrue(TEXT("Node has execution pins"), Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) && !Function->HasAnyFunctionFlags(FUNC_BlueprintPure));
	TestEqual(TEXT("Default base stagger is zero"), Function->GetMetaData(TEXT("CPP_Default_BaseStaggerDamage")), FString(TEXT("0.0")));
	TestEqual(TEXT("Default stagger multiplier is one"), Function->GetMetaData(TEXT("CPP_Default_StaggerMultiplier")), FString(TEXT("1.0")));
	TestEqual(TEXT("Default base groggy is zero"), Function->GetMetaData(TEXT("CPP_Default_BaseGroggyDamage")), FString(TEXT("0.0")));
	TestEqual(TEXT("Unconnected source is allowed"), Function->GetMetaData(TEXT("AutoCreateRefTerm")), FString(TEXT("Source")));

	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	if (!TestNotNull(TEXT("Attacker spawned"), Attacker) || !TestNotNull(TEXT("Target spawned"), Target)) return false;
	ARFoundationTests::BeginCombatActor(Attacker, 100.0f);
	ARFoundationTests::BeginCombatActor(Target, 1000.0f);
	Attacker->GetStatsComponent()->SetBaseStat(EARStatType::StaggerPower, 100.0f);
	Attacker->GetStatsComponent()->SetBaseStat(EARStatType::GroggyDamageAmplification, 50.0f);
	Target->GetStatsComponent()->SetBaseStat(EARStatType::StaggerResistance, 44.0f);
	Target->GetStatsComponent()->SetBaseStat(EARStatType::MaxGroggy, 100.0f);
	UARStaggerComponent* Stagger = Target->GetStaggerComponent();
	Stagger->bUseGroggyGauge = true;
	Stagger->BeginPlay();
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	FARCombatDamageRequest Request = ARFoundationTests::MakeDotSpec(Attacker, Target, NAME_None, EARDotStackPolicy::Independent).DamageRequest;
	Request.DamageName = TEXT("ResultHit");
	const FARCombatDamageResult Damage = UARCombatBlueprintLibrary::ApplyCombatDamage(Attacker, Request);
	TestTrue(TEXT("Real direct hit supplies a usable context"), Damage.WasApplied() && Damage.HitContext.IsUsable());
	FARSourceInfo Source;
	Source.Category = EARModifierSourceCategory::Relic;
	Source.SourceId = TEXT("ResultNodeTest");
	const auto Apply = [&](const FARCombatDamageResult& Input, float BaseStagger = 11.0f, float Multiplier = 2.0f, float BaseGroggy = 20.0f)
	{
		return UARCombatBlueprintLibrary::ApplyStaggerAndGroggyDamageFromResult(Input, BaseStagger, Multiplier, BaseGroggy, Source, TEXT("ResultEffect"));
	};
	const float HealthAfterHit = Target->GetHealthComponent()->GetCurrentHealth();
	TestTrue(TEXT("Zero defaults still accept a valid hit"), Apply(Damage, 0.0f, 1.0f, 0.0f).bValidRequest);
	TestEqual(TEXT("Zero defaults do not reduce groggy"), Stagger->GetCurrentGroggy(), 100.0f);

	// Reject all non-applied outcomes even if a caller retained a usable context.
	for (const EARDamageOutcome Outcome : { EARDamageOutcome::Invalid, EARDamageOutcome::Queued, EARDamageOutcome::Evaded, EARDamageOutcome::Blocked })
	{
		FARCombatDamageResult Rejected = Damage;
		Rejected.Outcome = Outcome;
		TestFalse(TEXT("Non-applied result is a no-op"), Apply(Rejected).bValidRequest);
	}
	TestFalse(TEXT("Default result is a no-op"), Apply(FARCombatDamageResult()).bValidRequest);
	FARCombatDamageResult InvalidHit = Damage;
	InvalidHit.HitContext.bValidHit = false;
	TestFalse(TEXT("Applied without a valid hit is rejected"), Apply(InvalidHit).bValidRequest);
	InvalidHit = Damage; InvalidHit.HitContext.HitId.Invalidate();
	TestFalse(TEXT("Applied without a hit ID is rejected"), Apply(InvalidHit).bValidRequest);
	InvalidHit = Damage; InvalidHit.HitContext.Target = nullptr;
	TestFalse(TEXT("Applied without a target is rejected"), Apply(InvalidHit).bValidRequest);
	AActor* NoComponent = TestWorld.World->SpawnActor<AActor>();
	InvalidHit.HitContext.Target = NoComponent;
	TestFalse(TEXT("Target without a stagger component is safe"), Apply(InvalidHit).bValidRequest);
	NoComponent->Destroy();
	TestFalse(TEXT("Destroyed target is safe"), Apply(InvalidHit).bValidRequest);
	const FARCombatDamageResult Repeat = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("Named repeat is blocked"), Repeat.Outcome, EARDamageOutcome::Blocked);
	TestFalse(TEXT("Named repeat cannot deal stagger or groggy"), Apply(Repeat).bValidRequest);
	TestEqual(TEXT("Rejected requests leave groggy unchanged"), Stagger->GetCurrentGroggy(), 100.0f);
	TestFalse(TEXT("Rejected requests do not stagger"), Stagger->IsStaggered());

	// Later stat changes must not alter the captured hit's offensive snapshot.
	Attacker->GetStatsComponent()->SetBaseStat(EARStatType::StaggerPower, 0.0f);
	Attacker->GetStatsComponent()->SetBaseStat(EARStatType::GroggyDamageAmplification, 0.0f);
	FARStaggerResult Result = Apply(Damage);
	TestTrue(TEXT("Applied result is processed without a BP branch"), Result.bValidRequest);
	TestEqual(TEXT("Stagger uses captured power and multiplier"), Result.FinalStaggerDamage, 44);
	TestFalse(TEXT("Equal resistance does not stagger"), Result.bStaggered);
	TestEqual(TEXT("Groggy uses captured power and amplification but not stagger multiplier"), Result.FinalGroggyDamage, 60);
	TestEqual(TEXT("Groggy gauge is reduced"), Result.CurrentGroggy, 40.0f);
	TestEqual(TEXT("Result node does not apply HP damage again"), Target->GetHealthComponent()->GetCurrentHealth(), HealthAfterHit);
	Stagger->ResetGroggyGauge();
	bool bArmorAdded = false;
	FARSuperArmorSpec Armor;
	const FARSuperArmorHandle ArmorHandle = Stagger->AddSuperArmor(Armor, bArmorAdded);
	TestTrue(TEXT("Super armor registered"), bArmorAdded);
	Result = Apply(Damage, 12.0f);
	TestTrue(TEXT("Super armor still blocks stagger"), Result.bBlockedBySuperArmor && !Result.bStaggered);
	TestEqual(TEXT("Super armor does not block groggy"), Result.CurrentGroggy, 40.0f);
	Stagger->RemoveSuperArmor(ArmorHandle);
	Result = Apply(Damage, 12.0f, 2.0f, 0.0f);
	TestTrue(TEXT("Sufficient stagger still applies through the result node"), Result.bStaggered);

	FARShieldSpec Shield;
	Shield.Amount = 20.0f;
	bool bShieldAdded = false;
	Target->GetHealthComponent()->ApplyShield(Shield, bShieldAdded);
	TestTrue(TEXT("Shield registered"), bShieldAdded);
	Request.DamageName = TEXT("ShieldResultHit");
	const FARCombatDamageResult ShieldHit = Combat->ApplyCombatDamage(Request);
	TestTrue(TEXT("Shield-only hit is Applied"), ShieldHit.WasApplied() && ShieldHit.HealthDamage == 0 && ShieldHit.ShieldDamage > 0);
	Stagger->ResetGroggyGauge();
	Result = Apply(ShieldHit, 0.0f, 1.0f, 5.0f);
	TestTrue(TEXT("Shield-only result can apply groggy"), Result.bValidRequest);
	TestEqual(TEXT("Shield-only groggy uses that hit's snapshot"), Result.CurrentGroggy, 95.0f);
	Target->Destroy();
	TestFalse(TEXT("Previously applied result is safe after target destruction"), Apply(ShieldHit).bValidRequest);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARDotStaggerOptionTest,
	"AR.Foundation.Combat.DotStaggerOption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARDotStaggerOptionTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	if (!TestNotNull(TEXT("Attacker spawned"), Attacker) || !TestNotNull(TEXT("Target spawned"), Target)) return false;
	ARFoundationTests::BeginCombatActor(Attacker, 100.0f);
	ARFoundationTests::BeginCombatActor(Target, 1000.0f);
	Target->GetStatsComponent()->SetBaseStat(EARStatType::MaxGroggy, 100.0f);
	UARStaggerComponent* Stagger = Target->GetStaggerComponent();
	Stagger->bUseGroggyGauge = true;
	Stagger->BeginPlay();
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	FARDamageOverTimeSpec Spec = ARFoundationTests::MakeDotSpec(Attacker, Target, TEXT("GroggyDot"), EARDotStackPolicy::Independent);
	Spec.DamageRequest.DamageName = TEXT("GroggyTick");
	Spec.StaggerTemplate.BaseGroggyDamage = 5.0f;
	bool bSuccess = false;
	EARRequestResult Reason;
	UARCombatBlueprintLibrary::ApplyDamageOverTime(Attacker, Spec, bSuccess, Reason);
	TestTrue(TEXT("DOT with checkbox off registers"), bSuccess);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Unchecked DOT does not apply groggy"), Stagger->GetCurrentGroggy(), 100.0f);
	TestEqual(TEXT("Unchecked DOT still applies HP damage"), Target->GetHealthComponent()->GetCurrentHealth(), 960.0f);
	Spec.bApplyStaggerAndGroggyEachTick = true;
	UARCombatBlueprintLibrary::ApplyDamageOverTime(Attacker, Spec, bSuccess, Reason);
	UARCombatBlueprintLibrary::ApplyDamageOverTime(Attacker, Spec, bSuccess, Reason);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Checked DOT applies groggy once per successful tick, not blocked parallel ticks"), Stagger->GetCurrentGroggy(), 80.0f);
	TestEqual(TEXT("Parallel same-name DOT shares the hit interval"), Target->GetHealthComponent()->GetCurrentHealth(), 920.0f);
	Spec.DamageRequest.DamageNameInterval = 0.5f;
	UARCombatBlueprintLibrary::ApplyDamageOverTime(Attacker, Spec, bSuccess, Reason);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Long hit interval also suppresses groggy on skipped ticks"), Stagger->GetCurrentGroggy(), 70.0f);
	TestEqual(TEXT("Long hit interval allows two HP hits"), Target->GetHealthComponent()->GetCurrentHealth(), 900.0f);
	Spec.DamageRequest.BaseDamage = 0.0f;
	UARCombatBlueprintLibrary::ApplyDamageOverTime(Attacker, Spec, bSuccess, Reason);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Zero-damage DOT does not create groggy hits"), Stagger->GetCurrentGroggy(), 70.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARAIMovementCCGateTest,
	"AR.Foundation.AI.MovementCCGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARAIMovementCCGateTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARBaseEnemy* Enemy = TestWorld.World->SpawnActor<AARBaseEnemy>();
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARAIController* Controller = TestWorld.World->SpawnActor<AARAIController>();
	if (!TestNotNull(TEXT("Enemy spawned"), Enemy)
		|| !TestNotNull(TEXT("Player spawned"), Player)
		|| !TestNotNull(TEXT("AR AI controller spawned"), Controller))
	{
		return false;
	}
	Enemy->DispatchBeginPlay();
	Player->DispatchBeginPlay();
	TestTrue(TEXT("Enemy BeginPlay ran"), Enemy->HasActorBegunPlay());
	TestTrue(TEXT("Enemy listens for status changes"), Enemy->GetStatusEffectComponent()->OnStatusAddedNative.IsBound());
	TestTrue(TEXT("Enemy listens for stagger changes"), Enemy->GetStaggerComponent()->OnStaggerStateChangedNative.IsBound());
	TestTrue(TEXT("Player listens for status changes"), Player->GetStatusEffectComponent()->OnStatusAddedNative.IsBound());
	Controller->Possess(Enemy);
	TestEqual(TEXT("Enemy uses AR AI controller by default"), Enemy->AIControllerClass.Get(), AARAIController::StaticClass());
	TestTrue(TEXT("Unrestricted enemy can request basic AI movement"), Controller->CanRequestBasicMove());

	UARStatusEffectDefinition* Root = NewObject<UARStatusEffectDefinition>();
	Root->DefinitionTag = ARGameplayTags::Status_Root;
	Root->StatusTag = ARGameplayTags::Status_Root;
	Root->BaseDuration = 2.0f;
	Root->bBlocksBasicMovement = true;
	Root->bBlocksAllMovement = false;
	Root->bBlocksRoll = true;
	Root->bBlocksSkillGroups = false;
	FARStatusEffectRequest StatusRequest;
	StatusRequest.Definition = Root;
	const FARStatusEffectResult EnemyRoot = Enemy->GetStatusEffectComponent()->ApplyStatusEffect(StatusRequest);
	TestEqual(TEXT("Enemy root applies"), EnemyRoot.Result, EARRequestResult::Success);
	TestTrue(TEXT("Root definition blocks basic movement"), Root->bBlocksBasicMovement);
	TestTrue(TEXT("Root is active on enemy"), Enemy->GetStatusEffectComponent()->BlocksBasicMovement());
	TestFalse(TEXT("Root locks enemy basic movement"), Enemy->GetMovementControlComponent()->CanBasicMove());
	TestFalse(TEXT("Root blocks AI path requests"), Controller->CanRequestBasicMove());
	TestEqual(TEXT("AR move wrapper rejects a rooted enemy"), Controller->ARMoveToActor(Player), EPathFollowingRequestResult::Failed);
	TestTrue(TEXT("Root still permits action-owned movement"), Enemy->GetMovementControlComponent()->CanMoveAtAll());
	FARActionRequest AttackAction;
	TestTrue(TEXT("Root still permits a non-roll action"), Enemy->GetActionComponent()->CanStartAction(AttackAction).IsSuccess());
	AttackAction.bIsRollAction = true;
	TestFalse(TEXT("Root blocks roll action"), Enemy->GetActionComponent()->CanStartAction(AttackAction).IsSuccess());
	TestTrue(TEXT("Enemy root can be removed"), Enemy->GetStatusEffectComponent()->RemoveStatusEffect(EnemyRoot.Handle));
	TestTrue(TEXT("AI path requests are allowed again after root"), Controller->CanRequestBasicMove());

	UARStatusEffectDefinition* Stun = NewObject<UARStatusEffectDefinition>();
	Stun->DefinitionTag = ARGameplayTags::Status_Stun;
	Stun->StatusTag = ARGameplayTags::Status_Stun;
	Stun->BaseDuration = 2.0f;
	Stun->bBlocksBasicMovement = true;
	Stun->bBlocksAllMovement = true;
	Stun->bBlocksRoll = true;
	Stun->bBlocksSkillGroups = true;
	Stun->bCancelActionsOnApply = true;
	Stun->ActionCancelReason = EARActionCancelReason::Stun;
	StatusRequest.Definition = Stun;
	const FARStatusEffectResult EnemyStun = Enemy->GetStatusEffectComponent()->ApplyStatusEffect(StatusRequest);
	TestEqual(TEXT("Enemy stun applies"), EnemyStun.Result, EARRequestResult::Success);
	TestFalse(TEXT("Stun blocks AI path requests"), Controller->CanRequestBasicMove());
	TestFalse(TEXT("Stun blocks all enemy movement"), Enemy->GetMovementControlComponent()->CanMoveAtAll());
	AttackAction.bIsRollAction = false;
	TestFalse(TEXT("Stun blocks new enemy actions"), Enemy->GetActionComponent()->CanStartAction(AttackAction).IsSuccess());
	TestTrue(TEXT("Enemy stun can be removed"), Enemy->GetStatusEffectComponent()->RemoveStatusEffect(EnemyStun.Handle));
	TestTrue(TEXT("AI path requests are allowed again after stun"), Controller->CanRequestBasicMove());

	const FARStatusEffectResult PlayerStun = Player->GetStatusEffectComponent()->ApplyStatusEffect(StatusRequest);
	TestEqual(TEXT("Player stun applies through the same status system"), PlayerStun.Result, EARRequestResult::Success);
	TestFalse(TEXT("Player movement is locked by stun"), Player->GetMovementControlComponent()->CanBasicMove());
	TestFalse(TEXT("Player action is blocked by stun"), Player->GetActionComponent()->CanStartAction(AttackAction).IsSuccess());
	TestTrue(TEXT("Player stun can be removed"), Player->GetStatusEffectComponent()->RemoveStatusEffect(PlayerStun.Handle));
	TestTrue(TEXT("Player movement returns after stun"), Player->GetMovementControlComponent()->CanBasicMove());

	FARStaggerRequest StaggerRequest;
	StaggerRequest.HitContext.HitId = FGuid::NewGuid();
	StaggerRequest.HitContext.Target = Enemy;
	StaggerRequest.HitContext.bValidHit = true;
	StaggerRequest.Template.BaseStaggerDamage = Enemy->GetStatsComponent()->GetFinalStat(EARStatType::StaggerResistance) + 1.0f;
	TestTrue(TEXT("Enemy staggers on sufficient stagger damage"), Enemy->GetStaggerComponent()->ApplyStaggerAndGroggyDamage(StaggerRequest).bStaggered);
	TestFalse(TEXT("Stagger blocks AI path requests"), Controller->CanRequestBasicMove());
	ARFoundationTests::AdvanceWorld(TestWorld.World, Enemy->GetStaggerComponent()->BaseStaggerDuration + 0.1f);
	TestTrue(TEXT("AI path requests return after stagger"), Controller->CanRequestBasicMove());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatusTenacityRefreshTest,
	"AR.Foundation.Status.TenacityAndRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARStatusTenacityRefreshTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	TestNotNull(TEXT("Test actor created"), ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status));
	Stats->SetBaseStat(EARStatType::Tenacity, 25.0f);

	UARStatusEffectDefinition* Definition = NewObject<UARStatusEffectDefinition>();
	Definition->DefinitionTag = ARGameplayTags::Status_Stun;
	Definition->StatusTag = ARGameplayTags::Status_Stun;
	Definition->BaseDuration = 4.0f;
	Definition->bAffectedByTenacity = true;

	FARStatusEffectRequest Request;
	Request.Definition = Definition;
	FARStatusEffectResult Result = Status->ApplyStatusEffect(Request);
	TestTrue(TEXT("Status applies"), Result.Result == EARRequestResult::Success);
	TestEqual(TEXT("Tenacity reduces four seconds by 25 percent"), Result.AppliedDuration, 3.0f);

	Request.DurationOverride = 2.0f;
	Result = Status->ApplyStatusEffect(Request);
	TestTrue(TEXT("Same status reports refresh"), Result.bRefreshedExisting);
	float Remaining = 0.0f;
	Status->GetStatusRemainingTime(ARGameplayTags::Status_Stun, Remaining);
	TestTrue(TEXT("Shorter refresh does not shorten existing duration"), Remaining >= 2.99f);
	TestEqual(TEXT("Only one status instance remains"), Status->GetActiveStatusEffects().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatEffectStacksAndTenacityTest,
	"AR.Foundation.Stats.EffectStacksAndTenacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARStatEffectStacksAndTenacityTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	if (!TestNotNull(TEXT("Test actor created"), ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status))) return false;
	Stats->SetBaseStat(EARStatType::Tenacity, 50.0f);
	FARStatModifierSpec Effect;
	Effect.StatType = EARStatType::AttackPower;
	Effect.Value = 10.0f;
	Effect.Duration = 4.0f;
	Effect.bAffectedByTenacity = true;
	Effect.Source.Category = EARModifierSourceCategory::Buff;
	Effect.Source.SourceId = TEXT("Test.PowerBuff");
	Effect.Source.DisplayName = FText::FromString(TEXT("Power buff"));
	Effect.HUDDisplay = EARStatEffectDisplay::Buff;
	bool bApplied = false;
	const FARStatModifierHandle First = Stats->AddStatModifier(Effect, bApplied);
	TestTrue(TEXT("First timed stack applies"), bApplied);
	bool bPermanent = false;
	float Remaining = 0.0f;
	Stats->GetModifierRemainingTime(First, bPermanent, Remaining);
	TestTrue(TEXT("Tenacity halves four-second modifier"), FMath::IsNearlyEqual(Remaining, 2.0f, 0.01f));
	FARStatModifierSpec SecondStat = Effect;
	SecondStat.StatType = EARStatType::SpellPower;
	SecondStat.Value = 5.0f;
	SecondStat.StackGroupHandle = First;
	const FARStatModifierHandle Grouped = Stats->AddStatModifier(SecondStat, bApplied);
	TestTrue(TEXT("Second stat joins first stack"), bApplied);
	TestEqual(TEXT("Two stat changes count as one stack"), Stats->GetModifiersBySource(EARModifierSourceCategory::Buff, Effect.Source.SourceId).StackCount, 1);
	TestEqual(TEXT("Grouped spell bonus applies"), Stats->GetFinalStat(EARStatType::SpellPower), 5.0f);
	const FARStatModifierHandle AnotherStack = Stats->AddStatModifier(Effect, bApplied);
	TestTrue(TEXT("Another application independently stacks"), bApplied);
	TestEqual(TEXT("Two applications count as two stacks"), Stats->GetModifiersBySource(EARModifierSourceCategory::Buff, Effect.Source.SourceId).StackCount, 2);
	TestEqual(TEXT("Two attack bonuses both apply"), Stats->GetFinalStat(EARStatType::AttackPower), 20.0f);
	TestEqual(TEXT("Same source is one HUD icon"), Stats->GetVisibleStatEffects().Num(), 1);
	TestEqual(TEXT("HUD icon carries two stacks"), Stats->GetVisibleStatEffects()[0].StackCount, 2);
	FARStatModifierHandle Removed;
	TestTrue(TEXT("Removing one stack removes its whole group"), Stats->RemoveOneModifierStack(EARModifierSourceCategory::Buff, Effect.Source.SourceId, EARModifierStackRemovalPolicy::Oldest, Removed));
	TestEqual(TEXT("Grouped spell bonus is gone"), Stats->GetFinalStat(EARStatType::SpellPower), 0.0f);
	TestEqual(TEXT("One attack bonus remains"), Stats->GetFinalStat(EARStatType::AttackPower), 10.0f);
	TestEqual(TEXT("One stack remains"), Stats->GetModifiersBySource(EARModifierSourceCategory::Buff, Effect.Source.SourceId).StackCount, 1);
	FARStatModifierSpec Counter = Effect;
	Counter.Source.SourceId = TEXT("Test.Counter");
	Counter.bStackOnly = true;
	Counter.Duration = -1.0f;
	Counter.Value = 1000.0f;
	Stats->AddStatModifier(Counter, bApplied);
	TestTrue(TEXT("Stack-only application succeeds"), bApplied);
	Stats->AddStatModifier(Counter, bApplied);
	TestEqual(TEXT("Pure counter stores two stacks"), Stats->GetModifiersBySource(EARModifierSourceCategory::Buff, Counter.Source.SourceId).StackCount, 2);
	TestEqual(TEXT("Pure counter does not change attack power"), Stats->GetFinalStat(EARStatType::AttackPower), 10.0f);
	Stats->SetBaseStat(EARStatType::Tenacity, 100.0f);
	Stats->AddStatModifier(Effect, bApplied);
	TestFalse(TEXT("Full tenacity blocks a checked finite effect"), bApplied);
	TestEqual(TEXT("Blocked effect adds no stack"), Stats->GetModifiersBySource(EARModifierSourceCategory::Buff, Effect.Source.SourceId).StackCount, 1);
	TestTrue(TEXT("Remaining attack stack can be removed"), Stats->RemoveStatModifier(AnotherStack));
	TestFalse(TEXT("Grouped handle was removed with its stack"), Stats->RemoveStatModifier(Grouped));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARStatStackBatchRemovalTest,
	"AR.Foundation.Stats.StackBatchRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARStatStackBatchRemovalTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	AActor* Target = ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status);
	if (!TestNotNull(TEXT("Test actor created"), Target)) return false;
	FARStatModifierSpec Spec;
	Spec.Source.Category = EARModifierSourceCategory::Relic;
	Spec.Source.SourceId = TEXT("Test.Batch");
	Spec.StatType = EARStatType::AttackPower;
	Spec.Value = 10.0f;
	Spec.Duration = -1.0f;
	bool Applied = false;
	const FARStatModifierHandle First = Stats->AddStatModifier(Spec, Applied);
	FARStatModifierSpec Grouped = Spec;
	Grouped.StatType = EARStatType::SpellPower;
	Grouped.Value = 7.0f;
	Grouped.StackGroupHandle = First;
	const FARStatModifierHandle FirstSpell = Stats->AddStatModifier(Grouped, Applied);
	Spec.Value = 20.0f;
	const FARStatModifierHandle Second = Stats->AddStatModifier(Spec, Applied);
	Spec.Value = 30.0f;
	const FARStatModifierHandle Third = Stats->AddStatModifier(Spec, Applied);
	int32 Removed = 99;
	TestFalse(TEXT("Full request exceeding stack count fails"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 4, Removed));
	TestEqual(TEXT("Failed full request reports zero"), Removed, 0);
	TestEqual(TEXT("Failure leaves all stat changes intact"), Stats->GetFinalStat(EARStatType::AttackPower), 60.0f);
	TestEqual(TEXT("Failure leaves grouped spell effect intact"), Stats->GetFinalStat(EARStatType::SpellPower), 7.0f);
	TestTrue(TEXT("Remove two oldest stacks, including ties at identical application time"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 2, Removed));
	TestEqual(TEXT("Counts two stack groups, not three modifiers"), Removed, 2);
	TestEqual(TEXT("Newest attack bonus remains"), Stats->GetFinalStat(EARStatType::AttackPower), 30.0f);
	TestEqual(TEXT("Grouped spell effect is removed with first stack"), Stats->GetFinalStat(EARStatType::SpellPower), 0.0f);
	TestFalse(TEXT("Oldest first handle gone"), Stats->RemoveStatModifier(First));
	TestFalse(TEXT("Grouped handle gone"), Stats->RemoveStatModifier(FirstSpell));
	TestFalse(TEXT("Second oldest handle gone"), Stats->RemoveStatModifier(Second));
	TestTrue(TEXT("Newest surviving handle removes individually"), Stats->RemoveStatModifier(Third));
	Spec.bStackOnly = true;
	const FARStatModifierHandle OldCounter = Stats->AddStatModifier(Spec, Applied);
	const FARStatModifierHandle NewCounter = Stats->AddStatModifier(Spec, Applied);
	TestTrue(TEXT("Remove newest counter by explicit policy"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 1, Removed, EARModifierStackRemovalPolicy::Newest));
	TestFalse(TEXT("Newest counter was selected"), Stats->RemoveStatModifier(NewCounter));
	FARStatModifierSpec Other = Spec;
	Other.Source.Category = EARModifierSourceCategory::Buff;
	const FARStatModifierHandle OtherCategory = Stats->AddStatModifier(Other, Applied);
	Other.Source.SourceId = TEXT("Test.Other");
	const FARStatModifierHandle OtherId = Stats->AddStatModifier(Other, Applied);
	TestFalse(TEXT("Zero count rejected"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 0, Removed));
	TestFalse(TEXT("Negative count rejected"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, -2, Removed));
	TestFalse(TEXT("None source cannot accidentally consume all sources"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, NAME_None, 1, Removed));
	TestTrue(TEXT("Partial mode removes available matching stacks"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 100, Removed, EARModifierStackRemovalPolicy::Oldest, false));
	TestEqual(TEXT("Partial mode reports actual count"), Removed, 1);
	TestFalse(TEXT("Old counter now gone"), Stats->RemoveStatModifier(OldCounter));
	TestTrue(TEXT("Same id in different category untouched"), Stats->RemoveStatModifier(OtherCategory));
	TestTrue(TEXT("Different id untouched"), Stats->RemoveStatModifier(OtherId));
	TestFalse(TEXT("No matching stack fails and zeroes output"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 1, Removed));
	TestEqual(TEXT("Empty result count zero"), Removed, 0);
	Removed = 99;
	TestFalse(TEXT("Null target safely fails"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(nullptr, Spec.Source.Category, Spec.Source.SourceId, 1, Removed));
	TestEqual(TEXT("Null target resets count"), Removed, 0);
	Spec.Duration = 0.01f;
	Stats->AddStatModifier(Spec, Applied);
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.1f);
	TestFalse(TEXT("Expired stack is not consumed"), UARStatsBlueprintLibrary::RemoveStatModifierStacks(Target, Spec.Source.Category, Spec.Source.SourceId, 1, Removed));
	Spec.Duration = -1.0f;
	Stats->AddStatModifier(Spec, Applied);
	TArray<FARStatModifierHandle> Handles;
	TestTrue(TEXT("Component batch API also works"), Stats->RemoveModifierStacks(Spec.Source.Category, Spec.Source.SourceId, 1, Removed, Handles));
	TestEqual(TEXT("Component returns removed handles"), Handles.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAROverallTakenStatIntegrationTest,
	"AR.Foundation.Stats.OverallTakenIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAROverallTakenStatIntegrationTest::RunTest(const FString& Parameters)
{
	// Stable saved IDs; the new 46 is displayed before physical vulnerability (27).
	static_assert(static_cast<uint8>(EARStatType::PhysicalDamageTakenIncrease) == 27);
	static_assert(static_cast<uint8>(EARStatType::FireDamageTakenIncrease) == 28);
	static_assert(static_cast<uint8>(EARStatType::MagicDamageTakenIncrease) == 29);
	static_assert(static_cast<uint8>(EARStatType::Tenacity) == 30);
	static_assert(static_cast<uint8>(EARStatType::CooldownReduction) == 45);
	static_assert(static_cast<uint8>(EARStatType::OverallDamageTakenIncrease) == 46);
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	if (!TestNotNull(TEXT("Attacker"), Attacker) || !TestNotNull(TEXT("Target"), Target)) return false;
	ARFoundationTests::BeginCombatActor(Attacker, 100);
	ARFoundationTests::BeginCombatActor(Target, 10000);
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	if (!TestNotNull(TEXT("Combat subsystem"), Combat)) return false;
	UARStatsComponent* Stats = Target->GetStatsComponent();
	TestEqual(TEXT("Default overall vulnerability is zero"), Stats->GetFinalStat(EARStatType::OverallDamageTakenIncrease), 0.0f);
	const TArray<FARFinalStatView> Views = Stats->GetAllFinalStatViews();
	const int32 OverallIndex = Views.IndexOfByPredicate([](const FARFinalStatView& View) { return View.StatType == EARStatType::OverallDamageTakenIncrease; });
	const int32 PhysicalIndex = Views.IndexOfByPredicate([](const FARFinalStatView& View) { return View.StatType == EARStatType::PhysicalDamageTakenIncrease; });
	TestEqual(TEXT("Character sheet includes 47 distinct stats"), Views.Num(), 47);
	TestTrue(TEXT("Overall vulnerability appears immediately before physical vulnerability"), OverallIndex >= 0 && PhysicalIndex == OverallIndex + 1);
	Stats->SetBaseStat(EARStatType::OverallDamageTakenIncrease, -50);
	TestEqual(TEXT("Overall vulnerability follows nonnegative attribute-stat rules"), Stats->GetFinalStat(EARStatType::OverallDamageTakenIncrease), 0.0f);
	Stats->SetBaseStat(EARStatType::OverallDamageTakenIncrease, 0);
	FARStatModifierSpec Effect;
	Effect.StatType = EARStatType::OverallDamageTakenIncrease;
	Effect.Operation = EARStatModifierOperation::Flat;
	Effect.Value = 50;
	Effect.Duration = -1;
	bool Applied = false;
	const FARStatModifierHandle Handle = Stats->AddStatModifier(Effect, Applied);
	TestTrue(TEXT("New stat accepts existing modifier nodes"), Applied);
	FARCombatDamageRequest Request;
	Request.Attacker = Attacker;
	Request.Target = Target;
	Request.BaseDamage = 100;
	Request.bCanCrit = false;
	Request.bGuaranteedHit = true;
	for (const auto Pair : { TPair<EARDamageAttribute, EARStatType>(EARDamageAttribute::Physical, EARStatType::PhysicalDamageTakenIncrease),
		TPair<EARDamageAttribute, EARStatType>(EARDamageAttribute::Fire, EARStatType::FireDamageTakenIncrease),
		TPair<EARDamageAttribute, EARStatType>(EARDamageAttribute::Magic, EARStatType::MagicDamageTakenIncrease) })
	{
		Stats->SetBaseStat(Pair.Value, 20);
		Request.Attribute = Pair.Key;
		for (EARDamageDelivery Delivery : { EARDamageDelivery::Direct, EARDamageDelivery::DamageOverTime })
		{
			Request.Delivery = Delivery;
			TestEqual(TEXT("Real combat captures both incoming stats for direct/DOT"), Combat->ApplyCombatDamage(Request).FinalDamage, 170);
		}
	}
	TestTrue(TEXT("New modifier removes through existing handle node"), Stats->RemoveStatModifier(Handle));
	TestEqual(TEXT("Removing modifier restores previous damage"), Combat->ApplyCombatDamage(Request).FinalDamage, 120);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARLoadoutDropAtomicTest,
	"AR.Foundation.Items.LoadoutDropAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARLoadoutDropAtomicTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	Loadout->BeginPlay();

	UARItemDefinition* Definition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	const FARLoadoutAcquisitionResult Begin = Loadout->BeginLoadoutAcquisition(Definition);
	FARRequestStatus CommitStatus;
	UARLoadoutItemInstance* Instance = Loadout->CommitLoadoutAcquisition(Begin.Token, CommitStatus);
	TestTrue(TEXT("Passive relic acquisition succeeds"), Instance && CommitStatus.IsSuccess());

	FARLoadoutDropRequest DropRequest;
	FARRequestStatus DropStatus;
	const bool bDropped = Loadout->DiscardLoadoutItem(Instance->GetInstanceId(), DropRequest, DropStatus);
	TestTrue(TEXT("Pickup spawn and inventory removal commit together"), bDropped && DropStatus.IsSuccess());
	TestNotNull(TEXT("Drop returns spawned pickup"), DropRequest.SpawnedPickup.Get());
	TestEqual(TEXT("Successful drop removes relic from inventory"), Loadout->GetLoadoutInventory().Num(), 0);
	TestTrue(TEXT("Spawned pickup keeps the same definition"), DropRequest.SpawnedPickup && DropRequest.SpawnedPickup->ItemDefinition == Definition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARUnifiedItemIdentityTest,
	"AR.Foundation.Items.UnifiedIdentityAndRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARUnifiedItemIdentityTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	ARFoundationTests::BeginPlayerSkillSystems(Player);
	Player->GetConsumableComponent()->BeginPlay();

	UARItemDefinition* Passive = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* Active = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* Consumable = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_Consumable, UARConsumableInstance::StaticClass());
	// The same number is allowed across different types; the composite key identifies each one.
	Passive->ItemId = 42;
	Active->ItemId = 42;
	Consumable->ItemId = 42;
	Passive->AdditionalTags = {TEXT("Fire"), TEXT("Stamina")};
	Active->AdditionalTags = {TEXT("Fire")};
	Consumable->AdditionalTags = {TEXT("Fire")};
	TestTrue(TEXT("Free-form label lookup trims and ignores case"), Passive->HasAdditionalTag(TEXT(" fire ")));
	TestFalse(TEXT("Unrelated label does not match"), Passive->HasAdditionalTag(TEXT("Ice")));
	TestTrue(TEXT("Common acquisition routes a passive relic"),
		UARItemCatalogBlueprintLibrary::TryAcquireItem(Player, Passive).IsSuccess());
	TestTrue(TEXT("Common acquisition routes an active relic"),
		UARItemCatalogBlueprintLibrary::TryAcquireItem(Player, Active).IsSuccess());
	TestTrue(TEXT("Common acquisition routes a consumable"),
		UARItemCatalogBlueprintLibrary::TryAcquireItem(Player, Consumable).IsSuccess());
	TestEqual(TEXT("Owned Fire count includes all item instances"),
		UARItemCatalogBlueprintLibrary::CountOwnedItemsByAdditionalTag(Player, TEXT("Fire"), FGameplayTag()), 3);
	TestEqual(TEXT("Type filter counts only passive relic copies"),
		UARItemCatalogBlueprintLibrary::CountOwnedItemsByAdditionalTag(Player, TEXT("Fire"), ARGameplayTags::Item_Type_PassiveRelic), 1);
	TestTrue(TEXT("A second copy is acquired independently"),
		UARItemCatalogBlueprintLibrary::TryAcquireItem(Player, Passive).IsSuccess());
	TestEqual(TEXT("Duplicate passive copies are counted twice"),
		UARItemCatalogBlueprintLibrary::CountOwnedItemsByAdditionalTag(Player, TEXT("Fire"), ARGameplayTags::Item_Type_PassiveRelic), 2);

	UARItemDefinition* Invalid = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARConsumableInstance::StaticClass());
	TestFalse(TEXT("Wrong runtime family is rejected"),
		UARItemCatalogBlueprintLibrary::TryAcquireItem(Player, Invalid).IsSuccess());
	Invalid->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
	Invalid->ItemId = 0;
	TestFalse(TEXT("Unassigned numeric ID is rejected"),
		UARItemCatalogBlueprintLibrary::TryAcquireItem(Player, Invalid).IsSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARItemOwnedGuaranteeCleanupTest,
	"AR.Foundation.Items.OwnedGuaranteeCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARItemOwnedGuaranteeCleanupTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	Player->GetStatsComponent()->BeginPlay();
	Player->GetStaggerComponent()->BeginPlay();
	Player->GetStatusEffectComponent()->BeginPlay();

	UARItemDefinition* FirstDefinition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* SecondDefinition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	FARStatModifierSpec DefaultAttack;
	DefaultAttack.Source.SourceId = TEXT("Test.DefaultGroup");
	DefaultAttack.HUDDisplay = EARStatEffectDisplay::Buff;
	DefaultAttack.StatType = EARStatType::AttackPower;
	DefaultAttack.Value = 2.0f;
	FARStatModifierSpec DefaultSpell = DefaultAttack;
	DefaultSpell.StatType = EARStatType::SpellPower;
	DefaultSpell.Value = 3.0f;
	FirstDefinition->DefaultStatModifiers.Add(DefaultAttack);
	FirstDefinition->DefaultStatModifiers.Add(DefaultSpell);
	UARLoadoutItemInstance* First = NewObject<UARLoadoutItemInstance>(Player);
	UARLoadoutItemInstance* Second = NewObject<UARLoadoutItemInstance>(Player);
	First->InitializeInstance(Player, FirstDefinition);
	Second->InitializeInstance(Player, SecondDefinition);
	TestTrue(TEXT("Both item instances register"), First->RegisterItem() && Second->RegisterItem());
	TestEqual(TEXT("Default modifiers sharing Source Id form one stack"),
		Player->GetStatsComponent()->GetModifiersBySource(EARModifierSourceCategory::Relic, DefaultAttack.Source.SourceId).StackCount, 1);
	FARStatModifierSpec OwnedStack;
	OwnedStack.bStackOnly = true;
	OwnedStack.Source.SourceId = TEXT("Test.SharedRelicStack");
	OwnedStack.HUDDisplay = EARStatEffectDisplay::Buff;
	bool bFirstStack = false, bSecondStack = false;
	First->ApplyItemStatModifier(OwnedStack, bFirstStack);
	Second->ApplyItemStatModifier(OwnedStack, bSecondStack);
	TestTrue(TEXT("Both relic stacks apply under explicit source ID"), bFirstStack && bSecondStack);
	TestEqual(TEXT("Explicit item source ID is queryable"),
		Player->GetStatsComponent()->GetModifiersBySource(EARModifierSourceCategory::Relic, OwnedStack.Source.SourceId).StackCount, 2);

	FARSuperArmorSpec Armor;
	Armor.Duration = -1.0f;
	FARCCImmunitySpec Immunity;
	Immunity.Duration = -1.0f;
	bool bFirstArmor = false, bSecondArmor = false, bFirstImmunity = false, bSecondImmunity = false;
	First->ApplyItemSuperArmor(Armor, bFirstArmor);
	Second->ApplyItemSuperArmor(Armor, bSecondArmor);
	First->ApplyItemCCImmunity(Immunity, bFirstImmunity);
	Second->ApplyItemCCImmunity(Immunity, bSecondImmunity);
	TestTrue(TEXT("Both items grant their own guarantees"), bFirstArmor && bSecondArmor && bFirstImmunity && bSecondImmunity);
	First->UnregisterItem(EARItemRemovalReason::Manual);
	TestEqual(TEXT("First item removes only its own stack"),
		Player->GetStatsComponent()->GetModifiersBySource(EARModifierSourceCategory::Relic, OwnedStack.Source.SourceId).StackCount, 1);
	TestTrue(TEXT("Second item keeps super armor after first is removed"), Player->GetStaggerComponent()->IsSuperArmorActive());
	TestTrue(TEXT("Second item keeps CC immunity after first is removed"), Player->GetStatusEffectComponent()->IsCCImmunityActive());
	Second->UnregisterItem(EARItemRemovalReason::Manual);
	TestEqual(TEXT("Second item clears final stack"),
		Player->GetStatsComponent()->GetModifiersBySource(EARModifierSourceCategory::Relic, OwnedStack.Source.SourceId).StackCount, 0);
	TestFalse(TEXT("Final item removal clears super armor"), Player->GetStaggerComponent()->IsSuperArmorActive());
	TestFalse(TEXT("Final item removal clears CC immunity"), Player->GetStatusEffectComponent()->IsCCImmunityActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARCCImmunityGuaranteeTest,
	"AR.Foundation.Status.CCImmunityGuarantee",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARCCImmunityGuaranteeTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	UARStatsComponent* Stats = nullptr;
	UARActionComponent* Action = nullptr;
	UARStaggerComponent* Stagger = nullptr;
	UARStatusEffectComponent* Status = nullptr;
	AActor* Target = ARFoundationTests::MakeActorWithComponents(TestWorld.World, Stats, Action, Stagger, Status);
	if (!TestNotNull(TEXT("Target created"), Target)) return false;

	UARStatusEffectDefinition* Stun = NewObject<UARStatusEffectDefinition>();
	Stun->StatusTag = ARGameplayTags::Status_Stun;
	Stun->BaseDuration = 4.0f;
	Stun->bAffectedByTenacity = false;
	Stun->bCanBeImmune = false;
	FARStatusEffectRequest Request;
	Request.Definition = Stun;

	FARCCImmunitySpec Spec;
	Spec.Duration = -1.0f;
	bool bAddedFirst = false;
	bool bAddedSecond = false;
	const FARCCImmunityHandle First = Status->AddCCImmunity(Spec, bAddedFirst);
	const FARCCImmunityHandle Second = Status->AddCCImmunity(Spec, bAddedSecond);
	TestTrue(TEXT("Independent CC immunity handles are granted"), bAddedFirst && bAddedSecond && First.IsValid() && Second.IsValid());
	TestEqual(TEXT("CC immunity rejects stun even when tenacity and ordinary immunity are bypassed"),
		Status->ApplyStatusEffect(Request).Result, EARRequestResult::Blocked);
	TestFalse(TEXT("Blocked CC never enters the active status list"), Status->HasStatus(ARGameplayTags::Status_Stun));
	TestFalse(TEXT("Blocked CC does not lock movement"), Status->BlocksBasicMovement());
	TestTrue(TEXT("Removing one handle succeeds"), Status->RemoveCCImmunity(First));
	TestTrue(TEXT("Second handle keeps the guarantee active"), Status->IsCCImmunityActive());
	TestEqual(TEXT("Stun remains blocked by the second handle"), Status->ApplyStatusEffect(Request).Result, EARRequestResult::Blocked);
	TestTrue(TEXT("Removing the final handle succeeds"), Status->RemoveCCImmunity(Second));
	TestFalse(TEXT("Guarantee ends with final handle"), Status->IsCCImmunityActive());
	Spec.Duration = 0.0f;
	bool bInvalidAdded = true;
	TestFalse(TEXT("Zero duration is invalid"), Status->AddCCImmunity(Spec, bInvalidAdded).IsValid());
	TestFalse(TEXT("Zero duration reports failure"), bInvalidAdded);

	Spec.Duration = 0.5f;
	bool bTimedAdded = false;
	Status->AddCCImmunity(Spec, bTimedAdded);
	TestTrue(TEXT("Timed guarantee starts"), bTimedAdded && Status->IsCCImmunityActive());
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.75f);
	TestFalse(TEXT("Timed guarantee expires"), Status->IsCCImmunityActive());
	TestEqual(TEXT("Stun applies normally after immunity expires"), Status->ApplyStatusEffect(Request).Result, EARRequestResult::Success);
	Status->ClearAllStatusEffects();

	// A valid non-CC tag serves as a sentinel; real non-CC definitions use their own status tags.
	UARStatusEffectDefinition* NonCC = NewObject<UARStatusEffectDefinition>();
	NonCC->StatusTag = ARGameplayTags::Damage_Attribute_Fire;
	NonCC->BaseDuration = 2.0f;
	Spec.Duration = -1.0f;
	bool bNonCCGuardAdded = false;
	const FARCCImmunityHandle Guard = Status->AddCCImmunity(Spec, bNonCCGuardAdded);
	TestTrue(TEXT("Guard for non-CC test is active"), bNonCCGuardAdded && Guard.IsValid());
	Request.Definition = NonCC;
	TestEqual(TEXT("CC immunity does not block an ordinary status"), Status->ApplyStatusEffect(Request).Result, EARRequestResult::Success);
	Status->ClearAllStatusEffects();
	NonCC->bIsCrowdControl = true;
	TestEqual(TEXT("Custom status marked as CC is blocked"), Status->ApplyStatusEffect(Request).Result, EARRequestResult::Blocked);
	Status->RemoveCCImmunity(Guard);
	Spec.Source.Category = EARModifierSourceCategory::Relic;
	Spec.Source.SourceId = FName(TEXT("OneRelicInstance"));
	bool bSourceAdded = false;
	Status->AddCCImmunity(Spec, bSourceAdded);
	TestTrue(TEXT("Source-owned immunity applies"), bSourceAdded && Status->IsCCImmunityActive());
	TestEqual(TEXT("Source removal only removes the matching immunity"),
		Status->RemoveCCImmunityBySource(EARModifierSourceCategory::Relic, FName(TEXT("OneRelicInstance"))), 1);
	TestFalse(TEXT("Source removal clears its final guarantee"), Status->IsCCImmunityActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARConsumableDropAtomicTest,
	"AR.Foundation.Items.ConsumableDropAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARConsumableDropAtomicTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	UARConsumableComponent* Consumables = Player->GetConsumableComponent();
	Consumables->BeginPlay();

	UARItemDefinition* Definition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_Consumable, UARConsumableInstance::StaticClass());
	const FARConsumableAcquisitionResult Acquire = Consumables->TryAcquireConsumable(Definition);
	TestTrue(TEXT("Consumable acquisition succeeds"), Acquire.Status.IsSuccess());

	FARConsumableDropRequest DropRequest;
	const FARRequestStatus DropStatus = Consumables->DropConsumableSlot(Acquire.SlotIndex, DropRequest);
	TestTrue(TEXT("Pickup spawn and slot removal commit together"), DropStatus.IsSuccess());
	TestNotNull(TEXT("Drop returns spawned pickup"), DropRequest.SpawnedPickup.Get());
	const TArray<FARConsumableSlotSnapshot> Slots = Consumables->GetConsumableSlots();
	TestTrue(TEXT("Successful drop clears the consumable slot"), Slots.IsValidIndex(Acquire.SlotIndex) && !Slots[Acquire.SlotIndex].bOccupied);
	TestTrue(TEXT("Spawned pickup keeps the same definition"), DropRequest.SpawnedPickup && DropRequest.SpawnedPickup->ConsumableDefinition == Definition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARLoadoutAcquisitionRevisionTest,
	"AR.Foundation.Items.LoadoutAcquisitionRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARLoadoutAcquisitionRevisionTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	Loadout->BeginPlay();

	UARItemDefinition* FirstDefinition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* SecondDefinition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());

	const FARLoadoutAcquisitionResult FirstRequest = Loadout->BeginLoadoutAcquisition(FirstDefinition);
	const FARLoadoutAcquisitionResult SecondRequest = Loadout->BeginLoadoutAcquisition(SecondDefinition);
	TestTrue(TEXT("Both requests are initially valid"), FirstRequest.Status.IsSuccess() && SecondRequest.Status.IsSuccess());

	FARRequestStatus SecondCommitStatus;
	TestNotNull(TEXT("Second request commits"), Loadout->CommitLoadoutAcquisition(SecondRequest.Token, SecondCommitStatus));
	FARRequestStatus StaleCommitStatus;
	TestNull(TEXT("Older request is rejected after the inventory revision changes"), Loadout->CommitLoadoutAcquisition(FirstRequest.Token, StaleCommitStatus));
	TestEqual(TEXT("Older request reports stale token"), StaleCommitStatus.Result, EARRequestResult::StaleRequest);
	TestEqual(TEXT("Only the committed relic exists"), Loadout->GetLoadoutInventory().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARSkillPriorityTransactionTest,
	"AR.Foundation.Items.SkillPriorityTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARSkillPriorityTransactionTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	ARFoundationTests::BeginPlayerSkillSystems(Player);

	UARItemDefinition* Definition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	Definition->DisplayName = FText::FromString(TEXT("Transaction Relic"));
	FARStatModifierSpec ProvidedAttackPower;
	ProvidedAttackPower.StatType = EARStatType::AttackPower;
	ProvidedAttackPower.Operation = EARStatModifierOperation::Flat;
	ProvidedAttackPower.Value = 5.0f;
	Definition->DefaultStatModifiers.Add(ProvidedAttackPower);
	Definition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("PrimaryA"), ARGameplayTags::Input_Skill_Primary, 0, 20.0f));
	Definition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("PrimaryB"), ARGameplayTags::Input_Skill_Primary, 0, 20.0f));
	Definition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("PrimaryExpensive"), ARGameplayTags::Input_Skill_Primary, 1, 70.0f));
	Definition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("Secondary"), ARGameplayTags::Input_Skill_1, 0, 0.0f));

	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	const FARLoadoutAcquisitionResult Begin = Loadout->BeginLoadoutAcquisition(Definition);
	FARRequestStatus CommitStatus;
	UARLoadoutItemInstance* RegisteredInstance = Loadout->CommitLoadoutAcquisition(Begin.Token, CommitStatus);
	TestNotNull(TEXT("Skill relic registers"), RegisteredInstance);
	FARLoadoutItemDisplayData DisplayData;
	TestTrue(TEXT("Inventory display data is available by stable instance id"),
		Loadout->GetLoadoutItemDisplayData(RegisteredInstance->GetInstanceId(), DisplayData));
	TestEqual(TEXT("Display data contains the definition-provided stat"), DisplayData.ProvidedStats.Num(), 1);
	TestEqual(TEXT("Display data contains every declared skill"), DisplayData.Skills.Num(), 4);
	TestFalse(TEXT("Auto-generated stat text is not empty"), DisplayData.ProvidedStats[0].DisplayText.IsEmpty());

	FARSkillGroupHandle PrimaryGroup;
	const FARRequestStatus PrimaryStatus = Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, PrimaryGroup);
	TestTrue(TEXT("Affordable first priority group executes"), PrimaryStatus.IsSuccess() && PrimaryGroup.IsValid());
	TestEqual(TEXT("Both same-priority skills start atomically"), Player->GetActionComponent()->GetActiveActionCount(), 2);
	TestEqual(TEXT("Only the accepted priority group spends mana"), Player->GetManaComponent()->GetCurrent(), 60.0f);

	FARSkillGroupHandle DuplicatePrimaryGroup;
	const FARRequestStatus DuplicatePrimaryStatus = Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, DuplicatePrimaryGroup);
	TestEqual(TEXT("An active input group cannot overlap itself"), DuplicatePrimaryStatus.Result, EARRequestResult::Blocked);
	TestFalse(TEXT("Rejected duplicate group has no handle"), DuplicatePrimaryGroup.IsValid());

	FARSkillGroupHandle SecondaryGroup;
	const FARRequestStatus SecondaryStatus = Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_1, SecondaryGroup);
	TestEqual(TEXT("A different input group cannot interrupt an active group"), SecondaryStatus.Result, EARRequestResult::Blocked);
	TestFalse(TEXT("Rejected secondary group has no handle"), SecondaryGroup.IsValid());
	TestEqual(TEXT("Rejected input leaves the primary actions intact"), Player->GetActionComponent()->GetActiveActionCount(), 2);
	TestEqual(TEXT("Rejected input does not spend mana"), Player->GetManaComponent()->GetCurrent(), 60.0f);
	Player->GetActionComponent()->CancelAllActions();
	TestTrue(TEXT("Another group starts after the previous group finishes"),
		Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_1, SecondaryGroup).IsSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARActiveRelicSlotInputTest,
	"AR.Foundation.Items.ActiveRelicSlotInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARActiveRelicSlotInputTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	ARFoundationTests::BeginPlayerSkillSystems(Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();

	UARItemDefinition* First = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* Second = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	FARSkillDefinition FirstSkill = ARFoundationTests::MakeSkill(TEXT("SlotSkillA"), ARGameplayTags::Input_Skill_1, 0, 10.0f);
	FirstSkill.InputMode = EARSkillInputMode::ActiveRelicSlot;
	FirstSkill.InputTag = FGameplayTag();
	FirstSkill.SlotSkillIndex = 1;
	First->SkillDefinitions.Add(FirstSkill);
	FARSkillDefinition SecondSkill = ARFoundationTests::MakeSkill(TEXT("SlotSkillB"), ARGameplayTags::Input_Skill_1, 0, 20.0f);
	SecondSkill.InputMode = EARSkillInputMode::ActiveRelicSlot;
	SecondSkill.InputTag = FGameplayTag();
	SecondSkill.SlotSkillIndex = 1;
	Second->SkillDefinitions.Add(SecondSkill);

	const FARLoadoutAcquisitionResult FirstRequest = Loadout->BeginLoadoutAcquisition(First);
	FARRequestStatus Status;
	UARLoadoutItemInstance* FirstInstance = Loadout->CommitLoadoutAcquisition(FirstRequest.Token, Status);
	TestTrue(TEXT("First active relic equips"), FirstInstance && Status.IsSuccess());
	const FARLoadoutAcquisitionResult SecondRequest = Loadout->BeginLoadoutAcquisition(Second);
	UARLoadoutItemInstance* SecondInstance = Loadout->CommitLoadoutAcquisition(SecondRequest.Token, Status);
	TestTrue(TEXT("Second active relic equips"), SecondInstance && Status.IsSuccess());

	FARSkillGroupHandle Group;
	TestEqual(TEXT("Slot skills are excluded from the legacy direct-tag route"),
		Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_1, Group).Result, EARRequestResult::InvalidDefinition);
	TestTrue(TEXT("Slot one starts its own relic skill"), Loadout->HandleActiveRelicSlotInput(1, 1, Group).IsSuccess());
	TestEqual(TEXT("Slot one spends only its own mana"), Player->GetManaComponent()->GetCurrent(), 90.0f);
	TestEqual(TEXT("Only one action starts"), Player->GetActionComponent()->GetActiveActionCount(), 1);
	Player->GetActionComponent()->CancelAllActions();

	TestTrue(TEXT("Slot two starts its own relic skill"), Loadout->HandleActiveRelicSlotInput(2, 1, Group).IsSuccess());
	TestEqual(TEXT("Slot two spends only its own mana"), Player->GetManaComponent()->GetCurrent(), 70.0f);
	Player->GetActionComponent()->CancelAllActions();
	const TArray<FARRegisteredSkillUIData> SkillUI = Loadout->GetRegisteredSkillUIData();
	TestEqual(TEXT("Both skills remain in UI data"), SkillUI.Num(), 2);
	if (SkillUI.Num() == 2)
	{
		TestEqual(TEXT("First skill resolves slot one"), SkillUI[0].ActiveRelicSlot, 1);
		TestEqual(TEXT("Second skill resolves slot two"), SkillUI[1].ActiveRelicSlot, 2);
		TestEqual(TEXT("Slot mode is explicit in UI data"), SkillUI[0].InputMode, EARSkillInputMode::ActiveRelicSlot);
		TestFalse(TEXT("Legacy tag is not exposed for slot-routed UI"), SkillUI[0].InputTag.IsValid());
	}

	FARLoadoutDropRequest Drop;
	TestTrue(TEXT("First relic can be discarded"), Loadout->DiscardLoadoutItem(FirstInstance->GetInstanceId(), Drop, Status));
	TestEqual(TEXT("Remaining relic follows the slot position"), Loadout->GetActiveRelics().Num(), 1);
	TestTrue(TEXT("Second relic now occupies slot one"), Loadout->GetActiveRelics()[0] == SecondInstance);
	TestTrue(TEXT("Its skill follows the new slot"), Loadout->HandleActiveRelicSlotInput(1, 1, Group).IsSuccess());
	TestEqual(TEXT("Moved relic spends its own cost"), Player->GetManaComponent()->GetCurrent(), 50.0f);
	TestEqual(TEXT("Empty slot two is rejected"), Loadout->HandleActiveRelicSlotInput(2, 1, Group).Result, EARRequestResult::InvalidHandle);

	UARItemDefinition* InvalidWeapon = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_Weapon, UARLoadoutItemInstance::StaticClass());
	InvalidWeapon->SkillDefinitions.Add(FirstSkill);
	TestEqual(TEXT("Slot input mode is reserved for active relics"),
		Loadout->BeginLoadoutAcquisition(InvalidWeapon).Status.Result, EARRequestResult::InvalidDefinition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARSkillTagFreeCooldownTest,
	"AR.Foundation.Items.TagFreeSkillCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARSkillTagFreeCooldownTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	ARFoundationTests::BeginPlayerSkillSystems(Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	UARItemDefinition* Definition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	FARSkillDefinition Skill = ARFoundationTests::MakeSkill(TEXT("TagFree"), ARGameplayTags::Input_Skill_Primary, 0, 10.0f);
	Skill.BaseCooldown = 1.0f;
	Definition->SkillDefinitions.Add(Skill);
	const FARLoadoutAcquisitionResult Acquisition = Loadout->BeginLoadoutAcquisition(Definition);
	TestTrue(TEXT("Tag-free skill definition is accepted"), Acquisition.Status.IsSuccess());
	FARRequestStatus Status;
	if (!TestNotNull(TEXT("Tag-free relic registers"),
		Loadout->CommitLoadoutAcquisition(Acquisition.Token, Status))) return false;
	FARSkillGroupHandle Group;
	TestTrue(TEXT("Input tag selects the skill without an action tag"),
		Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, Group).IsSuccess());
	TestEqual(TEXT("Successful use spends the configured cost"), Player->GetManaComponent()->GetCurrent(), 90.0f);
	Player->GetActionComponent()->CancelAllActions();
	TestEqual(TEXT("Cancelling the action does not bypass its cooldown"),
		Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, Group).Result, EARRequestResult::Cooldown);
	TestEqual(TEXT("Cooldown rejection does not spend another cost"), Player->GetManaComponent()->GetCurrent(), 90.0f);
	ARFoundationTests::AdvanceWorld(TestWorld.World, 1.25f);
	TestTrue(TEXT("The same skill becomes usable after cooldown"),
		Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, Group).IsSuccess());
	TestEqual(TEXT("Second successful use spends its cost"), Player->GetManaComponent()->GetCurrent(), 80.0f);
	Player->GetActionComponent()->CancelAllActions();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARConsumableSlotReductionTest,
	"AR.Foundation.Items.ConsumableSlotReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARItemSkillCancellationEventTest,
	"AR.Foundation.Items.SkillCancellationEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARItemSkillCancellationEventTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard ScriptGuard;
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player exists"), Player)) return false;
	ARFoundationTests::BeginPlayerSkillSystems(Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	UARActionComponent* Actions = Player->GetActionComponent();
	UFunction* CancelEvent = UARLoadoutItemInstance::StaticClass()->FindFunctionByName(TEXT("ReceiveItemSkillCancelled"));
	if (!TestNotNull(TEXT("Cancellation event is reflected"), CancelEvent)) return false;
	TestTrue(TEXT("Event is available for BP implementation"), CancelEvent->HasAllFunctionFlags(FUNC_Event | FUNC_BlueprintEvent));
	TestFalse(TEXT("Content event does not require a native override"), CancelEvent->HasAnyFunctionFlags(FUNC_Native));
	TestEqual(TEXT("Editor event name"), CancelEvent->GetMetaData(TEXT("DisplayName")), FString(TEXT("On Item Skill Cancelled")));
	ARFoundationTests::FScopedItemSkillEventRecorder Recorder;
	UARItemDefinition* FirstDefinition = ARFoundationTests::MakeItem(ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* SecondDefinition = ARFoundationTests::MakeItem(ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	FirstDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("SharedName"), ARGameplayTags::Input_Skill_Primary, 0, 3.0f));
	FirstDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("Extra"), ARGameplayTags::Input_Skill_Primary, 1, 2.0f));
	SecondDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("SharedName"), ARGameplayTags::Input_Skill_Primary, 0, 3.0f));
	FARRequestStatus Status;
	UARLoadoutItemInstance* First = Loadout->CommitLoadoutAcquisition(Loadout->BeginLoadoutAcquisition(FirstDefinition).Token, Status);
	UARLoadoutItemInstance* Second = Loadout->CommitLoadoutAcquisition(Loadout->BeginLoadoutAcquisition(SecondDefinition).Token, Status);
	if (!TestNotNull(TEXT("First acquired"), First) || !TestNotNull(TEXT("Second acquired"), Second)) return false;
	FARSkillGroupHandle Group;
	TestTrue(TEXT("All three candidates execute"), Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, Group).IsSuccess());
	TestEqual(TEXT("Three execution callbacks"), Recorder.Executions.Num(), 3);
	const ARFoundationTests::FItemSkillEventRecord* FirstRecord = Recorder.Executions.FindByPredicate([First](const auto& Entry)
	{
		return Entry.Instance == First && Entry.SkillId == TEXT("SharedName");
	});
	if (!TestNotNull(TEXT("First execution has its own handle"), FirstRecord)) return false;
	const FARActionHandle FirstHandle = FirstRecord->Handle;
	const float AttackBefore = Player->GetStatsComponent()->GetFinalStat(EARStatType::AttackPower);
	FARStatModifierSpec Modifier;
	Modifier.StatType = EARStatType::AttackPower;
	Modifier.Value = 10.0f;
	bool bApplied = false;
	Actions->ApplyActionStatModifier(FirstHandle, Modifier, bApplied);
	TestTrue(TEXT("Action-owned effect applied"), bApplied);
	FTimerHandle OwnedTimer;
	bool bTimerFired = false;
	TestWorld.World->GetTimerManager().SetTimer(OwnedTimer, FTimerDelegate::CreateLambda([&bTimerFired] { bTimerFired = true; }), 0.1f, false);
	Recorder.OnCancel = [&](const ARFoundationTests::FItemSkillEventRecord& Entry)
	{
		TestFalse(TEXT("Native action is already inactive inside callback"), Actions->IsActionActive(Entry.Handle));
		TestEqual(TEXT("Native action effect is already cleaned up"), Player->GetStatsComponent()->GetFinalStat(EARStatType::AttackPower), AttackBefore);
		TestWorld.World->GetTimerManager().ClearTimer(OwnedTimer);
	};
	TestTrue(TEXT("Cancel one executed skill"), Actions->CancelAction(FirstHandle, EARActionCancelReason::Stagger));
	TestEqual(TEXT("Only one item is notified"), Recorder.Cancellations.Num(), 1);
	if (Recorder.Cancellations.Num() != 1) return false;
	TestTrue(TEXT("Correct owning item despite identical skill names"), Recorder.Cancellations[0].Instance == First);
	TestEqual(TEXT("Original skill ID"), Recorder.Cancellations[0].SkillId, FName(TEXT("SharedName")));
	TestTrue(TEXT("Exact execution handle"), Recorder.Cancellations[0].Handle == FirstHandle);
	TestEqual(TEXT("Cancellation reason is forwarded"), Recorder.Cancellations[0].Reason, EARActionCancelReason::Stagger);
	TestFalse(TEXT("A second cancellation is rejected"), Actions->CancelAction(FirstHandle, EARActionCancelReason::Stun));
	Actions->OnActionCancelled.Broadcast(FirstHandle, EARActionCancelReason::Manual);
	TestEqual(TEXT("Duplicate notification is ignored"), Recorder.Cancellations.Num(), 1);
	for (const auto& Entry : Recorder.Executions)
	{
		if (Entry.Handle != FirstHandle) TestTrue(TEXT("Other executions end normally"), Actions->EndAction(Entry.Handle));
	}
	TestEqual(TEXT("Normal ends do not emit cancellation"), Recorder.Cancellations.Num(), 1);
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.25f);
	TestFalse(TEXT("Content cleanup can clear its custom timer"), bTimerFired);
	TestEqual(TEXT("Cancellation does not refund committed cost"), Player->GetManaComponent()->GetCurrent(), 92.0f);
	FARActionRequest UnrelatedRequest;
	UnrelatedRequest.OwningItemInstanceId = First->GetInstanceId();
	const FARActionHandle Unrelated = Actions->TryStartAction(UnrelatedRequest, Status);
	TestTrue(TEXT("An unexecuted manual action can be cancelled"), Actions->CancelAction(Unrelated, EARActionCancelReason::Manual));
	TestEqual(TEXT("Item ID alone does not fabricate a skill event"), Recorder.Cancellations.Num(), 1);
	Recorder.Executions.Reset();
	Recorder.OnCancel = nullptr;
	TestTrue(TEXT("Group bookkeeping permits another input"), Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, Group).IsSuccess());
	TestEqual(TEXT("Second execution has new handles"), Recorder.Executions.Num(), 3);
	Actions->CancelAllActions(EARActionCancelReason::Death);
	TestEqual(TEXT("Each remaining skill cancelled exactly once"), Recorder.Cancellations.Num(), 4);
	for (int32 Index = 1; Index < Recorder.Cancellations.Num(); ++Index)
	{
		TestEqual(TEXT("Death reason preserved"), Recorder.Cancellations[Index].Reason, EARActionCancelReason::Death);
	}
	TestEqual(TEXT("Unknown input has no skill to execute"), Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_2, Group).Result, EARRequestResult::InvalidDefinition);
	TestEqual(TEXT("Pre-execution rejection emits no cancellation event"), Recorder.Cancellations.Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARItemSkillCancellationReentrancyTest,
	"AR.Foundation.Items.SkillCancellationReentrancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARItemSkillCancellationReentrancyTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard ScriptGuard;
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player exists"), Player)) return false;
	ARFoundationTests::BeginPlayerSkillSystems(Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();
	UARActionComponent* Actions = Player->GetActionComponent();
	ARFoundationTests::FScopedItemSkillEventRecorder Recorder;
	UARItemDefinition* FirstDefinition = ARFoundationTests::MakeItem(ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	UARItemDefinition* SecondDefinition = ARFoundationTests::MakeItem(ARGameplayTags::Item_Type_ActiveRelic, UARLoadoutItemInstance::StaticClass());
	FirstDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("RemoveSelf"), ARGameplayTags::Input_Skill_Primary, 0, 0.0f));
	FirstDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("NotDispatched"), ARGameplayTags::Input_Skill_Primary, 1, 0.0f));
	SecondDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(TEXT("Survivor"), ARGameplayTags::Input_Skill_Primary, 2, 0.0f));
	FARRequestStatus Status;
	UARLoadoutItemInstance* First = Loadout->CommitLoadoutAcquisition(Loadout->BeginLoadoutAcquisition(FirstDefinition).Token, Status);
	UARLoadoutItemInstance* Second = Loadout->CommitLoadoutAcquisition(Loadout->BeginLoadoutAcquisition(SecondDefinition).Token, Status);
	if (!TestNotNull(TEXT("First exists"), First) || !TestNotNull(TEXT("Second exists"), Second)) return false;
	Recorder.OnExecute = [&](const ARFoundationTests::FItemSkillEventRecord& Entry)
	{
		if (Entry.Instance == First)
		{
			FARLoadoutDropRequest Drop;
			TestTrue(TEXT("Item can remove itself inside execution"), Loadout->DiscardLoadoutItem(First->GetInstanceId(), Drop, Status));
		}
	};
	Recorder.OnCancel = [&](const ARFoundationTests::FItemSkillEventRecord& Entry)
	{
		FARSkillGroupHandle NestedGroup;
		TestEqual(TEXT("Removal/teardown callback cannot restart skill dispatch"), Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, NestedGroup).Result, EARRequestResult::Blocked);
		Actions->OnActionCancelled.Broadcast(Entry.Handle, Entry.Reason);
	};
	FARSkillGroupHandle Group;
	TestTrue(TEXT("Initial transaction executes safely"), Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_Primary, Group).IsSuccess());
	TestEqual(TEXT("Only dispatched first skill plus survivor execute"), Recorder.Executions.Num(), 2);
	TestEqual(TEXT("Only dispatched skill receives removal cancellation"), Recorder.Cancellations.Num(), 1);
	if (Recorder.Cancellations.Num() != 1 || Recorder.Executions.Num() != 2) return false;
	TestEqual(TEXT("Correct removal skill"), Recorder.Cancellations[0].SkillId, FName(TEXT("RemoveSelf")));
	TestEqual(TEXT("Removal reason"), Recorder.Cancellations[0].Reason, EARActionCancelReason::ItemRemoved);
	TestFalse(TEXT("First runtime unregistered after callback"), First->IsRegistered());
	TestTrue(TEXT("Other item still executed after registered array mutation"), Recorder.Executions[1].Instance == Second);
	TestTrue(TEXT("Surviving action remains active"), Actions->IsActionActive(Recorder.Executions[1].Handle));
	Loadout->EndPlay(EEndPlayReason::Destroyed);
	TestEqual(TEXT("Owner teardown forwards cancellation before unbinding"), Recorder.Cancellations.Num(), 2);
	TestTrue(TEXT("Teardown notifies surviving item"), Recorder.Cancellations.Last().Instance == Second);
	TestEqual(TEXT("Teardown uses existing item-removal action reason"), Recorder.Cancellations.Last().Reason, EARActionCancelReason::ItemRemoved);
	TestEqual(TEXT("No active actions remain"), Actions->GetActiveActionCount(), 0);
	TestFalse(TEXT("Survivor is unregistered"), Second->IsRegistered());
	return true;
}

bool FARConsumableSlotReductionTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	UARConsumableComponent* Consumables = Player->GetConsumableComponent();
	Consumables->BeginPlay();

	UARItemDefinition* Definition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_Consumable, UARConsumableInstance::StaticClass());
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("Each initial consumable fits in its own slot"), Consumables->TryAcquireConsumable(Definition).Status.IsSuccess());
	}

	Consumables->SetBaseMaxConsumableSlots(1);
	const TArray<FARConsumableSlotSnapshot> ReducedSlots = Consumables->GetConsumableSlots();
	TestEqual(TEXT("Slot capacity is reduced to one"), Consumables->GetMaxConsumableSlots(), 1);
	TestEqual(TEXT("Only the retained slot remains in inventory"), ReducedSlots.Num(), 1);
	TestTrue(TEXT("The first item remains available"), ReducedSlots[0].bOccupied && !ReducedSlots[0].bOverflowSlot);
	int32 SpawnedPickupCount = 0;
	for (TActorIterator<AARConsumablePickup> It(TestWorld.World); It; ++It)
	{
		++SpawnedPickupCount;
	}
	TestEqual(TEXT("Overflow items are preserved as world pickups"), SpawnedPickupCount, 2);

	UARItemDefinition* UsedDefinition = nullptr;
	const FARRequestStatus UseStatus = Consumables->TryUseConsumableSlot(0, UsedDefinition);
	TestTrue(TEXT("Retained consumable can be used"), UseStatus.IsSuccess());
	TestTrue(TEXT("Use returns the consumed definition"), UsedDefinition == Definition);
	TestFalse(TEXT("Used slot becomes empty"), Consumables->GetConsumableSlots()[0].bOccupied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARDamageNameIntervalTest,
	"AR.Foundation.Combat.DamageNameInterval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARDamageNameIntervalTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARPlayerCharacter* OtherAttacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	AARBaseEnemy* OtherTarget = TestWorld.World->SpawnActor<AARBaseEnemy>();
	ARFoundationTests::BeginCombatActor(Attacker, 100.0f);
	ARFoundationTests::BeginCombatActor(OtherAttacker, 100.0f);
	ARFoundationTests::BeginCombatActor(Target, 1000.0f);
	ARFoundationTests::BeginCombatActor(OtherTarget, 1000.0f);
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	FARCombatDamageRequest Request = ARFoundationTests::MakeDotSpec(Attacker, Target, NAME_None, EARDotStackPolicy::Independent).DamageRequest;
	Request.DamageName = TEXT("Spike");
	Request.bGuaranteedHit = true;
	TestEqual(TEXT("Named interval defaults to 0.2 seconds"), Request.DamageNameInterval, 0.2f);
	TestFalse(TEXT("Bypass defaults off"), Request.bIgnoreDamageNameInterval);
	TestTrue(TEXT("Library first named hit applies"), UARCombatBlueprintLibrary::ApplyCombatDamage(Attacker, Request).WasApplied());
	const FARCombatDamageResult Blocked = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("Repeated hit is blocked"), Blocked.Outcome, EARDamageOutcome::Blocked);
	TestEqual(TEXT("Interval reports cooldown distinctly"), Blocked.FailureReason, EARRequestResult::Cooldown);
	TestEqual(TEXT("Blocked hit has zero damage"), Blocked.FinalDamage, 0);
	TestFalse(TEXT("Blocked hit cannot trigger stagger"), Blocked.HitContext.IsUsable());
	TestEqual(TEXT("Blocked hit preserves health"), Target->GetHealthComponent()->GetCurrentHealth(), 990.0f);
	EARRequestResult Reason;
	TestTrue(TEXT("Target-only preflight does not test the interval"), Combat->CanDamageTarget(Attacker, Target, Reason));
	FARCombatDamageRequest Changed = Request;
	Changed.Attacker = OtherAttacker;
	Changed.Attribute = EARDamageAttribute::Void;
	Changed.Delivery = EARDamageDelivery::DamageOverTime;
	Changed.Source.SourceId = TEXT("OtherSpike");
	TestEqual(TEXT("Attacker, attribute, source and delivery share a named window"), Combat->ApplyCombatDamage(Changed).FailureReason, EARRequestResult::Cooldown);
	Changed = Request;
	Changed.Target = OtherTarget;
	TestTrue(TEXT("Different target has independent window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed = Request;
	Changed.DamageName = TEXT("OtherDamage");
	TestTrue(TEXT("Different name has independent window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.1f);
	TestEqual(TEXT("Inside default interval is still blocked"), Combat->ApplyCombatDamage(Request).FailureReason, EARRequestResult::Cooldown);
	Changed = Request;
	Changed.DamageNameInterval = 0.01f;
	TestEqual(TEXT("A shorter new interval cannot shorten the existing window"), Combat->ApplyCombatDamage(Changed).FailureReason, EARRequestResult::Cooldown);
	Changed = Request;
	Changed.bIgnoreDamageNameInterval = true;
	TestTrue(TEXT("Bypass passes an active window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	TestTrue(TEXT("Bypass also permits repeated requests"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed = Request;
	Changed.DamageNameInterval = 0.0f;
	TestTrue(TEXT("Zero disables restriction for this request"), Combat->ApplyCombatDamage(Changed).WasApplied());
	TestEqual(TEXT("Bypass and zero do not clear the stored window"), Combat->ApplyCombatDamage(Request).FailureReason, EARRequestResult::Cooldown);
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.1001f);
	TestTrue(TEXT("Default window expires despite bypass hits"), Combat->ApplyCombatDamage(Request).WasApplied());
	Changed = Request;
	Changed.DamageName = TEXT("LongWindow");
	Changed.DamageNameInterval = 0.5f;
	TestTrue(TEXT("Custom interval first hit applies"), Combat->ApplyCombatDamage(Changed).WasApplied());
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.25f);
	TestEqual(TEXT("Custom half-second interval stays blocked at quarter-second"), Combat->ApplyCombatDamage(Changed).FailureReason, EARRequestResult::Cooldown);
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.251f);
	TestTrue(TEXT("Custom half-second interval expires"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed = Request;
	Changed.DamageName = NAME_None;
	TestTrue(TEXT("Unnamed legacy hit is unrestricted"), Combat->ApplyCombatDamage(Changed).WasApplied());
	TestTrue(TEXT("Unnamed hits do not share an accidental None window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed.DamageName = TEXT("InvalidWindow");
	for (float Invalid : { -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() })
	{
		Changed.DamageNameInterval = Invalid;
		TestEqual(TEXT("Invalid active interval is rejected"), Combat->ApplyCombatDamage(Changed).FailureReason, EARRequestResult::InvalidDefinition);
	}
	Changed.bIgnoreDamageNameInterval = true;
	TestTrue(TEXT("Bypass does not read inactive interval"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed = Request;
	Changed.DamageName = TEXT("FailedHit");
	Changed.BaseDamage = 0.0f;
	TestFalse(TEXT("Zero damage is not applied"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed.BaseDamage = 10.0f;
	TestTrue(TEXT("Zero damage did not consume named window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Changed.DamageName = TEXT("EvadedHit");
	Changed.bGuaranteedHit = false;
	FARStatModifierSpec Guarantee;
	Guarantee.StatType = EARStatType::Evasion;
	Guarantee.bGuaranteeEvasion = true;
	bool bEffectAdded = false;
	const FARStatModifierHandle EvasionHandle = Target->GetStatsComponent()->AddStatModifier(Guarantee, bEffectAdded);
	TestTrue(TEXT("Managed evasion guarantee applies"), bEffectAdded);
	TestEqual(TEXT("Guaranteed evasion prevents this hit"), Combat->ApplyCombatDamage(Changed).Outcome, EARDamageOutcome::Evaded);
	Changed.bGuaranteedHit = true;
	TestTrue(TEXT("Evaded hit did not consume named window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Target->GetStatsComponent()->RemoveStatModifier(EvasionHandle);
	Guarantee.StatType = EARStatType::OverallDamageReduction;
	Guarantee.bGuaranteeEvasion = false;
	Guarantee.bGuaranteeInvulnerability = true;
	const FARStatModifierHandle Invulnerability = Target->GetStatsComponent()->AddStatModifier(Guarantee, bEffectAdded);
	Changed.DamageName = TEXT("InvulnerableHit");
	TestEqual(TEXT("Invulnerability blocks a fresh name"), Combat->ApplyCombatDamage(Changed).Outcome, EARDamageOutcome::Blocked);
	Target->GetStatsComponent()->RemoveStatModifier(Invulnerability);
	TestTrue(TEXT("Invulnerability did not consume a named window"), Combat->ApplyCombatDamage(Changed).WasApplied());
	Request.DamageName = TEXT("ReentrantHit");
	bool bNested = false;
	FARCombatDamageResult NestedResult;
	const FDelegateHandle Delegate = Target->GetHealthComponent()->OnDamageAppliedNative.AddLambda(
		[&](AActor*, const FARCombatDamageResult&)
		{
			if (!bNested) { bNested = true; NestedResult = Combat->ApplyCombatDamage(Request); }
		});
	const float BeforeNested = Target->GetHealthComponent()->GetCurrentHealth();
	TestTrue(TEXT("Outer reentrant hit applies"), Combat->ApplyCombatDamage(Request).WasApplied());
	Target->GetHealthComponent()->OnDamageAppliedNative.Remove(Delegate);
	TestEqual(TEXT("Nested direct request uses existing queue"), NestedResult.Outcome, EARDamageOutcome::Queued);
	TestEqual(TEXT("Queued repeat cannot bypass callback-time window"), Target->GetHealthComponent()->GetCurrentHealth(), BeforeNested - 10.0f);
	Target->Destroy();
	Combat->Tick(0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARDotDamageNameIntervalTest,
	"AR.Foundation.Combat.DotDamageNameInterval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARDotDamageNameIntervalTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	ARFoundationTests::BeginCombatActor(Attacker, 100.0f);
	ARFoundationTests::BeginCombatActor(Target, 1000.0f);
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	FARDamageOverTimeSpec Spec = ARFoundationTests::MakeDotSpec(Attacker, Target, TEXT("IntervalBurn"), EARDotStackPolicy::Independent);
	Spec.DamageRequest.DamageName = TEXT("BurnDamage");
	bool bApplied;
	EARRequestResult Reason;
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	TestTrue(TEXT("First named DOT registers"), bApplied);
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	TestEqual(TEXT("Independent DOT still registers two instances"), Combat->GetActiveDamageOverTimeCount(Target), 2);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Same-name parallel DOTs share interval but preserve four scheduled catch-up ticks"), Target->GetHealthComponent()->GetCurrentHealth(), 960.0f);
	Spec.DamageRequest.bIgnoreDamageNameInterval = true;
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("DOT bypass restores independent ticks from both instances"), Target->GetHealthComponent()->GetCurrentHealth(), 880.0f);
	Spec.DamageRequest.bIgnoreDamageNameInterval = false;
	Spec.DamageRequest.DamageName = TEXT("LongBurn");
	Spec.DamageRequest.DamageNameInterval = 0.5f;
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Custom DOT interval intentionally suppresses alternate quarter-second ticks"), Target->GetHealthComponent()->GetCurrentHealth(), 860.0f);
	Spec.StackPolicy = EARDotStackPolicy::RefreshSameName;
	Spec.DamageRequest.DamageNameInterval = 0.2f;
	const FARDotHandle Handle = Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	TestTrue(TEXT("Refreshable interval DOT registers"), bApplied);
	FARDamageOverTimeSpec Changed = Spec;
	Changed.DamageRequest.DamageNameInterval = 0.5f;
	AddExpectedError(TEXT("Refresh DOT IntervalBurn has mismatched"), EAutomationExpectedErrorFlags::Contains, 3);
	TestFalse(TEXT("Refresh cannot silently change interval"), Combat->ApplyDamageOverTime(Changed, bApplied, Reason).IsValid());
	Changed = Spec; Changed.DamageRequest.bIgnoreDamageNameInterval = true;
	TestFalse(TEXT("Refresh cannot silently change bypass"), Combat->ApplyDamageOverTime(Changed, bApplied, Reason).IsValid());
	Changed = Spec; Changed.DamageRequest.DamageName = TEXT("OtherBurn");
	TestFalse(TEXT("Refresh cannot silently change throttle identity"), Combat->ApplyDamageOverTime(Changed, bApplied, Reason).IsValid());
	Changed = Spec; Changed.DamageRequest.DamageNameInterval = -1.0f;
	TestFalse(TEXT("DOT registration rejects invalid interval"), Combat->ApplyDamageOverTime(Changed, bApplied, Reason).IsValid());
	TestEqual(TEXT("Invalid interval fails before registration"), Reason, EARRequestResult::InvalidDefinition);
	Combat->RemoveDamageOverTime(Handle);
	Spec.StackPolicy = EARDotStackPolicy::Independent;
	Spec.DamageRequest.DamageName = TEXT("SharedDirectDot");
	FARCombatDamageRequest Direct = Spec.DamageRequest;
	Direct.DamageNameInterval = 0.5f;
	TestTrue(TEXT("Direct hit starts shared window"), Combat->ApplyCombatDamage(Direct).WasApplied());
	Spec.Duration = 0.25f;
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 0.25f);
	TestEqual(TEXT("Scheduled DOT inside direct window is suppressed"), Target->GetHealthComponent()->GetCurrentHealth(), 850.0f);
	Spec.DamageRequest.DamageName = TEXT("DetachedAttackerBurn");
	Spec.Duration = 1.0f;
	Combat->ApplyDamageOverTime(Spec, bApplied, Reason);
	Attacker->Destroy();
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Named DOT remains valid after attacker destruction"), Target->GetHealthComponent()->GetCurrentHealth(), 810.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARDotCatchUpTest,
	"AR.Foundation.Combat.DotCatchUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARDotCatchUpTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	TestNotNull(TEXT("Attacker spawned"), Attacker);
	TestNotNull(TEXT("Target spawned"), Target);
	ARFoundationTests::BeginCombatActor(Attacker, 100.0f);
	ARFoundationTests::BeginCombatActor(Target, 1000.0f);
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	TestNotNull(TEXT("Combat subsystem exists"), Combat);
	const IARCombatTargetInterface* AttackerInterface = Cast<IARCombatTargetInterface>(Attacker);
	const IARCombatTargetInterface* TargetInterface = Cast<IARCombatTargetInterface>(Target);
	TestNotNull(TEXT("Attacker exposes native combat target interface"), AttackerInterface);
	TestNotNull(TEXT("Target exposes native combat target interface"), TargetInterface);
	const EARCombatTeam AttackerTeam = AttackerInterface->GetCombatTeam();
	const EARCombatTeam TargetTeam = TargetInterface->GetCombatTeam();
	const bool bTargetable = TargetInterface->CanBeCombatTarget();
	TestTrue(*FString::Printf(TEXT("Attacker uses player combat team (actual %d)"), static_cast<int32>(AttackerTeam)),
		AttackerTeam == EARCombatTeam::Player);
	TestTrue(*FString::Printf(TEXT("Target uses enemy combat team (interface %d, direct %d, valid %d)"),
		static_cast<int32>(TargetTeam), static_cast<int32>(Target->GetCombatTeam()), IsValid(Target) ? 1 : 0),
		TargetTeam == EARCombatTeam::Enemy);
	TestTrue(*FString::Printf(TEXT("Target interface allows combat damage (interface %d, direct %d, health %.1f)"),
		bTargetable ? 1 : 0, Target->CanBeCombatTarget() ? 1 : 0, Target->GetHealthComponent()->GetCurrentHealth()),
		bTargetable);
	EARRequestResult TargetFailureReason = EARRequestResult::Rejected;
	const bool bCanDamage = Combat->CanDamageTarget(Attacker, Target, TargetFailureReason);
	TestTrue(*FString::Printf(TEXT("Player can damage enemy before DOT registration (reason %d)"),
		static_cast<int32>(TargetFailureReason)), bCanDamage);

	bool bApplied = false;
	EARRequestResult FailureReason = EARRequestResult::Rejected;
	const FARDotHandle Handle = Combat->ApplyDamageOverTime(
		ARFoundationTests::MakeDotSpec(Attacker, Target, TEXT("IndependentBurn"), EARDotStackPolicy::Independent),
		bApplied, FailureReason);
	TestTrue(TEXT("DOT registration succeeds"), bApplied && Handle.IsValid() && FailureReason == EARRequestResult::Success);
	TestEqual(TEXT("DOT does not deal an immediate first tick"), Target->GetHealthComponent()->GetCurrentHealth(), 1000.0f);

	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("One delayed frame catches up all four quarter-second ticks"), Target->GetHealthComponent()->GetCurrentHealth(), 960.0f);
	TestEqual(TEXT("Completed DOT is removed after catch-up"), Combat->GetActiveDamageOverTimeCount(Target), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARDotRefreshTest,
	"AR.Foundation.Combat.DotRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARDotRefreshTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Attacker = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	AARBaseEnemy* Target = TestWorld.World->SpawnActor<AARBaseEnemy>();
	TestNotNull(TEXT("Attacker spawned"), Attacker);
	TestNotNull(TEXT("Target spawned"), Target);
	ARFoundationTests::BeginCombatActor(Attacker, 100.0f);
	ARFoundationTests::BeginCombatActor(Target, 1000.0f);
	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	TestNotNull(TEXT("Combat subsystem exists"), Combat);
	EARRequestResult TargetFailureReason = EARRequestResult::Rejected;
	const bool bCanDamage = Combat->CanDamageTarget(Attacker, Target, TargetFailureReason);
	TestTrue(*FString::Printf(TEXT("Player can damage enemy before refresh DOT registration (reason %d)"),
		static_cast<int32>(TargetFailureReason)), bCanDamage);

	const FARDamageOverTimeSpec Spec = ARFoundationTests::MakeDotSpec(
		Attacker, Target, TEXT("RefreshBurn"), EARDotStackPolicy::RefreshSameName);
	bool bApplied = false;
	EARRequestResult FailureReason = EARRequestResult::Rejected;
	const FARDotHandle OriginalHandle = Combat->ApplyDamageOverTime(Spec, bApplied, FailureReason);
	TestTrue(TEXT("Refreshable DOT registration succeeds"), bApplied && OriginalHandle.IsValid());
	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 0.5f);
	TestEqual(TEXT("Two ticks occur before refresh"), Target->GetHealthComponent()->GetCurrentHealth(), 980.0f);

	const FARDotHandle RefreshedHandle = Combat->ApplyDamageOverTime(Spec, bApplied, FailureReason);
	TestTrue(TEXT("Matching refresh succeeds and preserves the handle"), bApplied && RefreshedHandle == OriginalHandle);
	TestEqual(TEXT("Refresh keeps exactly one DOT instance"), Combat->GetActiveDamageOverTimeCount(Target), 1);

	FARDamageOverTimeSpec MismatchedSpec = Spec;
	MismatchedSpec.DamageRequest.BaseDamage = 11.0f;
	AddExpectedError(TEXT("Refresh DOT RefreshBurn has mismatched"), EAutomationExpectedErrorFlags::Contains, 1);
	const FARDotHandle RejectedHandle = Combat->ApplyDamageOverTime(MismatchedSpec, bApplied, FailureReason);
	TestFalse(TEXT("Mismatched same-name refresh is rejected"), bApplied || RejectedHandle.IsValid());
	TestEqual(TEXT("Mismatched refresh reports invalid definition"), FailureReason, EARRequestResult::InvalidDefinition);
	TestEqual(TEXT("Rejected refresh leaves the original DOT active"), Combat->GetActiveDamageOverTimeCount(Target), 1);

	ARFoundationTests::AdvanceDotTime(TestWorld.World, Combat, 1.0f);
	TestEqual(TEXT("Refresh resets duration and first tick timing"), Target->GetHealthComponent()->GetCurrentHealth(), 940.0f);
	TestEqual(TEXT("Refreshed DOT eventually completes"), Combat->GetActiveDamageOverTimeCount(Target), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARShieldLifoAndExpiryTest,
	"AR.Foundation.Health.ShieldLifoAndExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARShieldLifoAndExpiryTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	ARFoundationTests::BeginCombatActor(Player, 100.0f);
	UARHealthComponent* Health = Player->GetHealthComponent();

	FARShieldSpec OlderShield;
	OlderShield.Amount = 30.0f;
	OlderShield.Duration = -1.0f;
	bool bApplied = false;
	const FARShieldHandle OlderHandle = Health->ApplyShield(OlderShield, bApplied);
	TestTrue(TEXT("Older permanent shield applies"), bApplied && OlderHandle.IsValid());

	FARShieldSpec NewerShield;
	NewerShield.Amount = 20.0f;
	NewerShield.Duration = -1.0f;
	const FARShieldHandle NewerHandle = Health->ApplyShield(NewerShield, bApplied);
	TestTrue(TEXT("Newer permanent shield applies"), bApplied && NewerHandle.IsValid());
	TestEqual(TEXT("Both shields add together"), Health->GetCurrentShield(), 50.0f);

	FARCombatDamageResult Damage;
	Damage.Outcome = EARDamageOutcome::Applied;
	Damage.FailureReason = EARRequestResult::Success;
	Damage.FinalDamage = 25;
	Damage.ShieldDamage = 25;
	Damage.HealthDamage = 0;
	TestTrue(TEXT("Resolved shield damage applies"), Health->ApplyResolvedDamage(Damage));
	TestEqual(TEXT("Shield damage leaves the expected total"), Health->GetCurrentShield(), 25.0f);

	float RemovedAmount = 0.0f;
	TestFalse(TEXT("Newest shield was consumed first and no longer exists"), Health->RemoveShield(NewerHandle, RemovedAmount));
	TestTrue(TEXT("Older shield retains the remaining amount"), Health->RemoveShield(OlderHandle, RemovedAmount));
	TestEqual(TEXT("Five damage spills into the older shield"), RemovedAmount, 25.0f);

	FARShieldSpec TimedShield;
	TimedShield.Amount = 10.0f;
	TimedShield.Duration = 0.5f;
	const FARShieldHandle TimedHandle = Health->ApplyShield(TimedShield, bApplied);
	TestTrue(TEXT("Timed shield applies"), bApplied && TimedHandle.IsValid());
	TestEqual(TEXT("Timed shield is initially visible"), Health->GetCurrentShield(), 10.0f);
	for (int32 Step = 0; Step < 3; ++Step)
	{
		TestWorld.World->Tick(LEVELTICK_All, 0.25f);
		Health->TickComponent(0.25f, LEVELTICK_All, nullptr);
	}
	TestEqual(TEXT("Timed shield expires after its duration"), Health->GetCurrentShield(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARResourceTransactionTest,
	"AR.Foundation.Resource.AtomicTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARResourceTransactionTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	Player->GetStatsComponent()->BeginPlay();
	Player->GetManaComponent()->BeginPlay();
	Player->GetStaminaComponent()->BeginPlay();

	FARResourceCost Cost;
	Cost.Mana = 20.0f;
	Cost.Stamina = 30.0f;
	FARSourceInfo Source;
	Source.SourceId = TEXT("Test.ResourceTransaction");
	float NewMana = 0.0f;
	float NewStamina = 0.0f;
	EARResourceType MissingResource = EARResourceType::Health;
	TestTrue(TEXT("Affordable mana and stamina are consumed together"),
		UARResourceBlueprintLibrary::TryConsumeResources(Player, Cost, Source, NewMana, NewStamina, MissingResource));
	TestEqual(TEXT("Successful transaction returns the remaining mana"), NewMana, 80.0f);
	TestEqual(TEXT("Successful transaction returns the remaining stamina"), NewStamina, 70.0f);
	TestEqual(TEXT("Successful transaction reports no missing resource"), MissingResource, EARResourceType::None);

	Cost.Mana = 81.0f;
	Cost.Stamina = 10.0f;
	TestFalse(TEXT("Unaffordable group is rejected before either resource changes"),
		UARResourceBlueprintLibrary::TryConsumeResources(Player, Cost, Source, NewMana, NewStamina, MissingResource));
	TestEqual(TEXT("Rejected transaction identifies mana"), MissingResource, EARResourceType::Mana);
	TestEqual(TEXT("Rejected transaction preserves mana"), Player->GetManaComponent()->GetCurrent(), 80.0f);
	TestEqual(TEXT("Rejected transaction preserves stamina"), Player->GetStaminaComponent()->GetCurrent(), 70.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARActionFullCleanupTest,
	"AR.Foundation.Action.FullCancellationCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARActionFullCleanupTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	UARStatsComponent* Stats = Player->GetStatsComponent();
	UARMovementControlComponent* Movement = Player->GetMovementControlComponent();
	UARStaggerComponent* Stagger = Player->GetStaggerComponent();
	UARActionComponent* Action = Player->GetActionComponent();
	Stats->SetBaseStat(EARStatType::AttackPower, 100.0f);
	Stats->BeginPlay();
	Movement->BeginPlay();
	Stagger->BeginPlay();
	Action->BeginPlay();

	FARActionRequest Request;
	Request.bBlockBasicMovementWhileActive = true;
	Request.CancelRules.bCancelOnStagger = true;
	FARRequestStatus StartStatus;
	const FARActionHandle Handle = Action->TryStartAction(Request, StartStatus);
	TestTrue(TEXT("Action starts"), StartStatus.IsSuccess() && Handle.IsValid());
	TestFalse(TEXT("Action-owned movement lock blocks basic movement"), Movement->CanBasicMove());
	TestTrue(TEXT("An active owned action may request forced movement"),
		Movement->RequestActionVelocity(Handle, FVector::ForwardVector, 300.0f));

	FARStatModifierSpec Modifier;
	Modifier.StatType = EARStatType::AttackPower;
	Modifier.Operation = EARStatModifierOperation::Flat;
	Modifier.Value = 50.0f;
	Modifier.Duration = -1.0f;
	bool bModifierApplied = false;
	Action->ApplyActionStatModifier(Handle, Modifier, bModifierApplied);
	TestTrue(TEXT("Action-owned stat modifier applies"), bModifierApplied);

	FARSuperArmorSpec Armor;
	Armor.Duration = -1.0f;
	bool bArmorApplied = false;
	Action->ApplyActionSuperArmor(Handle, Armor, bArmorApplied);
	TestTrue(TEXT("Action-owned super armor applies"), bArmorApplied && Stagger->IsSuperArmorActive());
	FARCCImmunitySpec Immunity;
	Immunity.Duration = -1.0f;
	bool bImmunityApplied = false;
	Action->ApplyActionCCImmunity(Handle, Immunity, bImmunityApplied);
	TestTrue(TEXT("Action-owned CC immunity applies"), bImmunityApplied && Player->GetStatusEffectComponent()->IsCCImmunityActive());

	AActor* HitboxActor = TestWorld.World->SpawnActor<AActor>();
	TestTrue(TEXT("Action accepts an owned hitbox actor"), Action->RegisterActionHitbox(Handle, HitboxActor));
	TestEqual(TEXT("Stagger reason cancels the action"), Action->CancelActionsByReason(EARActionCancelReason::Stagger), 1);
	TestFalse(TEXT("Cancelled action is no longer active"), Action->IsActionActive(Handle));
	TestEqual(TEXT("Cancellation removes the temporary stat modifier"), Stats->GetFinalStat(EARStatType::AttackPower), 100.0f);
	TestFalse(TEXT("Cancellation removes action-owned super armor"), Stagger->IsSuperArmorActive());
	TestFalse(TEXT("Cancellation removes action-owned CC immunity"), Player->GetStatusEffectComponent()->IsCCImmunityActive());
	TestTrue(TEXT("Cancellation releases the movement lock"), Movement->CanBasicMove());
	TestTrue(TEXT("Cancellation destroys the registered hitbox actor"), HitboxActor->IsActorBeingDestroyed());
	TestFalse(TEXT("A cancelled action handle cannot request more forced movement"),
		Movement->RequestActionVelocity(Handle, FVector::ForwardVector, 300.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPlayerUISnapshotAndBlockingTest,
	"AR.Foundation.UI.SnapshotAndInputBlocking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARPlayerUISnapshotAndBlockingTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	Player->GetStatsComponent()->BeginPlay();
	Player->GetHealthComponent()->BeginPlay();
	Player->GetManaComponent()->BeginPlay();
	Player->GetStaminaComponent()->BeginPlay();
	Player->GetActionComponent()->BeginPlay();
	Player->GetStatusEffectComponent()->BeginPlay();
	Player->GetLoadoutComponent()->BeginPlay();
	Player->GetConsumableComponent()->BeginPlay();
	UARUIManagerComponent* UI = Player->GetUIManagerComponent();
	UI->BeginPlay();
	TestTrue(TEXT("Player aim accepts a valid gameplay-plane point"),
		Player->SetAimWorldLocation(FVector(100.0f, 50.0f, 1000.0f)));

	UARStatusEffectDefinition* HUDStatusDefinition = NewObject<UARStatusEffectDefinition>();
	HUDStatusDefinition->DefinitionTag = ARGameplayTags::Status_Stun;
	HUDStatusDefinition->StatusTag = ARGameplayTags::Status_Stun;
	HUDStatusDefinition->BaseDuration = 5.0f;
	FARStatusEffectRequest HUDStatusRequest;
	HUDStatusRequest.Definition = HUDStatusDefinition;
	const FARStatusEffectResult HUDStatusResult = Player->GetStatusEffectComponent()->ApplyStatusEffect(HUDStatusRequest);
	TestEqual(TEXT("HUD test status applies"), HUDStatusResult.Result, EARRequestResult::Success);

	const FARPlayerHUDSnapshot Snapshot = UI->GetHUDSnapshot();
	TestEqual(TEXT("HUD snapshot includes current health"), Snapshot.Health.Current, 100.0f);
	TestEqual(TEXT("HUD snapshot includes current mana"), Snapshot.Mana.Current, 100.0f);
	TestEqual(TEXT("HUD snapshot includes current stamina"), Snapshot.Stamina.Current, 100.0f);
	TestEqual(TEXT("HUD snapshot exposes the default consumable slot count"), Snapshot.Consumables.Num(), 3);
	TestTrue(TEXT("HUD snapshot exposes a valid aim-world point"), Snapshot.bHasAimWorldLocation);
	TestTrue(TEXT("HUD aim point is projected onto the player plane"),
		Snapshot.AimWorldLocation.Equals(FVector(100.0f, 50.0f, Player->GetActorLocation().Z), 0.01f));
	TestEqual(TEXT("HUD snapshot includes active status effects"), Snapshot.StatusEffects.Num(), 1);
	FARStatModifierSpec VisibleStack;
	VisibleStack.bStackOnly = true;
	VisibleStack.Source.Category = EARModifierSourceCategory::Buff;
	VisibleStack.Source.SourceId = TEXT("Test.SwordStacks");
	VisibleStack.Source.DisplayName = FText::FromString(TEXT("Sword stacks"));
	VisibleStack.HUDDisplay = EARStatEffectDisplay::Buff;
	bool bStackAdded = false;
	const FARStatModifierHandle VisibleStackHandle = Player->GetStatsComponent()->AddStatModifier(VisibleStack, bStackAdded);
	TestTrue(TEXT("Stack-only effect applies"), bStackAdded);
	const FARPlayerHUDSnapshot CounterSnapshot = UI->GetHUDSnapshot();
	TestEqual(TEXT("Visible stat effects reach HUD snapshot"), CounterSnapshot.StatEffects.Num(), 1);
	if (CounterSnapshot.StatEffects.Num() == 1)
	{
		TestEqual(TEXT("One application is one stack"), CounterSnapshot.StatEffects[0].StackCount, 1);
	}
	TestTrue(TEXT("Visible stack can be removed"), Player->GetStatsComponent()->RemoveStatModifier(VisibleStackHandle));
	TestEqual(TEXT("Removed effect disappears from HUD snapshot"), UI->GetHUDSnapshot().StatEffects.Num(), 0);
	const TArray<FARFinalStatView> StatViews = Player->GetStatsComponent()->GetAllFinalStatViews();
	TestEqual(TEXT("Character sheet exposes every public stat in enum order"),
		StatViews.Num(), static_cast<int32>(EARStatType::Count));
	TestEqual(TEXT("Character sheet includes the default maximum health"),
		StatViews[static_cast<int32>(EARStatType::MaxHealth)].Breakdown.FinalValue, 100.0f);

	const FARRequestStatus OpenStatus = UI->OpenScreen(EARUIScreen::Inventory);
	TestTrue(TEXT("Inventory screen opens"), OpenStatus.IsSuccess());
	TestTrue(TEXT("Opening a screen blocks gameplay input"), Player->IsGameplayInputBlocked());
	TestEqual(TEXT("Only one screen may be open"), UI->OpenScreen(EARUIScreen::Shop).Result, EARRequestResult::Blocked);
	TestEqual(TEXT("Blocked request does not replace the current screen"), UI->GetCurrentScreen(), EARUIScreen::Inventory);
	TestTrue(TEXT("Current screen closes"), UI->CloseCurrentScreen());
	TestFalse(TEXT("Closing the screen restores gameplay input"), Player->IsGameplayInputBlocked());

	UARItemDefinition* UnownedDefinition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_PassiveRelic, UARLoadoutItemInstance::StaticClass());
	UnownedDefinition->DisplayName = FText::FromString(TEXT("Unowned preview relic"));
	FARStatModifierSpec ProvidedStat;
	ProvidedStat.StatType = EARStatType::AttackPower;
	ProvidedStat.Operation = EARStatModifierOperation::Flat;
	ProvidedStat.Value = 5.0f;
	UnownedDefinition->DefaultStatModifiers.Add(ProvidedStat);
	UnownedDefinition->SkillDefinitions.Add(ARFoundationTests::MakeSkill(
		TEXT("PreviewSkill"), ARGameplayTags::Input_Skill_1, 0, 0.0f));
	FARLoadoutItemDisplayData PreviewData;
	TestTrue(TEXT("Unowned definitions can provide shop or evolution display data"),
		Player->GetLoadoutComponent()->GetLoadoutDefinitionDisplayData(UnownedDefinition, PreviewData));
	TestFalse(TEXT("Definition preview has no runtime instance id"), PreviewData.InstanceId.IsValid());
	TestEqual(TEXT("Definition preview includes provided stats"), PreviewData.ProvidedStats.Num(), 1);
	TestEqual(TEXT("Definition preview includes declared skills"), PreviewData.Skills.Num(), 1);

	UARItemDefinition* ConsumableDefinition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_Consumable, UARConsumableInstance::StaticClass());
	ConsumableDefinition->DisplayName = FText::FromString(TEXT("Preview potion"));
	FARConsumableDisplayData ConsumablePreview;
	TestTrue(TEXT("Unowned consumable definition provides shop display data"),
		Player->GetConsumableComponent()->GetConsumableDefinitionDisplayData(ConsumableDefinition, ConsumablePreview));
	TestFalse(TEXT("Unowned consumable preview has no runtime instance id"), ConsumablePreview.InstanceId.IsValid());
	const FARConsumableAcquisitionResult ConsumableAcquire =
		Player->GetConsumableComponent()->TryAcquireConsumable(ConsumableDefinition);
	TestTrue(TEXT("Preview consumable can be acquired"), ConsumableAcquire.Status.IsSuccess());
	FARConsumableDisplayData OwnedConsumableData;
	TestTrue(TEXT("Owned consumable slot provides runtime display data"),
		Player->GetConsumableComponent()->GetConsumableSlotDisplayData(ConsumableAcquire.SlotIndex, OwnedConsumableData));
	TestEqual(TEXT("Owned consumable display keeps its slot index"), OwnedConsumableData.SlotIndex, ConsumableAcquire.SlotIndex);
	TestTrue(TEXT("Owned consumable display includes its runtime instance id"), OwnedConsumableData.InstanceId.IsValid());
	TestTrue(TEXT("Status can be removed after it was exposed to the HUD"),
		Player->GetStatusEffectComponent()->RemoveStatusEffect(HUDStatusResult.Handle));
	TestEqual(TEXT("HUD snapshot no longer exposes the removed status"), UI->GetHUDSnapshot().StatusEffects.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPlayerPostHitInvulnerabilityTest,
	"AR.Foundation.Player.PostHitInvulnerability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARPlayerPostHitInvulnerabilityTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARBaseEnemy* Enemy = TestWorld.World->SpawnActor<AARBaseEnemy>();
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Enemy spawned"), Enemy);
	TestNotNull(TEXT("Player spawned"), Player);
	Enemy->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 100.0f);
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 100.0f);
	Enemy->DispatchBeginPlay();
	Player->DispatchBeginPlay();
	TestTrue(TEXT("Player entered BeginPlay"), Player->HasActorBegunPlay());
	TestTrue(TEXT("Player listens for applied damage"), Player->GetHealthComponent()->OnDamageAppliedNative.IsBound());
	TestEqual(TEXT("Post-hit invulnerability is disabled by default"),
		Player->GetPostHitInvulnerabilityDuration(), 0.0f);
	UClass* TestPlayerBlueprintClass = LoadClass<AARPlayerCharacter>(
		nullptr, TEXT("/Game/Game/Tests/Player/BP_test_Player.BP_test_Player_C"));
	TestNotNull(TEXT("Test player Blueprint class loads"), TestPlayerBlueprintClass);
	if (TestPlayerBlueprintClass)
	{
		TestEqual(TEXT("Test player Blueprint also starts with post-hit invulnerability disabled"),
			TestPlayerBlueprintClass->GetDefaultObject<AARPlayerCharacter>()->GetPostHitInvulnerabilityDuration(), 0.0f);
	}

	UARCombatSubsystem* Combat = TestWorld.World->GetSubsystem<UARCombatSubsystem>();
	TestNotNull(TEXT("Combat subsystem exists"), Combat);
	FARCombatDamageRequest Request;
	Request.Attacker = Enemy;
	Request.Target = Player;
	Request.BaseDamage = 10.0f;
	Request.bGuaranteedHit = true;
	Request.bCanCrit = false;
	Request.bApplyAmplification = false;
	Request.bIgnoreDefense = true;

	const FARCombatDamageResult UnprotectedHit = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("First hit without post-hit invulnerability is applied"), UnprotectedHit.Outcome, EARDamageOutcome::Applied);
	TestFalse(TEXT("Default zero duration grants no invulnerability"), Player->GetStatsComponent()->HasGuaranteedInvulnerability());
	const FARCombatDamageResult SecondUnprotectedHit = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("Another direct hit is allowed with zero duration"), SecondUnprotectedHit.Outcome, EARDamageOutcome::Applied);
	TestEqual(TEXT("Both unprotected hits remove health"), Player->GetHealthComponent()->GetCurrentHealth(), 80.0f);

	Player->PostHitInvulnerabilityDuration = 0.35f;
	const FARCombatDamageResult FirstHit = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("First direct hit is applied"), FirstHit.Outcome, EARDamageOutcome::Applied);
	TestFalse(TEXT("First hit does not kill the player"), FirstHit.bKilledTarget);
	TestEqual(TEXT("First hit is direct damage"), FirstHit.HitContext.Delivery, EARDamageDelivery::Direct);
	TestEqual(TEXT("First hit is physical damage"), FirstHit.HitContext.Attribute, EARDamageAttribute::Physical);
	TestEqual(TEXT("First hit with enabled protection removes health"), Player->GetHealthComponent()->GetCurrentHealth(), 70.0f);
	TestTrue(TEXT("A surviving direct hit grants managed invulnerability"),
		Player->GetStatsComponent()->HasGuaranteedInvulnerability());

	const FARCombatDamageResult BlockedHit = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("A normal hit during post-hit invulnerability is blocked"), BlockedHit.Outcome, EARDamageOutcome::Blocked);
	TestEqual(TEXT("Blocked hit does not remove more health"), Player->GetHealthComponent()->GetCurrentHealth(), 70.0f);

	Request.Attribute = EARDamageAttribute::Void;
	const FARCombatDamageResult VoidHit = Combat->ApplyCombatDamage(Request);
	TestEqual(TEXT("Void damage bypasses post-hit invulnerability"), VoidHit.Outcome, EARDamageOutcome::Applied);
	TestEqual(TEXT("Void damage reaches health"), Player->GetHealthComponent()->GetCurrentHealth(), 60.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARWeaponEvolutionRulesTest,
	"AR.Foundation.Items.WeaponEvolutionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARWeaponEvolutionRulesTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	ARFoundationTests::BeginPlayerSkillSystems(Player);
	UARLoadoutComponent* Loadout = Player->GetLoadoutComponent();

	auto MakeWeapon = [](const TCHAR* Name, int32 Stage, FName Group)
	{
		UARItemDefinition* Definition = ARFoundationTests::MakeItem(
			ARGameplayTags::Item_Type_Weapon, UARLoadoutItemInstance::StaticClass(), FName(Name));
		Definition->EvolutionGroupId = Group;
		Definition->EvolutionStage = Stage;
		return Definition;
	};

	const FName EvolutionGroup(TEXT("Test.Caliburn"));
	UARItemDefinition* Stage1 = MakeWeapon(TEXT("TestWeaponStage1"), 1, EvolutionGroup);
	UARItemDefinition* Stage2 = MakeWeapon(TEXT("TestWeaponStage2"), 2, EvolutionGroup);
	UARItemDefinition* Stage3A = MakeWeapon(TEXT("TestWeaponStage3A"), 3, EvolutionGroup);
	UARItemDefinition* Stage3B = MakeWeapon(TEXT("TestWeaponStage3B"), 3, EvolutionGroup);
	UARItemDefinition* WrongGroup = MakeWeapon(TEXT("TestWeaponWrongGroup"), 3, TEXT("Test.Other"));
	Stage1->NextEvolutionCandidates.Add(Stage2);
	Stage2->NextEvolutionCandidates.Add(Stage3A);
	Stage2->NextEvolutionCandidates.Add(WrongGroup);
	Stage2->NextEvolutionCandidates.Add(Stage3B);

	const FARLoadoutAcquisitionResult Acquire = Loadout->BeginLoadoutAcquisition(Stage1);
	FARRequestStatus AcquireStatus;
	UARLoadoutItemInstance* InitialInstance = Loadout->CommitLoadoutAcquisition(Acquire.Token, AcquireStatus);
	TestTrue(TEXT("Stage-one weapon is equipped"), Acquire.Status.IsSuccess() && AcquireStatus.IsSuccess() && InitialInstance);

	const FARWeaponEvolutionResult Automatic = Loadout->RequestWeaponEvolution();
	TestTrue(TEXT("A single valid next stage evolves immediately"), Automatic.Status.IsSuccess());
	TestFalse(TEXT("Single-candidate evolution does not require a selection UI"), Automatic.bRequiresSelection);
	TestNotNull(TEXT("Automatic evolution returns the new runtime instance"), Automatic.EvolvedInstance.Get());
	TestTrue(TEXT("Stage two is now equipped"),
		Loadout->GetEquippedWeapon() && Loadout->GetEquippedWeapon()->GetItemDefinition() == Stage2);

	AddExpectedError(TEXT("Ignoring invalid evolution candidate"), EAutomationExpectedErrorFlags::Contains, 1);
	const FARWeaponEvolutionResult Selection = Loadout->RequestWeaponEvolution();
	TestTrue(TEXT("Multiple valid candidates produce a selection request"), Selection.Status.IsSuccess());
	TestTrue(TEXT("Multiple candidates require selection"), Selection.bRequiresSelection);
	TestTrue(TEXT("Selection request returns a valid token"), Selection.Token.IsValid());
	TestEqual(TEXT("Wrong-group candidates are rejected before UI presentation"), Selection.Candidates.Num(), 2);

	FARRequestStatus EvolutionStatus;
	UARLoadoutItemInstance* FinalInstance = Loadout->CommitWeaponEvolution(Selection.Token, Stage3B, EvolutionStatus);
	TestTrue(TEXT("A listed stage-three candidate can be committed"), EvolutionStatus.IsSuccess() && FinalInstance);
	TestTrue(TEXT("Selected stage three is equipped"),
		Loadout->GetEquippedWeapon() && Loadout->GetEquippedWeapon()->GetItemDefinition() == Stage3B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMovementSpeedStatBindingTest,
	"AR.Foundation.Character.MovementSpeedStatBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMovementSpeedStatBindingTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	TestWorld.World->InitializeActorsForPlay(FURL());
	AARBaseEnemy* Enemy = TestWorld.World->SpawnActor<AARBaseEnemy>();
	if (!TestNotNull(TEXT("Enemy spawned"), Enemy)) return false;
	UARStatsComponent* Stats = Enemy->GetStatsComponent();
	Stats->SetBaseStat(EARStatType::MoveSpeed, 240.0f);
	Enemy->DispatchBeginPlay();
	TestEqual(TEXT("Initial stat sets actual movement speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 240.0f);
	FARStatModifierSpec Modifier;
	Modifier.StatType = EARStatType::MoveSpeed;
	Modifier.Operation = EARStatModifierOperation::Flat;
	Modifier.Value = 60.0f;
	Modifier.Duration = -1.0f;
	bool bApplied = false;
	const FARStatModifierHandle Handle = Stats->AddStatModifier(Modifier, bApplied);
	TestTrue(TEXT("Speed modifier applies"), bApplied);
	TestEqual(TEXT("Buff changes actual movement speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 300.0f);
	Stats->RemoveStatModifier(Handle);
	TestEqual(TEXT("Removing the buff restores speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 240.0f);
	Stats->SetBaseStat(EARStatType::MoveSpeed, 180.0f);
	TestEqual(TEXT("Runtime base changes also update speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 180.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARActionStartStateGuardsTest,
	"AR.Foundation.Action.StartStateGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARActionStartStateGuardsTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	TestWorld.World->InitializeActorsForPlay(FURL());
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	Player->DispatchBeginPlay();
	UARActionComponent* Action = Player->GetActionComponent();
	FARActionRequest Skill;
	FARActionRequest Roll;
	Roll.bIsRollAction = true;
	FARRequestStatus Status;
	const FARActionHandle SkillHandle = Action->TryStartAction(Skill, Status);
	TestTrue(TEXT("Skill starts normally"), SkillHandle.IsValid());
	const FARActionHandle RollHandle = Action->TryStartAction(Roll, Status);
	TestTrue(TEXT("Roll may coexist with an already running skill"), RollHandle.IsValid());
	TestTrue(TEXT("Existing skill remains active"), Action->IsActionActive(SkillHandle));
	TestFalse(TEXT("A new skill cannot start during roll"), Action->TryStartAction(Skill, Status).IsValid());
	TestEqual(TEXT("Rolling reports blocked"), Status.Result, EARRequestResult::Blocked);
	TestFalse(TEXT("A second roll cannot start"), Action->TryStartAction(Roll, Status).IsValid());
	Action->EndAction(RollHandle);
	TestTrue(TEXT("Skills become available when roll ends"), Action->CanStartAction(Skill).IsSuccess());
	FARStaggerRequest Stagger;
	Stagger.HitContext.HitId = FGuid::NewGuid();
	Stagger.HitContext.Target = Player;
	Stagger.HitContext.bValidHit = true;
	Stagger.Template.BaseStaggerDamage = 1000.0f;
	TestTrue(TEXT("Stagger applies"), Player->GetStaggerComponent()->ApplyStaggerAndGroggyDamage(Stagger).bStaggered);
	TestFalse(TEXT("New skill is rejected while staggered"), Action->TryStartAction(Skill, Status).IsValid());
	TestEqual(TEXT("Stagger reports blocked"), Status.Result, EARRequestResult::Blocked);
	TestFalse(TEXT("Roll is rejected while staggered"), Action->TryStartAction(Roll, Status).IsValid());
	FARCombatDamageResult Lethal;
	Lethal.Outcome = EARDamageOutcome::Applied;
	Lethal.FinalDamage = 10000;
	Lethal.HealthDamage = 10000;
	Player->GetHealthComponent()->ApplyResolvedDamage(Lethal);
	TestTrue(TEXT("Player died"), Player->GetHealthComponent()->IsDead());
	TestFalse(TEXT("Dead player cannot start a skill"), Action->TryStartAction(Skill, Status).IsValid());
	TestEqual(TEXT("Death reports dead"), Status.Result, EARRequestResult::Dead);
	TestFalse(TEXT("Dead player cannot start a roll"), Action->TryStartAction(Roll, Status).IsValid());
	TestEqual(TEXT("No actions remain after rejected requests"), Action->GetActiveActionCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARInventoryConsumableUseTest,
	"AR.Foundation.UI.InventoryConsumableUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARInventoryConsumableUseTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	TestWorld.World->InitializeActorsForPlay(FURL());
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	Player->DispatchBeginPlay();
	UARItemDefinition* Definition = ARFoundationTests::MakeItem(
		ARGameplayTags::Item_Type_Consumable, UARConsumableInstance::StaticClass());
	UARConsumableComponent* Consumables = Player->GetConsumableComponent();
	UARUIManagerComponent* UI = Player->GetUIManagerComponent();
	TestTrue(TEXT("Consumable acquired"), Consumables->TryAcquireConsumable(Definition).Status.IsSuccess());
	UI->OpenScreen(EARUIScreen::Inventory);
	TestTrue(TEXT("Inventory still blocks gameplay hotkeys"), Player->IsGameplayInputBlocked());
	UARItemDefinition* UsedDefinition = nullptr;
	TestTrue(TEXT("Inventory can directly use a consumable"), Consumables->TryUseConsumableSlot(0, UsedDefinition).IsSuccess());
	TestTrue(TEXT("Use returns the definition"), UsedDefinition == Definition);
	TestFalse(TEXT("Used item is removed"), Consumables->GetConsumableSlots()[0].bOccupied);
	TestTrue(TEXT("Next consumable acquired"), Consumables->TryAcquireConsumable(Definition).Status.IsSuccess());
	Player->SetGameplayInputBlocked(true);
	TestEqual(TEXT("Explicit input lock also blocks inventory use"), Consumables->TryUseConsumableSlot(0, UsedDefinition).Result, EARRequestResult::Blocked);
	UI->CloseCurrentScreen();
	TestTrue(TEXT("Closing inventory preserves explicit input lock"), Player->IsGameplayInputBlocked());
	Player->SetGameplayInputBlocked(false);
	UI->OpenScreen(EARUIScreen::Menu);
	TestEqual(TEXT("Other screens do not allow use"), Consumables->TryUseConsumableSlot(0, UsedDefinition).Result, EARRequestResult::Blocked);
	TestNull(TEXT("Rejected use clears output"), UsedDefinition);
	TestTrue(TEXT("Rejected use preserves item"), Consumables->GetConsumableSlots()[0].bOccupied);
	UI->CloseCurrentScreen();
	TestFalse(TEXT("Closing menu restores gameplay input"), Player->IsGameplayInputBlocked());
	FARCombatDamageResult Lethal;
	Lethal.Outcome = EARDamageOutcome::Applied;
	Lethal.FinalDamage = 10000;
	Lethal.HealthDamage = 10000;
	Player->GetHealthComponent()->ApplyResolvedDamage(Lethal);
	TestEqual(TEXT("Death prevents use even after UI closes"), Consumables->TryUseConsumableSlot(0, UsedDefinition).Result, EARRequestResult::Dead);
	TestTrue(TEXT("Death rejection preserves item"), Consumables->GetConsumableSlots()[0].bOccupied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPlayerDashDirectionsTest,
	"AR.Foundation.Player.DashDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARPlayerDashDirectionsTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	TestWorld.World->InitializeActorsForPlay(FURL());
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	Player->DispatchBeginPlay();
	Player->RollCooldown = 0.0f;
	Player->GetStatsComponent()->SetBaseStat(EARStatType::RollStaminaCost, 0.0f);
	Player->SetAimDirection(-FVector::ForwardVector);
	for (const FVector2D Input : {FVector2D(0, 1), FVector2D(1, 1), FVector2D(1, 0), FVector2D(1, -1),
		FVector2D(0, -1), FVector2D(-1, -1), FVector2D(-1, 0), FVector2D(-1, 1)})
	{
		Player->HandleMove(FInputActionValue(Input));
		TestTrue(TEXT("Directional dash starts"), Player->TryStartRoll());
		TestTrue(TEXT("WASD including diagonals determines dash direction"),
			Player->RollDirection.Equals(FVector(Input.Y, Input.X, 0).GetSafeNormal()));
		TestTrue(TEXT("Diagonal direction is normalized"), FMath::IsNearlyEqual(Player->RollDirection.Size(), 1.0));
		Player->FinishRoll(false);
	}
	Player->HandleMoveReleased(FInputActionValue(FVector2D::ZeroVector));
	TestTrue(TEXT("Dash after release starts"), Player->TryStartRoll());
	TestTrue(TEXT("Released WASD falls back to aim"), Player->RollDirection.Equals(-FVector::ForwardVector));
	Player->FinishRoll(false);
	Player->HandleMove(FInputActionValue(FVector2D(1, 1)));
	TestTrue(TEXT("Mouse-only request starts"), Player->TryStartRoll(true));
	TestTrue(TEXT("V2 ignores held movement"), Player->RollDirection.Equals(-FVector::ForwardVector));
	Player->HandleMoveReleased(FInputActionValue(FVector2D::ZeroVector));
	Player->FinishRoll(false);
	TestTrue(TEXT("Release during dash is remembered"), Player->CurrentMoveInput.IsNearlyZero());
	Player->RollDirectionMode = EARRollDirectionMode::MouseOnly;
	Player->HandleMove(FInputActionValue(FVector2D(1, 1)));
	TestTrue(TEXT("Mouse-only option uses ordinary dash input"), Player->TryStartRoll());
	TestTrue(TEXT("Option ignores movement"), Player->RollDirection.Equals(-FVector::ForwardVector));
	Player->FinishRoll(false);
	Player->SetGameplayInputBlocked(true);
	TestFalse(TEXT("UI/manual blocking prevents either mode"), Player->TryStartRoll(true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPlayerDashMovementTest,
	"AR.Foundation.Player.DashMovementAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARPlayerDashMovementTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	TestWorld.World->InitializeActorsForPlay(FURL());
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	Player->DispatchBeginPlay();
	Player->RollCooldown = 0.0f;
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	Movement->bRunPhysicsWithNoController = true;
	Movement->SetMovementMode(MOVE_Flying);
	Movement->MaxFlySpeed = 100.0f;
	Movement->BrakingDecelerationFlying = 10000.0f;
	Player->SetAimDirection(FVector::ForwardVector);
	const FVector Start = Player->GetActorLocation();
	TestTrue(TEXT("Dash starts"), Player->TryStartRoll(true));
	TestEqual(TEXT("Dash consumes stamina once"), Player->GetStaminaComponent()->GetCurrent(), 80.0f);
	TestFalse(TEXT("Repeated dash cannot spend more stamina"), Player->TryStartRoll());
	TestEqual(TEXT("Rejected repeat preserves stamina"), Player->GetStaminaComponent()->GetCurrent(), 80.0f);
	for (int32 Index = 0; Index < 10; ++Index) Movement->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Dash travels at configured speed despite slow normal movement and braking"),
		FMath::IsNearlyEqual(Player->GetActorLocation().X - Start.X, 175.0, 5.0));
	Player->GetActionComponent()->CancelAction(Player->ActiveRollHandle, EARActionCancelReason::Stagger);
	TestFalse(TEXT("Cancellation clears active roll immediately"), Player->ActiveRollHandle.IsValid());
	TestEqual(TEXT("Cancellation releases root motion ownership"), Player->RollRootMotionId, static_cast<uint16>(0));
	const FVector CancelledAt = Player->GetActorLocation();
	Movement->TickComponent(0.01f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Cancelled dash does not continue moving"), Player->GetActorLocation().Equals(CancelledAt, 0.1));
	TestFalse(TEXT("Cancellation removes guaranteed evasion"), Player->GetStatsComponent()->HasGuaranteedEvasion());
	Player->GetStatsComponent()->SetBaseStat(EARStatType::RollStaminaCost, 200.0f);
	TestFalse(TEXT("Insufficient stamina rejects dash"), Player->TryStartRoll(true));
	Player->GetStatsComponent()->SetBaseStat(EARStatType::RollStaminaCost, 0.0f);
	const FVector FullStart = Player->GetActorLocation();
	TestTrue(TEXT("Full dash starts"), Player->TryStartRoll(true));
	for (int32 Index = 0; Index < 22; ++Index)
	{
		Movement->TickComponent(0.01f, LEVELTICK_All, nullptr);
		Player->UpdateRoll(0.01f);
	}
	TestTrue(TEXT("Full dash covers configured distance"), FMath::IsNearlyEqual(Player->GetActorLocation().X - FullStart.X, 350.0, 5.0));
	TestFalse(TEXT("Completed motion ends its action"), Player->ActiveRollHandle.IsValid());
	TestTrue(TEXT("Completion releases basic movement lock"), Player->GetMovementControlComponent()->CanBasicMove());
	TestFalse(TEXT("Completion removes guaranteed evasion"), Player->GetStatsComponent()->HasGuaranteedEvasion());
	AActor* Wall = TestWorld.World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Wall->AddInstanceComponent(Box);
	Box->SetBoxExtent(FVector(10, 200, 200));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	const FVector WallStart = Player->GetActorLocation();
	Wall->SetActorLocation(WallStart + FVector(100, 0, 0));
	TestTrue(TEXT("Dash toward wall starts"), Player->TryStartRoll(true));
	for (int32 Index = 0; Index < 22; ++Index)
	{
		Movement->TickComponent(0.01f, LEVELTICK_All, nullptr);
		Player->UpdateRoll(0.01f);
	}
	TestTrue(TEXT("Dash respects blocking collision"), Player->GetActorLocation().X < WallStart.X + 90.0);
	TestFalse(TEXT("Blocked dash still finishes"), Player->ActiveRollHandle.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPlayerDashCooldownTest,
	"AR.Foundation.Player.DashCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARPlayerDashCooldownTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	TestWorld.World->InitializeActorsForPlay(FURL());
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player)) return false;
	Player->DispatchBeginPlay();
	Player->SetAimDirection(FVector::ForwardVector);
	TestEqual(TEXT("No cooldown before first dash"), Player->GetRollCooldownRemaining(), 0.0f);
	TestTrue(TEXT("First dash starts"), Player->TryStartRoll(true));
	Player->FinishRoll(false);
	TestTrue(TEXT("Cooldown begins after dash ends"), FMath::IsNearlyEqual(Player->GetRollCooldownRemaining(), 0.50f, 0.01f));
	const float StaminaAfterFirst = Player->GetStaminaComponent()->GetCurrent();
	TestFalse(TEXT("Immediate retry is blocked"), Player->TryStartRoll());
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.25f);
	TestTrue(TEXT("Cooldown counts down"), FMath::IsNearlyEqual(Player->GetRollCooldownRemaining(), 0.25f, 0.03f));
	TestFalse(TEXT("Retry during cooldown remains blocked"), Player->TryStartRoll(true));
	TestEqual(TEXT("Blocked retries do not spend stamina"), Player->GetStaminaComponent()->GetCurrent(), StaminaAfterFirst);
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.30f);
	TestEqual(TEXT("Cooldown finishes"), Player->GetRollCooldownRemaining(), 0.0f);
	TestTrue(TEXT("Dash works again after cooldown"), Player->TryStartRoll());
	Player->FinishRoll(true);
	TestTrue(TEXT("Cancelled dash also starts cooldown"), Player->GetRollCooldownRemaining() > 0.0f);
	Player->RollCooldown = 0.0f;
	ARFoundationTests::AdvanceWorld(TestWorld.World, 0.51f);
	TestTrue(TEXT("Zero cooldown allows another dash"), Player->TryStartRoll(true));
	Player->FinishRoll(false);
	TestEqual(TEXT("Zero cooldown leaves no wait"), Player->GetRollCooldownRemaining(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARResourceHUDLiveUpdateTest,
	"AR.Foundation.UI.ResourceHUDLiveUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARResourceHUDLiveUpdateTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard ScriptGuard;
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	Player->DispatchBeginPlay();
	UClass* HUDClass = LoadClass<UARResourceHUDWidget>(nullptr,
		TEXT("/Game/Game/Tests/UI/WBP_TestResourceHUD.WBP_TestResourceHUD_C"));
	if (!TestNotNull(TEXT("Editable Designer HUD asset loads"), HUDClass)) return false;
	UARResourceHUDWidget* HUD = CreateWidget<UARResourceHUDWidget>(TestWorld.World, HUDClass);
	if (!TestNotNull(TEXT("HUD widget created"), HUD)) return false;
	HUD->TakeWidget();
	HUD->ObservePawn(Player);
	UProgressBar* HealthBar = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("HealthBar")));
	UProgressBar* ManaBar = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("ManaBar")));
	UProgressBar* StaminaBar = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("StaminaBar")));
	UTextBlock* ManaText = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("ManaValue")));
	UProgressBar* ShieldBar = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("ShieldBar")));
	UTextBlock* HealthText = Cast<UTextBlock>(HUD->GetWidgetFromName(TEXT("HealthValue")));
	if (!TestNotNull(TEXT("Health bar bound"), HealthBar)
		|| !TestNotNull(TEXT("Mana bar bound"), ManaBar)
		|| !TestNotNull(TEXT("Stamina bar bound"), StaminaBar)
		|| !TestNotNull(TEXT("Mana text bound"), ManaText)
		|| !TestNotNull(TEXT("Shield layer added to existing Designer HUD"), ShieldBar)
		|| !TestNotNull(TEXT("Health text bound"), HealthText)) return false;
	TestTrue(TEXT("Shield layer uses gray fill"), ShieldBar->GetFillColorAndOpacity().Equals(FLinearColor(0.55f, 0.55f, 0.55f)));
	TestTrue(TEXT("Shield and HP share the same overlay"), ShieldBar->GetParent() == HealthBar->GetParent());
	TestTrue(TEXT("Shield fill renders behind red HP"), ShieldBar->GetParent()->GetChildIndex(ShieldBar)
		< HealthBar->GetParent()->GetChildIndex(HealthBar));
	TestFalse(TEXT("No shield omits the extra number"), HealthText->GetText().ToString().Contains(TEXT("+")));
	TestEqual(TEXT("Initial health is full"), HealthBar->GetPercent(), 1.0f);
	TestEqual(TEXT("Initial mana is full"), ManaBar->GetPercent(), 1.0f);
	TestEqual(TEXT("Initial stamina is full"), StaminaBar->GetPercent(), 1.0f);

	float Remaining = 0;
	FARSourceInfo Source;
	Player->GetManaComponent()->TryConsume(25.0f, Source, Remaining);
	Player->GetStaminaComponent()->TryConsume(40.0f, Source, Remaining);
	AARBaseEnemy* Enemy = TestWorld.World->SpawnActor<AARBaseEnemy>();
	Enemy->DispatchBeginPlay();
	FARCombatDamageRequest Damage;
	Damage.Attacker = Enemy;
	Damage.Target = Player;
	Damage.BaseDamage = 30.0f;
	Damage.bApplyEvasion = false;
	Damage.bCanCrit = false;
	const FARCombatDamageResult Result = TestWorld.World->GetSubsystem<UARCombatSubsystem>()->ApplyCombatDamage(Damage);
	TestEqual(TEXT("Real combat request damages health"), Result.HealthDamage, 30);
	TestEqual(TEXT("Damage updates health without polling"), HealthBar->GetPercent(), 0.7f);
	FARShieldSpec ShieldSpec;
	ShieldSpec.Amount = 20.0f;
	bool bShieldApplied = false;
	const FARShieldHandle ShieldHandle = Player->GetHealthComponent()->ApplyShield(ShieldSpec, bShieldApplied);
	TestTrue(TEXT("HUD test shield applies"), bShieldApplied);
	TestEqual(TEXT("Shield snapshot updates"), Player->GetUIManagerComponent()->GetHUDSnapshot().Shield, 20.0f);
	TestEqual(TEXT("Gray combined fill extends past current HP"), ShieldBar->GetPercent(), 0.9f);
	TestEqual(TEXT("Red fill retains HP ratio below the cap"), HealthBar->GetPercent(), 0.7f);
	TestTrue(TEXT("Health label includes shield"), HealthText->GetText().ToString().Contains(TEXT("+ 20")));
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 200.0f);
	TestEqual(TEXT("Max HP change rescales HP"), HealthBar->GetPercent(), 0.35f);
	TestEqual(TEXT("Max HP change rescales shield"), ShieldBar->GetPercent(), 0.45f);
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxHealth, 100.0f);
	Damage.BaseDamage = 5.0f;
	const FARCombatDamageResult ShieldHit = TestWorld.World->GetSubsystem<UARCombatSubsystem>()->ApplyCombatDamage(Damage);
	TestEqual(TEXT("Shield absorbs real damage"), ShieldHit.ShieldDamage, 5);
	TestEqual(TEXT("Shield hit leaves HP unchanged"), HealthBar->GetPercent(), 0.7f);
	TestEqual(TEXT("Shield consumption shrinks gray fill"), ShieldBar->GetPercent(), 0.85f);
	float RemovedShield = 0.0f;
	TestTrue(TEXT("Remaining shield removes"), Player->GetHealthComponent()->RemoveShield(ShieldHandle, RemovedShield));
	TestEqual(TEXT("No gray extension after removal"), ShieldBar->GetPercent(), HealthBar->GetPercent());
	TestFalse(TEXT("Removed shield clears the extra number"), HealthText->GetText().ToString().Contains(TEXT("+")));
	ShieldSpec.Amount = 200.0f;
	const FARShieldHandle LargeShield = Player->GetHealthComponent()->ApplyShield(ShieldSpec, bShieldApplied);
	TestEqual(TEXT("Large shield remains visible within the health row"), ShieldBar->GetPercent(), 1.0f);
	TestEqual(TEXT("HP and overflowing shield share an expanded scale"), HealthBar->GetPercent(), 70.0f / 270.0f);
	TestTrue(TEXT("Large shield amount is not truncated"), HealthText->GetText().ToString().Contains(TEXT("+ 200")));
	Player->GetHealthComponent()->RemoveShield(LargeShield, RemovedShield);
	ShieldSpec.Amount = 10.0f;
	ShieldSpec.Duration = 0.1f;
	Player->GetHealthComponent()->ApplyShield(ShieldSpec, bShieldApplied);
	TestEqual(TEXT("Timed shield appears"), ShieldBar->GetPercent(), 0.8f);
	TestWorld.World->Tick(LEVELTICK_All, 0.2f);
	Player->GetHealthComponent()->TickComponent(0.2f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Expiry clears shield snapshot"), Player->GetUIManagerComponent()->GetHUDSnapshot().Shield, 0.0f);
	TestEqual(TEXT("Expiry clears the gray extension"), ShieldBar->GetPercent(), HealthBar->GetPercent());
	TestFalse(TEXT("Expiry clears shield label"), HealthText->GetText().ToString().Contains(TEXT("+")));
	TestEqual(TEXT("Consumption updates mana without polling"), ManaBar->GetPercent(), 0.75f);
	TestEqual(TEXT("Consumption updates stamina without polling"), StaminaBar->GetPercent(), 0.6f);
	TestTrue(TEXT("Mana label shows consumed resource"), ManaText->GetText().ToString().Contains(TEXT("75")));
	Player->GetManaComponent()->Restore(25.0f, Source);
	TestEqual(TEXT("Recovery updates mana"), ManaBar->GetPercent(), 1.0f);
	Player->GetStatsComponent()->SetBaseStat(EARStatType::MaxMana, 200.0f);
	TestEqual(TEXT("Maximum resource change updates the ratio"), ManaBar->GetPercent(), 0.5f);
	TestTrue(TEXT("Maximum resource change updates the label"), ManaText->GetText().ToString().Contains(TEXT("200")));
	FARStatModifierSpec VisibleStack;
	VisibleStack.bStackOnly = true;
	VisibleStack.Source.Category = EARModifierSourceCategory::Buff;
	VisibleStack.Source.SourceId = TEXT("Test.LiveStacks");
	VisibleStack.Source.DisplayName = FText::FromString(TEXT("Sword stacks"));
	VisibleStack.HUDDisplay = EARStatEffectDisplay::Buff;
	bool bStackAdded = false;
	const FARStatModifierHandle FirstStack = Player->GetStatsComponent()->AddStatModifier(VisibleStack, bStackAdded);
	TestTrue(TEXT("First live stack added"), bStackAdded);
	UHorizontalBox* EffectIcons = Cast<UHorizontalBox>(HUD->GetWidgetFromName(TEXT("StatEffectIcons")));
	if (!TestNotNull(TEXT("Effect icons created on existing HUD"), EffectIcons)) return false;
	TestEqual(TEXT("One icon appears for one visible source"), EffectIcons->GetChildrenCount(), 1);
	const FARStatModifierHandle SecondStack = Player->GetStatsComponent()->AddStatModifier(VisibleStack, bStackAdded);
	TestTrue(TEXT("Second live stack added"), bStackAdded);
	TestEqual(TEXT("Two stacks remain one icon"), EffectIcons->GetChildrenCount(), 1);
	TestEqual(TEXT("HUD snapshot reports two stacks"), Player->GetUIManagerComponent()->GetHUDSnapshot().StatEffects[0].StackCount, 2);
	TestTrue(TEXT("First stack removed"), Player->GetStatsComponent()->RemoveStatModifier(FirstStack));
	TestEqual(TEXT("One stack remains"), Player->GetStatsComponent()->GetModifiersBySource(EARModifierSourceCategory::Buff, TEXT("Test.LiveStacks")).StackCount, 1);
	TestTrue(TEXT("Second stack removed"), Player->GetStatsComponent()->RemoveStatModifier(SecondStack));
	TestEqual(TEXT("Removed effect icon disappears"), EffectIcons->GetChildrenCount(), 0);

	AARPlayerCharacter* Replacement = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	Replacement->DispatchBeginPlay();
	HUD->ObservePawn(Replacement);
	TestEqual(TEXT("Possession change refreshes health"), HealthBar->GetPercent(), 1.0f);
	TestFalse(TEXT("Possession change clears shield label"), HealthText->GetText().ToString().Contains(TEXT("+")));
	ShieldSpec.Duration = -1.0f;
	Player->GetHealthComponent()->ApplyShield(ShieldSpec, bShieldApplied);
	TestFalse(TEXT("Old pawn shields no longer drive HUD"), HealthText->GetText().ToString().Contains(TEXT("+")));
	Player->GetStaminaComponent()->TryConsume(10.0f, Source, Remaining);
	TestEqual(TEXT("Old pawn no longer drives HUD"), StaminaBar->GetPercent(), 1.0f);
	HUD->ObservePawn(nullptr);
	TestEqual(TEXT("No player hides HUD"), HUD->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Disconnect resets shield fill"), ShieldBar->GetPercent(), 0.0f);
	Replacement->GetManaComponent()->TryConsume(20.0f, Source, Remaining);
	TestEqual(TEXT("Disconnected HUD ignores subsequent resource events"), ManaBar->GetPercent(), 0.0f);
	return true;
}

#endif
