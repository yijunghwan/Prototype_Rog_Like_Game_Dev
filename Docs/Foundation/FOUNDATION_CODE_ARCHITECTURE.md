# Foundation 코드 아키텍처 설계서

> **상태:** 구현 전 코드 설계서  
> **상위 문서:** `FOUNDATION_SYSTEM_PLAN.md`  
> **Blueprint 계약:** `FOUNDATION_BLUEPRINT_PLAN.md`  
> **목적:** 기존 전투 시스템 계획을 실제 Unreal C++ 파일·컴포넌트·Data Asset·블루프린트 경계로 옮기기 위한 구현 청사진이다. 이 문서는 아직 코드를 작성하지 않는다.

---

## 1. 역할과 확정 전제

기존 계획서는 **게임 규칙이 무엇인가**를 정의한다. 이 문서는 그 규칙을 구현할 때 어떤 클래스가 무엇을 소유하고, 블루프린트가 어떤 공개 함수를 호출해야 하는지를 정의한다. 기존 문서의 간단한 `Source/Foundation` 폴더 그림은 방향 설명으로 남기되, 실제 파일 구조는 이 문서를 기준으로 한다.

- Unreal 월드를 사용하되 화면 표현은 **Paper2D, 32×32 기본 캐릭터·타일 도트 그래픽, 정사영(Orthographic) 탑뷰**다. 무기 궤적·대형 마법·폭발은 32×32 캔버스에 제한하지 않는다.
- 초기 Foundation은 **싱글플레이 전용**이며 Replication/RPC를 구현하지 않는다.
- 초기에는 게임 모듈 `Action_RogueLike` 하나 안에서 구현한다. 실제로 분리 필요성이 생기기 전에는 Foundation 플러그인이나 별도 모듈을 만들지 않는다.
- C++는 최종 판정과 상태를 소유한다. Blueprint는 공격 형태, 히트박스, 투사체, 플립북, VFX/SFX, 개별 조건을 만든다.
- Foundation은 특정 무기·유물·적·보스의 고유 이름이나 예외를 모른다.
- 초기 프로토타입에는 Gameplay Ability System을 도입하지 않는다. 합의된 `UARActionComponent` 기반의 자체 Action Handle 체계를 사용한다.

## 2. 유지보수 원칙

### 2.1 하나의 사실에는 한 소유자만 둔다

| 사실 | 유일한 C++ 소유자 | 외부 코드가 할 수 있는 일 |
|---|---|---|
| 최종 스탯 | `UARStatsComponent` | 수정치 요청·읽기·이벤트 구독 |
| 체력·보호막·사망 | `UARHealthComponent` | 회복/보호막/피해 요청·이벤트 구독 |
| MP·스태미나 | `UARManaComponent`, `UARStaminaComponent` | 소비/회복 요청·읽기 |
| 상태이상 | `UARStatusEffectComponent` | 적용/해제 요청·상태 읽기 |
| 경직·슈퍼아머·그로기 | `UARStaggerComponent` | 경직 요청·이벤트 구독 |
| 행동과 취소 | `UARActionComponent` | Handle로 시작/종료/취소 요청 |
| 이동과 이동 잠금 | `UARMovementControlComponent` | 기본/액션 이동 요청·잠금 요청 |
| 무기·유물 장착 | `UARLoadoutComponent` | 획득/제거/스킬 입력 요청 |
| 소모품 슬롯 | `UARConsumableComponent` | 획득/사용/드롭 요청 |
| 피해 해결·DOT | `UARCombatSubsystem` | 피해/DOT 요청 |

Blueprint와 다른 Component는 `CurrentHealth`, `FinalAttackPower`, `bIsStunned` 등을 직접 대입하지 않는다. 공통 상태를 바꾸려면 이 문서의 공개 함수만 사용한다.

### 2.2 C++는 판정, Blueprint는 콘텐츠

**C++의 책임**

- 팀·피격 대상 검증, 피해 공식, 회피/필중, 방어력, 보호막, 사망
- 스탯의 고정/합연산/곱연산/만료/출처별 제거
- MP·스태미나 소비와 회복 대기, 강인함 기반 상태이상 지속시간
- 경직·그로기·슈퍼아머·행동 취소
- 장착 한도, 아이템 인스턴스 생명주기, 스킬 입력 우선순위와 자원 원자 소비
- HUD가 구독할 데이터와 이벤트

**Blueprint의 책임**

- 베기 범위, 투사체, 폭발/장판, 플립북, 피격 위치, 카메라 연출, VFX/SFX
- 무기·유물·소모품의 고유 조건부 효과
- 적 AI·패턴·보스 그로기 연출. 단 이동과 공격은 Foundation 통로를 이용

### 2.3 Data Asset은 정보, Runtime Instance는 상태

- Definition Data Asset은 읽기 전용이다. 쿨다운, 스택, 남은 시간, 장착 주인을 저장하지 않는다.
- 런타임 `UObject` Instance만 스택, 쿨다운, 이벤트 구독 Handle, 자기 수정치 Handle을 저장한다.
- 아이템을 버리고 다시 획득하면 새 Instance를 만든다. 기존 런타임 상태는 남지 않는다.

### 2.4 이벤트 중심으로 만들고 Tick을 최소화한다

- HUD는 매 프레임 값을 읽지 않고 Component 변경 이벤트를 구독한다.
- 스탯은 수정치 변경 시에만 재계산 후 캐시한다.
- DOT는 각 Actor Tick이 아니라 World 단위 `UARCombatSubsystem` 하나가 처리한다.
- 만료 항목은 활성 항목이 있을 때만 최소 업데이트하거나 다음 만료 시간에 정리한다.
- 카메라 추적만 활성 플레이어 C++에서 보간한다. Blueprint Tick으로 카메라 위치를 바꾸지 않는다.

---

## 3. 실제 폴더 구조

`Public`에는 다른 C++/콘텐츠가 사용해야 하는 선언만 둔다. 계산 보조와 구현은 `Private`에 둔다.

