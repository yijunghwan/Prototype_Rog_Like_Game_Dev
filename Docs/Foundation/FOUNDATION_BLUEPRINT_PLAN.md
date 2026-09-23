# Foundation 블루프린트 설계서

> **상태:** 구현 전 Blueprint 계약서  
> **상위 문서:** `FOUNDATION_SYSTEM_PLAN.md`, `FOUNDATION_CODE_ARCHITECTURE.md`  
> **목적:** 콘텐츠 개발자가 Blueprint에서 무엇을 만들고, 어떤 C++ 노드에 어떤 값을 넣으며, 어떤 결과·이벤트를 받아야 하는지 정의한다. 이 문서는 실제 Blueprint 에셋을 아직 만들지 않는다.

---

## 1. Blueprint의 역할과 경계

Blueprint는 무기·유물·소모품·적·픽업·UI의 **콘텐츠 동작과 연출**을 만든다. 다음 값은 Blueprint 변수가 아니라 C++ Component가 최종 소유한다.

- 최종 스탯, 현재 체력·보호막·MP·스태미나
- 피해 공식과 대상 유효성, 회피·필중·치명타·방어력
- 상태이상 지속시간, 경직·그로기·슈퍼아머 판정
- Action 시작/취소/종료와 Action 소유 타이머/히트박스/이동 잠금
- 무기 1개·액티브 유물 2개·패시브 유물 무제한의 장착 상태

Blueprint가 하는 일은 다음과 같다.

- 어떤 시점에 어떤 피해/경직/상태이상 요청을 보낼지 결정
- 히트박스, 투사체, 장판, 플립북, VFX/SFX, 카메라 연출 제작
- 고유 패시브 조건, 고유 스택, 고유 자원 같은 콘텐츠 규칙 구현
- C++에서 반환한 결과에 맞춰 피격 연출·UI·사운드 처리

### Blueprint 공통 금지 규칙

- `CurrentHealth`, `CurrentMP`, `CurrentStamina`, Final Stat을 BP에서 직접 대입하지 않는다.
- Engine 기본 `Apply Damage`와 Foundation의 `Apply Combat Damage`를 섞지 않는다.
- 공격 선딜/후딜/차지/히트박스를 일반 `Delay` 또는 독립 Timer로 만들지 않는다.
- 적 이동/돌진을 장시간 `SetActorLocation`이나 독립 Timeline으로 만들지 않는다.
- 아이템 Data Asset에 쿨다운, 스택, 남은 시간, 장착자 같은 런타임 값을 쓰지 않는다.

---

## 2. 에셋 이름과 위치 규약

| 종류 | 접두사 | 예시 | 위치 |
|---|---|---|---|
| 캐릭터 BP | `BP_` | `BP_ARPlayerBase` | `Content/Game/Foundation/Blueprints/Characters` |
| 적 BP | `BP_` | `BP_Enemy_Slime` | `Content/Game/Characters/Enemies/[적이름]` |
| 런타임 아이템 BP | `BP_` | `BP_WeaponRuntime_Example` | `Content/Game/Weapons/[무기]/Runtime` |
| 픽업 BP | `BP_` | `BP_LoadoutItemPickup` | `Content/Game/Foundation/Blueprints/Pickups` |
| Definition Data Asset | `DA_` | `DA_Weapon_Example` | 해당 콘텐츠의 `Data` |
| 입력 에셋 | `IA_`, `IMC_` | `IA_Roll`, `IMC_Player` | `Content/Game/Foundation/Input` |
| Widget | `WBP_` | `WBP_HUD` | `Content/Game/Foundation/UI` 또는 `Content/Game/UI` |
| 테스트 BP | `BP_Test_` | `BP_Test_CombatDummy` | `Content/Game/Foundation/Test` |
| Flipbook/Sprite/Texture | `FB_`, `SPR_`, `T_` | `FB_Player_Idle` | 콘텐츠별 `Art` |

개별 무기/유물/적의 이름은 에셋 이름과 Data Asset ID에만 넣는다. Foundation 부모 BP와 C++ 노드 이름에는 콘텐츠 이름을 넣지 않는다.

---

## 3. 만들 Blueprint 부모와 역할

### 3.1 `BP_ARPlayerBase`

**부모 C++:** `AARPlayerCharacter`  
**위치:** `Foundation/Blueprints/Characters`  
**역할:** Paper2D 외형, 기본 Flipbook 상태 전환, Player 전용 카메라/피격/사망 연출의 공통 BP 부모.

