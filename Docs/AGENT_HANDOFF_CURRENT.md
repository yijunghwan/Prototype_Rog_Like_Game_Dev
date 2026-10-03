# 다음 에이전트용 작업 인계 — 현재 상태

## 2026-10-04 적 HTML NavMesh 책임 안내 — 완료

- 사용자 요청에 따라 적 이동 가이드의 NavMesh 직접 생성 절차(Volume 배치·크기·P 표시·빌드)를 제거. NavMesh 생성·관리는 맵 제작자 담당이며 없거나 경로가 연결되지 않아 경로 이동이 불가능하면 맵 제작자에게 문의하도록 안내. 직선 Request Basic Move는 NavMesh와 별개임을 유지.
- 적 전체·디테일 가이드, 이동 주의점, 적 스폰 가이드, 메인 적 이동 카드의 관련 문구·링크 제목도 일치시킴. 기존 navmesh-build 앵커는 북마크·링크 호환을 위해 유지. 생성기 두 개와 회귀 검사도 갱신하여 재생성 시 직접 만들기 안내가 복구되지 않도록 함.
- HTML21개/로컬 링크·리소스1691개, 적 디테일·NavMesh 책임 검사, 타이머 설명, 노드검색395개, 펼침·검색/inline JS4개 검사 통과. 게임 코드·BP·DA·맵 미변경, 빌드·Git 커밋/푸시 없음.

## 2026-10-04 적 디테일·상속 설정 가이드 — 완료

- 사용자 요청: 적 BP에서 상속받은 클래스/컴포넌트의 디테일 패널 항목을 노드 가이드와 별도로 설명. 사진의 Combat Team 및 추가 요청한 그로기 사용 설정 포함. hhc/ENEMY_DETAILS_GUIDE_KO.html 생성, 메인 적 카드/적 공통 네비게이션/적 전체 가이드 setup·groggy에서 연결. 루트는 가이드 메인 페이지.html 한 개 유지.
- 클래스 디폴트 vs 컴포넌트 선택 vs 맵 인스턴스 덮어쓰기, Combat Team(EditDefaultsOnly)의 Player/Enemy/Environment 실제 피해 규칙·조종/충돌과의 차이, AI Controller Class/Auto Possess AI/Auto Possess Player, Stats Base Stats/4개 Caps, Health 2개 설정, Stagger 4개 설정, Immune Status Tags, 이동 속도 반영/평면 제한/캡슐/외형/Actor·Component Tick 경계 설명. 각 항목 선택 위치·용도·기본값·설정 기준·주의점을 접이식으로 제공. 언리얼 전체 속성 사전이 아닌 AR 적 제작 핵심 참고서.
- 그로기: Use Groggy Gauge 기본false와 MaxGroggy 양수 필요, 원형 Max/Recovery 3스탯 기본0, BeginPlay 최대값 충전, Delay vs 기절기간 구분, RecoveryPerSecond 자연 회복, 소진 양수→0 이벤트·소진 잠금·잠금 중 자연 회복/차감 중단·Reset(-1) 복구 설명. 자동 기절 아님, CanBeStaggered/슈퍼아머/CC 면역 별개, 공격 측 양수 그로기 피해 및 기절 적용 실패/복구 정책 설명. 실제 이벤트 연결은 기존 적 전체 가이드 링크로 유지.
- 문서는 현재 헤더·구현·UE5.8 엔진 Pawn/Movement/Tick 선언과 Epic Navigation/Collision 자료 확인 후 작성. 원형 기본값과 사용자 BP/인스턴스 덮어쓰기를 구분. 게임 코드/BP/DA/레벨 미변경, 빌드·Git 커밋/푸시 없음.
- 생성원 Scripts/generate_enemy_details_guide.py, organize 호출/PAGES/Time 부록 제외 갱신. test_enemy_details_guide.py는 항목·그로기 경계·링크 및 기본값의 소스 드리프트 검사. HTML21개/로컬 링크·리소스1691개, 기본 조작/타이머/전체 노드 검색395개/펼침·검색/inline JS4개 검사 통과. 재생성 멱등성 확인. 브라우저 화면 렌더링/실제 적 플레이는 이번 문서 작업에서 미검증.

## 2026-10-04 기본 조작 안내 추가 — 완료 (무기 1번 키 해석 확인 필요)

- common/BASIC_CONTROLS_GUIDE_KO.html 추가. 메인의 공통 첫 카드·상단 바로가기와 공통 문서의 네비게이션에서 접근. 기본 표는 펼쳐 표시하고 추가 지정/충돌 안내는 접이식, 키 정책 문의도 기본 펼침.
- 사용자 기준: WASD 이동, Space 회피, I 인벤토리, E/R 유물 1/2번 슬롯, 1/2/3 소모품 슬롯, F 상호작용, 무기 2번 Q·3번 X. 원문 '1shfit f'는 모호하여 무기 1번 Shift 또는 Shift+F인지 비동기 질문했고 우선 Shift로 기재한다고 사용자에게 명시. 답변이 도착하면 생성기의 조작표와 test_controls_guide.py를 함께 수정할 것.
- 같은 기본 키에 추가 스킬 중복 지정 가능하되 충돌 주의, 현재 자유 지정 권장 키 Z/C, 키 추가·변경·건의는 이정환 문의 명시. 별개의 상호작용/슬롯 입력까지 스킬 우선순위가 자동 통합하지 않음 안내. 문서만 수정하며 게임의 실제 IA/IMC/BP/키 바인딩은 변경하지 않음.
- 생성기에서 재생성 가능. 기본 조작에는 무관한 시간 그룹 부록을 삽입하지 않도록 제외. 신규 조작표 회귀 검사 및 기존 링크/검색/펼침/타이머 설명 검사 통과: HTML20 / 로컬 링크·리소스1621개 / 노드검색395개. 생성 멱등성 확인. 빌드·게임 에셋 변경·Git 커밋/푸시 없음.

## 2026-10-04 가이드 폴더 정리·메인 이름 변경 — 완료

- 사용자 요청: Docs/Guides 최상위에는 메인 HTML 한 개만, 나머지는 nsh/hhc 등 분류별 폴더로 관리. index.html을 **가이드 메인 페이지.html**로 변경하고 title/h1/상단 메인 버튼 이름도 통일. 공통/common, 아이템/nsh, 적/hhc, 환경/environment, 주의점/precautions, 공유 리소스/assets 유지.
- 최상위 예제 MD 2개는 nsh/GUN_BLUEPRINT_EXAMPLE_KO.md와 hhc/TEST_ENEMY_BLUEPRINT_KO.md로 이동하고 상대 링크 수정. 최상위 스탯/피해 구형 위치 안내 HTML 2개만 삭제; 실제 상세 본문은 common에 유지. 삭제된 안내 파일은 Git에서 복구 가능하나 최상위에 재생성하지 말 것. 과거 브라우저 북마크/외부 file 링크는 새 경로로 변경해야 함.
- organize_development_guides.py의 HOME_NAME/GUIDE_HOME, 링크 재작성과 생성 경로를 갱신하고 이전 위치 안내 생성·초기 마이그레이션 제거. 기존 루트 URL은 링크 재작성 매핑에만 유지. Foundation 문서의 가이드 경로도 현행 common 경로로 수정. 검증기는 최상위 단일 파일·메인 제목·이전 index 링크 부재·이동한 예제 MD 상대 링크를 검사.
- 검증: HTML 19개 / 로컬 링크·리소스1598개 모두 유효, 노드 검색395개 및 검색/펼침·접기/타이머 설명/inline JS4개 검사 통과. 코드·게임 에셋은 수정하지 않았으며 빌드·Git 커밋/푸시 없음.

## 2026-10-04 레거시 아이템·액션 타이머 제거 — 완료

- 사용자 확인: 기존 레거시 노드를 모두 지웠으므로 코드와 HTML에서도 제거 요청. 에디터 미실행 확인 후 작업. GUN 및 다른 Blueprint/DA/Content 에셋을 자동 수정하지 않았음.
- SetItemTimerByEvent / SetActionTimerByEvent의 구형 UFUNCTION 선언·구현과 ARLifetimeTimer::Start 제거. 현재 사용 중인 SetGroupedItemTimerByEvent / SetGroupedActionTimerByEvent의 C++ 이름·핀·동작은 유지. BP 표시명은 Set Item Timer by Event / Set Action Timer by Event. 언리얼 기본 타이머는 변경하지 않음.
- 기존 수명/격리/입력 검증/부하 테스트를 시간 그룹 지원 API로 이전. Initial Start Delay는 추가 지연, Max Once Per Frame은 명시적 true로 검사하도록 수정. 음수 지연 구형 제한 대신 비유한 지연 거절 검사. 종료·취소·아이템 해제·소유자 파괴 자동 정리 테스트 보존.
- 생성기와 아이템·스킬·적 액션·액션 생명주기·시간 그룹 HTML의 레거시 호환 안내 삭제. 전체 노드 검색에서 구형 2항목 삭제(395개); 검색 테스트는 구형 항목 부재를 확인. 아래 시간 그룹 구현 기록의 'Legacy 보존'은 과거 상태이며 현재는 삭제됨.
- 정식 Development Editor 빌드 성공. 전체 AR 자동 테스트 **68/68 Success, 실패0/리포트 경고0/미실행0**: Saved/Automation/LegacyTimerRemoval/index.json, Saved/Logs/LegacyTimerRemoval.log. HTML 21개 / 로컬 링크·리소스1604개, 타이머 설명·검색·접기·JS 구문 검사 통과. 재생성 멱등성 확인, git diff --check 통과. 실제 플레이/화면 렌더링은 이번 작업에서 미검증.
- 기존 사용자 Content 변경 보존. 이번 작업 Git 커밋/푸시 없음. 에셋 업로드 금지 유지.

## 2026-10-04 World / Player 시간 그룹 — 구현·검증 완료

- 사용자 승인: 모든 선택형 기본 World. AR 플레이어 자체 시간은 Player, DA 쿨타임·Action Request·아이템/액션 타이머는 선택. 주의점에는 적 전체 World 권장 / 플레이어 아이템은 제작 의도에 따른 선택, 나머지 가이드는 기능 설명만. 작업 저장·에디터 종료 확인 후 빌드. GUN 그래프/에셋 자동 수정 금지, 에셋 업로드 금지 유지.
- UARTimeSubsystem/EARTimeGroup 및 ARTimeBlueprintLibrary 추가. World는 기존 엔진 글로벌 시간·Timer Manager, Player는 pause-aware 별도 시계·FTimerManager. PreActorTick에서 실제 유효 World 배율을 보상. ARPlayerCharacter 등록/해제 및 CustomTimeDilation 보정으로 캐릭터 Tick/Movement·회복 정상 진행. 구르기 쿨타임, 대상 플레이어의 스탯/실드/CC/슈퍼아머 기간과 HUD 효과 카운트다운도 Player 기준. 적/환경과 DOT·피해 이름 제한은 World. 게임 Pause는 양쪽 정지.
- FARSkillDefinition.CooldownTimeGroup 및 FARActionRequest.TimeGroup 기본 World. 쿨타임 시작/검사/UI/변경 모두 선택 시계 사용. Action Delay는 액션 그룹 상속. 새 SetGroupedItemTimerByEvent / SetGroupedActionTimerByEvent의 BP 표시명은 기존 Set Item/Action Timer by Event이며 TimeGroup 기본 World를 예약별 선택(액션 그룹 자동 상속 아님; Get Action Time Group 연결 가능).
- 새 타이머: Unreal Blueprint Set Timer by Event 기준 FirstDelay=Time+InitialStartDelay±Variance, 기본 Delay=0/Variance=0/MaxOncePerFrame=false. 같은 수명·Event 재등록은 교체, 다른 아이템/액션은 독립, Time≤0은 기존 이벤트 예약 제거. 비유한 값·그룹 무효·첫 지연 연산 오버플로 거절. 정상 종료/취소/아이템 해제/소유자 EndPlay에서 두 그룹 즉시 정리. 실행 중 자기 종료/해제도 테스트.
- 기존 SetItemTimerByEvent / SetActionTimerByEvent C++ 이름과 구현은 BlueprintInternalUseOnly 호환용 (Legacy)로 보존: 기존 World·-1/절대 첫 지연·호출마다 독립 예약 의미. 기존 그래프가 새 규칙으로 자동 전환되지 않음. 검색에서 새 노드를 배치하여 수동 교체할 것. GUN을 자동 변경하지 않았음.
- 그룹 제어 노드 Set/Get Time Group Rate, Get Time Group Seconds, Pause/Unpause/Clear Time Group Timer, Get Time Group Timer Remaining, Does Time Group Timer Exist, Is Time Group Timer Paused 추가. 핸들만으로 관리자 조회. 엔진 기본 핸들 노드는 Player manager를 제어할 수 없음. World배율 setter는 0.001~10 유한값만 받으며 실제 엔진 제한은 Getter 확인. 배율 복구는 자동 액션 귀속 아님; 중첩 슬로모션 정책·네트워크 복제·Chaos 플레이어 개별 시뮬레이션은 미구현.
- Docs/Guides/precautions/TIME_GROUP_GUIDE_KO.html 추가 + 메인/네비/18개 정규 가이드에 설명 연결. 타이머 기존 핀/재등록 설명 수정, 검색 카탈로그 397개(중복 subsystem 함수 노출 안 함), Legacy 숨김. 생성원 Scripts/update_time_group_guides.py + organize/generate scripts 변경. 재생성 멱등성 True.
- 정식 Development Editor 빌드 성공. 전체 AR 테스트 **68/68 Success, 실패0/리포트 경고0/미실행0**: Saved/Automation/TimeGroupsFinal/index.json, Saved/Logs/TimeGroupsFinal.log. 3개 시간 테스트는 rate 변경·pause·혼합 쿨타임·Action Delay·대상 효과 시계·첫 지연/variance·재등록/격리·삭제·숫자 경계 포함. 초기 소유자 삭제 검사 실패는 synthetic fixture가 PostInitializeComponents를 빠뜨려 RouteEndPlay를 건너뛴 원인으로 수정; 모든 안전성 테스트 fixture 생명주기 정확화. 기존 GUN GetInstanceId 미연결 컴파일 경고는 수정하지 않음. 실 플레이/물리 연출/패키징 테스트와 화면 QA는 별도.
- HTML 21개/로컬 링크·리소스1610개 유효, ID/마크업/JSON·JS 검사 및 타이머 문서/검색/접기 단위 테스트 통과, git diff --check 통과. 기존 사용자 Content 변경은 보존. 이번 작업 Git 커밋/푸시 없음. 이전 푸시는 저장소 공개 대상 승인 부족으로 차단된 상태; 추정해서 재시도하지 말 것.

