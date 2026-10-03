#include "Foundation/Items/ARItemDefinition.h"

#include "Foundation/Core/ARGameplayTags.h"

#if WITH_EDITOR
#include "Foundation/Items/ARConsumableInstance.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"
#endif

FPrimaryAssetId UARItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ARItem"), GetFName());
}

EARLoadoutItemKind UARItemDefinition::GetItemKind() const
{
	if (ItemTypeTag == ARGameplayTags::Item_Type_Weapon) return EARLoadoutItemKind::Weapon;
	if (ItemTypeTag == ARGameplayTags::Item_Type_ActiveRelic) return EARLoadoutItemKind::ActiveRelic;
	return EARLoadoutItemKind::PassiveRelic;
}

bool UARItemDefinition::IsConsumable() const
{
	return ItemTypeTag == ARGameplayTags::Item_Type_Consumable;
}

bool UARItemDefinition::HasAdditionalTag(const FString& Tag) const
{
	const FString Requested = Tag.TrimStartAndEnd();
	return !Requested.IsEmpty() && AdditionalTags.ContainsByPredicate([&Requested](const FString& Candidate)
	{
		return Candidate.TrimStartAndEnd().Equals(Requested, ESearchCase::IgnoreCase);
	});
}

#if WITH_EDITOR
EDataValidationResult UARItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	const auto Warn = [this, &Context](const FString& Message)
	{
		Context.AddMessage(EMessageSeverity::Warning, FText::FromString(TEXT("[AR 아이템] ") + Message))
			->AddToken(FUObjectToken::Create(this));
	};
	const bool bConsumable = IsConsumable();
	const bool bLoadout = ItemTypeTag == ARGameplayTags::Item_Type_Weapon
		|| ItemTypeTag == ARGameplayTags::Item_Type_ActiveRelic
		|| ItemTypeTag == ARGameplayTags::Item_Type_PassiveRelic;
	if (!bConsumable && !bLoadout)
	{
		Warn(TEXT("Item Type Tag가 지원되는 무기·액티브 유물·패시브 유물·소모품 유형이 아닙니다."));
	}
	if (ItemId <= 0) Warn(TEXT("Item Id는 1 이상이어야 합니다. 현재 값으로는 획득할 수 없습니다."));
	if (!RuntimeBehaviorClass)
	{
		Warn(TEXT("Runtime Behavior Class가 비어 있습니다. 데이터 전용 아이템도 해당 유형의 런타임 클래스를 지정하세요."));
	}
	else
	{
		if (RuntimeBehaviorClass->HasAnyClassFlags(CLASS_Abstract))
		{
			Warn(TEXT("Runtime Behavior Class가 추상 클래스입니다. 생성 가능한 클래스를 지정하세요."));
		}
		if ((bConsumable && !RuntimeBehaviorClass->IsChildOf(UARConsumableInstance::StaticClass()))
			|| (bLoadout && !RuntimeBehaviorClass->IsChildOf(UARLoadoutItemInstance::StaticClass())))
		{
			Warn(TEXT("Runtime Behavior Class의 부모가 아이템 유형과 맞지 않습니다. 소모품은 ARConsumableInstance, 무기·유물은 ARLoadoutItemInstance 자식을 사용하세요."));
		}
	}
	if (bConsumable && (!SkillDefinitions.IsEmpty() || !DefaultStatModifiers.IsEmpty()))
	{
		Warn(TEXT("소모품의 Skill Definitions와 Default Stat Modifiers는 현재 소모품 경로에서 사용하지 않습니다. 효과는 소모품 런타임에서 구현하세요."));
	}
	if (bLoadout)
	{
		TSet<FName> SeenSkillIds;
		for (int32 Index = 0; Index < SkillDefinitions.Num(); ++Index)
		{
			const FARSkillDefinition& Skill = SkillDefinitions[Index];
			const FString Prefix = FString::Printf(TEXT("Skill Definitions[%d] (%s): "), Index, *Skill.SkillId.ToString());
			if (Skill.SkillId.IsNone()) Warn(Prefix + TEXT("Skill Id가 비어 있습니다."));
			else if (SeenSkillIds.Contains(Skill.SkillId)) Warn(Prefix + TEXT("같은 아이템 안에 중복된 Skill Id가 있습니다. 각 스킬의 ID를 구분하세요."));
			SeenSkillIds.Add(Skill.SkillId);
			const bool bValidInput = Skill.InputMode == EARSkillInputMode::DirectTag
				? Skill.InputTag.IsValid()
				: Skill.InputMode == EARSkillInputMode::ActiveRelicSlot
					&& ItemTypeTag == ARGameplayTags::Item_Type_ActiveRelic && Skill.SlotSkillIndex > 0;
			if (!bValidInput) Warn(Prefix + TEXT("입력 설정이 유효하지 않습니다. Direct Tag는 Input Tag가 필요하며, Active Relic Slot은 액티브 유물에서만 사용하고 Slot Skill Index를 1 이상으로 지정하세요."));
			if (!FMath::IsFinite(Skill.ResourceCost.Mana) || !FMath::IsFinite(Skill.ResourceCost.Stamina)
				|| Skill.ResourceCost.Mana < 0.f || Skill.ResourceCost.Stamina < 0.f)
			{
				Warn(Prefix + TEXT("마나·스태미나 비용은 유한한 0 이상의 값이어야 합니다."));
			}
			if (!FMath::IsFinite(Skill.BaseCooldown) || !FMath::IsFinite(Skill.MinimumCooldown)
				|| Skill.BaseCooldown < 0.f || Skill.MinimumCooldown < 0.f)
			{
				Warn(Prefix + TEXT("기본·최소 쿨타임은 유한한 0 이상의 값이어야 합니다."));
			}
			if ((Skill.CooldownTimeGroup != EARTimeGroup::World && Skill.CooldownTimeGroup != EARTimeGroup::Player)
				|| (Skill.ActionRequest.TimeGroup != EARTimeGroup::World && Skill.ActionRequest.TimeGroup != EARTimeGroup::Player))
			{
				Warn(Prefix + TEXT("쿨타임·액션 시간 그룹은 World 또는 Player여야 합니다."));
			}
		}
		for (int32 Index = 0; Index < DefaultStatModifiers.Num(); ++Index)
		{
			if (DefaultStatModifiers[Index].Operation == EARStatModifierOperation::PermanentFlat)
			{
				Warn(FString::Printf(TEXT("Default Stat Modifiers[%d]: Permanent Flat은 아이템 귀속 효과에서 사용할 수 없습니다. 영구 변경은 일반 Apply Stat Modifier에서 의도적으로 적용하세요."), Index));
			}
		}
	}
	// These checks are advisory, not new restrictions. Preserve parent errors.
	return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif
