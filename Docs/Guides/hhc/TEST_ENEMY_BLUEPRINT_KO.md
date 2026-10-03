# 테스트 적 Blueprint 사용 안내

이 적의 동작은 기존 공통 시스템 노드를 연결한 Blueprint입니다. 전용 적 게임플레이 C++ 클래스는 추가하지 않습니다.

공통 제작 절차·노드·입력 핀은 [적 제작 HTML 참고서](ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html)를 보세요. 이 문서는 현재 테스트 적의 설정과 사용 안내입니다. [가이드 메인 페이지](<../가이드 메인 페이지.html>)

2026-10-03 이동 예제 갱신: 현재 `Enemy_Move`에는 네 가지 이동 방식이 있습니다. 실제 Tick 실행은 플레이어 Actor까지 NavMesh 경로를 탐색하는 방식 하나만 연결했습니다. 기존 사용자 작성 `Enemy_Attack`과 `Enemy_Event`는 유지합니다. CC는 [간편 노드 참고서](../common/CC_STAGGER_GUIDE_KO.html)에서 확인하세요.

## 생성 에셋과 배치

- 적: `/Game/Game/Objects/Characters/Enemies/Test_1/BP_TestEnemy`
- 자기 기절 상태: `/Game/Game/Objects/Characters/Enemies/Test_1/Data/DA_TestEnemy_GroggyStun`
- 부모: `ARBaseEnemy`. 기존 `ARAIController`와 스탯/체력/이동/액션/상태/경직 컴포넌트를 사용합니다.

콘텐츠 브라우저에서 BP_TestEnemy를 찾아 테스트 레벨 바닥 위에 배치합니다. 별도로 객체나 클래스를 선언하거나 입력 키를 등록할 필요가 없습니다. 플레이 시 로컬 Player Index 0의 Pawn을 찾아 추적합니다. 나중에 `Spawn Actor From Class`로 같은 BP를 스폰할 수도 있습니다. AI Controller Class는 `ARAIController`, Auto Possess AI는 `Placed in World or Spawned`입니다.

맵에는 `Nav Mesh Bounds Volume`을 추가하고 이동할 바닥을 덮도록 크기를 조정해야 합니다. P 키로 녹색 이동 영역을 확인하세요. [NavMesh 준비와 이동 참고서](ENEMY_MOVEMENT_GUIDE_KO.html). 이번 작업은 BP만 수정하며, 현재 저장된 Test_Level에는 NavMesh와 적 인스턴스가 없으므로 배치 작업이 필요합니다. 이 적은 바닥 위에서 일반 Walking을 사용하도록 Z=0 평면 고정·시작 시 평면 스냅을 해제했습니다. 공격 거리 판정은 기존 XY 기준을 유지합니다.

기존 레벨·플레이어·GUN 에셋은 변경하지 않습니다. 레벨에 적을 배치하는 작업은 사용자가 진행합니다.

## 동작

1. 매 프레임 플레이어의 유효성·팀/생존·이동 제한·현재 공격 액션을 검사합니다.
2. XY 거리 300cm 초과이면 `Get Controller` → `Cast To ARAIController` → `AR AI Move To Actor`로 플레이어까지 경로 탐색을 요청합니다. Cast에도 흰 실행선이 연결되어 있습니다. 매 프레임 경로를 새로 요청하지 않도록 기본 0.25초 간격을 둡니다.
3. 300cm 이내(경계 포함)이면 즉시 정지하고 `Try Start Action`으로 공격을 시작합니다.
4. 해당 액션이 일반 이동을 막는 동안 `Action Delay`로 1초를 기다립니다. 경직/기절 취소 규칙을 사용합니다.
5. Completed에서 살아 있는 대상과 현재 XY 반경을 다시 검사합니다. 범위 밖으로 도망갔으면 빗나갑니다.
6. 범위 안이면 물리 직접 피해를 적용하고, 맞음/빗나감 모두 `End Action`으로 종료합니다. 취소된 대기에서는 피해를 적용하지 않습니다.
7. 다음 프레임부터 추적 또는 다음 공격을 다시 판단합니다. 별도 추가 공격 쿨타임은 없으며 각 공격에는 1초 준비가 있습니다.

