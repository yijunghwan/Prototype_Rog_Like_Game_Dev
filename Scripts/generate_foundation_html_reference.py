"""Build the Korean/English Foundation reference from the source catalogue."""

from __future__ import annotations

import html
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOGUE = ROOT / "Docs/Foundation/FOUNDATION_BLUEPRINT_NODE_CATALOG.md"
SYSTEM = ROOT / "Docs/Foundation/FOUNDATION_SYSTEM_PLAN.md"
STAT_HEADER = ROOT / "Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h"
OUTPUT = ROOT / "Docs/Foundation/FOUNDATION_REFERENCE_KO_EN.html"

STAT_META = [
    ("최대 체력", "생존", "최대 체력. 현재 체력과 구분한다."),
    ("회복력", "생존", "콘텐츠가 회복량을 계산할 때 선택적으로 읽는 수치. 자동 체력 재생이 아니다."),
    ("물리 방어력", "방어", "물리 피해의 방어 계산에 사용한다."),
    ("화염 방어력", "방어", "화염 피해의 방어 계산에 사용한다."),
    ("마법 방어력", "방어", "마법 피해의 방어 계산에 사용한다."),
    ("전체 피해 감소율", "방어", "모든 일반 피해에 적용하는 독립 피해 감소율."),
    ("물리 피해 감소율", "방어", "물리 피해에 적용하는 독립 피해 감소율."),
    ("화염 피해 감소율", "방어", "화염 피해에 적용하는 독립 피해 감소율."),
    ("마법 피해 감소율", "방어", "마법 피해에 적용하는 독립 피해 감소율."),
    ("회피력", "방어", "회피 판정을 허용한 피해의 회피 확률. 캐릭터별 상한을 적용한다."),
    ("공격력", "기본 전투", "공격력 기반 기술의 계수 입력."),
    ("주문력", "기본 전투", "주문력 기반 기술의 계수 입력."),
    ("경직력", "기본 전투", "공격자가 주는 경직 피해를 강화한다."),
    ("이동 속도", "기본 전투", "CharacterMovement의 최대 기본 이동 속도에 반영한다."),
    ("공격 속도", "기본 전투", "100이 1배. 콘텐츠의 공격 시간 계산에 사용한다."),
    ("사거리", "기본 전투", "공격·투사체·스킬의 거리 계산에 사용하는 기준 수치."),
    ("치명타 확률", "기본 전투", "치명타 발생 확률(%). 캐릭터별 상한을 적용한다."),
    ("치명타 피해량", "기본 전투", "치명타 피해 배율(%). 150은 1.5배."),
    ("방어 관통률", "기본 전투", "대상의 양수 속성 방어력을 무시하는 비율(%)."),
    ("흡수", "기본 전투", "흡수 적용 피해 1,000당 수치 1마다 체력 1을 회복한다."),
    ("전체 피해 증폭", "피해 증폭", "모든 일반 피해를 증폭한다."),
    ("직접 피해 증폭", "피해 증폭", "직접 타격 피해만 증폭한다."),
    ("지속 피해 증폭", "피해 증폭", "주기적으로 적용되는 피해만 증폭한다."),
    ("물리 피해 증폭", "피해 증폭", "물리 속성 피해만 증폭한다."),
    ("화염 피해 증폭", "피해 증폭", "화염 속성 피해만 증폭한다."),
    ("마법 피해 증폭", "피해 증폭", "마법 속성 피해만 증폭한다."),
    ("그로기 피해 증폭", "피해 증폭", "공격이 주는 그로기 피해를 증폭한다."),
    ("물리 피격 피해 증가율", "피격 취약", "방어 계산 뒤 받는 물리 피해를 증가시킨다."),
    ("화염 피격 피해 증가율", "피격 취약", "방어 계산 뒤 받는 화염 피해를 증가시킨다."),
    ("마법 피격 피해 증가율", "피격 취약", "방어 계산 뒤 받는 마법 피해를 증가시킨다."),
    ("강인함", "경직·그로기", "기절·속박 등의 상태이상 지속시간을 줄이는 저항 수치."),
    ("경직 저항력", "경직·그로기", "일반 경직이 발생하는지를 판정하는 방어 측 기준."),
    ("최대 그로기 수치", "경직·그로기", "그로기 게이지의 최대량."),
    ("그로기 회복 지연 시간", "경직·그로기", "마지막 그로기 피해 후 회복이 시작되기까지의 초."),
    ("초당 그로기 회복량", "경직·그로기", "그로기 게이지가 초당 회복되는 양."),
    ("행운", "플레이어", "드롭·확률형 효과에서 콘텐츠가 읽는 기준 수치."),
    ("최대 스태미나", "플레이어", "스태미나의 최대량."),
    ("초당 스태미나 회복량", "플레이어", "회복 시작 후 초당 재생되는 스태미나."),
    ("스태미나 회복 대기시간", "플레이어", "마지막 소비 후 회복 시작까지의 초."),
    ("구르기 스태미나 소모량", "플레이어", "구르기/대시 한 번에 소비하는 스태미나."),
    ("구르기 거리", "플레이어", "구르기/대시의 목표 이동 거리(uu)."),
    ("구르기 회피 보장 시간", "플레이어", "구르기 시작 후 회피를 보장하는 시간(초)."),
    ("최대 MP", "플레이어", "MP의 최대량."),
    ("초당 MP 회복량", "플레이어", "초당 재생되는 MP."),
    ("최대 소모품 슬롯 수", "플레이어", "보유할 수 있는 소모품 슬롯 개수. 최종값은 내림."),
    ("쿨다운 감소율", "플레이어", "스킬 재사용 대기시간 감소율(%). 캐릭터별 상한과 스킬 최소 쿨다운을 따른다."),
]

PURPOSE = {
    "ActionDelay": "행동 핸들에 묶인 지연을 시작하고 완료·취소를 통지",
    "CanStartAction": "현재 상태에서 행동 시작 가능 여부를 사전 확인",
    "TryStartAction": "행동을 시작하고 정리용 핸들을 발급",
    "EndAction": "행동을 정상 종료하고 소유 효과를 정리",
    "CancelAction": "지정 행동을 사유와 함께 취소",
    "CancelActionsByReason": "해당 취소 사유를 허용한 행동들을 취소",
    "CancelRollActions": "진행 중인 구르기 행동을 취소",
    "CancelAllActions": "진행 중인 행동을 모두 취소",
    "CancelActionsByItemInstance": "특정 아이템 인스턴스의 행동을 취소",
    "SetActionCancelRules": "행동 핸들의 취소 규칙을 변경",
    "SetActionRollBlocked": "행동이 구르기를 막는지 설정",
    "SetActionBasicMovementBlocked": "행동이 기본 이동을 막는지 설정",
    "RegisterActionHitbox": "히트박스를 행동에 귀속해 종료 시 정리",
    "ApplyActionStatModifier": "행동 수명에 묶이는 스탯 보정을 적용",
    "ApplyActionSuperArmor": "행동 수명에 묶이는 슈퍼아머를 적용",
    "IsActionActive": "행동 핸들이 현재 유효하게 실행 중인지 조회",
    "IsRollBlocked": "현재 구르기가 차단되는지 조회",
    "IsBasicMovementBlocked": "현재 기본 이동이 차단되는지 조회",
    "GetActiveActionCount": "실행 중인 행동 수 조회",
    "SnapToTarget": "카메라를 플레이어 추적 지점으로 즉시 이동",
    "GetAimDirection": "현재 월드 조준 방향 조회",
    "SetAimDirection": "월드 조준 방향 설정",
    "GetCombatTeam": "전투 진영 조회",
    "GetSourceInfo": "피해 출처 정보를 조회",
    "CanDamageTarget": "팀·대상 규칙상 피해를 줄 수 있는지 확인",
    "ApplyCombatDamage": "회피·치명타·방어·보호막·체력 순으로 피해 처리",
    "ApplyDamageOverTime": "지속 피해를 등록·갱신",
    "RemoveDamageOverTime": "지속 피해 핸들 하나를 제거",
    "RemoveDamageOverTimeBySource": "지정 출처의 지속 피해 제거",
    "RemoveAllDamageOverTimeFromTarget": "대상에게 적용된 지속 피해 전부 제거",
    "GetActiveDamageOverTimeCount": "활성 지속 피해의 개수 조회",
    "CanBeCombatTarget": "전투 대상이 될 수 있는지 확인",
    "CanAcceptResolvedDamage": "계산된 피해를 받을 수 있는지 확인",
    "GetCombatSource": "피해 출처 Actor 조회",
    "TryAcquireConsumable": "소모품을 빈 슬롯에 획득",
    "TryUseConsumableSlot": "지정 슬롯 소모품의 사용 가능 여부를 확인하고 사용",
    "DropConsumableSlot": "슬롯의 소모품을 월드에 버림",
    "GetConsumableSlots": "소모품 슬롯 상태 목록 조회",
    "GetMaxConsumableSlots": "최종 소모품 슬롯 최대 개수 조회",
    "SetBaseMaxConsumableSlots": "소모품 슬롯의 기본 최대 개수 설정",
    "RetryPendingOverflowDrops": "슬롯 감소 후 보류된 초과 아이템 드롭 재시도",
    "GetConsumableSlotDisplayData": "보유 소모품 슬롯의 UI 표시 데이터 조회",
    "GetConsumableDefinitionDisplayData": "미보유 소모품 정의의 UI 표시 데이터 조회",
    "GetCurrentHealth": "현재 체력 조회",
    "GetMaxHealth": "최대 체력 조회",
    "GetHealthRatio": "현재 체력 비율 조회",
    "GetCurrentShield": "현재 보호막 합계 조회",
    "IsDead": "대상이 사망 상태인지 조회",
    "ClearShields": "활성 보호막 전체 제거",
    "ApplyShield": "보호막을 추가하고 핸들 발급",
    "RemoveShield": "보호막 핸들 하나를 제거",
    "RemoveShieldsBySource": "출처가 일치하는 보호막 제거",
    "InitializeHitbox": "히트박스의 출처·행동·명중 정책 초기화",
    "TryAcceptTarget": "대상을 중복 명중 규칙에 따라 수락",
    "HasAcceptedTarget": "대상을 이미 명중 처리했는지 조회",
    "ResetAcceptedTargets": "히트박스의 명중 대상 기록 초기화",
    "GetActionHandle": "히트박스 소유 행동 핸들 조회",
    "GetCurrentContext": "히트박스의 현재 타격 문맥 조회",
    "ProjectMouseToGameplayPlane": "마우스 위치를 게임 평면 월드 좌표로 투영",
    "RefreshInteractionCandidate": "주변 상호작용 후보를 다시 탐색",
    "TryInteract": "현재 후보와 상호작용 실행 시도",
    "GetCurrentCandidate": "현재 우선 상호작용 후보 조회",
    "GetCurrentPrompt": "현재 상호작용 안내 문구 조회",
    "CanInteract": "대상과 지금 상호작용할 수 있는지 확인",
    "GetInteractionPrompt": "상호작용 안내 문구 조회",
    "GetInteractionPriority": "후보 선택용 상호작용 우선순위 조회",
    "RequiresInteractionLineOfSight": "상호작용에 시야선 검사가 필요한지 조회",
    "Interact": "상호작용 대상의 동작 실행",
    "ApplyItemStatModifier": "아이템 해제 시 정리되는 스탯 보정을 적용",
    "RemoveOwnItemModifier": "현재 아이템 소유 보정 하나를 제거",
    "RemoveAllOwnItemModifiers": "현재 아이템 소유 보정 전체 제거",
    "SetItemUIState": "아이템 UI의 스택·충전량 상태 기록",
    "RemoveItemUIState": "아이템 UI 상태 항목 제거",
    "GetItemUIStates": "아이템 UI 상태 목록 조회",
    "GetInstanceId": "아이템 런타임 인스턴스 식별자 조회",
    "GetItemOwner": "아이템을 보유한 Actor 조회",
    "GetItemDefinition": "아이템 정의 에셋 조회",
    "IsRegistered": "아이템 스킬이 Loadout에 등록됐는지 조회",
    "CanExecuteItemSkill": "아이템 스킬을 실행할 수 있는지 판정",
    "ExecuteItemSkill": "아이템 BP의 스킬 실제 효과 실행 이벤트",
    "BeginLoadoutAcquisition": "아이템 획득 거래를 시작하고 교체 필요 여부 확인",
    "CommitLoadoutAcquisition": "아이템 획득/교체 거래 확정",
    "CancelLoadoutAcquisition": "대기 중인 획득 거래 취소",
    "CancelAllPendingRequests": "모든 대기 획득·선택 요청 취소",
    "DiscardLoadoutItem": "장착한 아이템을 해제하고 드롭",
    "HandleSkillInput": "입력 태그와 같은 등록 스킬 그룹 실행",
    "GetRegisteredSkillUIData": "등록 스킬의 HUD 표시 데이터 조회",
    "GetSkillCooldownState": "등록 스킬의 쿨다운 상태 조회",
    "ResetSkillCooldown": "등록 스킬 쿨다운 초기화",
    "ModifySkillCooldown": "등록 스킬 쿨다운 시간 변경",
    "RequestWeaponEvolution": "무기 진화 후보 요청",
    "CommitWeaponEvolution": "선택한 후보로 무기 진화 확정",
    "CancelWeaponEvolution": "무기 진화 선택 취소",
    "GetLoadoutInventory": "현재 장비·유물 스냅샷 조회",
    "GetEquippedWeapon": "장착 중인 무기 조회",
    "GetActiveRelics": "장착 중인 액티브 유물 목록 조회",
    "GetPassiveRelics": "장착 중인 패시브 유물 목록 조회",
    "GetLoadoutItemDisplayData": "보유 아이템의 UI 표시 데이터 조회",
    "GetLoadoutDefinitionDisplayData": "미보유 아이템 정의의 UI 표시 데이터 조회",
    "CanAfford": "자원 비용을 지불할 수 있는지 확인",
    "TryConsume": "자원 소비를 시도",
    "Restore": "자원 회복을 적용",
    "GetCurrent": "현재 자원량 조회",
    "GetMax": "최대 자원량 조회",
    "GetRatio": "현재/최대 비율 조회",
    "RequestBasicMove": "기본 조작 이동 요청",
    "RequestActionMove": "행동 중 방향 이동 요청",
    "RequestActionVelocity": "행동 중 속도 요청",
    "AcquireMovementLock": "이동 잠금 핸들 획득",
    "ReleaseMovementLock": "이동 잠금 핸들 반환",
    "ReleaseMovementLocksBySource": "출처가 일치하는 이동 잠금 제거",
    "CanBasicMove": "기본 이동이 허용되는지 조회",
    "CanMoveAtAll": "모든 종류의 이동이 허용되는지 조회",
    "StopMovementImmediately": "현재 이동 속도를 즉시 0으로 변경",
    "GetItemDefinition": "픽업/인스턴스의 아이템 정의 조회",
    "GetConsumableDefinition": "소모품 정의 에셋 조회",
    "GetSlotIndex": "픽업/소모품의 슬롯 번호 조회",
    "SetAimWorldLocation": "월드 조준 위치를 설정",
    "GetAimWorldLocation": "현재 월드 조준 위치 조회",
    "HasAimWorldLocation": "월드 조준 위치가 설정돼 있는지 조회",
    "ClearAimWorldLocation": "월드 조준 위치 기록 삭제",
    "IsGameplayInputBlocked": "게임플레이 입력 차단 여부 조회",
    "SetGameplayInputBlocked": "수동 게임플레이 입력 차단 설정",
    "GetPostHitInvulnerabilityDuration": "피격 후 무적 지속시간 조회",
    "TryStartRoll": "현재 이동 입력 또는 마우스 방향으로 대시 시작 시도",
    "GetRollCooldownRemaining": "대시 쿨타임의 남은 초 조회",
    "RestoreHealth": "체력 회복량 적용",
    "RestoreMana": "MP 회복량 적용",
    "RestoreStamina": "스태미나 회복량 적용",
    "TryConsumeMana": "MP 소비 시도",
    "TryConsumeStamina": "스태미나 소비 시도",
    "CanAffordResources": "MP·스태미나 비용 지불 가능 여부 확인",
    "TryConsumeResources": "MP·스태미나 비용을 함께 지불",
    "GetCurrentResource": "지정 자원의 현재·최대·비율 조회",
    "ApplyStaggerAndGroggyDamage": "경직/그로기 피해를 별도 경로로 적용",
    "AddSuperArmor": "슈퍼아머를 추가하고 핸들 발급",
    "RemoveSuperArmor": "슈퍼아머 핸들 제거",
    "RemoveSuperArmorBySource": "출처가 일치하는 슈퍼아머 제거",
    "ApplySuperArmor": "슈퍼아머 적용",
    "IsSuperArmorActive": "슈퍼아머 활성 여부 조회",
    "IsStaggered": "현재 경직 상태인지 조회",
    "IsStaggerImmune": "경직 면역 상태인지 조회",
    "GetCurrentGroggy": "현재 그로기 게이지 조회",
    "GetMaxGroggy": "최대 그로기 게이지 조회",
    "ResetGroggyGauge": "그로기 게이지 초기화",
    "ApplyStatModifier": "스탯 보정 추가 및 핸들 발급",
    "AddStatModifier": "스탯 보정 추가",
    "RemoveStatModifier": "스탯 보정 핸들 제거",
    "RemoveStatModifiersBySource": "출처가 일치하는 스탯 보정 제거",
    "RemoveOneStatModifierStack": "스탯 보정 중첩 한 겹 제거",
    "ClearStatModifiers": "활성 스탯 보정을 모두 제거",
    "GetFinalStat": "모든 보정을 적용한 최종 스탯 조회",
    "GetBaseStat": "Details에 지정된 기본 스탯 조회",
    "GetStatBreakdown": "최종 스탯의 계산 내역 조회",
    "GetAllFinalStatViews": "모든 최종 스탯의 표시 정보 조회",
    "GetDamageRemainingMultiplier": "피해 감소 후 남는 피해 배율 조회",
    "GetStatModifiersBySource": "출처가 일치하는 스탯 보정 조회",
    "GetModifierRemainingTime": "스탯 보정의 남은 지속시간 조회",
    "HasGuaranteedEvasion": "관리형 회피 보장 효과가 있는지 조회",
    "HasGuaranteedInvulnerability": "관리형 무적 보장 효과가 있는지 조회",
    "ApplyStatusEffect": "상태이상 적용/갱신",
    "RemoveStatusEffect": "상태이상 핸들 제거",
    "RemoveStatusEffectsBySource": "출처가 일치하는 상태이상 제거",
    "ClearAllStatusEffects": "활성 상태이상 전체 제거",
    "HasStatus": "특정 상태 태그 보유 여부 확인",
    "GetActiveStatusEffects": "활성 상태이상 목록 조회",
    "GetStatusRemainingTime": "상태이상 남은 시간 조회",
    "BlocksBasicMovement": "상태이상이 기본 이동을 막는지 조회",
    "BlocksAllMovement": "상태이상이 모든 이동을 막는지 조회",
    "BlocksRoll": "상태이상이 대시를 막는지 조회",
    "BlocksSkillGroups": "상태이상이 스킬 그룹을 막는지 조회",
    "OpenScreen": "UI 화면을 열고 입력 모드 변경 요청",
    "CloseCurrentScreen": "현재 UI 화면 닫기",
    "RequestBack": "뒤로 가기 입력 처리",
    "IsScreenOpen": "특정 화면이 열려 있는지 조회",
    "GetCurrentScreen": "현재 UI 화면 종류 조회",
    "SetGameplayUIInputMode": "게임플레이/UI 입력 모드 전환",
    "GetHUDSnapshot": "HUD에 필요한 현재 데이터 한 번에 조회",
    "GetStatsComponent": "캐릭터의 스탯 컴포넌트 참조 조회",
    "GetHealthComponent": "캐릭터의 체력 컴포넌트 참조 조회",
    "GetStatusEffectComponent": "캐릭터의 상태이상 컴포넌트 참조 조회",
    "GetStaggerComponent": "캐릭터의 경직·그로기 컴포넌트 참조 조회",
    "GetActionComponent": "캐릭터의 행동 컴포넌트 참조 조회",
    "GetMovementControlComponent": "캐릭터의 이동 제어 컴포넌트 참조 조회",
    "GetStaminaComponent": "플레이어의 스태미나 컴포넌트 참조 조회",
    "GetManaComponent": "플레이어의 MP 컴포넌트 참조 조회",
    "GetLoadoutComponent": "플레이어의 무기·유물 컴포넌트 참조 조회",
    "GetConsumableComponent": "플레이어의 소모품 컴포넌트 참조 조회",
    "GetInteractionComponent": "플레이어의 상호작용 컴포넌트 참조 조회",
    "GetUIManagerComponent": "플레이어의 UI 관리자 컴포넌트 참조 조회",
    "GetTopDownCamera": "플레이어 탑다운 카메라 참조 조회",
    "ReceiveConsumableRegistered": "소모품이 등록됐을 때 BP 연출·효과 구현",
    "ReceiveConsumableUnregistered": "소모품이 해제됐을 때 BP 정리 구현",
    "CanUseConsumable": "소모품 사용 가능 조건을 BP에서 판정",
    "ExecuteConsumableUse": "소모품 사용 시 실제 효과를 BP에서 실행",
    "GetConsumableOwner": "소모품을 보유한 플레이어 조회",
    "ReceiveTargetAccepted": "히트박스가 대상을 수락했을 때 BP 반응 구현",
    "ReceiveItemRegistered": "아이템이 장착·등록됐을 때 BP 효과 구현",
    "ReceiveItemUnregistered": "아이템이 해제될 때 BP 효과 정리",
    "ReceiveItemDefinitionAssigned": "픽업에 아이템 정의가 지정됐을 때 외형 갱신",
    "ReceiveConsumableDefinitionAssigned": "픽업에 소모품 정의가 지정됐을 때 외형 갱신",
    "TrySpawnLoadoutPickup": "무기·유물 픽업 Actor 생성 시도",
    "TrySpawnConsumablePickup": "소모품 픽업 Actor 생성 시도",
    "RemoveModifiers": "조건에 맞는 스탯 보정들을 제거",
    "ClearModifiers": "스탯 보정을 모두 제거",
    "RemoveOneModifierStack": "스탯 보정의 중첩 한 겹 제거",
    "GetModifiersBySource": "출처별 스탯 보정 목록 조회",
    "BP_AoEFinished": "기존 템플릿 범위 공격 완료 연출/정리 이벤트",
    "BP_CursorFeedback": "기존 템플릿 커서 피드백 구현 이벤트",
    "BP_Damaged": "기존 템플릿 NPC 피격 반응 구현 이벤트",
    "BP_UnitSelected": "기존 템플릿 유닛 선택 반응 이벤트",
    "BP_UnitDeselected": "기존 템플릿 유닛 선택 해제 반응 이벤트",
    "BP_StopAnimation": "기존 템플릿 애니메이션 중지 이벤트",
    "BP_InteractionBehavior": "기존 템플릿 상호작용 동작 구현 이벤트",
    "BP_SetZoomPercentage": "기존 템플릿 확대 비율 적용 이벤트",
    "BP_UpdateUnitsCount": "기존 템플릿 유닛 수 UI 갱신 이벤트",
    "UpdateItems": "기존 템플릿 아이템 표시 갱신",
    "UpdateScore": "기존 템플릿 점수 표시 갱신",
    "UpdateCombo": "기존 템플릿 콤보 표시 갱신",
    "DoAim": "템플릿 캐릭터의 조준 처리",
    "DoMove": "템플릿 캐릭터의 이동 처리",
    "DoDash": "템플릿 캐릭터의 대시 처리",
    "DoShoot": "템플릿 캐릭터의 발사 처리",
    "DoAoEAttack": "템플릿 캐릭터의 범위 공격 처리",
    "GetSelectedUnitsCount": "템플릿 전략 유닛의 선택 개수 조회",
    "SetZoomPercentage": "템플릿 카메라의 확대 비율 설정",
    "ResetZoom": "템플릿 카메라 확대 비율 초기화",
    "ToggleSelectAllUnits": "템플릿 전략 유닛 전체 선택 전환",
}

