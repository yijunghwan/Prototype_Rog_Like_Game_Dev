"""Inherited enemy Details reference. Documentation only; never loads/saves assets."""
from organize_development_guides import (
    GUIDES, block, chapter, node, note, page, pins, replace_block, save,
)

URL = 'hhc/ENEMY_DETAILS_GUIDE_KO.html'


def setting(key, title, location, meaning, rows):
    return node(key, title, meaning,
                '<p class="muted">선택 위치: ' + location + '</p>' + pins(rows), '설정')


def generate():
    quick = [
        ('BP 클래스 디폴트 → Character', 'Combat Team', 'Enemy', '#combat-team'),
        ('BP 클래스 디폴트 → Pawn', 'AI Controller Class / Auto Possess AI', 'ARAIController / Placed in World or Spawned', '#ai-settings'),
        ('Stats Component → Base Stats', 'Max Health / Move Speed / Attack Power', '적의 체력·이동 속도·공격 기본값', '#base-stats'),
        ('Stagger Component → Groggy', 'Use Groggy Gauge', '그로기를 사용할 적만 켜기', '#use-groggy-gauge'),
        ('Stats Component → Base Stats', 'Max Groggy', '그로기 사용 시 양수로 설정', '#groggy-stats'),
        ('Character Movement → Planar Movement', 'Constrain to Plane / Snap to Plane at Start', '바닥 보행인지 고정 평면 이동인지 확인', '#plane-settings'),
        ('Capsule Component → Collision', 'Collision Presets / Capsule 크기', '외형보다 실제 이동 충돌 크기 확인', '#capsule-settings'),
    ]
    table = '<div class="scroll"><table><thead><tr><th>선택할 곳</th><th>항목</th><th>적 제작 기준</th><th>설명</th></tr></thead><tbody>'
    table += ''.join(f'<tr><td>{location}</td><td>{name}</td><td>{value}</td><td><a href="{href}">펼쳐 보기</a></td></tr>' for location,name,value,href in quick)
    table += '</tbody></table></div>'
    sections = [
        ('where', '설정 찾기', chapter('where', '먼저 선택할 곳 · 클래스 디폴트와 컴포넌트',
            '<p>이 문서는 <code>ARBaseEnemy</code>를 상속한 적 BP의 <strong>디테일 패널</strong> 참고서입니다. 노드의 입력 핀 설명이 아니라, 어떤 객체를 선택해 어떤 기본 설정을 바꿀지 설명합니다. 언리얼의 모든 속성 사전은 아니며 적 제작에 중요한 항목을 다룹니다.</p>'
            + table + pins([
                ('BP 자체의 설정', '적 BP를 열고 <strong>클래스 디폴트</strong>를 선택합니다. Combat Team·AI Controller Class·Auto Possess AI·액터 Tick을 찾는 곳입니다.'),
                ('상속 컴포넌트의 설정', '왼쪽 컴포넌트 트리에서 Stats Component·Stagger Component·Health Component·Character Movement·Capsule Component 등을 선택합니다. 같은 이름을 검색해도 선택한 대상이 다르면 항목이 보이지 않을 수 있습니다.'),
                ('클래스 기본값 / 맵 인스턴스', '클래스 디폴트는 해당 BP의 기본값입니다. EditAnywhere 항목은 맵에 배치한 인스턴스에서 덮어쓸 수도 있습니다. 기존 인스턴스의 개별 값이 클래스 변경보다 우선할 수 있으므로 되돌리기 화살표와 실제 선택 대상을 확인합니다.'),
                ('Combat Team의 편집 범위', '프로젝트에서는 EditDefaultsOnly입니다. 맵에 놓인 적 한 개의 디테일보다는 해당 적 BP의 클래스 디폴트에서 설정합니다.'),
                ('검색·저장', '디테일 검색 필터를 지운 뒤 컴포넌트를 선택하고 영문 항목 이름으로 찾습니다. BP 컴파일·저장과 맵 인스턴스 변경 저장을 구분하세요.'),
            ]) + note('아래 기본값은 공통 C++ 원형 기준입니다. 자식 BP·맵 인스턴스·Construction Script·BeginPlay가 이미 덮어쓴 값은 다를 수 있습니다.'), True)),
        ('team-settings', '전투 팀', chapter('team-settings', 'Character · Combat Team',
            setting('combat-team', 'Combat Team · Player / Enemy / Environment',
                '적 BP 클래스 디폴트 → AR → Character 또는 Character → Combat Team',
                '공통 피해 시스템에서 아군/적군과 공격 가능한 대상을 구분하는 값입니다. 일반 적은 Enemy로 설정합니다.', [
                    ('Player', '플레이어 편. 현재 공통 피해 규칙에서는 Enemy를 공격합니다. 적을 Player로 바꾸면 플레이어와 같은 팀으로 판정되어 일반적인 플레이어 공격은 Same Team으로 거절될 수 있습니다.'),
                    ('Enemy', '적 편. ARBaseEnemy의 기본값입니다. Player를 공격하며 같은 Enemy끼리는 공통 피해가 거절됩니다.'),
                    ('Environment', '환경 공격 출처. 현재 공통 피해 규칙에서는 Player/Enemy에게 피해를 줄 수 있지만 Environment 자신은 피해 대상에서 제외됩니다. 적의 중립/무적 표현을 위해 임의로 선택하지 마세요.'),
                    ('팀과 컨트롤러의 차이', 'Combat Team을 Player로 바꿔도 사람이 조종하는 캐릭터가 되지는 않습니다. 조종은 Possess/Controller 설정, 전투 소속은 Combat Team입니다.'),
                    ('팀과 충돌의 차이', '팀을 바꾸어도 Capsule의 Block/Overlap 응답이나 NavMesh는 바뀌지 않습니다. AI가 누구를 추적할지도 별도 콘텐츠 판단입니다.'),
                ]) + note('사진처럼 Enemy BP에서 Player가 선택되어 있다면, 플레이어 편으로 만들 의도가 없는지 먼저 확인하세요.', 'bad'))),
        ('ai-settings', 'AI·조종 설정', chapter('ai-settings', 'Pawn · AI Controller와 자동 빙의',
            setting('ai-controller-class', 'AI Controller Class', 'BP 클래스 디폴트 → Pawn',
                '이 적을 AI가 조종할 때 생성할 컨트롤러 클래스입니다.', [
                    ('공통 기본', 'ARBaseEnemy는 ARAIController를 지정합니다. 이 컨트롤러가 공통 이동 잠금과 AR 경로 이동 요청을 처리합니다.'),
                    ('None / 다른 클래스', 'None이면 자동 AI 빙의에 사용할 클래스가 없습니다. 다른 클래스로 교체하면 ARAIController Cast와 전용 경로 이동·CC 정지 연동을 그대로 쓸 수 있다고 가정하지 마세요.'),
                ])
            + setting('auto-possess-ai', 'Auto Possess AI', 'BP 클래스 디폴트 → Pawn',
                '배치하거나 생성한 Pawn을 언제 AI Controller가 자동으로 조종할지 정합니다.', [
                    ('Disabled', '자동 AI 빙의 없음. 별도로 컨트롤러 생성·빙의를 관리할 때만 선택합니다.'),
                    ('Placed in World', '맵에 미리 배치한 적에 적용. 런타임 Spawn 적까지 자동 조종하는 설정은 아닙니다.'),
                    ('Spawned', '런타임 생성 적에 적용. 맵에 미리 놓은 적은 별도로 확인해야 합니다.'),
                    ('Placed in World or Spawned', '두 방식 모두 사용. ARBaseEnemy의 기본값이며 배치와 스폰을 함께 사용하는 적의 기본 기준입니다.'),
                ])
            + setting('auto-possess-player', 'Auto Possess Player', 'BP 클래스 디폴트 → Pawn',
                '어떤 사람의 Player Controller가 이 Pawn을 자동 조종할지 정합니다. 일반 적은 Disabled로 둡니다.', [
                    ('충돌 주의', '플레이어 자동 빙의를 켜면 Auto Possess AI가 무시될 수 있습니다. Combat Team만 Enemy여도 실제 조종자는 플레이어가 될 수 있습니다.'),
                    ('설정만으로 추적하지 않음', 'AI Controller가 생기는 것과 플레이어 추적 패턴이 실행되는 것은 다릅니다. 추적 판단 그래프와 맵의 NavMesh가 따로 필요합니다.'),
                ]) + '<p><a href="ENEMY_MOVEMENT_GUIDE_KO.html#navmesh">NavMesh 담당·문의</a> · <a href="ENEMY_MOVEMENT_GUIDE_KO.html#controller">컨트롤러·목표 연결</a></p>'
            + '<p>NavMesh 생성·관리는 맵 제작자가 담당합니다. NavMesh가 없으면 경로 이동을 할 수 없으므로 맵 제작자에게 문의하세요. AI 설정만 바꾸어 맵의 경로가 생기는 것은 아닙니다.</p>')),
        ('stat-settings', '기본 스탯', chapter('stat-settings', 'Stats Component · Base Stats와 상한',
            setting('base-stats', 'Base Stats', '왼쪽 Stats Component 선택 → AR → Stats → Base',
                '이 적의 기본 수치 목록입니다. 버프가 적용된 최종값이나 현재 체력/현재 그로기 게이지와는 다릅니다.', [
                    ('입력 방법', 'Map의 항목을 펼쳐 스탯 유형을 Key, 숫자를 Value로 입력합니다. 이미 같은 유형이 있으면 기존 값을 수정하세요. 필요한 항목이 없으면 추가합니다.'),
                    ('Max Health', '최대 체력. 공통 원형 기본 100. 현재 체력의 시작 방식은 Health Component의 Start At Full Health와 연결됩니다.'),
                    ('Move Speed', '기본 보행 속도. 공통 원형 기본 600. ARBaseCharacter가 최종 Move Speed를 Character Movement의 Max Walk Speed와 Max Walk Speed Crouched에 반영합니다.'),
                    ('Attack Power', '공격 콘텐츠가 조회해 쓰는 공격력. 공통 원형 Map에는 기본 항목이 없어 별도 설정 전 기본값은 0입니다. 수치를 넣는다고 공격 그래프나 피해가 자동 생성되지는 않습니다.'),
                    ('Stagger Resistance / Tenacity', '경직 저항 / 강인함. 경직 저항은 타격 경직 수치와 비교하고, 강인함은 강인함 적용을 선택한 CC 등의 기간에 영향을 줍니다. 서로 다른 값입니다.'),
                    ('그로기 항목', 'Max Groggy·Groggy Recovery Delay·Groggy Recovery Per Second는 아래 그로기 설정을 보세요. 공통 원형 Map에 없는 값은 별도로 지정하지 않으면 기본값 0입니다.'),
                    ('적에게 없는 플레이어 기능', 'Map에 Max Mana/Max Stamina 항목이 보이더라도 ARBaseEnemy에 플레이어의 Mana/Stamina 컴포넌트·입력이 자동 추가되지는 않습니다. Money는 플레이어 전용입니다.'),
                ])
            + setting('stat-caps', 'Max Evasion / Max Critical Chance / Max Tenacity / Max Cooldown Reduction',
                'Stats Component → AR → Stats → Caps', '최종 확률·비율 스탯의 허용 상한입니다. 그 스탯의 기본값을 지급하는 설정은 아닙니다.', [
                    ('공통 원형 기본', 'Max Evasion=50, Max Critical Chance=100, Max Tenacity=100, Max Cooldown Reduction=40.'),
                    ('기본값과 구분', 'Max Tenacity=100이어도 이 적의 Tenacity가 자동 100이 되는 것은 아닙니다. 실제 Tenacity는 Base Stats와 보정으로 결정됩니다.'),
                ]) + '<p><a href="../common/OBJECT_STAT_GUIDE_KO.html">전체 스탯 의미·공식</a> · <a href="../precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html">런타임 스탯 조작·수명</a></p>')),
        ('health-settings', '체력·피해 허용', chapter('health-settings', 'Health Component · 초기 체력과 피해 시스템',
            setting('start-full-health', 'Start At Full Health', 'Health Component → AR → Health',
                'BeginPlay에서 현재 체력을 최대 체력으로 채울지 정합니다. 공통 기본 true입니다.', [
                    ('켜기', 'Get Max Health 기준으로 현재 체력을 채워 시작합니다.'),
                    ('끄기', '원하는 초기 체력을 입력하는 칸이 따로 생기지는 않습니다. 현재 원형의 초기 Current Health는 0입니다. 별도 초기화가 없다면 0으로 시작하므로 의도한 것인지 확인하세요.'),
                    ('스탯과 구분', 'Max Health는 Stats Component 설정입니다. 실드는 별도의 Apply Shield 등으로 만들며 이 체크로 자동 지급되지 않습니다.'),
                ])
            + setting('damage-enabled', 'Damage System Enabled', 'Health Component → AR → Health',
                '공통 체력 피해 적용을 허용할지 정합니다. 공통 기본 true입니다.', [
                    ('켜기 / 끄기', 'true이면 살아 있는 유효 대상의 공통 피해를 허용합니다. false이면 공통 피해 요청이 거절됩니다.'),
                    ('전부 무효화하는 설정 아님', '팀·충돌·이동·추적 설정이 아니며 별도 CC/그로기 요청까지 모두 면역으로 바꾸는 기능도 아닙니다.'),
                ]) + '<p><a href="ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html#death">사망 처리·자동 삭제 여부</a> · <a href="../common/DAMAGE_NODES_FORMULA_GUIDE_KO.html#healing">회복</a></p>')),
        ('groggy-settings', '그로기 사용·회복', chapter('groggy-settings', '그로기 사용 · 체크와 수치를 함께 설정',
            setting('use-groggy-gauge', 'Use Groggy Gauge', 'Stagger Component → AR → Groggy',
                '이 적이 그로기 게이지를 사용할지 정합니다. 공통 기본 false입니다.', [
                    ('켜기', 'BeginPlay에서 Current Groggy를 최종 Max Groggy로 채웁니다. 유효 그로기 피해 요청을 받으면 게이지가 감소합니다.'),
                    ('끄기', '게이지는 사용하지 않습니다. 일반 경직 사용 여부인 Can Be Staggered와는 독립적인 설정입니다.'),
                    ('체크만으로 완성되지 않음', 'Max Groggy를 양수로 설정하고, 공격 측에서 그로기 피해를 전달하며, 소진 후 반응과 복구를 콘텐츠에서 구현해야 합니다.'),
                ])
            + setting('groggy-stats', 'Max Groggy / Groggy Recovery Delay / Groggy Recovery Per Second',
                'Stats Component → Base Stats', '그로기 최대값과 아직 소진되지 않은 게이지의 자연 회복 설정입니다.', [
                    ('Max Groggy', '최대 게이지. 사용하려면 양수여야 합니다. 예: 100은 설명용 값이며 밸런스 고정값이 아닙니다. 최대값 0으로 시작하면 양수→0의 소진 전이가 생기지 않습니다.'),
                    ('Groggy Recovery Delay', '마지막으로 그로기 게이지 피해를 받은 뒤 자연 회복을 시작하기까지 기다리는 초. 0이면 추가 대기 없음. 그로기 기절 시간 설정이 아닙니다.'),
                    ('Groggy Recovery Per Second', '초당 자연 회복할 게이지 양. 0이면 자연 회복 없음. 최대 게이지를 넘지 않습니다.'),
                    ('공통 원형 기본', '세 값은 공통 Base Stats Map에 기본 항목이 없어 별도 설정 전 기본값 0입니다. Use Groggy Gauge를 켰다면 적의 Base Stats에 필요한 값을 직접 넣으세요.'),
                    ('최대값을 나중에 바꿀 때', '현재 게이지가 항상 새 최대값까지 자동 충전되는 것은 아닙니다. 시작 시 채우는 동작과 런타임 Max Groggy 변경을 구분하세요.'),
                ])
            + note('<strong>게이지가 0이 된다고 자동으로 기절하지 않습니다.</strong> 실제 양수→0 전이에서 On Groggy Gauge Depleted를 한 번 알리고 소진 잠금이 걸립니다. 이 상태에서는 자연 회복과 추가 그로기 차감을 멈추며, 콘텐츠가 Reset Groggy Gauge로 복구해야 합니다.', 'bad')
            + '<ol><li>Stagger Component에서 Use Groggy Gauge를 켭니다.</li><li>Stats Component의 Base Stats에 양수 Max Groggy와 원하는 자연 회복 값을 넣습니다.</li><li>공격 측에서 피해 결과에 양수 Base Groggy Damage를 전달하는지 확인합니다. HP 피해만으로 게이지가 감소하지 않습니다.</li><li>On Groggy Gauge Depleted에서 자기 기절·연출 등 원하는 반응을 만듭니다.</li><li>복구 시점을 정하고 Reset Groggy Gauge를 호출합니다. New Current Value=-1은 최대치 복구입니다.</li></ol>'
            + '<p>즉시 복구할지, 해당 기절 종료 때 복구할지는 콘텐츠 정책입니다. 기절 적용이 CC 면역 등으로 실패했을 때의 복구도 처리하세요. 아래 링크에 실제 연결과 이벤트가 있습니다.</p>'
            + '<p><a href="ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html#groggy">소진 이벤트·자기 기절·게이지 복구</a> · <a href="ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html#stun-recovery">해당 기절 종료를 구분하는 방법</a> · <a href="../common/DAMAGE_NODES_FORMULA_GUIDE_KO.html#stagger-result">공격 측 그로기 피해 연결</a></p>', True)),
        ('stagger-settings', '경직·상태 면역', chapter('stagger-settings', 'Stagger / Status Effect · 경직과 CC를 구분',
            setting('can-be-staggered', 'Can Be Staggered', 'Stagger Component → AR → Stagger',
                '타격 경직에 반응할지 정합니다. 공통 기본 true입니다.', [
                    ('경직 판정', '공통 경직은 최종 경직 수치가 Stagger Resistance보다 클 때 시도합니다. 슈퍼아머·경직 후 면역도 확인합니다.'),
                    ('false의 범위', '타격 경직만 막습니다. Use Groggy Gauge가 켜져 있으면 그로기 피해는 별도로 처리되며, 기절/속박 CC까지 전부 면역이 되지는 않습니다.'),
                ])
            + setting('stagger-duration', 'Base Stagger Duration / Post Stagger Immunity Duration',
                'Stagger Component → AR → Stagger', '경직 자체의 유지 시간과 경직 종료 후 연속 경직을 막는 기간입니다.', [
                    ('Base Stagger Duration', '경직이 성립했을 때 유지할 초. 공통 기본 0.2초. 현재 이 값에는 CC의 강인함 기간 감소를 자동 적용하지 않습니다.'),
                    ('Post Stagger Immunity Duration', '경직 종료를 감지한 뒤 추가 경직을 막는 초. 공통 기본 0.1초. HP 피해 제한이나 동일 Damage Name 제한과는 다릅니다.'),
                ])
            + setting('immune-status-tags', 'Immune Status Tags', 'Status Effect Component → AR → Status',
                '면역 허용 상태가 새로 적용될 때 차단할 상태 태그 목록입니다.', [
                    ('예시', 'Status.Stun은 기절, Status.Root는 속박 상태를 구분하는 태그입니다. 상태 정의가 bCanBeImmune을 허용해야 목록 검사로 막습니다. 간편 Apply Crowd Control은 면역을 허용합니다.'),
                    ('경계', '이미 적용된 상태를 제거하는 설정이 아닙니다. 일반 경직과 그로기 게이지 피해를 이 목록으로 막는 것도 아닙니다.'),
                    ('그로기 자기 기절', 'Stun 면역을 설정하면 그로기 소진 뒤 자신에게 적용하려는 기절도 실패할 수 있습니다. 성공/실패 정책을 함께 확인하세요.'),
                ]) + '<p><a href="../common/CC_STAGGER_GUIDE_KO.html">CC·경직·강인함·슈퍼아머 차이</a></p>')),
        ('movement-settings', '이동·평면 제한', chapter('movement-settings', 'Character Movement · 보행 속도와 평면 제한',
            setting('walk-speed', 'Max Walk Speed / Default Land Movement Mode',
                'Character Movement → Walking / Movement Mode', '언리얼 보행 설정입니다. 공통 AR 스탯과 함께 동작합니다.', [
                    ('Max Walk Speed', '디테일에서 이 값만 바꾸어도 BeginPlay와 Move Speed 변경 때 ARBaseCharacter가 최종 스탯으로 다시 설정합니다. 적의 정상 보행 속도는 Stats Component의 Move Speed에서 지정하세요.'),
                    ('Default Land Movement Mode', '바닥 보행 적은 Walking을 기준으로 설정합니다. None·Flying 등 다른 모드는 의도한 이동 방식과 경로 설정이 있는지 확인하세요.'),
                    ('Nav Agent 크기', 'Nav Agent Radius/Height와 캡슐 크기에 따라 통과 가능한 경로가 달라집니다. 크기를 바꾼 뒤 맵의 경로와 맞지 않으면 맵 제작자에게 문의하세요. 외형만 작게 만들어도 좁은 경로를 통과할 수 있는 것은 아닙니다.'),
                ])
            + setting('plane-settings', 'Constrain to Plane / Snap to Plane at Start / Plane Constraint Normal·Origin',
                'Character Movement → Planar Movement', '이동을 특정 평면에 제한하고 시작 위치를 그 평면에 맞출지 정합니다.', [
                    ('공통 원형 기본', 'ARBaseCharacter 생성자는 Constrain to Plane=true, Plane Constraint Normal=(0,0,1), Snap to Plane at Start=true로 설정합니다. 자식 BP는 이미 다르게 지정했을 수 있습니다.'),
                    ('Constrain to Plane', '켜면 Plane Constraint Normal에 수직인 평면 안에서 이동합니다. Normal=(0,0,1)은 XY 평면입니다.'),
                    ('Plane Constraint Origin', '고정 평면이 지나는 위치. 평면의 Z가 바닥 높이와 같은 것인지, 캡슐 중심이 있어야 할 높이인지 구분하세요.'),
                    ('Snap to Plane at Start', '평면 제한이 켜진 경우 시작 위치를 해당 평면으로 맞춥니다. 캡슐 중심을 Z=0 같은 바닥면으로 이동시키면 적이 묻혀 보이거나 보행이 막힐 수 있습니다.'),
                    ('바닥 보행 적의 확인', '일반적인 바닥 보행에서는 불필요한 평면 고정·시작 스냅을 끄고 캡슐이 바닥 위에 놓이도록 배치하는 방식을 먼저 확인하세요. 고정 평면 게임이면 의도한 Origin을 별도로 맞춥니다.'),
                ]) + '<p><a href="ENEMY_MOVEMENT_GUIDE_KO.html">이동 방식·NavMesh 참고서</a> · <a href="../precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html">움직이지 않을 때 점검</a></p>')),
        ('collision-settings', '충돌·외형', chapter('collision-settings', 'Capsule / Sprite·Mesh · 외형과 이동 충돌',
            setting('capsule-settings', 'Capsule Half Height / Capsule Radius / Collision Presets',
                '상속 Capsule Component 선택 → Shape / Collision', '캐릭터 보행과 바닥·벽 충돌의 중심 설정입니다. 보이는 적 외형과 실제 충돌 크기를 구분합니다.', [
                    ('Capsule Half Height / Capsule Radius', '반높이 / 반지름. 단위는 cm입니다. Actor 위치는 캡슐 중심이므로 바닥 Z와 같은 중심 Z로 놓으면 캡슐 아래가 바닥에 들어갑니다.'),
                    ('Collision Presets', '일반 Character 캡슐은 Pawn 프리셋을 기준으로 필요한 응답을 확인합니다. 바닥·벽에 대한 Block이 없으면 보행/충돌이 예상과 달라집니다. 프로젝트 커스텀 채널이 있으면 해당 응답도 확인하세요.'),
                    ('Collision Enabled', 'No Collision이면 충돌 조회도 꺼집니다. 보행 캡슐의 충돌 조회를 유지하고, Query/Physics 구분은 실제 물리 사용 여부에 맞춥니다.'),
                    ('Generate Overlap Events', '겹침 이벤트를 쓸 때 필요한 설정입니다. 상대 컴포넌트의 겹침 생성과 Collision 응답도 맞아야 합니다. 이것만 켠다고 공격 피해나 적 탐지가 자동 구현되지는 않습니다.'),
                    ('Simulate Physics', '일반 AI Character 보행 캡슐에 임의로 켜지 마세요. Character Movement로 움직이는 것과 물리 시뮬레이션으로 움직이는 것은 별도 설계입니다.'),
                ])
            + setting('visual-settings', 'Sprite / 추가 Static Mesh · 위치·스케일·리소스',
                'Sprite 또는 추가한 외형 컴포넌트 선택 → Transform / Sprite·Mesh / Materials',
                'APaperCharacter 계열은 상속 Sprite를 가지며, 별도 Mesh 외형을 추가할 수도 있습니다.', [
                    ('리소스', 'Sprite는 Source Flipbook, Static Mesh는 Static Mesh와 Materials를 지정합니다. 리소스가 None이면 해당 컴포넌트 외형이 보이지 않을 수 있습니다.'),
                    ('위치·스케일', '외형 컴포넌트의 Relative Location/Scale은 캡슐 중심 기준입니다. 외형을 키운 것과 캡슐 충돌 크기를 키운 것은 다릅니다. Actor와 레벨 인스턴스 스케일도 함께 확인합니다.'),
                    ('순수 외형의 충돌', '시각용 Mesh라면 별도 충돌이 필요한지 검토합니다. 캡슐과 외형이 동시에 주변을 Block하면 이동이 막힐 수 있습니다. 순수 외형은 No Collision을 우선 검토하세요.'),
                ]) + '<p><a href="https://dev.epicgames.com/documentation/en-us/unreal-engine/collision-in-unreal-engine">언리얼 Collision 참고</a></p>')),
        ('tick-settings', 'Tick·상속 컴포넌트', chapter('tick-settings', 'Actor Tick과 컴포넌트 설정을 구분',
            setting('actor-tick', 'Start with Tick Enabled / Tick Interval (secs)',
                'BP 클래스 디폴트 → Actor Tick', '액터의 Event Tick 실행을 관리합니다. AI 행동을 자동으로 만드는 설정은 아닙니다.', [
                    ('Start with Tick Enabled', 'Event Tick으로 추적 판단을 만들었다면 액터 Tick이 실제 활성화되어야 합니다. 공통 C++ 원형은 Actor Tick을 기본적으로 사용하지 않으며 BP의 Tick 구성과 실제 실행 상태를 함께 확인합니다.'),
                    ('Tick Interval (secs)', '0 이하이면 활성 Tick은 매 프레임, 양수이면 해당 간격으로 실행됩니다. BP 판단의 응답 속도와 빈도를 정하는 값입니다. Tick 자체가 비활성화면 간격만 바꾸어도 실행되지 않습니다.'),
                    ('Actor / Component / Timer', 'Actor Tick과 Character Movement·Stagger 등의 컴포넌트 Tick, Timer Manager의 예약은 별개입니다. 액터 Tick을 껐다고 모든 타이머나 효과 만료가 자동 정리되지 않습니다.'),
                    ('컴포넌트 Tick 임의 변경 주의', 'Stats·Health·Status 등은 유한 효과/상태에 맞춰 자체 Tick을 조절합니다. Stagger Tick은 경직 종료·면역·그로기 자연 회복을 처리합니다. 최적화를 위해 상속 컴포넌트 Tick을 일괄 꺼 버리지 마세요.'),
                ])
            + pins([('Stats Component', 'Base Stats·상한·런타임 보정 관리.'),
                    ('Health Component', '현재 HP·실드·피해 적용·사망 알림.'),
                    ('Stagger Component', '경직·슈퍼아머·그로기 사용·소진/회복.'),
                    ('Status Effect Component', 'CC·상태 효과·Immune Status Tags.'),
                    ('Action Component', '활성 액션·취소·액션 귀속 자원 관리. 액션마다의 Cancel Rules/Time Group 등은 Action Request이지 적 클래스의 고정 디테일 설정이 아닙니다.'),
                    ('Movement Control Component', '공통 이동 요청·잠금. 속도 수치는 Stats/Character Movement와 구분합니다.'),
                    ('참조가 회색/읽기 전용인 경우', '상속 컴포넌트 참조 자체를 다른 객체로 바꾸는 칸과 컴포넌트 내부의 편집 가능한 설정을 구분하세요. 새 동일 역할 컴포넌트를 중복 추가하기보다 상속 컴포넌트를 선택합니다.')]))),
        ('check', '설정 점검표', chapter('check', '증상으로 찾기 · 배치 전 확인', pins([
            ('플레이어 공격이 적에게 적용되지 않음', '<a href="#combat-team">Combat Team=Enemy</a>, <a href="#damage-enabled">Damage System Enabled</a>, 실제 피해 대상/공격 출처·생존 상태·충돌 판정을 확인.'),
            ('적이 움직이지 않음', '<a href="#ai-settings">AI Controller Class·Auto Possess AI·Auto Possess Player</a>, <a href="#walk-speed">Move Speed·이동 모드</a>, NavMesh·목표·CC 잠금·추적 그래프 실행을 확인.'),
            ('바닥에 묻히거나 시작 위치가 바뀜', '<a href="#plane-settings">평면 고정·시작 스냅·Origin</a>, <a href="#capsule-settings">캡슐 반높이와 중심 Z</a>, 바닥/외형 충돌 응답을 확인.'),
            ('그로기가 줄지 않거나 이벤트가 오지 않음', '<a href="#use-groggy-gauge">Use Groggy Gauge</a>, <a href="#groggy-stats">Max Groggy&gt;0</a>, 공격 측 양수 그로기 피해·성공 결과 연결을 확인.'),
            ('그로기가 소진 후 계속 0임', '자연 회복은 소진 잠금 중 멈춥니다. <a href="#groggy-settings">Reset Groggy Gauge 복구 정책</a>과 기절 적용 실패 분기를 확인.'),
            ('경직을 껐는데 기절/그로기는 작동함', '<a href="#can-be-staggered">Can Be Staggered</a>는 일반 경직만 제어합니다. CC·그로기·슈퍼아머는 각각 범위가 다릅니다.'),
            ('클래스 디폴트를 바꿨는데 맵의 한 적만 그대로임', '<a href="#where">인스턴스 덮어쓰기</a>, 선택 컴포넌트, Construction Script·BeginPlay에서 다시 설정하는 경로를 확인.'),
        ]) + '<p>이 문서는 디테일 설정만 안내합니다. 공격·추적·기절·사망 그래프는 <a href="ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html">적 전체 가이드</a>에서 이어서 제작하세요.</p>')),
    ]
    sources = [
        ('Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h', 'Combat Team·상속 컴포넌트'),
        ('Source/Action_RogueLike/Private/Foundation/Characters/ARBaseEnemy.cpp', '적 AI 기본값'),
        ('Source/Action_RogueLike/Private/Foundation/Characters/ARBaseCharacter.cpp', '평면 제한·속도 반영'),
        ('Source/Action_RogueLike/Public/Foundation/Components/ARStaggerComponent.h', '경직·그로기 디테일'),
        ('Source/Action_RogueLike/Private/Foundation/Components/ARStaggerComponent.cpp', '소진 잠금·자연 회복·Reset'),
        ('Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h', 'Base Stats·Caps'),
        ('Source/Action_RogueLike/Private/Foundation/Components/ARStatsComponent.cpp', '스탯 원형 기본값'),
        ('Source/Action_RogueLike/Public/Foundation/Components/ARHealthComponent.h', '체력 디테일'),
        ('Source/Action_RogueLike/Private/Foundation/Combat/ARCombatSubsystem.cpp', '전투 팀 피해 규칙'),
        ('Source/Action_RogueLike/Public/Foundation/Components/ARStatusEffectComponent.h', '상태 면역 설정'),
    ]
    text = page(URL, 'enemy', '적 디테일 · 상속 설정 참고서',
                'BP의 클래스 디폴트와 상속 컴포넌트를 선택해 팀·AI·체력·경직·그로기·이동·충돌을 설정합니다.', sections, sources)
    text = text.replace('노드의 용도 → 연결 대상 → 입력·출력 → 정리 책임 순서로 확인합니다. 구조체는 설명 아래에서 펼쳐 보거나 Blueprint의 Split Struct Pin / Make 노드로 연결할 수 있습니다.',
                        '선택 위치 → 항목 이름 → 의미 → 적 제작 기준 → 다른 설정과의 관계 순서로 확인합니다. 그래프 제작은 관련 가이드로 이어집니다.')
    text = text.replace('노드·내용 찾기', '설정·내용 찾기').replace('노드 이름, 핀, 증상…', 'Combat Team, Use Groggy Gauge, 바닥…')
    save(URL, text)

    overview_url = 'hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html'
    overview = (GUIDES / overview_url).read_text(encoding='utf-8')
    setup = block(overview, 'setup')
    if 'ENEMY_DETAILS_GUIDE_KO.html#where' not in setup:
        setup = setup.replace('<div class="chapter-body">', '<div class="chapter-body">'
                              + note('<a href="ENEMY_DETAILS_GUIDE_KO.html#where">적 디테일·상속 설정 참고서</a>에서 클래스 디폴트와 컴포넌트 설정을 먼저 확인하세요. Combat Team·AI·스탯·체력·경직·그로기 사용·충돌을 항목별로 설명합니다.', 'good'), 1)
        overview = replace_block(overview, 'setup', setup)
    groggy = block(overview, 'groggy')
    if 'ENEMY_DETAILS_GUIDE_KO.html#groggy-settings' not in groggy:
        groggy = groggy.replace('<div class="chapter-body">', '<div class="chapter-body">'
                                + '<p><a href="ENEMY_DETAILS_GUIDE_KO.html#groggy-settings">그로기 사용 체크·최대 게이지·회복값 설정 먼저 보기</a></p>', 1)
        overview = replace_block(overview, 'groggy', groggy)
    save(overview_url, overview)


if __name__ == '__main__':
    generate()
