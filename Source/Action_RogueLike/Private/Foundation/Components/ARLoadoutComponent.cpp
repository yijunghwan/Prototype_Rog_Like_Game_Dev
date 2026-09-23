#include "Foundation/Components/ARLoadoutComponent.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARGameplayTags.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Items/ARLoadoutItemDefinition.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"
#include "Foundation/Interaction/ARItemPickupActors.h"
#include "Foundation/Interaction/ARWorldItemDropSubsystem.h"

#define LOCTEXT_NAMESPACE "ARLoadoutDisplay"

namespace
{
	FARProvidedStatDisplayData MakeProvidedStatDisplayData(const FARStatModifierSpec& Spec)
	{
		FARProvidedStatDisplayData Entry;
		Entry.StatType = Spec.StatType;
		Entry.Operation = Spec.Operation;
		Entry.Value = Spec.Value;
		if (const UEnum* StatEnum = StaticEnum<EARStatType>())
		{
			Entry.StatName = StatEnum->GetDisplayNameTextByValue(static_cast<int64>(Spec.StatType));
		}
		const FText Number = FText::AsNumber(Spec.Value);
		switch (Spec.Operation)
		{
		case EARStatModifierOperation::Flat:
			Entry.DisplayText = FText::Format(LOCTEXT("FlatStat", "{0}: {1}"), Entry.StatName, Number);
			break;
		case EARStatModifierOperation::AdditivePercent:
			Entry.DisplayText = FText::Format(LOCTEXT("AdditiveStat", "{0}: {1}% (additive)"), Entry.StatName, Number);
			break;
		case EARStatModifierOperation::Multiplicative:
			Entry.DisplayText = FText::Format(LOCTEXT("MultiplicativeStat", "{0}: x{1}"), Entry.StatName, Number);
			break;
		case EARStatModifierOperation::IndependentDamageReduction:
			Entry.DisplayText = FText::Format(LOCTEXT("IndependentReductionStat", "{0}: {1}% (independent)"), Entry.StatName, Number);
			break;
		default:
			Entry.DisplayText = FText::Format(LOCTEXT("UnknownStat", "{0}: {1}"), Entry.StatName, Number);
			break;
		}
		return Entry;
	}
}

UARLoadoutComponent::UARLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DroppedItemPickupClass = AARLoadoutItemPickup::StaticClass();
}

void UARLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerOwner = Cast<AARPlayerCharacter>(GetOwner());
	if (!PlayerOwner)
	{
		UE_LOG(LogARItems, Error, TEXT("LoadoutComponent requires AARPlayerCharacter owner."));
		return;
	}
	StatsComponent = PlayerOwner->GetStatsComponent();
	ActionComponent = PlayerOwner->GetActionComponent();
	ManaComponent = PlayerOwner->GetManaComponent();
	StaminaComponent = PlayerOwner->GetStaminaComponent();
	if (ActionComponent)
	{
		ActionComponent->OnActionEnded.AddDynamic(this, &UARLoadoutComponent::HandleActionEnded);
		ActionComponent->OnActionCancelled.AddDynamic(this, &UARLoadoutComponent::HandleActionCancelled);
	}
}

void UARLoadoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActionComponent)
	{
		ActionComponent->OnActionEnded.RemoveDynamic(this, &UARLoadoutComponent::HandleActionEnded);
		ActionComponent->OnActionCancelled.RemoveDynamic(this, &UARLoadoutComponent::HandleActionCancelled);
	}
	TArray<UARLoadoutItemInstance*> Instances;
	if (EquippedWeapon) Instances.Add(EquippedWeapon);
	for (UARLoadoutItemInstance* Instance : ActiveRelics) Instances.Add(Instance);
	for (UARLoadoutItemInstance* Instance : PassiveRelics) Instances.Add(Instance);
	for (UARLoadoutItemInstance* Instance : Instances)
	{
		UnregisterAndReleaseInstance(Instance, EARItemRemovalReason::OwnerDestroyed);
	}
	EquippedWeapon = nullptr;
	ActiveRelics.Reset();
	PassiveRelics.Reset();
	PendingAcquisitions.Reset();
	PendingEvolutions.Reset();
	Super::EndPlay(EndPlayReason);
}

