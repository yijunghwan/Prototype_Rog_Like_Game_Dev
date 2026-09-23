#pragma once

#include "CoreMinimal.h"
#include "Foundation/Items/ARConsumableTypes.h"
#include "Foundation/Items/ARItemTypes.h"
#include "Foundation/Status/ARStatusEffectTypes.h"
#include "ARUITypes.generated.h"

UENUM(BlueprintType)
enum class EARUIScreen : uint8
{
	None,
	Inventory,
	EvolutionSelection,
	Shop,
	Dialogue,
	Menu,
	Custom
};

USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARHUDResourceValue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Current = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Maximum = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Ratio = 0.0f;
};

/** Read-only data packet used to build or refresh the always-visible player HUD. */
USTRUCT(BlueprintType)
struct ACTION_ROGUELIKE_API FARPlayerHUDSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARHUDResourceValue Health;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Shield = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARHUDResourceValue Mana;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARHUDResourceValue Stamina;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector AimDirection = FVector::ForwardVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHasAimWorldLocation = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector AimWorldLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHasEquippedWeapon = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FARLoadoutItemSnapshot EquippedWeapon;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FARLoadoutItemSnapshot> ActiveRelics;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FARRegisteredSkillUIData> Skills;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FARConsumableSlotSnapshot> Consumables;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FARStatusEffectView> StatusEffects;
};