FIELD_PURPOSE = {
    "BaseStats": "각 EARStatType의 기본값을 입력하는 맵. 런타임 수정치는 별도 적용한다.",
    "CombatTeam": "캐릭터의 전투 진영. 플레이어/적 팀 판정에 사용한다.",
    "RollDirectionMode": "일반 대시의 이동키 우선/마우스 전용 방향 모드.",
    "RollDuration": "대시가 이동하는 시간(초). RollDistance와 함께 목표 속도가 정해진다.",
    "RollCooldown": "대시 종료·취소 뒤 다시 사용하기까지의 초. 0이면 비활성.",
    "PostHitInvulnerabilityDuration": "피격 후 일반 피해 무적이 지속되는 초.",
    "BaseFollowSpeed": "카메라가 가까울 때 사용하는 보간 반응 계수.",
    "AccelerationStartDistance": "카메라 추적 반응이 거리 때문에 빨라지기 시작하는 간격(uu).",
    "DistanceSpeedMultiplier": "시작 간격 초과 1uu당 추가할 카메라 보간 계수.",
    "MaximumFollowSpeed": "카메라 보간 반응 계수의 상한.",
    "SnapDistance": "카메라가 목표와 이 거리(2D)를 넘으면 즉시 붙는다.",
    "CameraHeight": "카메라 목표가 플레이어보다 높은 Z 간격(uu).",
    "SourceInfo": "환경 공격원의 출처 ID·분류·소유자 정보.",
    "MaxEvasion": "이 캐릭터의 최종 회피율 상한(%).",
    "MaxCriticalChance": "이 캐릭터의 최종 치명타 확률 상한(%).",
    "MaxTenacity": "이 캐릭터의 최종 강인함 상한(%).",
    "MaxCooldownReduction": "이 캐릭터의 최종 스킬 쿨다운 감소율 상한(%).",
    "InteractionRadius": "상호작용 후보를 찾는 반경(uu).",
    "ScanInterval": "상호작용 후보 재탐색 간격(초).",
    "LineOfSightChannel": "상호작용 시 시야선을 검사할 충돌 채널.",
    "MaxActiveRelics": "장착 가능한 액티브 유물 개수.",
    "DroppedItemPickupClass": "버린 장비/유물의 월드 픽업 BP 클래스.",
    "DropForwardDistance": "장비를 버릴 때 앞쪽으로 띄울 거리(uu).",
    "bStartAtFullHealth": "플레이 시작 시 체력을 최대치로 채울지 설정.",
    "bDamageSystemEnabled": "이 Actor가 Foundation 피해 처리를 받을지 설정.",
    "bStartFull": "플레이 시작 시 해당 자원을 최대치로 채울지 설정.",
    "bCanBeStaggered": "일반 경직을 받을 수 있는지 설정.",
    "BaseStaggerDuration": "일반 경직의 기본 지속시간(초).",
    "PostStaggerImmunityDuration": "경직 종료 뒤 재경직 면역 시간(초).",
    "bUseGroggyGauge": "그로기 게이지를 사용할지 설정.",
    "ImmuneStatusTags": "적용을 거부할 상태이상 태그 목록.",
    "InteractionPrompt": "픽업 상호작용 안내 문구.",
    "InteractionPriority": "여러 상호작용 대상 중 선택 우선순위.",
    "bRequiresLineOfSight": "픽업 상호작용에 시야선 검사를 요구할지 설정.",
    "ItemDefinition": "픽업이 지급할 무기/유물 정의 에셋.",
    "ConsumableDefinition": "픽업이 지급할 소모품 정의 에셋.",
    "MoveAction": "플레이어 이동에 연결된 Enhanced Input Action.",
    "RollAction": "기본 대시에 연결된 Enhanced Input Action.",
    "MouseRollAction": "마우스 방향 전용 대시에 연결된 Input Action.",
    "InteractAction": "상호작용에 연결된 Input Action.",
    "SkillInputBindings": "스킬 Input Action과 논리 Input Tag의 연결 목록.",
    "ConsumableSlotActions": "인덱스 순서대로 소모품 슬롯에 연결된 Input Action 목록.",
    "InventoryAction": "인벤토리 열기에 연결된 Input Action.",
    "UIBackAction": "UI 뒤로 가기에 연결된 Input Action.",
    "PlayerMappingContext": "플레이어가 사용할 실제 키 매핑 에셋.",
    "DefaultStatModifiers": "장착 중 아이템이 제공하는 기본 스탯 보정 목록.",
    "SkillDefinitions": "아이템이 등록하는 입력 태그·비용·쿨다운·행동 스킬 목록.",
    "RuntimeBehaviorClass": "아이템 실제 효과를 실행할 런타임 BP 클래스.",
    "UIStateDisplayDefinitions": "아이템 스택·충전량 등의 UI 표시 정의.",
    "DefinitionTag": "콘텐츠 정의를 구별하는 게임플레이 태그.",
    "ItemTypeTag": "무기/액티브/패시브/소모품 종류 태그.",
    "DisplayName": "UI에 보이는 이름.",
    "ShortDescription": "UI에 보이는 짧은 설명.",
    "DetailedDescription": "UI에 보이는 자세한 설명.",
    "Icon": "UI 표시용 아이콘 텍스처.",
    "bShowOnHUD": "스킬을 HUD에 표시할지 설정.",
    "ActionHandle": "이 히트박스가 속한 행동의 수명 핸들. 실행 중 변경하는 설정값은 아니다.",
    "HitPolicy": "한 대상에 대한 명중 허용 방식. 초기화 시 정책을 정한다.",
    "MappingPriority": "입력 매핑 컨텍스트가 다른 컨텍스트보다 우선할 정도.",
    "StatusTag": "상태이상의 종류를 식별하고 면역·차단을 조회하는 태그.",
    "BaseDuration": "상태이상 기본 지속시간(초).",
    "bAffectedByTenacity": "대상의 강인함에 따라 지속시간을 줄일지 설정.",
    "bCanBeImmune": "상태 면역 태그가 이 효과를 차단할 수 있는지 설정.",
    "bBlocksBasicMovement": "효과 중 기본 이동을 막는다.",
    "bBlocksAllMovement": "효과 중 행동 이동까지 포함한 모든 이동을 막는다.",
    "bBlocksRoll": "효과 중 구르기/대시를 막는다.",
    "bBlocksSkillGroups": "효과 중 스킬 그룹 시작을 막는다.",
    "bCancelActionsOnApply": "효과 적용 순간 진행 중인 행동 취소를 요청한다.",
    "ActionCancelReason": "상태이상이 행동을 취소할 때 전달할 사유.",
    "EvolutionGroupId": "서로 진화 관계인 무기 계열의 식별자.",
    "EvolutionStage": "무기 진화 단계.",
    "NextEvolutionCandidates": "다음 단계로 진화할 수 있는 무기 정의 목록.",
    "InteractionRange": "템플릿 유닛의 상호작용 도달 거리.",
    "CameraBoom": "기존 템플릿 카메라의 Spring Arm 참조.",
    "SpringArm": "기존 템플릿 카메라 Spring Arm 참조.",
    "Camera": "기존 템플릿 카메라 컴포넌트 참조.",
    "FloatingPawnMovement": "기존 전략 템플릿의 Pawn 이동 컴포넌트.",
    "StateTreeAI": "기존 트윈스틱 템플릿 AI 상태 트리 참조.",
    "SphereVisual": "기존 범위 공격의 구형 시각 효과 참조.",
    "CollisionSphere": "기존 템플릿의 구형 충돌 컴포넌트 참조.",
    "Mesh": "기존 템플릿의 표시 메시 컴포넌트 참조.",
    "ProjectileMovement": "기존 투사체의 이동 컴포넌트 참조.",
    "bHit": "기존 NPC 템플릿의 피격 상태 표시.",
}

