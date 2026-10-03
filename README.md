# Action_RogueLike

Unreal Engine 5.8 기반 싱글플레이 액션 로그라이크의 공통 개발 시스템과 콘텐츠 제작 참고서입니다. C++는 전투·스탯·자원·액션의 공통 판정과 생명주기를 담당하고, Blueprint는 무기·유물·적·투사체·공격 범위·연출을 제작합니다.

현재는 **개발자 통합·검증 단계**입니다. 기능과 참고 문서를 공유하는 저장소이며, 최종 게임 패키지나 안정성이 보증된 정식 릴리스가 아닙니다. 배포 전 확인 사항은 아래에 공개합니다.

## 시작할 때 읽을 문서

- **AI와 함께 작업한다면:** [AI전용.md](AI전용.md)를 먼저 읽도록 전달하세요. 제작 순서, 공통 API 사용 규칙, 정리 책임, 검증 방법이 있습니다.
- **개발 참고서:** [가이드 메인 페이지](Docs/Guides/가이드%20메인%20페이지.html)를 내려받은 프로젝트에서 브라우저로 여세요. 노드 설명과 구조체 핀을 펼쳐 보며 참고할 수 있습니다.
- **노드 찾기:** [전체 Blueprint 노드 검색](Docs/Guides/common/BLUEPRINT_NODE_SEARCH_KO.html)에서 용도와 입력·출력을 검색하세요.

GitHub의 HTML 파일 보기는 실행되는 가이드 페이지가 아닙니다. 저장소를 받은 뒤 로컬에서 HTML을 여세요. AI에게도 `AI전용.md`만 따로 전달하기보다 관련 `Docs/Guides`와 코드를 함께 제공하는 것이 좋습니다.

| 제작 분야 | 참고 문서 |
| --- | --- |
| 스탯·성장·버프 | [객체별 스탯](Docs/Guides/common/OBJECT_STAT_GUIDE_KO.html), [스탯 조작·수명](Docs/Guides/precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html) |
| 피해·DOT·회복·실드 | [피해 노드와 공식](Docs/Guides/common/DAMAGE_NODES_FORMULA_GUIDE_KO.html) |
| CC·경직 | [CC·경직](Docs/Guides/common/CC_STAGGER_GUIDE_KO.html) |
| 아이템 | [DA 제작](Docs/Guides/nsh/ITEM_ASSET_CREATION_GUIDE_KO.html), [런타임 기본](Docs/Guides/nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html), [스킬 제작](Docs/Guides/nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html) |
| 적 | [전체 제작](Docs/Guides/hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html), [디테일 설정](Docs/Guides/hhc/ENEMY_DETAILS_GUIDE_KO.html), [이동](Docs/Guides/hhc/ENEMY_MOVEMENT_GUIDE_KO.html), [액션](Docs/Guides/hhc/ENEMY_ACTION_GUIDE_KO.html) |
| 환경·드롭·상점 후보 | [적 스폰](Docs/Guides/environment/ENEMY_SPAWN_GUIDE_KO.html), [아이템 검색](Docs/Guides/environment/ITEM_SEARCH_GUIDE_KO.html), [픽업 소환](Docs/Guides/environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html) |
| 제작 시 주의점 | [액션 생명주기](Docs/Guides/precautions/ACTION_LIFECYCLE_GUIDE_KO.html), [스킬 우선순위](Docs/Guides/precautions/SKILL_PRIORITY_GUIDE_KO.html), [시간 그룹](Docs/Guides/precautions/TIME_GROUP_GUIDE_KO.html), [적 이동 점검](Docs/Guides/precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html) |
| 입력 정책 | [기본 조작](Docs/Guides/common/BASIC_CONTROLS_GUIDE_KO.html) |

## 공통 시스템

- 스탯 수정치, 스택 조회·소비, 아이템/액션 귀속 효과, 기본값을 바꾸는 Permanent Flat 및 플레이어 Money.
- 직접 피해·DOT, 속성·피해 공식, 동일 Damage Name 제한, 실드, 체력·마나·스태미나 회복.
- 경직·그로기, 기절·속박, 강인함, 슈퍼아머와 CC 면역.
- Action Handle 기반 시작·종료·취소, 아이템 스킬 사전 조건, 우선순위 묶음과 자원 거래.
- 아이템·액션 전용 타이머, Action Delay, World/Player 시간 그룹.
- 아이템 정의/런타임/획득·장착·버리기·상호작용과 카탈로그 조회.
- CC·이동 잠금 규칙을 이용하는 AR AI Controller와 경로 이동 요청.
- 플레이어 자원 HUD와 상태 변경 이벤트.

