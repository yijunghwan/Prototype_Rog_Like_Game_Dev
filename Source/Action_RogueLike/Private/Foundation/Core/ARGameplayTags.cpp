#include "Foundation/Core/ARGameplayTags.h"

namespace ARGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Combat_Team_Player, "Combat.Team.Player");
	UE_DEFINE_GAMEPLAY_TAG(Combat_Team_Enemy, "Combat.Team.Enemy");
	UE_DEFINE_GAMEPLAY_TAG(Combat_Team_Environment, "Combat.Team.Environment");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Attribute_Physical, "Damage.Attribute.Physical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Attribute_Fire, "Damage.Attribute.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Attribute_Magic, "Damage.Attribute.Magic");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Attribute_Void, "Damage.Attribute.Void");

	UE_DEFINE_GAMEPLAY_TAG(Damage_Delivery_Direct, "Damage.Delivery.Direct");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Delivery_DOT, "Damage.Delivery.DOT");

	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_Primary, "Input.Skill.Primary");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_1, "Input.Skill.1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Skill_2, "Input.Skill.2");
	UE_DEFINE_GAMEPLAY_TAG(Action_Roll, "Action.Roll");

	UE_DEFINE_GAMEPLAY_TAG(Status_Stun, "Status.Stun");
	UE_DEFINE_GAMEPLAY_TAG(Status_Root, "Status.Root");

	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Weapon, "Item.Type.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_ActiveRelic, "Item.Type.ActiveRelic");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_PassiveRelic, "Item.Type.PassiveRelic");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Consumable, "Item.Type.Consumable");
}