FARLoadoutAcquisitionResult UARLoadoutComponent::BeginLoadoutAcquisition(const UARLoadoutItemDefinition* Definition)
{
	FARLoadoutAcquisitionResult Result;
	if (!ValidateDefinition(Definition, Result.Status))
	{
		return Result;
	}
	if (Definition->GetItemKind() == EARLoadoutItemKind::ActiveRelic && ActiveRelics.Num() >= MaxActiveRelics)
	{
		Result.Status.Result = EARRequestResult::SlotFull;
		return Result;
	}
	Result.Token.Id = FGuid::NewGuid();
	FARPendingAcquisition Pending;
	Pending.Definition = Definition;
	Pending.Revision = LoadoutRevision;
	PendingAcquisitions.Add(Result.Token.Id, MoveTemp(Pending));
	Result.bWillReplaceWeapon = Definition->GetItemKind() == EARLoadoutItemKind::Weapon && EquippedWeapon != nullptr;
	Result.Status.Result = EARRequestResult::Success;
	return Result;
}

UARLoadoutItemInstance* UARLoadoutComponent::CommitLoadoutAcquisition(FARAcquisitionToken Token, FARRequestStatus& Status)
{
	Status.Result = EARRequestResult::InvalidHandle;
	FARPendingAcquisition Pending;
	if (!PendingAcquisitions.RemoveAndCopyValue(Token.Id, Pending))
	{
		return nullptr;
	}
	const UARLoadoutItemDefinition* Definition = Pending.Definition.Get();
	if (Pending.Revision != LoadoutRevision)
	{
		Status.Result = EARRequestResult::StaleRequest;
		return nullptr;
	}
	if (!ValidateDefinition(Definition, Status))
	{
		return nullptr;
	}
	if (Definition->GetItemKind() == EARLoadoutItemKind::ActiveRelic && ActiveRelics.Num() >= MaxActiveRelics)
	{
		Status.Result = EARRequestResult::SlotFull;
		return nullptr;
	}

	UARLoadoutItemInstance* NewInstance = CreateAndRegisterInstance(Definition, Status);
	if (!NewInstance)
	{
		return nullptr;
	}
	if (Definition->GetItemKind() == EARLoadoutItemKind::Weapon)
	{
		if (EquippedWeapon)
		{
			UnregisterAndReleaseInstance(EquippedWeapon, EARItemRemovalReason::Replaced);
		}
		EquippedWeapon = NewInstance;
	}
	else if (Definition->GetItemKind() == EARLoadoutItemKind::ActiveRelic)
	{
		ActiveRelics.Add(NewInstance);
	}
	else
	{
		PassiveRelics.Add(NewInstance);
	}
	NotifyLoadoutChanged();
	Status.Result = EARRequestResult::Success;
	return NewInstance;
}

bool UARLoadoutComponent::CancelLoadoutAcquisition(FARAcquisitionToken Token)
{
	return PendingAcquisitions.Remove(Token.Id) > 0;
}

bool UARLoadoutComponent::CancelWeaponEvolution(FAREvolutionToken Token)
{
	return PendingEvolutions.Remove(Token.Id) > 0;
}

void UARLoadoutComponent::CancelAllPendingRequests()
{
	PendingAcquisitions.Reset();
	PendingEvolutions.Reset();
}

bool UARLoadoutComponent::DiscardLoadoutItem(FGuid InstanceId, FARLoadoutDropRequest& DropRequest, FARRequestStatus& Status)
{
	Status.Result = EARRequestResult::InvalidHandle;
	DropRequest = FARLoadoutDropRequest();
	if (EquippedWeapon && EquippedWeapon->GetInstanceId() == InstanceId)
	{
		Status.Result = EARRequestResult::Rejected;
		return false;
	}
	TArray<TObjectPtr<UARLoadoutItemInstance>>* SourceArray = nullptr;
	int32 ItemIndex = ActiveRelics.IndexOfByPredicate([InstanceId](const UARLoadoutItemInstance* Instance)
	{
		return Instance && Instance->GetInstanceId() == InstanceId;
	});
	if (ItemIndex != INDEX_NONE)
	{
		SourceArray = &ActiveRelics;
	}
	else
	{
		ItemIndex = PassiveRelics.IndexOfByPredicate([InstanceId](const UARLoadoutItemInstance* Instance)
		{
			return Instance && Instance->GetInstanceId() == InstanceId;
		});
		if (ItemIndex != INDEX_NONE) SourceArray = &PassiveRelics;
	}
	if (!SourceArray)
	{
		return false;
	}
	UARLoadoutItemInstance* Instance = (*SourceArray)[ItemIndex];
	DropRequest.Definition = Instance->GetItemDefinition();
	DropRequest.SuggestedLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * DropForwardDistance;
	UARWorldItemDropSubsystem* DropSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UARWorldItemDropSubsystem>() : nullptr;
	AARLoadoutItemPickup* SpawnedPickup = nullptr;
	if (!DropSubsystem || !DropSubsystem->TrySpawnLoadoutPickup(
		DropRequest.Definition, DropRequest.SuggestedLocation, DroppedItemPickupClass, GetOwner(), SpawnedPickup))
	{
		Status.Result = EARRequestResult::Rejected;
		return false;
	}
	DropRequest.SpawnedPickup = SpawnedPickup;
	DropRequest.SuggestedLocation = SpawnedPickup->GetActorLocation();
	SourceArray->RemoveAt(ItemIndex);
	UnregisterAndReleaseInstance(Instance, EARItemRemovalReason::Discarded);
	NotifyLoadoutChanged();
	Status.Result = EARRequestResult::Success;
	return true;
}