| 받을 이벤트/값 | BP가 하는 일 | 반환/다음 행동 |
|---|---|---|
| 이동 방향, Aim Direction | 4/8방향 Flipbook 선택, 좌우 반전 | 없음 |
| `OnCharacterDeath` | 사망 Flipbook·연출 시작, 조작 불가 UI 처리 | 사망 정리는 C++가 이미 처리 |
| `OnDamageApplied(Result)` | 피격 플래시, 피해 숫자/VFX 요청 | Result의 실제 피해/치명타/속성 활용 |
| `OnStaggered(Result)` | 경직 Flipbook/사운드 | 실제 Action 취소는 C++가 처리 |
| `OnStatusChanged` | 상태 아이콘 또는 외형 변화 | 상태 자체를 변경하지 않음 |

이 BP는 무기 공격 그래프나 유물 효과를 직접 갖지 않는다. 해당 동작은 런타임 아이템 BP가 수행한다.

### 3.2 `BP_ARBaseEnemy`

**부모 C++:** `AARBaseEnemy`  
**위치:** `Foundation/Blueprints/Characters`  
**역할:** 적 개발자가 상속하는 최소 공통 부모. 공통 피격/경직/사망 Flipbook 연결점 제공.

적 BP의 공격 Event Graph는 반드시 `Try Start Action`으로 시작한다. AI 이동은 `Request Basic Move` 또는 `Request Action Move`를 사용한다. 적별 AI, 타깃 탐색, 공격 선택, 외형은 각 적 자식 BP에 둔다.

### 3.3 `BP_LoadoutItemPickup`

**부모/구현:** 일반 Actor + `IARInteractable`  
**역할:** 무기·액티브·패시브 유물 Definition을 월드에서 획득하게 하는 공용 픽업.

**Details 입력값**

| 값 | 형식 | 의미 |
|---|---|---|
| `ItemDefinition` | `UARLoadoutItemDefinition` 참조 | 획득할 아이템의 정적 정보 |
| `PickupVisual` | Sprite/Flipbook/Widget 등 | 월드 표시용, 규칙과 무관 |
| `InteractionText` | Text | F 상호작용 안내 |

**상호작용 흐름**

```text
F 입력
→ Player의 Begin Loadout Acquisition(ItemDefinition)
→ Result가 Ready면 Commit Loadout Acquisition(Token)
→ 성공 시 픽업 Destroy
→ 슬롯 가득/실패면 픽업 유지 및 실패 UI 표시
```

`Begin`이 “무기 교체 필요”를 반환하면 바로 Commit할 수 있다. 액티브 유물이 가득 찬 경우에는 현재 규칙상 교체 UI를 열지 않고 획득을 거절하며 픽업을 유지한다.

### 3.4 `BP_ConsumablePickup`

**부모/구현:** 일반 Actor + `IARInteractable`  
**Details 입력:** `UARConsumableDefinition`, 월드 외형, 상호작용 텍스트.  
**동작:** F 입력 → `Try Acquire Consumable(Definition)` → 성공 시 Destroy, 빈 슬롯 없음이면 유지. 동일 소모품도 각각 새 Instance로 획득한다.

### 3.5 `BP_ARCombatHitbox` (공용 또는 무기별 자식)

**부모 C++:** `AARActionHitboxActor`  
**역할:** 공격의 짧은 접촉 판정. 피해 수치를 소유하지 않고, 어떤 공격 Runtime BP가 어떤 Damage Request를 만들지 연결해 준다.

**생성 시 입력값**

| 값 | 의미 |
|---|---|
| `ActionHandle` | 이 Hitbox를 소유한 행동 Handle |
| `Instigator` | 공격자 Character/환경 출처 |
| `CollisionShape` | Box/Circle/Capsule 등 범위 |
| `Lifetime` | Hitbox 유지 시간. Action 종료 시 즉시 정리 |
| `HitPolicy` | 대상당 1회/다중 적중, 이미 맞은 Actor 목록 정책 |

**Overlap 흐름**

```text
유효 후보 Overlap
→ Can Damage Target(Attacker, Target)
→ 무기/유물 BP가 FARCombatDamageRequest 구성
→ Apply Combat Damage(Request)
→ 성공 Result로 피격 연출
→ 필요할 때 Apply Stagger And Groggy Damage(StaggerRequest)
```

Hitbox는 공격자 팀, Action Handle, 이미 맞은 대상 목록만 관리한다. 피해 공식, 경직 공식, 대상 체력 변경은 관리하지 않는다.

### 3.6 `BP_EnvironmentCombatSource`

**부모/구현:** 일반 Actor + `UARCombatSourceComponent`  
**역할:** 가시 함정, 용암, 낙석, 맵 장판 등 환경 공격의 공용 예제 부모.

