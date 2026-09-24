# Foundation Blueprint 노드 목록과 테스트 콘텐츠 제작 안내

한국어/영어 스탯 이름, Details 필드의 용도, 전체 노드 입력·출력을 한 화면에서 검색하려면 [Foundation 한·영 HTML 레퍼런스](FOUNDATION_REFERENCE_KO_EN.html)를 사용한다. 이 HTML은 Scripts/generate_foundation_html_reference.py로 현재 목록과 C++ 선언에서 생성한다.

작성 기준: 2026-09-24, 현재 `Source`의 실제 선언과 구현.

## 1. 검토 범위와 현재 상태

`Source`의 C++ 헤더 70개, 구현 파일 60개, 빌드/타깃 C# 파일 3개를 읽었다. Foundation 런타임, 자동화 테스트, 기본 TopDown 및 Strategy/TwinStick 템플릿을 포함한다. 기존 Foundation 문서 4개와 코드를 대조했다. 최초 목록은 정적 코드 검토로 작성했으며, 이후 2026-09-24 보완 내용을 반영했다. 보완 후 빌드·자동화 테스트 결과는 handoff의 최신 기록을 따른다. `.uasset` 내부 Blueprint 그래프와 엔진이 제공하는 모든 상속 노드를 읽었다는 의미는 아니다.

현재 프로젝트는 Paper2D 기반 탑다운 액션 로그라이크를 만들기 위한 C++ Foundation이다. 전투 계산, 자원, 스탯, 행동 수명, 상태이상, 장비/유물/소모품, 획득/교체/진화, 상호작용, UI 데이터 전달이 구현되어 있다. 개별 공격의 연출·판정 배치, 적 AI, 맵, 실제 위젯, 아이템 효과는 Blueprint 콘텐츠로 제작해야 한다. `Content/Game`은 검토 시점에 안내 문서만 있으며 실제 게임용 에셋 제작이 남아 있다.

## 2. 테스트 콘텐츠별 사용할 기반

| 만들 대상 | 사용할 클래스/데이터 | 콘텐츠에서 구현할 부분 |
|---|---|---|
| 테스트 게임모드 | `AARGameMode` 파생 BP | 플레이어·컨트롤러 클래스 및 테스트 맵의 GameMode 지정 |
| 플레이어 | `AARPlayerCharacter`, `AARPlayerController` 파생 BP | 스프라이트/플립북, 입력 액션·매핑, 충돌, 기본 스탯, HUD 연결 |
| 적/허수아비 | `AARBaseEnemy` 파생 BP | 외형, 추적/공격 AI, 공격 타이밍, 사망 연출·제거·드롭 |
| 환경 공격원/함정 | Actor + `UARCombatSourceComponent` | 작동 주기·범위, 대미지 요청 구성, 충돌 처리 |
| 공격 판정 | `AARActionHitboxActor` 파생 BP | Box/Sphere 등 충돌 컴포넌트, 오버랩 처리, 피해/경직 요청 |
| 무기 | `UARWeaponDefinition` 데이터 에셋 + `UARLoadoutItemInstance` 파생 BP | 기본 보정, 스킬 정의, 스킬 실행 그래프, 진화 후보 |
| 액티브 유물 | `UARActiveRelicDefinition` + 아이템 인스턴스 BP | 스킬 실행, 비용·쿨다운, 효과·UI 상태 |
| 패시브 유물 | `UARPassiveRelicDefinition` + 필요시 아이템 인스턴스 BP | 고정 스탯 또는 전투 이벤트 구독, 조건부 효과, 해제 시 정리 |
| 소모품 | `UARConsumableDefinition` + `UARConsumableInstance` 파생 BP | 사용 가능 판정과 실제 회복/버프 효과 |
| 상태이상 | `UARStatusEffectDefinition` | 태그, 지속시간·중첩/갱신 정책과 스탯 효과 등 데이터 설정 |
| 필드 아이템 | `AARLoadoutItemPickup`, `AARConsumablePickup` 파생 BP | 표시·충돌·Definition 지정, 할당 이벤트에 따른 외형 갱신 |
| 문/상자/제단 | `IARInteractableInterface`를 구현한 BP | 상호작용 가능 여부·안내 문구·우선순위·실행 결과 |
| HUD/인벤토리/선택창 | `UARUIManagerComponent` + 직접 만드는 Widget BP | 화면 요청 이벤트 수신, Widget 생성/제거, 선택 확정·취소 |
| 테스트 맵 | 새 Level | 바닥·벽·충돌, PlayerStart, 적/함정/픽업 배치. 맵 생성·런 진행 시스템은 별도 제작 |

무기/액티브/패시브의 실제 동작은 공통 아이템 인스턴스를 사용한다. 별도의 “무기 실행 컴포넌트”를 새로 붙이는 방식이 아니다. 무기는 1개, 액티브 유물은 기본 2개이며 `MaxActiveRelics`로 조정 가능하다. 패시브 유물에는 현재 별도 개수 제한이 없다.

## 3. 노드 종류 읽는 방법

- **호출**: BlueprintCallable로 노출한 일반 함수. const 함수는 엔진 처리에 따라 순수 조회 노드로 표시될 수도 있다.
- **조회**: `BlueprintPure` 함수. 값을 읽고 판단할 때 사용한다.
- **구현 이벤트**: 부모 클래스의 Overrides/Event에서 BP 구현을 작성한다. 반환값이 있는 NativeEvent는 함수 형태로 보일 수 있다.
- **이벤트 디스패처**: `Bind Event`/`Assign` 등으로 연결하여 발생을 받는다. 함수 호출 노드와 구분한다.
- **비동기**: `Action Delay`. 완료/취소 출력으로 다음 흐름을 연결한다.
- **데이터 필드**: BlueprintReadOnly는 읽기, BlueprintReadWrite는 읽기/쓰기 노출이다. EditDefaultsOnly/EditAnywhere는 에디터 설정 범위를 뜻한다.

아래 목록의 함수명은 C++ 식별자이다. 에디터는 보통 단어 사이를 띄워 표시한다. 명시된 `DisplayName`은 함께 기록했다. 같은 이름이 라이브러리와 컴포넌트에 모두 있으면 **대상 타입이 다른 별도 선언**이다. `AR|...`는 에디터 카테고리이며 대부분 Context Sensitive 상태에서는 올바른 컴포넌트/인스턴스 참조에서 끌어 검색해야 한다.

시그니처에서 `const ...&`는 대개 입력, 비-const 참조는 출력 핀이다. UObject 포인터의 반환은 객체 참조다. WorldContext 메타데이터는 자동으로 채워지는 문맥 입력일 수 있다. 정확한 핀 표시와 기본 숨김 여부는 에디터에서 최종 확인한다.

## 4. 기능별 사용 요약

| 종류 | 주로 사용할 노드 | 역할 |
|---|---|---|
| 피해 | `CanDamageTarget`, `ApplyCombatDamage` | 공격 가능 팀/대상 확인, 회피·치명타·방어·보호막·체력 처리 |
| 지속 피해 | `ApplyDamageOverTime`, `RemoveDamageOverTime` | DOT 등록·갱신·제거. Source별/대상 전체 제거는 CombatSubsystem에서 제공 |
| 자원 | `RestoreHealth/Mana/Stamina`, `CanAffordResources`, `TryConsumeResources` | 회복, 비용 사전 확인 및 지불 |
| 보호막 | `ApplyShield`, `RemoveShield`, `RemoveShieldsBySource` | 보호막 추가와 특정 핸들/출처 제거 |
| 스탯 | `ApplyStatModifier`, `RemoveStatModifier`, `GetFinalStat`, `GetStatBreakdown` | 보정 적용·제거, 최종값과 계산 내역 조회 |
| 행동 | `TryStartAction`, `EndAction`, `CancelAction`, `Action Delay` | 행동 시작/종료/취소, 취소 가능한 시간 대기 |
| 행동 소유 효과 | `ApplyActionStatModifier`, `ApplyActionSuperArmor`, `RegisterActionHitbox` | 행동 종료 시 같이 정리할 효과·판정 등록 |
| 이동 | `RequestBasicMove`, `RequestActionMove`, `RequestActionVelocity` | 일반 이동과 행동 중 이동 요청 |
| 적 AI 경로 이동 | `ARMoveToActor`, `ARMoveToLocation`, `CanRequestBasicMove` | NavMesh 목표 요청과 CC 이동 잠금 검사. Behavior Tree `Move To`도 같은 Controller 검사를 거침 |
| 이동 제한 | `AcquireMovementLock`, `ReleaseMovementLock`, `CanBasicMove`, `CanMoveAtAll` | 기본 이동/전체 이동 잠금 관리 |
| 경직·그로기 | `ApplyStaggerAndGroggyDamage`, `AddSuperArmor`, `ResetGroggyGauge` | 피해와 별개의 경직·그로기 처리 |
| 상태이상 | `ApplyStatusEffect`, `RemoveStatusEffect`, `HasStatus`, `GetActiveStatusEffects` | 상태 등록·해제·조회 및 이동/행동 차단 상태 조회 |
| 장비 획득 | `BeginLoadoutAcquisition`, `CommitLoadoutAcquisition`, `CancelLoadoutAcquisition` | 획득 요청과 필요한 교체 확인, 확정·취소 |
| 장비 버리기 | `DiscardLoadoutItem` | 해제와 월드 픽업 드롭 처리 |
| 스킬 | `HandleSkillInput`, `GetRegisteredSkillUIData`, `GetSkillCooldownState` | 입력 태그에 해당하는 스킬 그룹 실행, 비용/쿨다운 관리 |
| 쿨다운 조작 | `ResetSkillCooldown`, `ModifySkillCooldown` | 특정 등록 스킬의 쿨다운 초기화·시간 변경 |
| 무기 진화 | `RequestWeaponEvolution`, `CommitWeaponEvolution`, `CancelWeaponEvolution` | 후보 조회 및 자동/선택 진화 처리 |
| 아이템 효과 | `ApplyItemStatModifier`, `RemoveOwnItemModifier`, `RemoveAllOwnItemModifiers` | 아이템 해제에 연동되는 소유 보정 관리 |
| 아이템 UI | `SetItemUIState`, `RemoveItemUIState`, `GetItemUIStates` | 스택·충전량 같은 표시용 상태 전달 |
| 소모품 | `TryAcquireConsumable`, `TryUseConsumableSlot`, `DropConsumableSlot` | 슬롯 획득·사용·드롭 |
| 슬롯 확장 | `SetBaseMaxConsumableSlots`, `GetMaxConsumableSlots`, `RetryPendingOverflowDrops` | 소모품 용량 변경과 넘친 아이템 드롭 재시도 |
| 상호작용 | `RefreshInteractionCandidate`, `TryInteract`, `GetCurrentPrompt` | 근처 대상 선정, 안내, 상호작용 실행 |
| 월드 드롭 | `TrySpawnLoadoutPickup`, `TrySpawnConsumablePickup` | Definition을 지정한 픽업 생성 |
| UI | `OpenScreen`, `CloseCurrentScreen`, `RequestBack`, `GetHUDSnapshot` | 화면 상태/입력 모드와 표시용 데이터 전달 |
| 조준/카메라 | `GetAimDirection`, `GetAimWorldLocation`, `ProjectMouseToGameplayPlane`, `SnapToTarget` | 조준 방향·위치 조회, 커서 투영, 카메라 즉시 추적 |

## 5. 첫 테스트 콘텐츠 연결 예시

### 무기/액티브 스킬

1. Definition의 `SkillDefinitions`에 InputTag, SkillId, 자원 비용, 쿨다운과 행동 설정을 넣는다.
2. RuntimeBehaviorClass에 아이템 인스턴스 BP를 지정한다.
3. `ExecuteItemSkill(SkillId, ActionHandle)`에서 전달받은 핸들로 효과를 실행한다. **Loadout이 이미 행동을 시작했으므로 같은 스킬에서 TryStartAction을 다시 호출하지 않는다.**
4. 준비 시간은 `Action Delay`로 기다린다. 완료 후 판정을 만들고, 취소 출력에서는 후속 공격을 실행하지 않는다.
5. 히트박스 초기화 → 이벤트 연결 → 충돌 활성화 순으로 준비한다. 오버랩 대상은 `CanDamageTarget` 확인 후 `TryAcceptTarget`으로 중복을 거른다.
6. 수락된 대상에 `ApplyCombatDamage`, 필요하면 별도로 `ApplyStaggerAndGroggyDamage`를 호출한다. 피해 계산 호출만으로 경직 요청까지 자동 적용되는 것으로 가정하지 않는다.
7. 공격·후딜이 끝나면 `EndAction`한다. 실행 이벤트를 비워 두거나 종료하지 않으면 활성 행동이 남을 수 있다.

### 패시브 유물

고정 스탯은 Definition의 `DefaultStatModifiers`로 설정한다. 조건부 패시브는 `On Item Registered`에서 필요한 전투 이벤트를 구독하고, `On Item Unregistered`에서 구독·타이머·직접 생성한 객체를 정리한다. `CombatSubsystem`의 전투 이벤트는 월드 공용이므로 `Subject`가 `GetItemOwner()`인지 등 원하는 출처/대상을 확인해야 한다.

아이템 수명에 맞춰 없어져야 하는 보정은 `ApplyItemStatModifier`를 사용한다. 일반 라이브러리의 `ApplyStatModifier`로 추가한 보정이 자동으로 이 아이템 소유가 되는 것은 아니다.

### 소모품

`CanUseConsumable`에서 체력 최대 여부 등 실패 조건과 FailureTag를 판단한다. `ExecuteConsumableUse`에서 `GetConsumableOwner`에 회복/버프를 적용한다. 실행 이벤트는 반환값이 없는 void다. 실행 후 슬롯에서 제거되므로 실행 단계에서 실패 결과를 돌려 소비를 취소하는 구조는 현재 없다. 슬롯 인덱스는 0부터 시작한다.

