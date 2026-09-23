# Foundation 구현 인수인계 기록

**기록일:** 2026-09-23  
**프로젝트:** Unreal Engine 5.8 / Paper2D / 32×32 기본 캐릭터·타일 기반 탑뷰 액션 로그라이크
**안정 지점:** `Action_RogueLikeEditor Win64 Development` 빌드 성공, `AR.Foundation` 자동화 테스트 15/15 성공

이 문서는 다음 작업에서 “Foundation 구현 이어서 진행해줘”라고 요청했을 때 바로 이어가기 위한 현재 상태 기록이다. 방향 문서는 아래 세 문서를 기준으로 유지한다.

- `FOUNDATION_SYSTEM_PLAN.md`: 게임 규칙과 수치·공식
- `FOUNDATION_CODE_ARCHITECTURE.md`: C++ 소유권과 파일 구조
- `FOUNDATION_BLUEPRINT_PLAN.md`: 콘텐츠 Blueprint 규약과 노드 사용법

## 1. 현재 구현 완료 범위

### 프로젝트·공통 기반

- Paper2D, Enhanced Input, Gameplay Tags 및 Foundation 의존 모듈 설정
- 전투 팀, 피해 전달 방식·속성, 자원, 수정치, 요청 결과와 각종 Handle 공통 타입
- Foundation 전용 로그 카테고리와 네이티브 Gameplay Tag
- 2D 프로젝트용 렌더링 경량 설정과 Foundation 충돌 채널 예약

### 스탯·자원

- `UARStatsComponent`
  - 기본값 + 고정값 + 합연산% + 곱연산% 계산
  - 독립 피해 감소 곱연산
  - 시간 제한 수정치, 출처별 조회·제거, 중첩 한 개 제거
  - 회피·치명타·강인함·쿨다운 감소 상한과 방어력 최저 0 등 안전 규칙
  - 보장형 일반 무적·회피 토큰
- `UARHealthComponent`
  - 체력, 회복, 피해, 사망 이벤트
  - 보호막 후입선출 소비, 영구·시간 제한 보호막, 필터 제거
- `UARStaminaComponent`
  - 성공한 소비 후 회복 대기시간 갱신, 초당 회복
- `UARManaComponent`
  - 소비, 회복, 초당 회복
- 회복·소비·보호막 Blueprint 함수 라이브러리

### 피해·지속 피해·경직·상태이상

- `UARCombatSubsystem`과 순수 계산기 `FARDamageResolver`
- 물리·화염·마법·공허 피해, 직접·지속 피해, 방어·방관·취약·증폭·감소·회피·치명타 계산
- 공허 피해의 일반 방어·감소·무적 우회 및 선택적 보호막 무시
- 최종 피해 정수 버림과 성공 피해 최소 1 보장
- 흡수 누적 후 정수 단위 체력 회복
- 단일 통합 DOT 스케줄러
  - 첫 틱은 틱 간격 뒤
  - 독립 중첩 또는 같은 이름 갱신
  - 끊긴 프레임의 밀린 틱 보전
  - 공격자 제거 뒤 마지막 스냅샷 사용
- 경직 저항 비교, 슈퍼아머, 경직 면역, 선택형 그로기 게이지·회복·0 이벤트
- 상태이상 Definition Data Asset과 Runtime Component
  - 강인함 기반 지속시간 감소
  - 같은 태그는 더 긴 남은 시간 유지
  - Stun/Root 이동·행동 차단 통로

### 행동·이동·캐릭터·카메라

- `UARActionComponent`
  - Action Handle 시작·종료·취소
  - 경직·기절·구르기·기본 이동·아이템 해제·사망 취소 사유
  - 행동별 취소 규칙, 구르기/기본 이동 차단
  - Action 소유 스탯 수정치·슈퍼아머·이동 잠금·히트박스 자동 정리
- `Action Delay` 비동기 Blueprint 노드
  - Handle 종료·취소 시 Completed 대신 Cancelled 출력
- `AARActionHitboxActor`
  - Action Handle 등록, 대상당 1회/매 Overlap 정책, Action 종료 시 자동 제거
