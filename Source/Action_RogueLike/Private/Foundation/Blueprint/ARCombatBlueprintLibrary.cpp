#include "Foundation/Blueprint/ARCombatBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Foundation/Combat/ARCombatSubsystem.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"
#include "Foundation/Components/ARHealthComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Status/ARStatusEffectDefinition.h"

namespace
{
	UARCombatSubsystem* GetCombatSubsystem(const UObject* WorldContextObject)
	{
		const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		return World ? World->GetSubsystem<UARCombatSubsystem>() : nullptr;
	}
}

bool UARCombatBlueprintLibrary::CanDamageTarget(const UObject* WorldContextObject, AActor* Attacker, AActor* Target, EARRequestResult& FailureReason)
{
	if (const UARCombatSubsystem* Combat = GetCombatSubsystem(WorldContextObject))
	{
		return Combat->CanDamageTarget(Attacker, Target, FailureReason);
	}
	FailureReason = EARRequestResult::InvalidOwner;
	return false;
}

FARCombatDamageResult UARCombatBlueprintLibrary::ApplyCombatDamage(const UObject* WorldContextObject, const FARCombatDamageRequest& Request)
{
	if (UARCombatSubsystem* Combat = GetCombatSubsystem(WorldContextObject))
	{
		return Combat->ApplyCombatDamage(Request);
	}
	FARCombatDamageResult Result;
	Result.FailureReason = EARRequestResult::InvalidOwner;
	return Result;
}

FARDotHandle UARCombatBlueprintLibrary::ApplyDamageOverTime(const UObject* WorldContextObject, const FARDamageOverTimeSpec& Spec, bool& bSuccess, EARRequestResult& FailureReason)
{
	if (UARCombatSubsystem* Combat = GetCombatSubsystem(WorldContextObject))
	{
		return Combat->ApplyDamageOverTime(Spec, bSuccess, FailureReason);
	}
	bSuccess = false;
	FailureReason = EARRequestResult::InvalidOwner;
	return FARDotHandle();
}

bool UARCombatBlueprintLibrary::RemoveDamageOverTime(const UObject* WorldContextObject, FARDotHandle Handle)
{
	if (UARCombatSubsystem* Combat = GetCombatSubsystem(WorldContextObject))
	{
		return Combat->RemoveDamageOverTime(Handle);
	}
	return false;
}

FARStaggerResult UARCombatBlueprintLibrary::ApplyStaggerAndGroggyDamage(const FARStaggerRequest& Request)
{
	if (Request.HitContext.Target)
	{
		if (UARStaggerComponent* Stagger = Request.HitContext.Target->FindComponentByClass<UARStaggerComponent>())
		{
			return Stagger->ApplyStaggerAndGroggyDamage(Request);
		}
	}
	return FARStaggerResult();
}

FARStaggerResult UARCombatBlueprintLibrary::ApplyStaggerAndGroggyDamageFromResult(const FARCombatDamageResult& DamageResult,
	float BaseStaggerDamage, float StaggerMultiplier, float BaseGroggyDamage, const FARSourceInfo& Source, FName EffectName)
{
	if (!DamageResult.WasApplied() || !DamageResult.HitContext.IsUsable())
	{
		return FARStaggerResult();
	}

	FARStaggerRequest Request;
	Request.HitContext = DamageResult.HitContext;
	Request.Template.BaseStaggerDamage = BaseStaggerDamage;
	Request.Template.StaggerMultiplier = StaggerMultiplier;
	Request.Template.BaseGroggyDamage = BaseGroggyDamage;
	Request.Template.Source = Source;
	Request.Template.EffectName = EffectName;
	return ApplyStaggerAndGroggyDamage(Request);
}

FARSuperArmorHandle UARCombatBlueprintLibrary::ApplySuperArmor(AActor* Target, const FARSuperArmorSpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	if (UARStaggerComponent* Stagger = Target ? Target->FindComponentByClass<UARStaggerComponent>() : nullptr)
	{
		return Stagger->AddSuperArmor(Spec, bSuccess);
	}
	return FARSuperArmorHandle();
}

bool UARCombatBlueprintLibrary::RemoveSuperArmor(AActor* Target, FARSuperArmorHandle Handle)
{
	if (UARStaggerComponent* Stagger = Target ? Target->FindComponentByClass<UARStaggerComponent>() : nullptr)
	{
		return Stagger->RemoveSuperArmor(Handle);
	}
	return false;
}

