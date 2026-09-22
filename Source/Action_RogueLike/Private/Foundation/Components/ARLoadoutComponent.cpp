#include "Foundation/Components/ARLoadoutComponent.h"

#include "Foundation/Characters/ARPlayerCharacter.h"
#include "Foundation/Components/ARActionComponent.h"
#include "Foundation/Components/ARManaComponent.h"
#include "Foundation/Components/ARStaminaComponent.h"
#include "Foundation/Components/ARStatsComponent.h"
#include "Foundation/Core/ARLogChannels.h"
#include "Foundation/Items/ARLoadoutItemDefinition.h"
#include "Foundation/Items/ARLoadoutItemInstance.h"

UARLoadoutComponent::UARLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	auto RemoveFromArray = [this, InstanceId, &DropRequest](TArray<TObjectPtr<UARLoadoutItemInstance>>& Items) -> bool
	{
		const int32 Index = Items.IndexOfByPredicate([InstanceId](const UARLoadoutItemInstance* Instance)
		{
			return Instance && Instance->GetInstanceId() == InstanceId;
		});
		if (Index == INDEX_NONE) return false;
		UARLoadoutItemInstance* Instance = Items[Index];
		DropRequest.Definition = Instance->GetItemDefinition();
		DropRequest.SuggestedLocation = GetOwner()->GetActorLocation();
		Items.RemoveAt(Index);
		UnregisterAndReleaseInstance(Instance, EARItemRemovalReason::Discarded);
		return true;
	};
	if (!RemoveFromArray(ActiveRelics) && !RemoveFromArray(PassiveRelics))
	{
		return false;
	}
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
		if (Pair.Value.InputTag != InputTag)
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
	FARActiveSkillGroupRecord& Group = ActiveSkillGroups.Add(GroupHandle.Id);
	Group.InputTag = InputTag;
	const float CooldownReduction = StatsComponent ? StatsComponent->GetFinalStat(EARStatType::CooldownReduction) : 0.0f;
	for (FARRegisteredSkillRecord* Skill : Accepted)
	{
		FARActionRequest ActionRequest = Skill->Definition.ActionRequest;
		ActionRequest.SkillGroupHandle = GroupHandle;
		ActionRequest.OwningItemInstanceId = Skill->Instance->GetInstanceId();
		FARRequestStatus ActionStatus;
		const FARActionHandle ActionHandle = ActionComponent->TryStartAction(ActionRequest, ActionStatus);
		if (!ActionHandle.IsValid())
		{
			UE_LOG(LogARAction, Error, TEXT("Skill transaction precheck passed but action commit failed for %s."), *Skill->Definition.SkillId.ToString());
			continue;
		}
		Group.ActionHandles.Add(ActionHandle);
		Skill->CooldownTotal = FMath::Max(Skill->Definition.MinimumCooldown, Skill->Definition.BaseCooldown * (1.0f - FMath::Clamp(CooldownReduction, 0.0f, 100.0f) / 100.0f));
		Skill->CooldownEndsAt = GetNow() + Skill->CooldownTotal;
		Skill->Instance->ExecuteItemSkill(Skill->Definition.SkillId, ActionHandle);
	}
	if (Group.ActionHandles.IsEmpty())
	{
		ActiveSkillGroups.Remove(GroupHandle.Id);
		Status.Result = EARRequestResult::Rejected;
		return Status;
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
		UI.Icon = Skill.Definition.SkillIcon;
		UI.ResourceCost = Skill.Definition.ResourceCost;
		UI.CooldownTotal = Skill.CooldownTotal;
		UI.CooldownRemaining = FMath::Max(0.0f, static_cast<float>(Skill.CooldownEndsAt - GetNow()));
		UI.bReady = UI.CooldownRemaining <= 0.0f;
	}
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

FARWeaponEvolutionResult UARLoadoutComponent::RequestWeaponEvolution()
{
	FARWeaponEvolutionResult Result;
	const UARWeaponDefinition* Weapon = EquippedWeapon ? Cast<UARWeaponDefinition>(EquippedWeapon->GetItemDefinition()) : nullptr;
	if (!Weapon || Weapon->NextEvolutionCandidates.IsEmpty())
	{
		Result.Status.Result = EARRequestResult::InvalidDefinition;
		return Result;
	}
	Result.Token.Id = FGuid::NewGuid();
	Result.Candidates = Weapon->NextEvolutionCandidates;
	FARPendingEvolution Pending;
	Pending.Revision = LoadoutRevision;
	Pending.WeaponInstanceId = EquippedWeapon->GetInstanceId();
	for (const TSoftObjectPtr<UARWeaponDefinition>& Candidate : Weapon->NextEvolutionCandidates)
	{
		Pending.CandidatePaths.Add(Candidate.ToSoftObjectPath());
	}
	PendingEvolutions.Add(Result.Token.Id, MoveTemp(Pending));
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
	PendingAcquisitions.Reset();
	PendingEvolutions.Reset();
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
