#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ARCreateResourceHUDCommandlet.generated.h"

/** Creates the initial editable Designer asset once; never overwrites user edits. */
UCLASS()
class UARCreateResourceHUDCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UARCreateResourceHUDCommandlet();
	virtual int32 Main(const FString& Params) override;
};