```text
Action_RogueLike/
├─ Source/Action_RogueLike/
│  ├─ Action_RogueLike.Build.cs
│  ├─ Public/Foundation/
│  │  ├─ Core/
│  │  │  ├─ ARFoundationTypes.h
│  │  │  ├─ ARGameplayTags.h
│  │  │  ├─ ARResultTypes.h
│  │  │  └─ ARLogChannels.h
│  │  ├─ Interfaces/
│  │  │  ├─ ARCombatTargetInterface.h
│  │  │  └─ ARInteractableInterface.h
│  │  ├─ Characters/
│  │  │  ├─ ARBaseCharacter.h
│  │  │  ├─ ARPlayerCharacter.h
│  │  │  └─ ARBaseEnemy.h
│  │  ├─ Player/
│  │  │  ├─ ARPlayerController.h
│  │  │  └─ ARGameMode.h
│  │  ├─ Components/
│  │  │  ├─ ARStatsComponent.h
│  │  │  ├─ ARHealthComponent.h
│  │  │  ├─ ARStaminaComponent.h
│  │  │  ├─ ARManaComponent.h
│  │  │  ├─ ARStatusEffectComponent.h
│  │  │  ├─ ARStaggerComponent.h
│  │  │  ├─ ARActionComponent.h
│  │  │  ├─ ARMovementControlComponent.h
│  │  │  ├─ ARCombatSourceComponent.h
│  │  │  ├─ ARLoadoutComponent.h
│  │  │  ├─ ARConsumableComponent.h
│  │  │  ├─ ARCameraFollowComponent.h
│  │  │  ├─ ARInteractionComponent.h
│  │  │  └─ ARUIManagerComponent.h
│  │  ├─ Combat/
│  │  │  ├─ ARCombatSubsystem.h
│  │  │  ├─ ARCombatBlueprintLibrary.h
│  │  │  ├─ ARActionHitboxActor.h
│  │  │  ├─ ARDamageTypes.h
│  │  │  ├─ ARDotTypes.h
│  │  │  └─ ARStaggerTypes.h
│  │  ├─ Actions/
│  │  │  ├─ ARActionTypes.h
│  │  │  ├─ ARActionBlueprintLibrary.h
│  │  │  └─ ARAsyncActionDelay.h
│  │  ├─ Items/
│  │  │  ├─ ARLoadoutItemDefinition.h
│  │  │  ├─ ARWeaponDefinition.h
│  │  │  ├─ ARActiveRelicDefinition.h
│  │  │  ├─ ARPassiveRelicDefinition.h
│  │  │  ├─ ARConsumableDefinition.h
│  │  │  ├─ ARLoadoutItemInstance.h
│  │  │  ├─ ARConsumableInstance.h
│  │  │  └─ ARItemTypes.h
│  │  ├─ Status/
│  │  │  └─ ARStatusEffectDefinition.h
│  │  ├─ Interaction/
│  │  │  ├─ ARInteractionTypes.h
│  │  │  └─ ARInteractionBlueprintLibrary.h
│  │  └─ UI/
│  │     └─ ARUITypes.h
│  └─ Private/Foundation/
│     ├─ Core/ Characters/ Components/ Actions/ Items/ Interaction/ UI/
│     └─ Combat/
│        ├─ ARCombatSubsystem.cpp
│        ├─ ARActionHitboxActor.cpp
│        ├─ ARDamageResolver.h/.cpp
│        └─ ARDotScheduler.h/.cpp
├─ Content/Game/
│  ├─ Foundation/
│  │  ├─ Blueprints/Characters/ Pickups/ World/
│  │  ├─ Data/Items/ StatusEffects/ Camera/
│  │  ├─ Input/
│  │  ├─ UI/
│  │  └─ Test/
│  ├─ ArtShared/                 ← 여러 콘텐츠가 함께 쓰는 팔레트·머티리얼·공용 FX
│  ├─ Characters/                ← Player/Enemies/[콘텐츠명]/Art·Blueprints·Data
│  ├─ Weapons/                   ← [무기명]/Data·Runtime·Art·Projectiles
│  ├─ Relics/                    ← Active/Passive/[유물명]/Data·Runtime·Art
│  ├─ Consumables/               ← [소모품명]/Data·Runtime·Art
│  ├─ World/                     ← Maps·Tilesets·Props·EnvironmentFX
│  └─ UI/                        ← 게임 고유 Widget·UI 아트
├─ ArtSource/                    ← Aseprite/PSD와 원본 PNG. Content에 임포트하지 않음
│  ├─ Shared/ Characters/ Weapons/ Relics/ Consumables/ World/ UI/
└─ Docs/Foundation/
   ├─ FOUNDATION_SYSTEM_PLAN.md
   ├─ FOUNDATION_CODE_ARCHITECTURE.md
   ├─ FOUNDATION_BLUEPRINT_PLAN.md
   └─ FOUNDATION_ENEMY_CONTENT_GUIDE.md  ← 기반 구현 후 작성
```

### 폴더 규칙

- `Foundation` C++에는 특정 콘텐츠 이름을 넣지 않는다.
- 헤더는 전방 선언을 우선하고, 무거운 include는 `.cpp`에 둔다.
- `ARDamageResolver`는 `UObject`가 아닌 순수 C++ 계산기다. Blueprint가 피해 공식을 우회할 수 없고 수식 자동화 테스트가 쉽다.
- Data Asset, Runtime BP, Pickup BP, Widget은 `Content`에만 둔다.
- 테스트 맵과 더미 에셋은 `Content/Game/Foundation/Test`에만 둔다.

### Paper2D 도트 아트 저장 기준

도트 그래픽은 두 위치로 나눈다.

| 위치 | 넣는 것 | 목적 |
|---|---|---|
| `ArtSource/` | Aseprite/PSD 원본, 생성 직후 PNG, 레퍼런스, 작업 중 시트 | 언리얼이 임포트하지 않는 원본 작업 보관소 |
| `Content/Game/` | 임포트된 Texture, Paper Sprite, Paper Flipbook, Material, 실제 UI 아이콘 | 언리얼이 참조하고 패키징하는 게임 에셋 |

기본 캐릭터 프레임과 월드 타일이 32×32이라는 뜻이지, 모든 이미지 파일이 32×32이어야 한다는 뜻은 아니다. 예를 들어 8프레임 걷기 애니메이션은 `256×32` PNG 스프라이트 시트 하나로 저장하고, 언리얼에서 Texture → Paper Sprite들 → Flipbook 순으로 만든다. 긴 무기, 검 궤적, 폭발, 대형 마법은 별도 Sprite/Flipbook 레이어와 더 큰 캔버스를 허용한다. 같은 캐릭터/무기/유물이 쓰는 시트·스프라이트·플립북은 해당 콘텐츠의 `Art` 폴더 안에 함께 둔다.

초기 픽셀·타일 좌표 규격은 다음으로 고정한다.

- 원본 타일과 기본 캐릭터 프레임은 32×32 픽셀이다.
- 초기 `PixelsPerUnrealUnit`은 `0.5`로 두므로 32픽셀 타일 한 칸은 `64×64 UU`다.
- 게임플레이 이동 평면은 `XY`, 높이·표시 계층은 `Z`다. 기본 방향은 `W=+X`, `S=-X`, `D=+Y`, `A=-Y`다.
- 캐릭터는 타일 한 칸씩 스냅하지 않고 XY 실수 좌표로 연속 이동한다. 타일 좌표가 필요할 때만 `Floor((WorldPosition - MapOrigin) / 64)`로 변환한다.
- 방은 개별 타일 Actor의 집합이 아니라 Paper TileMap과 단순 충돌을 가진 방 모듈로 제작한다. 방 크기와 출입구 위치는 64 UU 격자 배수에 맞춘다.

전 프로젝트가 함께 쓰는 팔레트, 공용 머티리얼, 공통 충격/피격 FX만 `Content/Game/ArtShared`에 둔다. 재사용 가능성이 막연하다는 이유로 모든 Texture를 Shared에 넣지 않는다.

---

## 4. 공통 기반 파일과 Build 설정

### `Action_RogueLike.Build.cs`

