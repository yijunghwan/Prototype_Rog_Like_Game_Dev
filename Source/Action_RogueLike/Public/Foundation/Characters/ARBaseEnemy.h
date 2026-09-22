#pragma once

#include "CoreMinimal.h"
#include "Foundation/Characters/ARBaseCharacter.h"
#include "ARBaseEnemy.generated.h"

/** Minimal enemy base. AI and attack content belong in child Blueprint/C++ classes. */
UCLASS(Blueprintable)
class ACTION_ROGUELIKE_API AARBaseEnemy : public AARBaseCharacter
{
	GENERATED_BODY()

public:
	AARBaseEnemy();
};