EVENT_PURPOSE = {
    "OnTargetAccepted": "히트박스가 새 대상을 명중 대상으로 수락했을 때",
    "OnCharacterDeath": "캐릭터가 사망했을 때",
    "OnAimDirectionChanged": "캐릭터의 조준 방향이 바뀌었을 때",
    "OnAimWorldLocationChanged": "플레이어의 월드 조준 위치가 바뀌었을 때",
    "OnActionEnded": "행동이 정상 종료됐을 때",
    "OnActionCancelled": "행동이 지정 사유로 취소됐을 때",
    "Completed": "Action Delay의 대기 시간이 정상 완료됐을 때",
    "Cancelled": "Action Delay가 소유 행동 취소로 중단됐을 때",
    "OnDamageReceived": "피해 대상에게 유효한 피해 요청이 도달했을 때",
    "OnDamageHit": "피해가 실제 적중했을 때",
    "OnDamageEvaded": "대상이 공격을 회피했을 때",
    "OnDamageBlocked": "피해가 무효화되거나 차단됐을 때",
    "OnConsumableSlotsChanged": "소모품 슬롯 내용이나 용량이 바뀌었을 때",
    "OnConsumableDropRequested": "초과 소모품을 월드에 드롭해야 할 때",
    "OnHealthChanged": "현재/최대 체력이 변했을 때",
    "OnShieldChanged": "보호막 총량이 변했을 때",
    "OnDamageApplied": "체력·보호막 피해가 적용됐을 때",
    "OnDeath": "HealthComponent가 사망을 확정했을 때",
    "OnInteractionCandidateChanged": "현재 상호작용 후보가 바뀌었을 때",
    "OnInteractionCompleted": "대상과의 상호작용이 완료됐을 때",
    "OnLoadoutChanged": "장비·유물 보유/장착 상태가 바뀌었을 때",
    "OnRegisteredSkillsChanged": "사용 가능한 등록 스킬 목록이 바뀌었을 때",
    "OnItemUIStateChanged": "아이템 충전량·스택 같은 UI 상태가 바뀌었을 때",
    "OnResourceChanged": "MP 또는 스태미나의 현재/최대량이 바뀌었을 때",
    "OnMovementLockChanged": "이동 잠금 상태가 바뀌었을 때",
    "OnStaggered": "경직이 새로 적용됐을 때",
    "OnStaggerStateChanged": "경직 활성 상태가 바뀌었을 때",
    "OnGroggyChanged": "그로기 게이지가 변했을 때",
    "OnGroggyGaugeDepleted": "그로기 게이지가 소진됐을 때",
    "OnSuperArmorChanged": "슈퍼아머 활성 상태가 바뀌었을 때",
    "OnFinalStatChanged": "보정 적용 뒤 최종 스탯 값이 변했을 때",
    "OnStatModifiersChanged": "스탯 보정 목록이 변했을 때",
    "OnStatusAdded": "상태이상이 새로 적용됐을 때",
    "OnStatusUpdated": "상태이상 지속시간·중첩이 갱신됐을 때",
    "OnStatusRemoved": "상태이상이 제거됐을 때",
    "OnScreenOpenRequested": "UI 화면 열기 요청이 생겼을 때",
    "OnScreenCloseRequested": "UI 화면 닫기 요청이 생겼을 때",
    "OnScreenBackRequested": "UI 뒤로 가기 요청이 생겼을 때",
    "OnHUDSnapshotChanged": "HUD 표시 데이터 스냅샷이 변했을 때",
}