### 환경 함정

환경 공격원 Actor에 CombatSourceComponent를 붙이고 피해 요청의 공격원 정보를 구성한다. 이 컴포넌트는 Environment 팀 정보를 제공한다. 범위 충돌, 타이머, 경고 연출은 함정 BP가 담당한다. 행동 핸들이 없는 환경 공격을 억지로 ActionHitbox에 맞추기보다는 자체 충돌에서 전투 라이브러리를 호출할 수 있다.

### UI/획득 선택

UIManager는 Widget을 생성하지 않는다. 화면 열기/닫기 요청을 받아 Widget을 생성·제거하고, 교체/진화 확정은 받은 토큰으로 Commit을 호출한다. 취소 시 해당 Cancel도 연결한다. `RequestBack`은 뒤로가기 이벤트만 발생시키므로 위젯 쪽에서 취소/닫기 흐름을 완성한다.

## 6. 테스트 전에 알아둘 실제 구현 차이

아래는 보완 후의 현재 동작과 콘텐츠 연결 조건이다. 자동화 테스트와 실제 맵에서의 플레이 검증은 구분한다.

| 항목 | 실제 구현과 테스트 영향 |
|---|---|
| 이동속도 스탯 | 수정 완료. BeginPlay의 최종 MoveSpeed와 이후 스탯 변경을 실제 CharacterMovement 속도에 반영한다. 버프 해제도 반영된다. |
| 행동 시작 차단 | 수정 완료. 사망 시 Dead, 경직/진행 중 구르기 시 Blocked를 반환하며 새 행동을 시작하지 않는다. 이미 실행 중인 스킬과 이후 시작하는 구르기의 병행은 기존 취소 규칙을 따른다. |
| 서로 다른 입력 그룹 | 수정 완료. 기존 그룹의 참여 행동이 모두 끝나기 전에는 다른 입력 태그의 새 그룹도 거부한다. 한 입력으로 시작하는 여러 참여 스킬은 계속 함께 실행된다. |
| 인벤토리에서 소모품 사용 | 수정 완료. 인벤토리 Widget은 TryUseConsumableSlot을 직접 호출할 수 있다. 소모품 단축키는 UI가 열려 있는 동안 차단된다. 다른 화면/수동 입력 잠금/사망 시에는 사용하지 않는다. |
| 입력 잠금 | SetGameplayInputBlocked는 수동 잠금이며 화면 점유에 의한 차단과 별개다. IsGameplayInputBlocked는 둘을 합친 상태다. UI를 닫아도 수동 잠금은 유지된다. |
| 기본 이동 취소 | 이동 잠금으로 RequestBasicMove가 거부되면 BasicMovementInput 사유로 행동을 취소하지 않는다. |
| 히트박스 | 부모에는 충돌 도형이 없고 Actor 충돌도 꺼져 있다. InitializeHitbox가 자동 활성화하지 않는다. BP에서 도형/오버랩/활성화를 구성해야 한다. |
| 히트박스 수락 | TryAcceptTarget은 초기화·자기 자신·중복 등을 검사하지만 적대 관계 검사는 하지 않는다. CanDamageTarget을 별도로 사용한다. 디스패처와 구현 이벤트 양쪽에서 대미지를 중복 적용하지 않는다. |
| 히트박스 소유 | InitializeHitbox에서 행동에 등록된다. 같은 판정을 다시 RegisterActionHitbox할 필요가 없다. |
| 행동 정리 | 행동에 등록한 보정·슈퍼아머·이동 잠금·히트박스는 정리된다. 직접 만든 타이머/객체/이벤트 구독과 임의로 획득한 잠금은 별도 관리한다. 행동 속도도 일반 CleanupAction이 무조건 0으로 만들지는 않는다. |
| Action Delay | 정상 EndAction도 대기 중인 Delay에는 Cancelled로 전달된다. Duration이 0 이하이면 유효한 행동에서 즉시 완료된다. |
| 사망 이벤트 순서 | Health에서 OnDeath가 먼저 발생하고 이후 CombatSubsystem의 OnDamageReceived/OnDamageHit 등이 진행된다. 처치 패시브와 사망 시 정리를 설계할 때 순서에 유의한다. |
| 사망 처리 | 이동 중지·행동/상태 정리와 사망 이벤트까지 제공한다. 시체 제거·리스폰·런 종료는 콘텐츠 작업이다. |
| DOT | 회피와 OnHit 발동 경로를 사용하지 않는다. 같은 DOT 갱신 시 다음 틱 시각도 재설정되므로 틱보다 빠른 반복 갱신에서는 피해가 계속 미뤄질 수 있다. |
| DOT 출처 소멸 | 공격원이 없어지면 보관된 스냅샷을 사용한다. 이는 등록/최근 처리 시점 값이며 소멸 순간의 스탯을 반드시 새로 캡처한 값은 아니다. |
| 스탯 수치 | Multiplicative는 배율 값이다. 20% 증가를 1.2로 넣는다. 보정 Duration은 음수=영구, 양수=시간제이며 0은 거부된다. |
| 기본 공격력 | 기본 AttackPower/SpellPower와 ManaRecoveryPerSecond는 0이다. 공격력 계수만 있는 무기를 시험하려면 테스트용 BaseStats를 먼저 설정한다. |
| 스탯 설정 노드 | SetBaseStat은 C++ 전용이며 Blueprint 호출 노드가 아니다. 기본값은 에디터 BaseStats에서 설정하고 런타임 BP 효과에는 보정을 사용한다. |
| 아이템 등록 이벤트 | On Item Registered는 스킬 등록과 최종 슬롯 반영보다 먼저 호출된다. 최종 장비/등록 스킬 목록 갱신에는 OnLoadoutChanged/OnRegisteredSkillsChanged를 사용한다. |
| 진화 | 후보 필드는 NextEvolutionCandidates, 단계 필드는 EvolutionStage다. CommitWeaponEvolution에는 PrimaryAssetId가 아닌 실제 Candidate Definition 참조를 전달한다. 후보가 하나면 Request 단계에서 자동 진화할 수 있으므로 결과 상태를 분기한다. |
| 드롭 | OnConsumableDropRequested는 초과 슬롯 아이템의 실제 생성 뒤 알림이다. 이름만 보고 다시 Spawn하면 중복 드롭이 된다. 일반 버리기 결과에도 생성된 픽업 참조가 들어 있다. |
| 픽업 초기화 | 런타임 드롭의 Definition 할당 이벤트는 deferred spawn의 FinishSpawning 이전에 호출된다. 그 시점에 Construction Script가 만든 표시 컴포넌트가 준비되었다고 가정하지 않는다. |
| 입력 유지 | 스킬 입력은 Started에 연결되어 있다. 버튼을 누르고 있는 동안 반복 공격하는 동작은 별도 구현이 필요하다. |
| UI 알림 | 독립적인 OnShieldBroken이나 쿨다운 자연 만료 이벤트는 없다. 실제 제공되는 변경 이벤트/시간 조회를 조합한다. HUD 스냅샷 알림도 조준/자원 변화로 자주 올 수 있다. |
| 템플릿 재사용 | 기존 TwinStick 피해/점수/스폰 및 Strategy 선택/이동은 Foundation 전투와 별도 체계다. 그대로 붙여도 새 스탯·피해·아이템 효과로 자동 연결되지 않는다. |

테스트 제작 순서는 플레이어+허수아비+근접 무기 → 회복 소모품 → 스탯 패시브 → 액티브 공격 → 환경 DOT/상태이상 → 획득·교체·진화·UI 순이 적당하다. 먼저 작은 방 하나에서 피해·비용·쿨다운·취소·드롭이 일관되게 연결되는지 확인하면 콘텐츠 수를 늘릴 때 원인을 찾기 쉽다.

## 7. 전체 선언 목록

다음 목록은 헤더에서 Blueprint 노출 선언을 추출한 전수 목록이다. 카테고리별 함수와 이벤트, BP 데이터 필드, BlueprintType enum/struct를 구분한다. 함수 선언 수는 고유 기능 수나 에디터에서 생성되는 모든 노드 변형 수와 같지 않다. 구조체 Make/Break, 속성 Get/Set, 디스패처 Bind/Unbind 등의 파생 노드는 개별 함수 수에 넣지 않는다.

전수 집계: Foundation 함수/구현 이벤트 **231개**, 기존 템플릿 함수/구현 이벤트 **21개**, 이벤트 디스패처 선언 **41개**, BP 노출 데이터 필드 **360개**, BlueprintType 구조체 **50개**, 열거형 **23개**.

### 7.1 Foundation 함수 노드와 구현 이벤트

#### AR|Action

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARAsyncActionDelay](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Actions/ARAsyncActionDelay.h:18>) | `ActionDelay` / Action Delay | 비동기 | `static UARAsyncActionDelay* ActionDelay(UObject* WorldContextObject, UARActionComponent* ActionComponent, FARActionHandle ActionHandle, float Duration)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:40>) | `CanStartAction` | 조회 | `FARRequestStatus CanStartAction(const FARActionRequest& Request) const` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:43>) | `TryStartAction` | 호출 | `FARActionHandle TryStartAction(const FARActionRequest& Request, FARRequestStatus& Status)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:46>) | `EndAction` | 호출 | `bool EndAction(FARActionHandle Handle)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:47>) | `CancelAction` | 호출 | `bool CancelAction(FARActionHandle Handle, EARActionCancelReason Reason)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:48>) | `CancelActionsByReason` | 호출 | `int32 CancelActionsByReason(EARActionCancelReason Reason)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:49>) | `CancelRollActions` | 호출 | `int32 CancelRollActions(EARActionCancelReason Reason = EARActionCancelReason::Root)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:50>) | `CancelAllActions` | 호출 | `int32 CancelAllActions(EARActionCancelReason Reason = EARActionCancelReason::Manual)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:51>) | `CancelActionsByItemInstance` | 호출 | `int32 CancelActionsByItemInstance(FGuid ItemInstanceId, EARActionCancelReason Reason = EARActionCancelReason::ItemRemoved)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:53>) | `SetActionCancelRules` | 호출 | `bool SetActionCancelRules(FARActionHandle Handle, const FARActionCancelRules& Rules)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:54>) | `SetActionRollBlocked` | 호출 | `bool SetActionRollBlocked(FARActionHandle Handle, bool bBlocked)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:55>) | `SetActionBasicMovementBlocked` | 호출 | `bool SetActionBasicMovementBlocked(FARActionHandle Handle, bool bBlocked)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:56>) | `RegisterActionHitbox` | 호출 | `bool RegisterActionHitbox(FARActionHandle Handle, AActor* HitboxActor)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:57>) | `ApplyActionStatModifier` | 호출 | `FARStatModifierHandle ApplyActionStatModifier(FARActionHandle Handle, const FARStatModifierSpec& Spec, bool& bSuccess)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:58>) | `ApplyActionSuperArmor` | 호출 | `FARSuperArmorHandle ApplyActionSuperArmor(FARActionHandle Handle, const FARSuperArmorSpec& Spec, bool& bSuccess)` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:60>) | `IsActionActive` | 조회 | `bool IsActionActive(FARActionHandle Handle) const` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:61>) | `IsRollBlocked` | 조회 | `bool IsRollBlocked() const` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:62>) | `IsBasicMovementBlocked` | 조회 | `bool IsBasicMovementBlocked() const` |
| [UARActionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h:63>) | `GetActiveActionCount` | 조회 | `int32 GetActiveActionCount() const` |

#### AR|Camera

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARCameraFollowComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARCameraFollowComponent.h:22>) | `SnapToTarget` | 호출 | `void SnapToTarget()` |

#### AR|Character

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:34>) | `GetAimDirection` | 조회 | `FVector GetAimDirection() const` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:35>) | `SetAimDirection` | 호출 | `void SetAimDirection(FVector NewAimDirection)` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:37>) | `GetStatsComponent` | 조회 | `UARStatsComponent* GetStatsComponent() const` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:38>) | `GetHealthComponent` | 조회 | `UARHealthComponent* GetHealthComponent() const` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:39>) | `GetStatusEffectComponent` | 조회 | `UARStatusEffectComponent* GetStatusEffectComponent() const` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:40>) | `GetStaggerComponent` | 조회 | `UARStaggerComponent* GetStaggerComponent() const` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:41>) | `GetActionComponent` | 조회 | `UARActionComponent* GetActionComponent() const` |
| [AARBaseCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:42>) | `GetMovementControlComponent` | 조회 | `UARMovementControlComponent* GetMovementControlComponent() const` |

#### AR|Combat

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:17>) | `CanDamageTarget` | 조회 | `static bool CanDamageTarget(const UObject* WorldContextObject, AActor* Attacker, AActor* Target, EARRequestResult& FailureReason)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:20>) | `ApplyCombatDamage` | 호출 | `static FARCombatDamageResult ApplyCombatDamage(const UObject* WorldContextObject, const FARCombatDamageRequest& Request)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:23>) | `ApplyDamageOverTime` | 호출 | `static FARDotHandle ApplyDamageOverTime(const UObject* WorldContextObject, const FARDamageOverTimeSpec& Spec, bool& bSuccess, EARRequestResult& FailureReason)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:26>) | `RemoveDamageOverTime` | 호출 | `static bool RemoveDamageOverTime(const UObject* WorldContextObject, FARDotHandle Handle)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:29>) | `ApplyStaggerAndGroggyDamage` | 호출 | `static FARStaggerResult ApplyStaggerAndGroggyDamage(const FARStaggerRequest& Request)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:32>) | `ApplySuperArmor` | 호출 | `static FARSuperArmorHandle ApplySuperArmor(AActor* Target, const FARSuperArmorSpec& Spec, bool& bSuccess)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:35>) | `RemoveSuperArmor` | 호출 | `static bool RemoveSuperArmor(AActor* Target, FARSuperArmorHandle Handle)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:38>) | `ApplyStatusEffect` | 호출 | `static FARStatusEffectResult ApplyStatusEffect(AActor* Target, const FARStatusEffectRequest& Request)` |
| [UARCombatBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h:41>) | `RemoveStatusEffect` | 호출 | `static bool RemoveStatusEffect(AActor* Target, FARStatusEffectHandle Handle)` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:32>) | `CanDamageTarget` | 조회 | `bool CanDamageTarget(AActor* Attacker, AActor* Target, EARRequestResult& FailureReason) const` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:35>) | `ApplyCombatDamage` | 호출 | `FARCombatDamageResult ApplyCombatDamage(const FARCombatDamageRequest& Request)` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:38>) | `ApplyDamageOverTime` | 호출 | `FARDotHandle ApplyDamageOverTime(const FARDamageOverTimeSpec& Spec, bool& bSuccess, EARRequestResult& FailureReason)` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:41>) | `RemoveDamageOverTime` | 호출 | `bool RemoveDamageOverTime(FARDotHandle Handle)` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:44>) | `RemoveDamageOverTimeBySource` | 호출 | `int32 RemoveDamageOverTimeBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId, FName DotName = NAME_None)` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:47>) | `RemoveAllDamageOverTimeFromTarget` | 호출 | `int32 RemoveAllDamageOverTimeFromTarget(AActor* Target)` |
| [UARCombatSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARCombatSubsystem.h:50>) | `GetActiveDamageOverTimeCount` | 조회 | `int32 GetActiveDamageOverTimeCount(AActor* Target) const` |
| [UARCombatSourceComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARCombatSourceComponent.h:15>) | `GetCombatTeam` | 조회 | `EARCombatTeam GetCombatTeam() const` |
| [UARCombatSourceComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARCombatSourceComponent.h:18>) | `GetSourceInfo` | 조회 | `const FARSourceInfo& GetSourceInfo() const` |
| [IARCombatTargetInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARCombatTargetInterface.h:20>) | `GetCombatTeam` | 호출 | `virtual EARCombatTeam GetCombatTeam() const = 0` |
| [IARCombatTargetInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARCombatTargetInterface.h:23>) | `CanBeCombatTarget` | 호출 | `virtual bool CanBeCombatTarget() const = 0` |