Foundation 공개 헤더에 필요한 기본 의존성은 `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `GameplayTags`, `Paper2D`, `UMG`다. 현재 프로젝트의 TopDown/Strategy/TwinStick 템플릿 코드가 `AIModule`, `NavigationSystem`, `StateTreeModule`, `GameplayStateTreeModule`, `Niagara`, `Slate`를 이미 사용하므로 새 Foundation 테스트 맵이 정상 동작하고 템플릿 제거 여부를 결정하기 전에는 이 의존성을 제거하지 않는다. 템플릿 정리 후 공개 헤더에서 쓰지 않는 모듈은 `PrivateDependencyModuleNames`로 이동하거나 제거한다. `.uproject`에도 Paper2D를 명시적으로 활성화한다.

### `ARFoundationTypes.h`

공통 enum/struct만 둔다. 예시는 다음과 같다.

- `EARCombatTeam`: Player, Enemy, Environment
- `EARDamageDelivery`: Direct, DamageOverTime
- `EARDamageAttribute`: Physical, Fire, Magic, Void
- `EARStatType`, `EARStatModifierOperation`, `EARModifierSourceCategory`
- `EARStatusEffectType`: Stun, Root 및 이후 확장 값
- `EARActionCancelReason`: Stagger, Stun, Roll, MovementInput, Death, Manual
- `EARRequestResult`: Success, InvalidTarget, Cooldown, NotEnoughResource, Rejected 등

### `ARGameplayTags.h/.cpp`

반복되는 문자열은 Native Gameplay Tag로 선언한다.

```text
Combat.Team.Player / Enemy / Environment
Damage.Attribute.Physical / Fire / Magic / Void
Damage.Delivery.Direct / DOT
Input.Skill.Primary / Skill.1 / Skill.2 ...
Status.Stun / Status.Root
Item.Type.Weapon / ActiveRelic / PassiveRelic / Consumable
```

무기 고유 태그와 유물 고유 태그는 Data Asset에서 추가한다. Foundation은 특정 아이템 이름을 태그로 선언하지 않는다.

### `ARResultTypes.h`와 `ARLogChannels.h`

요청 함수는 Boolean만 돌려주지 않는다. 실패 원인과 선택 UI용 정보를 가진 `FARRequestResult` 또는 전투 전용 결과 구조체를 반환한다.

로그 채널은 `LogARFoundation`, `LogARCombat`, `LogARItems`, `LogARAction`으로 나눈다. 잘못된 DOT 갱신, 종료된 Action Handle 사용, 존재하지 않는 Modifier 제거는 개발 경고로 기록한다. MP 부족 같은 정상 플레이 실패는 경고로 기록하지 않는다.

---

## 5. Character와 Paper2D 구조

### `AARBaseCharacter`

플레이어/적의 공유 부모다. 입력, 카메라, 인벤토리, 장착 슬롯은 넣지 않는다.

**생성하는 Component**

- `UARStatsComponent`, `UARHealthComponent`, `UARStatusEffectComponent`
- `UARStaggerComponent`, `UARActionComponent`, `UARMovementControlComponent`

**공개 기능 후보**

| 함수/이벤트 | 역할 |
|---|---|
| `GetCombatTeam()` | 캐릭터 팀 반환 |
| `HandleCharacterDeath()` | Health 사망 이벤트를 받아 행동·이동 정리 |
| `OnCharacterDeath` | 캐릭터 BP가 연출을 구독하는 이벤트 |
| `GetAimDirection()` | 현재 월드 조준 방향 읽기 |

`IARCombatTargetInterface`를 네이티브로 구현한다. Blueprint에서 인터페이스를 새로 구현하지 않고 `AARBaseCharacter`의 BP 자식을 사용한다. 피해 계산은 인터페이스와 Component로 대상 유효성을 확인한다.

### `AARPlayerCharacter`

Player 전용 Component는 `UARStaminaComponent`, `UARManaComponent`, `UARLoadoutComponent`, `UARConsumableComponent`, `UARCameraFollowComponent`다. 절대 월드 위치를 사용하는 `CameraAnchor` Scene Component와 `UCameraComponent`를 두고 Camera Projection은 Orthographic으로 설정한다. 카메라 지연·가속은 `UARCameraFollowComponent`가 담당한다.

| 함수 | 책임 |
|---|---|
| `SetupPlayerInputComponent` | IMC의 액션을 C++ 함수에 한 번 바인딩 |
| `HandleMove` | WASD를 Movement Component로 전달 |
| `UpdateAimFromCursor` | 커서 좌표를 게임 평면에 투영해 Aim Direction 갱신 |
| `HandleRollPressed` | 스태미나·행동 상태를 확인 후 Roll Action 요청 |
| `HandleInteractPressed` | 가장 가까운 `IARInteractable` 대상에 요청 |
| `HandleSkillInput(InputTag)` | Loadout Component에 논리 입력 태그 전달 |
| `HandleUseConsumableSlot(Index)` | 숫자 슬롯의 즉발 사용 요청 |
| `SetGameplayInputBlocked` | 인벤토리/UI 때 게임플레이 입력 전체 차단 |

### `AARBaseEnemy`

Enemy 팀을 가진 최소 자식이다. AI와 공격 패턴은 넣지 않는다. 적 BP/후속 C++는 반드시 `UARMovementControlComponent`로 이동하고 `UARActionComponent` Handle로 공격한다.

### `AARPlayerController`, `AARGameMode`, UI·상호작용

`AARPlayerController`는 `IMC_Player` 등록, 마우스 위치의 월드 평면 투영, 게임/UI 입력 모드 전환을 소유한다. `AARPlayerCharacter`는 Controller가 전달한 이동·조준·논리 Input Tag를 실제 Component 요청으로 바꾼다. `UARUIManagerComponent`는 HUD, 인벤토리, 진화 선택 UI를 한 번에 하나만 열고 Player 사망 시 열린 UI와 대기 Token을 취소한다.

`UARInteractionComponent`는 Player 주변의 `ARInteractable` 채널을 조회해 거리, 우선순위, 선택적 시야 검사를 통과한 현재 후보를 보관한다. F 입력은 이 Component의 `TryInteract`만 호출한다. 상점·픽업·NPC는 `IARInteractable`을 구현하고, UI 생성은 직접 하지 않고 UI Manager에 요청한다.

`AARGameMode`는 새 Foundation Player/Controller를 기본 클래스로 지정한다. 기존 `/Game/TopDown` 맵과 클릭 이동 Controller는 Foundation Test Map이 정상 동작한 뒤에만 기본 설정에서 교체한다.

### Paper2D 및 카메라

- Character BP는 Paper2D Flipbook/Sprite Component를 붙인다.
- C++는 이동 방향, Aim Direction, Action 시작/종료, 피격/사망 이벤트만 제공한다. 어떤 Flipbook을 재생할지는 BP가 정한다.
- 2D 표현은 피해/충돌 계산을 화면 좌표로 바꾸라는 뜻이 아니다. 판정은 Unreal 월드 좌표에서 실행한다.
- 텍스처는 Nearest 필터링을 사용하고, 정사영 폭과 목표 해상도는 정수 배율이 유지되도록 조정한다.

초기 좌표·임포트 규약은 다음으로 고정한다.

- 게임플레이 이동 평면은 `XY`, 높이와 표시 우선순위는 `Z`다.
- Sprite/Flipbook은 정사영 카메라를 향하도록 공통 BP 부모에서 회전을 고정한다.
- 원본 기본 프레임·타일은 32×32이며 Texture는 Nearest, Mipmap 비활성, UI가 아닌 Sprite용 압축 설정을 사용한다.
- `PixelsPerUnrealUnit=0.5`, 타일 한 칸 `64 UU`를 초기 기준으로 사용한다. 목표 내부 해상도와 기준 `OrthoWidth`는 `PixelScaleProfile` 데이터로 한 곳에서 관리하고 Test Map에서 1픽셀 이동 안정성을 검증한다.
- 충돌은 Sprite의 불투명 픽셀을 사용하지 않고 Capsule/Box/Circle 같은 단순 Shape와 Query 전용 Hitbox를 사용한다.
- 표시 순서는 Z 계층과 Translucency Sort 규약으로 통일하고 개별 Sprite가 임의의 큰 Sort Priority를 사용하지 않는다.

프로젝트 충돌 채널은 `ARPlayer`, `AREnemy`, `ARInteractable`, `ARPlayerHitbox`, `AREnemyHitbox`, `ARProjectile`, `ARWorldObstacle`을 예약한다. Overlap은 후보 필터일 뿐이며 최종 팀·생존·피격 가능 판정은 Combat Subsystem이 다시 수행한다.

`UARCameraFollowComponent` 설정은 `BaseFollowSpeed`, `AccelerationStartDistance`, `MaxFollowSpeed`, `MaxLagDistance`, `OrthoWidth`, `CameraRotation`, `PixelScaleProfile`로 둔다. 기본 추적은 C++가 소유하고, 흔들림/줌은 BP가 연출 요청만 한다.

---

## 6. Component별 파일과 공개 API

이름은 구현 전 확정안이다. 선언 시 역할에 따라 `BlueprintCallable`, `BlueprintPure`, `BlueprintAssignable`를 붙인다. 캐시/정리 함수는 private로 유지한다.

### `UARStatsComponent`

**파일:** `Components/ARStatsComponent.h/.cpp`  
**책임:** 기본값 + 고정 + 합연산% + 곱연산% 최종 스탯과 시간 제한 Modifier.

| 공개 함수 | 역할 |
|---|---|
| `AddStatModifier(Spec)` | 수정치 등록 후 `FARStatModifierHandle` 반환 |
| `RemoveStatModifier(Handle)` | 정확한 한 수정치 제거 |
| `RemoveModifiers(Category, SourceId)` | Weapon/Relic/Buff/Other 또는 특정 출처 제거 |
| `ClearModifiers(Category)` | All 또는 한 분류 전체 초기화 |
| `GetFinalStat(StatType)` | 캐시된 최종값 반환 |
| `GetModifierRemainingTime(Handle)` | 영구/남은 시간 상태 반환 |
| `GetModifiersBySource(Category, SourceId)` | 존재 여부, 중첩 수, 가장 긴 남은 시간 반환 |
| `RemoveOneModifierStack(Category, SourceId, Policy)` | 최신/가장 오래된 중첩 하나 제거 |
| `GetStatBreakdown(StatType)` | 인벤토리/디버그용 읽기 전용 분해 정보 |

이벤트는 `OnFinalStatChanged`, `OnStatModifiersChanged`, `OnModifierAdded`, `OnModifierRemoved`다. Modifier Spec에는 출처 분류, 출처 ID, 표시 이름, 지속시간, 스탯, 계산 방식을 항상 넣는다. Buff는 같은 SourceId여도 Handle 기준으로 독립 중첩한다. `DisplayName`은 조회 키로 쓰지 않는다.

전체 피해 감소율 Modifier의 `bGuaranteeInvulnerability`, 회피력 Modifier의 `bGuaranteeEvasion`은 별도 관리형 Token을 함께 생성한다. 단순 수치 `+100`으로 구현하지 않으며 원 Modifier가 제거·만료되면 연결 Token도 제거한다. Action 귀속 Modifier는 Action 종료·취소 시 Handle로 정리한다.

### `UARHealthComponent`

**파일:** `Components/ARHealthComponent.h/.cpp`  
**책임:** 체력, LIFO 보호막, 회복, 사망. 피해 공식을 직접 계산하지 않고 Combat Subsystem 결과만 적용한다.

| 공개 함수 | 역할 |
|---|---|
| `CanAcceptResolvedDamage` | 사망 또는 명시적 시스템 비활성 상태 확인. 일반 무적은 여기서 거부하지 않음 |
| `ApplyResolvedDamage(Result)` | 보호막→체력 반영 |
| `RestoreHealth(Amount, Source)` | 최대 체력을 넘기지 않는 회복 |
| `ApplyShield(Spec)` | 수치·지속시간·출처가 있는 보호막 스택 추가 |
| `RemoveShield` | Handle/출처/전체 기준 제거 |
| `GetCurrentHealth`, `GetCurrentShield`, `IsDead` | 읽기 전용 조회 |

이벤트는 `OnHealthChanged`, `OnShieldChanged`, `OnDamageApplied`, `OnDeath`다. 체력/보호막 직접 Set 노드는 만들지 않는다.

일반 무적은 Request 속성과 공허 여부가 필요한 Combat Resolver에서 처리한다. Health Component가 Request를 보기 전에 일반 무적만으로 요청을 거부하면 공허 피해까지 잘못 막히므로 금지한다. 최대 체력 감소 시 현재 체력은 새 최대값으로 제한하고, 최대 체력 증가만으로 현재 체력을 회복하지 않는다.

### `UARStaminaComponent`, `UARManaComponent`

**파일:** `Components/ARStaminaComponent.h/.cpp`, `Components/ARManaComponent.h/.cpp`  
**책임:** 현재 자원, 소비, 회복, UI 이벤트.

| 공개 함수 | 역할 |
|---|---|
| `CanAfford(Amount)` | 소비 가능 여부 |
| `TryConsume(Amount, Source)` | 성공할 때만 실제 소비 |
| `Restore(Amount, Source)` | 최대값까지 회복 |
| `GetCurrent`, `GetMax`, `GetRatio` | HUD/BP 조회 |
| `OnResourceChanged` | UI/아이템 구독 이벤트 |

스태미나는 마지막 **성공 소비** 뒤 회복 대기시간이 지나야 재생한다. MP는 즉시 초당 재생하며 소수점을 누적한다. 스킬이 두 자원을 함께 요구하면 Loadout 실행 경로가 먼저 둘 다 예약하고, 가능할 때만 함께 소비한다.

최대 스태미나·MP 감소 시 현재값은 즉시 새 최대값으로 제한하고, 최대값 증가만으로 현재값을 채우지 않는다. 최대값 변화도 `OnResourceChanged` 원인 `MaxChanged`로 전달한다.

### `UARStatusEffectComponent`

**파일:** `Components/ARStatusEffectComponent.h/.cpp`  
**책임:** 상태 인스턴스, 강인함 기반 지속시간, 같은 상태 갱신, 이동/행동 잠금 연결.

정적 규칙은 `Status/ARStatusEffectDefinition.h`의 `UARStatusEffectDefinition`이 소유한다. Definition은 Primary Asset ID, Status Tag, 표시 정보, 기본 지속시간, 갱신 정책, 이동·구르기·Skill Group 제한과 Action 취소 사유를 가진다. 런타임 남은 시간·SourceId·Handle은 Component의 상태 인스턴스만 소유한다.

| 공개 함수 | 역할 |
|---|---|
| `ApplyStatusEffect(Request)` | 실제 지속시간과 Handle 반환 |
| `RemoveStatusEffect(Handle)` | 한 인스턴스 제거 |
| `RemoveStatusEffectsBySource(SourceId)` | 출처가 준 상태 전체 제거 |
| `HasStatus(Tag)`, `GetStatusRemainingTime` | BP/AI 읽기 |
| `ClearAllStatusEffects` | 사망/리셋 때 명시적 정리 |

Stun은 Action 취소와 실제 이동 잠금을, Root는 이동/구르기 잠금을 요청한다. Character에 쓰기 가능한 `bIsStunned`, `bIsRooted`는 만들지 않는다.

### `UARStaggerComponent`

**파일:** `Components/ARStaggerComponent.h/.cpp`  
**책임:** 일반 경직, 경직 면역, 슈퍼아머, 선택형 그로기 게이지.

| 공개 함수 | 역할 |
|---|---|
| `ApplyStaggerAndGroggyDamage(Request)` | 기본 경직/그로기 수치를 별도 공식으로 해결 |
| `AddSuperArmor(Spec)` | 출처와 지속시간을 가진 슈퍼아머 등록 |
| `RemoveSuperArmor`, `RemoveSuperArmorBySource` | 슈퍼아머 제거 |
| `IsSuperArmorActive` | 읽기 전용 확인 |
| `GetCurrentGroggy`, `GetMaxGroggy` | UI/보스 BP 읽기 |

이벤트는 `OnStaggered`, `OnGroggyChanged`, `OnGroggyGaugeDepleted`, `OnSuperArmorChanged`다. 그로기 0은 자동 기절이 아니라 이벤트만 발생시킨다.

### `UARMovementControlComponent`

**파일:** `Components/ARMovementControlComponent.h/.cpp`  
**책임:** WASD, 적 추적, Action 돌진 등 모든 이동 통로와 이동 잠금 토큰.

| 공개 함수 | 역할 |
|---|---|
| `RequestBasicMove(Direction)` | 플레이어/AI 기본 이동 요청 |
| `RequestActionMove(ActionHandle, Spec)` | 돌진/백스텝 같은 Action 소유 이동 |
| `AcquireMovementLock(SourceId, LockType)` | Root/Stun/Action 이유별 잠금 Handle 생성 |
| `ReleaseMovementLock(Handle)` | 정확한 잠금 해제 |
| `CanBasicMove`, `CanMoveAtAll` | 기본 이동 / 전체 이동 가능 여부 |

액션 소유 잠금은 Action 종료/취소에서 자동 해제한다. 적 개발자는 장시간 `SetActorLocation`이나 독립 Timeline으로 이동하지 않는다.

### `UARActionComponent`

**파일:** `Components/ARActionComponent.h/.cpp`, `Actions/ARActionTypes.h`, `Actions/ARAsyncActionDelay.h/.cpp`  
**책임:** 행동의 시작/종료/취소와 Skill Group, Action 소유 Delay·Hitbox·이동 잠금·임시 수정치·슈퍼아머 정리.

| 공개 함수/노드 | 역할 |
|---|---|
| `TryStartAction(Request)` | 가능하면 `FARActionHandle` 반환 |
| `CanStartAction` | 실패 이유를 포함한 사전 검사 |
| `EndAction(Handle)` | 정상 종료와 Action 소유 항목 정리 |
| `CancelAction(Handle, Reason)` | 강제 취소와 동일 정리 |
| `CancelActionsByReason` | Stun/Death처럼 다수 행동 취소 |
| `SetActionCancelRule` | 경직/기절/구르기/기본이동 취소 여부 |
| `SetActionRollBlocked` | Action 유지 중 구르기 불가 요청 |
| `SetActionBasicMovementBlocked` | Action 유지 중 WASD 이동 불가 요청 |
| `Action Delay` | Handle 취소 시 자동 종료되는 지연 노드 |
| `Register Action Hitbox` | Handle 귀속 히트박스 등록 |
| `Apply Action Stat Modifier` | 종료/취소 시 자동 제거되는 Modifier |
| `Apply Action Super Armor` | 종료/취소 시 자동 제거되는 슈퍼아머 Token |

`FARActionHandle`에는 Owner의 약한 참조와 단조 증가 Serial을 넣어, 오래된 Handle이 새 행동을 종료하지 못하게 한다.

한 번의 스킬 입력은 `FARSkillGroupHandle` 하나를 만들고 참여 스킬마다 별도 `FARActionHandle`을 만든다. 모든 정적/고유 조건 검사가 끝난 뒤 자원 소비, 쿨다운 시작, 참가 Action 생성을 한 트랜잭션으로 Commit한다. Commit 후 취소는 기본적으로 자원과 쿨다운을 환불하지 않는다. 그룹은 마지막 참가 Action이 종료될 때 끝나며, Item 제거는 그 Instance가 소유한 참가 Action만 `ItemRemoved` 사유로 취소한다.

### `UARLoadoutComponent`

**파일:** `Components/ARLoadoutComponent.h/.cpp`  
**책임:** 기존 개념의 Weapon/Relic Component를 코드상 하나로 통합한다. 세 유형의 장착/해제 생명주기, 스탯 수정치, 스킬 UI, 입력 우선순위가 같으므로 통합 관리하고, 슬롯 배열만 무기 1/액티브 2/패시브 무제한으로 나눈다.

| 공개 함수 | 역할 |
|---|---|
| `BeginLoadoutAcquisition(Definition)` | 획득 가능/교체 필요 여부와 UI용 결과 |
| `CommitLoadoutAcquisition(Token)` | 검증된 요청을 Instance 생성/장착으로 확정 |
| `CancelLoadoutAcquisition(Token)` | 선택 UI 취소 |
| `DiscardLoadoutItem(InstanceId)` | 월드 픽업 Spawn이 성공한 뒤에만 액티브/패시브 제거, 무기는 거부 |
| `GetEquippedWeapon`, `GetActiveRelics`, `GetPassiveRelics` | UI용 읽기 전용 스냅샷 |
| `HandleSkillInput(InputTag)` | 등록 스킬 후보를 수집해 Action Component의 Skill Group 실행 요청으로 전달 |
| `RequestWeaponEvolution`, `CommitWeaponEvolution` | 진화 후보/새 무기 Instance 교체 |

Loadout Component는 등록·소유권과 후보 열거의 유일한 소유자이고, Action Component는 우선순위 그룹 검사, 자원 예약·소비, 쿨다운 Commit, 참가 Action 생명주기의 유일한 소유자다. 같은 Input Tag의 스킬은 `InputPriority` 오름차순으로 검사한다. 같은 우선순위는 하나의 원자 그룹이며, 정적 쿨다운/MP/스태미나와 `CanExecuteItemSkill`을 모두 통과한 그룹만 함께 실행한다. 각 등록 스킬은 `FARRegisteredSkillHandle`로 식별하며 `SkillId` 단독으로 쿨다운을 찾지 않는다.

### `UARConsumableComponent`과 `UARCombatSourceComponent`

`UARConsumableComponent`는 가변 슬롯, 한 칸 하나, 중복 허용, 즉발 사용, 슬롯 감소 시 초과 소모품 드롭을 관리한다. 공개 함수는 `TryAcquireConsumable`, `TryUseConsumableSlot`, `GetConsumableSlots`, `SetMaxConsumableSlots`, `DropConsumableSlot`이다. 소모품은 Action을 만들지 않는다.

수동 폐기와 슬롯 감소는 `UARWorldItemDropSubsystem`을 통해 픽업을 먼저 Spawn한다. Spawn 성공 뒤에만 Runtime Instance를 해제한다. 슬롯 감소 중 Spawn이 실패하면 해당 소모품은 `bOverflowSlot`로 내부 보존하며 `RetryPendingOverflowDrops`로 다시 시도하므로 아이템이 소실되지 않는다.

`UARCombatSourceComponent`는 가시 함정·낙석·장판 같은 World Actor에 Environment 팀, 피해 출처 ID, 로그 이름을 제공한다. 환경은 공격자일 수 있지만 Combat Target은 아니다.

---

## 7. Combat 계층: 피해, DOT, 경직

### 데이터 구조

| 파일 | 핵심 구조 | 책임 |
|---|---|---|
| `ARDamageTypes.h` | `FARCombatDamageRequest`, `FARCombatDamageResult` | 직접/DOT 피해 요청과 결과 |
| `ARDotTypes.h` | `FARDamageOverTimeSpec`, `FARDotHandle` | DOT 이름, 간격, 갱신, Handle |
| `ARStaggerTypes.h` | `FARStaggerRequest`, `FARStaggerResult`, `FARSuperArmorSpec` | 경직/그로기/슈퍼아머 |

Damage Request는 공격자/환경 출처, 대상, 전달 방식, 속성, 기본 피해, 공격력/주문력 계수, 치명타/회피/증감식/방어력 무시/보호막 무시/**흡수 적용**/적중시 효과/필중 여부, 출처 ID와 표시 이름을 담는다.

경직과 그로기는 Damage Request에 숨기지 않는다. 공격 BP가 필요할 때 별도 `FARStaggerRequest`로 호출한다. 따라서 **경직력은 자동 경직을 만들지 않고, 공격이 제공한 기본 경직 피해 또는 기본 그로기 피해를 강화할 뿐이다.** 공격력·주문력도 스킬이 계수를 지정할 때만 참여하며, 계수 없는 깡 피해도 가능하다.

### `UARCombatSubsystem`

**파일:** `Combat/ARCombatSubsystem.h/.cpp`  
**종류:** `UTickableWorldSubsystem`
**책임:** 월드 단위 피해 진입점과 DOT 등록 목록의 유일한 소유자.

| 공개 함수 | 역할 |
|---|---|
| `CanDamageTarget(Request)` | 팀/대상/사망/피격 가능 여부 재검증 |
| `ApplyCombatDamage(Request)` | 단발 피해 해결 후 Result 반환 |
| `ApplyDamageOverTime(Spec)` | DOT 등록 후 Handle 반환 |
| `RemoveDamageOverTime(Handle/Source/Target)` | DOT 제거 |
| `GetActiveDotsForTarget` | 디버그/특수 UI 읽기 |

DOT 첫 틱은 `TickInterval` 뒤에 들어가고 기본 간격은 0.25초다. 프레임 지연으로 밀린 틱은 모두 처리한다. 갱신형 DOT는 같은 이름이며 기본 피해·지속시간·간격이 모두 같을 때만 남은 시간을 초기화하고, 다르면 경고 로그 후 기존 DOT를 유지한다. 독립 중첩은 별도 Handle로 처리한다.

DOT Instance는 최종 피해를 저장하지 않고 원본 Request를 보관해 매 틱 캐시된 최신 스탯으로 다시 계산한다. 공격자 참조가 사라지는 순간 마지막 유효 공격 스탯을 Snapshot하여 남은 틱을 계속 처리한다. `FARDamageOverTimeSpec`은 선택적 `FARStaggerRequestTemplate`을 가질 수 있으며, 존재하면 각 성공 틱의 Hit Context로 경직·그로기 요청을 실행한다. 활성 DOT 수와 한 프레임 밀린 틱 수가 설정 임계값을 넘으면 경고하되 합의된 틱을 버리지는 않는다.

### `AARActionHitboxActor`

**파일:** `Combat/ARActionHitboxActor.h/.cpp`  
**종류:** Action 소유의 공용 Hitbox Actor 부모  
**책임:** 충돌 범위, 유효 대상당 적중 정책, Action Handle 귀속만 관리한다. 피해 수치와 피해 공식은 소유하지 않는다.

`InitializeHitbox(ActionHandle, Instigator, HitPolicy, Lifetime)`으로 시작하고, `UARActionComponent::RegisterActionHitbox`에 등록한다. Action이 종료/취소되면 자동 제거된다. 무기/적 Blueprint 자식은 Box/Circle/Capsule 범위와 Overlap 시 어떤 Damage/Stagger Request를 만들지 구현한다.

### `FARDamageResolver`

**파일:** `Private/Foundation/Combat/ARDamageResolver.h/.cpp`  
**종류:** 외부 공개하지 않는 순수 C++ 공식 계산기.

```text
팀/대상 검증
→ 필중 또는 회피
→ 기본 피해 + 공격력/주문력 계수
→ 전체·직접/지속·속성 증폭
→ 치명타
→ 방관 후 속성 방어력
→ 대상 속성 피격 피해 증가율
→ 전체/속성 피해 감소율의 독립 곱연산
→ 내림/최소 피해 규칙
→ 보호막·체력 분배
```

공허 피해는 방어력·방관·일반 감소율·일반 무적 단계를 건너뛴다. 보호막은 `bIgnoreShield`일 때만 무시한다. Resolver는 Actor 생성, UI, VFX를 하지 않고 결과만 반환한다. 회피와 치명타는 주입된 `FRandomStream` 또는 테스트 Roll 값을 사용해 자동화 테스트에서 재현 가능해야 한다.

Combat Subsystem의 한 요청은 검증·계산·보호막/체력 반영·자원 변경 이벤트·피격/회피/차단 이벤트·공격자 OnHit·흡수·사망 이벤트 순으로 발행한다. Result의 처치 여부는 콜백 전에 확정한다. 콜백 중 새 피해가 들어오면 재귀 실행하지 않고 요청 큐에 넣어 현재 이벤트 체인이 끝난 뒤 처리한다. OnHit 추가 피해는 기본적으로 다시 OnHit을 발생시키지 않으며 설정 가능한 연쇄 깊이 경고를 둔다. Stats 변경 이벤트에서 다시 Modifier를 바꾸는 경우도 Dirty Queue로 다음 재계산 패스에 처리한다.

### Blueprint 전투 노드

`UARCombatBlueprintLibrary`는 World의 Combat Subsystem을 찾아 위임만 한다. Library 안에 공식을 중복하지 않는다.

- `Can Damage Target`
- `Apply Combat Damage`
- `Apply Damage Over Time` / `Remove Damage Over Time`
- `Apply Stagger And Groggy Damage`
- `Apply Super Armor` / `Remove Super Armor`
- `Apply Status Effect`

`Apply Combat Damage` 결과를 BP가 받아 피해 숫자·피격 VFX/SFX를 처리한다. 적중시 효과 활성 직접 피해가 성공하면 `On Damage Hit`이 발생하고, 여기서 만든 추가 피해는 기본적으로 다시 OnHit을 발생시키지 않는다.

---

## 8. Data Asset과 Runtime Instance

### Definition 계층

```text
UARLoadoutItemDefinition (추상 UPrimaryDataAsset)
├─ UARWeaponDefinition
├─ UARActiveRelicDefinition
└─ UARPassiveRelicDefinition