공통 시스템은 특정 콘텐츠의 고유 이름을 전제로 하지 않습니다. 개별 콘텐츠를 만들 때 공통 C++에 전용 분기를 추가하기보다 공개 AR 노드와 Blueprint를 재사용하세요. 자동 멀티플레이 지원이나 모든 물리·음향의 독립 시간 처리를 제공하는 것은 아닙니다.

## 개발 환경과 첫 실행

1. Unreal Engine 5.8을 설치하세요. 현재 검사 환경은 Windows / UE 5.8.2입니다. 다른 버전 호환성은 별도로 검증해야 합니다.
2. 해당 엔진이 지원하는 C++ 빌드 도구와 Windows SDK를 준비하세요. 저장소의 `.vsconfig`는 기존 개발 환경의 구성 참고 자료입니다.
3. 저장소를 복제하고 기존 Git LFS 에셋을 받는 환경이라면 Git LFS도 준비하세요. `.gitattributes`에 바이너리 에셋 설정이 있습니다.
4. 별도로 전달되는 아트가 있다면 지정된 버전을 원래 `ArtSource` / `Content/Game/Art` 경로에 배치하세요.
5. 필요하면 `.uproject`에서 프로젝트 파일을 생성하고, 작업을 저장한 뒤 에디터를 닫은 상태에서 정식 Editor 빌드를 실행하세요.

프로젝트 루트의 PowerShell에서:

```powershell
& .\Scripts\build_editor.ps1
```

기본 엔진 위치는 `C:\Program Files\Epic Games\UE_5.8`입니다. 다르다면:

```powershell
& .\Scripts\build_editor.ps1 -EngineRoot 'D:\Epic Games\UE_5.8'
```

빌드 후 `Action_RogueLike.uproject`를 열고, 필요한 콘텐츠가 있는지 확인하여 `/Game/Game/Tests/Maps/Test_Level`에서 테스트하세요. 단순 Blueprint Compile이나 Live Coding만으로 C++ 반영·구조 변경의 정식 검증을 대신하지 마세요. Editor 빌드 성공은 게임용 빌드·패키징 성공과 별개입니다.

## 제작할 때 꼭 지킬 계약

- **DA는 설정, 런타임 인스턴스는 실행 상태입니다.** 쿨타임·소유자·스택을 DA/CDO에 저장하지 마세요.
- 아이템 스킬은 `Can Execute Item Skill`에서 부작용 없이 조건을 검사하고, `Execute Item Skill`에서 받은 Action Handle로 실행합니다. 정상 완료와 내부 실패 출구에서 액션을 끝내세요. DA 비용을 BP에서 이중 차감하지 마세요.
- 적은 `Try Start Action` 성공 후 받은 핸들로 공격을 실행하고 종료합니다. `On Action Cancelled`에서는 콘텐츠가 직접 소유한 연출·구독·일반 타이머 등을 정리하세요.
- 아이템 보유 효과에는 Item Timer, 행동 중 효과에는 Action Timer를 사용하세요. 전용 예약의 자동 정리가 이미 적용한 일반 버프·독립 Actor·전역 배율까지 되돌려 주는 것은 아닙니다.
- 선택형 시간 그룹 기본값은 World입니다. 적은 World를 권장하고 플레이어 아이템은 의도에 맞춰 선택하세요. 액션 타이머 그룹은 액션 그룹을 자동 상속하지 않습니다.
- 적의 경로 탐색에는 맵 제작자가 준비한 NavMesh가 필요합니다. 없거나 연결되지 않았으면 맵 제작자에게 문의하세요.
- 스탯·자원·상태 내부 값을 직접 대입하거나 일반 피해 경로로 공통 전투 공식을 우회하지 마세요. Success·반환 핸들·실패 이유를 확인하세요.
- 키 추가·변경·건의는 이정환에게 문의하세요. 조작 가이드의 계획과 현재 IA/IMC 연결 상태를 구분하세요.

## 폴더와 공유 범위

```text
Source/             공통 C++와 에디터 전용 코드
Config/             태그·충돌·Asset Manager 등 프로젝트 설정
Content/Game/       BP·DA·입력·맵·UI·게임 아트
ArtSource/          아트 편집 원본
Docs/Guides/        로컬 HTML 개발 참고서
Scripts/            빌드·문서 생성·검증 도구
AI전용.md            AI 에이전트용 개발 안내
```