FARCCImmunityHandle UARCombatBlueprintLibrary::ApplyCCImmunity(AActor* Target, const FARCCImmunitySpec& Spec, bool& bSuccess)
{
	bSuccess = false;
	if (UARStatusEffectComponent* Status = Target ? Target->FindComponentByClass<UARStatusEffectComponent>() : nullptr)
	{
		return Status->AddCCImmunity(Spec, bSuccess);
	}
	return FARCCImmunityHandle();
}

bool UARCombatBlueprintLibrary::RemoveCCImmunity(AActor* Target, FARCCImmunityHandle Handle)
{
	if (UARStatusEffectComponent* Status = Target ? Target->FindComponentByClass<UARStatusEffectComponent>() : nullptr)
	{
		return Status->RemoveCCImmunity(Handle);
	}
	return false;
}

FARStatusEffectHandle UARCombatBlueprintLibrary::ApplyCrowdControl(AActor* Target, EARCrowdControlType CCType,
	float Duration, bool bAffectedByTenacity, bool& bSuccess, EARRequestResult& FailureReason,
	float& AppliedDuration, const FARSourceInfo& Source)
{
	bSuccess = false;
	AppliedDuration = 0.0f;
	FailureReason = EARRequestResult::InvalidTarget;
	UARStatusEffectComponent* Status = IsValid(Target) ? Target->FindComponentByClass<UARStatusEffectComponent>() : nullptr;
	if (!Status)
	{
		return FARStatusEffectHandle();
	}
	if (const UARHealthComponent* Health = Target->FindComponentByClass<UARHealthComponent>(); Health && Health->IsDead())
	{
		return FARStatusEffectHandle();
	}
	if (!FMath::IsFinite(Duration) || Duration <= 0.0f
		|| (CCType != EARCrowdControlType::Stun && CCType != EARCrowdControlType::Root))
	{
		FailureReason = EARRequestResult::InvalidDefinition;
		return FARStatusEffectHandle();
	}

	// Never mutate an asset/CDO. The active status keeps this transient definition
	// alive via its reflected reference, and releases it on removal/expiry.
	const bool bStun = CCType == EARCrowdControlType::Stun;
	UARStatusEffectDefinition* Definition = NewObject<UARStatusEffectDefinition>(Status, NAME_None, RF_Transient);
	Definition->StatusTag = bStun ? ARGameplayTags::Status_Stun : ARGameplayTags::Status_Root;
	Definition->BaseDuration = Duration;
	Definition->bAffectedByTenacity = bAffectedByTenacity;
	Definition->bIsCrowdControl = true;
	Definition->bCanBeImmune = true;
	Definition->bBlocksBasicMovement = true;
	Definition->bBlocksAllMovement = bStun;
	Definition->bBlocksRoll = true;
	Definition->bBlocksSkillGroups = bStun;
	Definition->bCancelActionsOnApply = bStun;
	Definition->ActionCancelReason = bStun ? EARActionCancelReason::Stun : EARActionCancelReason::Root;
	FARStatusEffectRequest Request;
	Request.Definition = Definition;
	Request.Source = Source;
	const FARStatusEffectResult Result = Status->ApplyStatusEffect(Request);
	FailureReason = Result.Result;
	bSuccess = Result.Result == EARRequestResult::Success;
	AppliedDuration = Result.AppliedDuration;
	return Result.Handle;
}

FARStatusEffectResult UARCombatBlueprintLibrary::ApplyStatusEffect(AActor* Target, const FARStatusEffectRequest& Request)
{
	if (UARStatusEffectComponent* Status = Target ? Target->FindComponentByClass<UARStatusEffectComponent>() : nullptr)
	{
		return Status->ApplyStatusEffect(Request);
	}
	FARStatusEffectResult Result;
	Result.Result = EARRequestResult::InvalidTarget;
	return Result;
}

bool UARCombatBlueprintLibrary::RemoveStatusEffect(AActor* Target, FARStatusEffectHandle Handle)
{
	if (UARStatusEffectComponent* Status = Target ? Target->FindComponentByClass<UARStatusEffectComponent>() : nullptr)
	{
		return Status->RemoveStatusEffect(Handle);
	}
	return false;
}