Details에서 Environment 팀·출처 ID·표시 이름을 가진다. 환경 BP는 피해 요청의 공격자에 자신 또는 Combat Source Component를 넣고 `Apply Combat Damage`를 호출한다. 환경은 피해 대상이 될 수 없다.

---

## 4. 아이템 Runtime Blueprint 계약

### 4.1 부모 클래스

| BP 부모 | C++ 부모 | 사용처 |
|---|---|---|
| `BP_WeaponRuntime_Base` | `UARLoadoutItemInstance` | 모든 무기 Runtime BP |
| `BP_ActiveRelicRuntime_Base` | `UARLoadoutItemInstance` | 모든 액티브 유물 |
| `BP_PassiveRelicRuntime_Base` | `UARLoadoutItemInstance` | 모든 패시브 유물 |
| `BP_ConsumableRuntime_Base` | `UARConsumableInstance` | 모든 소모품 |

Runtime BP는 월드 Actor가 아니다. 장착 중인 플레이어의 런타임 UObject이며, 월드 위치가 필요한 투사체/히트박스/장판은 별도 Actor를 Spawn한다.

### 4.2 Runtime BP가 받는 이벤트

| 이벤트 | 입력값 | BP 역할 | 반환값 |
|---|---|---|---|
| `OnItemRegistered` | Owner, Definition, Instance ID | 조건부 패시브 시작, 이벤트 구독, 초기 UI 상태 설정 | 없음 |
| `OnItemUnregistered` | 제거 이유 | 이벤트 구독/Spawn Actor/연출 정리 | 없음 |
| `CanExecuteItemSkill` | `SkillId` | 스택·고유 자원·특수 조건만 순수하게 확인 | `bool CanExecute`, 선택 실패 표시 태그 |
| `ExecuteItemSkill` | `SkillId`, `ActionHandle` | Action 아래에서 실제 스킬/공격 구현 | 없음 |
| `ExecuteConsumableUse` | Owner | 즉발 회복/효과 구현 | `bool Consumed`, 실패 이유 |

`CanExecuteItemSkill`은 스택을 줄이거나 자원을 소비하거나 Actor를 Spawn하지 않는다. Loadout Component가 후보를 검사할 때 여러 번 호출할 수 있기 때문이다.

### 4.3 Runtime BP가 호출하는 공통 기능

| 노드 | 주요 입력 | 주요 결과 | 사용 목적 |
|---|---|---|---|
| `Get Item Owner` | 없음 | Owner Character | 공격자/회복 대상 확보 |
| `Get Item Definition` | 없음 | Definition | 표시 정보/정적 Skill Definition 읽기 |
| `Apply Stat Modifier` | Stat, 방식, 값, 시간, 출처 | Modifier Handle | 조건부 버프/디버프 |
| `Remove Own Item Modifiers` | 선택 Modifier ID/전체 | 제거 개수 | 고유 버프 해제 |
| `Set Item UI State` | State ID, 현재/최대값 | 성공 여부 | 게이지/숫자/스택 UI 갱신 |
| `Try Start Action` | Owner, Action 설정 | Started, Handle, 실패 이유 | 스킬 생명주기 시작 |
| `Apply Combat Damage` | Damage Request | Damage Result | 실제 피해 요청 |
| `Apply Stagger And Groggy Damage` | Stagger Request | Stagger Result | 별도 경직/그로기 요청 |

아이템의 Definition에 이미 들어 있는 고정 Modifier는 C++ 등록 흐름이 자동 적용한다. Runtime BP는 “장착 직후 항상 적용되는 고정 스탯”을 다시 Apply하지 않는다.

---

## 5. Data Asset을 Blueprint에서 다루는 방법

### 5.1 무기/유물 Definition에서 입력할 값

| 그룹 | 필드 | 데이터 타입/설명 |
|---|---|---|
| 식별 | `PrimaryAssetId`, `DefinitionTag`, Item Type Tag | 에셋 로드 ID, 게임 규칙 ID와 Weapon/Active/Passive 분류 |
| 표시 | 이름, 짧은/상세 설명, 아이콘 | 인벤토리/HUD용 정적 정보 |
| 고정 효과 | `DefaultStatModifiers[]` | 스탯, Flat/Additive/Multiplicative, 값, 출처 정보 |
| 스킬 | `SkillDefinitions[]` | Skill ID, Input Tag, 우선순위, 쿨다운, MP/스태미나 비용, Action 설정 |
| 런타임 | `RuntimeBehaviorClass` | 실제 행동을 구현한 Runtime BP 클래스 |
| UI | `UIStateDisplayDefinitions[]` | 게이지/숫자/소형 스택, HUD/상세 표시 위치 |
| 무기 전용 | 진화 그룹/단계/다음 후보 | 무기 Definition에만 존재 |