#### AR|Consumable

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:28>) | `TryAcquireConsumable` | 호출 | `FARConsumableAcquisitionResult TryAcquireConsumable(UARConsumableDefinition* Definition)` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:31>) | `TryUseConsumableSlot` | 호출 | `FARRequestStatus TryUseConsumableSlot(int32 SlotIndex, UARConsumableDefinition*& UsedDefinition)` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:34>) | `DropConsumableSlot` | 호출 | `FARRequestStatus DropConsumableSlot(int32 SlotIndex, FARConsumableDropRequest& DropRequest)` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:37>) | `SetBaseMaxConsumableSlots` | 호출 | `void SetBaseMaxConsumableSlots(int32 NewSlotCount)` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:40>) | `RetryPendingOverflowDrops` | 호출 | `void RetryPendingOverflowDrops()` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:43>) | `GetConsumableSlots` | 조회 | `TArray<FARConsumableSlotSnapshot> GetConsumableSlots() const` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:52>) | `GetMaxConsumableSlots` | 조회 | `int32 GetMaxConsumableSlots() const` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:24>) | `ReceiveConsumableRegistered` / On Consumable Registered | 구현 이벤트 | `void ReceiveConsumableRegistered()` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:27>) | `ReceiveConsumableUnregistered` / On Consumable Unregistered | 구현 이벤트 | `void ReceiveConsumableUnregistered(EARConsumableRemovalReason Reason)` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:30>) | `CanUseConsumable` | 구현 이벤트(기본 구현 있음)/호출 가능 | `bool CanUseConsumable(FGameplayTag& FailureTag) const` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:33>) | `ExecuteConsumableUse` | 구현 이벤트 | `void ExecuteConsumableUse()` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:36>) | `GetConsumableOwner` | 조회 | `AARPlayerCharacter* GetConsumableOwner() const` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:37>) | `GetConsumableDefinition` | 조회 | `UARConsumableDefinition* GetConsumableDefinition() const` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:38>) | `GetInstanceId` | 조회 | `FGuid GetInstanceId() const` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:39>) | `GetSlotIndex` | 조회 | `int32 GetSlotIndex() const` |
| [UARConsumableInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableInstance.h:40>) | `IsRegistered` | 조회 | `bool IsRegistered() const` |

#### AR|Consumable|UI

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:46>) | `GetConsumableSlotDisplayData` | 조회 | `bool GetConsumableSlotDisplayData(int32 SlotIndex, FARConsumableDisplayData& DisplayData) const` |
| [UARConsumableComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:49>) | `GetConsumableDefinitionDisplayData` | 조회 | `bool GetConsumableDefinitionDisplayData(const UARConsumableDefinition* Definition, FARConsumableDisplayData& DisplayData) const` |

#### AR|Health

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:57>) | `CanAcceptResolvedDamage` | 조회 | `bool CanAcceptResolvedDamage() const` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:66>) | `RestoreHealth` | 호출 | `float RestoreHealth(float Amount, const FARSourceInfo& Source)` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:69>) | `ApplyShield` | 호출 | `FARShieldHandle ApplyShield(const FARShieldSpec& Spec, bool& bSuccess)` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:72>) | `RemoveShield` | 호출 | `bool RemoveShield(FARShieldHandle Handle, float& RemovedAmount)` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:75>) | `RemoveShieldsBySource` | 호출 | `int32 RemoveShieldsBySource(EARModifierSourceCategory Category, FName SourceId, EARShieldLifetimeFilter LifetimeFilter, float& RemovedAmount)` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:78>) | `ClearShields` | 호출 | `int32 ClearShields(float& RemovedAmount)` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:81>) | `GetCurrentHealth` | 조회 | `float GetCurrentHealth() const` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:84>) | `GetMaxHealth` | 조회 | `float GetMaxHealth() const` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:87>) | `GetHealthRatio` | 조회 | `float GetHealthRatio() const` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:90>) | `GetCurrentShield` | 조회 | `float GetCurrentShield() const` |
| [UARHealthComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:93>) | `IsDead` | 조회 | `bool IsDead() const` |

#### AR|Hitbox

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:25>) | `InitializeHitbox` | 호출 | `bool InitializeHitbox(FARActionHandle InActionHandle, AActor* InCombatSource, EARHitboxHitPolicy InHitPolicy, float Lifetime)` |
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:28>) | `TryAcceptTarget` | 호출 | `bool TryAcceptTarget(AActor* Candidate)` |
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:31>) | `ResetAcceptedTargets` | 호출 | `void ResetAcceptedTargets()` |
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:34>) | `GetActionHandle` | 조회 | `FARActionHandle GetActionHandle() const` |
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:35>) | `GetCombatSource` | 조회 | `AActor* GetCombatSource() const` |
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:36>) | `HasAcceptedTarget` | 조회 | `bool HasAcceptedTarget(AActor* Target) const` |
| [AARActionHitboxActor](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:41>) | `ReceiveTargetAccepted` / On Hitbox Target Accepted | 구현 이벤트 | `void ReceiveTargetAccepted(AActor* Target)` |

#### AR|Input

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARPlayerController](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Player/ARPlayerController.h:18>) | `ProjectMouseToGameplayPlane` | 조회 | `bool ProjectMouseToGameplayPlane(float PlaneZ, FVector& WorldPoint) const` |
| [AARPlayerController](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Player/ARPlayerController.h:21>) | `SetGameplayUIInputMode` | 호출 | `void SetGameplayUIInputMode(bool bUIOpen)` |

#### AR|Interaction

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARInteractionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARInteractionComponent.h:24>) | `TryInteract` | 호출 | `FARRequestStatus TryInteract()` |
| [UARInteractionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARInteractionComponent.h:27>) | `RefreshInteractionCandidate` | 호출 | `void RefreshInteractionCandidate()` |
| [UARInteractionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARInteractionComponent.h:30>) | `GetCurrentCandidate` | 조회 | `AActor* GetCurrentCandidate() const` |
| [UARInteractionComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARInteractionComponent.h:31>) | `GetCurrentPrompt` | 조회 | `FText GetCurrentPrompt() const` |
| [IARInteractableInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARInteractableInterface.h:21>) | `CanInteract` | 구현 이벤트(기본 구현 있음)/호출 가능 | `bool CanInteract(AARPlayerCharacter* Interactor, FText& FailureReason) const` |
| [IARInteractableInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARInteractableInterface.h:24>) | `GetInteractionPrompt` | 구현 이벤트(기본 구현 있음)/호출 가능 | `FText GetInteractionPrompt(AARPlayerCharacter* Interactor) const` |
| [IARInteractableInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARInteractableInterface.h:27>) | `GetInteractionPriority` | 구현 이벤트(기본 구현 있음)/호출 가능 | `int32 GetInteractionPriority(AARPlayerCharacter* Interactor) const` |
| [IARInteractableInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARInteractableInterface.h:30>) | `RequiresInteractionLineOfSight` | 구현 이벤트(기본 구현 있음)/호출 가능 | `bool RequiresInteractionLineOfSight() const` |
| [IARInteractableInterface](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interfaces/ARInteractableInterface.h:33>) | `Interact` | 구현 이벤트(기본 구현 있음)/호출 가능 | `FARRequestStatus Interact(AARPlayerCharacter* Interactor)` |

#### AR|Item

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:26>) | `ReceiveItemRegistered` / On Item Registered | 구현 이벤트 | `void ReceiveItemRegistered()` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:29>) | `ReceiveItemUnregistered` / On Item Unregistered | 구현 이벤트 | `void ReceiveItemUnregistered(EARItemRemovalReason Reason)` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:32>) | `CanExecuteItemSkill` | 구현 이벤트(기본 구현 있음)/호출 가능 | `bool CanExecuteItemSkill(FName SkillId, FGameplayTag& FailureTag) const` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:35>) | `ExecuteItemSkill` | 구현 이벤트 | `void ExecuteItemSkill(FName SkillId, FARActionHandle ActionHandle)` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:38>) | `ApplyItemStatModifier` | 호출 | `FARStatModifierHandle ApplyItemStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess)` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:41>) | `RemoveOwnItemModifier` | 호출 | `bool RemoveOwnItemModifier(FARStatModifierHandle Handle)` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:44>) | `RemoveAllOwnItemModifiers` | 호출 | `int32 RemoveAllOwnItemModifiers()` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:47>) | `SetItemUIState` | 호출 | `bool SetItemUIState(FGameplayTag StateId, float CurrentValue, float MaximumValue)` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:50>) | `RemoveItemUIState` | 호출 | `bool RemoveItemUIState(FGameplayTag StateId)` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:53>) | `GetItemOwner` | 조회 | `AARPlayerCharacter* GetItemOwner() const` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:54>) | `GetItemDefinition` | 조회 | `const UARLoadoutItemDefinition* GetItemDefinition() const` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:55>) | `GetInstanceId` | 조회 | `FGuid GetInstanceId() const` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:56>) | `IsRegistered` | 조회 | `bool IsRegistered() const` |
| [UARLoadoutItemInstance](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h:57>) | `GetItemUIStates` | 조회 | `TArray<FARItemUIState> GetItemUIStates() const` |

