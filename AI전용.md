# AI전용 — Action_RogueLike 개발 안내

이 문서는 이 프로젝트에서 작업하는 다른 개발자의 AI 에이전트를 위한 시작 안내서다. 작업을 맡길 때 이 파일을 먼저 읽도록 전달한다. 콘텐츠 제작법과 공통 시스템의 경계, 참고 문서, 검증 방법을 설명한다.

기준일: 2026-10-04. 엔진 연결 설정은 `Action_RogueLike.uproject`의 UE 5.8이다. 이번 기준 검사는 UE 5.8.2 / Windows에서 수행했다. 이후 코드와 설정이 달라졌다면 실제 파일을 다시 확인한다. 이 문서가 미래의 구현 상태나 배포 안전성을 보증하지 않는다.

## 1. 작업을 시작하기 전에

1. 요청이 설명·진단인지, 실제 수정·제작인지 구분한다. 진단 요청만 받았다면 임의로 수정하지 않는다.
2. 저장소 루트, 현재 브랜치, 작업 중인 변경을 확인한다. 기존 변경과 미추적 에셋을 보존하고 요청 범위만 수정한다.
3. 아래 가이드에서 해당 제작 분야와 주의점을 읽는다. 노드가 필요하면 전체 노드 검색을 사용한다.
4. 대상 Blueprint의 부모 클래스, DA, 사용 컴포넌트, 클래스 디폴트와 맵 인스턴스의 덮어쓰기를 확인한다.
5. C++ 공개 인터페이스와 실제 구현에서 노드 이름·핀·반환값을 확인한다. 가이드나 기억만으로 존재하지 않는 노드를 제안하지 않는다.
6. 아트가 별도 전달되는 환경이라면 필요한 아트 묶음과 버전, 누락된 참조부터 확인한다.

문서와 코드가 다르면 차이를 보고한다. 구현된 동작 확인에는 현재 코드·저장된 에셋·검증 결과를 사용하고, 사용자가 원하는 규칙과 다르다면 구현을 정답으로 간주해 요구사항을 덮어쓰지 않는다. 상위 작업 지침과 인간 사용자의 요청을 우선한다.

## 2. 가장 먼저 볼 가이드

프로젝트 내부 링크는 다른 개발자 PC에서도 사용할 수 있도록 상대 경로다. HTML 파일은 브라우저에서 열어 개발 중 참고한다.

- [가이드 메인 페이지](Docs/Guides/가이드%20메인%20페이지.html)
- [전체 Blueprint 노드 검색](Docs/Guides/common/BLUEPRINT_NODE_SEARCH_KO.html): 노드 용도, Target, 입력·출력 핀, 관련 상세 가이드 검색.

| 작업 | 먼저 읽을 문서 |
| --- | --- |
| 공통 스탯·효과 | [객체별 스탯](Docs/Guides/common/OBJECT_STAT_GUIDE_KO.html), [스탯 조작·생명주기](Docs/Guides/precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html) |
| 피해·DOT·실드·회복 | [피해 노드·공식](Docs/Guides/common/DAMAGE_NODES_FORMULA_GUIDE_KO.html) |
| CC·경직 | [CC·경직](Docs/Guides/common/CC_STAGGER_GUIDE_KO.html); 그로기는 적 가이드에서 확인 |
| 무기·유물·소모품 DA | [아이템 데이터 에셋](Docs/Guides/nsh/ITEM_ASSET_CREATION_GUIDE_KO.html) |
| 아이템 런타임 | [런타임 기본](Docs/Guides/nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html), [아이템 스킬 제작](Docs/Guides/nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html) |
| 액션·비용·시간 | [액션 생명주기](Docs/Guides/precautions/ACTION_LIFECYCLE_GUIDE_KO.html), [스킬 우선순위](Docs/Guides/precautions/SKILL_PRIORITY_GUIDE_KO.html), [시간 그룹](Docs/Guides/precautions/TIME_GROUP_GUIDE_KO.html) |
| 적 클래스 설정 | [적 전체](Docs/Guides/hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html), [상속·디테일 설정](Docs/Guides/hhc/ENEMY_DETAILS_GUIDE_KO.html) |
| 적 이동·공격 | [적 이동](Docs/Guides/hhc/ENEMY_MOVEMENT_GUIDE_KO.html), [적 액션](Docs/Guides/hhc/ENEMY_ACTION_GUIDE_KO.html), [이동 점검](Docs/Guides/precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html) |
| 환경·상점·드롭 | [적 스폰](Docs/Guides/environment/ENEMY_SPAWN_GUIDE_KO.html), [아이템 검색](Docs/Guides/environment/ITEM_SEARCH_GUIDE_KO.html), [픽업 소환](Docs/Guides/environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html) |
| 조작키 | [기본 조작](Docs/Guides/common/BASIC_CONTROLS_GUIDE_KO.html) |