FARRequestStatus UARLoadoutComponent::HandleSkillInput(FGameplayTag InputTag, FARSkillGroupHandle& GroupHandle)
{
	FARRequestStatus Status;
	GroupHandle = FARSkillGroupHandle();
	if (!InputTag.IsValid() || !PlayerOwner || PlayerOwner->IsGameplayInputBlocked())
	{
		Status.Result = EARRequestResult::Blocked;
		return Status;
	}
	for (const TPair<FGuid, FARActiveSkillGroupRecord>& Pair : ActiveSkillGroups)
	{
		if (Pair.Value.InputTag == InputTag)
		{
			Status.Result = EARRequestResult::Blocked;
			return Status;
		}
	}

	TArray<FARRegisteredSkillRecord*> Candidates;
	for (FARRegisteredSkillRecord& Skill : RegisteredSkills)
	{
		if (Skill.Definition.InputTag == InputTag && Skill.Instance && Skill.Instance->IsRegistered())
		{
			Candidates.Add(&Skill);
		}
	}
	Candidates.Sort([](const FARRegisteredSkillRecord& A, const FARRegisteredSkillRecord& B)
	{
		return A.Definition.InputPriority < B.Definition.InputPriority;
	});
	if (Candidates.IsEmpty())
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return Status;
	}

	float ReservedMana = 0.0f;
	float ReservedStamina = 0.0f;
	TArray<FARRegisteredSkillRecord*> Accepted;
	int32 Start = 0;
	while (Start < Candidates.Num())
	{
		const int32 Priority = Candidates[Start]->Definition.InputPriority;
		int32 End = Start;
		while (End < Candidates.Num() && Candidates[End]->Definition.InputPriority == Priority) ++End;
		bool bGroupValid = true;
		float GroupMana = 0.0f;
		float GroupStamina = 0.0f;
		for (int32 Index = Start; Index < End; ++Index)
		{
			FARRegisteredSkillRecord* Skill = Candidates[Index];
			if (GetNow() < Skill->CooldownEndsAt)
			{
				Status.Result = EARRequestResult::Cooldown;
				bGroupValid = false;
				break;
			}
			FGameplayTag FailureTag;
			if (!Skill->Instance->CanExecuteItemSkill(Skill->Definition.SkillId, FailureTag))
			{
				Status.Result = EARRequestResult::Rejected;
				bGroupValid = false;
				break;
			}
			if (!ActionComponent->CanStartAction(Skill->Definition.ActionRequest).IsSuccess())
			{
				Status.Result = EARRequestResult::Blocked;
				bGroupValid = false;
				break;
			}
			GroupMana += Skill->Definition.ResourceCost.Mana;
			GroupStamina += Skill->Definition.ResourceCost.Stamina;
		}
		if (bGroupValid && (!ManaComponent || ManaComponent->GetCurrent() + KINDA_SMALL_NUMBER < ReservedMana + GroupMana))
		{
			Status.Result = EARRequestResult::NotEnoughResource;
			bGroupValid = false;
		}
		if (bGroupValid && (!StaminaComponent || StaminaComponent->GetCurrent() + KINDA_SMALL_NUMBER < ReservedStamina + GroupStamina))
		{
			Status.Result = EARRequestResult::NotEnoughResource;
			bGroupValid = false;
		}
		if (!bGroupValid)
		{
			break;
		}
		ReservedMana += GroupMana;
		ReservedStamina += GroupStamina;
		for (int32 Index = Start; Index < End; ++Index) Accepted.Add(Candidates[Index]);
		Start = End;
	}
	if (Accepted.IsEmpty())
	{
		if (Status.Result == EARRequestResult::Rejected) Status.Result = EARRequestResult::Blocked;
		return Status;
	}

	FARSourceInfo CostSource;
	CostSource.Category = EARModifierSourceCategory::Other;
	CostSource.SourceId = TEXT("SkillGroup");
	float NewValue = 0.0f;
	if (ReservedMana > 0.0f && !ManaComponent->TryConsume(ReservedMana, CostSource, NewValue))
	{
		Status.Result = EARRequestResult::NotEnoughResource;
		return Status;
	}
	if (ReservedStamina > 0.0f && !StaminaComponent->TryConsume(ReservedStamina, CostSource, NewValue))
	{
		if (ReservedMana > 0.0f) ManaComponent->Restore(ReservedMana, CostSource);
		Status.Result = EARRequestResult::NotEnoughResource;
		return Status;
	}

	GroupHandle.Id = FGuid::NewGuid();
	TArray<FARActionHandle> StartedHandles;
	StartedHandles.Reserve(Accepted.Num());
	for (FARRegisteredSkillRecord* Skill : Accepted)
	{
		FARActionRequest ActionRequest = Skill->Definition.ActionRequest;
		ActionRequest.SkillGroupHandle = GroupHandle;
		ActionRequest.OwningItemInstanceId = Skill->Instance->GetInstanceId();
		FARRequestStatus ActionStatus;
		const FARActionHandle ActionHandle = ActionComponent->TryStartAction(ActionRequest, ActionStatus);
		if (!ActionHandle.IsValid())
		{
			for (const FARActionHandle& StartedHandle : StartedHandles)
			{
				ActionComponent->CancelAction(StartedHandle, EARActionCancelReason::Manual);
			}
			if (ReservedMana > 0.0f) ManaComponent->Restore(ReservedMana, CostSource);
			if (ReservedStamina > 0.0f) StaminaComponent->Restore(ReservedStamina, CostSource);
			UE_LOG(LogARAction, Error, TEXT("Skill transaction rolled back because action commit failed for %s."), *Skill->Definition.SkillId.ToString());
			GroupHandle = FARSkillGroupHandle();
			Status.Result = EARRequestResult::Rejected;
			return Status;
		}
		StartedHandles.Add(ActionHandle);
	}

	FARActiveSkillGroupRecord& Group = ActiveSkillGroups.Add(GroupHandle.Id);
	Group.InputTag = InputTag;
	Group.ActionHandles = StartedHandles;
	const float CooldownReduction = StatsComponent ? StatsComponent->GetFinalStat(EARStatType::CooldownReduction) : 0.0f;
	for (int32 Index = 0; Index < Accepted.Num(); ++Index)
	{
		FARRegisteredSkillRecord* Skill = Accepted[Index];
		Skill->CooldownTotal = FMath::Max(Skill->Definition.MinimumCooldown, Skill->Definition.BaseCooldown * (1.0f - FMath::Clamp(CooldownReduction, 0.0f, 100.0f) / 100.0f));
		Skill->CooldownEndsAt = GetNow() + Skill->CooldownTotal;
		Skill->Instance->ExecuteItemSkill(Skill->Definition.SkillId, StartedHandles[Index]);
	}
	OnRegisteredSkillsChanged.Broadcast();
	Status.Result = EARRequestResult::Success;
	return Status;
}