Definition의 “상세 설명”은 `FText`다. 제공 스탯 목록은 `DefaultStatModifiers`를 UI가 자동 변환해 표시한다. Runtime BP에서 조건부로 더해지는 효과는 설명 Text와 Runtime BP 모두에 의도적으로 기록해야 한다.

### 5.2 `SkillDefinition`이 보장하는 값

`SkillDefinition`은 공통 시작 검증에 필요한 **정적 비용**만 가진다.

- `SkillId`, `InputTag`, `InputPriority`
- 기본 쿨다운, 최소 쿨다운
- MP 비용, 스태미나 비용
- Action 취소/이동/구르기 규칙
- HUD에 표시할 아이콘·이름·슬롯 정보

특수 자원, 현재 스택, 대상 거리 같은 런타임 조건은 Runtime BP의 `CanExecuteItemSkill`에서 판단한다. 스킬 실행 때만 확정되는 추가 비용은 `ExecuteItemSkill` 내부에서 별도 `Try Consume`을 호출하되, 정적 비용과 중복 소비하지 않는다.

### 5.3 상태이상 Definition

`DA_Status_Stun`, `DA_Status_Root`는 `UARStatusEffectDefinition`을 사용한다. Primary Asset ID, Status Tag, 표시 이름·아이콘, 기본 지속시간, 동일 상태 갱신 정책, 이동·구르기·Skill Group 제한과 적용 시 Action 취소 사유만 저장한다. 남은 시간과 적용 대상·출처는 Data Asset에 기록하지 않는다.

---

## 6. Blueprint 공통 노드 상세 계약

### 6.1 스탯 노드

| 노드 | 입력 | 출력 | 동작 |
|---|---|---|---|
| `Apply Stat Modifier` | Target, Stat Type, Operation, Value, Duration(-1 영구), Source Category, Source ID, Display Name, 조건부 무적/회피 보장 | Modifier Handle, Success | 최종 스탯 변경 및 만료 예약. 보장 옵션은 호환 스탯일 때만 노출 |
| `Remove Stat Modifier` | Target, Modifier Handle | Success | 정확한 수정치 하나 제거 |
| `Remove Stat Modifiers` | Target, Category, 선택 Source ID | Removed Count | 출처 ID 기준 제거. Display Name은 조회 키가 아님 |
| `Clear Stat Modifiers` | Target, All/Weapon/Relic/Buff/Other | Removed Count | 분류 전체 초기화 |
| `Get Final Stat` | Target, Stat Type | Final Value | 캐시된 최종값 조회 |
| `Get Stat Modifier Remaining Time` | Target, Handle | Exists, IsPermanent, Remaining Seconds | UI/디버그용 |
| `Get Stat Modifiers By Source` | Target, Category, Source ID | Exists, Stack Count, Longest Remaining Time | 버프 UI/조건 검사 |
| `Remove One Stat Modifier Stack` | Target, Category, Source ID, Newest/Oldest | Removed, Handle | 중첩 하나만 제거 |

`On Final Stat Changed` 이벤트는 `Target`, `StatType`, `OldValue`, `NewValue`를 전달한다. `On Stat Modifiers Changed`는 Source ID, Display Name, 추가/제거/만료, 중첩 수, 가장 긴 남은 시간을 전달한다. UI/조건부 패시브는 이 이벤트를 구독할 수 있으나, 이벤트 안에서 같은 스탯을 무한 수정하는 구조는 만들지 않는다.

### 6.2 회복·자원·보호막 노드

| 노드 | 입력 | 출력 | 동작 |
|---|---|---|---|
| `Restore Health` | Target, Amount, Source ID, Source Name | Applied Amount, New Health | 최대 체력까지만 회복 |
| `Restore MP` | Target, Amount, Source | Applied Amount, New MP | 최대 MP까지만 회복 |
| `Restore Stamina` | Target, Amount, Source | Applied Amount, New Stamina | 최대 스태미나까지만 회복 |
| `Try Consume MP` | Target, Amount, Source | Success, New MP, Failure Reason | 가능할 때만 소비 |
| `Try Consume Stamina` | Target, Amount, Source | Success, New Stamina, Failure Reason | 성공 소비면 재생 대기 재시작 |
| `Can Afford Resources` | Target, MP Amount, Stamina Amount | Can Afford, Missing Resource | 값을 바꾸지 않는 사전 조회 |
| `Try Consume Resources` | Target, MP Amount, Stamina Amount, Source | Success, New MP/Stamina, Failure Reason | 둘 다 가능할 때만 함께 소비 |
| `Apply Shield` | Target, Amount, Duration(-1 영구), Source | Shield Handle, New Total Shield | 새 보호막을 LIFO 스택 맨 위에 추가 |
| `Remove Shield` | Target, Handle 또는 Source, Remove All | Removed Amount/Count | 보호막 제거 |
| `Get Current Resource` | Target, Health/Shield/MP/Stamina/Groggy | Current, Max, Ratio | UI와 조건 조회 |
| `Get Current Shield` | Target | Total Shield | 활성 보호막 총량 조회 |

