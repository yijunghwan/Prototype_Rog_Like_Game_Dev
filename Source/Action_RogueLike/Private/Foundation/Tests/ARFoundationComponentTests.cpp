#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "InputActionValue.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/BoxComponent.h"
#include "Foundation/Actions/ARActionTypes.h"
#include "Foundation/AI/ARAIController.h"
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
	AttackAction.ActionTag = ARGameplayTags::Action_Roll;
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
	TestEqual(TEXT("A different input group cannot interrupt an active group"), SecondaryStatus.Result, EARRequestResult::Blocked);
	TestFalse(TEXT("Rejected secondary group has no handle"), SecondaryGroup.IsValid());
	TestEqual(TEXT("Rejected input leaves the primary actions intact"), Player->GetActionComponent()->GetActiveActionCount(), 2);
	TestEqual(TEXT("Rejected input does not spend mana"), Player->GetManaComponent()->GetCurrent(), 60.0f);
	Player->GetActionComponent()->CancelAllActions();
	TestTrue(TEXT("Another group starts after the previous group finishes"),
		Loadout->HandleSkillInput(ARGameplayTags::Input_Skill_1, SecondaryGroup).IsSuccess());
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

	AActor* HitboxActor = TestWorld.World->SpawnActor<AActor>();
	TestTrue(TEXT("Action accepts an owned hitbox actor"), Action->RegisterActionHitbox(Handle, HitboxActor));
	TestEqual(TEXT("Stagger reason cancels the action"), Action->CancelActionsByReason(EARActionCancelReason::Stagger), 1);
	TestFalse(TEXT("Cancelled action is no longer active"), Action->IsActionActive(Handle));
	TestEqual(TEXT("Cancellation removes the temporary stat modifier"), Stats->GetFinalStat(EARStatType::AttackPower), 100.0f);
	TestFalse(TEXT("Cancellation removes action-owned super armor"), Stagger->IsSuperArmorActive());
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

	UARConsumableDefinition* ConsumableDefinition = NewObject<UARConsumableDefinition>();
	ConsumableDefinition->DefinitionTag = ARGameplayTags::Item_Type_Consumable;
	ConsumableDefinition->ItemTypeTag = ARGameplayTags::Item_Type_Consumable;
	ConsumableDefinition->DisplayName = FText::FromString(TEXT("Preview potion"));
	ConsumableDefinition->RuntimeBehaviorClass = UARConsumableInstance::StaticClass();
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
		nullptr, TEXT("/Game/Game/Foundation/Test/test_Player/BP_test_Player.BP_test_Player_C"));
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
		UARWeaponDefinition* Definition = NewObject<UARWeaponDefinition>(GetTransientPackage(), FName(Name));
		Definition->DefinitionTag = ARGameplayTags::Item_Type_Weapon;
		Definition->ItemTypeTag = ARGameplayTags::Item_Type_Weapon;
		Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
		Definition->EvolutionGroupId = Group;
		Definition->EvolutionStage = Stage;
		return Definition;
	};

	const FName EvolutionGroup(TEXT("Test.Caliburn"));
	UARWeaponDefinition* Stage1 = MakeWeapon(TEXT("TestWeaponStage1"), 1, EvolutionGroup);
	UARWeaponDefinition* Stage2 = MakeWeapon(TEXT("TestWeaponStage2"), 2, EvolutionGroup);
	UARWeaponDefinition* Stage3A = MakeWeapon(TEXT("TestWeaponStage3A"), 3, EvolutionGroup);
	UARWeaponDefinition* Stage3B = MakeWeapon(TEXT("TestWeaponStage3B"), 3, EvolutionGroup);
	UARWeaponDefinition* WrongGroup = MakeWeapon(TEXT("TestWeaponWrongGroup"), 3, TEXT("Test.Other"));
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
	Skill.ActionTag = ARGameplayTags::Input_Skill_Primary;
	FARActionRequest Roll;
	Roll.ActionTag = ARGameplayTags::Action_Roll;
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
	UARConsumableDefinition* Definition = NewObject<UARConsumableDefinition>();
	Definition->DefinitionTag = ARGameplayTags::Item_Type_Consumable;
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_Consumable;
	Definition->RuntimeBehaviorClass = UARConsumableInstance::StaticClass();
	UARConsumableComponent* Consumables = Player->GetConsumableComponent();
	UARUIManagerComponent* UI = Player->GetUIManagerComponent();
	TestTrue(TEXT("Consumable acquired"), Consumables->TryAcquireConsumable(Definition).Status.IsSuccess());
	UI->OpenScreen(EARUIScreen::Inventory);
	TestTrue(TEXT("Inventory still blocks gameplay hotkeys"), Player->IsGameplayInputBlocked());
	UARConsumableDefinition* UsedDefinition = nullptr;
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

#endif
