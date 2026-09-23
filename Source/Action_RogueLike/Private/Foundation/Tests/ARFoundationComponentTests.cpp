#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Foundation/Actions/ARActionTypes.h"
#include "Foundation/Blueprint/ARResourceBlueprintLibrary.h"
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
#include "Foundation/Items/ARConsumableDefinition.h"
#include "Foundation/Items/ARConsumableInstance.h"
#include "Foundation/Items/ARLoadoutItemDefinition.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include "Foundation/Interaction/ARItemPickupActors.h"
#include "Foundation/Interfaces/ARCombatTargetInterface.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"

namespace ARFoundationTests
{
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
		Skill.ActionRequest.ActionTag = ARGameplayTags::Action_Roll;
		return Skill;
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
	Request.ActionTag = ARGameplayTags::Action_Roll;
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

	UARPassiveRelicDefinition* Definition = NewObject<UARPassiveRelicDefinition>();
	Definition->DefinitionTag = ARGameplayTags::Item_Type_PassiveRelic;
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
	Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
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

	UARConsumableDefinition* Definition = NewObject<UARConsumableDefinition>();
	Definition->DefinitionTag = ARGameplayTags::Item_Type_Consumable;
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_Consumable;
	Definition->RuntimeBehaviorClass = UARConsumableInstance::StaticClass();
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

	UARPassiveRelicDefinition* FirstDefinition = NewObject<UARPassiveRelicDefinition>();
	FirstDefinition->DefinitionTag = ARGameplayTags::Item_Type_PassiveRelic;
	FirstDefinition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
	FirstDefinition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
	UARPassiveRelicDefinition* SecondDefinition = NewObject<UARPassiveRelicDefinition>();
	SecondDefinition->DefinitionTag = ARGameplayTags::Item_Type_PassiveRelic;
	SecondDefinition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
	SecondDefinition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();

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

	UARPassiveRelicDefinition* Definition = NewObject<UARPassiveRelicDefinition>();
	Definition->DefinitionTag = ARGameplayTags::Item_Type_PassiveRelic;
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
	Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
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
	TestTrue(TEXT("A different input group may execute concurrently"), SecondaryStatus.IsSuccess() && SecondaryGroup.IsValid());
	TestEqual(TEXT("Secondary action is added without disturbing the primary group"), Player->GetActionComponent()->GetActiveActionCount(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARConsumableSlotReductionTest,
	"AR.Foundation.Items.ConsumableSlotReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARConsumableSlotReductionTest::RunTest(const FString& Parameters)
{
	ARFoundationTests::FScopedTestWorld TestWorld;
	AARPlayerCharacter* Player = TestWorld.World->SpawnActor<AARPlayerCharacter>();
	TestNotNull(TEXT("Player spawned"), Player);
	UARConsumableComponent* Consumables = Player->GetConsumableComponent();
	Consumables->BeginPlay();

	UARConsumableDefinition* Definition = NewObject<UARConsumableDefinition>();
	Definition->DefinitionTag = ARGameplayTags::Item_Type_Consumable;
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_Consumable;
	Definition->RuntimeBehaviorClass = UARConsumableInstance::StaticClass();
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

	UARConsumableDefinition* UsedDefinition = nullptr;
	const FARRequestStatus UseStatus = Consumables->TryUseConsumableSlot(0, UsedDefinition);
	TestTrue(TEXT("Retained consumable can be used"), UseStatus.IsSuccess());
	TestTrue(TEXT("Use returns the consumed definition"), UsedDefinition == Definition);
	TestFalse(TEXT("Used slot becomes empty"), Consumables->GetConsumableSlots()[0].bOccupied);
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
	Request.ActionTag = ARGameplayTags::Action_Roll;
	Request.bBlockBasicMovementWhileActive = true;
	Request.CancelRules.bCancelOnStagger = true;
	FARRequestStatus StartStatus;
	const FARActionHandle Handle = Action->TryStartAction(Request, StartStatus);
	TestTrue(TEXT("Action starts"), StartStatus.IsSuccess() && Handle.IsValid());
	TestFalse(TEXT("Action-owned movement lock blocks basic movement"), Movement->CanBasicMove());

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

	AActor* HitboxActor = TestWorld.World->SpawnActor<AActor>();
	TestTrue(TEXT("Action accepts an owned hitbox actor"), Action->RegisterActionHitbox(Handle, HitboxActor));
	TestEqual(TEXT("Stagger reason cancels the action"), Action->CancelActionsByReason(EARActionCancelReason::Stagger), 1);
	TestFalse(TEXT("Cancelled action is no longer active"), Action->IsActionActive(Handle));
	TestEqual(TEXT("Cancellation removes the temporary stat modifier"), Stats->GetFinalStat(EARStatType::AttackPower), 100.0f);
	TestFalse(TEXT("Cancellation removes action-owned super armor"), Stagger->IsSuperArmorActive());
	TestTrue(TEXT("Cancellation releases the movement lock"), Movement->CanBasicMove());
	TestTrue(TEXT("Cancellation destroys the registered hitbox actor"), HitboxActor->IsActorBeingDestroyed());
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
	Player->GetLoadoutComponent()->BeginPlay();
	Player->GetConsumableComponent()->BeginPlay();
	UARUIManagerComponent* UI = Player->GetUIManagerComponent();
	UI->BeginPlay();

	const FARPlayerHUDSnapshot Snapshot = UI->GetHUDSnapshot();
	TestEqual(TEXT("HUD snapshot includes current health"), Snapshot.Health.Current, 100.0f);
	TestEqual(TEXT("HUD snapshot includes current mana"), Snapshot.Mana.Current, 100.0f);
	TestEqual(TEXT("HUD snapshot includes current stamina"), Snapshot.Stamina.Current, 100.0f);
	TestEqual(TEXT("HUD snapshot exposes the default consumable slot count"), Snapshot.Consumables.Num(), 3);

	const FARRequestStatus OpenStatus = UI->OpenScreen(EARUIScreen::Inventory);
	TestTrue(TEXT("Inventory screen opens"), OpenStatus.IsSuccess());
	TestTrue(TEXT("Opening a screen blocks gameplay input"), Player->IsGameplayInputBlocked());
	TestEqual(TEXT("Only one screen may be open"), UI->OpenScreen(EARUIScreen::Shop).Result, EARRequestResult::Blocked);
	TestEqual(TEXT("Blocked request does not replace the current screen"), UI->GetCurrentScreen(), EARUIScreen::Inventory);
	TestTrue(TEXT("Current screen closes"), UI->CloseCurrentScreen());
	TestFalse(TEXT("Closing the screen restores gameplay input"), Player->IsGameplayInputBlocked());

	UARPassiveRelicDefinition* UnownedDefinition = NewObject<UARPassiveRelicDefinition>();
	UnownedDefinition->DefinitionTag = ARGameplayTags::Item_Type_PassiveRelic;
	UnownedDefinition->ItemTypeTag = ARGameplayTags::Item_Type_PassiveRelic;
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
	return true;
}

#endif