회복력 기반 비율 회복은 콘텐츠 BP가 `MaxHealth × (FinalRecoveryPower / 100)`처럼 필요한 양을 계산한 뒤 `Restore Health`를 호출한다. `Restore Health`는 어떤 공식을 자동으로 가정하지 않는다.

### 6.3 피해 노드

`Apply Combat Damage`는 핀이 너무 많아지는 것을 피하기 위해 `FARCombatDamageRequest` 구조체 입력 하나를 중심으로 제공한다. BP에서 `Make Combat Damage Request` 노드로 구조체를 만든다.

| 구조체 필드 | 의미 |
|---|---|
| Attacker / Combat Source | 공격자 Character 또는 환경 출처 |
| Target | 피격 Actor |
| Delivery | Direct 또는 DamageOverTime |
| Attribute | Physical, Fire, Magic, Void |
| BaseDamage | 계수 없이도 가능한 기본 깡 피해 |
| AttackPowerCoefficient / SpellPowerCoefficient | 0이면 해당 스탯을 계산에 쓰지 않음 |
| `bGuaranteedHit` | 회피 판정 건너뜀 |
| `bApplyEvasion` | 회피 판정 사용 여부 |
| `bCanCrit` | 치명타 사용 여부 |
| `bApplyAmplification` | 공격자 증폭/대상 취약 적용 여부 |
| `bIgnoreDefense` | 방관 100% 취급 |
| `bIgnoreShield` | 보호막 관통 여부 |
| `bApplyAbsorption` | 공격자의 흡수 누적 적용 여부 |
| `bApplyOnHitEffects` | 성공 직접 타격의 OnHit 발생 여부 |
| Source ID / Damage Display Name | 로그·피해 숫자·후속 효과 식별 |

| 노드 | 입력 | 출력 |
|---|---|---|
| `Can Damage Target` | Attacker/Source, Target | Can Damage, Failure Reason |
| `Apply Combat Damage` | Damage Request | `FARCombatDamageResult` |
| `Apply Damage Over Time` | `FARDamageOverTimeSpec` | Success, DOT Handle, Failure Reason |
| `Remove Damage Over Time` | Target, DOT Handle 또는 Source/Name | Removed Count |

Damage Result에는 성공 여부, 회피 여부, 치명타 여부, 최종 피해, 보호막 피해, 체력 피해, 대상 사망 여부, OnHit 발생 여부가 들어간다. VFX·SFX·피해 숫자는 이 Result를 기준으로 처리한다.

전투 이벤트는 `On Damage Received`, `On Damage Hit`, `On Damage Evaded`, `On Damage Blocked`, `On Shield Broken`, `On Death`를 제공한다. 이벤트 안에서 발생한 새 피해는 현재 이벤트 체인이 끝난 뒤 처리되며, OnHit 추가 피해는 기본적으로 `bApplyOnHitEffects=false`를 사용한다.

DOT Spec은 Damage Request 외에 지속시간, Tick Interval, DOT Name, 독립 중첩/갱신형 정책과 선택적 `Stagger Request Template`을 입력받는다. 갱신형은 같은 이름에 기본 피해·지속시간·간격이 정확히 같을 때만 남은 시간을 초기화한다. 템플릿이 있으면 성공한 각 틱이 경직·그로기 요청도 실행한다.

### 6.4 경직·그로기·상태이상 노드

| 노드 | 입력 | 출력 | 동작 |
|---|---|---|---|
| `Make Stagger Request` | Attacker, Target, Base Stagger Damage, Stagger Multiplier, Base Groggy Damage, Source | Request Struct | 경직/그로기 요청 구성 |
| `Apply Stagger And Groggy Damage` | Stagger Request | `FARStaggerResult` | 경직력 계수와 대상 저항/슈퍼아머 처리 |
| `Apply Super Armor` | Target, Source ID, Duration | SuperArmor Handle | 일반 경직만 막는 관리형 슈퍼아머 |
| `Remove Super Armor` | Target, Handle/Source | Success/Count | 슈퍼아머 제거 |
| `Is Super Armor Active` | Target | bool | 읽기 전용 |
| `Reset Groggy Gauge` | Target, 선택 새 현재값 | Success, Current Groggy | 고갈 잠금 해제와 게이지 초기화 |
| `Apply Status Effect` | Target, Status Effect Definition, 선택 Duration Override, Source ID | Status Handle, Actual Duration | Definition 규칙과 강인함 반영 후 적용 |
| `Remove Status Effect` | Target, Handle/Source | Removed Count | 상태 제거 |
| `Has Status Effect` | Target, Status Tag | bool | 읽기 전용 |