- `UARMovementControlComponent`
  - 공통 기본 이동, Action 이동, 이동 잠금, 즉시 정지
- `AARBaseCharacter`, `AARBaseEnemy`, `AARPlayerCharacter`
  - 공통 전투 컴포넌트 구성
  - WASD 이동, 마우스 조준, 스태미나 기반 구르기, 구르기 중 보장 회피
- `UARCameraFollowComponent`
  - 정사영 탑뷰 카메라, 거리 기반 추적 가속과 최대 지연 제한

### 무기·유물·스킬 로드아웃

- Weapon / Active Relic / Passive Relic Definition Data Asset 계층
- Blueprintable Runtime Item Instance
  - 장착·해제 이벤트
  - 기본 스탯 수정치 자동 등록·해제
  - 자기 소유 수정치와 UI 상태 관리
- `UARLoadoutComponent`
  - 무기 1개, 액티브 유물 2개, 패시브 유물 무제한
  - Revision 기반 Begin/Commit 획득 토큰과 오래된 요청 거부
  - 무기 교체, 유물 폐기 요청, 무기 진화 후보·Commit
  - Definition의 스킬 자동 등록·해제
  - 같은 Input Tag 스킬을 우선순위 그룹별로 검사
  - 같은 우선순위 그룹은 쿨다운·MP·스태미나를 원자적으로 검사·소비
  - Action Handle로 각 런타임 스킬 실행, 쿨다운 UI Snapshot 제공

### 소모품

- Consumable Definition Data Asset과 Blueprintable Runtime Instance
- `UARConsumableComponent`
  - 기본 최종 스탯 3칸, 한 칸에 하나, 동일 소모품 독립 획득
  - 즉발 사용, 사용 성공 후 제거, Action 미점유
  - 슬롯 수 증가·감소 반영
  - 감소 시 높은 슬롯부터 월드 드롭 요청 이벤트
  - 슬롯 Snapshot과 변경 이벤트

### 상호작용·픽업·UI 통로

- `IARInteractableInterface`
- `UARInteractionComponent`
  - `ARInteractable` 채널 구체 탐색
  - 우선순위 → 거리 순 후보 선택
  - 선택적 시야 검사, 현재 안내 문구, F 상호작용 요청
- `AARLoadoutItemPickup`, `AARConsumablePickup`
  - 공통 획득 API 호출 후 성공할 때만 자체 제거
- `UARUIManagerComponent`
  - 인벤토리·진화·상점·대화·메뉴 등 점유 화면 한 개만 허용
  - UI가 열려도 월드 시간은 유지하고 플레이어 게임 입력만 차단
  - Controller의 Game/UI 입력 모드 전환
  - 사망 시 열린 화면과 Loadout 대기 요청 정리
- 인벤토리·뒤로가기·상호작용·소모품 슬롯용 Enhanced Input 연결 지점
- `GetLoadoutItemDisplayData`
  - 인벤토리 Widget이 런타임 객체 내부를 직접 해석하지 않도록 이름·설명·아이콘·제공 스탯·스킬·현재 UI 상태를 복사한 표시 구조체 반환
  - Definition의 고정 스탯 수정치를 지역화 가능한 표시 Text로 자동 변환
- `IARCombatTargetInterface`
  - C++ 캐릭터 기반에서만 구현하고 Blueprint 자식은 상속해 사용
  - 추상 부모의 `BlueprintNativeEvent`가 구체 자식에서 기본값을 반환하던 문제를 제거

## 2. 현재 검증 결과

### 빌드

```text
Target: Action_RogueLikeEditor Win64 Development
Result: Succeeded
```

### 자동화 테스트