TArray<FARRegisteredSkillUIData> UARLoadoutComponent::GetRegisteredSkillUIData() const
{
	TArray<FARRegisteredSkillUIData> Result;
	for (const FARRegisteredSkillRecord& Skill : RegisteredSkills)
	{
		if (!Skill.Definition.bShowOnHUD || !Skill.Instance) continue;
		FARRegisteredSkillUIData& UI = Result.AddDefaulted_GetRef();
		UI.RegisteredHandle = Skill.Handle;
		UI.ItemInstanceId = Skill.Instance->GetInstanceId();
		UI.SkillId = Skill.Definition.SkillId;
		UI.InputTag = Skill.Definition.InputTag;
		UI.DisplayName = Skill.Definition.SkillDisplayName;
		UI.Description = Skill.Definition.SkillDescription;
		UI.Icon = Skill.Definition.SkillIcon;
		UI.SourceDefinition = Skill.Instance->GetItemDefinition();
		UI.HUDSortOrder = Skill.Definition.HUDSortOrder;
		UI.ResourceCost = Skill.Definition.ResourceCost;
		UI.CooldownTotal = Skill.CooldownTotal;
		UI.CooldownRemaining = FMath::Max(0.0f, static_cast<float>(Skill.CooldownEndsAt - GetNow()));
		UI.bReady = UI.CooldownRemaining <= 0.0f;
	}
	Result.Sort([](const FARRegisteredSkillUIData& A, const FARRegisteredSkillUIData& B)
	{
		if (A.HUDSortOrder != B.HUDSortOrder)
		{
			return A.HUDSortOrder < B.HUDSortOrder;
		}
		return A.SkillId.LexicalLess(B.SkillId);
	});
	return Result;
}