Stagger Result에는 경직 시도/성공/슈퍼아머 차단 여부, 실제 그로기 피해, 현재 그로기, 그로기 0 도달 여부가 들어간다. `OnGroggyGaugeDepleted`는 보스 BP가 구독해 전용 행동을 시작한다. 일반 경직은 C++가 이동 중단·Action 취소·짧은 행동 불가·면역을 처리한다.

### 6.5 Action 노드

| 노드 | 입력 | 출력 | 동작 |
|---|---|---|---|
| `Try Start Action` | Owner, Action Tag, Cancel Rules, Block Roll, Block Basic Move | Started, Action Handle, Failure Reason | 행동을 시작하고 유효 Handle 반환 |
| `Can Start Action` | Owner, Action Tag/Rules | Can Start, Failure Reason | 시작 전 UI/AI 검사 |
| `End Action` | Action Handle | Success | 정상 종료와 Action 소유 항목 정리 |
| `Cancel Action` | Action Handle, Cancel Reason | Success | 강제 취소와 동일 정리 |
| `Action Delay` | Action Handle, Duration | Completed / Cancelled | 취소된 Handle이면 즉시 Cancelled |
| `Register Action Hitbox` | Action Handle, Hitbox Actor | Registered, Failure Reason | Hitbox를 Action 생명주기에 귀속 |
| `Apply Action Stat Modifier` | Action Handle, Modifier Spec | Modifier Handle, Success | 종료/취소 시 자동 제거 |
| `Apply Action Super Armor` | Action Handle, SuperArmor Spec | SuperArmor Handle, Success | 종료/취소 시 자동 제거 |
| `Set Action Roll Blocked` | Action Handle, Blocked | Success | Action 생존 동안 구르기 제어 |
| `Set Action Basic Movement Blocked` | Action Handle, Blocked | Success | Action 생존 동안 WASD 제어 |

Action 이벤트는 `On Action Ended(Handle)`, `On Action Cancelled(Handle, Reason)`다. BP는 애니메이션, VFX, 루프 SFX, Spawn한 보조 Actor를 두 이벤트에서 정리한다. 체력 피해/등록 DOT/이미 날아간 투사체처럼 취소 뒤에도 남는 것은 의도적으로 Action 밖에 둔다.

한 입력의 전체 묶음은 `Skill Group Handle`, 각 Runtime BP 호출은 서로 다른 `Action Handle`을 받는다. BP는 그룹 비용을 직접 소비하거나 쿨다운을 시작하지 않는다. 공통 C++가 모든 참가 스킬을 검증한 뒤 한 번에 Commit하며, 취소 후 기본 환불은 없다.

### 6.6 아이템·획득·진화·소모품 노드

| 노드 | 입력 | 출력 | 동작 |
|---|---|---|---|
| `Begin Loadout Acquisition` | Player, Item Definition | Result, Acquisition Token, 교체 필요 여부 | 슬롯/Definition 검사 |
| `Commit Loadout Acquisition` | Player, Token | Success, Item Instance, Failure Reason | 실제 장착 |
| `Cancel Loadout Acquisition` | Player, Token | Success | 선택 취소 |
| `Discard Loadout Item` | Player, Item Instance ID | Success, Spawned Pickup | 월드 픽업 생성 성공 뒤 액티브/패시브 제거 |
| `Request Weapon Evolution` | Player | Evolution Result, Candidate Definitions | 다음 단계 확인 |
| `Commit Weapon Evolution` | Player, Evolution Token, Candidate Primary Asset ID | Success, New Weapon Instance | Token 재검증 후 기존 무기 제거 및 새 Instance 등록 |
| `Try Acquire Consumable` | Player, Consumable Definition | Success, Slot Index, Failure Reason | 빈 슬롯에 새 Instance |
| `Try Use Consumable Slot` | Player, Slot Index | Success, Failure Reason | 즉발 실행 후 성공 시 슬롯 비움 |
| `Get Consumable Slots` | Player | Slot Snapshot Array | UI 표시 |
| `Drop Consumable Slot` | Player, Slot Index | Success, Spawned Pickup | 픽업 생성 성공 뒤 선택 소모품 제거 |