def esc(value: object) -> str:
    return html.escape(str(value), quote=True)

def clean(value: str) -> str:
    return value.replace(chr(96), "").replace("\\|", "|").strip()

def split_params(raw: str) -> list[str]:
    if not raw.strip():
        return []
    out, start, angle, paren = [], 0, 0, 0
    for index, char in enumerate(raw):
        if char == "<":
            angle += 1
        elif char == ">":
            angle -= 1
        elif char == "(":
            paren += 1
        elif char == ")":
            paren -= 1
        elif char == "," and not angle and not paren:
            out.append(raw[start:index].strip())
            start = index + 1
    out.append(raw[start:].strip())
    return out

def pins(signature: str) -> tuple[list[str], list[str]]:
    signature = clean(signature)
    match = re.match(r"(?:(?:static|virtual)\s+)?(.+?)\s+(\w+)\((.*)\)(?:\s+const)?(?:\s+override)?(?:\s*=\s*0)?$", signature)
    if not match:
        return ["원본 선언 참조"], ["원본 선언 참조"]
    ret, _, args = match.groups()
    ins, outs = [], []
    for arg in split_params(args):
        arg = re.sub(r"\s*=\s*.*$", "", arg).strip()
        if not arg:
            continue
        direction = outs if "&" in arg and not re.search(r"\bconst\b", arg) else ins
        direction.append(arg)
    if ret.strip() != "void":
        outs.insert(0, "Return Value: " + ret.strip())
    if match.group(2) == "ActionDelay":
        outs.extend(["Completed: 지연 완료 실행 핀", "Cancelled: 행동 취소 실행 핀"])
    return ins, outs