`Docs/Foundation`은 설계·계약과 과거 기록을 포함한다. 일부 문서의 '구현 전', 테스트 수, 완성도 등은 과거 상태다. 최신 구현 여부를 그 문구만으로 판단하지 않는다. `Docs/AGENT_HANDOFF_CURRENT.md`는 이 작업 환경의 인계 기록이므로 최신 부분만 필요한 범위에서 확인하고, 과거 로그를 새 개발자의 요구사항으로 취급하지 않는다.

## 3. 역할과 폴더 구조

**C++는 공통 판정·상태·수명을, Blueprint는 개별 콘텐츠를 담당한다.** 특정 무기·유물·적을 만들 때마다 Foundation에 전용 C++ 분기를 추가하지 않는다. 새 공통 기능이 정말 필요하면 기존 API로 불가능한 이유와 변경 범위를 설명한다.

```text
Action_RogueLike/
├─ AI전용.md                       이 안내서
├─ Action_RogueLike.uproject        엔진·모듈·플러그인 설정
├─ Source/Action_RogueLike/
│  ├─ Public/Foundation/           공개 클래스·구조체·Blueprint API
│  └─ Private/Foundation/          공통 구현과 자동 테스트
├─ Source/Action_RogueLikeEditor/   에디터 전용 코드
├─ Config/                         프로젝트 설정·태그·충돌·Asset Manager
├─ Content/Game/
│  ├─ Foundation/                  공용 기반 에셋
│  ├─ Objects/                     아이템·적·환경 BP와 DA
│  ├─ Tests/                       테스트 맵·플레이어·게임모드·HUD
│  ├─ Maps/                        본편 맵
│  ├─ UI/                          본편 UI
│  └─ Art/                         임포트한 게임 아트
├─ ArtSource/                      편집 원본
├─ Docs/Guides/                    분야별 HTML 참고서
└─ Scripts/                        빌드·문서 생성·검증 도구
```

- 현재 기반은 싱글플레이 중심이다. 네트워크 복제·예측이 지원된다고 가정하지 않는다.
- 플레이어는 `AARPlayerCharacter`, 적은 `AARBaseEnemy`, 공통 캐릭터는 `AARBaseCharacter` 계열을 사용한다.
- 공통 스탯·HP·실드·자원·상태이상·액션의 내부 값을 직접 대입하거나 별도 변수로 중복 소유하지 않는다. 공개 AR 노드를 통해 읽고 변경한다.
- 게임플레이 평면은 XY, 높이는 Z다. 거리 단위는 기본 Unreal 단위이며 300uu는 3m다. 거리 판정에서 Z를 포함할지는 사용하는 노드·콘텐츠 규칙을 확인한다.

## 4. 아이템을 만드는 기본 순서