#### AR|Loadout

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:64>) | `BeginLoadoutAcquisition` | 호출 | `FARLoadoutAcquisitionResult BeginLoadoutAcquisition(const UARLoadoutItemDefinition* Definition)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:67>) | `CommitLoadoutAcquisition` | 호출 | `UARLoadoutItemInstance* CommitLoadoutAcquisition(FARAcquisitionToken Token, FARRequestStatus& Status)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:70>) | `CancelLoadoutAcquisition` | 호출 | `bool CancelLoadoutAcquisition(FARAcquisitionToken Token)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:73>) | `CancelWeaponEvolution` | 호출 | `bool CancelWeaponEvolution(FAREvolutionToken Token)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:76>) | `CancelAllPendingRequests` | 호출 | `void CancelAllPendingRequests()` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:79>) | `DiscardLoadoutItem` | 호출 | `bool DiscardLoadoutItem(FGuid InstanceId, FARLoadoutDropRequest& DropRequest, FARRequestStatus& Status)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:82>) | `HandleSkillInput` | 호출 | `FARRequestStatus HandleSkillInput(FGameplayTag InputTag, FARSkillGroupHandle& GroupHandle)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:85>) | `GetRegisteredSkillUIData` | 조회 | `TArray<FARRegisteredSkillUIData> GetRegisteredSkillUIData() const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:88>) | `GetSkillCooldownState` | 조회 | `bool GetSkillCooldownState(FARRegisteredSkillHandle Handle, bool& bReady, float& Remaining, float& Total, float& Ratio) const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:91>) | `ResetSkillCooldown` | 호출 | `bool ResetSkillCooldown(FARRegisteredSkillHandle Handle)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:94>) | `ModifySkillCooldown` | 호출 | `bool ModifySkillCooldown(FARRegisteredSkillHandle Handle, float DeltaSeconds, float& NewRemaining)` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:97>) | `GetLoadoutInventory` | 조회 | `TArray<FARLoadoutItemSnapshot> GetLoadoutInventory() const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:106>) | `GetEquippedWeapon` | 조회 | `UARLoadoutItemInstance* GetEquippedWeapon() const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:109>) | `GetActiveRelics` | 조회 | `const TArray<UARLoadoutItemInstance*>& GetActiveRelics() const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:112>) | `GetPassiveRelics` | 조회 | `const TArray<UARLoadoutItemInstance*>& GetPassiveRelics() const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:115>) | `RequestWeaponEvolution` | 호출 | `FARWeaponEvolutionResult RequestWeaponEvolution()` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:118>) | `CommitWeaponEvolution` | 호출 | `UARLoadoutItemInstance* CommitWeaponEvolution(FAREvolutionToken Token, const UARWeaponDefinition* Candidate, FARRequestStatus& Status)` |

#### AR|Loadout|UI

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:100>) | `GetLoadoutItemDisplayData` | 조회 | `bool GetLoadoutItemDisplayData(FGuid InstanceId, FARLoadoutItemDisplayData& DisplayData) const` |
| [UARLoadoutComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:103>) | `GetLoadoutDefinitionDisplayData` | 조회 | `bool GetLoadoutDefinitionDisplayData(const UARLoadoutItemDefinition* Definition, FARLoadoutItemDisplayData& DisplayData) const` |

#### AR|Mana

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARManaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:24>) | `CanAfford` | 조회 | `bool CanAfford(float Amount) const` |
| [UARManaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:25>) | `TryConsume` | 호출 | `bool TryConsume(float Amount, const FARSourceInfo& Source, float& NewMana)` |
| [UARManaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:26>) | `Restore` | 호출 | `float Restore(float Amount, const FARSourceInfo& Source)` |
| [UARManaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:27>) | `GetCurrent` | 조회 | `float GetCurrent() const` |
| [UARManaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:28>) | `GetMax` | 조회 | `float GetMax() const` |
| [UARManaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:29>) | `GetRatio` | 조회 | `float GetRatio() const` |

#### AR|Movement

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARAIController](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/AI/ARAIController.h:22>) | `ARMoveToActor` / AR AI Move To Actor | 호출 | `EPathFollowingRequestResult::Type ARMoveToActor(AActor* Goal, float AcceptanceRadius = -1.0f)` |
| [AARAIController](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/AI/ARAIController.h:25>) | `ARMoveToLocation` / AR AI Move To Location | 호출 | `EPathFollowingRequestResult::Type ARMoveToLocation(FVector Destination, float AcceptanceRadius = -1.0f)` |
| [AARAIController](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/AI/ARAIController.h:28>) | `CanRequestBasicMove` | 조회 | `bool CanRequestBasicMove() const` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:34>) | `RequestBasicMove` | 호출 | `bool RequestBasicMove(FVector Direction, float Scale = 1.0f)` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:37>) | `RequestActionMove` | 호출 | `bool RequestActionMove(FARActionHandle ActionHandle, FVector Direction, float Scale = 1.0f)` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:40>) | `RequestActionVelocity` | 호출 | `bool RequestActionVelocity(FARActionHandle ActionHandle, FVector Direction, float Speed)` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:43>) | `AcquireMovementLock` | 호출 | `FARMovementLockHandle AcquireMovementLock(FName SourceId, EARMovementLockType LockType)` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:46>) | `ReleaseMovementLock` | 호출 | `bool ReleaseMovementLock(FARMovementLockHandle Handle)` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:49>) | `ReleaseMovementLocksBySource` | 호출 | `int32 ReleaseMovementLocksBySource(FName SourceId)` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:52>) | `CanBasicMove` | 조회 | `bool CanBasicMove() const` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:55>) | `CanMoveAtAll` | 조회 | `bool CanMoveAtAll() const` |
| [UARMovementControlComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h:58>) | `StopMovementImmediately` | 호출 | `void StopMovementImmediately()` |

#### AR|Pickup

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARLoadoutItemPickup](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARItemPickupActors.h:46>) | `ReceiveItemDefinitionAssigned` / On Pickup Definition Assigned | 구현 이벤트 | `void ReceiveItemDefinitionAssigned()` |
| [AARConsumablePickup](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARItemPickupActors.h:64>) | `ReceiveConsumableDefinitionAssigned` / On Pickup Definition Assigned | 구현 이벤트 | `void ReceiveConsumableDefinitionAssigned()` |
| [UARWorldItemDropSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARWorldItemDropSubsystem.h:18>) | `TrySpawnLoadoutPickup` | 호출 | `bool TrySpawnLoadoutPickup(const UARLoadoutItemDefinition* Definition, FVector DesiredLocation, TSubclassOf<AARLoadoutItemPickup> PickupClass, AActor* DropOwner, AARLoadoutItemPickup*& SpawnedPickup)` |
| [UARWorldItemDropSubsystem](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARWorldItemDropSubsystem.h:22>) | `TrySpawnConsumablePickup` | 호출 | `bool TrySpawnConsumablePickup(UARConsumableDefinition* Definition, FVector DesiredLocation, TSubclassOf<AARConsumablePickup> PickupClass, AActor* DropOwner, AARConsumablePickup*& SpawnedPickup)` |

#### AR|Player

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `SetGameplayInputBlocked` | 호출 | `void SetGameplayInputBlocked(bool bBlocked)` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `IsGameplayInputBlocked` | 조회 | `bool IsGameplayInputBlocked() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetStaminaComponent` | 조회 | `UARStaminaComponent* GetStaminaComponent() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetManaComponent` | 조회 | `UARManaComponent* GetManaComponent() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetLoadoutComponent` | 조회 | `UARLoadoutComponent* GetLoadoutComponent() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetConsumableComponent` | 조회 | `UARConsumableComponent* GetConsumableComponent() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h:52>) | `GetInteractionComponent` | 조회 | `UARInteractionComponent* GetInteractionComponent() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetUIManagerComponent` | 조회 | `UARUIManagerComponent* GetUIManagerComponent() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetTopDownCamera` | 조회 | `UCameraComponent* GetTopDownCamera() const` |

#### AR|Player|Aim

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetAimWorldLocation` | 조회 | `FVector GetAimWorldLocation() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `HasAimWorldLocation` | 조회 | `bool HasAimWorldLocation() const` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `SetAimWorldLocation` | 호출 | `bool SetAimWorldLocation(FVector WorldLocation)` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `ClearAimWorldLocation` | 호출 | `void ClearAimWorldLocation()` |

#### AR|Player|Damage

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>) | `GetPostHitInvulnerabilityDuration` | 조회 | `float GetPostHitInvulnerabilityDuration() const` |

#### AR|Resource

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:14>) | `RestoreHealth` | 호출 | `static float RestoreHealth(AActor* Target, float Amount, const FARSourceInfo& Source)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:15>) | `RestoreMana` | 호출 | `static float RestoreMana(AActor* Target, float Amount, const FARSourceInfo& Source)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:16>) | `RestoreStamina` | 호출 | `static float RestoreStamina(AActor* Target, float Amount, const FARSourceInfo& Source)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:17>) | `TryConsumeMana` | 호출 | `static bool TryConsumeMana(AActor* Target, float Amount, const FARSourceInfo& Source, float& NewMana)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:18>) | `TryConsumeStamina` | 호출 | `static bool TryConsumeStamina(AActor* Target, float Amount, const FARSourceInfo& Source, float& NewStamina)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:19>) | `CanAffordResources` | 조회 | `static bool CanAffordResources(AActor* Target, const FARResourceCost& Cost, EARResourceType& MissingResource)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:20>) | `TryConsumeResources` | 호출 | `static bool TryConsumeResources(AActor* Target, const FARResourceCost& Cost, const FARSourceInfo& Source, float& NewMana, float& NewStamina, EARResourceType& MissingResource)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:21>) | `ApplyShield` | 호출 | `static FARShieldHandle ApplyShield(AActor* Target, const FARShieldSpec& Spec, bool& bSuccess, float& NewTotalShield)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:22>) | `RemoveShield` | 호출 | `static bool RemoveShield(AActor* Target, FARShieldHandle Handle, float& RemovedAmount, float& NewTotalShield)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:23>) | `RemoveShieldsBySource` | 호출 | `static int32 RemoveShieldsBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARShieldLifetimeFilter LifetimeFilter, float& RemovedAmount, float& NewTotalShield)` |
| [UARResourceBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARResourceBlueprintLibrary.h:24>) | `GetCurrentResource` | 조회 | `static bool GetCurrentResource(AActor* Target, EARResourceType ResourceType, float& Current, float& Maximum, float& Ratio)` |

#### AR|Roll

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h:52>) | `TryStartRoll` | 호출 | `bool TryStartRoll(bool bForceMouseDirection = false)` |
| [AARPlayerCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h:53>) | `GetRollCooldownRemaining` | 조회 | `float GetRollCooldownRemaining() const` |

#### AR|Stagger

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:38>) | `ApplyStaggerAndGroggyDamage` | 호출 | `FARStaggerResult ApplyStaggerAndGroggyDamage(const FARStaggerRequest& Request)` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:41>) | `AddSuperArmor` | 호출 | `FARSuperArmorHandle AddSuperArmor(const FARSuperArmorSpec& Spec, bool& bSuccess)` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:44>) | `RemoveSuperArmor` | 호출 | `bool RemoveSuperArmor(FARSuperArmorHandle Handle)` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:47>) | `RemoveSuperArmorBySource` | 호출 | `int32 RemoveSuperArmorBySource(EARModifierSourceCategory Category, FName SourceId)` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:50>) | `IsSuperArmorActive` | 조회 | `bool IsSuperArmorActive() const` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:53>) | `IsStaggered` | 조회 | `bool IsStaggered() const` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:56>) | `IsStaggerImmune` | 조회 | `bool IsStaggerImmune() const` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:59>) | `GetCurrentGroggy` | 조회 | `float GetCurrentGroggy() const` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:62>) | `GetMaxGroggy` | 조회 | `float GetMaxGroggy() const` |
| [UARStaggerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:65>) | `ResetGroggyGauge` | 호출 | `float ResetGroggyGauge(float NewCurrentValue = -1.0f)` |

#### AR|Stamina

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARStaminaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:24>) | `CanAfford` | 조회 | `bool CanAfford(float Amount) const` |
| [UARStaminaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:25>) | `TryConsume` | 호출 | `bool TryConsume(float Amount, const FARSourceInfo& Source, float& NewStamina)` |
| [UARStaminaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:26>) | `Restore` | 호출 | `float Restore(float Amount, const FARSourceInfo& Source)` |
| [UARStaminaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:27>) | `GetCurrent` | 조회 | `float GetCurrent() const` |
| [UARStaminaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:28>) | `GetMax` | 조회 | `float GetMax() const` |
| [UARStaminaComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:29>) | `GetRatio` | 조회 | `float GetRatio() const` |

#### AR|Stats

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:14>) | `ApplyStatModifier` | 호출 | `static FARStatModifierHandle ApplyStatModifier(AActor* Target, const FARStatModifierSpec& Spec, bool& bSuccess)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:15>) | `RemoveStatModifier` | 호출 | `static bool RemoveStatModifier(AActor* Target, FARStatModifierHandle Handle)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:16>) | `RemoveStatModifiersBySource` | 호출 | `static int32 RemoveStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:17>) | `ClearStatModifiers` | 호출 | `static int32 ClearStatModifiers(AActor* Target, EARModifierSourceCategory Category = EARModifierSourceCategory::All)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:18>) | `RemoveOneStatModifierStack` | 호출 | `static bool RemoveOneStatModifierStack(AActor* Target, EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:19>) | `GetFinalStat` | 조회 | `static float GetFinalStat(AActor* Target, EARStatType StatType)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:20>) | `GetStatBreakdown` | 조회 | `static FARStatBreakdown GetStatBreakdown(AActor* Target, EARStatType StatType)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:21>) | `GetAllFinalStatViews` | 조회 | `static TArray<FARFinalStatView> GetAllFinalStatViews(AActor* Target)` |
| [UARStatsBlueprintLibrary](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h:22>) | `GetStatModifiersBySource` | 조회 | `static FARStatModifierQueryResult GetStatModifiersBySource(AActor* Target, EARModifierSourceCategory Category, FName SourceId)` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:94>) | `AddStatModifier` | 호출 | `FARStatModifierHandle AddStatModifier(const FARStatModifierSpec& Spec, bool& bSuccess)` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:97>) | `RemoveStatModifier` | 호출 | `bool RemoveStatModifier(FARStatModifierHandle Handle)` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:100>) | `RemoveModifiers` | 호출 | `int32 RemoveModifiers(EARModifierSourceCategory Category, FName SourceId)` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:103>) | `ClearModifiers` | 호출 | `int32 ClearModifiers(EARModifierSourceCategory Category = EARModifierSourceCategory::All)` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:106>) | `RemoveOneModifierStack` | 호출 | `bool RemoveOneModifierStack(EARModifierSourceCategory Category, FName SourceId, EARModifierStackRemovalPolicy Policy, FARStatModifierHandle& RemovedHandle)` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:109>) | `GetFinalStat` | 조회 | `float GetFinalStat(EARStatType StatType) const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:112>) | `GetBaseStat` | 조회 | `float GetBaseStat(EARStatType StatType) const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:115>) | `GetStatBreakdown` | 조회 | `FARStatBreakdown GetStatBreakdown(EARStatType StatType) const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:118>) | `GetAllFinalStatViews` | 조회 | `TArray<FARFinalStatView> GetAllFinalStatViews() const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:121>) | `GetModifiersBySource` | 조회 | `FARStatModifierQueryResult GetModifiersBySource(EARModifierSourceCategory Category, FName SourceId) const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:124>) | `GetModifierRemainingTime` | 조회 | `bool GetModifierRemainingTime(FARStatModifierHandle Handle, bool& bIsPermanent, float& RemainingSeconds) const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:127>) | `GetDamageRemainingMultiplier` | 조회 | `float GetDamageRemainingMultiplier(EARStatType ReductionStat) const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:130>) | `HasGuaranteedInvulnerability` | 조회 | `bool HasGuaranteedInvulnerability() const` |
| [UARStatsComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:133>) | `HasGuaranteedEvasion` | 조회 | `bool HasGuaranteedEvasion() const` |

#### AR|Status

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:35>) | `ApplyStatusEffect` | 호출 | `FARStatusEffectResult ApplyStatusEffect(const FARStatusEffectRequest& Request)` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:38>) | `RemoveStatusEffect` | 호출 | `bool RemoveStatusEffect(FARStatusEffectHandle Handle)` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:41>) | `RemoveStatusEffectsBySource` | 호출 | `int32 RemoveStatusEffectsBySource(EARModifierSourceCategory Category, FName SourceId)` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:44>) | `ClearAllStatusEffects` | 호출 | `int32 ClearAllStatusEffects()` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:47>) | `HasStatus` | 조회 | `bool HasStatus(FGameplayTag StatusTag) const` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:50>) | `GetStatusRemainingTime` | 조회 | `bool GetStatusRemainingTime(FGameplayTag StatusTag, float& RemainingTime) const` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:53>) | `GetActiveStatusEffects` | 조회 | `TArray<FARStatusEffectView> GetActiveStatusEffects() const` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:56>) | `BlocksBasicMovement` | 조회 | `bool BlocksBasicMovement() const` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:57>) | `BlocksAllMovement` | 조회 | `bool BlocksAllMovement() const` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:58>) | `BlocksRoll` | 조회 | `bool BlocksRoll() const` |
| [UARStatusEffectComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:59>) | `BlocksSkillGroups` | 조회 | `bool BlocksSkillGroups() const` |

#### AR|UI

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:28>) | `OpenScreen` | 호출 | `FARRequestStatus OpenScreen(EARUIScreen Screen, UObject* ContextObject = nullptr)` |
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:31>) | `CloseCurrentScreen` | 호출 | `bool CloseCurrentScreen()` |
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:34>) | `RequestBack` | 호출 | `void RequestBack()` |
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:37>) | `IsScreenOpen` | 조회 | `bool IsScreenOpen() const` |
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:38>) | `GetCurrentScreen` | 조회 | `EARUIScreen GetCurrentScreen() const` |
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:39>) | `GetCurrentContext` | 조회 | `UObject* GetCurrentContext() const` |

#### AR|UI|HUD

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UARUIManagerComponent](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h:40>) | `GetHUDSnapshot` | 조회 | `FARPlayerHUDSnapshot GetHUDSnapshot() const` |


### 7.2 기존 템플릿의 노드와 구현 이벤트

#### AoE Attack

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [ATwinStickAoEAttack](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/Gameplay/TwinStickAoEAttack.h:70>) | `BP_AoEFinished` | 구현 이벤트 | `void BP_AoEFinished()` |

#### Cursor

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AStrategyPlayerController](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyPlayerController.h:295>) | `BP_CursorFeedback` / Cursor Feedback | 구현 이벤트 | `void BP_CursorFeedback(FVector Location, bool bPositive)` |

#### Damage

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [ATwinStickCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:197>) | `BP_Damaged` / Damaged | 구현 이벤트 | `void BP_Damaged()` |

#### Input

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [ATwinStickCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:170>) | `DoMove` | 호출 | `void DoMove(float AxisX, float AxisY)` |
| [ATwinStickCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:174>) | `DoAim` | 호출 | `void DoAim(float AxisX, float AxisY)` |
| [ATwinStickCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:178>) | `DoDash` | 호출 | `void DoDash()` |
| [ATwinStickCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:182>) | `DoShoot` | 호출 | `void DoShoot()` |
| [ATwinStickCharacter](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:186>) | `DoAoEAttack` | 호출 | `void DoAoEAttack()` |

#### NPC

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [AStrategyUnit](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyUnit.h:82>) | `BP_UnitSelected` / Unit Selected | 구현 이벤트 | `void BP_UnitSelected()` |
| [AStrategyUnit](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyUnit.h:86>) | `BP_UnitDeselected` / Unit Deselected | 구현 이벤트 | `void BP_UnitDeselected()` |
| [AStrategyUnit](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyUnit.h:90>) | `BP_StopAnimation` / Stop Animation | 구현 이벤트 | `void BP_StopAnimation()` |
| [AStrategyUnit](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyUnit.h:94>) | `BP_InteractionBehavior` / Interaction Behavior | 구현 이벤트 | `void BP_InteractionBehavior(AStrategyUnit* Interactor)` |

#### Score

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UTwinStickUI](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/UI/TwinStickUI.h:21>) | `UpdateItems` | 구현 이벤트 | `void UpdateItems(int32 Score)` |
| [UTwinStickUI](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/UI/TwinStickUI.h:25>) | `UpdateScore` | 구현 이벤트 | `void UpdateScore(int32 Score)` |
| [UTwinStickUI](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/UI/TwinStickUI.h:29>) | `UpdateCombo` | 구현 이벤트 | `void UpdateCombo(int32 Combo)` |

#### UI

| 대상 클래스 | 함수/표시명 | 종류 | 정확한 선언 |
|---|---|---|---|
| [UStrategyTouchControls](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/UI/StrategyTouchControls.h:31>) | `BP_SetZoomPercentage` / Set Zoom Percentage | 구현 이벤트 | `void BP_SetZoomPercentage(float Percentage)` |
| [UStrategyTouchControls](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/UI/StrategyTouchControls.h:37>) | `ResetZoom` | 호출 | `void ResetZoom()` |
| [UStrategyTouchControls](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/UI/StrategyTouchControls.h:41>) | `ToggleSelectAllUnits` | 호출 | `void ToggleSelectAllUnits()` |
| [UStrategyTouchControls](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/UI/StrategyTouchControls.h:45>) | `SetZoomPercentage` | 호출 | `void SetZoomPercentage(float Percentage)` |
| [UStrategyUI](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/UI/StrategyUI.h:29>) | `BP_UpdateUnitsCount` / Update Units Count | 구현 이벤트 | `void BP_UpdateUnitsCount()` |
| [UStrategyUI](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/UI/StrategyUI.h:35>) | `GetSelectedUnitsCount` | 조회 | `int32 GetSelectedUnitsCount()` |


### 7.3 이벤트 디스패처 전체

아래 선언에서 타입과 핀 이름이 번갈아 나온다. Completed/Cancelled는 Action Delay의 출력이기도 하다.

#### AARActionHitboxActor

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnTargetAccepted` | `AActor*, Target` | AR\|Hitbox |