## 2026-10-04 귀속 타이머 가이드 전반 통합 — 완료

- 사용자 요청: 새 아이템/액션 타이머를 제작 가이드와 주의점 HTML 전반에 반영하고 사용을 권장. 기존 구현된 Set Item Timer by Event / Set Action Timer by Event의 헤더·동작 기준으로 문서만 수정. C++/GUN/BP/DA/IA/IMC/레벨 미변경, 빌드·에디터 실행·Git 커밋/푸시 없음. 기존 게임 개발 변경과 사용자 Content 보존.
- 아이템 런타임 기본/스킬의 #managed-timers를 점검표 뒤 부록에서 본문 앞쪽으로 이동하고 목차 추가. 보유 효과는 Item Timer, 실행 스킬은 Action Timer를 권장. 적 액션에는 Action Timer만 핀 상세 안내(Item Timer는 적 Self에 사용 불가 명시). 기본/고급 핀, Custom Event/Create Event, 예약 직후 실행선 vs 지연 Event 콜백, Success/실패 출구, 정상 End Action, 실제 취소/해제, 중복 예약·실행별 저장 상태·유효 대상·Pause/Clear 설명. Action Delay는 순차 대기 분기 옵션으로 유지.
- 기존 '타이머는 자동 정리 안 됨'은 일반 엔진 Timer에만 해당하도록 수정. On Item Unregistered 전에 Item Timer Clear / 해제 중 신규 예약 거절, On Item Skill Cancelled와 On Action Cancelled 직하에 액션 예약 자동 정리와 변수·연출·구독·일반 Timer 수동 정리 경계. 런타임·스킬·적 전체의 소유권 표/증상/점검표도 갱신.
- DA→런타임 예약 안내, 공통 스탯/피해의 시간 만료와 예약 구분, DOT 자체 틱/피해 이름 제한에 BP Timer 불필요, 이미 적용한 일반 버프·DOT·독립 Actor/Permanent Flat은 예약 Clear로 회수되지 않음 명시. 적 이동/이동 주의점에는 상시 AI 판단 루프를 짧은 공격 액션에 묶지 말 것, CC 후 경로 재요청, 일반 Timer EndPlay 정리. 독립 스포너/픽업 Actor도 일반 Timer 예외로 안내. 아이템 검색·우선순위의 기존 조회/사전 검사 안내는 유지.
- organize_development_guides.py의 원본 템플릿/보강 단계와 generate_node_search_guide.py를 함께 갱신. 전체 노드 검색에서 신규 타이머 권장 용도·콜백 경계·관련 상세 앵커와 On Item Unregistered 설명 갱신. 신규 Scripts/test_timer_guide_references.py는 TOC/권장 흐름/적 Target 제한/AI·스포너 예외/DOT·스탯 수명 경계 회귀 검사.
- 검증: 20 HTML, 로컬 링크/에셋1527개, 중복 ID/태그/누락 링크 없음. 385개 노드 검색·필터·hash/인쇄, 공통 검색/펼침·접기 및 inline JS6개 구문 검사 통과. 타이머 내용 회귀 검사 통과. 생성기 재실행 HTML SHA 변화0(멱등성). 브라우저 화면 렌더링/실제 BP 실행은 이번 문서 작업에서 미검증.

## 2026-10-04 스탯 알림 안전성·귀속 타이머·입력 태그 — 완료

- 사용자 요청: 스탯 변경 이벤트 중 재변경 안전성 보강, 아이템/액션 타이머, 부하 검증 가능 여부, Q/W/E/R/A/S/D/F/G/Z/X/C 중 누락 입력 태그. 사용자 저장·에디터 종료 확인 후 구현·정식 Editor Development 빌드 성공. 개발용 통합 상태창(3번)은 설명만 했고 구현하지 않음. GUN 그래프·게임 에셋·레벨·IA/IMC 변경 및 Git 커밋/푸시 없음. 사용자 기존 Content 변경/삭제/신규 IA는 보존.
- ARStatsComponent: 네이티브 FScopedNotifications로 추가/제거/스택 소비/만료/기본값 변경의 캐시 업데이트를 먼저 완료. 대기 중 동일 스탯/출처 알림은 합치고 중첩 콜백 알림은 재귀하지 않고 현재 Broadcast 이후 큐에서 처리. 알림은 스냅샷으로 전달해 배열/맵 참조 무효화를 방지. 1회 flush 1024 알림 예산 초과는 경고 후 다음 컴포넌트 틱으로 이월(콘텐츠 무한 루프를 자동 수정하는 기능 아님). 유한 효과 만료는 관련 스탯별 한 번 재계산, 틱은 시간제 효과/대기 알림이 있을 때만 사용. 파괴/EndPlay 후 새 변경·남은 알림 차단. 소유자 없는 미등록 계산용 컴포넌트는 기존대로 지원; 모든 SetBaseStat 비유한 값 거절. 실제 무결성 전체를 보장한다고 주장하지 말 것.
- 액션/아이템 귀속 적용은 소유 핸들 기록까지 스탯 알림 보류. 액션 정리/아이템 다중 제거/획득·버림·진화·Loadout EndPlay 거래도 알림 배치. 아이템 제거는 재진입과 신규 효과·타이머·UI 생성 차단, 소유 핸들/UI 스냅샷을 분리해 반복 중 변경 안전성 보강. IsRegistered는 On Item Unregistered 중 false이며 Get Item Owner/기존 제거 경로는 사용 가능. 액션 EndPlay 중 새 액션 시작도 거절.
- 새 노드 **Set Action Timer by Event**(AR Action Component Target + Action Handle) / **Set Item Timer by Event**(ARLoadoutItemInstance Self). 매개변수 없는 Event, Time>0, Looping, Success + 실제 FTimerHandle 반환. 고급 Initial Start Delay=-1은 Time, 0은 다음 Timer Manager 처리, 양수는 지정 지연; -1 외 음수/비유한 값 거절. Max Once Per Frame 기본true. false Looping은 1회 지연이므로 별도 Item Delay 미추가; 기존 Action Delay 유지. 호출마다 독립 타이머이며 Tick에서 중복 시작 금지. 엔진 핸들 Pause/Unpause/Clear 호환. 액션 End/Cancel/Owner EndPlay, 아이템 해제/교체/정상 Loadout Owner EndPlay에 자동 Clear. 아이템 타이머는 스킬 취소만으로 멈추지 않음. 완료만으로 End Action하지 않음. 원래 일반 타이머/BP는 자동 전환하거나 수정하지 않음. 내부 ARLifetimeTimer는 weak callback/predicate를 쓰고 무효 콜백/수명은 정리, 실행 콜백 안 자기 종료/해제도 지원.
- Config/DefaultGameplayTags.ini에 **Input.Skill.W / A / S / D**만 추가. 나머지 요청 키 태그는 이미 있음. 키 매핑·기존 이동/상호작용·IA/IMC 변경 없음. 자동화에서 요청한 12태그 실제 등록 확인.
- 신규 ARLifetimeSafetyTests.cpp 7검사. 최종 **AR. 65/65 Success, 실패0/보고서 경고0/미실행0**, Saved/Automation/LifetimeSafetyVerified/index.json / Saved/Logs/LifetimeSafetyVerified.log. 추가 중 제거+256재추가, 알림 재귀/1100단계 예산 이월, 그룹 다중 스탯 캐시 원자 갱신, 액션취소+128액션 재할당, 아이템 해제, 등록 후 인벤토리 가시성, 만료 후 재적용과 틱, 콜백 소유자 파괴, 타이머 1회/반복/Pause/Unpause/Clear/CC/자기 종료/아이템 해제/Owner EndPlay/실행별 격리/첫 지연/프레임 따라잡기·무효값 검증. 최초 64검사에서는 ownerless 계산 컴포넌트를 신규 CanMutate가 거절해 기존 수식2검사 실패; 호환성 복원 후 전체 재검증. 최초 빌드의 잘못된 GuardValue include 제거 후 정상 빌드. 로그 시작 시 기존 GUN GetInstanceId 미연결 BP 경고는 남았고 에셋 수정 안 함.
- 합성 부하 검사: **32 적 × 20 사이클**, 시간제 보정 총10240, 액션/등록 히트박스/DOT/CC/액션 타이머 각640, 아이템 타이머20; 직접 피해640 및 동일 이름 즉시 중복640 거절, 잔여 효과/액션/등록 Actor/타이머 정리 확인. 렌더링 FPS·실제 NavMesh 주행·패키징·강제 GC/장시간 soak는 미검증. 단위 검사 실행시간을 게임 성능 상한으로 해석하지 말 것.
- 가이드 생성기와 HTML 동기화: 아이템 기본/스킬·적 액션·액션 생명주기의 #managed-timers, 스탯 수명의 #notifications, 취소 이벤트 직하 자동/직접 정리 경계. 전체 노드 검색385항목(호출141)에서 신규2노드 용도·Target·입력·기본값·반환 핸들 설명. 정적 QA20 HTML/1502 로컬 링크·에셋 및 검색/공통 가이드 JS 검사 통과. 실제 브라우저 렌더링 QA 미실행.

## 2026-10-03 간단한 제작용 경고 — 완료