1. `UARItemDefinition` DA를 만들고 타입·Item Id·설명·런타임 클래스·스킬 정의를 설정한다. 조회 키는 **Item Type Tag + Item Id**이며 같은 타입에서 ID가 중복되면 안 된다. 자유 분류인 Additional Tags는 여러 아이템이 공유할 수 있다.
2. 무기·유물은 `UARLoadoutItemInstance`, 소모품은 `UARConsumableInstance` 계열로 제작한다. 타입과 런타임 계열을 맞춘다.
3. DA는 설정 데이터다. 실행 중 스택·남은 시간·소유자·쿨타임을 DA/CDO에 저장하지 않는다. 그런 상태는 런타임 인스턴스에 둔다.
4. `On Item Registered`에서 필요한 보유 효과와 구독을 시작한다. `On Item Unregistered`에서 자체 변수·연출·구독 등 콘텐츠가 소유한 항목을 정리한다.
5. 공용 `ARItemPickup` 계열 BP의 Item Definition으로 DA를 지정한다. 단순히 아이템마다 별도 픽업 C++ 클래스를 만들 필요는 없다.
6. `Try Acquire Item` 같은 공용 획득 경로의 결과를 확인한다. 획득 실패인데 픽업을 먼저 삭제하지 않는다.

### 아이템 스킬의 시작·사전 조건·끝

```text
입력 요청 → 공통 후보/조건/비용 검사 → 액션 생성·쿨타임 기록
          → Execute Item Skill(Skill Id, Action Handle)
          → 콘텐츠 실행 → End Action(받은 Handle)
```

- `Can Execute Item Skill`은 BP에서 오버라이드하는 **사전 조건 조회 함수**다. 탄약·대상·자체 조건을 검사만 한다. 비용 차감, 효과 적용, 타이머 시작을 넣지 않는다.
- `Execute Item Skill`에 도달한 스킬은 공통 거래를 통과한 상태다. 같은 스킬의 액션을 다시 `Try Start Action`으로 시작하거나 DA의 마나·스태미나 비용을 다시 차감하지 않는다.
- 탄약 등 DA 공통 자원 비용이 아닌 콘텐츠 자원은 실행 시 별도로 처리하고 성공 여부를 확인한다.
- 전달된 Action Handle을 해당 실행의 종료·타이머·효과에 사용한다. 동시에 가능한 여러 실행의 핸들을 전역 변수 하나로 덮어쓰지 않는다.
- 정상 완료와 콘텐츠 내부 실패 출구 모두 액션이 남지 않게 처리한다. 즉시 실행 스킬도 필요한 작업 후 끝낸다. 실행 이후 콘텐츠 실패가 자동 비용·쿨타임 환불을 의미하지 않는다.
- `On Item Skill Cancelled`는 이 인스턴스가 실행한 스킬의 **취소** 알림이다. 일반 정상 완료 알림이 아니다. Skill Id와 Action Handle을 구분하여 자체 연출·구독·일반 타이머 등을 정리한다.
- 액티브 유물의 장착 슬롯 키는 플레이어의 슬롯 바인딩을 따른다. 유물 콘텐츠에서 특정 물리 키나 1번 장착 슬롯을 고정하지 않는다. DA의 입력 모드·스킬 인덱스는 실제 정의를 확인한다.

### 스킬 우선순위의 정확한 규칙

- 한 요청에서 모인 후보를 Input Priority의 **작은 숫자부터** 처리한다. 반드시 0이 있어야 하는 것은 아니다.
- 같은 숫자는 한 묶음이다. 묶음 전체 조건과 마나·스태미나 합계를 검사한다. 가능한 일부만 선택하지 않는다.
- 앞서 통과한 묶음의 예약 비용을 포함해 현재 묶음을 감당해야 한다.
- 하나라도 쿨타임·사전 조건·액션 시작 조건에 실패하거나 비용이 부족하면 **그 묶음과 이후 묶음은 중단**하고, 앞서 통과한 묶음만 최종 거래 대상으로 남는다.
- 최종 액션 시작 거래 자체가 실패하면 롤백 경로가 있다. '앞쪽 묶음은 어떤 상황에서도 무조건 실행'이라고 설명하지 않는다.
- 같은 물리 키의 별도 상호작용·슬롯 요청까지 자동으로 한 우선순위 거래에 합치는 구조는 아니다.
- 활성 스킬 그룹이 남아 있으면 새 스킬 요청을 막는다. 입력이 먹지 않을 때 End Action 누락도 확인한다.

