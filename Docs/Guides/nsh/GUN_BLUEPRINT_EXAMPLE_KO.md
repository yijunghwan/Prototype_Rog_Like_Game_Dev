# GUN 액티브 유물 블루프린트 예제

[가이드 메인 페이지](<../가이드 메인 페이지.html>)

게임플레이 동작은 DA와 블루프린트에 있다. 초기 제작에 사용한 `ARGunExampleCommandlet`은 사용자 요청으로 프로젝트와 Git 저장소 밖에 보관했다. 현재 프로젝트 소스/빌드에는 이 생성·검증 도구가 없으며, 저장된 GUN 블루프린트는 도구 없이 동작한다. 데이터 에셋 편집 화면을 정리하는 별도의 에디터 모듈은 유지한다.

## 에셋과 그래프

- DA: `/Game/Game/Objects/Items/Relics/Active/Test_1_R_A/Data/DA_GUN_R_A`
- 런타임: 같은 폴더의 `Runtime/BP_tes1_R_A`
  - `CanExecuteItemSkill`: 탄약/장전 상태 사전 검사. 실패한 입력은 비용과 쿨타임을 소비하지 않는다.
  - `GUN_FirstPickup`: 플레이어 스택 `Relic / GUN.FirstPickup`을 검사하여 최초 획득에만 실드 10을 부여한다.
  - `GUN_Skills`: `Execute Item Skill`에서 Skill Id로 발사/장전/차단을 나눈다.
  - `GUN_UnregisterCleanup`: 유물에 귀속된 액션을 취소한다. 플레이어 탄약/최초 획득 마커/실드는 지우지 않는다.
- 투사체: `Runtime/BP_GUN_Projectile`의 `GUN_ProjectileFlight`.

## 확정 동작

슬롯의 로컬 스킬 1번은 5초 장전이다. 기존 탄약이 8발보다 적을 때 8발까지 채우며, 장전 중에는 발사/중복 장전을 거절한다. 최초 탄약은 예제 기본값으로 0발이다.

탄약은 `Apply Stat Modifier`의 `Stack Only`를 사용한 플레이어 소유 스택이다. 소스는 `Category=Relic`, `Source Id=GUN.Ammo`다. 조회/소비/제거 노드는 두 값을 정확히 맞춘다. `Apply Item Stat Modifier`를 사용하지 않으므로 유물을 버려도 탄약이 남는다.

장전 페널티는 `Apply Action Stat Modifier`로 물리/화염/마법 받는 피해 증가에 각각 Flat 50을 더한다. 완료 시 `End Action`, 경직/기절/아이템 제거 시 액션 취소가 해당 핸들들을 회수한다. 기존 시스템에서 공허 피해는 이 스텟을 우회하므로 이 예제에서도 공허는 제외된다. 기존 증가 효과와는 시스템 규칙대로 합산된다.

장전 대기는 일반 Delay나 반복 타이머가 아니라 `Action Delay`다. `Completed`에서만 탄약을 채우고, `Cancelled`에서는 장전 상태만 해제한다. 액션 취소 시 대기 타이머도 제거된다.

발사는 탄약 1스택을 소비하고 투사체를 생성한다. 생성 실패 시 1스택을 환불한다. 성공하면 `Apply Item CC Immunity`와 `Apply Item Super Armor`로 각 1초 보장을 부여한다. 이는 강인함 스텟의 증가가 아닌 확정 CC 면역이며 이미 걸린 CC를 해제하는 효과는 아니다. 시간 만료 또는 유물 제거 시 회수된다. 발사 액션은 즉시 종료하므로 쿨타임 감소로 발사 간격이 짧아져도 이전 발사의 1초 버프가 조기에 끝나지 않는다.

최초 실드는 지속시간 -1의 실드 10이다. 피해로 소모될 수 있으며, 재획득으로 다시 지급되지 않는다. 최초 획득 마커와 탄약의 보존 범위는 현재 플레이어 객체의 생존 기간이다. 세이브 파일 보존은 별도 기능이다.

## 스택 개수 지정 제거

`Remove Stat Modifier Stacks`의 `Count`에 제거할 스택 수를 지정한다(기본 1). `Target`은 `Get Item Owner`로 받은 플레이어다. `Category=Relic`, `Source Id=GUN.Ammo`를 맞춘다. `Removed Count`는 실제 제거한 스택 그룹 수다. 한 그룹에 여러 스탯 변경이 있으면 함께 제거한다.