- 사용자 승인 범위: DA 설정 검사 + AI 이동 실패 로그. 실행 차단·자동 수정·타이머/액션 자동 종료는 추가하지 않음. 게임 콘텐츠·GUN·맵 저장 없음. 에디터가 닫혀 있음을 확인한 뒤 정식 Editor 빌드 성공.
- ARItemDefinition의 WITH_EDITOR IsDataValid: 지원 유형/ID, 런타임 클래스 누락·추상·유형별 부모, 중복/빈 SkillId, 입력/비용/쿨타임, 아이템 기본 효과 PermanentFlat, 소모품에서 무시되는 스킬·기본 효과 경고. FDataValidationContext의 Warning + FUObjectToken 클릭 링크. 부모의 Invalid는 보존하되 신규 검사는 경고만 반환(Valid여도 경고 있음), 기존 획득·저장·게임 규칙 변경 없음. DA 우클릭 Asset Actions → Validate Assets, 저장 검증은 에디터 설정에 따름. InputTag/우선순위 공유는 허용, 액티브 슬롯의 빈 InputTag도 정상. 전체 카탈로그 ID 중복 검사와는 별개.
- ARAIController: NoPawn/NoMovementControl/InvalidGoal/InvalidDestination/NoNavMesh/StartOffNavMesh/GoalOffNavMesh/MoveFailed 안내를 LogARFoundation Warning에 객체·Controller 이름과 함께 기록. 같은 이유는 Controller별 최대 5초마다 한 번, 추가 NavMesh 진단 자체도 5초 간격. 기존 Super::MoveTo 결과·경로는 그대로. CC·사망·이동 잠금의 정상 거절과 AlreadyAtGoal/성공은 경고 제외. Shipping은 로그/추가 Nav 조회 제외. Cast/Target/실행선 때문에 노드가 호출되지 않는 경우와 수락 이후 장기 정체는 탐지하지 못함. Nav 투영 실패는 설정 확인 안내이지 특정 메시가 원인이라는 확정 진단은 아님.
- 전체 AR 자동화 **58/58 Success, 실패0/경고0/미실행0**. Saved/Automation/AuthoringWarningsVerified/index.json 및 Saved/Logs/AuthoringWarningsVerified.log. 신규 3검사: DA 경고+객체 토큰/값 미수정, 입력 공유·슬롯·비용·부모, 이동 로그 횟수·5초 재안내·이동 잠금 무경고·AlreadyAtGoal. 초기 테스트 코드의 TArray 내부 원소를 직접 Add하는 복사 오류와 목표를 같은 위치에 둔 잘못된 실패 가정을 수정한 뒤 전체 재검증. 제품 규칙은 변경하지 않음. 첫 빌드 NavAgentInterface include 경로도 실제 엔진 위치로 수정.
- 가이드: nsh/ITEM_ASSET_CREATION_GUIDE_KO.html#authoring-warnings 및 precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html#warnings. 이동 문서는 organizer 생성기와 동기화. 20 HTML/로컬 링크·에셋1493개 검사, 기존 가이드/검색 JS 모형 검사 통과. 실제 메시지 로그 UI·실제 맵의 시작/목표 Nav 투영 경고 화면 검증은 별도.
- 직전 이동 진단에서 저장된 Test_Level에는 NavMesh/병합 벽/적(label BP_TestEnemy2, 실제 BP_TestEnemy 클래스)이 있음. 이전 아래 기록의 '적/NavMesh 없음'은 당시 상태. 복사 PIE 월드에서 플레이어 test_Debug_body의 Nav 영향만 끄면 목표 투영 실패가 해소되고 이동 수락·이동을 확인했으며 원본은 저장하지 않았음. 진단 도구 LocalTools는 repo 밖이고 uproject 임시 연결 제거 후 정상 빌드로 복원.
- 이번 경고 작업 커밋/푸시 없음. 직전 푸시 요청은 대상 저장소 확인 단계에서 차단됨. 사용자 요청으로 미푸시 커밋을 0e69e10(가이드·스크립트만)으로 수정했고 Content는 로컬 미커밋 상태로 보존. 에셋 업로드 금지 유지. 새 푸시 권한을 추정하지 말 것.

## 2026-10-03 BP_TestEnemy 이동 예제 네 종류 — 완료

- 사용자가 BP_TestEnemy1이 아닌 BP_TestEnemy를 지목. 방향 이동, 좌표 직선 이동, Actor NavMesh 이동, 좌표 NavMesh 이동을 설명용으로 보여주되 Actor NavMesh만 실제 실행선 연결하도록 요청했다. 기존 사용자 Enemy_Attack/Enemy_Event 유지. 사용자 삭제 BP1/2/_/NewBlueprint 복구하지 않음.
- 실제 원인: Enemy_Move의 ARMoveToActor Target에 연결된 impure Cast To ARAIController의 execute가 미연결되어 컴파일 시 정리됨. 범위 Branch False → 재요청 0.25초 게이트 → Get Controller/Cast 실행 → AR AI Move To Actor → LastPathRequestResult 저장으로 수정. AcceptanceRadius=0. 기본 방향/직선 위치/AI 위치 예제는 진입 실행선 미연결이며 내부 참고용 흐름만 연결. 네 설명 박스/변수 추가. Tick 도달성 검사에서 Basic 2/Actor 1/Location 1, Actor만 실행됨.
- 이 BP만 CharacterMovement 평면 고정과 시작 평면 스냅을 해제해 캡슐 중심을 Z=0에 묻지 않도록 함. AIControllerClass=ARAIController, AutoPossessAI=PlacedInWorldOrSpawned 확인. 공격 XY 300cm/1초 액션 준비/현재 공격력 비례/취소/그로기·사망 기존 흐름 유지. 현재 Enemy_Event는 직접 이벤트 + ApplyCrowdControl + Delay + ResetGroggyGauge, 예전 Bind/DA 흐름이 아님. 해당 설명을 TEST_ENEMY_BLUEPRINT_KO.md에 갱신.
- 수정 직후·저장 재로드 각각 **12/12 실행 검사**, 저장 후 재컴파일 오류0/경고0. 첫 수정 commandlet 프로세스는 수정 전 BP의 기존 Cast 오류를 로드할 때 기록해 exit1이지만 수정 컴파일/검사/저장은 성공. 새 프로세스 VerifyMovement는 exit0/오류0/경고0으로 검증. 테스트 월드에는 NavMesh 없음: 경로 요청 Failed 기록/설명용 입력 미실행/재요청간격/3m공격/중복방지/1초선딜/공격력/취소/그로기/CC해제/사망 검사. 실제 장애물 우회 주행·PIE 시각 QA 미실행.
- 저장된 Test_Level에는 NavMeshBoundsVolume/RecastNavMesh와 적 인스턴스가 없음. 맵 수정은 하지 않음. 사용자가 Nav Mesh Bounds Volume과 적을 배치해야 실제 추적 가능. 전체 Content 작업 전후 SHA256 비교: BP_TestEnemy만 변경, 모든 다른 파일 동일. uproject 임시 AdditionalPluginDirectories 원복(diff0), 정식 Editor 빌드 성공. 도구 없는 Python 검사 exit0/오류0/경고0, BP 독립 컴파일/AI 설정/평면 설정 확인.
- 생성·검사 도구는 repo 밖 LocalTools/EnemyBlueprintAuthoring, EnemyMovementShowcase.inl과 verify_movement_detached.py. 게임 C++ 추가·수정 없음, 외부 플러그인 의존 없음. 백업 Saved/EnemyMovementBackup/20261003_190524/BP_TestEnemy.uasset. 로그 EnemyMovementPatch/EnemyMovementReload/EnemyMovementDetached 및 BuildEditor.log. BP 변경은 커밋·푸시하지 않음.
- export 비교에서 Enemy_Event 블록은 바이트 동일. Enemy_Attack 실행 노드/연결은 동일하며, 재컴파일 과정에서 기존 미연결 MakeStruct 잔여 블록만 제거됨(88줄 삭제만, 신규·변경 줄 없음). 실행 흐름은 작성 도구에서 수정하지 않았다.

## 2026-10-03 공통 가이드 전체 노드 검색 — 완료

- `Docs/Guides/common/BLUEPRINT_NODE_SEARCH_KO.html` 추가, 메인 및 공통 분야 버튼 연결. 현재 Source/Action_RogueLike 헤더의 Blueprint 노출 선언에서 직접 추출한 **383항목: 호출139/비동기1/조회111/구현 이벤트35/디스패처43/구조체54**. 엔진·플러그인 전체 기본 노드나 에셋별 Custom Event/Get/Set 전수 사전은 아님을 페이지에 명시. 함수/이벤트의 표시명·종류·Target·용도·핀 방향·기본값·고급/자동 문맥·구조체 내부 필드·열거형26개 선택값과 원본 연결 제공. 동일 이름은 대상 클래스별로 분리. 호환/숨김 선언은 기본 제외하고 체크 시 표시.
- 오프라인 node-search.js/css: 이름만/용도·핀 포함 검색, 띄어쓰기·대소문자 무시, 종류/분야 필터, 40개씩 추가 표시, 노드 펼침·접기, 깊은 링크와 인쇄 상태 복원. 이름 정확 일치 또는 유일 결과는 자동 펼침. 반환값 있는 BP 오버라이드와 void 이벤트의 핀 방향 구분. 액션 취소 알림의 별도 Timer 정리 책임 유지. Make/Break 항목은 자동 파생 데이터 사전이며 읽기 전용 핸들 임의 조립 금지 안내.
- `Scripts/generate_node_search_guide.py`와 `test_node_search_guide.js` 추가. organizer에 생성 연결해 재실행 시 새 검색 페이지/내비게이션이 유지됨. 현재 소스에서 노드/구조체/enum을 추출하며 미해석 노출 선언은 실패 처리. Saved/GuideQA/node-reference.json은 검사 산출물. 함수 핀 설명/용도 보강에는 기존 한영 생성기의 한글 설명 사전을 재사용하되 오래된 카탈로그 핀 목록은 사용하지 않음.
- 정적 QA **20 HTML, 로컬 링크/에셋1492개 오류0**, 검색·필터·호환 선택·페이지 추가·hash·인쇄 복원은 비렌더링 DOM 모형 검사 통과. 기존 가이드 JS 검사도 통과. 실제 브라우저 화면/레이아웃 QA는 미실행. 게임 코드·BP·DA·레벨 변경 없음, 빌드 불필요. 이번 작업은 커밋·푸시하지 않음.

## 2026-10-03 전체 개발 변경 Git 반영 — 푸시 전 검증

- 사용자가 가이드뿐 아니라 기존 게임 개발 변경까지 모두 커밋·푸시하도록 명시 승인했다. 기존 C++/에디터 편집 모듈/게임 에셋/입력 에셋 이동·삭제/가이드와 개발 스크립트를 함께 반영한다. 프로젝트 밖 `LocalTools/GunBlueprintAuthoring`의 GUN 생성 도구는 포함하지 않으며, Python 캐시는 `.gitignore`로 제외한다.
- 푸시 직전 정식 Editor Development 빌드 성공(최신 상태), 전체 `AR.` 자동화 **55/55 Success, 실패·경고·미실행 0**. 결과: `Saved/Automation/PrePush/index.json`, `Saved/Logs/PrePushAutomation.log`. 가이드 검사19 HTML/로컬 링크518개 및 JS 구문·모형 상호작용 검사 통과. Git LFS 무결성 검사 통과. GUN 4개 에셋 SHA256은 기존 기록과 동일하며 이번 Git 작업에서 어떤 게임 에셋도 수정하거나 저장하지 않았다.
- 아래 각 작업의 “커밋/푸시 없음”은 해당 작업 종료 당시의 과거 기록이다. 실제 커밋·원격 반영 여부는 Git 이력을 기준으로 확인한다.

## 2026-10-03 개발 참고서 포털·분야별 HTML — 완료

