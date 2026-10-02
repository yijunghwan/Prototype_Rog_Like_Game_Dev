# 테스트 적 Blueprint 사용 안내

이 적의 동작은 기존 공통 시스템 노드를 연결한 Blueprint입니다. 전용 적 게임플레이 C++ 클래스는 추가하지 않습니다.

공통 제작 절차·노드·입력 핀은 [적 제작 HTML 참고서](hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html)를 보세요. 이 문서는 현재 테스트 적의 설정과 사용 안내입니다.

2026-10-03 추가: 새 그래프에서는 `Apply Crowd Control`로 DA 없이 자기 기절을 적용할 수 있습니다(Target=Self / CC Type=Stun / Duration=3 / Affected By Tenacity 해제 시 정확한 3초). Success 확인 후 Return Value 핸들을 저장합니다. 기존 적의 DA/그래프는 자동 교체하지 않았으며 기존 `Apply Status Effect`도 그대로 사용할 수 있습니다. [새 노드의 모든 핀](hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html#apply-crowd-control).

## 생성 에셋과 배치

- 적: `/Game/Game/Objects/Characters/Enemies/Test_1/BP_TestEnemy`
- 자기 기절 상태: `/Game/Game/Objects/Characters/Enemies/Test_1/Data/DA_TestEnemy_GroggyStun`
- 부모: `ARBaseEnemy`. 기존 `ARAIController`와 스탯/체력/이동/액션/상태/경직 컴포넌트를 사용합니다.

콘텐츠 브라우저에서 BP_TestEnemy를 찾아 테스트 레벨 바닥 위에 배치합니다. 별도로 객체나 클래스를 선언하거나 입력 키를 등록할 필요가 없습니다. 플레이 시 로컬 Player Index 0의 Pawn을 찾아 추적합니다. 나중에 `Spawn Actor From Class`로 같은 BP를 스폰할 수도 있습니다.

기존 레벨·플레이어·GUN 에셋은 변경하지 않습니다. 레벨에 적을 배치하는 작업은 사용자가 진행합니다.

## 동작

1. 매 프레임 플레이어의 유효성·팀/생존·이동 제한·현재 공격 액션을 검사합니다.
2. XY 거리 300cm 초과이면 `Request Basic Move`로 플레이어 방향에 이동 입력을 줍니다.
3. 300cm 이내(경계 포함)이면 즉시 정지하고 `Try Start Action`으로 공격을 시작합니다.
4. 해당 액션이 일반 이동을 막는 동안 `Action Delay`로 1초를 기다립니다. 경직/기절 취소 규칙을 사용합니다.
5. Completed에서 살아 있는 대상과 현재 XY 반경을 다시 검사합니다. 범위 밖으로 도망갔으면 빗나갑니다.
6. 범위 안이면 물리 직접 피해를 적용하고, 맞음/빗나감 모두 `End Action`으로 종료합니다. 취소된 대기에서는 피해를 적용하지 않습니다.
7. 다음 프레임부터 추적 또는 다음 공격을 다시 판단합니다. 별도 추가 공격 쿨타임은 없으며 각 공격에는 1초 준비가 있습니다.

추적은 직선 방향 이동입니다. NavMesh 장애물 우회/인지/멀티플레이 타깃 선택은 이 테스트의 범위가 아닙니다. 이동은 현재 평면 게임 기준으로 Z를 제거하고 공격 거리도 XY만 봅니다. 같은 층 플레이어를 대상으로 사용하세요.

## 공격 수치와 변경 위치

피해 요청: Base Damage=0, Attack Power Coefficient=`AttackPowerCoefficient`(기본 1), Spell Power Coefficient=0입니다. 적의 **최종 공격력**을 피해 시스템이 읽습니다. 계수가 1이고 공격력이 10이면 방어·증폭 등 후속 계산 전 기본 피해는 10입니다. 실제 최종 피해가 항상 10인 것은 아닙니다.

| 항목 | 기본값 | 수정 위치 |
|---|---:|---|
| 공격 반경 | 300cm | Class Defaults → Test Enemy → Attack Radius Cm |
| 공격 준비 시간 | 1초 | Attack Windup Seconds |
| 공격력 계수 | 1 | Attack Power Coefficient |
| 자기 기절 시간 | 3초 | Groggy Stun Seconds |
| 개발 중 범위 표시 | 켜짐 | Show Attack Debug Circle |
| 공격력 | 10 | Stats Component → Base Stats → AttackPower |
| 이동 속도 | 200cm/s | Stats Component → Base Stats → MoveSpeed |
| 최대 그로기 게이지 | 100 | Stats Component → Base Stats → MaxGroggy |
| 그로기 자연 회복 | 0 | Stats Component → Base Stats → GroggyRecoveryPerSecond |

기본 범위 표시는 선딜 동안 표시하는 빨간 개발용 Debug Circle입니다. 정식 게임 이펙트나 범위 HUD가 아니고 패키징의 Shipping에서 표시를 보장하지 않습니다.

## 그로기와 사망

새로 제작하는 ARBaseEnemy 자식에서는 `Event On Character Death`, `Event On Groggy Gauge Depleted`, `Event On Shield Broken` 등의 직접 이벤트를 놓을 수 있습니다. 등장 초기화는 기존 `Event BeginPlay`를 사용합니다. 현재 저장된 테스트 적은 기존 Bind 방식을 그대로 유지하며 이번 변경으로 그래프를 교체하지 않았습니다. 직접 이벤트와 기존 Bind에 같은 처리를 동시에 연결하면 두 번 실행될 수 있으므로 한 방식만 사용하세요.

`On Groggy Gauge Depleted`에 연결된 이벤트가 자기 자신에게 기절 상태를 적용합니다. 기절 상태는 모든 이동과 새 액션을 막고, 진행 중 공격을 Stun 사유로 취소합니다. 3초를 고정으로 사용하기 위해 이 자기 기절 Definition에는 강인함에 의한 지속시간 감소를 끕니다.

`On Status Removed`에서 **자기 기절의 정확한 Status Handle**이 제거됐는지 확인하고 그로기 게이지를 최대치로 초기화합니다. 다른 상태가 사라졌다고 게이지를 초기화하지 않습니다. 외부 시스템이 자기 기절을 해제하면 실제 해제 시점에 복귀합니다. 별도 전체 CC 면역을 나중에 추가하면 자기 기절 요청도 거절될 수 있으므로 정책을 다시 결정해야 합니다.

일반 HP 피해만 주면 그로기 게이지는 깎이지 않습니다. 플레이어 공격에 `Apply Stagger And Groggy Damage From Result`와 양수 Base Groggy Damage를 연결해야 그로기 소진을 시험할 수 있습니다. 이 작업에서는 기존 GUN 그래프를 변경하지 않습니다.

`On Character Death`에 연결된 이벤트는 `Destroy Actor`를 호출합니다. 공통 사망 처리에서 먼저 액션 취소·이동 정지가 실행됩니다. 사망 연출/드롭은 현재 넣지 않습니다.

## 그래프 구분

- `Enemy_Tracking`: 플레이어 획득, 일반 이동, 거리 검사, 공격 시작 호출.
- `Enemy_Attack`: 액션 시작·핸들 저장, 선딜, 원형 거리 판정, 공격력 기반 피해, 정상 종료.
- `Enemy_Lifecycle`: 사망/그로기/상태 제거 이벤트 바인딩과 정리.

공격 준비는 일반 타이머가 아니라 `Action Delay`를 사용하므로 액션 취소/종료에 따라 대기가 정리됩니다. Actor Tick을 사용하므로 AI 루프 타이머를 별도로 수거할 필요가 없습니다.

## 검증과 다음 확인

- 생성 직후와 저장 후 재로드에서 각각 21개 그래프 실행 검사 통과: 추적 입력, 3m 경계, 중복 공격 방지, 1초 준비, 공격력 반영, 범위 이탈, 취소 후 지연 타격 없음, 그로기 기절/3초 종료/게이지 복구, 사망 삭제.
- 외부 제작 도구 연결을 제거한 정식 Editor 빌드 성공. 도구 없이 새 적 에셋 로드·컴파일 및 기본 설정 확인 성공. 기존 콘텐츠도 저장 없이 컴파일했으며, GUN의 기존 GetInstanceId 미연결 경고는 수정하지 않았습니다.
- 공통 시스템 자동 검사 `AR.Foundation` 45/45 통과. 기존 GUN 4개 에셋의 작업 전후 해시 동일.

이 검사는 임시 테스트 월드/화면 없는 실행 기준입니다. 실제 레벨의 바닥·충돌·이동 외형은 BP_TestEnemy를 배치해 Play로 확인하세요. 장애물 우회와 공격 애니메이션은 아직 포함하지 않습니다.
