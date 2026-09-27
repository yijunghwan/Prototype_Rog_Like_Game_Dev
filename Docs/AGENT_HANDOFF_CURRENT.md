# 다음 에이전트용 작업 인계 — 현재 상태

기준일: 2026-09-28

이 문서를 먼저 읽고 기존 문서와 실제 코드를 확인하면 현재 작업을 이어갈 수 있다. 이 문서는 프로젝트 상태 전달용이며, 특정 다음 기능의 구현을 자동으로 요청하는 문서는 아니다. 다음 작업 범위는 사용자의 새 요청에 따른다.

## 1. 프로젝트와 작업 원칙

- 프로젝트 루트: `C:\Users\ghksd\Desktop\Action_RogLike\Prototype_Rog_Like_Game_Dev`
- 상위 작업 폴더: `C:\Users\ghksd\Desktop\Action_RogLike`
- 프로젝트 파일: `Action_RogueLike.uproject`
- 엔진: Unreal Engine 5.8. 설치 경로 `C:\Program Files\Epic Games\UE_5.8`.
- 모듈: `Action_RogueLike`. 에디터 빌드 타깃: `Action_RogueLikeEditor Win64 Development`.
- Paper2D 기반 탑다운 액션 로그라이크. 공통 C++ Foundation 위에 플레이어·적·무기·유물·소모품·환경 콘텐츠를 만드는 단계다.
- 사용자와 한국어로 소통한다. 에디터 항목은 실제 영어 이름도 함께 알려주면 좋다.
- **사용자가 명시적으로 요청할 때만 GitHub 커밋·푸시한다. 이번 인계 묶음은 사용자의 업로드 요청에 따라 검증 후 커밋·푸시한다. 이후 작업은 별도 요청 없이 올리지 않는다.**
- 기존 작업과 사용자 BP/Widget 편집을 보존한다. 작업 폴더를 정리한다는 이유로 되돌리거나 에셋을 재생성하지 않는다.
- C++ 빌드 전 에디터 실행 상태를 확인한다. 열려 있으면 저장·종료가 필요한지 판단하고, 사용자의 편집 내용을 강제로 버리거나 에디터를 강제 종료하지 않는다. 인계 작성 시에는 실행 중인 UnrealEditor가 없었다.
- 미정인 게임 수치·규칙은 확정 사실처럼 만들지 않는다. 사용자가 허용한 테스트용 제안과 확정 규칙을 구분한다.

## 2. 먼저 읽을 문서

1. [이 문서](AGENT_HANDOFF_CURRENT.md): 최신 요약, 변경 내용과 인계 시 주의사항.
2. [Foundation 구현 기록](Foundation/FOUNDATION_IMPLEMENTATION_HANDOFF.md): 구현·수정·검증 이력. 앞부분의 21개 테스트는 과거 결과다. 아래 날짜별 보완 기록까지 읽는다.
3. [Foundation 시스템 계획](Foundation/FOUNDATION_SYSTEM_PLAN.md): 게임 규칙과 스탯·전투 공식.
4. [코드 구조](Foundation/FOUNDATION_CODE_ARCHITECTURE.md): 컴포넌트 책임과 파일 구조.
5. [Blueprint 제작 계획](Foundation/FOUNDATION_BLUEPRINT_PLAN.md): 콘텐츠 제작 방법과 연결 규약.
6. [노드 목록](Foundation/FOUNDATION_BLUEPRINT_NODE_CATALOG.md) / [한영 HTML 레퍼런스](Foundation/FOUNDATION_REFERENCE_KO_EN.html): 노드·Details·입출력 참조.
7. [칼리번·엑스칼리버 제작 가안](Content/Weapons/CALIBURN_EXCALIBUR_DESIGN_DRAFT.md): 사용자와 정리한 무기 기획. 아직 구현한 무기가 아니다.
8. [임시 HUD 편집 안내](../Content/Game/Foundation/UI/README.md): 실제 Widget 외형 수정 방법.

과거 문서의 “Content에는 경로만 있다”, “UI는 아직 없다” 등의 기록은 현재 상태와 다를 수 있다. 최신 보완 기록과 실제 에셋·코드를 기준으로 판단한다. 기획 문서와 구현 상태도 구분한다.

## 3. Git 상태 — 다음 작업 전에 꼭 확인