- 사용자 요청에 따라 Docs/Guides/가이드 메인 페이지.html 메인 + 독립 참고서16개(공통3/아이템3/적3/환경3/주의점4) 구성. 기존 nsh/hhc 유지, common/environment/precautions/assets 추가. 공통 스탯/피해는 common으로 이동하고 루트의 이전2 HTML은 새 위치+hash 연결용 진입 페이지로 유지. 원래 내용 전체를 잃지 않도록 상세 블록을 재사용했으며 기존 상대 링크/분리한 적 앵커를 갱신. 기존 Foundation 문서의 루트 링크는 진입 페이지로 계속 사용 가능.
- shared guide.css에 상단 분야 이동·동일 분야 문서 버튼·줄바꿈·좁은 화면 레이아웃·인쇄 대응. guide.js는 새 문서와 기존 적 전체/아이템 에셋/런타임 기본에 공통 접이식 검색·초기화·펼치기/접기·hash 상위 펼침·인쇄 상태 복원. 기존 스탯 동적48/47 목록 필터와 기존 피해/아이템 스킬 핀 UI·검색은 보존. Guide main은16문서 카드와 분야 바로가기. 모든 canonical 페이지는 공통 탐색 바 정확히1개.
- CC 문서는 간편 Apply Crowd Control·From Result 경직·저항/면역·상태 제거를 다룸. 스탯 둔화는 Spec Affected By Tenacity 체크만 짧게 설명하고 STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html#tenacity로 연결. 그로기 상세는 적 전체 #groggy 안내. 아이템 On Item Skill Cancelled와 적 On Action Cancelled의 용도 바로 아래에 별도 Timer Handle 저장/Clear and Invalidate Timer by Handle 등 직접 정리 책임 추가. 실제 취소 알림 기준이며 모든 경직이 무조건 취소된다고 설명하지 않음.
- SKILL_PRIORITY_GUIDE_KO.html은 입력 키/IA/InputTag/슬롯 설정 비교를 다루지 않음. 실제 ExecuteSkillCandidates 기준 작은 숫자부터·같은 숫자 원자 묶음·MP/SP 자원별 누적 비용·조건 하나 실패/비용 부족 시 해당묶음과 이후중단·앞서통과묶음만실행 설명. 최종 액션 거래 실패 롤백·Execute 이후 콘텐츠 실패의 비용/쿨타임 환불 없음도 명시. 스탯 상세는 실제 enum Independent Damage Reduction(Override 아님), Permanent Flat, 필드/소유권/제거/Count 스택 소비까지 포함.
- 적 전체 가이드13장15노드: 전체 흐름·대상·기본설정·그로기/회복/사망/실드 이벤트 유지. 이동5장8노드/NavMesh 생성(공식 Epic 안내 링크·P 표시·바닥/충돌/Agent 확인)과 직접 액션4장14노드를 hhc로 분리. 전체에는 버튼형 링크가 있는 짧은 장을 남김. DA 자기 기절 상세는 없애 간편 CC를 권장. 피해 저수준 HitContext 조립 중복 설명은 없애 From Result를 우선하고 DOT Template 입력은 유지. 소유권/목적 다른 적용·제거 노드는 중복으로 취급하지 않음; 어떤 게임 API도 삭제하지 않음.
- 환경: 적 Spawn Actor·단일확률/가중치 조합·재시도/생존수, 아이템 Find Key/AdditionalTag·정확타입/문자열라벨·DA 반환·카탈로그 범위/비용·소유 제외·Shuffle N개, World Subsystem→Try Spawn Loadout Pickup 및 소모품/여러 외형 Class/Definition Assigned 이벤트. 새로운 스폰/검색 기능을 구현했다고 주장하지 않고 현재 코드와 엔진 조합을 설명함.
- Scripts/organize_development_guides.py(기존 HTML 재구성+신규 문서 템플릿), validate_development_guides.py, test_guide_controls.js 추가. 신규 생성 문서 수정은 생성기와 동기화; 기존 문서는 HTML 유지/보강 방식. 생성기 재실행 SHA 비교 변경0. 정적 QA **19 HTML(메인+16참고서+2이전주소), 로컬 링크/에셋518개, 누락0/중복ID0/태그오류0**, JS6개 구문검사. 렌더링 없는 DOM 모형 단위검사로 접힌내용검색/없음/초기화복원/펼침·접기/hash/인쇄복원 통과. Saved/GuideQA/inline-scripts.json은 검사 산출물.
- Browser Use의 file:// 요청이 보안 정책으로 거절되어 우회·로컬서버·다른 브라우저·CDP를 사용하지 않음. 실제 화면/폭/브라우저 상호작용 검증은 미실행이며 정적+모형 검사만 완료. 컴퓨터 사용 스킬 안내를 읽고 Browser Use를 우선했으나 정책 차단. 게임 C++/BP/DA/레벨 미변경, 이번 문서 요청에는 빌드 불필요. GUN 그래프 변경 금지 유지. 커밋/푸시 없음.

## 2026-10-03 Money · Permanent Flat — 완료

- 사용자 승인대로 EARStatModifierOperation에 PermanentFlat=4(DisplayName Permanent Flat)를 추가. 기존 Flat/AdditivePercent/Multiplicative/Override 번호 유지. EARStatType Money=47, Count=48; 기존 스탯 번호 유지. Money는 ARPlayerCharacter 및 자식만 지원하며 기본값0. 플레이어 스탯 목록48개, 적/환경47개. Money는 float로 소수 유지하며 UI/소비/획득 로직을 자동 추가하지 않음.
- 일반 Apply Stat Modifier에서 Permanent Flat을 선택하면 기본 스탯에 Value를 직접 더함. 양수 증가/음수 감소, 버프 레코드·타이머·출처·HUD·제거 핸들 없음. 성공해도 Return Value는 빈 핸들이므로 Success로 확인. 기존 임시 버프/퍼센트/곱연산 및 최종 안전 제한은 유지. Duration/Source/HUD/강인함/Stack Group 무시, Stack Only/보장 무적·회피 플래그와 조합은 실패. 해당 항목의 편집 조건/가이드 추가. 캐릭터 생존 수명 내 유지이며 저장·새 객체·리셋 간 이전 기능이 아님.
- ApplyPermanentFlat은 C++ 내부 API이며 Blueprint 새 별도 노드가 아님. 컴포넌트 Add Stat Modifier, 아이템/액션 귀속 노드, DA Default Stat Modifiers에서는 Permanent Flat 거절하여 되돌릴 수 없는 귀속 수정을 방지. 아이템에서는 Get Item Owner를 일반 Apply Stat Modifier Target에 전달해야 함. Money 차감은 기본 잔액 기준이고 부족하면 원자적으로 실패, 음수 기본 잔액 불가. 임시 Money 버프는 기본 잔액 부족을 보충하지 못함. 비유한 수/범위 밖 스탯/합산 overflow 거절. 잘못 작성된 시작 Money 음수·비유한 값은 BeginPlay에서0으로 정규화.
- 정식 Editor 빌드 성공. 신규 ARPermanentStatTests.cpp 3개 및 편집 조건 테스트 포함 전체 **AR. 55/55 Success, 실패0/경고0/미실행0**. Saved/Automation/PermanentFlatMoney/index.json / Saved/Logs/PermanentFlatMoney.log. 반복200회 성장 시 버프 레코드와 틱 미생성, 제거/아이템 해제/액션 종료 이후 유지, 기존 배율·하한, 돈 소수/초과 차감/플레이어 한정/시작값 정규화 및 귀속 API 차단 검사.
- 저장 콘텐츠14BP/2ItemDA 읽기·메모리 컴파일 오류0. Saved/Logs/PermanentFlatMoneyContent.log. GUN 기존 GetInstanceId 실행 미연결 경고2건 유지. 최종 시작값 안전 처리 전 콘텐츠 컴파일을 수행했으며, 이후 반사 API 변경 없음. 어떤 BP/DA/레벨도 저장하지 않음. DA_GUN_R_A/BP_GUN_Projectile/BP_test1_R_A SHA256 작업 전후 동일. GUN 그래프 변경 금지 유지.
- OBJECT_STAT_GUIDE_KO.html #permanent-flat, nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html #permanent-flat, ITEM_ASSET_CREATION_GUIDE_KO.html 및 Foundation 계획/구조/카탈로그/인계/HTML 생성기 동기화. 레퍼런스 스탯48/노드263. 세 가이드 링크·태그·ID·JS 구문 및48/47 스탯 필터 정적 QA 통과. 화면/GUI PIE 검증은 미실행. 아이템 검색/픽업 소환 추가 구현, 돈 HUD, 저장 시스템, 기존 적 이동 문제는 범위 밖. 커밋/푸시 없음.

## 2026-10-03 DA 없는 간편 CC — 완료

- 사용자 요청으로 `Apply Crowd Control`(AR|Combat|CC) 추가. Target Actor, CC Type(Stun/Root), Duration 기본1초/양수, Affected By Tenacity 기본true. Success와 상태 핸들 Return Value; Source/Failure Reason/Applied Duration은 고급 핀. enum EARCrowdControlType 신규. 기존 DA/CC 면역/해제 API 유지, 어떤 BP/DA/레벨도 자동 교체하거나 저장하지 않음.
- 요청별 RF_Transient UARStatusEffectDefinition을 Status Component outer로 생성, 기존 ApplyStatusEffect로 전달. 활성 효과의 reflected 참조가 수명 소유; 에셋/CDO 변경 없음. Stun 전체 이동/롤/새 스킬 차단과 취소 규칙에 따른 Stun 취소, Root 기본 이동/롤 차단 및 기존 롤만 취소(스킬 허용). 면역·강인함·같은 태그 긴 시간 갱신·동일 핸들·기존 On Status Removed 재사용. Duration 음수는 이 노드에서 무효(DA fallback 아님), dead/무효 대상 거절.
- 정식 Editor 빌드 성공. 신규 ARCrowdControlTests.cpp 3개 포함 **AR.Foundation 51/51 Success, 실패0/경고0/미실행0**. Saved/Automation/SimpleCrowdControl/index.json / Saved/Logs/SimpleCrowdControl.log. 첫 검증의 강제 CollectGarbage는 이전 테스트 월드의 미회수 아이템 EndPlay/BP콜백 충돌로 assertion; 호출을 제거했고 GC 강제 검증 완료를 주장하지 않음. 이어서 테스트 월드 NAME_None 충돌 및 큰 world tick delta clamp 발견, GUID 월드명과 작은 프레임 진행으로 테스트만 수정 후 전체 통과. 제품 상태 시스템 변경 없이 새 라이브러리 wrapper와 enum만 추가.
- 저장 콘텐츠 컴파일 검증 성공(저장 없음): Saved/Logs/SimpleCrowdControlContent.log, 0 errors. GUN의 기존 GetInstanceId 실행 미연결 경고2건은 그대로이며 요청대로 수정하지 않음. DA_GUN_R_A/BP_GUN_Projectile/BP_test1_R_A의 SHA256은 작업 전후 동일.
- 적 참고서 실제 경로 Docs/Guides/hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html #apply-crowd-control 신규 카드(13챕터41foldouts, 로컬 링크/태그/ID/JS 구문 검사 통과). 기존 자기 기절 DA 카드 유지. Foundation catalog/설계문서/생성기/HTML(263노드)/테스트 적 사용문서 동기화. 외부 validate_enemy_guide.py 기대개수41로 수정. UI/PIE 확인은 미실행; 새 검색 이름/기본핀은 reflected automation 확인. 사용자 편집 테스트 적1/2의 이동/NavMesh/평면 원점 문제는 이번 요청에 포함하지 않아 미수정.

## 2026-10-02 Character 직접 이벤트·실드 파괴·회복 참고서 — 완료

- 사용자 저장·종료 확인 뒤 정식 Editor 빌드 성공. ARBaseCharacter의 직접 Blueprint 이벤트 6개: On Character Death(KillingDamage), On Groggy Gauge Depleted, On Shield Broken(PreviousShield/Reason), On Damage Applied(DamageResult), On Action Cancelled(ActionHandle/Reason), On Status Removed(Status). ARBaseEnemy/Player 자식에서 바로 놓는다. 등장/스폰 초기화는 기존 Event BeginPlay. 구독을 Super::BeginPlay 앞에서 준비하여 BP BeginPlay 중 처리도 전달한다. 사망은 기존 액션·이동·상태 정리 뒤, 상태 제거는 이동 잠금 갱신 뒤 전달한다. 기존 디스패처/Bind 유지; 같은 효과 중복 연결 금지.
- Health OnShieldBroken(BlueprintAssignable) / Native 추가. 사용자 확정대로 총 실드 양수→0의 피해 소진 또는 수동 제거만 알림; 시간 만료·Ignore Shield·부분/실패/빈 제거 제외. Damage 또는 Other, 직전 총합 PreviousShield. Changed 콜백 전에 전이 스냅샷, Changed 뒤 Broken 전달. 배치 제거 한 번은 한 번. 콜백이 새 실드를 추가해도 원래 전이 알림 유지.
- ARCharacterEventTests.cpp 신규 3개 포함 AR.Foundation **48/48 Success, 실패0/경고0/미실행0**. 첫 실행 DirectEvents의 const-ref 구조체 테스트 thunk가 값 읽기 매크로를 사용하여 2검사 실패; 테스트만 P_GET_STRUCT_REF로 수정/재빌드 후 전체 통과. Saved/Automation/CharacterDirectEvents/index.json 및 Saved/Logs/CharacterDirectEvents.log. 실드 수동/피해/만료/우회/배치/재진입, 실제 Character 콜백/액션 취소/그로기/정확한 상태 핸들/사망 정리, Restore의 최대치·무효·미소유 컴포넌트·부활 불가 검증.
- 기존 Restore Health/Mana/Stamina API는 새 중복 노드를 만들지 않고 피해 가이드 #healing에 용도→개별 핀→Source 펼침→실제 회복량 반환 설명 추가. 현재값을 최대치까지만 더하며 최대 스탯 변경/부활/RecoveryPower 자동 배율이 아니다. 기본 적은 Mana/Stamina 컴포넌트 없음. 피해 가이드 #shield-broken에 직접 이벤트/디스패처와 파괴 조건 추가.
- 적 HTML 실제 경로는 사용자가 이동한 **Docs/Guides/hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html**. 직접 이벤트/실드 파괴/회복 항목 추가(13장/40펼침), 링크 깊이 수정. 피해 가이드 11장/45펼침. 두 가이드 HTML 균형·중복ID·모든 파일/앵커와 JS 구문 확인. 브라우저 file:// 정책 거절은 우회하지 않았으며 화면 QA 미검증. API 카탈로그/3설계 문서/HTML 레퍼런스 및 생성기 갱신.
- 저장 콘텐츠 12BP/2ItemDA 읽기·컴파일 성공, 저장 없음. Saved/Logs/CharacterDirectEventsSavedContent.log. 오류0, 기존 GUN GetInstanceId 미연결 경고2회(1종)는 그대로 유지. GUN 4에셋 SHA256 작업 전후 동일. 기존 테스트 적의 Bind 그래프도 교체/저장하지 않았고 레벨/플레이어/GUN 에셋 변경 없음. 외부 검사 도구 LocalTools/EnemyBlueprintAuthoring 유지, 게임 소스에 제작 도구 없음. GUI PIE·커밋·푸시 없음.