bool UARLoadoutComponent::GetSkillCooldownState(FARRegisteredSkillHandle Handle, bool& bReady, float& Remaining, float& Total, float& Ratio) const
{
	const FARRegisteredSkillRecord* Skill = FindSkill(Handle);
	if (!Skill)
	{
		bReady = false; Remaining = 0.0f; Total = 0.0f; Ratio = 0.0f;
		return false;
	}
	Total = Skill->CooldownTotal;
	Remaining = FMath::Max(0.0f, static_cast<float>(Skill->CooldownEndsAt - GetNow()));
	bReady = Remaining <= 0.0f;
	Ratio = Total > 0.0f ? Remaining / Total : 0.0f;
	return true;
}

bool UARLoadoutComponent::ResetSkillCooldown(FARRegisteredSkillHandle Handle)
{
	if (FARRegisteredSkillRecord* Skill = FindSkill(Handle))
	{
		Skill->CooldownEndsAt = 0.0;
		Skill->CooldownTotal = 0.0f;
		OnRegisteredSkillsChanged.Broadcast();
		return true;
	}
	return false;
}

bool UARLoadoutComponent::ModifySkillCooldown(FARRegisteredSkillHandle Handle, float DeltaSeconds, float& NewRemaining)
{
	NewRemaining = 0.0f;
	FARRegisteredSkillRecord* Skill = FindSkill(Handle);
	if (!Skill || !FMath::IsFinite(DeltaSeconds)) return false;
	const double Now = GetNow();
	Skill->CooldownEndsAt = FMath::Max(Now, Skill->CooldownEndsAt + DeltaSeconds);
	NewRemaining = static_cast<float>(Skill->CooldownEndsAt - Now);
	OnRegisteredSkillsChanged.Broadcast();
	return true;
}

TArray<FARLoadoutItemSnapshot> UARLoadoutComponent::GetLoadoutInventory() const
{
	TArray<FARLoadoutItemSnapshot> Result;
	if (EquippedWeapon) Result.Add(MakeSnapshot(EquippedWeapon));
	for (const UARLoadoutItemInstance* Item : ActiveRelics) Result.Add(MakeSnapshot(Item));
	for (const UARLoadoutItemInstance* Item : PassiveRelics) Result.Add(MakeSnapshot(Item));
	return Result;
}

bool UARLoadoutComponent::GetLoadoutItemDisplayData(FGuid InstanceId, FARLoadoutItemDisplayData& DisplayData) const
{
	const UARLoadoutItemInstance* Instance = FindItemInstance(InstanceId);
	const UARLoadoutItemDefinition* Definition = Instance ? Instance->GetItemDefinition() : nullptr;
	if (!Instance || !GetLoadoutDefinitionDisplayData(Definition, DisplayData))
	{
		DisplayData = FARLoadoutItemDisplayData();
		return false;
	}

	DisplayData.InstanceId = Instance->GetInstanceId();
	DisplayData.UIStates = Instance->GetItemUIStates();
	return true;
}

bool UARLoadoutComponent::GetLoadoutDefinitionDisplayData(const UARLoadoutItemDefinition* Definition, FARLoadoutItemDisplayData& DisplayData) const
{
	DisplayData = FARLoadoutItemDisplayData();
	if (!Definition)
	{
		return false;
	}
	DisplayData.Definition = Definition;
	DisplayData.DefinitionTag = Definition->DefinitionTag;
	DisplayData.ItemTypeTag = Definition->ItemTypeTag;
	DisplayData.Kind = Definition->GetItemKind();
	DisplayData.DisplayName = Definition->DisplayName;
	DisplayData.ShortDescription = Definition->ShortDescription;
	DisplayData.DetailedDescription = Definition->DetailedDescription;
	DisplayData.Icon = Definition->Icon;
	DisplayData.Skills = Definition->SkillDefinitions;
	DisplayData.ProvidedStats.Reserve(Definition->DefaultStatModifiers.Num());
	for (const FARStatModifierSpec& Spec : Definition->DefaultStatModifiers)
	{
		DisplayData.ProvidedStats.Add(MakeProvidedStatDisplayData(Spec));
	}
	return true;
}