- 브랜치: `main`.
- 이번 작업의 시작 기준 커밋: `4caba0a` — `feat: 적 AI 이동과 CC 연동 및 템플릿 콘텐츠 정리`.
- 최신 커밋은 `git log -1`로 확인한다. 아래 HUD·기획·문서와 이 인계 문서는 함께 커밋하는 인계 묶음이다.
- 원격: `https://github.com/yijunghwan/Prototype_Rog_Like_Game_Dev.git`.
- 사용자가 최종 검증 후 문서까지 포함한 GitHub 업로드를 요청했다. 검증 결과는 8절에 기록했다.
- 원격 체크아웃은 최신 `origin/main`을 사용한다. 작업 시작 시 로컬 상태와 원격 버전을 확인하고 추가 편집을 보존한다.

이번 인계 묶음의 주요 변경:

- `Content/Game/Foundation/UI/README.md`
- `Content/Game/Foundation/UI/WBP_TestResourceHUD.uasset` — 신규, 실제 Widget 에셋.
- `Docs/Content/Weapons/CALIBURN_EXCALIBUR_DESIGN_DRAFT.md` — 신규.
- 이 인계 문서 — 신규.
- `Docs/Foundation/FOUNDATION_BLUEPRINT_NODE_CATALOG.md`
- `Docs/Foundation/FOUNDATION_CODE_ARCHITECTURE.md`
- `Docs/Foundation/FOUNDATION_IMPLEMENTATION_HANDOFF.md`
- `Docs/Foundation/FOUNDATION_REFERENCE_KO_EN.html`
- `Scripts/generate_foundation_html_reference.py`
- `Source/Action_RogueLike/Action_RogueLike.Build.cs`
- `Source/Action_RogueLike/Public/Foundation/Player/ARPlayerController.h`
- `Source/Action_RogueLike/Private/Foundation/Player/ARPlayerController.cpp`
- `Source/Action_RogueLike/Public/Foundation/Components/ARUIManagerComponent.h`
- `Source/Action_RogueLike/Private/Foundation/Components/ARUIManagerComponent.cpp`
- `Source/Action_RogueLike/Public/Foundation/UI/README.md`
- `Source/Action_RogueLike/Public/Foundation/UI/ARResourceHUDWidget.h` — 신규.
- `Source/Action_RogueLike/Private/Foundation/UI/ARResourceHUDWidget.cpp` — 신규.
- `Source/Action_RogueLike/Public/Foundation/UI/ARCreateResourceHUDCommandlet.h` — 신규.
- `Source/Action_RogueLike/Private/Foundation/UI/ARCreateResourceHUDCommandlet.cpp` — 신규.
- `Source/Action_RogueLike/Private/Foundation/Tests/ARFoundationComponentTests.cpp`

목록은 작업 시작 시 `git status --short`로 다시 확인한다.

## 4. 현재 구현 상태

### 공통 전투 기반

스탯 수정치·자원 소비/회복·보호막·피해 계산·지속 피해·경직/그로기·상태이상·Action 수명·아이템/유물/소모품·장착/획득/진화·UI 데이터 전달은 Foundation에 구현돼 있다. 개별 콘텐츠의 실제 타격·애니메이션·의사결정·효과 조합은 별도 제작한다.

- 플레이어와 적은 `AARBaseCharacter`를 공통 부모로 사용한다.
- 환경 가시·함정은 일반 Actor에 `UARCombatSourceComponent`를 붙여 Environment 공격원으로 구성한다.
- 현재 공격속도 스탯은 **100이 1배**다. 무기 기획의 “1.0이면 초당 1회” 표현을 코드의 스탯 단위와 혼동하지 않는다.
- 플레이어 피격 후 일반 무적 `Post Hit Invulnerability Duration` 기본값은 사용자 요청으로 0이다.
- 무적·회피·필중·공허 피해의 규칙은 기존 전투 계산기를 확인한다. 보호막 무시와 보호막 제거는 서로 다르다.

### 플레이어·대시·카메라

- WASD 이동과 마우스 월드 조준이 연결돼 있다.
- 기본 대시는 이동 입력이 있으면 정규화한 8방향 이동, 입력이 없으면 마우스 방향이다.
- `IA_Roll_v2`/MouseRollAction은 이동 입력과 무관하게 마우스 방향 대시다.
- 대시 거리 등은 스탯과 캐릭터 설정을 사용한다. `RollDuration`과 `RollCooldown`은 캐릭터 Details 값이다.
- 대시 쿨타임 기본 0.50초. 정상 종료·취소 시 시작하고 두 입력 경로가 공유한다.
- 카메라 추적 컴포넌트가 있으며 속도 관련 Details로 반응을 조절한다.
- 카메라 경계 박스 제한은 개념만 논의했으며 아직 구현하지 않았다.

### 적 AI 이동과 CC — 구현 완료

주요 파일:

