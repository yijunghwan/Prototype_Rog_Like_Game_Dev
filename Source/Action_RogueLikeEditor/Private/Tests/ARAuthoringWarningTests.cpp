#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include "Foundation/Items/ARConsumableInstance.h"
#include <limits>

namespace ARAuthoringWarningTests
{
	UARItemDefinition* MakeDefinition()
	{
		auto* Definition = NewObject<UARItemDefinition>();
		Definition->ItemTypeTag = ARGameplayTags::Item_Type_Weapon;
		Definition->ItemId = 1;
		Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
		FARSkillDefinition Skill;
		Skill.SkillId = TEXT("Fire");
		Skill.InputTag = ARGameplayTags::Input_Skill_Primary;
		Definition->SkillDefinitions.Add(Skill);
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARItemAdvisoryValidationTest, "AR.Editor.ItemDefinition.AdvisoryWarnings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARItemAdvisoryValidationTest::RunTest(const FString& Parameters)
{
	auto* Definition = ARAuthoringWarningTests::MakeDefinition();
	FDataValidationContext Valid;
	TestEqual(TEXT("Valid weapon passes"), Definition->IsDataValid(Valid), EDataValidationResult::Valid);
	TestEqual(TEXT("No warnings for valid data"), Valid.GetNumWarnings(), 0u);
	Definition->RuntimeBehaviorClass = nullptr;
	const FARSkillDefinition Duplicate = Definition->SkillDefinitions[0];
	Definition->SkillDefinitions.Add(Duplicate);
	FARStatModifierSpec Spec;
	Spec.Operation = EARStatModifierOperation::PermanentFlat;
	Definition->DefaultStatModifiers.Add(Spec);
	FDataValidationContext Warning;
	TestEqual(TEXT("Warnings are advisory, not a new validation failure"), Definition->IsDataValid(Warning), EDataValidationResult::Valid);
	TestEqual(TEXT("Missing runtime, duplicate ID, owned permanent operation"), Warning.GetNumWarnings(), 3u);
	TestEqual(TEXT("No blocking errors"), Warning.GetNumErrors(), 0u);
	for (const auto& Issue : Warning.GetIssues())
	{
		TestTrue(TEXT("Warning has clickable object token"), Issue.TokenizedMessage.IsValid()
			&& Issue.TokenizedMessage->GetMessageTokens().ContainsByPredicate([](const TSharedRef<IMessageToken>& Token)
			{
				return Token->GetType() == EMessageToken::Object;
			}));
	}
	TestNull(TEXT("Validation never fills runtime automatically"), Definition->RuntimeBehaviorClass.Get());
	TestEqual(TEXT("Validation never renames duplicated skills"), Definition->SkillDefinitions[0].SkillId, Definition->SkillDefinitions[1].SkillId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARItemWarningInputRulesTest, "AR.Editor.ItemDefinition.WarningInputRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARItemWarningInputRulesTest::RunTest(const FString& Parameters)
{
	auto* Definition = ARAuthoringWarningTests::MakeDefinition();
	FARSkillDefinition Second = Definition->SkillDefinitions[0];
	Second.SkillId = TEXT("Secondary");
	Definition->SkillDefinitions.Add(Second);
	FDataValidationContext SharedInput;
	Definition->IsDataValid(SharedInput);
	TestEqual(TEXT("Shared InputTag and priority are intentional and not warned"), SharedInput.GetNumWarnings(), 0u);
	Definition->SkillDefinitions[0].InputMode = EARSkillInputMode::ActiveRelicSlot;
	FDataValidationContext WrongSlot;
	Definition->IsDataValid(WrongSlot);
	TestEqual(TEXT("Slot mode on weapon warns"), WrongSlot.GetNumWarnings(), 1u);
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_ActiveRelic;
	Definition->SkillDefinitions[0].InputTag = FGameplayTag();
	FDataValidationContext Slot;
	Definition->IsDataValid(Slot);
	TestEqual(TEXT("Active relic slot does not require InputTag"), Slot.GetNumWarnings(), 0u);
	Definition->SkillDefinitions[0].SlotSkillIndex = 0;
	Definition->SkillDefinitions[1].ResourceCost.Mana = -1.f;
	Definition->SkillDefinitions[1].BaseCooldown = std::numeric_limits<float>::quiet_NaN();
	FDataValidationContext BadNumbers;
	Definition->IsDataValid(BadNumbers);
	TestEqual(TEXT("Invalid slot, cost and cooldown warn independently"), BadNumbers.GetNumWarnings(), 3u);
	Definition->SkillDefinitions.Reset();
	Definition->ItemTypeTag = ARGameplayTags::Item_Type_Consumable;
	Definition->RuntimeBehaviorClass = UARConsumableInstance::StaticClass();
	FDataValidationContext Consumable;
	Definition->IsDataValid(Consumable);
	TestEqual(TEXT("Consumable without loadout skills is valid"), Consumable.GetNumWarnings(), 0u);
	Definition->RuntimeBehaviorClass = UARLoadoutItemInstance::StaticClass();
	FDataValidationContext WrongParent;
	Definition->IsDataValid(WrongParent);
	TestEqual(TEXT("Wrong runtime parent warns"), WrongParent.GetNumWarnings(), 1u);
	return true;
}

#endif