- 코드·설정·스크립트·문서는 GitHub에서 관리합니다.
- 아트 원본과 대용량 게임 아트는 별도 공유할 수 있습니다. 코드 버전과 필요한 아트 묶음 버전을 맞추고 원래 경로를 유지하세요.
- **BP·DA·IMC·맵도 에셋입니다.** 아트 제외와 전체 에셋 제외는 다릅니다. 기능 에셋 공유 범위는 담당자의 지시를 따릅니다.
- 이번 업로드에서는 기존 요청에 따라 **새 Content 에셋 변경/추가/삭제와 아트 변경을 제외**했습니다. 따라서 GitHub의 기존 테스트 에셋이 로컬 최신 버전과 같다고 가정하면 안 됩니다. 기존에 추적된 에셋과 그 이력은 유지됩니다.
- 이 설명만으로 ArtSource/Art의 Git 제외 정책이 새로 적용되는 것은 아닙니다. 실제 `.gitignore`와 추적 상태를 확인하세요.
- `Intermediate`, `.vs`, `Binaries`, `DerivedDataCache`, `Saved` 등 캐시는 `.gitignore`로 제외합니다. 다른 개발자의 PC에서 빌드·IDE 사용 중 다시 만들어집니다.

## 검증 방법

프로젝트 루트에서 문서 검사를 실행할 수 있습니다. Python과 Node.js가 필요합니다.

```powershell
python Scripts/validate_development_guides.py
python Scripts/test_enemy_details_guide.py
python Scripts/test_controls_guide.py
python Scripts/test_timer_guide_references.py
node Scripts/test_node_search_guide.js
node Scripts/test_guide_controls.js
git diff --check
```

Unreal 자동 테스트는 `AR.` 이름 공간을 사용합니다. 실행 명령과 보고서 위치는 [AI전용.md](AI전용.md)의 검증 항목을 참고하세요. 콘텐츠는 정상 사용뿐 아니라 비용 부족·연타·CC 취소·아이템 해제·소유자 파괴·레벨 종료와 시간 배율 변경을 함께 확인하세요.

HTML을 수정할 때는 `Scripts`의 해당 생성기에도 변경을 반영해야 재생성으로 수정 내용이 사라지지 않습니다. 에셋 생성·이름 변경·재배치 스크립트는 읽기 전용 검사가 아니므로 실행 전에 대상과 쓰기 범위를 확인하세요.

## 배포 전 확인 사항 — 2026-10-04

현재 진단에서 발견한 미해결 사항입니다. 이 업로드는 코드를 공유하는 작업이며 아래 문제를 수정한 작업이 아닙니다.

1. **비에디터 Development 게임 빌드 실패:** 일부 자동 테스트가 에디터 전용 `UFunction::GetMetaData`를 게임 대상에서도 호출합니다. 테스트의 빌드 조건/모듈 배치를 보강해야 합니다.
2. **기존 테스트 종료 후 강제 GC 충돌:** `ActiveRelicSlotInput` 테스트 세계에 남은 아이템의 해제 이벤트가 GC 중 호출되는 문제가 재현되었습니다. 테스트 종료 처리와 알림 안전성 조사가 필요합니다. 정상 액터 생명주기를 사용한 독립 검사는 30회 통과했으며, 일반 플레이에서 동일 충돌이 발생하는지는 미확정입니다.
3. **조작 문서와 기본 IMC의 차이:** 검사한 기본 매핑은 WASD·Space·F·E/R·좌클릭입니다. 일부 문서상 조작은 그 기본 목록에 아직 연결되지 않았습니다. 다른 입력 프로필까지 모두 검증한 결과는 아닙니다.
4. Shipping 빌드, 최종 패키지 실행, 다른 PC와 장시간 플레이는 추가 검증이 필요합니다.

해당 진단에서 기존 자동 테스트 68건, BP 10개·아이템 정의 2개 검증, 가이드 HTML 21개 정적 검사, Windows 쿠킹과 화면 없는 600프레임 실행은 통과했습니다. 이 결과는 진단 시점의 로컬 코드·에셋 기준이며, 이번에 제외된 최신 에셋까지 GitHub에 포함되었다는 뜻은 아닙니다. 전체 시스템의 무결성을 보장하지 않습니다.

`Docs/Foundation`에는 설계서와 과거 구현 기록이 함께 있습니다. 과거 완료율·테스트 수보다 현재 코드와 최신 검사 결과를 기준으로 판단하세요.
