#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Foundation/Items/ARItemDefinition.h"
#include "ARItemDefinitionDetails.h"

class FActionRogueLikeEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomClassLayout(UARItemDefinition::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FARItemDefinitionDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyEditor.UnregisterCustomClassLayout(UARItemDefinition::StaticClass()->GetFName());
			PropertyEditor.NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE(FActionRogueLikeEditorModule, Action_RogueLikeEditor)
