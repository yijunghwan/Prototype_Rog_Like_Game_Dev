#include "Foundation/AI/ARAIController.h"

#include "Foundation/Components/ARMovementControlComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "Foundation/Core/ARLogChannels.h"

void AARAIController::OnPossess(APawn* InPawn)
{
	NextMoveWarningTimes.Reset();
	UnbindMovementControl();
	Super::OnPossess(InPawn);
	MovementControlComponent = InPawn ? InPawn->FindComponentByClass<UARMovementControlComponent>() : nullptr;
	if (MovementControlComponent)
	{
		MovementControlComponent->OnMovementLockChangedNative.AddUObject(this, &AARAIController::HandleMovementLockChanged);
		if (!MovementControlComponent->CanBasicMove())
		{
			StopMovement();
		}
	}
}

void AARAIController::OnUnPossess()
{
	UnbindMovementControl();
	Super::OnUnPossess();
}

void AARAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMovementControl();
	Super::EndPlay(EndPlayReason);
}

void AARAIController::UnbindMovementControl()
{
	if (MovementControlComponent)
	{
		MovementControlComponent->OnMovementLockChangedNative.RemoveAll(this);
		MovementControlComponent = nullptr;
	}
}

bool AARAIController::CanRequestBasicMove() const
{
	return IsValid(GetPawn()) && IsValid(MovementControlComponent) && MovementControlComponent->CanBasicMove();
}

FPathFollowingRequestResult AARAIController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	if (!CanRequestBasicMove())
	{
		WarnBlockedMoveConfiguration();
		return FPathFollowingRequestResult();
	}
	const FPathFollowingRequestResult Result = Super::MoveTo(MoveRequest, OutPath);
	if (Result.Code == EPathFollowingRequestResult::Failed) WarnFailedMove(MoveRequest);
	return Result;
}

EPathFollowingRequestResult::Type AARAIController::ARMoveToActor(AActor* Goal, float AcceptanceRadius)
{
	if (!CanRequestBasicMove())
	{
		WarnBlockedMoveConfiguration();
		return EPathFollowingRequestResult::Failed;
	}
	if (!IsValid(Goal))
	{
		WarnMove(TEXT("InvalidGoal"), TEXT("Goal Actor가 없거나 제거되었습니다. 이동 노드의 Goal 연결을 확인하세요."));
		return EPathFollowingRequestResult::Failed;
	}
	return MoveToActor(Goal, AcceptanceRadius);
}

EPathFollowingRequestResult::Type AARAIController::ARMoveToLocation(FVector Destination, float AcceptanceRadius)
{
	if (!CanRequestBasicMove())
	{
		WarnBlockedMoveConfiguration();
		return EPathFollowingRequestResult::Failed;
	}
	if (Destination.ContainsNaN())
	{
		WarnMove(TEXT("InvalidDestination"), TEXT("Destination에 유효하지 않은 수치가 있습니다. 이동 목표 좌표를 확인하세요."));
		return EPathFollowingRequestResult::Failed;
	}
	return MoveToLocation(Destination, AcceptanceRadius);
}

bool AARAIController::IsWarningDue(FName Reason) const
{
#if !UE_BUILD_SHIPPING
	const double* NextTime = NextMoveWarningTimes.Find(Reason);
	return !NextTime || !GetWorld() || GetWorld()->GetRealTimeSeconds() >= *NextTime;
#else
	return false;
#endif
}

void AARAIController::WarnMove(FName Reason, const FString& Message)
{
#if !UE_BUILD_SHIPPING
	if (!IsWarningDue(Reason)) return;
	NextMoveWarningTimes.Add(Reason, (GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0) + 5.0);
	const FString ActorName = GetPawn() ? GetPawn()->GetActorNameOrLabel() : GetActorNameOrLabel();
	UE_LOG(LogARFoundation, Warning, TEXT("[AR 이동:%s] %s (Controller=%s): %s"),
		*Reason.ToString(), *ActorName, *GetName(), *Message);
#endif
}