## 2026-10-02 적 제작 HTML 참고서 — 작성 완료

- 사용자 요청: 적 제작 가이드를 이전 개발 참고서 형식으로 작성. Docs/Guides/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html 신규 작성, 기존 아이템 스킬 문서의 다크 테마/목차/검색/접이식 항목/전체 펼침·접기/인쇄 구조 재사용. 13장, 37개 접이식 노드 설명. 용도→Target→입출력→호출 시점/주의 순서, Action Request/기절 Definition은 제자리 하위 펼침.
- 실제 ARBaseEnemy/Character/AIController/MovementControl/Action/ActionDelay/Stagger/Status/Hitbox/Combat API 기준. Actor와 아이템 런타임 차이, 컴포넌트 캐시, 대상/XY·Z 정책, 직선/경로/액션 이동, 직접 액션 비용·쿨타임·중복 방지 책임, 성공 핸들/종료/취소, 피해 결과→경직·그로기, 단일 원 판정 vs 다중 AoE, 액션 귀속 vs 독립 투사체, 그로기 자기 기절/정확한 StatusHandle 복구, 사망/정리/스폰·확률 및 점검표 포함. 특정 유물 고정 예제·새 게임플레이 코드 없음.
- 피해/아이템 스킬 HTML 상단과 테스트 적 MD에 새 문서 링크 추가. 아이템 스킬 상단 피해 링크가 nsh 내 잘못된 위치를 참조하던 것은 ../DAMAGE_NODES_FORMULA_GUIDE_KO.html로 수정. 나머지 기존 내용 유지.
- 정적 검사 13장/37펼침/52링크/56유일ID, 태그 균형/파일·앵커/JS 구문/diff whitespace 통과. 검증 스크립트는 외부 LocalTools/EnemyBlueprintAuthoring/validate_enemy_guide.py에 보관. Browser Use가 file:// 열기를 프로토콜 정책으로 거절하여 우회/다른 브라우저/로컬서버를 시도하지 않았다. 화면/인터랙션 QA는 미검증이며 정적 검사만 완료. 문서 작업이므로 게임 코드/에셋 수정·빌드 없음. 커밋/푸시 없음.

## 2026-10-02 테스트 적 Blueprint — 제작·검증 완료

- 사용자 요청: 플레이어 추적 → XY 3m 이내 정지 → Try Start Action/Action Delay로 1초 선딜 → 3m 원 판정 공격, On Character Death로 삭제, 그로기 소진 시 3초 자기 기절. 피해는 적 최종 공격력 비례. 별도 클래스/객체 선언 없이 새 BP를 레벨에 배치하면 된다.
- 신규 `/Game/Game/Objects/Characters/Enemies/Test_1/BP_TestEnemy`(ARBaseEnemy 자식)와 `Data/DA_TestEnemy_GroggyStun` 저장. 일반 K2 노드만 사용하며 전용 게임플레이 C++는 추가하지 않았다. Enemy_Tracking/Enemy_Attack/Enemy_Lifecycle 그래프로 구분. Player Pawn 0을 추적하고 RequestBasicMove로 직선 이동(장애물 우회/NavMesh 없음). 원 공격은 이 단일 플레이어의 현재 XY 거리만 판정하며 다른 대상들을 스캔하지 않는다. Z 제외. 공격력10/이동속도200/MaxGroggy100/회복0 기본 설정, 계수1/BaseDamage0/직접 물리 피해/치명타 끔. 별도 공격 쿨타임 없이 각 공격에 1초 준비.
- TryStartAction 성공 시 핸들 저장/일반 이동 차단. ActionDelay 완료 후 유효한 생존 대상·거리 재확인, 맞음/빗나감 모두 EndAction. 취소 시 지연 타격 없음. 선딜 개발용 빨간 DebugCircle 제공. GroggyGaugeDepleted에서 자기 Stun DA 적용(강인함 감소 끔), 정확한 저장 StatusHandle 제거 시 게이지 최대치 복구. 사망은 DestroyActor. 전체 CC 면역 추가 시 자기 기절도 거절될 수 있다는 문서 주의사항 유지.
- 외부 제작 도구는 `C:/Users/ghksd/Desktop/game_dev/LocalTools/EnemyBlueprintAuthoring`에만 있다. 임시 AdditionalPluginDirectories 제거 후 .uproject 내용 작업 전으로 복구, 정식 빌드 성공. 도구 DLL/PDB도 프로젝트 Binaries에서 외부 ProjectBuildArtifacts로 이동(복구 가능), 모듈 manifest의 임시 항목 제거. 정상 target/uproject/manifest에 도구 의존성 없음. Source에는 도구를 넣지 않았다.
- 생성 직후와 저장 후 -Verify 각각 21/21 그래프 실행 검사 통과/새 BP 컴파일 오류0 경고0. Saved/Logs/TestEnemyAuthoring.log, TestEnemySavedVerify.log 참조. 외부 플러그인 없는 Python 검사에서 새 BP/DA 기본값 확인, 콘텐츠 10BP/2ItemDA 읽기·컴파일 성공/저장 없음. 기존 GUN GetInstanceId 미연결 경고2회(1종)는 사용자 지시로 유지. Saved/Logs/TestEnemySavedContent.log 참조. 검증 스크립트 최초 Python class API 오용은 스크립트만 수정 후 재실행 성공.
- AR.Foundation 45/45 Success/실패0/경고0/미실행0, Saved/Automation/TestEnemyFoundation/index.json. GUN 4 uasset 작업 전후 SHA256 동일. 기존 레벨/플레이어/GUN 저장·수정 없음. Docs/Guides/hhc/TEST_ENEMY_BLUEPRINT_KO.md에 설정·사용·제약·검증 안내. GUI PIE·실제 바닥/충돌/이동 외형은 미검증이며 사용자가 레벨 배치 후 확인해야 한다. 커밋·푸시 없음.

## 2026-10-02 피해 결과 직접 연결 경직·그로기 노드 — 빌드·검증 완료

- 사용자 요청: Apply Combat Damage의 Return Value를 경직·그로기 노드에 바로 연결하고, Applied 분기/Break/Make Request는 개발자가 조립하지 않도록 한다. UARCombatBlueprintLibrary에 `ApplyStaggerAndGroggyDamageFromResult`(표시 이름 `Apply Stagger And Groggy Damage From Result`) 추가. Damage Result와 개별 Base Stagger Damage=0 / Stagger Multiplier=1 / Base Groggy Damage=0 입력, Source(미연결 허용)와 Effect Name=None은 고급 핀이다. 흰 실행선과 Return Value→Damage Result 두 연결만 하면 된다. 기존 Request 기반 함수·컴포넌트 노드·저장 그래프는 그대로 유지했다.
- 내부에서 WasApplied + HitContext.IsUsable을 확인해 Invalid/Queued/Evaded/Blocked(이름별 간격 차단 포함) 결과는 기본 FARStaggerResult 반환/무처리. Applied 결과의 Hit Context와 수치를 기존 경직 컴포넌트 처리에 위임하며 공식은 중복하지 않는다. HP를 재적용하지 않고 타격 당시 스탯 스냅샷을 유지한다. 실드-only 성공 타격도 허용. 대상 소멸/컴포넌트 없음은 안전하게 거절. Queued 완료 자동 추적이나 같은 Hit Id 재사용 자동 중복 차단은 추가하지 않았다.
- DOT는 이미 Spec.bApplyStaggerAndGroggyEachTick + StaggerTemplate으로 성공 틱에만 적용하므로 코드는 변경하지 않았다. 반환값은 Dot Handle이지 Damage Result가 아니다. 새로운 Combat.DotStaggerOption 회귀 테스트로 체크 해제/체크/병렬 같은 이름 차단/긴 간격 차단/피해0 틱을 검증했다. Combat.StaggerFromDamageResult는 reflected callable/default pins, 실제 직접 결과, 모든 실패 outcome(유효 context 잔존 포함), 이름 제한, 무효 context/ID/target, 타깃 소멸, 스냅샷·배율, 슈퍼아머·저항, HP 미재적용 및 실드 성공을 검증했다.
- 사용자 저장·종료 확인 후 Scripts/build_editor.ps1 정식 빌드 성공. AR.Foundation **45/45 Success, 오류0/경고0/미실행0**. Saved/Automation/StaggerFromResult/index.json / Saved/Logs/StaggerFromResult.log 참조. 기존 9BP/2DA 읽기·컴파일 검증 성공, 오류0/기존 GUN GetInstanceId 미연결 경고2회(1종), Saved/Logs/StaggerFromResultSavedContent.log. 에셋은 저장하지 않았다.
- 피해 HTML 가이드에서 #apply-stagger를 새 간편 노드로 안내하고, 기존 경직 노드는 #apply-stagger-context로 별도 유지(기존 구조체/결과 anchor 유지). 노드 선택표/개별 핀/내부 무시 조건/DOT 체크/중복 호출 주의 및 시스템 계획·BP 계획·코드 구조·카탈로그·HTML 참조/생성기 설명을 갱신했다. GUN 4 uasset의 작업 전후 해시 동일. GUI 에디터 재개·커밋·푸시 없음.

## 2026-10-02 피해 이름별 재피격 무시 — 정식 빌드·검증 완료

- 사용자 확정: 동일 DamageName 피해를 기본 0.2초 동안 무시, 요청별 간격 지정 및 무시 체크 필요. 기존 FARCombatDamageRequest에 DamageNameInterval=0.2f / bIgnoreDamageNameInterval=false 추가. Make Request/핀 분할에서 사용하며 별도 피해 노드·콘텐츠 BP 타이머가 필요 없다. 간격은 EditCondition으로 무시 체크 시 편집 비활성이다.
- 기준은 대상 + DamageName이며 공격자·출처·속성·Direct/DOT 모두 공유한다. Applied 성공 직후, Health/Combat 콜백 전에 다음 허용 시각을 기록한다. 제한 중 요청은 Blocked/Cooldown, 피해0/무효 HitContext/온히트·흡혈 없음, OnDamageBlocked 알림만 전달하며 나중에 재적용하지 않는다. 회피·무적·피해0·무효 요청은 제한을 시작하지 않는다. 기존 연쇄 피해 큐를 통과한 재진입 요청도 실제 처리 시 다시 제한된다.
- None은 미지정이므로 제외한다. 간격0/무시 체크는 해당 요청의 검사·기록만 생략하며 기존 제한을 삭제·연장하지 않는다. 유효 제한의 음수·NaN·무한 값은 InvalidDefinition. 이전 성공 요청의 양수 간격은 다음 요청의 짧은 양수 값으로 단축하지 않는다. 공허·필중도 이름 제한을 우회하지 않는다. 약한 대상 참조 맵을 사용해 만료·대상 소멸 및 월드 Deinitialize 시 회수한다.
- DOT의 중첩/갱신 등록과 별개로 실제 틱 피해를 제한한다. DueTicks에 예정 시각을 저장하고 StableSort 후 그 시각으로 검사해 지연된 정상 간격의 틱을 한 프레임 시각으로 몰아 차단하지 않는다. Interval보다 촘촘한 틱·같은 이름 병렬 틱은 의도적으로 차단한다. 갱신 정의 비교에 DamageName/간격/무시 체크 추가(무시 시 비활성 간격 값 제외).
- 사용자 저장·종료 확인 후 Scripts/build_editor.ps1 정식 빌드 성공. AR.Foundation **43/43 Success, 오류0/경고0/미실행0**. 새 Combat.DamageNameInterval 및 DotDamageNameInterval 포함. 최초 검사는 일반 회피 100 설정이 Stats 상한에 걸려 확정 회피가 아닌 테스트 설정 오류로 42/43이었고, 관리형 확정 회피 및 무적 검사로 수정 후 재빌드·전체 재검사 성공했다. Saved/Automation/DamageNameIntervalFinal/index.json / Saved/Logs/DamageNameIntervalFinal.log 참조.
- 저장 콘텐츠 9BP/2DA 로드·컴파일 성공, 오류0/기존 GUN GetInstanceId 미연결 경고2회(1종). Saved/Logs/DamageNameIntervalSavedContent.log 참조. 어떤 에셋도 저장하지 않았다. GUN 4 uasset 작업 전후 SHA256 동일. 피해 HTML 가이드의 직접/DOT 입력과 공식·용어·알림, 시스템/블루프린트 계획·코드 구조·노드 카탈로그·HTML 참조를 갱신했다. 문서 HTML 태그 균형/중복ID0/로컬앵커오류0 확인. GUI 에디터 자동 실행·커밋·푸시 없음.