- `Public/Foundation/AI/ARAIController.h`
- `Private/Foundation/AI/ARAIController.cpp`
- `Private/Foundation/Characters/ARBaseEnemy.cpp`
- `ARMovementControlComponent`, `ARStatusEffectComponent`, `ARStaggerComponent`, `ARBaseCharacter`.

위 Source 경로의 기준 폴더는 `Source/Action_RogueLike/`다.

- `AARBaseEnemy`는 기본 AI Controller로 `AARAIController`, 자동 점유로 `PlacedInWorldOrSpawned`를 사용한다.
- Blueprint 노드: `AR AI Move To Actor`, `AR AI Move To Location`, `Can Request Basic Move`.
- 호출 Target은 적 객체 자체가 아니라 해당 적의 `AR AIController`다.
- 앞의 두 노드는 NavMesh 경로 요청이다. 반환값은 Failed/AlreadyAtGoal/RequestSuccessful 요청 상태이며, 비동기 완료 실행 핀은 없다.
- `Can Request Basic Move`는 공통 이동 잠금 검사이며 길 찾기 성공을 보장하지 않는다.
- 기본 이동이 잠기면 새 경로 요청을 거부하고 진행 중인 경로를 중단한다.
- 해제 후 이전 경로는 자동 재개하지 않는다. AI가 목표를 판단해 다시 요청한다.
- 이 Controller를 사용하는 Behavior Tree의 일반 `Move To`도 공통 MoveTo 검사에 연결돼 있다.
- `Request Basic Move`는 방향 이동이며 NavMesh 노드가 아니다. 스킬 이동은 `Request Action Move`/`Request Action Velocity`를 사용한다.
- Stun/Root는 실제 Data Asset을 만들고 차단 필드를 설정해야 한다. 상태 태그 이름만 지정했다고 모든 규칙이 자동으로 정해지는 것은 아니다.
- 진행 중 스킬 취소는 상태의 `Cancel Actions On Apply`·사유 Stun과 스킬의 `Cancel On Stun` 등을 함께 지정한다. 새 행동은 `Blocks Skill Groups`와 Action 시작 검사로 차단한다.
- 적/플레이어 스킬을 직접 BP 로직으로만 실행하면 Action의 차단·정리 규칙을 우회할 수 있다.
- C++ 핵심 Status/Stagger/Movement 연결에는 네이티브 이벤트를 사용하며 기존 BP 이벤트도 유지했다.

아직 없는 것: 적별 추적·공격 BT/BP, 실제 Stun/Root Data Asset, NavMesh 추적의 실제 콘텐츠 시각 테스트.

## 5. 임시 자원 HUD — 제작·적용 완료

실제 에셋:

`Content/Game/Foundation/UI/WBP_TestResourceHUD.uasset`

언리얼 패키지:

`/Game/Game/Foundation/UI/WBP_TestResourceHUD`

- 부모 `UARResourceHUDWidget`.
- 화면 왼쪽 위에 체력(빨강), 스태미나(초록), 마나(파랑)의 바와 현재값/최대값 표시.
- 기본 UMG 도형 사용. 외부 이미지·텍스처는 필요 없다.
- 사용자의 최신 요청대로 **숫자는 소수점 없이 반올림**한다. 내부 자원값과 바 비율은 실수 정밀도를 유지한다.
- 형식 설정은 `ARResourceHUDWidget.cpp`의 `ApplySnapshot` 안 `Format.MaximumFractionalDigits = 0`이다. Designer의 Text를 바꿔도 런타임 숫자 형식을 바꾸지는 못한다.
- `AARPlayerController`가 로컬 플레이어에 자동 생성한다. `bShowResourceHUD`와 `ResourceHUDClass`로 표시·교체 설정.
- UI Manager 초기 Snapshot과 변경 이벤트를 구독한다. Pawn 교체 시 이전 구독을 해제하며, 소유 Pawn이 없으면 숨긴다.
- `HitTestInvisible`로 마우스 전투 입력을 가로채지 않는다.
- 실제 게임모드와 테스트 맵에서 자동 표시를 확인했다.

외형은 Widget의 Designer에서 수정한다. 데이터 연결 유지를 위해 아래 이름·타입을 유지한다.

- Progress Bar: `HealthBar`, `StaminaBar`, `ManaBar`.
- Text: `HealthValue`, `StaminaValue`, `ManaValue`.

기존 에셋을 임의로 덮어쓰지 않는다. `-run=ARCreateResourceHUD`는 초기 에셋 생성용이며 기존 파일이 있으면 보존한다. 네이티브 레이아웃은 에셋이 없을 때의 대체 화면이다. 일반적인 외형 수정은 에셋에서 한다.

