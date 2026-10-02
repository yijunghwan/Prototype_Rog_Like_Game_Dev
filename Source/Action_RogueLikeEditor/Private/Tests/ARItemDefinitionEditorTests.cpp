#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Foundation/Items/ARItemDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARItemDefinitionEditConditionsTest,
	"AR.Editor.ItemDefinition.EditConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARItemDefinitionEditConditionsTest::RunTest(const FString& Parameters)
{
	const auto CheckCondition = [this](const UStruct* Struct, FName Field, const TCHAR* Expected)
	{
		const FProperty* Property = FindFProperty<FProperty>(Struct, Field);
		if (!TestNotNull(*Field.ToString(), Property)) return;
		TestEqual(*(Field.ToString() + TEXT(" edit condition")), Property->GetMetaData(TEXT("EditCondition")), FString(Expected));
		TestTrue(*(Field.ToString() + TEXT(" hides when unused")), Property->HasMetaData(TEXT("EditConditionHides")));
	};
	const UScriptStruct* Spec = FARStatModifierSpec::StaticStruct();
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, StatType), TEXT("!bStackOnly"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, Operation), TEXT("!bStackOnly"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, Value), TEXT("!bStackOnly"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, Duration), TEXT("Operation != EARStatModifierOperation::PermanentFlat"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, Source), TEXT("Operation != EARStatModifierOperation::PermanentFlat"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, HUDDisplay), TEXT("Operation != EARStatModifierOperation::PermanentFlat"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, bAffectedByTenacity), TEXT("Operation != EARStatModifierOperation::PermanentFlat && Duration > 0.0"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, bGuaranteeInvulnerability),
		TEXT("bGuaranteeInvulnerability || (!bStackOnly && Operation != EARStatModifierOperation::PermanentFlat && StatType == EARStatType::OverallDamageReduction)"));
	CheckCondition(Spec, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, bGuaranteeEvasion),
		TEXT("bGuaranteeEvasion || (!bStackOnly && Operation != EARStatModifierOperation::PermanentFlat && StatType == EARStatType::Evasion)"));
	CheckCondition(FARSkillDefinition::StaticStruct(), GET_MEMBER_NAME_CHECKED(FARSkillDefinition, InputTag),
		TEXT("InputMode == EARSkillInputMode::DirectTag"));
	CheckCondition(FARSkillDefinition::StaticStruct(), GET_MEMBER_NAME_CHECKED(FARSkillDefinition, SlotSkillIndex),
		TEXT("InputMode == EARSkillInputMode::ActiveRelicSlot"));
	CheckCondition(FARSkillDefinition::StaticStruct(), GET_MEMBER_NAME_CHECKED(FARSkillDefinition, HUDSortOrder), TEXT("bShowOnHUD"));
	CheckCondition(FARUIStateDisplayDefinition::StaticStruct(), GET_MEMBER_NAME_CHECKED(FARUIStateDisplayDefinition, MaxDisplaySlots),
		TEXT("DisplayType==EARItemUIStateDisplayType::SmallStack"));
	const FProperty* Type = FindFProperty<FProperty>(UARItemDefinition::StaticClass(), GET_MEMBER_NAME_CHECKED(UARItemDefinition, ItemTypeTag));
	if (TestNotNull(TEXT("Item type property"), Type))
	{
		TestEqual(TEXT("Item type picker filter"), Type->GetMetaData(TEXT("Categories")), FString(TEXT("Item.Type")));
	}
	return true;
}

#endif