UARConsumableDefinition (UPrimaryDataAsset)
```

Definition의 공통 필드는 Asset Manager가 제공하는 `PrimaryAssetId`, 게임 규칙용 `DefinitionTag`, Item Type Tag, 표시 이름, 짧은/상세 설명, UI 아이콘, 기본 스탯 수정치, Skill Definitions, Runtime Behavior Class, UI State Display Definitions이다. `PrimaryAssetId`는 로드·저장, `DefinitionTag`는 규칙·검색, `FText` 이름은 표시 전용으로 역할을 분리한다. Weapon Definition에만 진화 그룹/단계/다음 후보 정보를 둔다. 사용하지 않는 빈 필드는 공통 부모에 넣지 않는다. Project Asset Manager에는 `ARLoadoutItem`, `ARConsumable`, `ARStatusEffect` 타입과 `/Game/Game/...` 스캔 경로가 등록되어 있다.

### Runtime Instance 계층

```text
UARLoadoutItemInstance (추상 Blueprintable UObject)
├─ BP_WeaponRuntime_Base
├─ BP_ActiveRelicRuntime_Base
└─ BP_PassiveRelicRuntime_Base

UARConsumableInstance (추상 Blueprintable UObject)
└─ BP_ConsumableRuntime_Base
```

Loadout/Consumable Component는 `UPROPERTY(Transient)` 슬롯·배열로 Instance를 강하게 소유하며 Instance의 Outer가 된다. Instance는 Owner Character를 약하게, 장착 중인 Definition을 `TObjectPtr<const ...>`로 강하게 참조한다. Instance는 `FGuid` Instance ID, 자기 Modifier Handle, 등록 Skill Handle, 쿨다운, UI 상태, 이벤트 구독 Handle만 소유한다. 제거와 EndPlay에서 구독을 먼저 해제하고 Component 소유 참조를 마지막에 제거한다.

| 함수/이벤트 | 성격 | 역할 |
|---|---|---|
| `InitializeInstance` | C++ 내부 | Owner/Definition/ID 설정 |
| `RegisterItem` | C++ 흐름 | 고정 스탯→스킬→구독→BP 등록 이벤트 |
| `OnItemRegistered` | BP 이벤트 | 조건부 패시브/연출 시작 |
| `CanExecuteItemSkill(SkillId)` | BP Native Event, 순수 판정 | 스택·특수 자원 등 고유 조건 검사 |
| `ExecuteItemSkill(SkillId, ActionHandle)` | BP 이벤트 | 공격/스킬 실제 구현 |
| `SetItemUIState` | C++ 함수 | 게이지/숫자/소형 스택 데이터 전달 |
| `UnregisterItem` | C++ 흐름 | 구독 해제·Modifier 제거·BP 해제 이벤트 |
| `OnItemUnregistered` | BP 이벤트 | 고유 Object/연출 정리 |

픽업, 상점, 보상, 제단은 아이템을 직접 장착하지 않는다. 모두 `Begin Loadout Acquisition → Commit Loadout Acquisition` 경로를 사용한다. 월드 픽업은 `BP_LoadoutItemPickup`/`BP_ConsumablePickup`이 `IARInteractable`로 구현한다.

Acquisition/Evolution Token은 Player 약한 참조, 요청 당시 Loadout Revision, 현재 무기 InstanceId, 허용 후보 Primary Asset ID를 보관한다. Commit에서 모두 다시 검증하고 사망·다른 장착 변경·UI 취소로 오래된 요청은 `StaleRequest`로 거부한다. 월드 드롭은 유효 위치와 Pickup 생성 가능성을 먼저 확인한 뒤 Instance 제거를 Commit하며, Spawn 실패 시 아이템을 잃지 않도록 원상 복구한다.

---

## 9. Action Handle과 적 개발 규약

### 표준 흐름

```text
입력 또는 AI 패턴
→ Try Start Action
→ Action Handle 획득
→ 필요 시 Roll/Basic Move 잠금
→ Action Delay로 선딜
→ Register Action Hitbox 또는 투사체 생성
→ Apply Combat Damage / Apply Stagger And Groggy Damage
→ Action Delay로 후딜
→ End Action
```

경직·기절·사망·허용된 구르기 시 Action Handle이 취소된다. Action이 소유한 Delay, Hitbox, 이동 잠금, 임시 Modifier는 자동 정리된다. 이미 발사된 투사체, 등록된 DOT, 영구 장착 효과만 의도적으로 Action 밖에서 유지한다.

Action Request/Skill Definition은 아래를 명시한다.

- `bCancelOnStagger`, `bCancelOnStun`, `bCancelOnRoll`, `bCancelOnBasicMovementInput`
- `bBlockRollWhileActive`, `bBlockBasicMovementWhileActive`

“구르기로 취소되는가”와 “구르기를 시작할 수 있는가”는 다른 값이다. 차지 중 구르기를 막으려면 후자를 켜고, 구르면서도 유지되는 스킬은 전자를 끈다.

### 적/무기 개발자가 금지할 것

- 일반 `Delay` 또는 독립 Timer로 공격 선딜·후딜·차지를 관리하지 않는다.
- 체력, 스탯, 경직 Bool을 BP에서 직접 대입하지 않는다.
- Root/Stun을 위해 이속을 0으로 바꿨다가 수동 복구하지 않는다.
- 장거리 이동을 `SetActorLocation` 반복이나 독립 Timeline으로 만들지 않는다.
- 엔진 기본 `Apply Damage`와 Foundation 피해 경로를 섞지 않는다.

기반 구현 후 이 규약은 예제 적/무기를 포함해 `FOUNDATION_ENEMY_CONTENT_GUIDE.md`로 따로 작성한다.

---

## 10. 초기화, 장착, 해제 순서

### Character BeginPlay

```text
Native Gameplay Tag 등록
→ Character Spawn
→ 공통 Component 생성 및 상호 참조 검증
→ Details 기본 스탯 캐시 계산
→ Player 전용 Component 초기화
→ 시작 Item Definition으로 Runtime Instance 생성
→ 고정 Modifier 등록
→ Skill/UI 데이터 등록
→ Runtime BP OnItemRegistered
→ HUD가 Component 이벤트 구독
→ 입력 활성화
```

시작 무기가 없는 테스트 상황에서도 Player는 안전해야 한다. “현재 무기는 항상 존재한다”는 가정을 공통 코드에 넣지 않는다.

### 아이템 장착/교체

```text
Definition/Runtime Class 유효성 검사
→ 슬롯 규칙 검사
→ 필요한 기존 무기 Instance Unregister
→ 새 Instance 생성 (Outer: Loadout Component 또는 Player)
→ Definition 기본 Modifier 등록
→ Skill/UI 등록
→ Runtime BP OnItemRegistered
→ OnLoadoutChanged 발행
```

새 무기 Definition/Runtime Class 검증은 기존 무기 제거 전에 끝낸다. 교체 실패로 빈 무기 상태가 되면 안 된다.

### 종료 정리

| 상황 | 필수 정리 |
|---|---|
| Action 취소 | Delay/Hitbox/이동 잠금/Action Modifier 제거, 취소 이벤트 |
| 아이템 제거 | 이벤트 구독 해제, 자기 Modifier와 UI 제거, BP 해제 이벤트 |
| 소모품 사용 | 성공 시 슬롯 비우고 Instance 제거 |
| Character 사망 | 모든 Action 취소, 이동 잠금, 사망 이벤트 한 번 |
| EndPlay | DOT/구독/Instance 안전 정리, 파괴 중 World 접근 금지 |

---

## 11. UI 데이터 전달

Widget은 매 프레임 Component를 순회하지 않는다. 아래 이벤트를 구독해 표시 데이터만 갱신한다.

| UI | 구독 이벤트 |
|---|---|
| 체력/보호막 | `OnHealthChanged`, `OnShieldChanged` |
| MP/스태미나 | 각 `OnResourceChanged` |
| 무기/액티브 유물 | `OnLoadoutChanged`, 스킬 쿨다운 이벤트 |
| 아이템 게이지/스택 | `OnItemUIStateChanged` |
| 상태 아이콘 | `OnStatusAdded/Updated/Removed` |
| 인벤토리 최종 스탯 | `OnFinalStatChanged`, 열 때 `GetStatBreakdown` |
| 소모품 슬롯 | `OnConsumableSlotsChanged` |
| 통합 HUD | 초기 `GetHUDSnapshot`, 이후 `OnHUDSnapshotChanged` |

`FARItemUIState`는 State ID, 표시 방식(게이지/숫자/1~6 소형 스택), 현재값, 최대값, 표시 대상(HUD/상세/양쪽)만 전달한다. Runtime BP는 Widget을 직접 만들지 않는다.

`FARPlayerHUDSnapshot`은 Health/Shield/Mana/Stamina, Aim Direction, 장착 무기, 액티브 유물, 등록 스킬, 소모품 슬롯을 한 번에 복사해 전달한다. `UARUIManagerComponent`가 각 소유 Component 이벤트를 받아 통합 이벤트를 발행하므로 Widget이 여러 시스템의 내부 배열을 직접 순회하지 않는다. 상점·진화처럼 미보유 Definition을 표시할 때는 `GetLoadoutDefinitionDisplayData`, 보유 인스턴스 상세에는 `GetLoadoutItemDisplayData`를 사용한다.

인벤토리와 진화 UI는 동시에 열지 않는다. Player의 `SetGameplayInputBlocked`로 이동·구르기·스킬·소모품·상호작용을 막되 게임은 멈추지 않는다.

---

## 12. 구현 순서와 완료 기준

### 단계 0 — 기반

- Git 기준점, `.gitignore`/LFS, Build.cs와 `.uproject`의 Paper2D·GameplayTags
- 기존 클릭 이동 TopDown Controller/GameMode와 새 Foundation Player/Controller/GameMode의 이행 경로
- 로그, Gameplay Tag, 공통 Types, 충돌 채널, Asset Manager 설정, Test Map
- Paper2D Player 최소 BP와 Orthographic Camera
- 2D 프로토타입에 불필요한 Ray Tracing/Lumen/Substrate 설정 검토

**완료:** 빈 맵에서 Paper2D Player가 WASD·마우스 조준·카메라 추적을 정상 수행한다.

### 단계 1 — Character, 스탯, 생존

- Base/Player/Enemy, Stats/Health/Movement
- Modifier 등록·제거·캐시, 체력/보호막/회복/사망 이벤트

**완료:** 더미 BP가 수정치를 넣고 빼면 최종 스탯/HUD가 이벤트로 갱신되고 회복은 최대 체력을 넘지 않는다.

### 단계 2 — 피해와 자원

- Damage Request/Result, Resolver, Combat Subsystem
- 물리/화염/마법/공허, 회피·필중·치명타·방어력·보호막
- MP/Stamina

**완료:** 모든 속성, 회피/일반 무적/공허/보호막 무시, 자원 동시 소비가 확정 수식대로 테스트된다.

### 단계 3 — DOT, 경직, 상태이상, Action

- DOT Scheduler, Stagger, Status, Action 소유 Delay/Hitbox/Modifier
- Stun/Root, 슈퍼아머, 그로기 이벤트

**완료:** 더미 적이 경직 시 공격을 정확히 취소하고, 슈퍼아머·그로기·상태 갱신을 분리 검증한다.

### 단계 4 — 아이템/소모품/UI 데이터

- Definition/Instance, Loadout/Consumable
- 장착/교체/버리기/픽업/진화/입력 우선순위
- HUD/인벤토리 데이터 이벤트

**완료:** 테스트용 무기 1개, 액티브 2개, 패시브 다수, 소모품으로 모든 슬롯 규칙을 검증한다.

### 단계 5 — 타 개발자 통합

- 다른 개발자 규약으로 만든 예제 적 하나와 무기 하나 통합
- Action 취소 누락 코드 리뷰
- 적 개발자 가이드 작성

**완료:** Foundation 담당자가 예제 콘텐츠 내부를 수정하지 않아도 정상 동작한다.

---

## 13. 테스트 전략

### 수식 자동화 테스트

`FARDamageResolver`와 Stats 계산을 일반 C++ 데이터로 실행해 Automation Test를 작성한다.

- 방어력/방관/최소 피해 1, 독립 감소율/음수 감소율
- 회피·필중·회피 보장·일반 무적·공허
- 공격력/주문력/경직력 계수 유무와 깡 피해
- 경직 저항/슈퍼아머/그로기 0 이벤트
- 강인함 상태 지속시간
- DOT 갱신형/독립형/프레임 지연 틱

### Component 통합 테스트

- 출처별 Modifier 제거, Buff 중첩, 시간 만료
- 보호막 LIFO와 지속시간 만료
- 스태미나 회복 대기, MP 소수점 누적
- Action 취소 시 Action 소유 항목 정리
- 무기 교체/유물 버리기 후 Modifier/스킬 UI 제거
- 소모품 슬롯 감소 시 초과 드롭

### Test Map

`Content/Game/Foundation/Test`에는 Combat Dummy, Stagger/그로기 Dummy, DOT 테스트 패널, 아이템 장착/진화 상자, Action 취소 디버그 Widget을 둔다. 콘텐츠 버그는 먼저 여기서 Foundation 문제인지 BP 문제인지 재현한다.

---

## 14. 지금 결정하지 않는 항목과 코드 작성 전 체크

아래는 의도적으로 나중에 정한다.

- 스탯 기본값, 무기별 계수, 밸런스
- 실제 무기/유물/소모품과 스킬 키 배치
- 보스 그로기 연출, 상점/드롭/방 생성/영구 성장
- 카메라 흔들림 수치, 목표 해상도별 정확한 Ortho Width
- 추가 상태이상과 전용 효과

다만 다음은 구현 중 바꾸지 않는 기반 규칙이다.

- 최종 스탯의 고정→합연산→곱연산 순서
- 일반 피해와 공허 피해의 분리
- 체력 피해와 경직/그로기 요청의 분리
- 모든 취소 가능 행동은 Action Handle 아래에서 실행
- Data Asset과 Runtime Instance의 분리
- C++ 최종 판정, Blueprint 콘텐츠 구현 경계

코드 작성 전 마지막 확인:

- [ ] 공통 Types와 Gameplay Tag가 콘텐츠 이름을 포함하지 않는가?
- [ ] Data Asset에 쿨다운/스택 같은 런타임 값이 없는가?
- [ ] 직접 체력·스탯 대입 노드를 만들지 않는가?
- [ ] 피해/DOT/경직/상태/이동에 각각 단일 공통 경로가 있는가?
- [ ] 예제 적과 예제 무기가 Action Handle 취소를 검증하는가?
- [ ] 모든 공식이 Automation Test로 고정되는가?
- [ ] 무기/유물 없이도 Foundation Test Map에서 공통 시스템을 검증할 수 있는가?

이 체크리스트를 통과한 뒤 실제 Foundation C++ 구현을 시작한다.