`Action_RogueLike.Build.cs`에 SlateCore를 추가하고 UnrealEd/UMGEditor는 에디터 타깃에서만 의존하도록 했다.

아직 표시하지 않는 것: 보호막, 스킬 슬롯·쿨타임, 아이템/패시브 스택, 인벤토리·진화 선택창. 해당 데이터 기반은 있지만 실제 화면은 추가 제작해야 한다.

## 6. 무기 기획 — 구현하지 않은 상태

[칼리번·엑스칼리버 제작 가안](Content/Weapons/CALIBURN_EXCALIBUR_DESIGN_DRAFT.md)을 전달 기준으로 사용한다. 상세 계수·단계별 스킬은 원문에 있으므로 여기서 중복 정의하지 않는다.

핵심 확정 내용:

- 1차 봉인된 칼리번 → 2차 칼리번 → 3차 엑스칼리버.
- 기본 스탯은 미정.
- 아발론의 축복: 1킬당 1스택, 최대 100. 진화 후 누적 상태 유지.
- 격렬한 벼락: 최대 10/20/30스택. 관통 구간 10→20%, 20→30%, 30→40%.
- 1스킬 사용당 후속 적중 포함 최대 5스택 획득. 남은 획득 가능량은 다음 사용으로 이월하지 않음.
- 타격 성공 횟수로 획득 판정. 한 타격이 여러 적을 맞혀도 1회, 2연격은 2회.
- **체력 피해 시에만 5스택 감소. 보호막 전량 흡수 시 감소하지 않음.**
- 10초 미획득 시 10스택 감소 후 다시 10초를 셈.
- 호수가의 여인의 축복: 검 최대 4개. 보유 검당 SP×0.25 마법 적중 효과. 회전 타격 기본도 SP×0.25.
- AAA1S 선딜 완료는 회전 시작 시점이며 검 생성 조건을 대체하지 않음.
- 투척 관통은 통과 가능한 적 수다. 2차는 1명 통과 후 다음 타격에 삭제, 3차는 2명 통과 후 다음 타격에 삭제.
- 복합 속성 피해도 논리적 타격 하나당 적중 효과 1회.
- 해방은 층당 1회, 7초, 세계 0.4배/플레이어 정상 시간. 재시전 이름은 원탁의 기사들.
- 재시전 도중 해방 시간 만료로 종료하지 않고, 재시전 완료 후 반드시 종료.
- 종료 시 7초간 받는 피해 +100%, 현재 마나 0, 패시브 2·3 스택 0. 패시브 1 유지.

기획에 필요한 공통 기능 중 아직 보강해야 하는 것: 진화 시 런타임 상태 이전, 스킬 충전, 누르기/떼기 입력, 공격별 확정 치명타, 넉백/끌어당기기, 복합 피해의 적중 집계, 층별 사용 횟수, 플레이어 정상 시간 타이머 및 전체 피격 취약 규칙.

무기 문서의 “남은 확인 항목”도 읽는다. 예를 들어 2스킬 기절이 어느 타격까지 적용되는지, 강화 불 적중 버프 계수, 일반 마법검 회전 판정, 시간 감속 시 플레이어 CC 기준 등은 미정 또는 해석 확인 항목이다. 완성 무기라고 보고 넘어가지 않는다.

## 7. 에셋 정리와 기본 맵

- 새 게임 콘텐츠는 `Content/Game` 아래에 만든다.
- 현재 기본 맵: `/Game/Game/Foundation/Test/Test_Level.Test_Level`.
- 기본 게임모드: `/Game/Game/Foundation/Blueprints/BP_TestGameMode.BP_TestGameMode_C`.
- 맵 파일은 `Test_Level.umap`이다. `.uasset`로 찾으면 없는 것으로 오판할 수 있다.
- 템플릿 TopDown/Variant/Mannequin 등은 앞선 사용자 요청으로 게임 콘텐츠와 분리·제거했고 Git 변경까지 반영했다. 삭제 상태를 임의로 복구하지 않는다.
- 당시 보관 폴더: `C:\Users\ghksd\Desktop\Action_RogLike\_UnrealTemplateArchive\Prototype_Rog_Like_Game_Dev` — 프로젝트 밖이며 GitHub에 포함되지 않는다.

## 8. 최신 검증 결과