## 2026-10-02 피해 참고서 — 노드 용도·핀 중심 구성으로 개편

- 사용자가 현재 `Docs/Guides/common/DAMAGE_NODES_FORMULA_GUIDE_KO.html`을 지정했다. 이전 nsh 위치가 아니라 이 경로에만 수정했다. 직접 피해·DOT·경직 노드를 용도 설명 → 입력 핀 → 출력 핀 → 구조체 연결법 순으로 개편했다. Request/Spec/Return Value는 실제 이름·타입을 유지하고 펼침 제목에는 실제 포함 값도 표시한다.
- Request의 수치·대상/옵션, 피해 Result·Hit Context, DOT Spec·매 틱 Request·Stagger Template, 경직 Request/Result를 노드 안에 포함시켰다. 출처 Category/Source Id/Display Name와 중첩 타격 기록을 제자리에서 펼쳐 읽는다. 큰 필드 표를 핀별 카드로 바꿨고 구조체 통째 연결·Make/Break·핀 분할 안내는 별도 유지했다. 공식은 뒤쪽, 용어는 마지막 장으로 옮겼다. 기존 앵커 ID 보존 및 이동된 문서의 상대 링크 수정.
- 정적 검사 ID 중복0/상대 링크 및 앵커 오류0. 로컬 브라우저에서 내부값·출처 펼침, 검색의 중첩 필드 노출, 검색0결과/초기화, 61개 details 전체 펼침/접기, 가로 넘침 없음, JavaScript 오류0 확인. 게임 코드·BP·DA·피해 공식 자체는 변경하지 않았고 빌드는 필요하지 않아 실행하지 않았다. 테스트 탭/서버/일회성 외부 편집 스크립트는 정리한다. 커밋·푸시 없음.

## 2026-10-02 아이템 스킬 취소 직접 이벤트 — 빌드·검증 완료

- 사용자 승인으로 UARLoadoutItemInstance에 BlueprintImplementableEvent `ReceiveItemSkillCancelled`(표시 이름 `On Item Skill Cancelled`) 추가. 출력 SkillId / ActionHandle / EARActionCancelReason Reason. 런타임 Event Graph에서 직접 구현하며 별도 Bind가 필요 없다. 해당 인스턴스의 ExecuteItemSkill에 도달한 실행만 알린다. 정상 EndAction·사전 실패·실행 전 롤백·수동 TryStartAction은 제외한다.
- Loadout은 정확한 FARActionHandle에 약한 아이템 참조/SkillId를 연결한다. 액션 자동 정리 후 실행 기록을 먼저 제거하고 이벤트를 전달해 중복 알림을 방지한다. EndPlay에서도 소유 액션 취소까지 구독을 유지한다. 제거·실행 콜백 중 중첩 입력을 막고 등록 배열을 BP 콜백 전에 복사해 사용자 이벤트의 아이템 제거에 대응했다. 비용·쿨다운 환불은 변경하지 않았다.
- `Scripts/build_editor.ps1` 정식 빌드 성공. `AR.Foundation` 41/41 Success, 실패0/경고0/미실행0. 새 Items.SkillCancellationEvent 및 Items.SkillCancellationReentrancy는 인스턴스/핸들 분리, 자동 정리 순서, 자체 타이머 회수, 중복·정상 종료 제외, 실행 중 아이템 제거·배열 변경·EndPlay를 검사한다. 테스트용 반사 이벤트 기록기는 테스트 범위에서만 함수 플래그/네이티브 포인터를 임시 변경하고 복원한다. 게임 예제/저작용 C++ 도구는 추가하지 않았다.
- `Saved/Automation/ItemSkillCancellation/index.json`, `Saved/Logs/ItemSkillCancellation.log` 및 `ItemSkillCancellationSavedContent.log` 참조. 저장 콘텐츠 9BP/2DA 로드·컴파일 오류0. 기존 BP_test1_R_A의 미연결 GetInstanceId 실행 핀 경고가 2회 출력되어 유일 경고1종이다. 사용자 지시에 따라 GUN 그래프는 수정하지 않았다. GUN 폴더 4 uasset의 작업 전후 SHA256 동일, 어떤 에셋도 저장하지 않았다.
- 스킬/유물 런타임 HTML 참고서와 API 카탈로그/코드 구조에 직접 이벤트 계약 및 타이머 정리 사용법 추가. GUI 에디터 자동 실행·커밋·푸시 없음.

## 2026-10-02 데미지 노드·공식 HTML 참고서

- 사용자 요청으로 `Docs/Guides/nsh/DAMAGE_NODES_FORMULA_GUIDE_KO.html` 작성. 기존 아이템 스킬 참고서 스타일을 재사용한 10장/36개 접이식 묶음이며 검색·목차·전체 펼치기/접기·해시 세부항목 열기·인쇄 대응을 제공한다. 스킬 런타임 참고서 상단에 새 문서 링크 추가. 특정 GUN 예제 없이 개발용 공통 피해 API 참고서로 구성했다.
- 실제 Damage Resolver/Combat Subsystem/Health/Stats/Stagger/Hitbox와 공개 헤더 기준: 전달 방식·속성 용어, 전체/전달/속성 주는 피해 합연산, 전체/속성 받는 피해 합연산, 방어/관통, 피해 감소 잔여 곱, 치명타·공허 예외, 정수화와 실제 자원 차감 반환값, 실드·회피·무적·흡혈(/1000), Request 공개18필드/Result9필드 및 Hit Context, DOT Spec·등록·갱신·출처 제거·틱·공격자 소멸, 경직/그로기 공식·입출력, 이벤트 및 중복 충돌 경계를 설명했다. 일반 DOT는 액션·아이템 소유 자동 회수가 아님을 명시했다.
- 정적 검사: ID 중복0/상대 링크·앵커 오류0. 로컬 브라우저에서 검색/검색0결과/초기화/46개 details 펼치기·접기/목차 및 세부 Request 옵션 바로가기 확인. 가로 페이지 넘침 없음·JavaScript 오류0. 테스트 탭과 로컬 서버는 종료한다. 게임 코드·BP·DA는 수정하지 않았고 이번 문서 작업에서 빌드를 실행하지 않았다. 커밋·푸시 없음.

## 2026-10-02 에디터 모듈 로드 충돌 복구 — 정식 빌드·자동 검사 완료

- 사용자가 게임 모듈 로드 실패를 확인해 달라고 요청한 뒤 정식 빌드를 승인했다. 실행 로그에서 런타임 `UnrealEditor-Action_RogueLike-1304.dll`과 에디터 모듈이 의존하는 기존 `UnrealEditor-Action_RogueLike.dll`이 동시에 로드되어 `UARResourceHUDWidget` 등 클래스 중복 등록으로 실패하는 것을 확인했다. 원인은 핫 리로드 DLL과 기존 DLL의 혼용이다.
- 에디터 종료를 확인하고 `Scripts/build_editor.ps1`로 Editor Win64 Development 정식 빌드 성공. 빌드 도구가 생성된 핫 리로드 DLL/PDB/lib와 이전 메타데이터를 정리하고 정식 런타임 DLL을 링크했다. `Binaries/Win64/UnrealEditor.modules`는 두 모듈 모두 정식 이름을 사용한다. 소스·블루프린트·DA 변경이나 임의 폴더 삭제는 하지 않았다.
- 새 UnrealEditor-Cmd 프로세스에서 두 모듈 정상 로드 및 `AR.Foundation` 39/39 Success, 실패0/경고0/미실행0을 확인했다. `Saved/Automation/ModuleLoadRecovery/index.json`, `Saved/Logs/ModuleLoadRecovery.log`, `Saved/Logs/BuildEditor.log` 참조. 아래 실드 HUD 빌드 대기 기록은 이 완료 기록으로 대체된다. `UI.ResourceHUDLiveUpdates`의 실드 표시/수명 검사도 Success, 오류0/경고0. 화면을 띄운 수동 PIE 외형 검증은 별도이며 이번 실행은 NullRHI 자동 검사다.
- GUN 폴더의 4 uasset 작업 전후 SHA256 동일. `WBP_TestResourceHUD.uasset` SHA256도 기존 `7992CB40FB612E15B3D06986B48D0435ED28AAAF79386DB486A849C15F2D30B1` 유지. 에셋을 저장하지 않았으며 GUI 에디터를 자동 실행하지 않았다. 커밋·푸시 없음.

## 2026-10-02 체력 HUD 실드 표시 — 빌드 대기

- 사용자 요청: 같은 출처 피해의 최소 틱 간격 여부 확인 + 체력 HUD에 회색 실드 표시. 조사 결과 ProcessDamageRequest에는 출처별 공통 시간 제한이 없고, DOT는 효과별 TickInterval(기본 0.25초, 편집 ClampMin 0.001초, 런타임 유한 양수 검증)로 처리한다. 늦어진 DOT는 한 프레임에서 누락 틱을 따라잡는다. 히트박스 OncePerTarget과 플레이어 피격 후 무적(기본 0초)은 별도 제한이다. 피해 처리 로직은 수정하지 않았다.
- `ARResourceHUDWidget.h/.cpp`에 ShieldBar 추가. 기존 HealthBar 부모 Overlay에 런타임으로 회색 하단 레이어를 삽입하며 HealthBar 배경만 투명화해 빨간 HP 뒤 회색 실드를 노출한다. 분모 Max(MaxHP, HP+Shield), 빨간 HP/분모, 회색 합계/분모. 숫자는 실드>0일 때 HP/MaxHP + Shield, 제거/만료 시 일반 표기로 복귀한다. 자체 ShieldBar를 가진 Designer도 연결 가능. 기존 WBP_TestResourceHUD 에셋을 저장/재생성하지 않았다. 해당 에셋 SHA256 `7992CB40FB612E15B3D06986B48D0435ED28AAAF79386DB486A849C15F2D30B1`.
- 기존 `AR.Foundation.UI.ResourceHUDLiveUpdates` 테스트에 회색/레이어 순서, 실드 획득·실피해 흡수·제거·최대HP 변경·초과량·시간 만료·플레이어 교체·연결 해제 검사를 추가했다. 객체별 스탯 가이드에 실드 HUD 표시 규칙 추가. 기존 dirty 변경은 보존, GUN C++/DA/BP 및 피해 제한은 변경하지 않았다.
- **아직 빌드/실행 검증 전.** 에디터 PID 17500 열림을 확인하고 사용자에게 저장 후 종료 요청을 비동기 질문으로 보냈다. 종료 응답 후 Scripts/build_editor.ps1 정식 빌드(필요시 require_escalated), UI.ResourceHUDLiveUpdates 단독, AR.Foundation 전체 자동화 실행 및 레포트 failed/errors 확인. 테스트 에셋 SHA 유지 확인. 실제 화면/PIE 외형은 가능하면 별도 확인. git diff --check 및 설치 엔진 API(GetWidgetStyle/InsertChildAt/GetPadding) 정적 확인 성공.

## 2026-10-02 스킬 런타임 가이드 유틸리티 노드 보강

- 보강 검증: 상대 링크/목차 앵커 누락 0, ID 중복 0. 로컬 브라우저에서 08장 목차 자동 펼치기, 새 조준 설명 표시, 37개 전체 펼치기/접기, 연결 예제 표시 확인. 가로 넘침 없음, JavaScript 오류 0. 테스트 탭과 로컬 서버는 종료했다.
- 사용자 첨부 `6388a0a5-8c0f-4899-bcc9-cc99f014a87b`의 Blueprint 텍스트를 확인하고, 스킬 가이드 08장에 Get Item Owner / Get Aim Direction / Get Actor Location의 역할·Target·반환 타입·연결 예시를 추가했다. 실제 반환형은 ARPlayerCharacter, 조준은 현재 XY 정규화 방향, 위치는 Actor 루트 월드 좌표임을 구분했다.
- 연관 계산 Vector×Float / Vector+Vector, Make Transform / Spawn Actor from Class, Actor 대상 Get Final Stat(Range)도 추가했다. 11장/26개 노드 묶음으로 확장. 생성 거리와 사거리 분리, 생성 실패 정리, 독립 탄의 액션 수명 분리, 생성 시 노출 변수, GUN 임시 거리 환산을 설명했다. 게임 C++·DA·GUN 그래프는 변경하지 않았다.

