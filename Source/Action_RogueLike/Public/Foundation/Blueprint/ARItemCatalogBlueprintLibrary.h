#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Foundation/Core/ARFoundationTypes.h"
#include "ARItemCatalogBlueprintLibrary.generated.h"

class AARPlayerCharacter;
class UARItemDefinition;

/** Blueprint entry points for the unified item catalog and type-routed acquisition. */
UCLASS()
class ACTION_ROGUELIKE_API UARItemCatalogBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** A key is unique within its item type. Ambiguous keys fail instead of choosing an arbitrary asset. */
	UFUNCTION(BlueprintCallable, Category="AR|Item|Catalog")
	static UARItemDefinition* FindItemByKey(FGameplayTag ItemTypeTag, int32 ItemId);

	/** Check every saved item definition for missing fields, wrong runtime family, and duplicate (type, id) keys. */
	UFUNCTION(BlueprintCallable, Category="AR|Item|Catalog")
	static bool ValidateItemCatalog(TArray<FString>& Errors);

	/** Free-form labels are shared; this query may return multiple definitions. */
	UFUNCTION(BlueprintCallable, Category="AR|Item|Catalog")
	static TArray<UARItemDefinition*> FindItemsByAdditionalTag(const FString& AdditionalTag, FGameplayTag TypeFilter);

	/** Counts owned copies, not unique definitions. Invalid TypeFilter includes every item type. */
	UFUNCTION(BlueprintPure, Category="AR|Item|Catalog")
	static int32 CountOwnedItemsByAdditionalTag(const AARPlayerCharacter* Player, const FString& AdditionalTag, FGameplayTag TypeFilter);

	/** Dispatches to loadout or consumable acquisition without exposing two paths to content Blueprints. */
	UFUNCTION(BlueprintCallable, Category="AR|Item")
	static FARRequestStatus TryAcquireItem(AARPlayerCharacter* Player, UARItemDefinition* Definition);
};