추적은 NavMesh 기반입니다. NavMesh가 없거나 대상까지 경로가 없으면 요청이 실패하며 직선 이동으로 자동 우회하지 않습니다. 마지막 요청 결과는 `Last Path Request Result`에 저장됩니다. `Request Successful`은 경로 요청을 접수했다는 의미이지 목적지에 도착했다는 뜻이 아닙니다. CC가 기존 이동 요청을 중단한 뒤에는 해제 후 추적 루프에서 다시 요청합니다. 인지·멀티플레이 타깃 선택은 구현하지 않았으며 공격 거리와 직선 이동 예제의 거리는 XY만 봅니다. 같은 층 플레이어를 대상으로 사용하세요.

## 이동 예제 네 가지

`Enemy_Move`의 설명 박스에서 비교할 수 있습니다. 초록색 03만 실제 흐름에 연결되어 있으며, 나머지 박스는 진입 실행 핀이 미연결입니다. 설명용 노드끼리 내부 실행선이 연결되어 있어도 Tick에서 진입하지 않으므로 실행되지 않습니다.

| 예제 | 중심 노드 | 입력·특징 | 현재 실행 |
|---|---|---|---|
| 01 방향 이동 | Request Basic Move | Example Move Direction, 기본 월드 +X. 매 프레임 호출, 목표·도착 판정 없음 | 미연결 |
| 02 위치까지 직선 이동 | Branch → Stop Movement Immediately / Request Basic Move | 목표 - 현재 위치를 Normalize 2D. XY 거리가 허용 거리 이내면 정지. 매 프레임 호출, 장애물 우회 없음 | 미연결 |
| 03 Actor까지 경로 탐색 | Get Controller → Cast To ARAIController → AR AI Move To Actor | Goal=TargetPlayer. Acceptance Radius=0으로 캡슐 도착 판정이 공격 반경 바깥에서 먼저 멈추지 않도록 설정 | 연결 |
| 04 위치까지 경로 탐색 | Get Controller → Cast To ARAIController → AR AI Move To Location | Destination=Example Move Location, 기본 (500,0,0). Acceptance Radius=20cm, 엔진 도착 판정에 캡슐 반경도 영향 | 미연결 |

다른 예제를 시험할 때에는 기존 03 실행 연결을 끊고 선택한 예제의 진입 핀으로 연결하세요. 기본 방향/직선 이동은 매 프레임 입력이 필요합니다. 위치 경로 탐색은 요청 후 AI가 이동하므로 재요청 정책을 별도로 정하세요. 두 이동 예제를 동시에 실행하면 이동 입력이나 AI 목적지가 충돌할 수 있습니다. 직선 도착 예제는 높은 속도에서 허용 거리를 지나치는 경우 감속·이동 스텝 제한을 추가해야 합니다.

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
| 경로 재요청 간격 | 0.25초 | Path Request Interval Seconds (양수 사용) |
| 설명용 방향 | (1,0,0) | Example Move Direction |
| 설명용 위치 | (500,0,0) | Example Move Location (월드 좌표) |
| 직선 도착 허용 거리 | 20cm | Example Arrival Tolerance Cm |
| 최대 그로기 게이지 | 100 | Stats Component → Base Stats → MaxGroggy |
| 그로기 자연 회복 | 0 | Stats Component → Base Stats → GroggyRecoveryPerSecond |

기본 범위 표시는 선딜 동안 표시하는 빨간 개발용 Debug Circle입니다. 정식 게임 이펙트나 범위 HUD가 아니고 패키징의 Shipping에서 표시를 보장하지 않습니다.

## 그로기와 사망

현재 사용자 작성 `Enemy_Event`에는 `Event On Character Death`와 `Event On Groggy Gauge Depleted` 직접 이벤트가 있습니다. 이번에는 이 그래프를 교체하지 않았습니다. 직접 이벤트와 기존 Bind에 같은 처리를 동시에 연결하면 두 번 실행될 수 있으므로 한 방식만 사용하세요.