## 5. 스탯·효과의 수명 선택

| 의도 | 기본 선택 | 정리 책임 |
| --- | --- | --- |
| 아이템을 보유하는 동안 효과 | Apply Item Stat Modifier | 아이템 해제 시 소유 수정치 자동 회수 |
| 해당 행동 중에만 효과 | Apply Action Stat Modifier | 해당 액션 종료·취소 시 자동 회수 |
| 아이템과 무관하게 대상에 남는 버프·스택 | Apply Stat Modifier | Duration 만료 또는 콘텐츠의 핸들/출처 제거 |
| 기본 스탯 자체를 영구 증감 | Apply Stat Modifier의 Permanent Flat | 버프 기록·핸들·만료 없음; 자동 되돌리기·저장 기능 없음 |

- 일반 Flat 버프에 Duration=-1을 주는 것과 Permanent Flat은 다르다. 전자는 영구 유지되는 수정치 기록이고, 후자는 기본값을 직접 변경한다.
- Permanent Flat은 아이템·액션 귀속 수정치 경로에 넣지 않는다. 버려도 성장·돈을 유지하려는 의도를 확인한다. 현재 Money는 플레이어 계열 전용이다.
- 반환 핸들과 Success를 확인한다. 출처 제거에는 **Category와 Source Id 모두 일치**해야 한다. 광범위한 Clear/출처 제거로 다른 콘텐츠 효과를 지우지 않는다.
- 스택 소비는 `Remove Stat Modifier Stacks`의 Count·Removed Count·Success와 전체 개수 요구 옵션을 확인한다. 스택 확인 후 실제 소비 실패 가능성도 무시하지 않는다.
- Percentage 단위, Flat/Additive/Multiplicative 계산과 스탯별 제한은 스탯 가이드를 확인한다. 같은 수치 0.5/50을 임의로 혼용하지 않는다.
- 스탯 변경 이벤트에서 같은 값을 끝없이 재적용하지 않는다. 반복 적용이 필요한 영구 성장도 버프 기록을 무한히 쌓는 방식이 맞는지 검토한다.

## 6. 전용 타이머와 시간 그룹

### 예약의 수명

- 아이템 보유 효과: **Set Item Timer by Event**. 아이템 해제·소유자 종료에서 회수된다. 스킬 취소만으로는 끝나지 않는다.
- 행동 실행: **Set Action Timer by Event**. 정상 종료·취소·소유자 종료에서 회수된다. 타이머 완료가 액션을 자동 종료하지는 않는다.
- 순차 대기: **Action Delay**. 활성 액션의 시간 그룹을 따르며 Completed/Cancelled를 구분한다.
- 일반 Actor의 스포너·상시 AI 판단 루프에는 일반 엔진 타이머를 사용할 수 있지만 자체 EndPlay 정리가 필요하다. 상시 판단 루프를 잠깐의 공격 액션 수명에 묶지 않는다.

전용 타이머를 쓰더라도 독립 Actor·이미 적용한 일반 버프·DOT·외부 구독·전역 배율 등 모든 부작용이 자동 회수되는 것은 아니다. 취소 이벤트 설명과 콘텐츠 코드에서 자동 정리/수동 정리 경계를 명시한다.

현재 전용 타이머의 C++ 이름은 `SetGroupedItemTimerByEvent` / `SetGroupedActionTimerByEvent`, BP 표시 이름은 위 이름이다. 제거된 레거시 함수를 새 콘텐츠에 사용하거나 복원하지 않는다.

- 기본 Initial Start Delay=0, Variance=0이면 첫 실행도 Time초 후다. 첫 지연은 `Time + Initial Start Delay ± Variance`다.
- 같은 아이템/액션에서 동일 Event를 다시 예약하면 기존 예약을 교체한다. 다른 수명 소유자와는 독립이다.
- 예약 직후 흰 실행선과 지연 후 호출되는 Event는 다르다. 지연시킬 작업은 Event 콜백에 연결한다.