무기는 인벤토리에서 직접 버리는 노드가 없다. 새 무기 획득 또는 진화만 무기 교체를 만든다.

### 6.7 등록 스킬·쿨다운·UI 조회 노드

| 노드 | 입력 | 출력 | 동작 |
|---|---|---|---|
| `Get Registered Skill UI Data` | Player | UI Snapshot 배열 | Registered Skill Handle, Item Instance ID, Skill ID, 입력 태그, 아이콘, 비용, 쿨다운 상태 반환 |
| `Get Skill Cooldown State` | Registered Skill Handle | Ready, Remaining, Total, Ratio | 동일 Skill ID 충돌 없이 정확한 등록 스킬 조회 |
| `Reset Skill Cooldown` | Registered Skill Handle | Success | 남은 쿨다운을 0으로 변경 |
| `Modify Skill Cooldown` | Registered Skill Handle, Delta Seconds | Success, New Remaining | 음수 감소, 양수 증가 |
| `Get Loadout Inventory` | Player | 무기/액티브/패시브 Snapshot | Runtime UObject를 직접 수정하지 않는 UI 데이터 반환 |
| `Get Loadout Item Display Data` | Item Instance ID | Display Data, Found | 아이콘·설명·자동 생성 스탯·스킬·UI 상태 반환 |
| `Get Loadout Definition Display Data` | Item Definition | Display Data, Found | 미보유 상점 상품·진화 후보의 정적 표시 정보 반환. Instance ID와 런타임 UI 상태는 비어 있음 |

두 표시 데이터 노드는 `UARLoadoutComponent`에서 호출한다. 보유 아이템은 Instance 기반 노드, 아직 소유하지 않은 상점 상품·진화 후보는 Definition 기반 노드를 사용한다. 반환된 `ProvidedStats[].DisplayText`는 즉시 목록에 표시할 수 있고, 커스텀 디자인이 필요하면 함께 반환되는 Stat Type·Operation·Value로 Widget에서 다시 표현한다.

`SkillId`는 한 Definition 내부의 의미 식별자이고 실제 등록 스킬은 `FARRegisteredSkillHandle`로 구분한다. 쿨다운 노드는 Owner+SkillId만으로 대상을 찾지 않는다.

---

## 7. 표준 Blueprint 그래프 예시

### 7.1 직접 타격 무기 스킬

```text
Execute Item Skill(SkillId, Handle)
→ SkillId 분기
→ Action Delay(선딜)
→ BP_ARCombatHitbox Spawn
→ Register Action Hitbox(Handle, Hitbox)
→ Overlap마다 Make Damage Request
→ Apply Combat Damage
→ 필요 시 Make Stagger Request
→ Apply Stagger And Groggy Damage
→ Action Delay(후딜)
→ End Action(Handle)
```

피해 Result가 성공이고 `bApplyOnHitEffects`가 켜진 경우에만 무기/유물의 OnHit 효과가 이어진다. Hitbox가 사라졌더라도 Action은 후딜이 끝날 때까지 유지할 수 있다.

### 7.2 지속 피해 장판

```text
장판 Overlap Begin
→ Can Damage Target
→ Make DOT Spec(이름, 지속시간, 0.25초 간격, 속성, 갱신/독립 정책)
→ Apply Damage Over Time
→ DOT Handle을 장판의 대상별 목록에 저장

장판 종료 또는 대상 이탈 시
→ 정책에 따라 Remove Damage Over Time 또는 그대로 유지
```

“대상 이탈 시 제거”는 장판의 콘텐츠 정책이며, DOT 시스템의 기본 규칙이 아니다.

### 7.3 경직 가능한 적 근접 공격

```text
AI 공격 선택
→ Try Start Action
→ Action Delay(선딜)
→ Hitbox Spawn/Register
→ Apply Combat Damage
→ Apply Stagger And Groggy Damage
→ Action Delay(후딜)
→ End Action

On Action Cancelled
→ Hitbox/VFX/SFX/Flipbook 정리
```

일반 경직이 성공하면 C++가 Action을 취소한다. 적 BP가 경직 신호를 별도로 받아 공격 타이머를 찾고 지우는 구조를 만들지 않는다.

### 7.4 조건부 패시브 유물

```text
OnItemRegistered
→ Owner의 On Damage Hit 이벤트 구독

On Damage Hit(Result)
→ 속성/유형/Source ID 조건 검사
→ 조건 충족 시 Apply Stat Modifier 또는 Apply Combat Damage

OnItemUnregistered
→ 이벤트 구독 해제
→ Remove Own Item Modifiers
```