FARWeaponEvolutionResult UARLoadoutComponent::RequestWeaponEvolution()
{
	FARWeaponEvolutionResult Result;
	const UARWeaponDefinition* Weapon = EquippedWeapon ? Cast<UARWeaponDefinition>(EquippedWeapon->GetItemDefinition()) : nullptr;
	if (!Weapon || Weapon->EvolutionGroupId.IsNone() || Weapon->NextEvolutionCandidates.IsEmpty())
	{
		Result.Status.Result = EARRequestResult::InvalidDefinition;
		return Result;
	}

	TSet<FSoftObjectPath> UniquePaths;
	for (const TSoftObjectPtr<UARWeaponDefinition>& CandidateReference : Weapon->NextEvolutionCandidates)
	{
		UARWeaponDefinition* Candidate = CandidateReference.LoadSynchronous();
		FARRequestStatus CandidateStatus;
		if (!ValidateEvolutionCandidate(Weapon, Candidate, CandidateStatus))
		{
			UE_LOG(LogARItems, Warning,
				TEXT("Ignoring invalid evolution candidate '%s' for weapon '%s'. Candidates must use the same group and the next stage."),
				*GetNameSafe(Candidate), *GetNameSafe(Weapon));
			continue;
		}
		const FSoftObjectPath CandidatePath(Candidate);
		if (!UniquePaths.Contains(CandidatePath))
		{
			UniquePaths.Add(CandidatePath);
			Result.Candidates.Add(Candidate);
		}
	}
	if (Result.Candidates.IsEmpty())
	{
		Result.Status.Result = EARRequestResult::InvalidDefinition;
		return Result;
	}

	Result.Token.Id = FGuid::NewGuid();
	FARPendingEvolution Pending;
	Pending.Revision = LoadoutRevision;
	Pending.WeaponInstanceId = EquippedWeapon->GetInstanceId();
	for (const TSoftObjectPtr<UARWeaponDefinition>& Candidate : Result.Candidates)
	{
		Pending.CandidatePaths.Add(Candidate.ToSoftObjectPath());
	}
	PendingEvolutions.Add(Result.Token.Id, MoveTemp(Pending));

	if (Result.Candidates.Num() == 1)
	{
		FARRequestStatus CommitStatus;
		UARWeaponDefinition* Candidate = Result.Candidates[0].LoadSynchronous();
		Result.EvolvedInstance = CommitWeaponEvolution(Result.Token, Candidate, CommitStatus);
		Result.Token = FAREvolutionToken();
		Result.Status = CommitStatus;
		return Result;
	}

	Result.bRequiresSelection = true;
	Result.Status.Result = EARRequestResult::Success;
	return Result;
}

UARLoadoutItemInstance* UARLoadoutComponent::CommitWeaponEvolution(FAREvolutionToken Token, const UARWeaponDefinition* Candidate, FARRequestStatus& Status)
{
	Status.Result = EARRequestResult::InvalidHandle;
	FARPendingEvolution Pending;
	if (!PendingEvolutions.RemoveAndCopyValue(Token.Id, Pending)) return nullptr;
	if (Pending.Revision != LoadoutRevision || !EquippedWeapon || EquippedWeapon->GetInstanceId() != Pending.WeaponInstanceId)
	{
		Status.Result = EARRequestResult::StaleRequest;
		return nullptr;
	}
	if (!Candidate || !Pending.CandidatePaths.Contains(FSoftObjectPath(Candidate)))
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return nullptr;
	}
	const UARWeaponDefinition* Current = Cast<UARWeaponDefinition>(EquippedWeapon->GetItemDefinition());
	if (!ValidateEvolutionCandidate(Current, Candidate, Status))
	{
		return nullptr;
	}
	UARLoadoutItemInstance* NewInstance = CreateAndRegisterInstance(Candidate, Status);
	if (!NewInstance) return nullptr;
	UnregisterAndReleaseInstance(EquippedWeapon, EARItemRemovalReason::Evolved);
	EquippedWeapon = NewInstance;
	NotifyLoadoutChanged();
	Status.Result = EARRequestResult::Success;
	return NewInstance;
}