### World / Player

- 선택형 기본값은 **World**. 적의 이동·공격·쿨타임·타이머는 World를 권장한다. 플레이어 측 아이템은 콘텐츠 의도에 따라 선택한다.
- DA의 Cooldown Time Group과 Action Request의 Time Group은 별도 설정이다.
- Action Delay는 액션 그룹을 상속하지만 **Action Timer의 Time Group은 별도 선택**이다. 같은 기준이 필요하면 `Get Action Time Group`을 연결한다.
- 플레이어 캐릭터의 기본 시간 로직과 플레이어 대상 효과 기간은 Player, 적·환경과 DOT·동일 피해 이름 제한은 World를 따른다. 플레이어가 쏜 투사체가 자동으로 Player 시간이 되는 것은 아니다.
- 게임 Pause에서는 두 그룹 모두 멈춘다. Player는 운영체제 실시간이 아니다.
- Player 예약 제어에는 `Pause/Unpause/Clear Time Group Timer`와 그룹용 조회 노드를 사용한다. 엔진 기본 핸들 노드는 Player Timer Manager를 찾지 못한다.
- 전역 배율 변경은 자동 액션 귀속이 아니다. 종료·취소·해제 때 복구하고 중첩 효과 정책을 확인한다. 플레이어 Custom Time Dilation을 별도 BP에서 덮어쓰지 않는다.

## 7. 적 제작과 이동

1. `AARBaseEnemy` 자식 BP를 만들고 Combat Team=Enemy, 스탯·체력·캡슐·외형 등 디테일을 확인한다. 팀은 피해 관계이며 AI 소유나 충돌 채널과 같은 개념이 아니다.
2. 경로 이동에는 AI Controller Class=`ARAIController`, 필요 시 Auto Possess AI=`Placed in World or Spawned`를 설정한다. 플레이어 자동 소유 설정과 혼동하지 않는다.
3. 적 BP의 `Get Controller` → `Cast To ARAIController` → **AR AI Move To Actor / AR AI Move To Location**으로 연결한다. Target은 적 액터가 아니라 그 적의 AR AI Controller다. 목표 액터 또는 좌표를 별도 Goal/Destination 핀에 넣는다.
4. 반환값은 이동 요청 수락/실패 여부이며 도착 완료 알림이 아니다. 기절·이동 잠금 해제 뒤 필요한 경로 요청을 다시 낼 수 있도록 구성한다.
5. **NavMesh 생성·관리는 맵 제작자 책임**이다. 경로가 없거나 연결되지 않아 움직이지 않으면 맵 제작자에게 문의한다. 적 제작 작업에서 임의로 맵의 NavMesh를 변경하지 않는다.

`Request Basic Move`는 방향 기반 이동으로 경로 탐색을 하지 않는다. 목적지까지 직선 이동도 방향·거리 판단으로 구성할 수 있으나 장애물 우회는 보장하지 않는다. 직접 위치 순간 변경으로 일반 이동을 대체해 CC·충돌 규칙을 우회하지 않는다.

### 적 공격·사망·그로기