`On Groggy Gauge Depleted`의 기존 흐름은 살아 있는지 확인한 뒤 `Apply Crowd Control`로 자기 기절을 적용합니다. 기절 상태는 이동과 새 액션을 막고, 진행 중 공격을 Stun 사유로 취소합니다. 현재 설정은 강인함에 의한 지속시간 감소를 끕니다. 기존 DA_TestEnemy_GroggyStun은 남아 있지만 이 직접 CC 흐름의 필수 입력은 아닙니다.

현재 게이지 회복은 사용자 작성 `Delay` 뒤 `Reset Groggy Gauge`로 최대치에 채우는 방식입니다. 상태가 실제 제거되는 순간을 추적하는 방식은 아니므로, 기절 시간·Delay를 다르게 설정하거나 외부에서 기절을 조기 해제하면 서로 어긋날 수 있습니다. 정확한 해제 시점을 따르려면 별도로 자기 기절의 핸들과 `On Status Removed`를 비교하는 구조를 사용하세요. CC 면역 때문에 자기 기절이 거절되는 경우의 회복 정책도 별도로 결정해야 합니다.

일반 HP 피해만 주면 그로기 게이지는 깎이지 않습니다. 플레이어 공격에 `Apply Stagger And Groggy Damage From Result`와 양수 Base Groggy Damage를 연결해야 그로기 소진을 시험할 수 있습니다. 이 작업에서는 기존 GUN 그래프를 변경하지 않습니다.

`On Character Death`에 연결된 이벤트는 `Destroy Actor`를 호출합니다. 공통 사망 처리에서 먼저 액션 취소·이동 정지가 실행됩니다. 사망 연출/드롭은 현재 넣지 않습니다.

## 그래프 구분

- `Enemy_Move`: 플레이어 획득, 이동/거리 검사, 공격 시작 호출, 네 가지 이동 예제.
- `Enemy_Attack`: 액션 시작·핸들 저장, 선딜, 원형 거리 판정, 공격력 기반 피해, 정상 종료.
- `Enemy_Event`: 사용자 작성 사망/그로기 직접 이벤트와 회복 처리.

공격 준비는 일반 타이머가 아니라 `Action Delay`를 사용하므로 액션 취소/종료에 따라 대기가 정리됩니다. Actor Tick을 사용하므로 AI 루프 타이머를 별도로 수거할 필요가 없습니다.

## 검증과 다음 확인

- 수정 직후와 저장 후 재로드에서 각각 12개 그래프 실행 검사 통과: Controller 경로 요청, 재요청 간격, 설명용 이동 미실행, 3m 경계, 중복 공격 방지, 1초 준비, 공격력 반영, 취소 후 지연 타격 없음, 그로기 기절/게이지 복구, CC 후 재요청, 사망 삭제.
- 저장된 BP 재로드에서 컴파일 오류·경고 0. 외부 제작 도구 연결을 제거한 정식 Editor 빌드 성공. 도구 없이 BP 로드·컴파일, AI Controller/배치·스폰 자동 소유권/평면 고정 해제 확인 성공.
- 작업 시작 시점과 전체 Content 해시를 비교해 BP_TestEnemy만 변경된 것을 확인했습니다. 기존 사용자 삭제 에셋·맵 변경은 유지했으며, 플레이어·GUN·맵은 이번 작업에서 저장하지 않았습니다. 수정 전 BP는 Saved/EnemyMovementBackup에 백업했습니다.

이 검사는 화면 없는 임시 테스트 월드 기준입니다. NavMesh 없는 월드에서 요청 실패를 정상 처리하는 것까지 확인했으며, 실제 장애물 우회 주행·PIE 화면은 검증하지 않았습니다. 레벨에 NavMesh와 BP_TestEnemy를 배치해 Play로 확인하세요. 공격 애니메이션은 포함하지 않습니다.