## 2026-10-02 객체별 스탯 / 스킬 런타임 HTML 가이드

- 사용자 요청으로 `Docs/Guides/common/OBJECT_STAT_GUIDE_KO.html`과 `SKILL_RUNTIME_ACTION_GUIDE_KO.html`을 작성했다. 외부 라이브러리/서버 없이 로컬 HTML로 열 수 있다. 게임 C++·DA·GUN BP는 변경하지 않았다. 기존 유물 런타임 가이드 상단에 두 문서 링크를 추가했다.
- 객체별 가이드는 Player/Enemy/Environment 탭, 스탯 47개 설명 펼치기, 한국어/영문 검색, 분류/사용 상태 필터, C++ 초기값·안전 범위·사용 조건을 제공한다. 특정 저장 BP 값이나 실행 중 스냅샷이 아님을 명시했다. 적 기본 클래스의 자원/Loadout/구르기 부재, 환경 CombatSource만으로 Stats가 생기지 않음, 현재 Environment 피격 거절 규칙, 공격속도/Range/RecoveryPower/Luck의 콘텐츠 연결 필요를 구분한다.
- 스킬 가이드는 11장/20개 노드 묶음. Execute Item Skill 전에 처리되는 DA 비용/쿨다운/액션 시작, Switch 분기 후 스킬별 핸들 저장(값 핀 필수), 정상 End Action/취소 정리 구분, Action Delay 완료/취소, 액션/아이템/대상 효과 수명, CC 취소 규칙, 히트박스/이동, 등록/해제, 증상 점검을 포함한다. 같은 런타임의 다른 실행이 지연 핸들을 덮어쓰지 않도록 안내했다.
- 검증: EARStatType의 47개 키와 가이드 항목이 중복/누락 없이 일치. 파일 상대 링크·목차 앵커·ID 중복 검사 성공. 로컬 브라우저에서 3객체 모두 47개, 상태 배지, 검색/필터, 펼치기/접기, 키보드 탭 이동, 스킬 목차 자동 펼치기 확인. 두 HTML JavaScript 오류 0. 테스트용 브라우저 탭은 닫았고 에셋은 로드/수정/저장하지 않았다. 커밋/푸시 없음.

## 2026-10-02 GUN 저작 도구 프로젝트 밖 분리

- 사용자 요청: GitHub에 의도하지 않은 생성·검증 코드가 올라가지 않도록 프로젝트 밖으로 이동. `ARGunExampleCommandlet.cpp/.h`, `Scripts/open_gun_example.py`, `Scripts/inspect_gun_item.py`를 Git 저장소 밖 `C:/Users/ghksd/Desktop/game_dev/LocalTools/GunBlueprintAuthoring` 아래 원래 상대 경로로 이동하고 SHA256 일치를 확인했다. 삭제하지 않았으며 외부 보관본으로 복구 가능하다.
- GUN 폴더 내 5 uasset은 변경하지 않았다. 공통 런타임 액션/스탯 코드, DA 편집용 `ARItemDefinitionDetails` 및 그 에디터 모듈/테스트는 유지했다. 생성기 전용 BlueprintGraph/KismetCompiler/AssetRegistry 모듈 의존성만 제거했다.
- 아래 `-run=ARGunExample` 생성/검증/migration 기록은 과거 작업 기록이다. 현재 소스에는 해당 commandlet이 없으므로 실행하거나 자동 복원하지 않는다. 사용자 저장/종료 확인 후 정식 빌드 성공. 외부 `validate_detached_tools.py`로 새 모듈에서 commandlet 클래스 미등록 및 저장 콘텐츠 10BP/2DA 로드·컴파일 성공을 확인했다(`Saved/Logs/GunToolsDetachedVerified.log`, 오류0/기존 deprecated 경고2). 에셋은 저장하지 않았다. 첫 빌드는 에디터 DLL 잠금으로 실패했으나 종료 후 재빌드에 성공했다. 커밋/푸시하지 않았다.

## 2026-10-02 전체 피격 증가 합연산 + 개수 지정 스택 제거 — 빌드/검증 완료

- 사용자 확정: 전체 피격 증가와 해당 속성 피격 증가를 합연산. 기존 스탯 숫자는 고정하고 표시 순서만 관련 그룹에 배치. `OverallDamageTakenIncrease=46`을 선언 순서상 속성별 취약 앞에 추가하고 기존 `PhysicalDamageTakenIncrease=27`부터 `CooldownReduction=45`까지 유지, `Count=47`. `GetAllFinalStatViews`는 선언 순서로 표시하지만 저장 키는 숫자가 그대로다. HTML 생성기도 명시값을 이름으로 오인하지 않도록 보강했다.
- 기본 0, 최종 최소 0. Combat Subsystem이 대상의 전체 증가율을 읽고 Resolver는 방어력 적용 후 `1+(전체+해당속성)/100`을 적용한다. 직접/DOT 동일, 증감식 비활성화 시 둘 다 우회, 공허는 기존 취약 우회 규칙을 유지한다. 피해 감소와 공격자 증폭은 별도 기존 단계다. 예: 전체50+물리20=×1.7, ×1.8이 아님.
- 최신 사용자 지시 **“건 노드는 건들지마”**에 따라 GUN 폴더의 그래프·DA·픽업·투사체 에셋은 모두 수정/저장하지 않았다. 폴더 내 5 uasset의 작업 전후 SHA256이 모두 동일하다. GUN 장전의 물리/화염/마법 페널티 3노드와 이전 1스택 제거 호출은 그대로다. 1스택 함수는 검색에서 숨겨도 저장 콘텐츠 호환용으로 남겨야 한다. 기존 호출의 deprecated 컴파일 안내 2건은 예상된 호환 경고이며 오류가 아니다.
- 사용자가 저장 후 에디터 종료를 확인하여 `Scripts/build_editor.ps1` 정식 Editor Development 빌드 성공. `AR.Foundation` **39/39 Success** (`Saved/Logs/OverallTakenFoundation.log`). 새 `Combat.OverallDamageTakenIncrease`, `Stats.OverallTakenIntegration`, `Stats.StackBatchRemoval` 포함. 기존 GUN 예제 실행도 35검사 실패0 (`OverallTakenGunVerify.log`), 저장 콘텐츠 로드/컴파일은 10BP/2DA 성공/오류0 (`OverallTakenSavedContent.log`), 어떤 에셋도 저장하지 않았다.
- `-MigrateStacks`는 최신 지시에 따라 GUN 폴더를 명시적으로 제외한다. 실행 결과 해당 폴더 밖 구 노드 0개, 에셋 변경0 (`StackNodesMigration.log`). 새 `Remove Stat Modifier Stacks`는 정식 빌드에 반영되어 에디터 재개 시 사용 가능. Count 기본1/UHT메타데이터 확인, 부족 시 기본 무차감 실패, 부분 제거 옵션 및 실제 제거 그룹 수 제공. 기존 1개 노드의 호환용 함수는 BlueprintInternalUseOnly+DeprecatedFunction으로 검색에서 숨겼다.
- 계획서·카탈로그·코드 구조·HTML 레퍼런스·GUN 가이드를 갱신했다. 에디터는 아직 재실행하지 않았다. 커밋/푸시하지 않았다. 아래 적용 대기 기록은 이 완료 기록으로 대체된다.

## 2026-10-02 스택 개수 지정 제거 — 이전 적용 대기 기록(위 완료 기록 우선)

- 사용자가 `RemoveOneStatModifierStack`을 없애고 원하는 개수만큼 제거하는 노드로 변경하라고 명시 요청했다. `RemoveStatModifierStacks(Target, Category, SourceId, Count, RemovedCount, Policy=Oldest, bRequireFullCount=true)`와 컴포넌트 `RemoveModifierStacks`를 추가했다. Count 기본 1 메타데이터, 부족 시 기본 무차감 실패, 선택적 부분 제거, 실제 제거 그룹 수, 그룹 전체 제거, 만료/무효 요청 방어, 동일 적용시각의 삽입순서 선택을 구현했다. **아직 빌드/실행 검증 전이다.**
- 기존 1개 노드는 BlueprintInternalUseOnly+DeprecatedFunction으로 검색에서 숨기며 저장된 외부 그래프 호환용으로만 유지한다. `ARGunExampleCommandlet -MigrateStacks`는 모든 /Game BP 중 이 구 호출만 개수 1의 새 호출로 재구성한다. 사용자 그래프/배치를 재생성하지 않고, 원본 백업 후 모든 변경 BP 컴파일 성공 시에만 저장한다. 이전 RemovedHandle 출력에 연결이 있으면 자동 변환 대신 전체를 중단한다. **아직 migration을 실행하지 않았다.** GUN 생성기만 새 호출로 변경했다.
- `AR.Foundation.Stats.StackBatchRemoval` 테스트 추가(정확 개수, 부족/부분 모드, 그룹 효과, 순서, 같은 시각, category/id 분리, 무효/만료, null target). 기존 36개와 함께 총 37개 예상. 카탈로그/코드 구조/HTML 생성기/GUN 가이드에 새 노드 설명을 반영했고 HTML을 재생성했다. 코드 변경은 다른 dirty 작업을 보존해 적용했다.
- 에디터 프로세스 15124가 열려 있어 사용자에게 모든 작업 저장/에디터 종료를 비동기 질문으로 요청했다. 아직 답변 없음. 다음에는 종료 확인 후 Scripts/build_editor.ps1 정식 빌드(엔진 캐시 접근 때문에 require_escalated 필요), -run=ARGunExample -MigrateStacks, 새 저장 그래프 -Verify(사용자가 GUN BP 이름을 바꿨을 가능성 있음), AR.Foundation 전체 자동화, 저장 콘텐츠 컴파일 검사. 기존 BP 파일을 재생성하지 않는다.
- 읽기 전용 키 조회 `Scripts/inspect_player_key_routes.py`와 `Saved/Logs/PlayerKeyRoutesInspectionNames.log`로 IMC_Player의 E→IA_ActiveRelic1, R→IA_ActiveRelic2 및 BP_PlayerController 사용 컨텍스트 IMC_Player를 확인했다. BP_test_Player slot1/skill1, slot2/skill2는 기존 상태 그대로다. 앞서 장전 키가 미연결이라고 한 설명은 잘못됐고, 슬롯1 장전은 기존 E 매핑을 그대로 받는다. 에셋 입력 설정은 변경하지 않았다.

## 2026-10-02 GUN 액티브 유물 노드 예제

- 사용자 요청으로 `DA_GUN_R_A`와 `Runtime/BP_tes1_R_A`, `Runtime/BP_GUN_Projectile`에 실제 Blueprint 노드로 장전/발사/탄약/실드/보장 효과/투사체 이동을 구현했다. 게임플레이용 C++ GUN 클래스나 동작 함수는 추가하지 않았다. `Source/Action_RogueLikeEditor/Private/ARGunExampleCommandlet.*`는 에디터 전용 생성·검증 도구다.
- DA의 기존 Skill Id `Reload`, `Fire`, `DoneUseWeapon`과 장전 마나 비용 1을 보존했다. Fire: MouseLeft, 우선도 0, 기본 쿨타임 1초, 최소 0. 우선도 1의 기존 마나 1e9 게이트로 무기 기본공격 우선도 10을 차단한다.
- 장전 5초, CC 취소, 물리/화염/마법 받는 피해 증가 Flat 50(공허는 기존 규칙대로 제외), 탄약 최대 8발 충전. 플레이어 소유 `Relic/GUN.Ammo` 스택 사용. 첫 획득 마커 `Relic/GUN.FirstPickup`로 영구 지속시간 실드 10 중복 지급 방지. 발사 후 1초 아이템 소유 CC 면역+슈퍼아머. 유물 제거 시 액션/버프 회수, 탄약/실드 보존.
- 미정 피해량과 공격력/주문력 계수는 0. 임시 사거리 환산 `RangeCmPerStat=100`cm, 임시 탄속 `BulletSpeed=1500`cm/s. 첫 탄약은 예제 기본값 0. 사거리 종료/충돌 시 투사체 삭제. 플레이어 입력 바인딩/맵 배치는 변경하지 않았다.
- Editor Win64 Development 정식 빌드 성공. 저장된 두 BP 모두 컴파일 오류/경고 0. `-run=ARGunExample -Verify`의 실제 그래프 실행 검사 35개 통과, 실패 0 (`Saved/Logs/GunVerify.log`). `AR.Foundation` 36/36 통과 (`GunFoundationRegression.log`). 저장 콘텐츠 검사도 성공 (`GunSavedContentValidation.log`), 검사는 에셋을 저장하지 않았다. 수동 PIE 조작감 검증은 별도다.
- 생성 전 DA 백업 `Saved/GunExampleBackup/20261002_033528`. 해당 런타임 BP는 저장된 파일이 없어 정확히 요청한 경로에 생성했다. 생성기는 기존 노드가 있으면 거절하므로 사용자 수정 후 덮어쓰기하지 않는다. 자세한 흐름과 임시값은 `Docs/Guides/nsh/GUN_BLUEPRINT_EXAMPLE_KO.md` 참조. 커밋·푸시하지 않았다.
- 정식 빌드 후 Python으로 확인한 테스트 플레이어의 Skill Input Bindings 두 항목 모두 IA_Mouse_Left / InputTag=None. 슬롯 바인딩은 slot=1 / skill_index=1, slot=2 / skill_index=2. 이 작업은 플레이어를 수정하지 않았다. 실제 좌클릭 조작에는 Input.Skill.MouseLeft 연결이 필요하고, 2번 슬롯 GUN 장전에는 로컬 스킬 인덱스 1이 필요하다. 이전 Live Coding 세션의 Python 조회는 DA 필드값을 잘못 보고했으며, 새 프로세스에서 원래 Reload/Fire/DoneUseWeapon 설정이 정상 확인됐다.

