#include "Foundation/Blueprint/ARCombatBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Foundation/Combat/ARCombatSubsystem.h"
#include "Foundation/Components/ARStaggerComponent.h"
#include "Foundation/Components/ARStatusEffectComponent.h"

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

