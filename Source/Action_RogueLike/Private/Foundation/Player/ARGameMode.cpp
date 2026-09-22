#include "Foundation/Player/ARGameMode.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Player/ARPlayerController.h"

AARGameMode::AARGameMode()
{
	DefaultPawnClass = AARPlayerCharacter::StaticClass();
	PlayerControllerClass = AARPlayerController::StaticClass();
}