- 에디터 Development 빌드 성공.
- HUD 최초 적용 최종 전체 `AR.Foundation`: 29 성공 / 0 실패 / 0 경고.
- 이후 소수점 제거 변경을 빌드하고 기존 `AR.Foundation.UI.ResourceHUDLiveUpdates` 단독 재검증: 1 성공 / 0 실패 / 0 경고.
- 단독 테스트는 실제 Widget 에셋 로드, 실제 전투 피해, 마나·스태미나 소비/회복, 최대 마나 변경, Pawn 교체, 구독 해제를 검증한다.
- 실제 `Test_Level` 게임 실행에서 세 자원 바·숫자가 표시되는 스크린샷을 확인했다.
- 마지막 HTML 변경은 문서 변경만이며 생성·HTML 파싱·세 AI 노드의 분류/Target/바로가기 연결을 확인했다.
- 업로드 직전 모든 최신 변경과 소수점 제거를 포함해 에디터 Development 빌드 및 전체 AR.Foundation을 다시 검증했다: **29 성공 / 0 실패 / 0 경고**, 미실행 0. 결과: `Saved/Automation/HandoffFinal/index.json`, `Saved/Logs/HandoffFinalTests.log`, `Saved/Logs/HandoffFinalBuild.log`.

결과 파일:

- `Saved/Automation/ResourceHUDFinal/index.json`
- `Saved/Automation/ResourceHUDInteger/index.json`
- `Saved/Logs/ResourceHUDTestsFinal.log`
- `Saved/Logs/ResourceHUDIntegerTests.log`
- `Saved/Logs/ResourceHUDIntegerBuild.log`
- `Saved/Screenshots/WindowsEditor/ScreenShot00001.png`

Saved는 일반적으로 Git 대상이 아니므로 다른 체크아웃에는 결과·스크린샷이 없을 수 있다.

## 9. 빌드·검증·문서 갱신 방법

프로젝트 루트의 PowerShell 기준:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' Action_RogueLikeEditor Win64 Development '-Project=C:\Users\ghksd\Desktop\Action_RogLike\Prototype_Rog_Like_Game_Dev\Action_RogueLike.uproject' -WaitMutex -NoHotReload
```

Foundation 테스트:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\ghksd\Desktop\Action_RogLike\Prototype_Rog_Like_Game_Dev\Action_RogueLike.uproject' -unattended -nop4 -nosplash -NullRHI -nosound -DDC-ForceMemoryCache '-ExecCmds=Automation RunTests AR.Foundation' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=C:\Users\ghksd\Desktop\Action_RogLike\Prototype_Rog_Like_Game_Dev\Saved\Automation\NextAgentFoundation' '-abslog=C:\Users\ghksd\Desktop\Action_RogLike\Prototype_Rog_Like_Game_Dev\Saved\Logs\NextAgentFoundation.log'
```

대상을 좁히려면 `AR.Foundation.UI.ResourceHUDLiveUpdates` 등으로 바꾼다. UnrealEditor-Cmd의 종료 코드만 보고 성공 여부를 판단하지 말고 레포트의 failed와 errors를 확인한다. 샌드박스에서 캐시 접근·빌드가 거부되면 필요한 권한으로 실행한다. 일반적인 SDK 부족 출력과 실제 대상 테스트 실패를 구분한다.

HTML 재생성:

```powershell
& 'C:\Users\ghksd\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' Scripts/generate_foundation_html_reference.py
```

런타임 Python 경로가 바뀌었다면 사용 가능한 Python을 확인한다. HTML을 단독으로 손으로 편집하지 말고, 원본 노드 목록이나 생성기를 수정한 뒤 재생성한다.

현재 HTML은 46개 스탯, 118개 클래스 필드, 244개 구조체 필드, 255개 함수/이벤트 노드, 41개 이벤트 디스패처를 포함한다. 상단의 ‘적 AI 이동’ 링크는 별도 안내로 이동한다. AI 노드는 실제 `AR|AI|Movement` 분류로 정리했고 Controller Target·입력·출력·CC 설명을 추가했다.

## 10. 다음 에이전트의 시작 순서

1. 이 문서와 `git status --short`, `git log -1`을 확인하고 추가 로컬 편집을 보존한다.
2. 사용자의 새 요청을 확인한다. 현재 추가 콘텐츠의 구현 시작까지 요청한 상태는 아니다.
3. 관련 Foundation 문서와 실제 코드를 읽는다. 이전 대화만 보고 구현 완료를 추정하지 않는다.
4. UI는 기존 Widget을 편집하고, 무기는 가안의 확정 규칙과 미정 항목을 구분해 진행한다.
5. 요청 범위를 구현·검증하고 기존 handoff와 관련 참조 문서에 변경을 기록한다.
6. 명시적 요청 없이 커밋·푸시하지 않는다.