#### AARBaseCharacter

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnCharacterDeath` | `AARBaseCharacter*, Character, const FARCombatDamageResult&, KillingDamage` | AR\|Character |
| `OnAimDirectionChanged` | `AARBaseCharacter*, Character, FVector, AimDirection` | AR\|Character |

#### AARPlayerCharacter

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnAimWorldLocationChanged` | `AARPlayerCharacter*, Player, FVector, WorldLocation` | AR\|Player\|Aim |

#### UARActionComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnActionEnded` | `FARActionHandle, Handle` | AR\|Action |
| `OnActionCancelled` | `FARActionHandle, Handle, EARActionCancelReason, Reason` | AR\|Action |

#### UARAsyncActionDelay

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `Completed` | `없음` | (카테고리 미지정) |
| `Cancelled` | `없음` | (카테고리 미지정) |

#### UARCombatSubsystem

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnDamageReceived` | `AActor*, Subject, const FARCombatDamageResult&, Result` | AR\|Combat |
| `OnDamageHit` | `AActor*, Subject, const FARCombatDamageResult&, Result` | AR\|Combat |
| `OnDamageEvaded` | `AActor*, Subject, const FARCombatDamageResult&, Result` | AR\|Combat |
| `OnDamageBlocked` | `AActor*, Subject, const FARCombatDamageResult&, Result` | AR\|Combat |

#### UARConsumableComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnConsumableSlotsChanged` | `const TArray<FARConsumableSlotSnapshot>&, Slots` | AR\|Consumable |
| `OnConsumableDropRequested` | `const FARConsumableDropRequest&, DropRequest` | AR\|Consumable |

#### UARHealthComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnHealthChanged` | `AActor*, Target, float, CurrentHealth, float, MaxHealth, float, Delta, EARResourceChangeReason, Reason` | AR\|Health |
| `OnShieldChanged` | `AActor*, Target, float, CurrentShield, float, Delta, FARShieldHandle, Handle, EARResourceChangeReason, Reason` | AR\|Health |
| `OnDamageApplied` | `AActor*, Target, const FARCombatDamageResult&, Result` | AR\|Health |
| `OnDeath` | `AActor*, Target, const FARCombatDamageResult&, KillingDamage` | AR\|Health |

#### UARInteractionComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnInteractionCandidateChanged` | `AActor*, Candidate, FText, Prompt` | AR\|Interaction |
| `OnInteractionCompleted` | `AActor*, Target, FARRequestStatus, Status` | AR\|Interaction |

#### UARLoadoutComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnLoadoutChanged` | `int32, Revision` | AR\|Loadout |
| `OnRegisteredSkillsChanged` | `없음` | AR\|Loadout |
| `OnItemUIStateChanged` | `FGuid, ItemInstanceId, const FARItemUIState&, State, bool, bRemoved` | AR\|Loadout |

#### UARLoadoutItemInstance

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnItemUIStateChanged` | `FGuid, ItemInstanceId, const FARItemUIState&, State, bool, bRemoved` | AR\|Item |

#### UARManaComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnResourceChanged` | `AActor*, Target, float, Current, float, Maximum, float, Delta, EARResourceChangeReason, Reason, const FARSourceInfo&, Source` | AR\|Mana |

#### UARMovementControlComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnMovementLockChanged` | `AActor*, Target, bool, bCanBasicMove, bool, bCanMoveAtAll` | AR\|Movement |

#### UARStaggerComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnStaggered` | `AActor*, Target, const FARStaggerResult&, Result` | AR\|Stagger |
| `OnStaggerStateChanged` | `AActor*, Target, bool, bIsStaggered` | AR\|Stagger |
| `OnGroggyChanged` | `AActor*, Target, float, Current, float, Maximum, float, Delta` | AR\|Stagger |
| `OnGroggyGaugeDepleted` | `AActor*, Target` | AR\|Stagger |
| `OnSuperArmorChanged` | `AActor*, Target, bool, bIsActive` | AR\|Stagger |

#### UARStaminaComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnResourceChanged` | `AActor*, Target, float, Current, float, Maximum, float, Delta, EARResourceChangeReason, Reason, const FARSourceInfo&, Source` | AR\|Stamina |

#### UARStatsComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnFinalStatChanged` | `AActor*, Target, EARStatType, StatType, float, OldValue, float, NewValue` | AR\|Stats |
| `OnStatModifiersChanged` | `AActor*, Target, FName, SourceId, FText, DisplayName, int32, StackCount, float, LongestRemainingTime` | AR\|Stats |

#### UARStatusEffectComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnStatusAdded` | `AActor*, Target, const FARStatusEffectView&, Status` | AR\|Status |
| `OnStatusUpdated` | `AActor*, Target, const FARStatusEffectView&, Status` | AR\|Status |
| `OnStatusRemoved` | `AActor*, Target, const FARStatusEffectView&, Status` | AR\|Status |

#### UARUIManagerComponent

| 이벤트 | 전달 인자(타입, 이름) | 카테고리 |
|---|---|---|
| `OnScreenOpenRequested` | `EARUIScreen, Screen, UObject*, ContextObject` | AR\|UI |
| `OnScreenCloseRequested` | `EARUIScreen, Screen` | AR\|UI |
| `OnScreenBackRequested` | `EARUIScreen, Screen` | AR\|UI |
| `OnHUDSnapshotChanged` | `const FARPlayerHUDSnapshot&, Snapshot` | AR\|UI\|HUD |


### 7.4 BP 데이터 필드 전체

구조체 필드, 데이터 에셋 설정, 객체 속성을 소유 타입별로 모았다. 원본 링크에서 에디터 설정 범위와 Clamp 등 메타데이터를 확인할 수 있다. 읽기/쓰기는 BP 접근 권한이다.

#### AAction_RogueLikeCharacter

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Action_RogueLikeCharacter.h:23>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<UCameraComponent> TopDownCameraComponent` | 읽기 | Components |
| `TObjectPtr<USpringArmComponent> CameraBoom` | 읽기 | Components |

#### AARActionHitboxActor

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:44>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARActionHandle ActionHandle` | 읽기 | AR\|Hitbox |
| `EARHitboxHitPolicy HitPolicy = EARHitboxHitPolicy::OncePerTarget` | 읽기 | AR\|Hitbox |

#### AARBaseCharacter

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h:57>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARCombatTeam CombatTeam = EARCombatTeam::Enemy` | 읽기 | AR\|Character |
| `TObjectPtr<UARStatsComponent> StatsComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARHealthComponent> HealthComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARStatusEffectComponent> StatusEffectComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARStaggerComponent> StaggerComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARActionComponent> ActionComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARMovementControlComponent> MovementControlComponent` | 읽기 | AR\|Components |

#### AARConsumablePickup

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARItemPickupActors.h:61>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<UARConsumableDefinition> ConsumableDefinition` | 읽기 | AR\|Pickup |

#### AARItemPickupBase

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARItemPickupActors.h:26>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<USphereComponent> InteractionVolume` | 읽기 | AR\|Pickup |
| `FText InteractionPrompt` | 읽기 | AR\|Pickup |
| `int32 InteractionPriority = 0` | 읽기 | AR\|Pickup |
| `bool bRequiresLineOfSight = false` | 읽기 | AR\|Pickup |

#### AARLoadoutItemPickup

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Interaction/ARItemPickupActors.h:43>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<const UARLoadoutItemDefinition> ItemDefinition` | 읽기 | AR\|Pickup |