- 적 공격은 `Get Action Component` → `Try Start Action` 성공 확인 → 받은 Handle로 선딜/타이머/효과 → `End Action` 순서다. 아이템의 Execute 이벤트와 달리 적은 직접 액션을 시작한다.
- `On Action Cancelled`에서 자체 연출·독립 자원·일반 타이머 등을 정리한다. 액션 귀속 예약은 자동 정리된다.
- `On Character Death`는 죽음 알림이지 자동 Destroy Actor 명령이 아니다. 즉시 삭제·사망 애니메이션 후 삭제 중 콘텐츠 의도대로 처리한다.
- 그로기는 Use Groggy Gauge와 양수 MaxGroggy 등 디테일 설정이 필요하다. `On Groggy Gauge Depleted`는 소진 알림이며 자동 기절이 아니다. 자체 CC와 게이지 복구 정책을 연결한다.
- 표준 기절/속박은 **Apply Crowd Control**에서 Target·CC Type·Duration을 지정한다. 강인함 적용은 기본 켜짐이다. 단순 Stun/Root 때문에 별도 DA를 반드시 만들 필요는 없다.
- 스탯 조작으로 구현하는 효과에도 Affected By Tenacity를 설정할 수 있다. 해당 핀과 수명은 스탯 조작 가이드에서 확인한다. 강인함 수치, 슈퍼아머, CC 면역을 같은 기능으로 취급하지 않는다.
- `On Shield Broken`은 실드 총합이 양수→0으로 피해 소진 또는 명시적 제거될 때다. 시간 만료와 실드를 무시한 HP 피해는 그 파괴 이벤트 조건이 아니다.
- 캡슐 No Collision으로 적끼리/플레이어와의 충돌을 해결하지 않는다. 바닥·벽 충돌은 유지하고 필요한 Pawn/AR 채널 응답만 조정한다. 외형 컴포넌트의 별도 충돌도 확인한다.

## 8. 공격 판정·피해·회복

- 공격 범위·Overlap·Trace·투사체 이동과 외형은 Blueprint 콘텐츠가 만든다. 피해 해결은 **Apply Combat Damage / Apply Damage Over Time** 공통 경로를 사용한다.
- Attacker·Target·피해 이름·속성·단타/DOT·계수 등 핀의 의미와 계산 순서는 피해 가이드를 따른다. 일반 엔진 Apply Damage로 프로젝트 공식을 우회하지 않는다.
- 경직·그로기는 HP 피해와 별개다. 직접 피해 반환값을 **Apply Stagger And Groggy Damage From Result**의 Damage Result에 연결할 수 있다. 이 노드는 성공 적용 결과만 처리하며, 같은 결과를 여러 번 전달하는 중복 호출까지 막아주지는 않는다.
- DOT의 경직·그로기 설정은 DOT Spec을 확인한다. 이미 자동 처리하는 경로와 수동 적용을 동시에 연결해 중복시키지 않는다.
- 같은 Damage Name 제한은 기본 0.2초이며 요청에서 기간·무시 옵션을 정할 수 있다. 실제 제한 키와 예외는 현재 피해 가이드·구현을 확인한다. 이 기능이 공격 범위 내 모든 개별 효과를 자동으로 중복 방지한다고 확대 해석하지 않는다.
- Restore Health/Mana/Stamina는 **현재 자원**을 최대치 안에서 회복하는 노드다. 최대 스탯 증가·부활·RecoveryPower 자동 배율로 해석하지 않는다.
- 아이템 검색은 DA 참조/배열을 다루며 DA 이름 문자열이 곧 스폰 클래스인 것은 아니다. 검색 결과→소유 제외→중복 없는 추첨→픽업 클래스 선택→Item Definition 할당을 구분한다. 동일 DA의 픽업 외형 선택 정책도 명시한다.

## 9. 입력과 아트 공유

- 기본 조작 정책은 조작 가이드를 따른다. WASD·Space·I·F, 유물 E/R, 소모품 1/2/3, 무기 스킬 Shift/Q/X와 중복 입력 주의가 문서에 있다. 무기 첫 키의 원래 표현은 인계 기록에 미확정 해석이 있으므로 변경 전 담당자 확인이 필요하다.
- Input Action, Input Tag, 물리 키, 슬롯 번호를 서로 같은 값이라고 가정하지 않는다. 실제 PlayerController의 IMC 참조와 플레이어 바인딩을 확인한다.
- 키 추가·변경·건의는 **이정환**에게 문의한다. 문서에 적힌 계획을 이유로 IA·IMC를 무단 변경하지 않는다.
- `ArtSource`와 `Content/Game/Art`는 별도 공유를 사용할 수 있다. 현재 저장소에서 실제 제외되었는지는 `.gitignore`와 Git 추적 상태를 확인한다. 이 안내서를 썼다고 제외 설정까지 적용된 것은 아니다.
- 별도 아트는 원래 경로에 설치하고 코드 버전에 필요한 아트 묶음 버전을 기록한다. 누락 참조를 발견하면 아트 부재부터 보고하고 임의 경로 변경·참조 삭제로 숨기지 않는다.
- BP·DA·IMC·맵도 `.uasset/.umap` 에셋이다. '아트 제외'를 '모든 에셋 제외'로 해석하지 않는다. Git LFS 설정은 `.gitattributes`에서 확인하고, 파일 공유 방식이나 LFS 이력을 임의로 바꾸지 않는다.