bool UARLoadoutComponent::ValidateDefinition(const UARLoadoutItemDefinition* Definition, FARRequestStatus& Status) const
{
	if (!PlayerOwner || !Definition || !Definition->DefinitionTag.IsValid() || !Definition->RuntimeBehaviorClass)
	{
		Status.Result = !PlayerOwner ? EARRequestResult::InvalidOwner : EARRequestResult::InvalidDefinition;
		return false;
	}
	if (Definition->RuntimeBehaviorClass->HasAnyClassFlags(CLASS_Abstract))
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return false;
	}
	FGameplayTag ExpectedItemType;
	switch (Definition->GetItemKind())
	{
	case EARLoadoutItemKind::Weapon: ExpectedItemType = ARGameplayTags::Item_Type_Weapon; break;
	case EARLoadoutItemKind::ActiveRelic: ExpectedItemType = ARGameplayTags::Item_Type_ActiveRelic; break;
	case EARLoadoutItemKind::PassiveRelic: ExpectedItemType = ARGameplayTags::Item_Type_PassiveRelic; break;
	default: break;
	}
	if (!ExpectedItemType.IsValid() || Definition->ItemTypeTag != ExpectedItemType)
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return false;
	}
	TSet<FName> SkillIds;
	for (const FARSkillDefinition& Skill : Definition->SkillDefinitions)
	{
		if (Skill.SkillId.IsNone() || !Skill.InputTag.IsValid() || !Skill.ActionRequest.ActionTag.IsValid()
			|| !FMath::IsFinite(Skill.BaseCooldown) || !FMath::IsFinite(Skill.MinimumCooldown)
			|| !FMath::IsFinite(Skill.ResourceCost.Mana) || !FMath::IsFinite(Skill.ResourceCost.Stamina)
			|| Skill.BaseCooldown < 0.0f || Skill.MinimumCooldown < 0.0f || Skill.ResourceCost.Mana < 0.0f || Skill.ResourceCost.Stamina < 0.0f
			|| SkillIds.Contains(Skill.SkillId))
		{
			Status.Result = EARRequestResult::InvalidDefinition;
			return false;
		}
		SkillIds.Add(Skill.SkillId);
	}
	Status.Result = EARRequestResult::Success;
	return true;
}

bool UARLoadoutComponent::ValidateEvolutionCandidate(
	const UARWeaponDefinition* Current,
	const UARWeaponDefinition* Candidate,
	FARRequestStatus& Status) const
{
	if (!Current || !Candidate || Current == Candidate || Current->EvolutionGroupId.IsNone()
		|| Candidate->EvolutionGroupId != Current->EvolutionGroupId
		|| Candidate->EvolutionStage != Current->EvolutionStage + 1)
	{
		Status.Result = EARRequestResult::InvalidDefinition;
		return false;
	}
	return ValidateDefinition(Candidate, Status);
}

UARLoadoutItemInstance* UARLoadoutComponent::CreateAndRegisterInstance(const UARLoadoutItemDefinition* Definition, FARRequestStatus& Status)
{
	UARLoadoutItemInstance* Instance = NewObject<UARLoadoutItemInstance>(this, Definition->RuntimeBehaviorClass);
	if (!Instance)
	{
		Status.Result = EARRequestResult::Rejected;
		return nullptr;
	}
	Instance->InitializeInstance(PlayerOwner, Definition);
	Instance->OnItemUIStateChanged.AddDynamic(this, &UARLoadoutComponent::HandleItemUIStateChanged);
	if (!Instance->RegisterItem())
	{
		Instance->OnItemUIStateChanged.RemoveDynamic(this, &UARLoadoutComponent::HandleItemUIStateChanged);
		Status.Result = EARRequestResult::Rejected;
		return nullptr;
	}
	RegisterSkills(Instance);
	Status.Result = EARRequestResult::Success;
	return Instance;
}

void UARLoadoutComponent::UnregisterAndReleaseInstance(UARLoadoutItemInstance* Instance, EARItemRemovalReason Reason)
{
	if (!Instance) return;
	if (ActionComponent) ActionComponent->CancelActionsByItemInstance(Instance->GetInstanceId(), EARActionCancelReason::ItemRemoved);
	UnregisterSkills(Instance);
	Instance->UnregisterItem(Reason);
	Instance->OnItemUIStateChanged.RemoveDynamic(this, &UARLoadoutComponent::HandleItemUIStateChanged);
}