#### AARPlayerCharacter

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<UInputAction> MoveAction` | 읽기 | AR\|Input |
| `TObjectPtr<UInputAction> RollAction` | 읽기 | AR\|Input |
| `TObjectPtr<UInputAction> MouseRollAction` | 읽기 | AR\|Input |
| `TObjectPtr<UInputAction> InteractAction` | 읽기 | AR\|Input |
| `TArray<FARSkillInputBinding> SkillInputBindings` | 읽기 | AR\|Input |
| `TArray<TObjectPtr<UInputAction>> ConsumableSlotActions` | 읽기 | AR\|Input |
| `TObjectPtr<UInputAction> InventoryAction` | 읽기 | AR\|Input |
| `TObjectPtr<UInputAction> UIBackAction` | 읽기 | AR\|Input |
| `float RollDuration = 0.20f` | 읽기 | AR\|Roll |
| `float RollCooldown = 0.50f` | 읽기 | AR\|Roll |
| `EARRollDirectionMode RollDirectionMode = EARRollDirectionMode::MovementOrMouse` | 읽기/쓰기 | AR\|Roll |
| `float PostHitInvulnerabilityDuration = 0.0f` | 읽기 | AR\|Damage |
| `TObjectPtr<UARStaminaComponent> StaminaComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARManaComponent> ManaComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARLoadoutComponent> LoadoutComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARConsumableComponent> ConsumableComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARInteractionComponent> InteractionComponent` | 읽기 | AR\|Components |
| `TObjectPtr<UARUIManagerComponent> UIManagerComponent` | 읽기 | AR\|Components |
| `TObjectPtr<USceneComponent> CameraAnchor` | 읽기 | AR\|Components |
| `TObjectPtr<UCameraComponent> TopDownCamera` | 읽기 | AR\|Components |
| `TObjectPtr<UARCameraFollowComponent> CameraFollowComponent` | 읽기 | AR\|Components |

#### AARPlayerController

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Player/ARPlayerController.h:24>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<UInputMappingContext> PlayerMappingContext` | 읽기 | AR\|Input |
| `int32 MappingPriority = 0` | 읽기 | AR\|Input |

#### AStrategyPawn

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyPawn.h:22>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `UCameraComponent* Camera` | 읽기 | Components |
| `UFloatingPawnMovement* FloatingPawnMovement` | 읽기 | Components |

#### AStrategyUnit

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_Strategy/StrategyUnit.h:30>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `USphereComponent* InteractionRange` | 읽기 | Components |

#### ATwinStickAIController

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/AI/TwinStickAIController.h:21>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `UStateTreeAIComponent* StateTreeAI` | 읽기 | Components |

#### ATwinStickAoEAttack

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/Gameplay/TwinStickAoEAttack.h:22>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `UStaticMeshComponent* SphereVisual` | 읽기 | Components |
| `USphereComponent* CollisionSphere` | 읽기 | Components |

#### ATwinStickCharacter

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/TwinStickCharacter.h:29>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `USpringArmComponent* SpringArm` | 읽기 | Components |
| `UCameraComponent* Camera` | 읽기 | Components |

#### ATwinStickNPC

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/AI/TwinStickNPC.h:50>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bHit = false` | 읽기 | NPC |

#### ATwinStickPickup

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/Gameplay/TwinStickPickup.h:21>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `USphereComponent* CollisionSphere` | 읽기 | Components |
| `UStaticMeshComponent* Mesh` | 읽기 | Components |

#### ATwinStickProjectile

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Variant_TwinStick/Gameplay/TwinStickProjectile.h:22>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `USphereComponent* CollisionSphere` | 읽기 | Components |
| `UStaticMeshComponent* Mesh` | 읽기 | Components |
| `UProjectileMovementComponent* ProjectileMovement` | 읽기 | Components |

#### FARAcquisitionToken

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:299>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARActionCancelRules

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Actions/ARActionTypes.h:13>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bCancelOnStagger = true` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bCancelOnStun = true` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bCancelOnRoll = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bCancelOnBasicMovementInput = false` | 읽기/쓰기 | (카테고리 미지정) |

#### FARActionHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:257>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |
| `TWeakObjectPtr<UObject> Owner` | 읽기 | Handle |
| `int64 Serial = 0` | 읽기 | Handle |

#### FARActionRequest

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Actions/ARActionTypes.h:24>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTag ActionTag` | 읽기/쓰기 | (카테고리 미지정) |
| `FARActionCancelRules CancelRules` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bBlockRollWhileActive = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bBlockBasicMovementWhileActive = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bIsRollAction = false` | 읽기/쓰기 | (카테고리 미지정) |
| `FARSkillGroupHandle SkillGroupHandle` | 읽기/쓰기 | (카테고리 미지정) |
| `FGuid OwningItemInstanceId` | 읽기/쓰기 | (카테고리 미지정) |

#### FARCombatDamageRequest

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:57>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<AActor> Attacker = nullptr` | 읽기/쓰기 | (카테고리 미지정) |
| `TObjectPtr<AActor> Target = nullptr` | 읽기/쓰기 | (카테고리 미지정) |
| `EARDamageDelivery Delivery = EARDamageDelivery::Direct` | 읽기/쓰기 | (카테고리 미지정) |
| `EARDamageAttribute Attribute = EARDamageAttribute::Physical` | 읽기/쓰기 | (카테고리 미지정) |
| `float BaseDamage = 0.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `float AttackPowerCoefficient = 0.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `float SpellPowerCoefficient = 0.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bGuaranteedHit = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bApplyEvasion = true` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bCanCrit = true` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bApplyAmplification = true` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bIgnoreDefense = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bIgnoreShield = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bApplyAbsorption = false` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bApplyOnHitEffects = true` | 읽기/쓰기 | (카테고리 미지정) |
| `FARSourceInfo Source` | 읽기/쓰기 | (카테고리 미지정) |
| `FName DamageName = NAME_None` | 읽기/쓰기 | (카테고리 미지정) |
| `FGameplayTag AttackTag` | 읽기/쓰기 | (카테고리 미지정) |

#### FARCombatDamageResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:85>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARDamageOutcome Outcome = EARDamageOutcome::Invalid` | 읽기 | (카테고리 미지정) |
| `EARRequestResult FailureReason = EARRequestResult::Rejected` | 읽기 | (카테고리 미지정) |
| `bool bCritical = false` | 읽기 | (카테고리 미지정) |
| `bool bKilledTarget = false` | 읽기 | (카테고리 미지정) |
| `bool bOnHitEffectsTriggered = false` | 읽기 | (카테고리 미지정) |
| `int32 FinalDamage = 0` | 읽기 | (카테고리 미지정) |
| `int32 ShieldDamage = 0` | 읽기 | (카테고리 미지정) |
| `int32 HealthDamage = 0` | 읽기 | (카테고리 미지정) |
| `FARDamageHitContext HitContext` | 읽기 | (카테고리 미지정) |

#### FARConsumableAcquisitionResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:55>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARRequestStatus Status` | 읽기 | (카테고리 미지정) |
| `int32 SlotIndex = INDEX_NONE` | 읽기 | (카테고리 미지정) |
| `FGuid InstanceId` | 읽기 | (카테고리 미지정) |

#### FARConsumableDisplayData

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:39>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `int32 SlotIndex = INDEX_NONE` | 읽기 | (카테고리 미지정) |
| `FGuid InstanceId` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<const UARConsumableDefinition> Definition = nullptr` | 읽기 | (카테고리 미지정) |
| `FGameplayTag DefinitionTag` | 읽기 | (카테고리 미지정) |
| `FGameplayTag ItemTypeTag` | 읽기 | (카테고리 미지정) |
| `FText DisplayName` | 읽기 | (카테고리 미지정) |
| `FText ShortDescription` | 읽기 | (카테고리 미지정) |
| `FText DetailedDescription` | 읽기 | (카테고리 미지정) |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | (카테고리 미지정) |

#### FARConsumableDropRequest

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:65>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<UARConsumableDefinition> Definition = nullptr` | 읽기 | (카테고리 미지정) |
| `FVector SuggestedLocation = FVector::ZeroVector` | 읽기 | (카테고리 미지정) |
| `int32 PreviousSlotIndex = INDEX_NONE` | 읽기 | (카테고리 미지정) |
| `bool bCausedBySlotReduction = false` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<AARConsumablePickup> SpawnedPickup = nullptr` | 읽기 | (카테고리 미지정) |

#### FARConsumableSlotSnapshot

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:26>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `int32 SlotIndex = INDEX_NONE` | 읽기 | (카테고리 미지정) |
| `bool bOccupied = false` | 읽기 | (카테고리 미지정) |
| `FGuid InstanceId` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<UARConsumableDefinition> Definition = nullptr` | 읽기 | (카테고리 미지정) |
| `bool bOverflowSlot = false` | 읽기 | (카테고리 미지정) |

#### FARDamageHitContext

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:40>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid HitId` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<AActor> Attacker = nullptr` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<AActor> Target = nullptr` | 읽기 | (카테고리 미지정) |
| `EARDamageDelivery Delivery = EARDamageDelivery::Direct` | 읽기 | (카테고리 미지정) |
| `EARDamageAttribute Attribute = EARDamageAttribute::Physical` | 읽기 | (카테고리 미지정) |
| `bool bValidHit = false` | 읽기 | (카테고리 미지정) |
| `float StaggerPowerSnapshot = 0.0f` | 읽기 | (카테고리 미지정) |
| `float GroggyDamageAmplificationSnapshot = 0.0f` | 읽기 | (카테고리 미지정) |

#### FARDamageOverTimeSpec

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDotTypes.h:13>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARCombatDamageRequest DamageRequest` | 읽기/쓰기 | (카테고리 미지정) |
| `float Duration = 1.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `float TickInterval = 0.25f` | 읽기/쓰기 | (카테고리 미지정) |
| `FName DotName = NAME_None` | 읽기/쓰기 | (카테고리 미지정) |
| `EARDotStackPolicy StackPolicy = EARDotStackPolicy::Independent` | 읽기/쓰기 | (카테고리 미지정) |
| `bool bApplyStaggerAndGroggyEachTick = false` | 읽기/쓰기 | (카테고리 미지정) |
| `FARStaggerRequestTemplate StaggerTemplate` | 읽기/쓰기 | (카테고리 미지정) |

#### FARDotHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:289>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FAREvolutionToken

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:309>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARFinalStatView

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:53>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARStatType StatType = EARStatType::MaxHealth` | 읽기 | Stat |
| `FText DisplayName` | 읽기 | Stat |
| `FARStatBreakdown Breakdown` | 읽기 | Stat |

#### FARHUDResourceValue

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/UI/ARUITypes.h:26>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float Current = 0.0f` | 읽기 | (카테고리 미지정) |
| `float Maximum = 0.0f` | 읽기 | (카테고리 미지정) |
| `float Ratio = 0.0f` | 읽기 | (카테고리 미지정) |

#### FARItemUIState

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:68>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTag StateId` | 읽기 | (카테고리 미지정) |
| `float CurrentValue = 0.0f` | 읽기 | (카테고리 미지정) |
| `float MaximumValue = 0.0f` | 읽기 | (카테고리 미지정) |
| `EARItemUIStateDisplayType EffectiveDisplayType = EARItemUIStateDisplayType::Number` | 읽기 | (카테고리 미지정) |

#### FARLoadoutAcquisitionResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:162>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARRequestStatus Status` | 읽기 | (카테고리 미지정) |
| `FARAcquisitionToken Token` | 읽기 | (카테고리 미지정) |
| `bool bWillReplaceWeapon = false` | 읽기 | (카테고리 미지정) |

#### FARLoadoutDropRequest

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:172>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<const UARLoadoutItemDefinition> Definition = nullptr` | 읽기 | (카테고리 미지정) |
| `FVector SuggestedLocation = FVector::ZeroVector` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<AARLoadoutItemPickup> SpawnedPickup = nullptr` | 읽기 | (카테고리 미지정) |

#### FARLoadoutItemDisplayData

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:132>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid InstanceId` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<const UARLoadoutItemDefinition> Definition = nullptr` | 읽기 | (카테고리 미지정) |
| `FGameplayTag DefinitionTag` | 읽기 | (카테고리 미지정) |
| `FGameplayTag ItemTypeTag` | 읽기 | (카테고리 미지정) |
| `EARLoadoutItemKind Kind = EARLoadoutItemKind::PassiveRelic` | 읽기 | (카테고리 미지정) |
| `FText DisplayName` | 읽기 | (카테고리 미지정) |
| `FText ShortDescription` | 읽기 | (카테고리 미지정) |
| `FText DetailedDescription` | 읽기 | (카테고리 미지정) |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | (카테고리 미지정) |
| `TArray<FARProvidedStatDisplayData> ProvidedStats` | 읽기 | (카테고리 미지정) |
| `TArray<FARSkillDefinition> Skills` | 읽기 | (카테고리 미지정) |
| `TArray<FARItemUIState> UIStates` | 읽기 | (카테고리 미지정) |

#### FARLoadoutItemSnapshot

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:151>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid InstanceId` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<const UARLoadoutItemDefinition> Definition = nullptr` | 읽기 | (카테고리 미지정) |
| `EARLoadoutItemKind Kind = EARLoadoutItemKind::PassiveRelic` | 읽기 | (카테고리 미지정) |
| `TArray<FARItemUIState> UIStates` | 읽기 | (카테고리 미지정) |

#### FARMovementLockHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:247>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FAROffensiveStatSnapshot

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:22>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float AttackPower = 0.0f` | 읽기 | (카테고리 미지정) |
| `float SpellPower = 0.0f` | 읽기 | (카테고리 미지정) |
| `float StaggerPower = 0.0f` | 읽기 | (카테고리 미지정) |
| `float GroggyDamageAmplification = 0.0f` | 읽기 | (카테고리 미지정) |
| `float CriticalChance = 0.0f` | 읽기 | (카테고리 미지정) |
| `float CriticalDamage = 150.0f` | 읽기 | (카테고리 미지정) |
| `float DefensePenetration = 0.0f` | 읽기 | (카테고리 미지정) |
| `float Absorption = 0.0f` | 읽기 | (카테고리 미지정) |
| `float OverallAmplification = 0.0f` | 읽기 | (카테고리 미지정) |
| `float DeliveryAmplification = 0.0f` | 읽기 | (카테고리 미지정) |
| `float AttributeAmplification = 0.0f` | 읽기 | (카테고리 미지정) |

#### FARPlayerHUDSnapshot

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/UI/ARUITypes.h:37>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARHUDResourceValue Health` | 읽기 | (카테고리 미지정) |
| `float Shield = 0.0f` | 읽기 | (카테고리 미지정) |
| `FARHUDResourceValue Mana` | 읽기 | (카테고리 미지정) |
| `FARHUDResourceValue Stamina` | 읽기 | (카테고리 미지정) |
| `FVector AimDirection = FVector::ForwardVector` | 읽기 | (카테고리 미지정) |
| `bool bHasAimWorldLocation = false` | 읽기 | (카테고리 미지정) |
| `FVector AimWorldLocation = FVector::ZeroVector` | 읽기 | (카테고리 미지정) |
| `bool bHasEquippedWeapon = false` | 읽기 | (카테고리 미지정) |
| `FARLoadoutItemSnapshot EquippedWeapon` | 읽기 | (카테고리 미지정) |
| `TArray<FARLoadoutItemSnapshot> ActiveRelics` | 읽기 | (카테고리 미지정) |
| `TArray<FARRegisteredSkillUIData> Skills` | 읽기 | (카테고리 미지정) |
| `TArray<FARConsumableSlotSnapshot> Consumables` | 읽기 | (카테고리 미지정) |
| `TArray<FARStatusEffectView> StatusEffects` | 읽기 | (카테고리 미지정) |

#### FARProvidedStatDisplayData

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:119>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARStatType StatType = EARStatType::AttackPower` | 읽기 | (카테고리 미지정) |
| `EARStatModifierOperation Operation = EARStatModifierOperation::Flat` | 읽기 | (카테고리 미지정) |
| `float Value = 0.0f` | 읽기 | (카테고리 미지정) |
| `FText StatName` | 읽기 | (카테고리 미지정) |
| `FText DisplayText` | 읽기 | (카테고리 미지정) |

#### FARRegisteredSkillHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:279>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARRegisteredSkillUIData

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:98>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARRegisteredSkillHandle RegisteredHandle` | 읽기 | (카테고리 미지정) |
| `FGuid ItemInstanceId` | 읽기 | (카테고리 미지정) |
| `FName SkillId = NAME_None` | 읽기 | (카테고리 미지정) |
| `FGameplayTag InputTag` | 읽기 | (카테고리 미지정) |
| `FText DisplayName` | 읽기 | (카테고리 미지정) |
| `FText Description` | 읽기 | (카테고리 미지정) |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<const UARLoadoutItemDefinition> SourceDefinition = nullptr` | 읽기 | (카테고리 미지정) |
| `int32 HUDSortOrder = 0` | 읽기 | (카테고리 미지정) |
| `FARResourceCost ResourceCost` | 읽기 | (카테고리 미지정) |
| `float CooldownRemaining = 0.0f` | 읽기 | (카테고리 미지정) |
| `float CooldownTotal = 0.0f` | 읽기 | (카테고리 미지정) |
| `bool bReady = true` | 읽기 | (카테고리 미지정) |

#### FARRequestStatus

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:332>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARRequestResult Result = EARRequestResult::Rejected` | 읽기 | Request |
| `FText Message` | 읽기 | Request |

#### FARResourceCost

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:320>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float Mana = 0.0f` | 읽기/쓰기 | Resource |
| `float Stamina = 0.0f` | 읽기/쓰기 | Resource |

#### FARShieldHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:217>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARShieldSpec

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:16>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float Amount = 0.0f` | 읽기/쓰기 | Shield |
| `float Duration = -1.0f` | 읽기/쓰기 | Shield |
| `FARSourceInfo Source` | 읽기/쓰기 | Shield |

#### FARSkillDefinition

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:79>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FName SkillId = NAME_None` | 읽기 | Identity |
| `FGameplayTag InputTag` | 읽기 | Input |
| `int32 InputPriority = 0` | 읽기 | Input |
| `FARResourceCost ResourceCost` | 읽기 | Cost |
| `float BaseCooldown = 0.0f` | 읽기 | Cooldown |
| `float MinimumCooldown = 0.0f` | 읽기 | Cooldown |
| `FARActionRequest ActionRequest` | 읽기 | Action |
| `FText SkillDisplayName` | 읽기 | Display |
| `FText SkillDescription` | 읽기 | Display |
| `TSoftObjectPtr<UTexture2D> SkillIcon` | 읽기 | Display |
| `bool bShowOnHUD = true` | 읽기 | Display |
| `int32 HUDSortOrder = 0` | 읽기 | Display |

#### FARSkillGroupHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:269>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARSkillInputBinding

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<UInputAction> InputAction = nullptr` | 읽기 | Input |
| `FGameplayTag InputTag` | 읽기 | Input |

#### FARSourceInfo

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:193>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARModifierSourceCategory Category = EARModifierSourceCategory::Other` | 읽기/쓰기 | Source |
| `FName SourceId = NAME_None` | 읽기/쓰기 | Source |
| `FText DisplayName` | 읽기/쓰기 | Source |

#### FARStaggerRequest

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:24>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARDamageHitContext HitContext` | 읽기/쓰기 | (카테고리 미지정) |
| `FARStaggerRequestTemplate Template` | 읽기/쓰기 | (카테고리 미지정) |

#### FARStaggerRequestTemplate

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:12>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float BaseStaggerDamage = 0.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `float StaggerMultiplier = 1.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `float BaseGroggyDamage = 0.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `FARSourceInfo Source` | 읽기/쓰기 | (카테고리 미지정) |
| `FName EffectName = NAME_None` | 읽기/쓰기 | (카테고리 미지정) |

#### FARStaggerResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:33>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bValidRequest = false` | 읽기 | (카테고리 미지정) |
| `bool bStaggerAttempted = false` | 읽기 | (카테고리 미지정) |
| `bool bStaggered = false` | 읽기 | (카테고리 미지정) |
| `bool bBlockedBySuperArmor = false` | 읽기 | (카테고리 미지정) |
| `bool bGroggyDepleted = false` | 읽기 | (카테고리 미지정) |
| `int32 FinalStaggerDamage = 0` | 읽기 | (카테고리 미지정) |
| `int32 FinalGroggyDamage = 0` | 읽기 | (카테고리 미지정) |
| `float CurrentGroggy = 0.0f` | 읽기 | (카테고리 미지정) |

#### FARStatBreakdown

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:40>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float BaseValue = 0.0f` | 읽기 | Stat |
| `float FlatTotal = 0.0f` | 읽기 | Stat |
| `float AdditivePercentTotal = 0.0f` | 읽기 | Stat |
| `float MultiplicativeProduct = 1.0f` | 읽기 | Stat |
| `float FinalValue = 0.0f` | 읽기 | Stat |

#### FARStatModifierHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:207>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARStatModifierQueryResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:63>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bExists = false` | 읽기 | Stat |
| `int32 StackCount = 0` | 읽기 | Stat |
| `bool bHasPermanent = false` | 읽기 | Stat |
| `float LongestRemainingTime = 0.0f` | 읽기 | Stat |

#### FARStatModifierSpec

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:13>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARStatType StatType = EARStatType::AttackPower` | 읽기/쓰기 | Stat |
| `EARStatModifierOperation Operation = EARStatModifierOperation::Flat` | 읽기/쓰기 | Stat |
| `float Value = 0.0f` | 읽기/쓰기 | Stat |
| `float Duration = -1.0f` | 읽기/쓰기 | Stat |
| `FARSourceInfo Source` | 읽기/쓰기 | Stat |
| `bool bGuaranteeInvulnerability = false` | 읽기/쓰기 | Guarantee |
| `bool bGuaranteeEvasion = false` | 읽기/쓰기 | Guarantee |

#### FARStatusEffectHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:227>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARStatusEffectRequest

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectTypes.h:15>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TObjectPtr<const UARStatusEffectDefinition> Definition = nullptr` | 읽기/쓰기 | Status |
| `float DurationOverride = -1.0f` | 읽기/쓰기 | Status |
| `FARSourceInfo Source` | 읽기/쓰기 | Status |

#### FARStatusEffectResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectTypes.h:26>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `EARRequestResult Result = EARRequestResult::Rejected` | 읽기 | Status |
| `FARStatusEffectHandle Handle` | 읽기 | Status |
| `FGameplayTag StatusTag` | 읽기 | Status |
| `float AppliedDuration = 0.0f` | 읽기 | Status |
| `bool bRefreshedExisting = false` | 읽기 | Status |

#### FARStatusEffectView

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectTypes.h:38>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARStatusEffectHandle Handle` | 읽기 | Status |
| `FGameplayTag StatusTag` | 읽기 | Status |
| `TObjectPtr<const UARStatusEffectDefinition> Definition = nullptr` | 읽기 | Status |
| `FARSourceInfo Source` | 읽기 | Status |
| `float RemainingTime = 0.0f` | 읽기 | Status |

#### FARSuperArmorHandle

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:237>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGuid Id` | 읽기 | Handle |

#### FARSuperArmorSpec

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:48>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float Duration = -1.0f` | 읽기/쓰기 | (카테고리 미지정) |
| `FARSourceInfo Source` | 읽기/쓰기 | (카테고리 미지정) |

#### FARUIStateDisplayDefinition

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:54>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTag StateId` | 읽기 | (카테고리 미지정) |
| `FText DisplayName` | 읽기 | (카테고리 미지정) |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | (카테고리 미지정) |
| `EARItemUIStateDisplayType DisplayType = EARItemUIStateDisplayType::Number` | 읽기 | (카테고리 미지정) |
| `int32 MaxDisplaySlots = 6` | 읽기 | (카테고리 미지정) |
| `bool bShowNumberAlongside = false` | 읽기 | (카테고리 미지정) |
| `EARItemUIStatePlacement Placement = EARItemUIStatePlacement::Both` | 읽기 | (카테고리 미지정) |

#### FARWeaponEvolutionResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:182>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARRequestStatus Status` | 읽기 | (카테고리 미지정) |
| `FAREvolutionToken Token` | 읽기 | (카테고리 미지정) |
| `TArray<TSoftObjectPtr<class UARWeaponDefinition>> Candidates` | 읽기 | (카테고리 미지정) |
| `bool bRequiresSelection = false` | 읽기 | (카테고리 미지정) |
| `TObjectPtr<UARLoadoutItemInstance> EvolvedInstance = nullptr` | 읽기 | (카테고리 미지정) |

#### UARCameraFollowComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARCameraFollowComponent.h:24>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float BaseFollowSpeed = 5.0f` | 읽기/쓰기 | AR\|Camera |
| `float AccelerationStartDistance = 150.0f` | 읽기/쓰기 | AR\|Camera |
| `float DistanceSpeedMultiplier = 0.025f` | 읽기/쓰기 | AR\|Camera |
| `float MaximumFollowSpeed = 18.0f` | 읽기/쓰기 | AR\|Camera |
| `float SnapDistance = 1200.0f` | 읽기/쓰기 | AR\|Camera |
| `float CameraHeight = 2000.0f` | 읽기/쓰기 | AR\|Camera |

#### UARCombatSourceComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARCombatSourceComponent.h:21>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FARSourceInfo SourceInfo` | 읽기 | AR\|Combat |

#### UARConsumableComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARConsumableComponent.h:57>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TSubclassOf<AARConsumablePickup> DroppedConsumablePickupClass` | 읽기 | AR\|Consumable\|Drop |
| `float DropForwardDistance = 96.0f` | 읽기 | AR\|Consumable\|Drop |

#### UARConsumableDefinition

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableDefinition.h:19>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTag DefinitionTag` | 읽기 | Identity |
| `FGameplayTag ItemTypeTag` | 읽기 | Identity |
| `FText DisplayName` | 읽기 | Display |
| `FText ShortDescription` | 읽기 | Display |
| `FText DetailedDescription` | 읽기 | Display |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | Display |
| `TSubclassOf<UARConsumableInstance> RuntimeBehaviorClass` | 읽기 | Runtime |

#### UARHealthComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:103>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bStartAtFullHealth = true` | 읽기 | AR\|Health |
| `bool bDamageSystemEnabled = true` | 읽기 | AR\|Health |

#### UARInteractionComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARInteractionComponent.h:36>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `float InteractionRadius = 160.0f` | 읽기 | AR\|Interaction |
| `float ScanInterval = 0.1f` | 읽기 | AR\|Interaction |
| `TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility` | 읽기 | AR\|Interaction |

#### UARLoadoutComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARLoadoutComponent.h:125>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `int32 MaxActiveRelics = 2` | 읽기 | AR\|Loadout |
| `TSubclassOf<AARLoadoutItemPickup> DroppedItemPickupClass` | 읽기 | AR\|Loadout\|Drop |
| `float DropForwardDistance = 96.0f` | 읽기 | AR\|Loadout\|Drop |

#### UARLoadoutItemDefinition

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemDefinition.h:21>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTag DefinitionTag` | 읽기 | Identity |
| `FGameplayTag ItemTypeTag` | 읽기 | Identity |
| `FText DisplayName` | 읽기 | Display |
| `FText ShortDescription` | 읽기 | Display |
| `FText DetailedDescription` | 읽기 | Display |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | Display |
| `TArray<FARStatModifierSpec> DefaultStatModifiers` | 읽기 | Stats |
| `TArray<FARSkillDefinition> SkillDefinitions` | 읽기 | Skills |
| `TSubclassOf<UARLoadoutItemInstance> RuntimeBehaviorClass` | 읽기 | Runtime |
| `TArray<FARUIStateDisplayDefinition> UIStateDisplayDefinitions` | 읽기 | UI |

#### UARManaComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARManaComponent.h:32>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bStartFull = true` | 읽기 | AR\|Mana |

#### UARStaggerComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h:74>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bCanBeStaggered = true` | 읽기 | AR\|Stagger |
| `float BaseStaggerDuration = 0.2f` | 읽기 | AR\|Stagger |
| `float PostStaggerImmunityDuration = 0.1f` | 읽기 | AR\|Stagger |
| `bool bUseGroggyGauge = false` | 읽기 | AR\|Groggy |

#### UARStaminaComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStaminaComponent.h:32>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `bool bStartFull = true` | 읽기 | AR\|Stamina |

#### UARStatsComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:144>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `TMap<EARStatType, float> BaseStats` | 읽기 | AR\|Stats\|Base |
| `float MaxEvasion = 50.0f` | 읽기 | AR\|Stats\|Caps |
| `float MaxCriticalChance = 100.0f` | 읽기 | AR\|Stats\|Caps |
| `float MaxTenacity = 100.0f` | 읽기 | AR\|Stats\|Caps |
| `float MaxCooldownReduction = 40.0f` | 읽기 | AR\|Stats\|Caps |

#### UARStatusEffectComponent

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h:65>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTagContainer ImmuneStatusTags` | 읽기 | AR\|Status |

#### UARStatusEffectDefinition

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectDefinition.h:19>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FGameplayTag DefinitionTag` | 읽기 | Identity |
| `FGameplayTag StatusTag` | 읽기 | Identity |
| `FText DisplayName` | 읽기 | Display |
| `TSoftObjectPtr<UTexture2D> Icon` | 읽기 | Display |
| `float BaseDuration = 1.0f` | 읽기 | Rules |
| `bool bAffectedByTenacity = true` | 읽기 | Rules |
| `bool bCanBeImmune = true` | 읽기 | Rules |
| `bool bBlocksBasicMovement = true` | 읽기 | Rules |
| `bool bBlocksAllMovement = true` | 읽기 | Rules |
| `bool bBlocksRoll = true` | 읽기 | Rules |
| `bool bBlocksSkillGroups = false` | 읽기 | Rules |
| `bool bCancelActionsOnApply = false` | 읽기 | Rules |
| `EARActionCancelReason ActionCancelReason = EARActionCancelReason::Stun` | 읽기 | Rules |

#### UARWeaponDefinition

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemDefinition.h:41>)

| 필드 선언/기본값 | BP 접근 | 카테고리 |
|---|---|---|
| `FName EvolutionGroupId = NAME_None` | 읽기 | Evolution |
| `int32 EvolutionStage = 1` | 읽기 | Evolution |
| `TArray<TSoftObjectPtr<UARWeaponDefinition>> NextEvolutionCandidates` | 읽기 | Evolution |


### 7.5 BlueprintType 구조체 색인

Make/Break 또는 핀 분할에 사용하는 데이터 타입이다. 필드는 위의 동일 이름 표에 정리했다. C++ 전용 IsValid/IsSuccess/WasApplied 같은 멤버 함수는 BP 노드로 노출되지 않는다. 결과 필드를 분기하고, 행동 유효성은 IsActionActive 등을 사용한다.

- [FARAcquisitionToken](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:295>)
- [FARActionCancelRules](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Actions/ARActionTypes.h:8>)
- [FARActionHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:253>)
- [FARActionRequest](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Actions/ARActionTypes.h:19>)
- [FARCombatDamageRequest](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:52>)
- [FARCombatDamageResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:80>)
- [FARConsumableAcquisitionResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:50>)
- [FARConsumableDisplayData](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:34>)
- [FARConsumableDropRequest](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:60>)
- [FARConsumableSlotSnapshot](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:21>)
- [FARDamageHitContext](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:35>)
- [FARDamageOverTimeSpec](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDotTypes.h:8>)
- [FARDotHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:285>)
- [FAREvolutionToken](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:305>)
- [FARFinalStatView](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:48>)
- [FARHUDResourceValue](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/UI/ARUITypes.h:21>)
- [FARItemUIState](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:63>)
- [FARLoadoutAcquisitionResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:157>)
- [FARLoadoutDropRequest](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:167>)
- [FARLoadoutItemDisplayData](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:127>)
- [FARLoadoutItemSnapshot](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:146>)
- [FARMovementLockHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:243>)
- [FAROffensiveStatSnapshot](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:17>)
- [FARPlayerHUDSnapshot](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/UI/ARUITypes.h:32>)
- [FARProvidedStatDisplayData](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:114>)
- [FARRegisteredSkillHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:275>)
- [FARRegisteredSkillUIData](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:93>)
- [FARRequestStatus](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:327>)
- [FARResourceCost](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:315>)
- [FARShieldHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:213>)
- [FARShieldSpec](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h:11>)
- [FARSkillDefinition](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:74>)
- [FARSkillGroupHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:265>)
- [FARSkillInputBinding](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h:21>)
- [FARSourceInfo](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:188>)
- [FARStaggerRequest](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:19>)
- [FARStaggerRequestTemplate](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:7>)
- [FARStaggerResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:28>)
- [FARStatBreakdown](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:35>)
- [FARStatModifierHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:203>)
- [FARStatModifierQueryResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:58>)
- [FARStatModifierSpec](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h:8>)
- [FARStatusEffectHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:223>)
- [FARStatusEffectRequest](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectTypes.h:10>)
- [FARStatusEffectResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectTypes.h:21>)
- [FARStatusEffectView](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Status/ARStatusEffectTypes.h:33>)
- [FARSuperArmorHandle](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:233>)
- [FARSuperArmorSpec](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARStaggerTypes.h:43>)
- [FARUIStateDisplayDefinition](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:49>)
- [FARWeaponEvolutionResult](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:177>)

### 7.6 BlueprintType 열거형 전체

#### EARActionCancelReason

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:125>)

~~~cpp
None,
	Stagger,
	Stun,
	Root,
	Roll,
	BasicMovementInput,
	ItemRemoved,
	Death,
	Manual
~~~

#### EARCombatTeam

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:7>)

~~~cpp
Player,
	Enemy,
	Environment
~~~

#### EARConsumableRemovalReason

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARConsumableTypes.h:11>)

~~~cpp
Used,
	Dropped,
	SlotReduced,
	OwnerDestroyed,
	Manual
~~~

#### EARDamageAttribute

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:22>)

~~~cpp
Physical,
	Fire,
	Magic,
	Void
~~~

#### EARDamageDelivery

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:15>)

~~~cpp
Direct,
	DamageOverTime
~~~

#### EARDamageOutcome

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARDamageTypes.h:7>)

~~~cpp
Invalid,
	Queued,
	Applied,
	Evaded,
	Blocked
~~~

#### EARDotStackPolicy

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:166>)

~~~cpp
Independent,
	RefreshSameName
~~~

#### EARHitboxHitPolicy

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Combat/ARActionHitboxActor.h:8>)

~~~cpp
OncePerTarget,
	EveryAcceptedOverlap
~~~

#### EARItemRemovalReason

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:23>)

~~~cpp
Replaced,
	Discarded,
	Evolved,
	OwnerDestroyed,
	Manual
~~~

#### EARItemUIStateDisplayType

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:33>)

~~~cpp
Gauge,
	Number,
	SmallStack
~~~

#### EARItemUIStatePlacement

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:41>)

~~~cpp
HUD,
	Details,
	Both
~~~

#### EARLoadoutItemKind

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Items/ARItemTypes.h:15>)

~~~cpp
Weapon,
	ActiveRelic,
	PassiveRelic
~~~

#### EARModifierSourceCategory

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:31>)

~~~cpp
Weapon,
	Relic,
	Buff,
	Other,
	All
~~~

#### EARModifierStackRemovalPolicy

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:173>)

~~~cpp
Newest,
	Oldest
~~~

#### EARMovementLockType

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:139>)

~~~cpp
BasicMovementOnly,
	AllMovement
~~~

#### EARRequestResult

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:146>)

~~~cpp
Success,
	InvalidOwner,
	InvalidSource,
	InvalidTarget,
	ForbiddenTarget,
	SameTeam,
	Dead,
	Blocked,
	Cooldown,
	NotEnoughResource,
	InvalidHandle,
	InvalidDefinition,
	SlotFull,
	StaleRequest,
	Rejected
~~~

#### EARResourceChangeReason

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:114>)

~~~cpp
Damage,
	Restore,
	Consume,
	Regeneration,
	MaxChanged,
	Other
~~~

#### EARResourceType

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:102>)

~~~cpp
Health,
	Shield,
	Stamina,
	Mana,
	Groggy,
	/** Used by affordability nodes when no resource is missing. */
	None
~~~

#### EARShieldLifetimeFilter

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:180>)

~~~cpp
All,
	TimedOnly,
	PermanentOnly
~~~

#### EARStatModifierOperation

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:41>)

~~~cpp
Flat,
	AdditivePercent,
	Multiplicative,
	IndependentDamageReduction
~~~

#### EARStatType

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Core/ARFoundationTypes.h:50>)

~~~cpp
MaxHealth,
	RecoveryPower,
	PhysicalDefense,
	FireDefense,
	MagicDefense,
	OverallDamageReduction,
	PhysicalDamageReduction,
	FireDamageReduction,
	MagicDamageReduction,
	Evasion,
	AttackPower,
	SpellPower,
	StaggerPower,
	MoveSpeed,
	AttackSpeed,
	Range,
	CriticalChance,
	CriticalDamage,
	DefensePenetration,
	Absorption,
	OverallDamageAmplification,
	DirectDamageAmplification,
	DamageOverTimeAmplification,
	PhysicalDamageAmplification,
	FireDamageAmplification,
	MagicDamageAmplification,
	GroggyDamageAmplification,
	PhysicalDamageTakenIncrease,
	FireDamageTakenIncrease,
	MagicDamageTakenIncrease,
	Tenacity,
	StaggerResistance,
	MaxGroggy,
	GroggyRecoveryDelay,
	GroggyRecoveryPerSecond,
	Luck,
	MaxStamina,
	StaminaRecoveryPerSecond,
	StaminaRecoveryDelay,
	RollStaminaCost,
	RollDistance,
	RollEvasionDuration,
	MaxMana,
	ManaRecoveryPerSecond,
	MaxConsumableSlots,
	CooldownReduction,
	Count UMETA(Hidden)
~~~

#### EARUIScreen

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/UI/ARUITypes.h:9>)

~~~cpp
None,
	Inventory,
	EvolutionSelection,
	Shop,
	Dialogue,
	Menu,
	Custom
~~~


#### EARRollDirectionMode

[원본 선언](<C:/Users/ghksd/Desktop/Action_RogLike/Prototype_Rog_Like_Game_Dev/Source/Action_RogueLike/Public/Foundation/Characters/ARPlayerCharacter.h:21>)

~~~cpp
MovementOrMouse,
MouseOnly
~~~
### 7.7 함수 목록에 포함되지 않는 기반 기능

- ARGameMode와 ARBaseEnemy는 기반 클래스이지만 자체 추가 Blueprint 함수 노드는 없다. 상속 기능을 사용한다.
- TwinStick StateTree의 GetPlayer 네이티브 태스크는 일반 UFUNCTION 노드가 아니다. 기존 템플릿 AI용 태스크다.
- 일반 엔진의 SpawnActor, GetWorldSubsystem, CreateWidget, AddToViewport, Bind Event, Make/Break Struct 등은 프로젝트가 새로 추가한 함수가 아니므로 함수 개수에서 제외했다.
- SetBaseStat, 픽업 AssignItemDefinition/AssignConsumableDefinition, 카메라 SetCameraAnchor 등은 C++ 함수이며 일반 BP 호출 목록에 없다.

### 7.8 테스트 플레이어의 대시 입력

기본 RollAction은 현재 이동 입력이 있으면 8방향 정규화, 없으면 조준 방향으로 대시한다. MouseRollAction에 연결된 IA_Roll_v2는 이동키와 관계없이 조준 방향으로 대시한다. RollDirectionMode = MouseOnly로 설정하면 기본 RollAction도 항상 조준 방향을 사용한다. TryStartRoll(true)는 BP에서 같은 마우스 전용 경로를 호출한다.

현재 테스트 BP의 RollDuration은 0.20초이며 RootMotionSource가 충돌 인식 대시 속도를 유지한다. IA_Move는 Axis2D / Cumulative다. 자세한 IMC 교체 방법은 FOUNDATION_BLUEPRINT_PLAN.md의 최신 테스트 플레이어 대시 연결 절을 따른다.

### 7.9 대시 쿨타임과 카메라 Details

`RollCooldown`은 플레이어 BP의 Class Defaults > AR|Roll에서 수정한다. 기본값은 0.50초이고 0이면 사용하지 않는다. 정상 종료나 취소 시 대시 이동 소스가 정리될 때 시작하며 두 입력 경로가 공유한다. `GetRollCooldownRemaining`은 남은 초를 반환하는 BP 조회 노드다.

카메라 반응은 플레이어 BP의 `CameraFollowComponent` > Details > AR|Camera에 있는 `BaseFollowSpeed`, `AccelerationStartDistance`, `DistanceSpeedMultiplier`, `MaximumFollowSpeed`, `SnapDistance`, `CameraHeight`로 조절한다. 처음 네 값은 거리 기반 보간 반응을 정하고, SnapDistance 이상의 XY 간격에서는 즉시 목표 위치로 이동한다. 보간 반응은 거리/초 속도가 아니다.