고급 입력 `Require Full Count`는 기본 체크다. 예를 들어 3개를 요구하는데 2개만 있으면 아무것도 소비하지 않고 false/0을 반환한다. 해제하면 남은 2개를 제거하고 true/2를 반환한다. 0/음수 요청, Source Id=None, 유효 대상/스택 없음은 false/0이다. `Policy` 기본 Oldest는 먼저 쌓인 그룹부터 제거하며, Newest는 나중에 쌓인 그룹부터 제거한다. 적용 시간이 같으면 적용 순서로 결정한다. 만료된 스택은 소비하지 않는다.

기존 1개 제거 노드는 새 노드 검색에서 숨기고 저장 콘텐츠 호환용으로만 유지한다. 사용자의 최신 요청에 따라 GUN 그래프의 기존 제거 노드는 교체하지 않았다. 신규 그래프에서는 개수 지정 노드를 사용하면 된다. 기존 노드를 컴파일하면 deprecated 안내가 발생할 수 있지만 기능은 호환된다.

공통 스탯 `Overall Damage Taken Increase`(전체 피격 피해 증가율)도 사용할 수 있다. 해당 속성의 피격 증가율과 합산하며, 전체 50 + 물리 20이면 물리 피해는 70% 증가다. 공허는 기존 규칙대로 제외한다. GUN 장전의 기존 물리/화염/마법 노드 3개는 사용자 요청대로 그대로 유지했다.

## DA 설정과 입력

- 기존 Skill Id `Reload`, `Fire`, `DoneUseWeapon`을 유지한다.
- `Fire`: Direct Tag `Input.Skill.MouseLeft`, 우선도 0, 기본 쿨타임 1초, 최소 쿨타임 0. 기존 시스템의 쿨타임 감소를 적용받는다.
- `DoneUseWeapon`: 같은 태그, 우선도 1, 기존 마나 비용 1,000,000,000. 해당 그룹 실패로 이후 우선도 10 무기 기본공격을 막는다. 플레이어가 이 비용을 실제로 감당하면 차단되지 않는 예제용 방식이다.
- `Reload`: Active Relic Slot 모드, 로컬 Skill Index 1. 실제 1/2번 장착 슬롯과 키는 플레이어가 결정한다. 기존 DA의 장전 마나 비용 1은 보존했다.
- 테스트 플레이어의 `Skill Input Bindings`에서 `IA_Mouse_Left → Input.Skill.MouseLeft`가 연결되어 있어야 한다.
- 두 유물 슬롯 모두 이 유물의 장전을 호출하려면 각각의 슬롯 바인딩 `Skill Index`가 1이어야 한다. 슬롯 번호와 로컬 스킬 번호를 혼동하지 않는다.

## 임시값과 수정 지점

- 런타임 변수 `ProjectileDamage=0`. 투사체 피해 요청의 공격력/주문력 계수도 0.
- 런타임 변수 `RangeCmPerStat=100`: 최종 플레이어 사거리 스텟 × 100cm를 발사 시 읽는다. 예: 사거리 10이면 1,000cm. 무기/유물의 사거리 보너스도 최종 스텟에 포함된다.
- 투사체 변수 `BulletSpeed=1500`: 임시 속도 1,500cm/초.
- 투사체는 이동 프레임마다 남은 거리만큼 이동량을 제한하여 사거리 끝에서 삭제된다.
- 충돌용 Sphere 루트는 물리 시뮬레이션 없이 Query Only + BlockAllDynamic이다. 시각용 Sphere 메시는 No Collision이다.
- 충돌 시 유효 적이면 피해 요청(현재 0)을 보내고 삭제한다. 벽/공격 불가 대상이면 피해 없이 삭제한다. 관통은 구현하지 않았다.

키보드/마우스 실제 플레이 감각과 맵 배치는 별도 PIE 확인이 필요하다. 자동 실행 검증은 저장된 그래프의 장전 취소, 8발 충전, 발사, 버프 만료, 사거리 이동/삭제, 우선도 차단, 유물 제거/재획득을 검사한다.

## 백업과 검증

생성 전 DA 백업: `Saved/GunExampleBackup/20261002_033528/DA_GUN_R_A.uasset`. 지정 런타임 BP는 생성 당시 저장된 파일이 없어 새로 만들었다.

정식 빌드 로그: `Saved/Logs/BuildEditor.log`. 블루프린트 생성 로그: `Saved/Logs/GunGenerate.log`. 저장된 블루프린트 실행 검증 로그: `Saved/Logs/GunVerify.log`.

위 로그는 도구를 분리하기 전의 검증 기록이다. 현재 프로젝트에서는 `-run=ARGunExample -Verify`나 `-MigrateStacks`를 실행할 수 없다. 외부 보관본은 기존 그래프에 노드가 있으면 생성을 중단하도록 되어 있지만, 자동으로 복원하거나 실행하지 않는다.