void UARLoadoutComponent::RegisterSkills(UARLoadoutItemInstance* Instance)
{
	if (!Instance || !Instance->GetItemDefinition()) return;
	for (const FARSkillDefinition& Definition : Instance->GetItemDefinition()->SkillDefinitions)
	{
		FARRegisteredSkillRecord& Skill = RegisteredSkills.AddDefaulted_GetRef();
		Skill.Handle.Id = FGuid::NewGuid();
		Skill.Instance = Instance;
		Skill.Definition = Definition;
	}
	OnRegisteredSkillsChanged.Broadcast();
}

void UARLoadoutComponent::UnregisterSkills(UARLoadoutItemInstance* Instance)
{
	RegisteredSkills.RemoveAll([Instance](const FARRegisteredSkillRecord& Skill) { return Skill.Instance == Instance; });
	OnRegisteredSkillsChanged.Broadcast();
}

FARRegisteredSkillRecord* UARLoadoutComponent::FindSkill(FARRegisteredSkillHandle Handle)
{
	return RegisteredSkills.FindByPredicate([&Handle](const FARRegisteredSkillRecord& Skill) { return Skill.Handle == Handle; });
}

const FARRegisteredSkillRecord* UARLoadoutComponent::FindSkill(FARRegisteredSkillHandle Handle) const
{
	return RegisteredSkills.FindByPredicate([&Handle](const FARRegisteredSkillRecord& Skill) { return Skill.Handle == Handle; });
}

const UARLoadoutItemInstance* UARLoadoutComponent::FindItemInstance(FGuid InstanceId) const
{
	if (EquippedWeapon && EquippedWeapon->GetInstanceId() == InstanceId)
	{
		return EquippedWeapon;
	}
	if (const TObjectPtr<UARLoadoutItemInstance>* Found = ActiveRelics.FindByPredicate([InstanceId](const UARLoadoutItemInstance* Instance)
	{
		return Instance && Instance->GetInstanceId() == InstanceId;
	}))
	{
		return Found->Get();
	}
	if (const TObjectPtr<UARLoadoutItemInstance>* Found = PassiveRelics.FindByPredicate([InstanceId](const UARLoadoutItemInstance* Instance)
	{
		return Instance && Instance->GetInstanceId() == InstanceId;
	}))
	{
		return Found->Get();
	}
	return nullptr;
}

FARLoadoutItemSnapshot UARLoadoutComponent::MakeSnapshot(const UARLoadoutItemInstance* Instance) const
{
	FARLoadoutItemSnapshot Snapshot;
	if (Instance)
	{
		Snapshot.InstanceId = Instance->GetInstanceId();
		Snapshot.Definition = Instance->GetItemDefinition();
		Snapshot.Kind = Snapshot.Definition ? Snapshot.Definition->GetItemKind() : EARLoadoutItemKind::PassiveRelic;
		Snapshot.UIStates = Instance->GetItemUIStates();
	}
	return Snapshot;
}

void UARLoadoutComponent::NotifyLoadoutChanged()
{
	++LoadoutRevision;
	// Keep outstanding tokens until they are committed or explicitly cancelled.
	// Their captured revision then lets the caller distinguish StaleRequest from an invalid handle.
	OnLoadoutChanged.Broadcast(LoadoutRevision);
}

double UARLoadoutComponent::GetNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UARLoadoutComponent::HandleItemUIStateChanged(FGuid ItemInstanceId, const FARItemUIState& State, bool bRemoved)
{
	OnItemUIStateChanged.Broadcast(ItemInstanceId, State, bRemoved);
}

void UARLoadoutComponent::HandleActionEnded(FARActionHandle Handle)
{
	RemoveActionFromSkillGroups(Handle);
}

void UARLoadoutComponent::HandleActionCancelled(FARActionHandle Handle, EARActionCancelReason Reason)
{
	RemoveActionFromSkillGroups(Handle);
}

void UARLoadoutComponent::RemoveActionFromSkillGroups(FARActionHandle Handle)
{
	TArray<FGuid> EmptyGroups;
	for (TPair<FGuid, FARActiveSkillGroupRecord>& Pair : ActiveSkillGroups)
	{
		Pair.Value.ActionHandles.Remove(Handle);
		if (Pair.Value.ActionHandles.IsEmpty()) EmptyGroups.Add(Pair.Key);
	}
	for (const FGuid& GroupId : EmptyGroups) ActiveSkillGroups.Remove(GroupId);
}

#undef LOCTEXT_NAMESPACE