```text
AR.Foundation.Combat.DamageFormula       Success
AR.Foundation.Combat.DotCatchUp          Success
AR.Foundation.Combat.DotRefresh          Success
AR.Foundation.Combat.EvasionAndMinimum   Success
AR.Foundation.Combat.VoidRules           Success
AR.Foundation.Stats.IndependentReduction Success
AR.Foundation.Stats.ModifierFormula      Success
AR.Foundation.Action.OwnedCleanup        Success
AR.Foundation.Combat.StaggerGroggyRules  Success
AR.Foundation.Items.ConsumableDropAtomic Success
AR.Foundation.Items.ConsumableSlotReduction Success
AR.Foundation.Items.LoadoutAcquisitionRevision Success
AR.Foundation.Items.LoadoutDropAtomic    Success
AR.Foundation.Items.SkillPriorityTransaction Success
AR.Foundation.Status.TenacityAndRefresh  Success
합계: 15 성공 / 0 실패 / 0 경고
```

## 3. 다음 작업에서 가장 먼저 할 일

1. **누락된 Blueprint 편의 계층과 UI 이벤트 점검**
   - 상점·진화 후보처럼 아직 소유하지 않은 Definition용 표시 데이터 생성 노드
   - HUD가 Tick 조회 없이 구독할 통합 Snapshot/이벤트 점검
2. **추가 자동화 테스트**
   - 보호막 후입선출·시간 만료
   - 행동 취소 시 이동 잠금·슈퍼아머·히트박스 정리
   - UI 화면 점유와 입력 차단
3. **Editor에서 콘텐츠 에셋 생성**
   - `IA_Move`, `IA_Roll`, `IA_Interact`, `IA_Inventory`, `IA_UIBack`, 소모품 슬롯 IA와 `IMC_Player`
   - `BP_ARPlayer`, `BP_ARBaseEnemy`, 공용 Hitbox/Pickup BP
   - 테스트용 Weapon/Relic/Consumable/Status Definition과 Runtime BP
   - `WBP_HUD`, `WBP_Inventory`, `WBP_EvolutionSelection`
   - Foundation Test Map
4. Test Map 검증 후에만 기존 TopDown 기본 GameMode/Map을 Foundation 쪽으로 교체한다.
5. 기반 사용법이 실제 BP에서 검증되면 `FOUNDATION_ENEMY_CONTENT_GUIDE.md`와 아이템 제작 가이드를 작성한다.

## 4. 현재 알려진 주의점

- 현재 C++는 빌드되며 Damage/DOT·Action·Stagger/Groggy·Status·Loadout/Consumable 핵심 규칙 15개가 자동 검증된다.
- 전투 대상 인터페이스는 Blueprint에서 새로 구현하지 않는다. `AARBaseCharacter`의 Blueprint 자식을 만들어 상속된 팀·생존 판정을 사용한다.
- 유물·소모품 수동 폐기는 픽업 Spawn 성공 뒤에만 인스턴스를 제거한다. 슬롯 감소 Spawn 실패 시 초과 슬롯에 보존하고 재시도한다.
- 실제 Input Action, Mapping Context, Data Asset, Runtime BP, Widget, Test Map은 아직 생성하지 않았다. C++ 입력 포인터가 비어 있으면 해당 기능은 실행되지 않는다.
- 기존 `/Game/TopDown` 기본 맵과 GameMode를 의도적으로 유지하고 있다. Foundation Test Map이 확인되기 전에는 변경하지 않는다.
- 월드 드롭은 `UARWorldItemDropSubsystem`이 처리한다. 실제 프로젝트에서는 컴포넌트의 Pickup Class를 외형 BP 자식으로 지정해야 한다.
- 실제 플레이 화면, Paper2D 픽셀 스케일, 애니메이션, UI 디자인은 검증 전이다.

## 5. 완성도 판단

현재 Foundation 전체 구현 완성도는 **10단계 중 5단계**로 평가한다.

- C++ 기반 규칙과 핵심 API: 약 6~7단계
- 테스트·안전성: 약 5~6단계
- 실제 Blueprint/에셋 연결과 플레이 가능한 프로토타입: 약 2~3단계

즉 핵심 뼈대와 주요 트랜잭션 테스트는 존재하지만, 실제 콘텐츠 에셋을 연결해 한 판을 플레이하고 팀원이 사용할 수 있다고 말하려면 Editor 에셋, HUD/인벤토리, Test Map 검증이 더 필요하다.