void AARAIController::WarnBlockedMoveConfiguration()
{
	if (!IsValid(GetPawn()))
	{
		WarnMove(TEXT("NoPawn"), TEXT("AI Controller가 제어하는 Pawn이 없습니다. 적의 Auto Possess AI와 AI Controller Class, 빙의 상태를 확인하세요."));
	}
	else if (!IsValid(MovementControlComponent))
	{
		WarnMove(TEXT("NoMovementControl"), TEXT("Movement Control Component가 없습니다. ARBaseEnemy 자식 및 컴포넌트 구성을 확인하세요."));
	}
	// A valid pawn blocked by CC, action locks or death is not an authoring error.
}

void AARAIController::WarnFailedMove(const FAIMoveRequest& MoveRequest)
{
#if !UE_BUILD_SHIPPING
	// Do not repeat navigation projections for every failed Tick request.
	const FName DiagnosticKey(TEXT("NavigationDiagnostic"));
	if (!IsWarningDue(DiagnosticKey)) return;
	NextMoveWarningTimes.Add(DiagnosticKey, (GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0) + 5.0);
	if (!MoveRequest.IsValid())
	{
		WarnMove(TEXT("InvalidGoal"), TEXT("이동 요청의 목표가 유효하지 않습니다. Goal Actor·Destination을 확인하세요."));
		return;
	}
	if (!MoveRequest.IsUsingPathfinding())
	{
		WarnMove(TEXT("MoveFailed"), TEXT("이동 요청이 실패했습니다. 이동 컴포넌트와 목표를 확인하세요."));
		return;
	}
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ANavigationData* NavData = Navigation
		? Navigation->GetNavDataForProps(GetNavAgentPropertiesRef(), GetNavAgentLocation()) : nullptr;
	if (!NavData)
	{
		WarnMove(TEXT("NoNavMesh"), TEXT("이 적이 사용할 Navigation Data를 찾지 못했습니다. Nav Mesh Bounds Volume 범위, NavMesh 생성 상태, Agent 설정을 확인하세요."));
		return;
	}
	FNavLocation Projected;
	const FVector Extent = NavData->GetDefaultQueryExtent();
	if (!Navigation->ProjectPointToNavigation(GetNavAgentLocation(), Projected, Extent, NavData))
	{
		WarnMove(TEXT("StartOffNavMesh"), TEXT("적의 현재 위치를 NavMesh 위에서 찾지 못했습니다. 배치 높이, 바닥 충돌, P 키의 경로 표시를 확인하세요."));
		return;
	}
	FVector Goal = MoveRequest.GetGoalLocation();
	if (MoveRequest.IsMoveToActorRequest())
	{
		const AActor* Actor = MoveRequest.GetGoalActor();
		if (const INavAgentInterface* NavGoal = Cast<const INavAgentInterface>(Actor))
		{
			Goal = FQuatRotationTranslationMatrix(Actor->GetActorQuat(), NavGoal->GetNavAgentLocation()).TransformPosition(NavGoal->GetMoveGoalOffset(this));
		}
		else Goal = Actor->GetActorLocation();
	}
	const FString Target = MoveRequest.IsMoveToActorRequest()
		? MoveRequest.GetGoalActor()->GetActorNameOrLabel() : Goal.ToString();
	if (!Navigation->ProjectPointToNavigation(Goal, Projected, Extent, NavData))
	{
		WarnMove(TEXT("GoalOffNavMesh"), FString::Printf(TEXT("목표 %s 위치를 NavMesh 위에서 찾지 못했습니다. 목표/바닥의 높이와 충돌, 메시의 Can Ever Affect Navigation 설정을 확인하세요. 특정 메시가 원인이라고 확정하는 경고는 아닙니다."), *Target));
		return;
	}
	WarnMove(TEXT("MoveFailed"), FString::Printf(TEXT("목표 %s 이동 요청이 실패했습니다. 경로 단절, Navigation Filter, 이동 컴포넌트 상태를 확인하세요. 정확한 원인을 확정하지 못한 일반 경고입니다."), *Target));
#endif
}

void AARAIController::HandleMovementLockChanged(AActor* Target, bool bCanBasicMove, bool bCanMoveAtAll)
{
	if (Target == GetPawn() && !bCanBasicMove)
	{
		StopMovement();
		MovementControlComponent->StopMovementImmediately();
	}
}
