"""Time group reference and contextual notes, called by the portal generator."""
from organize_development_guides import (
    GUIDES, PAGES, page, chapter, node, pins, note, save, upsert_reference, rel,
)


def update():
    url = 'precautions/TIME_GROUP_GUIDE_KO.html'
    sections = [
        ('choice', '선택 기준', chapter('choice', 'World / Player · 선택과 권장 기준',
            '<p><strong>적의 이동·공격·선후딜·쿨타임·타이머는 모두 World로 맞추는 것을 권장합니다.</strong> 플레이어 측 무기·유물 제작 시에는 의도에 따라 World 또는 Player를 선택하세요. 모든 선택형 필드·새 타이머 노드의 기본값은 World입니다.</p>'
            + pins([('World', '언리얼 월드 시간. 적·일반 Actor·투사체·물리·일반 엔진 타이머·일반 Delay·DOT가 기본적으로 따릅니다. 플레이어가 발사한 총알도 자동으로 Player가 되지 않습니다.'),
                    ('Player', 'ARPlayerCharacter 이동과 자체 회복·구르기 쿨타임, 플레이어 대상 스탯 보정·실드·CC·슈퍼아머 기간의 기준입니다. 선택한 스킬 쿨타임·전용 타이머에도 사용할 수 있습니다.'),
                    ('게임 일시정지', 'World와 Player 모두 멈춥니다. Player는 게임 pause 중에도 흐르는 운영체제 실시간이 아닙니다.'),
                    ('수명과 시간', '아이템/액션 귀속은 언제 회수할지, Time Group은 어느 속도로 흐를지입니다. Player 예약도 아이템 해제·실제 액션 취소 시 즉시 제거됩니다.')]), True)),
        ('setup', '설정 위치', chapter('setup', 'DA · 액션 · 타이머 설정 위치', pins([
            ('아이템 DA → Skill Definitions → Cooldown Time Group', '스킬 쿨타임의 기준. 시작 뒤 그룹 속도가 바뀌면 남은 시간의 진행 속도도 바뀝니다. 비용·우선순위 규칙은 바뀌지 않습니다.'),
            ('아이템 DA → Skill Definitions → Action Request → Time Group', '그 스킬이 생성하는 Action Handle에 저장됩니다. 액션은 시간이 지났다고 자동 종료되지 않습니다.'),
            ('Try Start Action → Request → Time Group', '직접 시작하는 적/콘텐츠 액션의 기준. 기본 World. Request 구조체를 Split하거나 Make ARAction Request로 설정할 수 있습니다.'),
            ('Action Delay', '연결한 활성 Action Handle의 그룹을 자동 사용합니다. 별도 그룹 핀이 없습니다. 정상 End 또는 취소로 액션이 먼저 종료되면 대기를 정리하고 Cancelled로 나옵니다.'),
            ('Set Item Timer by Event / Set Action Timer by Event → Time Group', '각 예약의 그룹을 독립적으로 선택합니다. 기본 World이며 액션 타이머도 Action Request를 자동 상속하지 않습니다. 동일 기준이 필요하면 Get Action Time Group 출력을 연결합니다.'),
            ('대상 효과의 Duration', '대상이 ARPlayerCharacter면 Player, 적·환경은 World입니다. Apply Action/Item Stat Modifier의 귀속 수명은 이 기간보다 먼저 끝날 수 있습니다. DOT 기간·틱·동일 Damage Name 제한은 World입니다.'),
        ]))),
        ('rate', '속도 제어', chapter('rate', '슬로모션 제어 노드',
            node('set-rate', 'Set Time Group Rate', '한 월드의 그룹 속도를 바꾸는 전역 요청입니다. 핀의 World Context는 현재 Blueprint 문맥에서 자동 연결됩니다.', pins([
                ('Time Group', 'World 또는 Player.'), ('Rate', '1=정상, 0.2=5배 느림. 유한한 0.001~10만 허용하며 0은 거절합니다. 완전 정지는 게임 Pause를 사용하세요. World는 엔진 배율 제한을 받을 수 있습니다.'),
                ('Return Value', '요청 수락 여부. 실제 배율은 Get Time Group Rate로 확인합니다.'),
            ]) + '<div class="flow">시작: Set Time Group Rate(World, 0.2)<br>Player Rate=1이면 AR 플레이어는 정상 속도, 적·총알·월드는 느려짐<br>종료/취소: 이전 World 배율을 저장한 값으로 복구</div>'
            + note('그룹 배율 변경은 액션 귀속 자원이 아니므로 자동 복구되지 않습니다. 종료·취소·아이템 해제에서 복구 책임을 명시하세요. 여러 슬로모션이 겹치면 한 콘텐츠가 임의로 1로 덮어쓰지 않도록 상위 관리 정책이 필요합니다.'))
            + node('query-rate', 'Get Time Group Rate / Get Time Group Seconds', '현재 배율 또는 해당 그룹 누적 초를 조회합니다.', pins([
                ('Time Group', '조회할 기준.'), ('Rate 반환', 'Player 배율 또는 실제 유효 World 배율.'), ('Seconds 반환', '게임 Pause를 제외한 누적 그룹 초. 서로 다른 그룹의 절대 시각을 빼서 기간을 계산하지 마세요.'),
            ])))),
        ('handles', '타이머 핸들', chapter('handles', '그룹 타이머 제어 · 남은 시간',
            '<p>전용 타이머의 Return Value를 Timer Handle 변수로 저장합니다. 아래 노드는 Handle만 받으며 그룹 관리자를 자동 찾습니다. 별도 그룹을 다시 지정할 필요가 없습니다.</p>'
            + pins([('Pause Time Group Timer', '남은 시간을 보존해 일시정지합니다.'), ('Unpause Time Group Timer', '일시정지된 예약을 재개합니다. 제거된 예약을 되살리지는 않습니다.'),
                    ('Clear Time Group Timer', '예약을 제거합니다. 저장 변수 자체는 무효화하지 않으므로 Does Time Group Timer Exist로 확인합니다.'),
                    ('Get Time Group Timer Remaining', '그룹 초 기준 남은 시간. 존재하지 않으면 -1.'), ('Does Time Group Timer Exist / Is Time Group Timer Paused', '예약 존재 / 일시정지 상태를 확인합니다.')])
            + note('언리얼 기본 Pause Timer by Handle / Clear and Invalidate Timer by Handle 등은 World Timer Manager만 조회하므로 Player 예약을 제어하지 못합니다. 그룹용 노드는 일반 World 엔진 예약에도 사용할 수 있습니다.')
            + '<p><a href="ACTION_LIFECYCLE_GUIDE_KO.html#managed-timers">전용 타이머의 모든 입력·콜백·정리 설명</a></p>')),
        ('compatibility', '적용 범위·한계', chapter('compatibility', '제공 노드와 적용 범위', pins([
            ('아이템·액션 전용 타이머', '시간 그룹을 지원하는 Set Item Timer by Event / Set Action Timer by Event만 제공합니다. 첫 지연은 Time + Initial Start Delay ± Variance입니다. Time=1, Initial Start Delay=0, Variance=0이면 첫 호출도 1초 뒤입니다.'),
            ('기존 일반 엔진 타이머·Delay', '자동 그룹 전환·수명 귀속이 없습니다. 계속 World 기준입니다.'),
            ('Custom Time Dilation', '등록된 ARPlayerCharacter는 시스템이 World 보상을 관리합니다. 별도 콘텐츠가 이를 매 프레임 덮어쓰지 마세요. 플레이어 그룹 속도는 Set Time Group Rate로 변경합니다.'),
            ('물리·소리·연출', 'Player 분리는 AR 캐릭터 Tick/Character Movement와 프로젝트 시간 로직 대상입니다. Chaos 물리 시뮬레이션·오디오·모든 애니메이션을 자동으로 독립 시뮬레이션하는 기능은 아닙니다.'),
            ('멀티플레이', '현재 월드 단위 기능이며 배율 변경의 네트워크 복제·예측을 추가 구현한 것은 아닙니다.'),
            ('에셋 보존', '기존 GUN·적 Blueprint 그래프를 자동 교체하거나 DA의 기존 수치를 바꾸지 않습니다. 기존 DA의 새 선택 항목은 World로 시작합니다.'),
        ]))),
    ]
    save(url, page(url, 'precautions', '시간 그룹 · 선택과 주의점',
                   '흐르는 속도와 정리 수명을 구분하고, 슬로모션 중 무엇을 느리게 할지 결정합니다.', sections))

    for category, target, _, _ in PAGES:
        if target in (url, 'common/BLUEPRINT_NODE_SEARCH_KO.html', 'common/BASIC_CONTROLS_GUIDE_KO.html', 'hhc/ENEMY_DETAILS_GUIDE_KO.html'):
            continue
        path = GUIDES / target
        details_link = rel(path, GUIDES / url)
        body = '<p><code>World</code>와 <code>Player</code>라는 시간 그룹이 있습니다. 선택형 기본값은 World이며 게임 Pause 시 두 그룹 모두 멈춥니다. 시간 기준은 아이템/액션의 자동 정리 수명과 별개입니다.</p>'
        if category == 'item':
            body += '<p>DA의 Skill Definitions에서 Cooldown Time Group과 Action Request → Time Group을 별도로 선택합니다. Action Delay는 액션의 그룹을 따르며, 아이템·액션 타이머의 Time Group은 예약마다 따로 선택합니다.</p>'
        elif category == 'enemy':
            body += '<p>Try Start Action의 Request → Time Group이 Action Delay의 기준이며, Set Action Timer by Event에는 별도 Time Group 핀이 있습니다. 일반 엔진 타이머·AI 이동은 World 시간입니다. 그룹 선택은 CC·이동 가능 검사나 취소 규칙을 바꾸지 않습니다.</p>'
        elif 'STAT' in target or 'CC_' in target:
            body += '<p>대상 ARPlayerCharacter의 보정·실드·CC·슈퍼아머 기간은 Player, 적·환경 대상은 World 기준입니다. 자동 만료와 아이템/액션 해제에 따른 회수는 별개입니다.</p>'
        elif 'DAMAGE' in target:
            body += '<p>직접 피해는 즉시 처리하며 DOT의 기간·틱과 동일 Damage Name 제한은 World 기준입니다. 선딜 타이머는 그룹 선택이 가능하고, 대상 실드의 기간은 플레이어/그 외 대상의 자체 시간 기준을 따릅니다.</p>'
        elif category == 'environment':
            body += '<p>일반 Actor·투사체·스포너의 엔진 Timer와 Delay는 World 기준입니다. 플레이어가 생성했다고 Player로 바뀌지 않습니다. 전용 액션/아이템 타이머는 그룹 선택이 가능합니다.</p>'
        body += f'<p><a href="{details_link}">그룹 설정 위치·제어 노드·호환과 주의점</a></p>'
        save(target, upsert_reference(path.read_text(encoding='utf-8'), 'time-groups', '시간 그룹 · 기능과 설정', body))


if __name__ == '__main__':
    update()