## 10. 변경과 검증

### 변경 원칙

- 콘텐츠 제작은 필요한 BP·DA·픽업을 수정한다. 기반 계약 변경이나 새 공통 노드가 필요하면 이유와 영향 범위를 먼저 설명한다.
- UObject 수명을 C++에서 보관할 때 GC가 추적하는 참조와 약한 참조를 의도대로 사용한다. DA/CDO를 실행 상태 저장소로 쓰지 않는다. Delegate 콜백 중 컬렉션 변경·재진입과 EndPlay/GC 중 이벤트 호출을 검토한다.
- 기존 enum 값은 저장된 에셋에 영향을 줄 수 있다. 중간 삽입·번호 재배치·구조체/함수 삭제는 참조 영향과 이행 경로 없이 진행하지 않는다.
- 기존 GUN과 테스트 예제는 참고 자료다. 해당 예제 수정이 요청된 경우를 제외하고 자동 교체·재생성하지 않는다.
- 외부 에셋 생성 도구를 게임 런타임 의존성으로 넣지 않는다. Scripts의 에셋 생성·이름 변경·재배치 도구는 읽기 전용이 아니므로 목적과 쓰기 대상을 먼저 확인한다.
- UCLASS/USTRUCT/UENUM/UFUNCTION 등 반영을 바꾸는 작업은 저장·에디터 종료·정식 빌드·재시작으로 검증한다. Live Coding 또는 BP Compile만으로 반영되었다고 주장하지 않는다.
- HTML을 바꿀 때는 해당 생성 스크립트도 확인한다. 생성 결과만 고치면 재생성 시 변경이 사라질 수 있다. 생성기를 무조건 전체 실행하지 말고 출력 범위를 확인한다.
- `.vs`, `Intermediate`, `Binaries`, `DerivedDataCache`, `Saved`는 일반 공유 소스가 아니다. 디스크 정리 요청 없이 삭제하지 않으며 Saved의 로그·세이브·복구본을 버리지 않는다.
- 커밋·푸시·외부 업로드는 사용자가 요청한 범위만 수행한다. 에셋이나 아트 공개 권한을 추정하지 않는다. 이 문서는 자동 푸시 권한을 부여하지 않는다.

### 빌드와 테스트 예시

아래 명령은 **프로젝트 루트**에서 실행한다. 엔진 설치 경로는 작업 PC에 맞춘다. 테스트/빌드 도구의 파일 쓰기 범위와 에디터 종료 필요성을 확인한 뒤 실행한다.

```powershell
# Editor 빌드: Scripts가 기본 UE_5.8 경로를 사용한다.
& .\Scripts\build_editor.ps1
# 다른 설치 경로라면 -EngineRoot 인자를 전달한다.
```

```powershell
# 기존 AR 자동 테스트. 실행 중 프로젝트 저장/에셋 수정 작업을 병행하지 않는다.
$projectRoot = (Get-Location).Path
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
& "$engineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$projectRoot\Action_RogueLike.uproject" -unattended -nop4 -NullRHI -nosound -nosplash `
  '-ExecCmds=Automation RunTests AR.' '-TestExit=Automation Test Queue Empty' `
  "-ReportExportPath=$projectRoot\Saved\Automation\DeveloperCheck" `
  "-abslog=$projectRoot\Saved\Logs\DeveloperCheck.log"
