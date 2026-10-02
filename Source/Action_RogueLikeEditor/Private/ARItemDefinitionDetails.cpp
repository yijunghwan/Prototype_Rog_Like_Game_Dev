#include "ARItemDefinitionDetails.h"

#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"
#include "IPropertyUtilities.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Items/ARItemDefinition.h"

namespace
{
	void HideChild(IDetailLayoutBuilder& DetailBuilder, const TSharedPtr<IPropertyHandle>& Parent, FName ChildName)
	{
		if (!Parent || !Parent->IsValidHandle()) return;
		const TSharedPtr<IPropertyHandle> Child = Parent->GetChildHandle(ChildName, false);
		if (Child && Child->IsValidHandle()) DetailBuilder.HideProperty(Child);
	}

	void CustomizeArrayElements(IDetailLayoutBuilder& DetailBuilder, FName PropertyName,
		TFunctionRef<void(const TSharedPtr<IPropertyHandle>&)> CustomizeElement)
	{
		const TSharedRef<IPropertyHandle> Property = DetailBuilder.GetProperty(PropertyName);
		if (!Property->IsValidHandle()) return;
		// Newly added entries need the same contextual hiding as existing entries.
		if (const TSharedPtr<IPropertyHandleArray> Array = Property->AsArray())
		{
			const TWeakPtr<IPropertyUtilities> WeakUtilities = DetailBuilder.GetPropertyUtilities();
			Array->SetOnNumElementsChanged(FSimpleDelegate::CreateLambda([WeakUtilities]()
			{
				if (const TSharedPtr<IPropertyUtilities> Utilities = WeakUtilities.Pin())
				{
					Utilities->RequestForceRefresh();
				}
			}));
		}
		uint32 NumChildren = 0;
		Property->GetNumChildren(NumChildren);
		for (uint32 Index = 0; Index < NumChildren; ++Index)
		{
			const TSharedPtr<IPropertyHandle> Element = Property->GetChildHandle(Index);
			if (Element && Element->IsValidHandle()) CustomizeElement(Element);
		}
	}
}

TSharedRef<IDetailCustomization> FARItemDefinitionDetails::MakeInstance()
{
	return MakeShared<FARItemDefinitionDetails>();
}

void FARItemDefinitionDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	bool bHasWeapon = false;
	bool bAllConsumables = !Objects.IsEmpty();
	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		const UARItemDefinition* Definition = Cast<UARItemDefinition>(Object.Get());
		if (!Definition)
		{
			bAllConsumables = false;
			continue;
		}
		bHasWeapon |= Definition->ItemTypeTag == ARGameplayTags::Item_Type_Weapon;
		bAllConsumables &= Definition->IsConsumable();
	}

	if (!bHasWeapon)
	{
		DetailBuilder.HideCategory("Evolution");
	}
	if (bAllConsumables)
	{
		// These arrays are not consumed by the current consumable runtime path.
		DetailBuilder.HideCategory("Stats");
		DetailBuilder.HideCategory("Skills");
		DetailBuilder.HideCategory("UI");
	}

	// Default effects last until item removal. Registration supplies their
	// duration, group handles and source category; these are not author inputs.
	CustomizeArrayElements(DetailBuilder, GET_MEMBER_NAME_CHECKED(UARItemDefinition, DefaultStatModifiers),
		[&DetailBuilder](const TSharedPtr<IPropertyHandle>& Element)
		{
			HideChild(DetailBuilder, Element, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, Duration));
			HideChild(DetailBuilder, Element, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, bAffectedByTenacity));
			HideChild(DetailBuilder, Element, GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, StackGroupHandle));
			HideChild(DetailBuilder, Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FARStatModifierSpec, Source), false),
				GET_MEMBER_NAME_CHECKED(FARSourceInfo, Category));
		});
	CustomizeArrayElements(DetailBuilder, GET_MEMBER_NAME_CHECKED(UARItemDefinition, SkillDefinitions),
		[&DetailBuilder](const TSharedPtr<IPropertyHandle>& Element)
		{
			const TSharedPtr<IPropertyHandle> Request = Element->GetChildHandle(GET_MEMBER_NAME_CHECKED(FARSkillDefinition, ActionRequest), false);
			// Assigned for each skill execution by the loadout transaction.
			HideChild(DetailBuilder, Request, GET_MEMBER_NAME_CHECKED(FARActionRequest, SkillGroupHandle));
			HideChild(DetailBuilder, Request, GET_MEMBER_NAME_CHECKED(FARActionRequest, OwningItemInstanceId));
		});

	TSharedRef<IPropertyHandle> TypeHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UARItemDefinition, ItemTypeTag));
	if (TypeHandle->IsValidHandle())
	{
		const TWeakPtr<IPropertyUtilities> WeakUtilities = DetailBuilder.GetPropertyUtilities();
		TypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([WeakUtilities]()
		{
			if (const TSharedPtr<IPropertyUtilities> Utilities = WeakUtilities.Pin())
			{
				Utilities->RequestForceRefresh();
			}
		}));
	}
}