## 2026-10-02 Action Tag 저작 항목 제거

- 사용자 요청으로 `FARActionRequest::ActionTag`와 행동 시작·스킬 정의의 태그 필수 검사를 제거했다. 행동은 고유 `FARActionHandle`로 관리하며, 스킬 선택은 기존 Input Tag/액티브 유물 슬롯과 Skill Id를 사용한다. 취소 규칙·이동 제한·아이템 인스턴스 소유권·쿨타임·코스트 규칙은 바꾸지 않았다.
- 저장된 DA의 이전 ActionTag 값은 삭제된 프로퍼티로 로드 시 무시된다. 에셋을 재생성하거나 일괄 저장하지 않는다. Blueprint에서 옛 ActionTag 핀을 직접 사용한 외부 콘텐츠는 별도 노드 정리가 필요하다.
- `Action.Roll` 네이티브 태그 등록 자체는 기존 저장 데이터 호환을 위해 유지했다. 더 이상 행동을 시작하거나 구르기를 구분하는 데 쓰지 않는다. 구르기는 기존 `bIsRollAction`을 사용한다.
- `AR.Foundation.Action.TagFreeLifecycle`, `AR.Foundation.Items.TagFreeSkillCooldown` 회귀 테스트와 `Scripts/validate_action_tag_removal.py`의 저장 콘텐츠 로드/Blueprint 컴파일 검사를 추가했다. 이후 GUN 작업 때 사용자 저장/종료 후 정식 빌드를 성공했고, 해당 테스트를 포함한 AR.Foundation 36/36과 저장 콘텐츠 검사(8 BP, 2 DA)를 통과했다. Live Coding으로 구조체 변경을 검증하지 않는다.

## 2026-09-30 보장 효과 확장 (아래 아이템 통합 기록보다 최신)

- 슈퍼아머는 기존 `UARStaggerComponent`의 핸들 기반 시스템을 유지한다. `UARLoadoutItemInstance::ApplyItemSuperArmor`로 부여하면 유물/무기 해제 시 해당 인스턴스가 준 핸들만 자동 회수된다.
- CC 면역은 강인함 스탯과 분리된 `UARStatusEffectComponent`의 중첩 가능한 핸들 기반 보장 효과다. `ApplyCCImmunity` / `RemoveCCImmunity`, 아이템 소유 `ApplyItemCCImmunity` / `RemoveOwnItemCCImmunity`, 행동 소유 `ApplyActionCCImmunity`가 있다. 지속시간 `-1`은 무기한, `0`은 무효다. 마지막 핸들이 끝나야 면역이 사라진다.
- 기절·속박 태그는 자동 CC다. 새 상태이상은 `UARStatusEffectDefinition::bIsCrowdControl`로 표시한다. 면역 중 새 CC는 적용 전에 `Blocked`로 거절되어 이동/행동 중단 이벤트가 발생하지 않는다. 이미 걸린 CC는 해제하지 않으며 비-CC는 정상 적용된다. 이 보장은 `bAffectedByTenacity` 및 일반 상태 면역 허용 플래그와 별개다.
- 최종 Editor Win64 Development 빌드 성공, `AR.Foundation` 자동화 테스트 **32/32 통과**. 로그: `Saved/Logs/FoundationGuaranteesFinal.log`. 새 유물 에셋은 생성하지 않았으며, 커밋·푸시도 하지 않았다.

## 2026-09-30 아이템 통합 변경 (이전 기록보다 우선)

- 저작 가능한 아이템 데이터 에셋은 `UARItemDefinition` 하나다. `ItemTypeTag`는 Weapon/ActiveRelic/PassiveRelic/Consumable 중 하나, `ItemId`는 유형 내 고유한 양의 정수, `AdditionalTags`는 중복 허용 문자열 배열이다. 아이템별 `DefinitionTag`와 옛 종류별 Definition 클래스는 제거했다. 전투·입력·상태이상의 Gameplay Tag는 그대로 유지한다.
- 무기·유물 런타임은 `UARLoadoutItemInstance`, 소모품 런타임은 `UARConsumableInstance` 자식 클래스로 분리된다. 둘 다 공통 Definition의 `RuntimeBehaviorClass`에 지정한다. 공통 `Try Acquire Item` 노드는 유형에 따라 기존 장비/소모품 컴포넌트로 보낸다.
- `Find Item By Key`, `Find Items By Additional Tag`, `Count Owned Items By Additional Tag`, `Validate Item Catalog`가 추가됐다. Asset Manager는 `ARItem` 단일 유형으로 아이템 경로를 스캔한다.
- 사용자가 삭제를 허용한 테스트 유물 에셋 5개(`R_P_Test_1` 포함)는 Windows 휴지통으로 이동했다. 테스트 전용 C++ 클래스/태그도 제거했다. 새 실제 콘텐츠 에셋은 아직 만들지 않았다.
- 상세 제작법은 [아이템 에셋 제작 가이드](Guides/ITEM_ASSET_CREATION_GUIDE_KO.html)를 우선한다. 이전 노드 카탈로그·한영 레퍼런스의 아이템 표는 아직 통합 전 스냅샷이다. 그 외 스탯·전투 표는 계속 참고 가능하다.
- 마지막 카탈로그 검사 노드까지 포함한 Editor Win64 Development 빌드가 성공했고 `AR.Foundation` 자동화 테스트 30/30이 통과했다. 로그: `Saved/Logs/FoundationFinalItemUnification.log`. 실제 제작 아이템 에셋은 아직 없으므로 카탈로그가 저장된 콘텐츠를 찾는 양성 경로와 에디터 내 픽업 조작은 콘텐츠 생성 후 수동 검증이 필요하다.
- 이 변경을 Git 커밋·푸시하지 않았다. 작업 시작 전부터 존재한 다른 사용자의 에셋·폴더 재정리 변경도 보존했다. 아래의 이전 업로드 지시는 과거 인계 기록으로만 취급하고 현재 요청의 승인으로 해석하지 않는다.

기준일: 2026-09-29

이 문서를 먼저 읽고 기존 문서와 실제 코드를 확인하면 현재 작업을 이어갈 수 있다. 이 문서는 프로젝트 상태 전달용이며, 특정 다음 기능의 구현을 자동으로 요청하는 문서는 아니다. 다음 작업 범위는 사용자의 새 요청에 따른다.

## 1. 프로젝트와 작업 원칙

- 현재 프로젝트 루트: `C:\Users\ghksd\Desktop\game_dev\Action_RogueLike` (다른 PC에서는 `.uproject` 위치를 기준으로 찾는다).
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
8. [임시 HUD 편집 안내](../Content/Game/Tests/UI/폴더%20설명!.md): 실제 Widget 외형 수정 방법.

과거 문서의 “Content에는 경로만 있다”, “UI는 아직 없다” 등의 기록은 현재 상태와 다를 수 있다. 최신 보완 기록과 실제 에셋·코드를 기준으로 판단한다. 기획 문서와 구현 상태도 구분한다.

## 3. Git 상태 — 다음 작업 전에 꼭 확인

- 브랜치: `main`.
- 이번 작업의 시작 기준 커밋: `4caba0a` — `feat: 적 AI 이동과 CC 연동 및 템플릿 콘텐츠 정리`.
- 최신 커밋은 `git log -1`로 확인한다. 아래 HUD·기획·문서와 이 인계 문서는 함께 커밋하는 인계 묶음이다.
- 원격: `https://github.com/yijunghwan/Prototype_Rog_Like_Game_Dev.git`.
- 사용자가 최종 검증 후 문서까지 포함한 GitHub 업로드를 요청했다. 검증 결과는 8절에 기록했다.
- 원격 체크아웃은 최신 `origin/main`을 사용한다. 작업 시작 시 로컬 상태와 원격 버전을 확인하고 추가 편집을 보존한다.

2026-09-28 인계 묶음의 주요 변경(아래 경로는 2026-09-29 정리 후의 위치):

- `Content/Game/Tests/UI/폴더 설명!.md`
- `Content/Game/Tests/UI/WBP_TestResourceHUD.uasset` — 실제 Widget 에셋.
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

`Content/Game/Tests/UI/WBP_TestResourceHUD.uasset`

언리얼 패키지:

`/Game/Game/Tests/UI/WBP_TestResourceHUD`

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
- 현재 기본 맵: `/Game/Game/Tests/Maps/Test_Level.Test_Level`.
- 기본 게임모드: `/Game/Game/Tests/Blueprints/BP_TestGameMode.BP_TestGameMode_C`.
- 콘텐츠 폴더는 `Foundation`(공용 기반), `Art`(임포트 그래픽), `Objects`(동작·정의), `Maps`(본편), `UI`(본편 화면), `Tests`(검증)로 구분한다. 편집 원본은 프로젝트 루트 `ArtSource`다. 각 폴더의 `폴더 설명!.md`에 용도를 기록한다.
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
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' Action_RogueLikeEditor Win64 Development '-Project=C:\Users\ghksd\Desktop\game_dev\Action_RogueLike\Action_RogueLike.uproject' -WaitMutex -NoHotReload
```

Foundation 테스트:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\ghksd\Desktop\game_dev\Action_RogueLike\Action_RogueLike.uproject' -unattended -nop4 -nosplash -NullRHI -nosound -DDC-ForceMemoryCache '-ExecCmds=Automation RunTests AR.Foundation' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=C:\Users\ghksd\Desktop\game_dev\Action_RogueLike\Saved\Automation\NextAgentFoundation' '-abslog=C:\Users\ghksd\Desktop\game_dev\Action_RogueLike\Saved\Logs\NextAgentFoundation.log'
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

## 11. 2026-09-29 콘텐츠 폴더 정리

- `ArtSource`는 편집 원본, `Content/Game/Art`는 언리얼 임포트 그래픽, `Objects`는 동작·정의, `Maps`는 본편 레벨, `Tests`는 테스트 전용으로 나눴다. `Foundation`과 `UI`는 공용 기반과 본편 UI를 각각 유지한다.
- 각 콘텐츠 폴더의 `폴더 설명!.md`에 용도를 기록했다. 기존 README도 같은 이름으로 옮겼으며, 그 내용은 새 경로에 맞게 갱신했다.
- 실제 테스트 플레이어·입력·HUD·게임모드·맵은 `Tests`로 Unreal AssetTools/맵 저장 기능을 사용하여 옮겼다. C++·Config의 참조도 함께 갱신했다. C++ 에디터 Development 빌드 성공, 새 맵 열기 성공, `AR.Foundation` 29/29 성공(0 실패·0 경고). 검증 결과는 `Saved/Automation/ContentReorgFinal/index.json`이다.
- 로컬에서 수정돼 있던 기존 `Test_Level.umap`은 `Saved/ContentReorgBackup/Test_Level.before_reorg.umap`에 작업 전 백업했다. 이 백업은 `Saved` 안에 있어 Git에 포함되지 않는다. 새 테스트 맵에 수정 내용이 보존된 것을 확인했고 구 경로 맵을 제거했다.
- 이 폴더 정리 작업은 아직 커밋·푸시하지 않았다.
