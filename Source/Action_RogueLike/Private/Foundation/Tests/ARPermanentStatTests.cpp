#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Foundation/Blueprint/ARStatsBlueprintLibrary.h"
#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Characters/ARBaseEnemy.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include <limits>

namespace ARPermanentStatTests
{
	struct FWorldScope
	{
		UWorld* World;
		FWorldScope()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARPermanentStat_%s"), *FGuid::NewGuid().ToString())), GetTransientPackage());
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
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			T* Actor = World->SpawnActor<T>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			Actor->DispatchBeginPlay();
			return Actor;
		}
	};
	FARStatModifierSpec Permanent(EARStatType Type, float Value)
	{
		FARStatModifierSpec Spec;
		Spec.StatType = Type;
		Spec.Operation = EARStatModifierOperation::PermanentFlat;
		Spec.Value = Value;
		Spec.Source.SourceId = TEXT("PermanentTest");
		return Spec;
	}
	bool Apply(AActor* Actor, const FARStatModifierSpec& Spec)
	{
		bool bSuccess = false;
		UARStatsBlueprintLibrary::ApplyStatModifier(Actor, Spec, bSuccess);
		return bSuccess;
	}
	bool HasMoneyView(UARStatsComponent* Stats)
	{
		return Stats->GetAllFinalStatViews().ContainsByPredicate([](const FARFinalStatView& View) { return View.StatType == EARStatType::Money; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPermanentStatBaseTest, "AR.Foundation.Stats.PermanentFlat.BaseAndLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARPermanentStatBaseTest::RunTest(const FString& Parameters)
{
	using namespace ARPermanentStatTests;
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARStatsComponent* Stats = Player->GetStatsComponent();
	Stats->SetBaseStat(EARStatType::AttackPower, 10.0f);
	FARStatModifierSpec Percent;
	Percent.StatType = EARStatType::AttackPower;
	Percent.Operation = EARStatModifierOperation::AdditivePercent;
	Percent.Value = 50.0f;
	bool bSuccess = false;
	const FARStatModifierHandle PercentHandle = UARStatsBlueprintLibrary::ApplyStatModifier(Player, Percent, bSuccess);
	TestTrue(TEXT("Legacy percent produces reversible handle"), bSuccess && PercentHandle.IsValid());
	FARStatModifierSpec Spec = Permanent(EARStatType::AttackPower, 10.0f);
	Spec.Duration = 0.0f; // Ignored, unlike a recorded modifier.
	Spec.HUDDisplay = EARStatEffectDisplay::Buff;
	Spec.bAffectedByTenacity = true;
	const FARStatModifierHandle Handle = UARStatsBlueprintLibrary::ApplyStatModifier(Player, Spec, bSuccess);
	TestTrue(TEXT("Permanent operation succeeds without a handle"), bSuccess && !Handle.IsValid());
	TestEqual(TEXT("Base is directly changed"), Stats->GetBaseStat(EARStatType::AttackPower), 20.0f);
	TestEqual(TEXT("Existing percent still applies"), Stats->GetFinalStat(EARStatType::AttackPower), 30.0f);
	TestFalse(TEXT("Source query finds no permanent record"), Stats->GetModifiersBySource(EARModifierSourceCategory::All, Spec.Source.SourceId).bExists);
	TestTrue(TEXT("No HUD buff record"), Stats->GetVisibleStatEffects().IsEmpty());
	TestFalse(TEXT("There is no removable handle"), UARStatsBlueprintLibrary::RemoveStatModifier(Player, Handle));
	TestEqual(TEXT("Source removal cannot undo base edit"), UARStatsBlueprintLibrary::RemoveStatModifiersBySource(Player, EARModifierSourceCategory::All, Spec.Source.SourceId), 0);
	Stats->RemoveStatModifier(PercentHandle);
	TestEqual(TEXT("Removing percent leaves new base"), Stats->GetFinalStat(EARStatType::AttackPower), 20.0f);
	for (int32 Index = 0; Index < 200; ++Index) Apply(Player, Permanent(EARStatType::AttackPower, 1.0f));
	TestEqual(TEXT("Repeated growth accumulates only the value"), Stats->GetBaseStat(EARStatType::AttackPower), 220.0f);
	TestEqual(TEXT("No modifier entries accumulated"), Stats->ClearModifiers(), 0);
	TestFalse(TEXT("Permanent-only changes do not enable tick"), Stats->IsComponentTickEnabled());
	TestTrue(TEXT("Negative permanent change succeeds"), Apply(Player, Permanent(EARStatType::AttackPower, -20.0f)));
	TestEqual(TEXT("Negative delta subtracts"), Stats->GetBaseStat(EARStatType::AttackPower), 200.0f);
	Spec = Permanent(EARStatType::AttackPower, 1.0f);
	Spec.bStackOnly = true;
	TestFalse(TEXT("Stack-only incompatible"), Apply(Player, Spec));
	Spec.bStackOnly = false;
	Spec.bGuaranteeEvasion = true;
	TestFalse(TEXT("Guarantees incompatible"), Apply(Player, Spec));
	TestFalse(TEXT("Null target is safe"), Apply(nullptr, Permanent(EARStatType::AttackPower, 1.0f)));
	TestFalse(TEXT("Non-finite delta rejected"), Apply(Player, Permanent(EARStatType::AttackPower, std::numeric_limits<float>::infinity())));
	TestFalse(TEXT("Invalid stat rejected"), Apply(Player, Permanent(EARStatType::Count, 1.0f)));
	Stats->SetBaseStat(EARStatType::AttackPower, std::numeric_limits<float>::max());
	TestFalse(TEXT("Overflow rejected atomically"), Apply(Player, Permanent(EARStatType::AttackPower, std::numeric_limits<float>::max())));
	TestEqual(TEXT("Overflow leaves base intact"), Stats->GetBaseStat(EARStatType::AttackPower), std::numeric_limits<float>::max());
	TestTrue(TEXT("Existing final clamp still applies"), Apply(Player, Permanent(EARStatType::MaxHealth, -200.0f)));
	TestEqual(TEXT("Max health final minimum preserved"), Stats->GetFinalStat(EARStatType::MaxHealth), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMoneyStatTest, "AR.Foundation.Stats.Money.PlayerOnlyBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMoneyStatTest::RunTest(const FString& Parameters)
{
	using namespace ARPermanentStatTests;
	static_assert(static_cast<uint8>(EARStatModifierOperation::Flat) == 0);
	static_assert(static_cast<uint8>(EARStatModifierOperation::IndependentDamageReduction) == 3);
	static_assert(static_cast<uint8>(EARStatModifierOperation::PermanentFlat) == 4);
	static_assert(static_cast<uint8>(EARStatType::OverallDamageTakenIncrease) == 46);
	static_assert(static_cast<uint8>(EARStatType::Money) == 47);
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARStatsComponent* Stats = Player->GetStatsComponent();
	TestEqual(TEXT("Money defaults to zero"), Stats->GetFinalStat(EARStatType::Money), 0.0f);
	TestTrue(TEXT("Player list includes Money"), HasMoneyView(Stats));
	TestTrue(TEXT("Earn money"), Apply(Player, Permanent(EARStatType::Money, 100.0f)));
	TestTrue(TEXT("Spend money"), Apply(Player, Permanent(EARStatType::Money, -30.0f)));
	TestEqual(TEXT("Money balance"), Stats->GetFinalStat(EARStatType::Money), 70.0f);
	TestFalse(TEXT("Overdraft rejected"), Apply(Player, Permanent(EARStatType::Money, -71.0f)));
	TestEqual(TEXT("Failed spend leaves balance intact"), Stats->GetBaseStat(EARStatType::Money), 70.0f);
	Stats->SetBaseStat(EARStatType::Money, -1.0f);
	TestEqual(TEXT("Direct setter cannot make negative money"), Stats->GetBaseStat(EARStatType::Money), 70.0f);
	TestTrue(TEXT("Exact spend allowed"), Apply(Player, Permanent(EARStatType::Money, -70.0f)));
	TestEqual(TEXT("Exact spend ends at zero"), Stats->GetFinalStat(EARStatType::Money), 0.0f);
	AARBaseEnemy* Enemy = Scope.Spawn<AARBaseEnemy>();
	TestFalse(TEXT("Enemy cannot receive money"), Apply(Enemy, Permanent(EARStatType::Money, 100.0f)));
	TestFalse(TEXT("Enemy list excludes money"), HasMoneyView(Enemy->GetStatsComponent()));
	Enemy->GetStatsComponent()->SetBaseStat(EARStatType::Money, 100.0f);
	TestFalse(TEXT("Money is not stored on enemy"), Enemy->GetStatsComponent()->BaseStats.Contains(EARStatType::Money));
	TestEqual(TEXT("Enemy money query returns zero"), Enemy->GetStatsComponent()->GetFinalStat(EARStatType::Money), 0.0f);
	FARStatModifierSpec Legacy = Permanent(EARStatType::Money, 10.0f);
	Legacy.Operation = EARStatModifierOperation::Flat;
	bool bSuccess = false;
	TestFalse(TEXT("Recorded money also rejects enemy"), UARStatsBlueprintLibrary::ApplyStatModifier(Enemy, Legacy, bSuccess).IsValid());
	TestFalse(TEXT("Recorded money reports failure"), bSuccess);
	const FARStatModifierHandle MoneyBuff = UARStatsBlueprintLibrary::ApplyStatModifier(Player, Legacy, bSuccess);
	TestTrue(TEXT("Player can still use reversible money adjustment"), bSuccess && MoneyBuff.IsValid());
	TestEqual(TEXT("Temporary money changes final value"), Stats->GetFinalStat(EARStatType::Money), 10.0f);
	TestFalse(TEXT("Temporary money cannot fund a base overdraft"), Apply(Player, Permanent(EARStatType::Money, -10.0f)));
	Stats->RemoveStatModifier(MoneyBuff);
	TestTrue(TEXT("Money uses existing fractional stat representation"), Apply(Player, Permanent(EARStatType::Money, 0.5f)));
	TestEqual(TEXT("Money is not implicitly rounded"), Stats->GetFinalStat(EARStatType::Money), 0.5f);
	AARPlayerCharacter* AuthoredPlayer = Scope.World->SpawnActor<AARPlayerCharacter>();
	AuthoredPlayer->GetStatsComponent()->BaseStats.Add(EARStatType::Money, -50.0f);
	AuthoredPlayer->DispatchBeginPlay();
	TestEqual(TEXT("Invalid authored starting money normalizes to zero"), AuthoredPlayer->GetStatsComponent()->GetBaseStat(EARStatType::Money), 0.0f);
	AActor* Environment = Scope.World->SpawnActor<AActor>();
	UARStatsComponent* EnvironmentalStats = NewObject<UARStatsComponent>(Environment);
	Environment->AddInstanceComponent(EnvironmentalStats);
	EnvironmentalStats->RegisterComponent();
	TestFalse(TEXT("Environment cannot receive money"), Apply(Environment, Permanent(EARStatType::Money, 100.0f)));
	TestFalse(TEXT("Environment list excludes money"), HasMoneyView(EnvironmentalStats));
	TestTrue(TEXT("Non-player ordinary stats permit permanent changes"), Apply(Enemy, Permanent(EARStatType::AttackPower, 20.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARPermanentOwnershipTest, "AR.Foundation.Stats.PermanentFlat.OwnershipGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARPermanentOwnershipTest::RunTest(const FString& Parameters)
{
	using namespace ARPermanentStatTests;
	FWorldScope Scope;
	AARPlayerCharacter* Player = Scope.Spawn<AARPlayerCharacter>();
	UARStatsComponent* Stats = Player->GetStatsComponent();
	FARStatModifierSpec Spec = Permanent(EARStatType::AttackPower, 10.0f);
	bool bSuccess = true;
	TestFalse(TEXT("Recorded component path rejects Permanent Flat"), Stats->AddStatModifier(Spec, bSuccess).IsValid());
	TestFalse(TEXT("Recorded path reports failure"), bSuccess);
	FARRequestStatus Start;
	const FARActionHandle Action = Player->GetActionComponent()->TryStartAction(FARActionRequest(), Start);
	TestTrue(TEXT("Action starts"), Action.IsValid());
	TestFalse(TEXT("Action-owned permanent rejected"), Player->GetActionComponent()->ApplyActionStatModifier(Action, Spec, bSuccess).IsValid());
	TestFalse(TEXT("Action-owned reports failure"), bSuccess);
	UARItemDefinition* Definition = NewObject<UARItemDefinition>(Player);
	UARLoadoutItemInstance* Item = NewObject<UARLoadoutItemInstance>(Player);
	Item->InitializeInstance(Player, Definition);
	TestTrue(TEXT("Empty item registers"), Item->RegisterItem());
	TestFalse(TEXT("Item-owned permanent rejected"), Item->ApplyItemStatModifier(Spec, bSuccess).IsValid());
	TestFalse(TEXT("Item-owned reports failure"), bSuccess);
	TestEqual(TEXT("Rejected owned calls do not modify base"), Stats->GetBaseStat(EARStatType::AttackPower), 0.0f);
	TestTrue(TEXT("General node in item content can apply permanent"), Apply(Item->GetItemOwner(), Spec));
	Player->GetActionComponent()->EndAction(Action);
	Item->UnregisterItem(EARItemRemovalReason::Discarded);
	TestEqual(TEXT("Owner cleanup does not undo permanent"), Stats->GetFinalStat(EARStatType::AttackPower), 10.0f);
	Definition->DefaultStatModifiers.Add(Spec);
	UARLoadoutItemInstance* InvalidItem = NewObject<UARLoadoutItemInstance>(Player);
	InvalidItem->InitializeInstance(Player, Definition);
	AddExpectedError(TEXT("Failed to apply default modifier"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("DA default permanent rejected without irreversible mutation"), InvalidItem->RegisterItem());
	TestEqual(TEXT("Failed DA registration leaves base intact"), Stats->GetBaseStat(EARStatType::AttackPower), 10.0f);
	return true;
}

#endif