def purpose(name: str, category: str, kind: str) -> str:
    if name in PURPOSE:
        return PURPOSE[name]
    if kind == "구현 이벤트":
        return "콘텐츠 BP가 " + re.sub(r"(?<!^)(?=[A-Z])", " ", name) + " 발생 시 동작을 구현"
    words = re.sub(r"(?<!^)(?=[A-Z])", " ", name)
    if name.startswith(("Get", "Has", "Is", "Can")):
        return words + " 상태를 조회하거나 조건을 판정"
    if name.startswith(("On", "Receive", "BP_")):
        return words + " 발생 시 BP 동작 구현"
    return category + " 기능: " + words + " 수행"

def parse_catalogue(text: str):
    sections = {"7.1": [], "7.2": [], "7.3": [], "7.4": []}
    current, group = "", ""
    node_pat = re.compile(r"^\| \[([^\]]+)\]\(<([^>]+)>\) \| (.*?) \| (.*?) \| (.*?) \|$")
    field_pat = re.compile(r"^\| (.+?) \| (읽기(?:/쓰기)?) \| (.+?) \|$")
    event_pat = re.compile(r"^\| (.+?) \| (.+?) \| (.+?) \|$")
    source = ""
    for line in text.splitlines():
        head = re.match(r"^### (7\.[1-4])\b", line)
        if head:
            current, group, source = head.group(1), "", ""
            continue
        if line.startswith("### ") or line.startswith("## "):
            current = ""
        if line.startswith("#### "):
            group = line[5:].strip()
        link = re.search(r"\[원본 선언\]\(<([^>]+)>\)", line)
        if link:
            source = link.group(1)
        if current in ("7.1", "7.2"):
            match = node_pat.match(line)
            if match:
                owner, link, label, kind, sig = match.groups()
                name_match = re.search(r"[\w_]+", clean(label))
                name = name_match.group(0) if name_match else clean(label)
                sections[current].append((group, owner, link, name, clean(label), kind, clean(sig)))
        elif current == "7.3":
            match = event_pat.match(line)
            if match and clean(match.group(1)) not in ("이벤트", "---"):
                sections[current].append((group, clean(match.group(1)), clean(match.group(2)), clean(match.group(3))))
        elif current == "7.4":
            match = field_pat.match(line)
            if match and not line.startswith("| 필드"):
                sections[current].append((group, source, clean(match.group(1)), match.group(2), clean(match.group(3))))
    return sections

def source_link(raw: str) -> str:
    match = re.search(r"/Source/(.+?)(?::(\d+))?$", raw)
    if not match:
        return ""
    rel = "../../Source/" + match.group(1)
    return '<a href="' + esc(rel) + '" title="원본 C++ 선언">원본</a>'

def field_parts(decl: str) -> tuple[str, str, str]:
    base, _, default = decl.partition(" = ")
    found = re.search(r"(\w+)$", base)
    name = found.group(1) if found else base
    type_name = base[:found.start()].strip() if found else ""
    return name, type_name, default

def editor_access(source: str, name: str) -> str:
    match = re.search(r"/Source/(.+?)(?::\d+)?$", source)
    if not match:
        return "확인 필요"
    path = ROOT / "Source" / match.group(1)
    if not path.exists():
        return "확인 필요"
    data = path.read_text(encoding="utf-8")
    flags = ""
    for found in re.finditer(r"\b" + re.escape(name) + r"\b\s*(?:=|;)", data):
        prefix = data[max(0, found.start() - 500):found.start()]
        prop_start = prefix.rfind("UPROPERTY")
        if prop_start < 0:
            continue
        between = prefix[prop_start:]
        if ";" in between or "UFUNCTION" in between:
            continue
        flags = between
        break
    if not flags:
        return "확인 필요"
    if "EditAnywhere" in flags:
        return "기본값·배치 인스턴스 편집"
    if "EditDefaultsOnly" in flags:
        return "BP 기본값 편집"
    if "EditInstanceOnly" in flags:
        return "배치 인스턴스 편집"
    if "Visible" in flags:
        return "조회 전용"
    return "BP 접근 전용"

def field_purpose(name: str, type_name: str, group: str) -> str:
    if name in FIELD_PURPOSE:
        return FIELD_PURPOSE[name]
    if name.endswith("Component") or name in ("CameraAnchor", "TopDownCamera", "InteractionVolume"):
        return "구성요소 참조. Details 값은 해당 컴포넌트를 선택하여 조절한다."
    if name.endswith("Class"):
        return "생성에 사용할 " + group + " BP 클래스를 지정한다."
    if name.endswith("Definition"):
        return "사용할 " + group + " 정의 에셋을 지정한다."
    if name.startswith("b"):
        return group + " 동작의 활성 여부를 제어한다."
    if type_name.startswith(("float", "int")):
        return group + " 동작에 사용하는 수치다. 구체적 계산은 원본 선언/구현을 참고한다."
    return group + "에 사용하는 데이터/참조다."

def stat_rows() -> list[tuple[str, str, str, str, str]]:
    data = STAT_HEADER.read_text(encoding="utf-8")
    block = re.search(r"enum class EARStatType\s*:\s*uint8\s*\{(.*?)\};", data, re.S)
    assert block
    names = [part.strip() for part in block.group(1).split(",") if part.strip() and not part.strip().startswith("Count")]
    assert len(names) == len(STAT_META), (len(names), len(STAT_META))
    stats_cpp = (ROOT / "Source/Action_RogueLike/Private/Foundation/Components/ARStatsComponent.cpp").read_text(encoding="utf-8")
    defaults = dict(re.findall(r"BaseStats\.Add\(EARStatType::(\w+),\s*([\d.]+)f?\)", stats_cpp))
    return [(name, ko, category, desc, defaults.get(name, "0")) for name, (ko, category, desc) in zip(names, STAT_META)]