```

문서만 변경했다면 다음을 확인한다. Python·Node.js는 작업 환경에서 사용할 수 있는 실행 파일을 사용한다.

```powershell
python Scripts/validate_development_guides.py
python Scripts/test_enemy_details_guide.py
python Scripts/test_controls_guide.py
python Scripts/test_timer_guide_references.py
node Scripts/test_node_search_guide.js
node Scripts/test_guide_controls.js
git diff --check
```

실제 검증에서는 정상 성공뿐 아니라 비용 부족·탄약 소진·연타·도중 CC·아이템 버리기·소유자 사망/파괴·레벨 종료를 확인한다. 시간을 다루는 기능은 World/Player 속도 변경과 Pause도 확인한다. 반환값·Failure Reason·Output Log를 확인한다. 자동 테스트만으로 화면·입력·장애물 우회·최종 패키지까지 검증했다고 주장하지 않는다.

## 11. 현재 확인된 제한·미해결 사항

아래는 **2026-10-04 진단 시점**의 기록이다. 담당자의 요청과 현황을 확인하고 해결되었다면 기록을 갱신한다. 새로운 콘텐츠 제작 요청을 이 문제들 전체의 수정 요청으로 확대하지 않는다.

1. **비에디터 Development 게임 빌드가 실패했다.** `ARCrowdControlTests.cpp`와 `ARFoundationComponentTests.cpp`가 에디터 전용 `UFunction::GetMetaData`를 게임 대상에서도 컴파일한다. 에디터 전용 검사의 빌드 조건/배치를 수정해야 한다. Editor 빌드 성공은 게임 빌드 성공의 증명이 아니다.
2. **기존 테스트 후 강제 GC에서 충돌을 재현했다.** `AR.Foundation.Items.ActiveRelicSlotInput` 종료 후 GC하면 이전 테스트 세계의 도달 불가능한 아이템에 `ReceiveItemUnregistered`가 호출되어 assertion이 발생한다. 테스트 fixture의 종료 처리와 GC 중 알림 안전성을 조사해야 한다. 정상적인 Actor 생명주기로 구성한 독립 검사는 30회 통과했으며 일반 플레이에서 동일 장애가 발생하는지는 미확정이다.
3. **기본 조작 설명과 현재 기본 IMC에 차이가 있다.** 검사한 IMC의 9건은 WASD, Space, F, E/R, 좌클릭이다. I/Escape/1/2/3/Shift/Q/X/우클릭은 해당 기본 목록에서는 미연결이고 ConsumableSlotActions는 비어 있었다. 다른 프로필이나 BP 개별 입력까지 전부 조사한 결과는 아니다. 계획과 구현 완료를 혼동하지 않는다.
4. 최종 패키지 실행·Shipping·다른 PC·장시간 플레이는 미검증이다. 네트워크 대응이나 모든 물리·음향·애니메이션의 독립 Player 시간도 보장하지 않는다.

당일 확인된 범위는 기존 AR 테스트 68건, BP 10개·아이템 정의 2개의 검증, 가이드 HTML 21개의 정적 검증, Windows 쿠킹, 화면 없는 600프레임 실행 등이다. 건수는 앞으로 달라지므로 다음에는 최신 결과로 보고한다. '전체 시스템 무결함' 또는 '배포 준비 완료'라는 의미가 아니다.

## 12. 작업 완료 보고

마지막에 다음을 간결하게 보고한다.

- 제작/수정한 내용과 대상 파일·에셋·설정 위치.
- 콘텐츠 측과 공통 시스템 측의 변경 범위.
- 효과/예약의 소유자와 정상 종료·취소·아이템 해제 시 정리 책임.
- 실행한 검증과 결과, 실행하지 않은 항목.
- 알려진 문제, 필요한 담당자 판단, 다음 안전한 작업.
- Git 커밋·푸시 또는 공유 설정을 변경했는지.

사람인 개발자에게는 추상적으로 'Self를 가져오기'라고만 설명하지 말고, **어떤 BP에서 어떤 노드의 어떤 핀을 연결하는지** 알 수 있게 설명한다. 모르는 핀·기본값은 추측하지 않고 코드/디테일을 확인한 뒤 안내한다.