OnHit에서 만든 별도 피해는 재귀 방지를 위해 `bApplyOnHitEffects = false`를 기본으로 한다.

---

## 8. UI Blueprint 계획

### `WBP_HUD`

**표시:** 체력, 보호막, MP, 스태미나, 현재 무기, 무기 스킬 아이콘/쿨다운, 액티브 유물 2칸, 아이템 커스텀 상태, 상태이상, 에임 위치.

| 구독 데이터 | 입력 | Widget 동작 |
|---|---|---|
| Health/Shield 변경 | Current, Max, Delta | 게이지와 수치 갱신 |
| Resource 변경 | Current, Max | MP/스태미나 갱신 |
| Loadout 변경 | 장착 아이템/스킬 UI Snapshot | 아이콘/키/쿨다운 영역 재구성 |
| Item UI State 변경 | State ID, Current, Max, Display Type | 게이지/숫자/1~6 스택 갱신 |
| Status 변경 | Status List | 아이콘 추가/갱신/제거 |
| `On HUD Snapshot Changed` | `FARPlayerHUDSnapshot` | 위 변경 통로를 하나로 받아 현재 HUD 전체 갱신 |

HUD는 Damage Resolver, Stats Component의 내부 Modifier 배열, 아이템 Runtime BP를 직접 수정하지 않는다.
Widget 생성 직후 `Get HUD Snapshot`을 한 번 호출해 초기 화면을 만들고, 그 뒤에는 `On HUD Snapshot Changed`를 구독한다. Snapshot에는 체력·보호막·MP·스태미나·조준 방향·현재 무기·액티브 유물·등록 스킬·소모품 슬롯이 포함된다. 쿨다운 원형 애니메이션은 Snapshot에 든 시작 상태를 바탕으로 Widget이 표시 시간만 진행하며, 시스템 Component를 매 Tick 순회하지 않는다.

### `WBP_Inventory`

**입력:** Player/Loadout/Consumable Snapshot, `GetStatBreakdown` 결과.  
**동작:** 좌측에는 최종 스탯, 우측에는 무기·액티브·패시브·소모품을 표시한다. Hover는 짧은 설명, 선택은 이미지/제공 스탯/상세 설명/스킬 정보를 표시한다.

| 사용자 입력 | 호출 노드 | 결과 |
|---|---|---|
| 액티브/패시브 우클릭 → 버리기 확인 | `Discard Loadout Item` | 성공 시 아이템 목록 갱신, 픽업 생성 |
| 소모품 우클릭 → 사용 | `Try Use Consumable Slot` | 성공 시 해당 슬롯 갱신 |
| 소모품 우클릭 → 버리기 | `Drop Consumable Slot` | 성공 시 슬롯 갱신 |
| Esc | Player UI 상태 변경 | 선택 해제 후 창 닫기 |

무기에는 버리기 버튼을 표시하지 않는다. UI가 열려 있는 동안 게임은 계속 진행되지만 Player의 게임플레이 입력은 차단한다.

### `WBP_EvolutionSelection`

**입력:** `Request Weapon Evolution`의 Candidate Definitions.  
**출력:** 사용자가 선택한 후보 Primary Asset ID와 Evolution Token을 `Commit Weapon Evolution`에 전달. 후보가 하나면 자동 확정 가능하며, 후보가 없으면 UI를 열지 않는다.

---

## 9. Blueprint 작성 완료 체크리스트

- [ ] Definition에는 정적 정보만, Runtime BP에는 런타임 상태만 있는가?
- [ ] 모든 취소 가능한 공격/차지/돌진이 `Try Start Action`으로 시작하는가?
- [ ] 모든 공격 Delay/Hitbox/임시 버프가 Action Handle 아래에 있는가?
- [ ] 피해는 `Apply Combat Damage`, 경직/그로기는 별도 `Apply Stagger And Groggy Damage`를 쓰는가?
- [ ] 공격력/주문력/경직력 계수를 사용하지 않는 깡 피해도 Base 값으로 정상 처리되는가?
- [ ] OnHit 추가 피해가 자기 자신을 무한 호출하지 않는가?
- [ ] Item 제거 시 이벤트 구독, Modifier, Spawn Actor, UI 상태를 정리하는가?
- [ ] 적 이동과 Root/Stun이 `UARMovementControlComponent` 경로를 사용하는가?
- [ ] UI는 이벤트를 구독하며 Tick에서 시스템 상태를 반복 조회하지 않는가?

이 체크리스트를 만족하는 콘텐츠 BP만 Foundation 호환 콘텐츠로 취급한다.