def tag(text: str) -> str:
    return '<span class="tag">' + esc(text) + "</span>"

def render() -> tuple[str, dict[str, int]]:
    sections = parse_catalogue(CATALOGUE.read_text(encoding="utf-8"))
    stats = stat_rows()
    detail_groups = [row for row in sections["7.4"] if row[0].startswith(("A", "U"))]
    struct_groups = [row for row in sections["7.4"] if row[0].startswith("F")]
    class_count = len(set(row[0] for row in detail_groups))
    node_count = len(sections["7.1"]) + len(sections["7.2"])
    head = """<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Foundation 스탯 · Details · 노드 한영 참조</title>
<style>
:root{color-scheme:dark;--bg:#0b1020;--panel:#111a2c;--panel2:#18253c;--line:#2b3953;--text:#edf3ff;--muted:#9fb0ca;--accent:#6ee7c7;--blue:#8dbbff}
*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;background:radial-gradient(circle at 0 0,#1b3450,transparent 35%),var(--bg);font:15px/1.55 system-ui,"Malgun Gothic",sans-serif;color:var(--text)}
header{padding:42px clamp(20px,5vw,70px) 30px;border-bottom:1px solid var(--line)}h1{font-size:clamp(29px,4vw,45px);line-height:1.2;margin:0 0 15px}h2{font-size:24px;margin:0 0 14px}h3{font-size:18px;margin:27px 0 12px;color:var(--accent)}
p{margin:8px 0;color:var(--muted)}strong{color:var(--text)}.wrap{max-width:1650px;margin:auto;padding:30px clamp(16px,4vw,48px) 80px}
.summary{display:flex;flex-wrap:wrap;gap:10px;margin-top:20px}.pill{background:var(--panel);border:1px solid var(--line);border-radius:999px;padding:7px 14px;color:var(--text)}
.toolbar{position:sticky;top:0;z-index:5;padding:13px 0;background:var(--bg);border-bottom:1px solid var(--line);display:flex;gap:10px;flex-wrap:wrap;align-items:center}
input,select{background:var(--panel2);color:var(--text);border:1px solid var(--line);border-radius:10px;padding:10px 12px;font:inherit}input{flex:1;min-width:240px}nav a{display:inline-block;padding:8px 11px;color:var(--accent);text-decoration:none}nav a:hover{text-decoration:underline}
section{scroll-margin-top:85px;margin:36px 0}.intro{padding:17px 20px;border-left:3px solid var(--accent);background:var(--panel);border-radius:10px;margin-bottom:25px}
.tablebox{overflow:auto;border:1px solid var(--line);border-radius:12px;background:var(--panel)}table{border-collapse:collapse;width:100%;min-width:880px}th,td{text-align:left;vertical-align:top;border-bottom:1px solid var(--line);padding:11px 12px}th{position:sticky;top:0;background:#21314a;color:#d8e7ff;white-space:nowrap}tr:last-child td{border-bottom:0}tr:hover td{background:#1a2940}
code{font-family:ui-monospace,Consolas,monospace;color:#e8d6a6;font-size:.93em;overflow-wrap:anywhere}a{color:var(--blue)}.muted{color:var(--muted)}.tag{display:inline-block;background:#1d3b44;color:#9cf4d9;border-radius:6px;padding:2px 6px;margin:2px;white-space:nowrap}
.pin{display:block;margin:3px 0}.pin code{color:#cce0ff}.group{color:var(--accent);font-weight:700}.note{font-size:13px;color:var(--muted)}.hidden{display:none!important}footer{border-top:1px solid var(--line);padding:25px;color:var(--muted)}
.jump{display:flex;gap:9px;flex-wrap:wrap;margin:16px 0 26px}.jump a{background:#183a44;border:1px solid #315f65;border-radius:9px;color:#a7f3d9;padding:8px 11px;text-decoration:none}.jump a:hover{background:#24525c}
</style></head><body><header><h1>Foundation 한·영 레퍼런스</h1>
<p>스탯 이름 · 환경/적/플레이어 및 관련 컴포넌트 Details · Blueprint 노드 입력/출력</p>
<div class="summary">"""
    head += "".join('<span class="pill">' + esc(label) + "</span>" for label in (
        str(len(stats)) + "개 스탯", str(class_count) + "개 클래스 Details",
        str(len(detail_groups)) + "개 클래스 필드", str(len(struct_groups)) + "개 구조체 필드",
        str(node_count) + "개 함수/이벤트 노드",
        str(len(sections["7.3"])) + "개 이벤트 디스패처",
    ))
    head += """</div></header><main class="wrap"><div class="intro">
<strong>범위</strong> 프로젝트 C++가 추가한 Foundation 및 기존 템플릿의 Blueprint 노드와 노출 필드를 다룬다.
언리얼 엔진 기본 노드·엔진 상속 Details 전체와 바이너리 BP 그래프 내부 커스텀 변수는 포함하지 않는다.
Details의 “조회 전용” 필드는 값 설정이 아니라 컴포넌트 참조다. 시그니처에서 비-const 참조 인자는 출력 핀으로 분류했다.
World Context/Target 같은 핀은 에디터가 자동 연결하거나 숨길 수 있으므로 실제 에디터 표시가 최종 기준이다.</div>
<div class="toolbar"><input id="q" type="search" placeholder="한국어·영어 이름, 클래스, 입력/출력 검색" aria-label="검색">
<select id="scope" aria-label="범위"><option value="all">전체</option><option value="stats">스탯</option><option value="details">Details</option><option value="nodes">노드</option><option value="events">이벤트</option></select>
<nav><a href="#stats">스탯</a><a href="#details">Details</a><a href="#nodes">노드</a><a href="#events">이벤트</a></nav></div>"""
    body = ['<section id="stats" data-scope="stats"><h2>스탯 / Stats</h2><p>StatsComponent의 Base Stats에서 기본값을 설정한다. 아래 기본값은 C++ 생성자 기준이며 BP가 재정의했을 수 있다. 현재 체력·스태미나·MP 등은 스탯이 아니라 실시간 자원이다.</p><div class="tablebox"><table><thead><tr><th>한국명</th><th>영어명 / EARStatType</th><th>분류</th><th>용도</th><th>C++ 기본값</th></tr></thead><tbody>']
    for name, ko, category, desc, default in stats:
        body.append('<tr class="entry" data-scope="stats"><td><strong>' + esc(ko) + '</strong></td><td><code>' + esc(name) + '</code></td><td>' + esc(category) + '</td><td>' + esc(desc) + '</td><td><code>' + esc(default) + '</code></td></tr>')
    body.append('</tbody></table></div><p class="note">구르기 지속시간 RollDuration과 쿨타임 RollCooldown은 EARStatType이 아닌 플레이어 Details 설정값이다.</p></section>')
    body.append('<section id="details" data-scope="details"><h2>Details / 노출 필드</h2><p>Foundation이 추가한 Actor·Component·Definition 클래스 필드를 모두 포함한다. 적 AARBaseEnemy는 자체 추가 필드가 없고 AARBaseCharacter와 공통 컴포넌트 설정을 상속한다. 환경 함정은 전용 Actor가 아닌 일반 Actor + UARCombatSourceComponent 구성이므로 각 함정의 범위·주기는 제작한 BP에서 별도로 설정한다.</p><div class="jump"><a href="#detail-AARPlayerCharacter">플레이어</a><a href="#detail-AARBaseCharacter">적·공통 캐릭터</a><a href="#detail-UARStatsComponent">기본 스탯·상한</a><a href="#detail-UARCombatSourceComponent">환경 공격원</a><a href="#detail-UARCameraFollowComponent">카메라</a></div>')
    last_group = ""
    for group, source, decl, access, category in detail_groups:
        if group != last_group:
            if last_group:
                body.append("</tbody></table></div>")
            body.append('<h3 id="detail-' + esc(group) + '">' + esc(group) + ' ' + source_link(source) + '</h3><div class="tablebox"><table><thead><tr><th>영어 필드명</th><th>타입 / 기본값</th><th>용도</th><th>Details 편집 범위</th><th>BP 접근·카테고리</th></tr></thead><tbody>')
            last_group = group
        name, type_name, default = field_parts(decl)
        edit = editor_access(source, name)
        body.append('<tr class="entry" data-scope="details"><td><code>' + esc(name) + '</code></td><td><code>' + esc(type_name) + '</code>' + ('<br><span class="muted">기본값 ' + esc(default) + '</span>' if default else '') + '</td><td>' + esc(field_purpose(name, type_name, group)) + '</td><td>' + esc(edit) + '</td><td>' + esc(access) + ' · ' + esc(category) + '</td></tr>')
    if last_group:
        body.append("</tbody></table></div>")
    body.append('<h3>노드·Details 안쪽의 구조체 필드</h3><p>아래 FAR 구조체는 단독 Actor 설정이 아니다. 피해 요청, 스킬 정의, 출처 정보, 보정값 등의 안쪽 필드로 사용한다. 읽기/쓰기 표시는 Blueprint 핀 접근이며 에디터 편집 범위와는 다르다.</p>')
    last_group = ""
    for group, source, decl, access, category in struct_groups:
        if group != last_group:
            if last_group:
                body.append("</tbody></table></div>")
            body.append('<h3>' + esc(group) + ' ' + source_link(source) + '</h3><div class="tablebox"><table><thead><tr><th>영어 필드명</th><th>타입 / 기본값</th><th>용도</th><th>편집 범위</th><th>BP 접근·카테고리</th></tr></thead><tbody>')
            last_group = group
        name, type_name, default = field_parts(decl)
        edit = editor_access(source, name)
        body.append('<tr class="entry" data-scope="details"><td><code>' + esc(name) + '</code></td><td><code>' + esc(type_name) + '</code>' + ('<br><span class="muted">기본값 ' + esc(default) + '</span>' if default else '') + '</td><td>' + esc(field_purpose(name, type_name, group)) + '</td><td>' + esc(edit) + '</td><td>' + esc(access) + ' · ' + esc(category) + '</td></tr>')
    if last_group:
        body.append("</tbody></table></div>")
    body.append('</section>')
    body.append('<section id="nodes" data-scope="nodes"><h2>Blueprint 노드 / 영어명·용도·입출력</h2><p>동일한 이름이라도 대상 클래스가 다르면 별도 노드다. 입력/출력은 C++ 선언 기준으로 정리했으며 실행 흐름 핀과 자동 Target 핀은 생략했다.</p>')
    for key, title in (("7.1", "Foundation"), ("7.2", "기존 템플릿")):
        body.append('<h3>' + title + '</h3><div class="tablebox"><table><thead><tr><th>카테고리 · 대상</th><th>영어 노드명</th><th>용도</th><th>Input</th><th>Output</th><th>선언</th></tr></thead><tbody>')
        for category, owner, link, name, label, kind, sig in sections[key]:
            inputs, outputs = pins(sig)
            inp = "".join('<span class="pin"><code>' + esc(p) + '</code></span>' for p in inputs) or '<span class="muted">없음</span>'
            out = "".join('<span class="pin"><code>' + esc(p) + '</code></span>' for p in outputs) or '<span class="muted">없음</span>'
            body.append('<tr class="entry" data-scope="nodes"><td><span class="group">' + esc(category) + '</span><br><code>' + esc(owner) + '</code></td><td><strong><code>' + esc(label) + '</code></strong><br><span class="note">' + esc(kind) + '</span></td><td>' + esc(purpose(name, category, kind)) + '</td><td>' + inp + '</td><td>' + out + '</td><td>' + source_link(link) + '<br><code class="note">' + esc(sig) + '</code></td></tr>')
        body.append('</tbody></table></div>')
    body.append('</section>')
    body.append('<section id="events" data-scope="events"><h2>이벤트 디스패처 / 전달 출력</h2><p>Bind Event 또는 Assign으로 구독한다. Input은 구독 대상 인스턴스이며, 아래 전달 인자는 이벤트 발생 시 콜백으로 나오는 값이다.</p><div class="tablebox"><table><thead><tr><th>소유 클래스</th><th>영어 이벤트명</th><th>용도</th><th>Input</th><th>Output / 전달 값</th></tr></thead><tbody>')
    for owner, name, args, category in sections["7.3"]:
        body.append('<tr class="entry" data-scope="events"><td><code>' + esc(owner) + '</code><br><span class="note">' + esc(category) + '</span></td><td><code>' + esc(name) + '</code></td><td>' + esc(EVENT_PURPOSE[name]) + '</td><td>대상 인스턴스 및 Bind Event</td><td><code>' + esc(args) + '</code></td></tr>')
    body.append('</tbody></table></div></section>')
    footer = """</main><footer>원본: FOUNDATION_BLUEPRINT_NODE_CATALOG.md, FOUNDATION_SYSTEM_PLAN.md, Source/Action_RogueLike/Public/Foundation. 생성일 2026-09-24.
<span id="shown"></span></footer>
<script>
const q=document.getElementById("q"),scope=document.getElementById("scope"),shown=document.getElementById("shown");
function filter(){const query=q.value.trim().toLocaleLowerCase(),value=scope.value;let count=0;
document.querySelectorAll(".entry").forEach(row=>{const visible=(value==="all"||row.dataset.scope===value)&&row.textContent.toLocaleLowerCase().includes(query);row.classList.toggle("hidden",!visible);if(visible)count++});
document.querySelectorAll("section[data-scope]").forEach(section=>section.classList.toggle("hidden",value!=="all"&&section.dataset.scope!==value));
shown.textContent=" · 현재 표시 "+count+"개 항목";}
q.addEventListener("input",filter);scope.addEventListener("change",filter);filter();
</script></body></html>"""
    counts = {"stats": len(stats), "class_details": len(detail_groups), "struct_fields": len(struct_groups), "nodes": node_count, "events": len(sections["7.3"])}
    return head + "\n".join(body) + footer, counts

if __name__ == "__main__":
    document, counts = render()
    OUTPUT.write_text(document, encoding="utf-8")
    print(OUTPUT)
    print(counts)
