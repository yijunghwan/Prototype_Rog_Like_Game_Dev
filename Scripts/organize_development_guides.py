"""Generate the offline guide portal and mechanically organize existing references.

Content is authored here; existing detailed HTML is retained rather than retyped.
Re-running is supported. Game code/assets are never touched.
"""
from pathlib import Path
from html import escape
from html.parser import HTMLParser
from urllib.parse import urlsplit, unquote
import os
import re

ROOT = Path(__file__).resolve().parents[1]
GUIDES = ROOT / 'Docs/Guides'
HOME_NAME = '가이드 메인 페이지.html'
GUIDE_HOME = GUIDES / HOME_NAME
PAGES = [
    ('common', 'common/BASIC_CONTROLS_GUIDE_KO.html', '기본 조작', '기본 키 배치·중복 지정과 충돌 주의·키 추가/변경 문의'),
    ('common', 'common/BLUEPRINT_NODE_SEARCH_KO.html', '전체 노드 검색', '프로젝트 노드·이벤트 이름 검색과 입력·출력·구조체 설명'),
    ('common', 'common/OBJECT_STAT_GUIDE_KO.html', '스탯', '객체별 스탯·공식·기본값과 현재 자원'),
    ('common', 'common/DAMAGE_NODES_FORMULA_GUIDE_KO.html', '피해·회복', '직접 피해·DOT·실드·회복·계산 공식'),
    ('common', 'common/CC_STAGGER_GUIDE_KO.html', 'CC·경직', '기절·속박·경직·강인함·면역'),
    ('item', 'nsh/ITEM_ASSET_CREATION_GUIDE_KO.html', '데이터 에셋', '아이템 DA 항목·타입·스킬 정의'),
    ('item', 'nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html', '런타임 기본', '등록·해제·아이템 타이머·효과·픽업 연결'),
    ('item', 'nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html', '아이템 스킬', '사전 조건·실행·액션 타이머·취소·종료'),
    ('enemy', 'hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html', '적 전체 가이드', '기본 설정·대상·그로기·사망·실드 이벤트'),
    ('enemy', 'hhc/ENEMY_DETAILS_GUIDE_KO.html', '적 디테일 설정', '상속 설정·Combat Team·AI·충돌·스탯·그로기 사용'),
    ('enemy', 'hhc/ENEMY_MOVEMENT_GUIDE_KO.html', '적 이동', 'NavMesh 담당·문의·컨트롤러·추적·정지'),
    ('enemy', 'hhc/ENEMY_ACTION_GUIDE_KO.html', '적 액션', '직접 액션 시작·공격 타이머·취소·종료'),
    ('environment', 'environment/ENEMY_SPAWN_GUIDE_KO.html', '적 스폰', '적 클래스 생성·확률·가중치·실패 처리'),
    ('environment', 'environment/ITEM_SEARCH_GUIDE_KO.html', '아이템 검색', 'DA 검색·소유 제외·중복 없이 N개 추첨'),
    ('environment', 'environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html', '픽업 소환', 'DA 결과→픽업 생성·여러 외형 선택'),
    ('precautions', 'precautions/ACTION_LIFECYCLE_GUIDE_KO.html', '액션 생명주기', '핸들·자동 정리·타이머·취소 이벤트 책임'),
    ('precautions', 'precautions/SKILL_PRIORITY_GUIDE_KO.html', '스킬 우선순위', '같은 우선순위 묶음·누적 비용·중단 규칙'),
    ('precautions', 'precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html', '이동 주의점', '경로/직선 혼동·CC 중단·반복 요청·실패 진단'),
    ('precautions', 'precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html', '스탯 조작·수명', '일반/아이템/액션 귀속·Permanent Flat·제거·스택'),
    ('precautions', 'precautions/TIME_GROUP_GUIDE_KO.html', '시간 그룹', 'World·Player 선택·슬로모션·타이머 제어·기존 그래프 호환'),
]
CATEGORIES = [('common','공통'),('item','아이템'),('enemy','적'),('environment','환경'),('precautions','주의점')]
MOVE_MAP = {GUIDES/'OBJECT_STAT_GUIDE_KO.html': GUIDES/'common/OBJECT_STAT_GUIDE_KO.html',
            GUIDES/'DAMAGE_NODES_FORMULA_GUIDE_KO.html': GUIDES/'common/DAMAGE_NODES_FORMULA_GUIDE_KO.html',
            GUIDES/'index.html': GUIDE_HOME,
            GUIDES/'GUN_BLUEPRINT_EXAMPLE_KO.md': GUIDES/'nsh/GUN_BLUEPRINT_EXAMPLE_KO.md',
            GUIDES/'TEST_ENEMY_BLUEPRINT_KO.md': GUIDES/'hhc/TEST_ENEMY_BLUEPRINT_KO.md'}
ANCHOR_MAP = {}

def rel(source, target):
    return os.path.relpath(target, source.parent).replace('\\','/')

def link(href, label):
    return f'<a href="{escape(href, quote=True)}">{escape(label)}</a>'

def pins(rows):
    return '<dl class="pins">'+''.join(f'<dt>{escape(k)}</dt><dd>{v}</dd>' for k,v in rows)+'</dl>'

def note(text, kind=''):
    return f'<p class="note {kind}">{text}</p>'

def node(key, name, purpose, body='', tag='노드'):
    return f'<details class="node" id="{key}"><summary><span class="tag">{tag}</span>{name}</summary><div class="node-body"><p>{purpose}</p>{body}</div></details>'

def chapter(key, title, body, opened=False):
    return f'<details class="chapter" id="{key}"'+(' open' if opened else '')+f'><summary>{title}</summary><div class="chapter-body">{body}</div></details>'

def cleanup(kind):
    event = 'On Item Skill Cancelled' if kind == 'item' else 'On Action Cancelled'
    item_note=' 아이템 타이머는 스킬 취소만으로 멈추지 않고 아이템 해제 때 정리됩니다.' if kind=='item' else ''
    return note(f'<strong>취소 후 직접 정리할 것:</strong> 경직·기절 등으로 실제 액션이 취소되면 시스템이 액션 생명주기와 액션 귀속 자원을 처리합니다. <code>Set Action Timer by Event</code>와 <code>Action Delay</code>는 자동 정리됩니다. 일반 <code>Set Timer by Event</code>·독립 Actor·공격 Bool·연출은 별도 정리가 필요합니다. <code>{event}</code>에서 해당 실행 핸들을 확인한 뒤 일반 Timer Handle을 <code>Clear and Invalidate Timer by Handle</code>에 연결하고 자체 상태를 정리하세요.{item_note} 모든 경직이 무조건 취소를 일으키지는 않습니다.')

def managed_timers(context='all'):
    shared=pins([('Event','매개변수 없는 Custom Event 또는 Create Event. 같은 수명 안에서 같은 Event를 재등록하면 기존 예약을 교체합니다.'),('Time','실행 간격(초). 양수로 예약하며 0·음수는 같은 수명·Event의 기존 예약을 제거하고 Success=false. 비유한 값은 거절합니다.'),('Looping','false는 1회 예약, true는 정리될 때까지 반복합니다.'),('Time Group','World / Player. 기본 World. 흐르는 시간 기준이며 자동 정리 수명과 별개입니다.'),('Success / Return Value','예약 성공 Bool / Timer Handle. Pause Time Group Timer·Unpause Time Group Timer·Clear Time Group Timer 및 그룹 조회 노드로 제어합니다.'),('Initial Start Delay · 고급','기본 0. 첫 실행은 Time + Initial Start Delay 뒤입니다. Time=1, Delay=0이면 1초 뒤, Delay=2이면 3초 뒤입니다.'),('Initial Start Delay Variance · 고급','기본 0. 첫 대기에 ±Variance 무작위 편차를 더합니다. 합계가 음수면 언리얼처럼 Time으로 대체됩니다. 이후 반복은 Time 간격입니다.'),('Max Once Per Frame · 고급','기본 false로 언리얼 Blueprint 타이머와 같습니다. true면 밀린 반복을 한 프레임에 최대 한 번 호출합니다.')])
    action=node('set-action-timer','Set Action Timer by Event','스킬·적 공격의 선딜, 후딜, 반복 실행은 이 노드를 우선 사용하세요. 한 액션 실행에 예약을 귀속하여 정상 End Action·실제 Cancel·소유 Actor EndPlay 때 자동 Clear합니다.',pins([('Target','Get Action Component에서 받은 AR Action Component. 아이템에서는 Get Item Owner → Get Action Component.'),('Action Handle','Execute Item Skill 또는 Try Start Action에서 받은 이번 실행의 핸들. 무효/다른 관리자 핸들은 실패.')])+shared)
    item=node('set-item-timer','Set Item Timer by Event','무기·유물 보유 중 주기 효과 또는 1회 지연 호출은 이 노드를 우선 사용하세요. 버림·교체·소유자 EndPlay로 아이템이 해제되면 자동 Clear합니다.',pins([('Target','등록된 ARLoadoutItemInstance 런타임 Self. 적 Actor·픽업 Actor·소모품 런타임에 놓는 노드가 아닙니다.'),('액션과 차이','스킬 취소만으로는 멈추지 않습니다. 스킬 선딜/반복 공격에는 액션 타이머를 사용하세요.'),('해제 이벤트 순서','아이템 타이머는 On Item Unregistered 호출 전에 제거됩니다. 해제 이벤트에서 새 아이템 타이머를 시작할 수 없습니다.')])+shared)
    selection='<p>먼저 <strong>어느 수명이 끝날 때 예약도 없어져야 하는지</strong> 선택합니다. 새 아이템·공격 그래프에서 일반 Set Timer by Event를 기본으로 쓰지 말고, 아래 귀속 노드를 사용하세요.</p>'
    if context=='enemy':
        selection+='<p>적 공격에는 액션 타이머를 사용합니다. Set Item Timer by Event는 아이템 런타임 전용이라 적 Self에는 사용할 수 없습니다. 적의 상시 AI 판단 루프는 공격 액션과 분리합니다.</p>'
    else:
        selection+=pins([('스킬/적 공격이 끝나면 중단','Set Action Timer by Event. 보유 중 효과와 구분합니다.'),('무기/유물을 버리면 중단','Set Item Timer by Event. 스킬 취소 중에도 보유 효과를 계속할 때 사용합니다.'),('스포너·상시 AI·독립 Actor','일반 엔진 Timer를 사용하고 Actor EndPlay 등에서 직접 Clear합니다. 수명 관리를 위해 가짜 공격 액션을 만들지 않습니다.')])
    flow=('<div class="flow">적: Try Start Action → Status 성공 → 이번 Action Handle 저장<br>Get Action Component → Set Action Timer by Event(Action Handle, Event, Time, Looping=false)<br>예약 Success → 대기 / 예약 실패 → End Action과 자체 상태 정리<br>Event 콜백 → 대상 유효성·공격 조건 확인 → 판정 → End Action</div>' if context=='enemy' else
          '<div class="flow">보유 효과: On Item Registered → Set Item Timer by Event(Target=Self) → Success 확인<br>스킬: Execute Item Skill → 받은 Action Handle 저장 → Set Action Timer by Event → Success 확인<br>Event 콜백 → 대상/콘텐츠 조건 확인 → 효과 처리 → 스킬 완료라면 End Action</div>')
    return chapter('managed-timers','권장 타이머 · 수명 선택과 연결',selection+action+(item if context!='enemy' else '')+flow+
        node('timer-callback','Event 콜백 · 완료와 취소의 차이','흰 실행 출력은 예약 직후이지 시간이 지난 출력이 아닙니다. 실제 효과는 Event에 연결한 Custom Event 안에서 실행합니다.',pins([('매개변수','Event는 입력 핀이 없는 Custom Event / Create Event입니다. 액션 핸들·대상은 실행별 상태에 저장해 콜백에서 읽습니다.'),('정상 완료','1회 호출이 끝나도 액션이 자동 종료되지는 않습니다. 마지막 효과 뒤 같은 관리자·핸들로 End Action. 실패 출구도 종료합니다.'),('자동 취소/해제','타이머에는 Cancelled 출력이 없습니다. On Item Skill Cancelled / On Action Cancelled / On Item Unregistered에서 자체 변수·연출·구독을 정리합니다.'),('콜백의 대상','귀속 수명과 별개로 대상 Actor가 삭제되거나 사망할 수 있습니다. Is Valid·사망·거리 등 콘텐츠 조건을 다시 확인합니다.'),('동시 실행','하나의 저장 변수로 이전 실행 핸들을 덮어쓰지 마세요. 중복 실행을 막거나 실행별 상태를 구분해야 합니다.')]),'연결 규칙')+
        node('timer-pitfalls','중복 예약·Pause·이미 적용한 효과','전용 타이머는 예약의 수명만 관리합니다. 콘텐츠의 모든 상태를 자동 되돌리는 기능은 아닙니다.',pins([('중복 예약','같은 아이템 또는 같은 Action Handle 안의 같은 Event는 교체합니다. 다른 아이템·액션은 독립적입니다. Tick마다 재등록하면 예약 시간이 계속 초기화될 수 있습니다.'),('Pause / Unpause / Clear','Return Value를 저장해 그룹용 제어 노드에 연결합니다. Player 예약은 엔진 기본 월드 Timer Handle 노드로 제어하지 마세요. 일시정지 중에도 수명 종료 시 제거되고 Clear 뒤 Unpause로 되살릴 수 없습니다.'),('CC와 취소 규칙','실제 Action 취소 시 액션 타이머가 제거됩니다. 아이템 보유 타이머는 별개입니다.'),('기존 일반 Timer','일반 엔진 타이머는 World입니다. 자동 전환되지 않으며 직접 Clear하거나 귀속 노드로 교체해야 합니다.'),('이미 적용한 효과','일반 보정·독립 투사체·DOT·Bool·구독은 Timer Clear만으로 사라지지 않습니다.'),('단순 1회 대기','Looping=false로 한 번 예약할 수 있습니다. Action Delay는 활성 Action Request의 Time Group을 자동으로 따르고 Completed/Cancelled로 분기합니다. 일반 Delay는 World이며 귀속 수명이 없습니다.')]),'주의점'))

def refresh_cleanup(text, kind):
    return re.sub(r'<p class="note[^\"]*"><strong>취소 후 직접 정리할 것:</strong>.*?</p>',lambda _: cleanup(kind),text,flags=re.S)

def add_managed_timers(text, context='all'):
    if 'managed-timers' in DetailBlocks(text).blocks:
        text=replace_block(text,'managed-timers','')
    # Place the working reference before existing chapters, not after the checklist.
    pos=text.index('<details class="chapter"')
    text=text[:pos]+managed_timers(context)+text[pos:]
    if 'href="#managed-timers"' not in text:
        text=re.sub(r'(<nav (?:class="local-toc"[^>]*|aria-label="목차")>\s*<div class="wrap">)',r'\1<a href="#managed-timers">권장 타이머</a>',text,count=1)
    return text

def navigation(path, category):
    home = rel(path, GUIDE_HOME)
    globals_ = link(home,'가이드 메인 페이지') + ''.join(f'<a href="{home}#{key}"'+(' aria-current="true"' if key==category else '')+f'>{name}</a>' for key,name in CATEGORIES)
    peers = ''.join(f'<a href="{rel(path, GUIDES/url)}"'+(' aria-current="page"' if path==GUIDES/url else '')+f'>{name}</a>' for key,url,name,_ in PAGES if key==category)
    return f'<nav class="guide-nav" aria-label="가이드 이동"><div class="wrap"><div class="guide-global">{globals_}</div>'+ (f'<div class="guide-peers">{peers}</div>' if peers else '')+'</div></nav>'

def page(url, category, title, lead, sections, sources=()):
    path = GUIDES/url
    toc = ''.join(link('#'+key, text) for key,text,_ in sections)
    footer = ' · '.join(link(rel(path, ROOT/source), label) for source,label in sources)
    return f'''<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{title} · Action RogueLike</title><link rel="stylesheet" href="{rel(path,GUIDES/'assets/guide.css')}"></head>
<body data-guide-ui="shared"><!-- GUIDE PORTAL START -->{navigation(path,category)}<!-- GUIDE PORTAL END -->
<header><div class="wrap"><div class="eyebrow">Action RogueLike · 개발자 참고서 · 2026-10-04</div><h1>{title}</h1><p class="lead">{lead}</p><p class="guide-contract">노드의 용도 → 연결 대상 → 입력·출력 → 정리 책임 순서로 확인합니다. 구조체는 설명 아래에서 펼쳐 보거나 Blueprint의 Split Struct Pin / Make 노드로 연결할 수 있습니다.</p></div></header>
<nav class="local-toc" aria-label="이 문서 목차"><div class="wrap">{toc}</div></nav>
<main class="wrap"><div class="guide-tools"><label for="node-search">노드·내용 찾기</label><input id="node-search" type="search" placeholder="노드 이름, 핀, 증상…" autocomplete="off"><button id="clear-search" type="button">검색 지우기</button><button id="expand-all" type="button">모두 펼치기</button><button id="collapse-all" type="button">모두 접기</button></div><p id="search-result" class="search-result" role="status" aria-live="polite"></p>
{''.join(html for _,_,html in sections)}
</main><footer><div class="wrap"><p>게임 코드·에셋을 자동 수정하지 않는 개발 참고서입니다. 엔진 기본 노드와 프로젝트 AR 노드를 구분하고, 실제 레벨에서 성공·실패·취소를 검증하세요.</p><p>{footer}</p></div></footer><script src="{rel(path,GUIDES/'assets/guide.js')}"></script></body></html>'''

def save(url, text):
    path = GUIDES/url
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\n')

COMBAT = 'Source/Action_RogueLike/Public/Foundation/Blueprint/ARCombatBlueprintLibrary.h'
ACTION = 'Source/Action_RogueLike/Public/Foundation/Components/ARActionComponent.h'
STATS = 'Source/Action_RogueLike/Public/Foundation/Blueprint/ARStatsBlueprintLibrary.h'
ITEM = 'Source/Action_RogueLike/Public/Foundation/Items/ARLoadoutItemInstance.h'

def build_controls():
    rows = [
        ('이동', 'W / A / S / D', '앞 / 왼쪽 / 뒤 / 오른쪽 방향 이동'),
        ('구르기 · 회피', 'Space', '기본 회피 입력'),
        ('인벤토리', 'I', '인벤토리 열기'),
        ('액티브 유물 1번 슬롯', 'E', '현재 1번 슬롯에 장착한 유물 사용'),
        ('액티브 유물 2번 슬롯', 'R', '현재 2번 슬롯에 장착한 유물 사용'),
        ('소모품 1번 슬롯', '1', '1번 슬롯 소모품 사용'),
        ('소모품 2번 슬롯', '2', '2번 슬롯 소모품 사용'),
        ('소모품 3번 슬롯', '3', '3번 슬롯 소모품 사용'),
        ('상호작용', 'F', '픽업 획득 등 현재 상호작용 대상에게 요청'),
        ('무기 1번 스킬', 'Shift', '무기 1번 스킬 입력'),
        ('무기 2번 스킬', 'Q', '무기 2번 스킬 입력'),
        ('무기 3번 스킬', 'X', '무기 3번 스킬 입력'),
    ]
    table = '<div class="scroll"><table><thead><tr><th scope="col">기능</th><th scope="col">기본 키</th><th scope="col">용도</th></tr></thead><tbody>'
    table += ''.join(f'<tr><th scope="row">{escape(name)}</th><td><code>{escape(key)}</code></td><td>{escape(description)}</td></tr>' for name,key,description in rows)
    table += '</tbody></table></div>'
    sections = [
        ('keys', '기본 조작표', chapter('keys', '기본 조작 · 제작 기준',
            '<p>콘텐츠 제작 시 아래 기본 키 배치를 기준으로 사용하세요. 유물·소모품 키는 현재 장착된 슬롯의 아이템을 사용하는 입력입니다.</p>'
            + table + note('이 문서는 키 배치 안내입니다. 가이드 편집만으로 플레이어의 실제 입력 설정이나 게임 에셋이 변경되지는 않습니다.'), opened=True)),
        ('assignment', '추가 지정·충돌 주의', chapter('assignment', '추가 키 지정 · 같은 키를 재사용할 때',
            '<p>위 기본 조작 키에 스킬을 <strong>중복 지정하는 것도 가능합니다.</strong> 의도적으로 같은 입력을 공유할 수 있지만 <strong>충돌에 주의하세요.</strong> 기존 동작이 자동으로 대체되거나 한 동작만 실행된다고 가정하지 마세요.</p>'
            + pins([('자유롭게 지정 가능한 권장 키', '<code>Z</code>, <code>C</code>. 새 콘텐츠에서 키가 필요한 경우 우선 고려할 수 있습니다. 다른 콘텐츠가 이미 사용하는지는 함께 확인하세요.'),
                    ('중복 지정 시 확인', '한 번 누를 때 기존 이동·상호작용·슬롯 사용·무기 스킬 중 어떤 동작들이 요청되는지 확인합니다. 예를 들어 F에 스킬을 추가하면 상호작용 입력과 겹칠 수 있습니다.'),
                    ('공유 입력의 스킬 우선순위', '같은 스킬 요청에 포함된 스킬들은 시스템의 우선순위·조건·비용 규칙으로 판단됩니다. 별개의 상호작용이나 슬롯 입력까지 이 우선순위가 자동으로 통합하는 것은 아닙니다.'),
                    ('동작 검증', '단독 사용과 중복 지정 상태를 모두 테스트하세요. UI를 열었을 때와 비용 부족·쿨타임·CC 중에도 의도한 입력만 실행되는지 확인합니다.')])
            + '<p><a href="../precautions/SKILL_PRIORITY_GUIDE_KO.html#rule">공유 스킬 입력의 우선순위·비용 규칙</a></p>')),
        ('contact', '키 추가·변경·건의', chapter('contact', '키 정책 문의',
            note('<strong>키를 추가하거나 변경하고 싶거나, 조작 키에 대한 건의가 있으면 이정환에게 문의하세요.</strong>', 'good')
            + '<p>원하는 키와 용도, 기존 키와의 중복 여부를 함께 알려주세요.</p>', opened=True)),
    ]
    save('common/BASIC_CONTROLS_GUIDE_KO.html', page('common/BASIC_CONTROLS_GUIDE_KO.html', 'common',
        '기본 조작 · 키 배치 참고서', '기본 키를 확인하고, 추가 지정과 충돌 주의사항을 확인합니다.', sections))

def build_cc():
    s=[]
    s.append(('distinction','구분',chapter('distinction','CC·경직·스탯 효과 구분',
        '<p>기절·속박은 상태 효과이며 경직은 타격 결과의 별도 시스템입니다. 이동 속도를 낮추는 스탯 효과는 공통 기절/속박 상태와 동일하지 않습니다.</p>'+pins([
        ('Stun · 기절','이동/구르기/새 스킬을 차단하고, 기존 액션에 Stun 사유의 취소를 요청합니다. 개별 액션의 취소 규칙을 따릅니다.'),
        ('Root · 속박','일반 이동/구르기를 막고 진행 중인 구르기를 취소합니다. 일반 스킬 전체를 기절처럼 취소하지는 않습니다.'),
        ('Stagger · 경직','타격의 최종 경직 수치가 저항보다 클 때 발동할 수 있습니다. 슈퍼아머·면역·대상 설정도 확인합니다.'),
        ('Groggy · 그로기','게이지 소진 반응과 복구는 적 콘텐츠에서 설계합니다. <a href="../hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html#groggy">적 가이드의 그로기·이벤트</a>에서 설명합니다.')]),True)))
    s.append(('apply-cc','간편 CC',chapter('apply-cc','DA 없이 기절·속박 적용',node('apply-crowd-control','Apply Crowd Control','Actor에게 기절 또는 속박을 지정한 시간 동안 적용합니다. 상태 DA를 만들 필요가 없는 간편 노드입니다.',pins([
        ('Target','상태를 받을 Actor. Status Effect Component를 가진 유효한 전투 대상이 필요합니다.'),('CC Type','Stun / Root 선택.'),('Duration','초 단위 양수, 기본 1초. 0·음수·비유한 값은 실패합니다.'),('Affected By Tenacity','기본 체크됨. 강인함에 따라 이번 요청의 지속시간을 줄입니다. 정확한 고정 시간을 의도하면 해제하지만 CC 면역까지 무시하지는 않습니다.'),('Success / Return Value','적용·갱신 성공 Bool / 상태 핸들. 성공했을 때만 핸들을 저장합니다.'),('고급 핀','Source: 출처 정보. Failure Reason: 실패 이유. Applied Duration: 이번 요청의 강인함 적용 후 시간이며 현재 남은 시간이 아닙니다.')])+note('같은 상태 태그는 중복 층 대신 더 긴 남은 시간으로 갱신하며 기존 핸들을 재사용합니다. 짧은 요청으로 기존 상태의 규칙이 바뀐다고 가정하지 마세요.')+node('cc-source','Source 구조체','상태 출처를 기록합니다. 취소될 액션의 핸들 또는 CC 종류 자체는 아닙니다.',pins([('Category','Weapon / Relic / Other 등 출처 분류.'),('Source Id','콘텐츠가 정한 효과 출처 이름.'),('Display Name','표시용 이름. 효과 식별 키가 아닙니다.')]),'구조체'), 'AR | Combat | CC'))))
    s.append(('stagger','경직 연결',chapter('stagger','직접 피해 결과 → 경직',node('result-stagger','Apply Stagger And Groggy Damage From Result','Apply Combat Damage의 결과를 그대로 받아 실제 적용된 타격에 경직 피해를 추가합니다. HP 피해를 다시 주지 않으며, 회피·차단·동일 Damage Name 제한으로 무시된 결과는 처리하지 않습니다.',pins([
        ('흰 실행선 / Damage Result','Apply Combat Damage 실행 출력 → 이 노드 실행 입력. 피해 Return Value → Damage Result.'),('Base Stagger Damage','타격의 기본 경직 수치. 0이면 경직 시도 없음.'),('Stagger Multiplier','경직 배율, 기본 1.'),('Base Groggy Damage','게이지 피해 기본값. 그로기를 안 쓸 공격은 0. 상세는 적 가이드.'),('고급 Source / Effect Name','경직 요청의 출처·효과 이름. 타격별 중복 방지 키가 아닙니다.'),('Return Value','경직 요청 결과 구조체. Valid Request / Staggered / Blocked By Super Armor 등 결과 확인.')])+'<div class="flow">Apply Combat Damage → Apply Stagger And Groggy Damage From Result<br>Damage Return Value ─→ Damage Result</div>'+note('같은 성공 결과를 이 노드에 여러 번 넣으면 반복 처리될 수 있습니다. 타격당 한 번만 연결합니다. DOT는 피해 가이드의 틱별 경직 옵션을 사용하세요.')+'<p><a href="DAMAGE_NODES_FORMULA_GUIDE_KO.html#stagger-result">피해 가이드의 상세 핀·DOT 연결</a></p>'))))
    s.append(('resistance','저항·면역',chapter('resistance','강인함·경직 저항·슈퍼아머·CC 면역',pins([
        ('Tenacity · 강인함','CC 지속시간 감소에 사용합니다. 체크된 유한 스탯 효과의 지속시간에도 적용할 수 있습니다. 경직 저항과 다른 값입니다.'),('Stagger Resistance · 경직 저항','최종 경직 피해가 이 값보다 <strong>클 때</strong> 경직이 가능합니다. 같으면 발동하지 않습니다. 경직 시간 자체를 강인함으로 줄이는 구조는 아닙니다.'),('Super Armor · 슈퍼아머','경직 발동을 막는 핸들 기반 효과. 기절·속박·그로기까지 모두 막는 통합 면역이 아닙니다.'),('CC Immunity · CC 면역','새 기절·속박 등 CC 적용을 막습니다. 이미 걸린 상태를 자동 제거하지는 않습니다.')])+node('guarantee-ownership','Apply Super Armor / Apply CC Immunity','일반 Actor 대상 효과입니다. 액션 동안만 보장하려면 Apply Action 계열, 아이템 보유 동안이면 Apply Item 계열을 선택합니다.',pins([('일반 Target / Spec','실제 대상 Actor / Duration과 Source. 성공 핸들을 보관해 대응 Remove 노드로 조기 제거합니다.'),('액션 귀속','Action Component + Action Handle. 액션 종료·취소 때 회수.'),('아이템 귀속','런타임 Self. 아이템 해제 때 회수.')])+'<p><a href="../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#owned-resources">액션 귀속과 자동 정리</a></p>'))))
    s.append(('stat-cc','스탯 효과',chapter('stat-cc','스탯으로 구현한 둔화 등 · 강인함 체크만',
        '<p>스탯 조정으로 둔화 같은 효과를 구현한다면 <code>Apply Stat Modifier</code> 또는 해당 귀속 노드의 <strong>Spec Affected By Tenacity</strong>를 체크하여 유한 지속시간에 강인함을 적용할 수 있습니다.</p><p>세부 필드·시간 계산·수명·제거는 <a href="../precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html#tenacity">스탯 조작·생명주기의 강인함 항목</a>을 보세요. 여기서는 어떤 체크를 쓰는지만 안내합니다. 스탯 변경 자체가 Stun/Root 상태를 등록하는 것은 아닙니다.</p>')))
    s.append(('removed','해제·취소',chapter('removed','상태 해제와 액션 취소 알림',node('remove-status','Remove Status Effect','저장한 정확한 상태 핸들로 상태를 조기 제거합니다.',pins([('Target','상태를 보유한 Actor.'),('Handle','Apply Crowd Control의 성공 Return Value.'),('Return Value','제거 성공 Bool. 이미 만료된 핸들은 실패할 수 있습니다.')]))+node('status-event','On Status Removed','실제로 상태가 사라졌을 때 후처리합니다. ARBaseCharacter 자식에서는 직접 이벤트를 놓을 수 있습니다.',pins([('Status','제거된 상태 구조체. Handle을 자신이 저장한 상태 핸들과 비교하여 자신의 효과만 처리합니다.')]))+node('cancel-event','On Item Skill Cancelled / On Action Cancelled','경직·기절이 실제 액션 취소로 이어진 경우 실행별 콘텐츠 정리를 받는 알림입니다.',cleanup('item')+'<p><a href="../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#cancel-events">캐릭터/아이템 이벤트 구분과 타이머 정리</a></p>','이벤트'))))
    save('common/CC_STAGGER_GUIDE_KO.html',page('common/CC_STAGGER_GUIDE_KO.html','common','CC·경직 참고서','간편 기절·속박, 피해 뒤 경직 연결, 저항과 면역을 필요한 항목만 펼쳐 확인합니다. 그로기 소진·복구는 적 가이드에서 다룹니다.',s,[(COMBAT,'CC·경직 노드'),('Source/Action_RogueLike/Private/Foundation/Components/ARStaggerComponent.cpp','경직 계산'),('Source/Action_RogueLike/Private/Foundation/Components/ARStatusEffectComponent.cpp','상태·강인함')]))

def build_priority():
    s=[]
    s.append(('rule','검사 순서',chapter('rule','작은 숫자부터 · 같은 숫자는 한 묶음',
        '<p>아이템 DA의 <code>Skill Definitions → Input Priority</code>를 설정합니다. <strong>작은 숫자가 먼저</strong>이며 0 → 1 → 2 순서입니다. 0이 반드시 있어야 하는 것은 아니고, -1도 0보다 먼저 검사됩니다.</p><p>이 규칙은 <strong>한 번의 입력 요청에서 선정된 후보</strong>끼리 적용됩니다. 서로 독립적으로 들어온 요청들을 물리 키가 같다는 이유만으로 한 거래에 합치지 않습니다.</p>'+pins([
        ('동일 우선순위','같은 숫자의 모든 후보가 한 묶음입니다. 조건을 각각 검사하고 마나·스태미나 비용을 자원별로 합산합니다. 일부만 골라 실행하지 않습니다.'),('조건 실패','묶음의 하나라도 쿨타임·Can Execute Item Skill·액션 시작 검사에 실패하면 <strong>그 묶음 전체와 이후 묶음</strong>을 중단합니다.'),('비용 부족','앞서 통과한 묶음의 예약 비용 + 현재 묶음 비용을 감당할 수 없으면 동일하게 중단합니다. 뒤의 저렴한 묶음으로 건너뛰지 않습니다.'),('앞서 통과한 묶음','통과한 앞쪽 묶음만 최종 시작할 수 있습니다. 첫 묶음부터 실패했다면 실행도 비용 소비도 없습니다.')]),True)))
    s.append(('cost','누적 비용',chapter('cost','묶음 비용 합산 · 앞선 비용까지 포함',
        '<div class="flow">후보 정렬 → 같은 우선순위 조건 검사 → 묶음 비용 합산<br>현재 자원 ≥ 앞선 예약 비용 + 묶음 비용 ?<br>예: 묶음 추가 → 다음 우선순위 / 아니오: 중단 → 앞서 통과한 묶음만 실행</div>'+note('모든 우선순위의 비용을 처음부터 한꺼번에 합산해서 전부 거절하는 구조도, 같은 묶음에서 가능한 스킬만 실행하는 구조도 아닙니다.')+
        '<div class="scroll"><table><thead><tr><th>보유 마나 100</th><th>묶음 비용</th><th>판정</th></tr></thead><tbody><tr><td>우선순위 0 · A20 + B20</td><td>40</td><td>통과, 40 예약</td></tr><tr><td>우선순위 1 · C35 + D35</td><td>70</td><td>누적110이므로 묶음 전체 거절</td></tr><tr><td>우선순위 2 · E5</td><td>5</td><td>1에서 중단했으므로 검사하지 않음</td></tr></tbody></table></div><p>최종 A와 B만 시작하며 마나40 소비. 마나와 스태미나는 별개로 검사하며 한 자원이라도 부족하면 묶음이 실패합니다.</p>')))
    s.append(('preflight','사전 조건',chapter('preflight','Can Execute Item Skill · 사용자 정의 조건',node('can-execute','Can Execute Item Skill','DA 자원 비용 외의 조건을 실행 전에 판단하는 BP 함수 오버라이드입니다. 탄약·대상·자체 변수 등을 조회하고 Bool을 반환합니다.',pins([('Skill Id','검사 중인 아이템 내부 스킬 이름. 스킬별 조건을 분기합니다.'),('Return Value','True면 이 조건 통과. 이후 공통 검사까지 모두 통과해야 실제 실행됩니다.'),('Failure Tag','선택적인 실패 이유. 자동 HUD 출력 기능은 아닙니다.')])+note('읽기 전용으로 작성합니다. 여기서 비용 차감·스택 소비·효과 적용·타이머 시작을 하지 마세요. DA 비용 외 탄약 소비는 실행 시 별도로 성공 여부를 확인해야 합니다.')+'<p><a href="../nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html#can-execute">오버라이드 만드는 단계와 핀 연결</a></p>','함수 오버라이드'))))
    s.append(('commit','실행·환불',chapter('commit','통과 뒤 거래와 실행 · 실패 시 구분',
        '<p>통과한 후보의 총 비용을 지불하고 액션들을 시작한 뒤 쿨타임을 기록하여 <code>Execute Item Skill</code>을 호출합니다. 같은 우선순위는 원자적으로 승인하는 단위이지 BP 효과를 문자 그대로 동시에 실행하는 스레드라는 뜻이 아닙니다.</p>'+pins([
        ('액션 시작 거래 실패','최종 액션 시작 중 실패하면 이미 시작한 핸들을 취소하고 거래 비용을 돌려주는 방어 경로가 있습니다. 이 경우 앞서 통과한 묶음도 실행 확정된 것이 아닙니다.'),('Execute Item Skill 이후 콘텐츠 실패','이미 지불한 비용과 기록한 쿨타임은 자동 환불되지 않습니다. 효과가 실패했어도 받은 액션의 정상 종료/취소 책임은 남습니다.'),('실행 중 취소','경직·기절 등으로 취소되어도 자동 비용/쿨타임 환불을 뜻하지 않습니다.'),('활성 스킬 그룹','현재 시스템은 활성 그룹이 남아 있으면 새 스킬 요청을 차단합니다. End Action 누락을 확인하세요.')])+'<p><a href="ACTION_LIFECYCLE_GUIDE_KO.html">실행·종료·취소 생명주기</a></p>')))
    s.append(('check','점검표',chapter('check','제작자 점검표',
        '<ul><li>함께 성공하거나 함께 실패해야 하는 스킬에 같은 우선순위를 사용했는가?</li><li>앞쪽 스킬이 쿨타임이면 뒤쪽도 막히는 것이 의도인가?</li><li>같은 묶음에서 비용을 한 번만 처리하는가? DA 비용을 BP에서 다시 소비하지 않는가?</li><li>Can Execute Item Skill이 검사만 하고 부작용을 만들지 않는가?</li><li>정상 완료와 실패 출구 모두 받은 핸들을 끝내는가?</li></ul>')))
    save('precautions/SKILL_PRIORITY_GUIDE_KO.html',page('precautions/SKILL_PRIORITY_GUIDE_KO.html','precautions','스킬 우선순위·비용 거래','한 입력 요청의 후보를 우선순위 묶음으로 판정하는 실제 시스템 규칙입니다. 키·Input Action·입력 태그의 설정 방법은 다루지 않습니다.',s,[('Source/Action_RogueLike/Private/Foundation/Components/ARLoadoutComponent.cpp','ExecuteSkillCandidates 실제 처리'),('Source/Action_RogueLike/Private/Foundation/Tests/ARFoundationComponentTests.cpp','우선순위 거래 검증')]))

def build_lifecycle():
    s=[]
    s.append(('managed-timers','귀속 타이머',managed_timers()))
    s.append(('contract','기본 계약',chapter('contract','시작 경로·핸들·정상 종료',pins([
        ('아이템 스킬','Execute Item Skill에 도달했다면 시스템이 이미 액션을 시작하고 DA 비용·쿨타임을 처리했습니다. 받은 Action Handle을 사용하고 동일 액션을 다시 시작하지 않습니다.'),('적의 직접 액션','Get Action Component → Try Start Action → Status 성공 확인 → Return Value 핸들 저장. 비용·쿨타임·패턴 조건은 적 콘텐츠 책임입니다.'),('정상 완료','같은 Action Component의 End Action에 이번 Action Handle을 전달합니다. 완료/빗나감/생성 실패 등 모든 출구에 종료 책임을 둡니다.'),('식별값','Action Component는 관리자 참조, Action Handle은 한 실행의 식별값입니다. Skill Id / Timer Handle / Stat Modifier Handle과 대체할 수 없습니다.')]),True)))
    s.append(('owned-resources','자동 정리',chapter('owned-resources','자동 정리와 직접 정리의 경계',
        '<div class="scroll"><table><thead><tr><th>대상</th><th>정리 시점·담당</th></tr></thead><tbody><tr><td>Apply Action Stat Modifier / Super Armor / CC Immunity</td><td>시간 만료 또는 액션 End/Cancel 때 공통 회수</td></tr><tr><td>Register Action Hitbox로 등록한 Actor</td><td>액션 End/Cancel 때 Destroy</td></tr><tr><td>액션 귀속 이동 잠금</td><td>해당 액션 종료 시 회수. 다른 잠금은 남음</td></tr><tr><td>Action Delay 내부 대기</td><td>액션 End/Cancel 때 정리, Cancelled 경로 통보</td></tr><tr><td>Set Timer by Event의 Timer Handle</td><td>자동 액션 귀속 없음. 제작자가 Clear</td></tr><tr><td>독립 투사체·다른 Actor의 효과·별도 DOT</td><td>일반 적용/생성만으로 액션 귀속되지 않음. 유지/제거 정책을 별도 구현</td></tr><tr><td>공격 Bool·연출·자신이 Bind한 구독</td><td>제작자가 초기화·정확한 연결 해제</td></tr></tbody></table></div>'+note('Source Id 또는 Spawn Actor의 Owner를 채웠다는 이유로 액션 자동 정리 목록에 등록되지 않습니다. 귀속 노드를 통해 성공한 핸들/Actor가 등록됐는지가 기준입니다.'))))
    s.append(('cancel-events','취소 이벤트',chapter('cancel-events','취소 이벤트 · 용도 아래에 정리 연결',
        node('item-cancelled','On Item Skill Cancelled','아이템 런타임에서 실제 실행했던 자기 스킬이 취소됐을 때 받습니다. 직접 이벤트이며 별도 Bind가 필요 없습니다.',cleanup('item')+pins([('Skill Id','취소된 아이템 내부 스킬. Switch on Name으로 정리 분기.'),('Action Handle','취소된 특정 실행. 저장 핸들과 비교해 오래된 콜백이 새 실행을 지우지 않게 합니다.'),('Reason','Stagger / Stun / ItemRemoved / Death / Manual 등 실제 취소 사유.')])+note('정상 End Action·사전 검사 실패·실행 전 거래 롤백에는 이 스킬 취소 알림이 오지 않습니다. 이미 취소된 액션을 다시 End Action하지 않습니다.'),'아이템 직접 이벤트')+
        node('character-cancelled','On Action Cancelled','ARBaseCharacter 자식(적/플레이어)에서 자기 행동 취소를 직접 받습니다. 일반 Actor는 보유한 Action Component의 디스패처를 구독해야 합니다.',cleanup('enemy')+pins([('Action Handle','관리자가 취소한 실행. 자기 공격 핸들과 비교합니다.'),('Reason','취소 사유. 특정 사유만 정리할지 정책을 정하되 모든 실제 취소 경로에 필요한 정리를 빠뜨리지 않습니다.')])+note('한 처리에 직접 이벤트와 Bind를 중복 연결하지 않습니다. Action Delay.Cancelled에도 같은 정리를 연결했다면 중복 호출돼도 안전하게 구성합니다.'),'캐릭터 직접 이벤트'))))
    s.append(('timer-cleanup','일반 Timer 예외',chapter('timer-cleanup','일반 엔진 Timer가 필요한 예외·조기 제거',node('clear-timer','Clear and Invalidate Timer by Handle','기존 일반 Timer의 수동 정리, 귀속 Timer를 수명보다 먼저 끝내려는 경우에 사용합니다. 새 스킬·아이템은 위의 전용 Timer를 우선 사용하세요.',pins([('Handle','각 Set Timer 노드의 Return Value를 저장한 Timer Handle 변수. Action Handle을 넣는 곳이 아닙니다.')])+'<div class="flow">상시 AI/스포너: BeginPlay → 일반 Set Timer by Event → Timer Handle 저장<br>Actor EndPlay → Clear and Invalidate Timer by Handle<br>기존 일반 공격 Timer: 실제 취소 이벤트 → 실행 핸들 비교 → Clear → 자체 상태 초기화</div>'+note('Pause / Unpause는 남은 시간을 보존·재개할 때 사용합니다. Clear한 Timer는 Unpause로 되살릴 수 없습니다. 전용 Timer는 액션 종료/아이템 해제로 자동 제거되므로 그 목적으로 수동 Clear를 중복 작성할 필요는 없습니다. 변수·연출 정리는 별개입니다.')+'<p>AI 전체 판단·웨이브 루프를 한 공격의 액션 핸들에 묶으면 공격이 끝날 때 판단도 멈춥니다. 공격용 예약만 액션 Timer로 분리하세요.</p>','언리얼 기본'))))
    s.append(('delay','순차 대기 옵션',chapter('delay','Action Delay · 순차 실행 분기가 필요할 때',node('action-delay','Action Delay','한 번 기다린 뒤 실행선을 이어가고 Completed/Cancelled로 분기하려면 사용할 수 있는 액션 귀속 대기입니다. 반복 콜백은 Set Action Timer by Event를 사용하세요.',pins([('Action Component / Action Handle','이번 실행의 같은 관리자와 활성 핸들.'),('Duration','초 단위 대기 시간.'),('Completed','시간을 채운 후 효과/판정/다음 단계. 여기서 정상 End Action까지 이어갑니다.'),('Cancelled','대기 중 End/Cancel 또는 무효 요청. 공격하지 않고 자체 정리만 수행합니다.')])+note('상단 즉시 실행 출력은 대기를 마친 출력이 아닙니다. Action Delay의 Cancelled는 정상 End에서도 올 수 있으므로 취소 사유가 필요하면 실제 취소 이벤트를 사용하세요.')))))
    s.append(('rules','취소 규칙',chapter('rules','Cancel Rules · 종료와 강제 취소 구분',pins([
        ('Cancel On Stagger / Cancel On Stun','기본 true. 해당 사유의 취소 요청을 받았을 때 현재 액션을 취소하도록 합니다. 꺼져 있다면 경직/기절이 생겨도 해당 이유로 이 액션이 자동 취소되지는 않을 수 있습니다.'),('Cancel On Roll / Cancel On Basic Movement Input','기본 false. 액션별 중단 정책을 설정합니다.'),('Cancel Action','특정 핸들을 직접 취소. 자동 사유 규칙과 별개의 명시적 요청입니다.'),('End Action','정상 종료 알림. 취소 이벤트를 대신 발생시키는 노드가 아닙니다.'),('시간만 끝내고 종료하지 않은 경우','일반 타이머 완료나 Action Delay.Completed만으로 액션이 끝나지 않습니다. End Action을 연결해야 합니다.')])+'<p><a href="../hhc/ENEMY_ACTION_GUIDE_KO.html#action-request">적의 Action Request 입력</a> · <a href="../nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html#cancel">아이템 액션 설정</a></p>')))
    save('precautions/ACTION_LIFECYCLE_GUIDE_KO.html',page('precautions/ACTION_LIFECYCLE_GUIDE_KO.html','precautions','액션 핸들·생명주기·정리 책임','아이템과 적이 공유하는 종료 계약입니다. 취소 이벤트의 용도 바로 아래에서 자체 타이머 정리까지 확인할 수 있습니다.',s,[(ACTION,'액션 API'),(ITEM,'아이템 취소 이벤트'),('Source/Action_RogueLike/Private/Foundation/Actions/ARAsyncActionDelay.cpp','액션 대기')]))

def build_stat_lifecycle():
    s=[]
    s.append(('notifications','변경 알림 안전성',chapter('notifications','스탯 변경 이벤트 안에서 다시 변경할 때',
        '<p>스탯 적용·제거는 최종값 캐시와 귀속 핸들 등록을 끝낸 뒤 변경 알림을 보냅니다. 묶음 제거는 관련 스탯 최종값을 모두 갱신한 상태에서 알림을 전달합니다.</p><p>알림 콜백에서 다시 적용·제거하면 데이터는 즉시 바뀌지만, 추가 알림은 현재 알림 전달 이후 순서대로 처리됩니다. 같은 처리에서 대기 중인 동일 스탯/출처 알림은 합쳐질 수 있습니다. 이벤트 개수를 적용 횟수나 스택 수로 사용하지 말고 조회 노드로 확인하세요.</p>'+note('한 콜백이 같은 값을 계속 변경해 자기 이벤트를 무한 유발하면 처리 예산 경고를 기록하고 남은 알림은 다음 틱으로 넘깁니다. 재귀·한 프레임 무한 반복 방지 장치이지 잘못된 콘텐츠 로직의 자동 수정은 아닙니다. 콜백 안에서 즉시 제거된 효과의 반환 핸들은 이미 비활성일 수 있습니다.'))))
    s.append(('choose','노드 선택',chapter('choose','어디까지 유지할지 먼저 선택',
        '<div class="scroll"><table><thead><tr><th>의도</th><th>권장 노드</th><th>Target / 수명</th></tr></thead><tbody><tr><td>대상에게 독립 버프·스택</td><td>Apply Stat Modifier</td><td>Actor / 시간·명시적 제거·대상 수명</td></tr><tr><td>아이템 버리면 사라질 효과</td><td>Apply Item Stat Modifier</td><td>런타임 Self / 시간 또는 아이템 해제</td></tr><tr><td>행동 끝나면 사라질 효과</td><td>Apply Action Stat Modifier</td><td>Action Component + Action Handle / 시간 또는 End·Cancel</td></tr><tr><td>버프 없이 기본값 증가·감소</td><td>Apply Stat Modifier · Permanent Flat</td><td>Actor / 기본값 자체 변경, 제거 핸들 없음</td></tr></tbody></table></div>'+note('위 세 적용 노드는 생명주기가 달라 중복이 아닙니다. Blueprint 작성 위치가 아이템 내부라는 이유만으로 일반 노드가 아이템 귀속으로 바뀌지 않습니다. Duration=-1도 아이템/액션 해제를 무시하지 않습니다.'),True)))
    apply_body=pins([('Target','일반 노드는 실제 대상 Actor. 아이템에서는 Get Item Owner, 픽업에서는 실제 Interactor를 전달합니다.'),('Spec','스탯·연산·값·시간·출처·스택·HUD 설정 구조체. 아래에서 필드별로 펼칩니다.'),('Success','요청 성공 Bool. 실패면 효과가 생겼다고 간주하지 않습니다.'),('Return Value','일반 임시 효과의 Stat Modifier Handle. 성공했을 때 저장해 정확한 조기 제거에 사용합니다. Permanent Flat 성공은 빈 핸들입니다.')])
    s.append(('apply','적용 핀',chapter('apply','스탯 적용 노드 · 실제 연결 대상',node('apply-stat','Apply Stat Modifier','Actor에게 스탯 보정 또는 기록용 스택을 추가합니다. Permanent Flat은 보정 대신 기본값 증감으로 분기합니다.',apply_body)+node('apply-item','Apply Item Stat Modifier','소유 플레이어에게 보정을 적용하고 이 아이템의 자동 회수 목록에 넣습니다.',pins([('Target','ARLoadoutItemInstance 런타임 Self. 플레이어 Actor를 연결하는 핀이 아닙니다.'),('Spec','같은 스탯 구조체. Category는 아이템 종류에 맞춰 Weapon/Relic으로 설정되고, Source Id가 비어 있으면 인스턴스 기반 값이 사용됩니다.'),('Success / Return Value','성공 Bool / 보정 핸들. 아이템 해제 때 회수됩니다. Permanent Flat은 거절.')]))+node('apply-action','Apply Action Stat Modifier','액션 소유자의 스탯에 보정을 적용하고 현재 행동에 귀속합니다.',pins([('Target','Action Handle을 발급한 Action Component.'),('Handle','같은 관리자의 활성 Action Handle. 보정 반환 핸들과 다른 타입입니다.'),('Spec','스탯 설정. Permanent Flat은 거절합니다.'),('Success / Return Value','성공 Bool / Stat Modifier Handle. 시간 전에 액션이 끝나면 함께 제거됩니다.')])))))
    spec = node('spec-stat','Stat Type / Operation / Value','어떤 숫자를 어떤 방식으로 바꿀지 정합니다.',pins([('Stat Type','EARStatType. Money는 플레이어만 지원합니다. 현재 체력/현재 마나는 최대 스탯과 다릅니다.'),('Flat','고정치 추가. 퍼센트 단위 스탯에 Flat 50이면 그 스탯 수치에 50%p 추가.'),('Additive Percent','일반 계산에서 Base+Flat 값에 대한 합산 비율, Value=50이면 +50%. 기본이 0이면 비율만 추가해도 0일 수 있습니다.'),('Multiplicative','배율끼리 곱합니다. Value=2는 2배.'),('Independent Damage Reduction','피해 감소 스탯에서만 사용합니다. Value=20이면 별도20% 감소 층으로 잔여 배율에 곱합니다. 감소20%와30%는 1−(0.8×0.7)=44% 감소. 공격력 등 다른 스탯에 사용하면 실패합니다.'),('Permanent Flat','일반 Apply Stat Modifier에서만 기본값 증감. 아래 전용 항목 참조.')]),'구조체 필드')
    spec += node('duration','Duration / Source','효과 수명과 검색·제거 키를 정합니다.',pins([('Duration','양수 초: 시간 만료. 음수: 시간 제한 없음(정리 소유권 종료는 적용). 0은 보정 적용 실패. Permanent Flat에서는 무시.'),('Source.Category','출처 분류. 검색·제거 때 실제 적용된 Category와 맞춰야 합니다.'),('Source.Source Id','효과 출처 이름. 이름만으로 한 인스턴스 전용이 되는 것은 아닙니다.'),('Source.Display Name','표시용 이름. 검색 식별 키는 Category + Source Id입니다.')]),'구조체 필드')
    spec += node('stack-fields','Stack Only / Stack Group Handle','스탯 변화 없이 개수를 기록하거나 여러 보정을 한 스택으로 묶습니다.',pins([('Stack Only','체크하면 숫자 스탯을 바꾸지 않고 1개 적용 그룹을 기록합니다. 탄약 같은 카운트용. Permanent Flat과 함께 사용 불가.'),('Stack Group Handle','첫 보정의 성공 핸들을 후속 보정에 전달하면 같은 스택 그룹에 묶습니다. 같은 출처여도 그룹당 여러 스탯/수명이 있을 수 있습니다.'),('스택 개수','수정치 레코드 개수가 아니라 해당 출처의 서로 다른 적용 그룹 수입니다. 한 그룹 제거 시 그 그룹의 여러 보정이 함께 제거될 수 있습니다.')]),'구조체 필드')
    spec += node('hud','HUD Display / HUD Name / HUD Icon','표시할 버프 정보를 지정합니다. 표시를 숨겨도 실제 효과와 정리 수명은 그대로입니다.',pins([('HUD Display','Hidden / Buff / Debuff. Hidden이면 HUD Name/Icon 사용 안 함.'),('HUD Name / HUD Icon','표시용 이름 / 아이콘 에셋. 일부 값은 Details에서 조건부 비활성화됩니다.'),('Guarantee Invulnerability','허용되는 Overall Damage Reduction 보정의 확정 무적 옵션. 일반 스탯 숫자 변경과 별도 의미.'),('Guarantee Evasion','허용되는 Evasion 보정의 확정 회피 옵션. Stack Only/Permanent Flat과 조합 불가.')]),'구조체 필드')
    s.append(('spec','구조체 필드',chapter('spec','Spec 펼치기 · 실제 필드별 설명','<p>Blueprint에서 Spec 핀 우클릭 → <code>Split Struct Pin</code>으로 항목을 펼치거나, <code>Make ARStatModifierSpec</code>을 통째로 연결합니다. 에디터의 조건부 숨김과 기존 분할 핀 표시는 다를 수 있으므로 쓰지 않는 필드는 의미를 확인하세요.</p>'+spec)))
    s.append(('tenacity','강인함',chapter('tenacity','Affected By Tenacity · 유한 효과 시간 감소',node('affected-by-tenacity','Spec Affected By Tenacity','스탯 변화로 구현한 둔화 등도 강인함의 지속시간 감소를 받도록 선택할 수 있습니다.',pins([('설정','Apply Stat Modifier / Apply Item Stat Modifier / Apply Action Stat Modifier의 Spec에서 Affected By Tenacity 체크. 기본 false.'),('Duration','양수인 유한 시간 효과에만 적용. Duration=-1의 무기한 효과나 Permanent Flat에는 적용 안 됨.'),('계산','적용 시점 대상의 최종 Tenacity를 사용: Duration × (1 − Clamp(Tenacity,0,100)/100). 값의 크기를 줄이는 것이 아니라 시간을 줄입니다.'),('예시','Duration=5, Tenacity=40이면 3초. 대상의 이후 강인함 변화로 기존 타이머를 계속 다시 계산하지 않습니다.'),('성공 확인','최종 지속시간이 0이면 적용이 실패할 수 있습니다. Success를 검사하며 무조건 핸들을 저장하지 않습니다.')])+note('스탯 보정은 이동 속도 등의 수치를 바꿀 뿐 기절·속박 상태 등록이 아닙니다. 실제 Stun/Root는 <a href="../common/CC_STAGGER_GUIDE_KO.html#apply-crowd-control">Apply Crowd Control</a>을 사용합니다.')))))
    s.append(('permanent-flat','영구 기본값',chapter('permanent-flat','Permanent Flat · 돈·기본 성장',node('permanent','Apply Stat Modifier · Operation=Permanent Flat','버프 레코드를 추가하지 않고 기본값에 Value를 직접 더합니다. 양수 증가 / 음수 감소.',pins([('Target / Stat Type','기본값을 변경할 Actor / 원하는 스탯. Money는 ARPlayerCharacter 및 자식만 지원, 기본0, float 소수 유지.'),('Operation / Value','Permanent Flat / 기본값 증감량. 변경된 기본값에 기존 임시 Flat·퍼센트·곱연산과 최종 안전 범위를 재적용합니다.'),('Success / Return Value','Success로 판단. 성공이어도 Return Value는 빈 보정 핸들입니다. Is Valid 핸들 검사로 성공 여부를 판단하지 않습니다.'),('무시되는 필드','Duration / Source / HUD / Affected By Tenacity / Stack Group. Stack Only와 무적·회피 Guarantee는 해제해야 합니다.'),('제거·수명','시간 만료·출처 제거·아이템 해제·액션 종료로 되돌리지 않습니다. 대상 객체 수명 안에서 유지되며 자동 세이브/새 캐릭터 이전은 아닙니다.'),('돈 차감','기본 잔액 + 음수 Value가 0보다 작으면 아무 변경 없이 실패. 임시 Money 버프는 기본 잔액 부족을 메우지 않습니다.'),('지원 제한','Apply Item / Apply Action / 컴포넌트 Add Stat Modifier / DA Default Stat Modifiers에서는 이 모드를 거절. 아이템 BP에서도 일반 Apply 노드의 Target=Get Item Owner면 사용 가능.')])+note('Flat + Duration=-1은 제거할 수 있는 무기한 버프입니다. Permanent Flat은 기본값 자체를 변경하므로 다른 동작입니다. 중복 획득·이벤트 재진입에 의한 중복 지급은 제작자가 방지합니다.')))))
    s.append(('remove','제거·차감',chapter('remove','핸들·출처·수량에 따른 제거',node('remove-handle','Remove Stat Modifier / Remove Own Item Modifier','한 효과를 조기 제거하려면 적용 때 받은 정확한 보정 핸들을 사용합니다.',pins([('Remove Stat Modifier','일반 라이브러리 버전: Target=효과를 가진 Actor, Handle=성공 보정 핸들 → Bool.'),('Remove Own Item Modifier','Target=런타임 Self, Handle=이 아이템이 소유한 보정 → Bool. 다른 아이템/액션 소유 핸들은 대상이 아닙니다.'),('이미 만료','실패 Bool일 수 있습니다. 핸들이 남아 있는 것과 효과가 활성인 것은 다릅니다.')]))+node('remove-source','Remove Stat Modifiers by Source','효과 Category와 Source Id로 해당 출처의 보정을 한 번에 제거합니다.',pins([('Target','효과를 보유한 Actor. 아이템 Self가 아닙니다.'),('Category / Source Id','두 키를 실제 적용 값과 일치시킵니다. Category=All / Source Id=None은 전체를 포함하는 와일드카드이므로 실수로 쓰지 않습니다.'),('Return Value','제거된 수정치 레코드 건수. 스택 그룹 개수와 같다고 가정하지 마세요.')])+note('공유 출처라면 다른 아이템이 준 같은 키의 효과도 포함할 수 있습니다. 한 아이템만 조기 정리하려면 자신의 핸들이나 Own Item 노드를 사용합니다.'))+node('remove-stacks','Remove Stat Modifier Stacks','원하는 개수의 적용 그룹을 소비합니다. 1개 소비도 Count=1로 처리하므로 이전 단일 제거 노드를 따로 사용할 필요가 없습니다.',pins([('Target / Category / Source Id','효과 보유 Actor / 적용 때의 두 출처 키.'),('Count','제거할 스택 그룹 수. 양의 정수, 기본1.'),('Require Full Count','기본 true. 부족하면 하나도 제거하지 않고 실패. false면 있는 만큼 제거.'),('Policy','기본 Oldest: 오래된 그룹부터 / Newest: 최근 그룹부터. 같은 출처라도 구성 효과와 만료 시간이 다를 수 있습니다.'),('Return Value / Removed Count','성공 Bool / 실제 제거된 스택 그룹 수. 소비 성공 후 후속 효과를 실행합니다.')])))))
    s.append(('query','조회·주의점',chapter('query','값·스택·남은 시간 조회',node('query-source','Get Stat Modifiers by Source','Category와 Source Id로 현재 적용 그룹 정보를 조회합니다. 스탯 수치를 바꾸거나 스택을 소비하지 않습니다.',pins([('Target / Category / Source Id','효과 보유 Actor / 두 출처 키.'),('Exists / Stack Count','현재 효과 존재 / 그룹 개수.'),('Has Permanent','Duration이 음수인 <strong>무기한 버프</strong>가 있는지. Permanent Flat 이력을 찾는 필드가 아닙니다.'),('Longest Remaining Time','해당 출처의 가장 긴 남은 시간. 무기한 버프가 있으면 -1.')]))+node('query-stat','Get Final Stat / Get Stat Breakdown','실제 계산 수치나 Base·Flat·Percent·Multiplier 내역을 조회합니다. 기본값만은 Stats Component의 Get Base Stat으로 확인합니다.','<p><a href="../common/OBJECT_STAT_GUIDE_KO.html">객체별 스탯과 공식</a></p>')+note('일반 Apply를 아이템에서 호출하면 해제 후에도 남을 수 있습니다. 액션 귀속을 너무 일찍 End하면 5초 효과도 즉시 회수될 수 있습니다. 영구 성장을 위한 Permanent Flat을 버프로 제거하려 하지 마세요.'))))
    save('precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html',page('precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html','precautions','스탯 조작 노드·생명주기','효과를 얼마나 유지할지 먼저 고르고, 적용·조회·제거 핀을 찾아보는 상세 참고서입니다.',s,[(STATS,'일반 스탯 노드'),('Source/Action_RogueLike/Public/Foundation/Components/ARStatsComponent.h','Spec 전체 필드'),(ITEM,'아이템 소유 효과'),(ACTION,'액션 소유 효과')]))

def build_environment():
    spawn=[]
    spawn.append(('prepare','스포너 역할',chapter('prepare','적 BP와 스포너의 역할',
        '<p>적 BP는 한 개체의 이동·공격·사망을 담당합니다. 스포너 Actor는 어떤 적을 언제 어디에 몇 마리 만들지, 최대 수와 재시도를 관리합니다. 하나의 적 BP 클래스를 여러 번 생성할 수 있으며 인스턴스마다 C++ 선언을 할 필요는 없습니다.</p>'+pins([('적 클래스','ARBaseEnemy 자식 BP. 외형·Capsule·Base Stats·AI Controller Class·Auto Possess AI를 준비합니다.'),('생성 지점','충돌과 바닥 높이를 고려한 월드 좌표. 경로 추적 적은 NavMesh 위에서 시작할 수 있도록 검증합니다.'),('확률·관리 목록','생성 전에 클래스와 지점을 선택하고, 생성 성공한 Actor만 목록/생존 수에 등록합니다.')]),True)))
    spawn.append(('spawn-enemy','생성 노드',chapter('spawn-enemy','Spawn Actor from Class',node('spawn-actor','Spawn Actor from Class','선택한 적 클래스를 월드에 생성하는 언리얼 기본 노드입니다. 확률·웨이브 규칙·보상 계산을 내장하지 않습니다.',pins([('Class','ARBaseEnemy 자식 BP 클래스 참조. DA 객체 참조를 직접 넣는 핀이 아닙니다.'),('Spawn Transform','월드 Location / Rotation / Scale. Capsule이 바닥 안에 파묻히지 않도록 위치를 정합니다.'),('Collision Handling Override','막힌 위치에서 조정·실패·강제 생성 중 어떤 정책을 쓸지 선택. 기본값만 믿지 말고 실패 경로를 연결합니다.'),('Owner / Instigator','선택적인 엔진 소유자/행위자. 이것만으로 팀·피해 출처·액션 수명이 자동 설정되지 않습니다.'),('Return Value','생성된 Actor 참조. Is Valid 확인 뒤 관리 목록에 추가합니다.'),('Expose on Spawn 변수','해당 적 BP에서 노출한 값이 있을 때만 추가 핀이 생깁니다. 모든 Spawn 노드의 공통 핀이 아닙니다.')])+'<div class="flow">클래스·위치 선택 → Make Transform → Spawn Actor from Class → Is Valid<br>성공: 목록 추가 / 실패: 재시도·생성 수 처리 정책</div>','언리얼 기본'))))
    spawn.append(('probability','단일 확률',chapter('probability','몇 % 확률로 한 번 생성하기',node('chance','Random Float in Range → 비교 → Branch','스폰 호출 전에 생성 여부를 판단합니다. 확률0/1은 별도 분기하면 경계값이 명확합니다.',pins([('Probability 변수','Float 0~1. 0.25는25%, 1은100%. 값을 Clamp(0,1)하거나 유효 범위를 확인합니다.'),('0 또는 1','Probability<=0: 생성하지 않음. >=1: 바로 생성.'),('0~1 사이','Random Float in Range(Min=0,Max=1) → Float < Float(Probability) → Branch True에서 Spawn.'),('평가 주기','스폰 시도당 한 번 뽑습니다. Tick마다25%를 반복 검사하면 짧은 시간에 거의 반드시 생성되므로 의도한 확률과 다릅니다.')])+'<p>Timer/웨이브 이벤트로 <strong>시도 횟수와 간격</strong>을 정한 뒤 확률을 평가합니다. 확률 통과와 실제 Spawn 성공은 별개입니다.</p>','언리얼 기본·조합'))))
    spawn.append(('weights','가중치',chapter('weights','여러 적 중 가중치로 한 종류 선택',
        '<p>콘텐츠 구조체 배열에 <code>Enemy Class</code>와 <code>Weight</code>를 저장합니다. 이 페이지는 기존 노드 조합법이며 별도의 프로젝트 가중치 추첨 노드가 구현됐다는 뜻은 아닙니다.</p><ol><li>유효 클래스이고 Weight&gt;0인 후보만 모읍니다.</li><li>가중치 합 TotalWeight를 계산합니다. 후보 없음/합0이면 생성하지 않습니다.</li><li>Random Float in Range(0,TotalWeight)를 한 번 평가하고 변수에 저장합니다.</li><li>For Each Loop with Break에서 누적 Weight를 더합니다.</li><li>RandomValue &lt; 누적값인 첫 후보를 선택하고 Break합니다. 난수의 상단 경계값에는 마지막 유효 후보를 선택하도록 처리합니다.</li><li>선택한 Class로 Spawn Actor from Class를 호출합니다.</li></ol>'+note('각 후보에 따로 확률 Branch를 걸면 한 번에 여러 종류가 생성될 수 있습니다. 한 종류만 고르는 가중치 추첨과 다릅니다. 반복 사용할 무작위 값은 변수에 저장하여 순수 노드가 연결마다 다시 평가되지 않게 합니다.')+'<p>가중치1·3은 상대 비율25%·75%입니다. 전역 생성 여부25%와 종류별 가중치를 함께 쓸 때는 두 단계로 분리합니다.</p>')))
    spawn.append(('lifecycle','생성 후 관리',chapter('lifecycle','AI 소유·사망·삭제·최대 수',
        '<p>ARBaseEnemy 기본은 ARAIController 및 배치/스폰 자동 빙의입니다. 자식 BP의 덮어쓰기 때문에 실제 Controller가 없는지 확인하고, 필요한 경우 엔진 <code>Spawn Default Controller</code>를 사용합니다. NavMesh 없이 클래스를 생성했다고 경로 이동까지 보장되지는 않습니다. NavMesh 준비는 맵 제작자가 담당하므로 없으면 맵 제작자에게 문의하세요.</p>'+pins([('On Character Death','적 자체 사망 처리는 직접 이벤트를 사용합니다. 외부 스포너가 추적하려면 생성한 적의 사망 디스패처에 자기 이벤트를 Bind할 수 있습니다.'),('삭제 수 집계','사망 뒤 연출 동안 Actor가 남을 수 있습니다. 살아 있는 수와 월드 Actor 수를 구분하고, 사망·Destroy 양쪽으로 중복 차감하지 않습니다.'),('소유한 Timer','스포너 EndPlay에서 자체 반복 타이머를 Clear. 스포너 제거 시 생성된 적까지 제거할지는 콘텐츠 정책입니다.')])+'<p><a href="../hhc/ENEMY_MOVEMENT_GUIDE_KO.html#navmesh-build">NavMesh 담당·문의</a> · <a href="../hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html#character-death">사망 이벤트</a></p>')))
    save('environment/ENEMY_SPAWN_GUIDE_KO.html',page('environment/ENEMY_SPAWN_GUIDE_KO.html','environment','적 스폰·확률 참고서','스포너에서 클래스를 선택하고, 생성 성공과 생존 수를 관리하는 노드 조합입니다.',spawn,[('Source/Action_RogueLike/Private/Foundation/Characters/ARBaseEnemy.cpp','적 기본 컨트롤러'),('Source/Action_RogueLike/Public/Foundation/Characters/ARBaseCharacter.h','사망 이벤트')]))

    search=[]
    search.append(('catalog','검색 범위',chapter('catalog','아이템 DA 카탈로그를 검색한다',
        '<p>검색 결과는 <strong>ARItemDefinition 데이터 에셋 참조</strong>입니다. DA 파일명 문자열·Item Id만 반환하는 것이 아니고, 플레이어의 소유 아이템 런타임이나 월드의 픽업 Actor를 검색하는 노드도 아닙니다.</p>'+pins([('스캔 경로','현재 Asset Manager의 ARItem 범위는 /Game/Game/Foundation/Data/Items 및 /Game/Game/Objects/Items입니다. 범위 밖 DA는 설정 없이 자동 검색되지 않습니다.'),('Item Type Tag','정확한 타입 필터. 부모 태그를 넣으면 모든 유물 하위 타입을 자동 포함하는 계층 검색이 아닙니다.'),('Additional Tags','DA의 문자열 목록. 스킬 입력 Gameplay Tag와 다릅니다. 앞뒤 공백 제거·대소문자 무시로 일치합니다.'),('호출 빈도','카탈로그 조회는 에셋을 로드할 수 있으므로 Tick에서 반복 검색하지 말고 필요할 때 조회하여 후보 배열을 캐시합니다.')]),True)))
    search.append(('find','검색 노드',chapter('find','단일 키 또는 추가 태그 검색',node('find-key','Find Item by Key','타입과 ID가 일치하는 DA 한 개를 찾습니다.',pins([('Item Type Tag','정확한 아이템 타입 Gameplay Tag.'),('Item Id','양의 정수. ID는 타입과 함께 식별 키를 이룹니다.'),('Return Value','DA 참조. 결과가 없거나 키가 중복되면 비어 있으며 임의 하나를 고르지 않습니다.')]))+node('find-additional','Find Items by Additional Tag','추가 태그가 일치하는 DA 여러 개를 배열로 찾습니다.',pins([('Additional Tag','검색할 문자열 라벨. 빈 문자열로 전체 검색하지 않습니다.'),('Type Filter','정확한 타입만 포함. 비우면 타입 제한 없음.'),('Return Value','ARItemDefinition 참조 배열. 길이0도 정상 결과이며 순서가 고정된 보상 순위라고 가정하지 않습니다.')])+note('현재 이 노드는 추가 태그가 필수입니다. Type Filter만 넣고 Additional Tag를 비워 모든 유물을 조회하는 API가 아닙니다. 전체 유물 후보가 필요하면 공통 추가 라벨을 DA에 준비하거나 별도 목록을 사용하세요.'))+node('validate-catalog','Validate Item Catalog','누락 필드·런타임 종류·타입+ID 중복 등을 개발 중 확인합니다.',pins([('Return Value / Errors','검사 성공 Bool / 오류 문자열 배열. 에셋을 자동 수정하지 않습니다.')])))))
    search.append(('exclude-owned','보유 제외',chapter('exclude-owned','플레이어 소유 유물을 후보에서 제외',node('owned-inventory','Get Loadout Inventory','플레이어의 Loadout Component에서 현재 보유 스냅샷 배열을 얻습니다. 스냅샷의 Definition을 카탈로그 결과와 비교합니다.',pins([('Target','플레이어 Get Loadout Component 반환 참조.'),('Return Value','ARLoadoutItemSnapshot 배열. Break / Split으로 Definition 참조를 꺼냅니다.')])+'<ol><li>검색 결과를 CandidateDefinitions 배열 변수에 저장합니다.</li><li>Get Loadout Inventory → For Each Loop.</li><li>Array Element의 Definition을 꺼내 CandidateDefinitions의 <code>Remove Item</code>에 연결합니다.</li><li>Loop Completed에서 남은 후보로 추첨합니다.</li></ol>'+note('배열을 순회하며 같은 배열의 원소를 지우는 방식 대신 <strong>보유 배열을 순회하고 후보 배열을 수정</strong>합니다. DA 참조 비교는 같은 정의의 모든 보유 복사본을 후보에서 제외하는 정책입니다. 소모품은 별도 Consumable Slots를 조회해야 합니다.')))))
    search.append(('random-n','N개 추첨',chapter('random-n','중복 없이 N개 선택',
        '<ol><li>후보 배열의 중복 DA를 먼저 제거합니다. 조합한 목록이라면 빈 Selected 배열에 <code>Add Unique</code>로 모은 뒤 후보로 사용합니다.</li><li>배열 <code>Shuffle</code>을 호출합니다.</li><li>Count = Clamp(N, 0, Length(CandidateDefinitions))를 계산합니다.</li><li>Count&gt;0일 때만 For Loop(First Index=0, Last Index=Count−1)로 앞쪽 Count개를 Get하여 결과 배열에 Add합니다.</li><li>빈 후보면 선택 결과도 빈 배열이며 별도 대체 보상을 결정합니다.</li></ol>'+note('랜덤 인덱스를 N번 단순 반복하면 같은 DA가 여러 번 나올 수 있습니다. Shuffle 후 앞부분을 선택하면 중복 없는 후보 배열에서 재추첨 중복이 없습니다.')+'<p>화면 선택지로 보여줄지, 바로 픽업으로 생성할지는 다음 단계의 콘텐츠 정책입니다. <a href="ITEM_PICKUP_SPAWN_GUIDE_KO.html">선택 DA로 픽업 소환</a></p>')))
    search.append(('count','개수 조회',chapter('count','개수만 필요한 경우',node('count-owned','Count Owned Items by Additional Tag','추가 라벨과 타입이 일치하는 소유 아이템 개수를 셉니다. 어떤 DA를 보유했는지 나열하는 반환 배열은 아닙니다.',pins([('Player','검사할 플레이어 Actor.'),('Additional Tag / Type Filter','추가 문자열 라벨 / 선택적인 정확한 타입 필터.'),('Return Value','보유 복사본의 개수. 같은 정의의 두 개는2로 셀 수 있습니다. 소모품 슬롯은 점유된 슬롯 기준으로 포함합니다.')])))))
    save('environment/ITEM_SEARCH_GUIDE_KO.html',page('environment/ITEM_SEARCH_GUIDE_KO.html','environment','아이템 DA 검색·필터·추첨','검색 → 보유 유물 제외 → 중복 없이 N개 선택까지, 현재 노드로 연결할 수 있는 흐름을 정리합니다.',search,[('Source/Action_RogueLike/Public/Foundation/Blueprint/ARItemCatalogBlueprintLibrary.h','검색 노드'),('Source/Action_RogueLike/Private/Foundation/Blueprint/ARItemCatalogBlueprintLibrary.cpp','검색 정책'),('Config/DefaultGame.ini','카탈로그 스캔 경로')]))

    pickup=[]
    pickup.append(('flow','연결 흐름',chapter('flow','검색 DA → 픽업 Actor',
        '<div class="flow">아이템 검색·선택 → Definition 참조<br>Get World Subsystem(ARWorldItemDropSubsystem) → Try Spawn Loadout Pickup<br>Return Value → Branch → 성공 Spawned Pickup 참조 사용</div><p>아이템 정의(DA), 월드 외형(픽업 Actor), 획득 후 동작(아이템 런타임)은 서로 다른 객체입니다. DA 자체를 Spawn Actor의 Class에 넣지 않습니다.</p>',True)))
    pickup.append(('subsystem','관리자 얻기',chapter('subsystem','Get World Subsystem',node('get-subsystem','Get World Subsystem','현재 월드의 픽업 생성 관리자를 얻습니다. Actor/레벨 BP에서 Subsystem Class를 선택합니다.',pins([('Class / Subsystem Class','ARWorldItemDropSubsystem 선택. 엔진 버전/에디터 표시에 따라 선택 핀 이름이 다를 수 있습니다.'),('Return Value','ARWorldItemDropSubsystem 참조. 이 참조에서 핀을 끌어 Try Spawn Loadout Pickup 검색.'),('월드 없는 객체','임의 UObject에는 월드 문맥이 없을 수 있습니다. 월드 Actor에서 생성하거나 유효 월드 문맥을 가진 호출 경로를 사용합니다.')]),'언리얼 기본'))))
    pickup.append(('spawn-pickup','생성 노드',chapter('spawn-pickup','Try Spawn Loadout Pickup',node('try-spawn','Try Spawn Loadout Pickup','무기·액티브 유물·패시브 유물용 픽업을 생성하고 선택한 DA를 생성 완료 전에 전달합니다.',pins([('Target','현재 월드 ARWorldItemDropSubsystem.'),('Definition','검색·추첨 결과 ARItemDefinition 참조.'),('Desired Location','원하는 월드 위치 Vector. 최종 위치는 충돌 조정으로 달라질 수 있습니다.'),('Pickup Class','ARLoadoutItemPickup 자식 BP 클래스. 비우면 기본 클래스 사용. 외형이 필요한 경우 Mesh 등을 준비한 자식 BP를 지정합니다.'),('Drop Owner','선택적인 엔진 Owner/Instigator용 Actor. 플레이어에게 자동 지급하는 핀이 아닙니다.'),('Return Value','생성 성공 Bool. 충돌 등의 이유로 실패할 수 있습니다.'),('Spawned Pickup','성공한 월드 픽업 Actor 참조. 실패하면 비어 있습니다.')])+note('내부 충돌 정책은 Adjust If Possible But Don’t Spawn If Colliding입니다. Bool 성공에서만 관리 목록·드롭 수를 갱신합니다. 이 노드에 생성 회전/스케일 선택 핀이 없으므로 자식 BP 기본값이나 성공 Actor를 별도로 설정합니다.')))))
    pickup.append(('appearance','외형 선택',chapter('appearance','같은 DA에 픽업 외형이 여러 개일 때',
        '<p>DA와 Pickup Class는 별도 입력이므로 <strong>같은 Definition을 다른 픽업 BP로 생성할 수 있습니다.</strong> DA 검색 결과만으로 어떤 픽업 클래스를 쓸지 자동 결정하지는 않습니다.</p><ol><li>상자형/빛나는 구체형 등 ARLoadoutItemPickup 자식 BP들을 준비합니다.</li><li>콘텐츠 배열·맵·설정에서 원하는 Pickup Class를 선택합니다.</li><li>같은 Definition + 선택 Class를 Try Spawn Loadout Pickup에 전달합니다.</li></ol>'+node('definition-assigned','On Pickup Definition Assigned','픽업에 DA가 지정될 때 실행되는 자식 BP 이벤트입니다. Item Definition을 읽어 표시·외형을 설정할 수 있습니다.',pins([('배치 위치','ARLoadoutItemPickup 자식 BP의 직접 이벤트.'),('입력','별도 Definition 출력 핀 없음. 자기 Item Definition 속성을 읽습니다.'),('호출 시점','Try Spawn Loadout Pickup은 생성 완료/BeginPlay 전에 정의를 지정합니다. BeginPlay에서 캐시한 값이 이미 준비됐다고 가정하지 않습니다.')]),'픽업 직접 이벤트')+note('기존 클래스 디폴트의 DA와 검색으로 선택한 DA를 혼동하지 마세요. 생성 관리자는 선택 Definition으로 덮어 지정합니다. ARItemPickup과 ARLoadoutItemPickup은 이름이 비슷하지만 이 소환 노드의 Class 계열은 후자입니다.'))))
    pickup.append(('other-types','획득·소모품',chapter('other-types','획득과 소모품 경로 구분',node('spawn-consumable','Try Spawn Consumable Pickup','소모품 DA를 월드 픽업으로 생성할 때 사용합니다. 소모품 슬롯 획득 경로는 Loadout 유물과 달라 별도 노드가 필요합니다.',pins([('Target / Definition / Desired Location','같은 월드 관리자 / 소모품 DA / 월드 위치.'),('Pickup Class','ARConsumablePickup 자식 클래스.'),('Drop Owner / Return Value / Spawned Pickup','엔진 소유자 / 생성 성공 Bool / 생성 Actor 참조.')]))+node('direct-acquire','Try Acquire Item','픽업을 만들지 않고 지정 플레이어에게 DA 획득을 요청하려는 별도 상황에 사용합니다. 월드에 보이는 픽업 생성 노드를 대체하는 목적은 아닙니다.',pins([('Player / Definition','받을 플레이어 / 아이템 DA.'),('Return Value','Request Status 구조체. Success 확인 후 보상 지급 완료로 처리합니다. 슬롯·교체 선택 등의 이유로 실패할 수 있습니다.')]))+'<p>기본 픽업 획득은 플레이어 Interaction Component의 요청을 통해 처리합니다. 성공해야 픽업이 사라지며, 사용자 정의 추가 스탯 지급은 획득 성공과 중복 방지를 함께 설계합니다.</p>')))
    save('environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html',page('environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html','environment','아이템·유물 픽업 소환','선택된 DA로 월드 픽업을 만들고 외형 클래스를 지정하는 연결 참고서입니다.',pickup,[('Source/Action_RogueLike/Public/Foundation/Interaction/ARWorldItemDropSubsystem.h','픽업 소환 핀'),('Source/Action_RogueLike/Private/Foundation/Interaction/ARWorldItemDropSubsystem.cpp','생성·충돌 정책'),('Source/Action_RogueLike/Public/Foundation/Interaction/ARItemPickupActors.h','픽업 클래스·이벤트')]))

# Preserve the existing anchor so bookmarks and cross-guide links keep working.
NAV_REQUIREMENT = node('navmesh-build','NavMesh · 맵 제작자 담당','NavMesh는 AI가 이동 가능한 영역과 경로를 찾는 데 필요한 맵 데이터입니다. 생성·관리는 맵 제작자가 담당합니다.',
    note('NavMesh가 없으면 AR AI Move To Actor / AR AI Move To Location을 사용하는 적은 경로 이동을 할 수 없습니다. NavMesh가 없거나 경로가 연결되지 않아 움직이지 않는다면 맵 제작자에게 문의하세요.')+
    '<p>적 제작자가 이 가이드를 따라 NavMesh를 직접 만들 필요는 없습니다. Request Basic Move 같은 직선 이동 입력은 NavMesh 경로 탐색과 별개입니다.</p>', '맵 의존성')

def build_movement_checklist():
    s=[]
    s.append(('choose','노드 선택',chapter('choose','장애물 우회가 필요한가?',pins([
        ('경로 추적','AR AI Move To Actor / AR AI Move To Location. Target은 ARAIController. NavMesh와 유효 목표 필요.'),('직선 입력','Request Basic Move. Target은 Movement Control Component. 방향을 반복 전달하며 장애물 우회 경로를 계산하지 않습니다.'),('공격 돌진','Request Action Move / Request Action Velocity. 활성 Action Handle이 필요하며 일반 추적을 대신하는 경로 탐색이 아닙니다.')])+note('엔진 Add Movement Input이나 임의 Set Actor Location으로 공통 이동 제한을 우회할 수 있습니다. 공통 CC 정책을 유지하려면 해당 목적의 AR 노드를 사용하세요.'),True)))
    s.append(('connection','정확한 연결',chapter('connection','적 Actor ≠ Controller ≠ Component',
        '<div class="flow">경로: 적 Get Controller → Cast To ARAIController → As ARAIController → AR AI Move To Actor.Target<br>목표: Get Player Pawn 또는 저장 TargetActor → Goal<br>직선: 적 Get Movement Control Component → Request Basic Move.Target</div>'+pins([('Self','적 BP에서 자기 적 Actor. 컨트롤러 자체가 아닙니다.'),('Cast Failed','실제 Controller 없음/종류 다름. Auto Possess AI·AI Controller Class·Get Controller를 확인합니다.'),('목표 좌표','Actor 목표는 Goal 참조, 좌표 목표는 Destination Vector. 둘을 혼동하지 않습니다.')]))))
    s.append(('blocked','CC·재개',chapter('blocked','노드 내부가 검사하는 것과 제작자 책임',
        '<p>AR 경로/일반 이동 요청은 공통 Can Basic Move를 검사하여 사망·기절·경직·이동 잠금 중인 요청을 거절합니다. ARAIController는 이동 잠금 발생 시 진행 중 경로도 정지시킵니다.</p>'+note('CC가 풀려도 정지시킨 경로가 자동 재개된다고 가정하지 않습니다. AI 판단 루프가 이동 가능·목표 유효성을 다시 확인하고 필요하면 새 요청을 발행해야 합니다.')+pins([('공격 조건','이동 노드가 적의 사거리·쿨타임·타깃 선정까지 판단하지 않습니다.'),('도착 판정','Request Successful은 요청 수락이지 이동 완료가 아닙니다. 공격 전 현재 거리/높이를 다시 확인합니다.'),('정지','AI 경로를 취소하려면 Controller의 Stop Movement. 순간 속도 정지와 경로 요청 정지는 역할이 다릅니다.'),('잠금 해제','자신이 발급받은 이동 잠금만 해제합니다. 다른 상태·액션의 잠금을 전체 Clear하지 않습니다.')]))))
    s.append(('diagnose','안 움직임',chapter('diagnose','안 움직일 때 순서대로 확인',
        '<ol><li>실행선이 실제로 노드까지 오는가? 시작 Tick/Timer/이벤트가 동작하는가?</li><li>적 Get Controller가 유효하고 ARAIController Cast가 성공하는가?</li><li>Goal이 유효한 실제 Pawn/Actor인가? 플레이어 생성 전에 캐시한 빈 참조는 아닌가?</li><li>MoveSpeed&gt;0이고 현재 Can Basic Move가 true인가?</li><li>맵 제작자가 NavMesh를 준비했는가? NavMesh가 없거나 시작/목표까지 경로가 연결되지 않으면 맵 제작자에게 문의하세요.</li><li>Capsule·바닥 충돌·이동 평면 제한·물리 시뮬레이션이 충돌하지 않는가?</li><li>경로 반환 Failed / Already At Goal / Request Successful 중 무엇인가?</li><li>공격 종료의 End Action 또는 자체 이동 잠금 해제가 누락됐는가?</li></ol><p><a href="../hhc/ENEMY_MOVEMENT_GUIDE_KO.html#navmesh-build">NavMesh 담당·문의</a> · <a href="ACTION_LIFECYCLE_GUIDE_KO.html">액션 수명 점검</a></p>')))
    s.append(('warnings','개발용 경고',chapter('warnings','출력 로그에서 이동 경고 확인',
        '<p>개발용 실행에서 AR AI Move To Actor / Location 요청이 실패하면 <strong>출력 로그(Output Log)</strong>의 <code>LogARFoundation</code>에 <code>[AR 이동:원인]</code>, 객체 이름, Controller 이름, 확인할 설정을 표시합니다. 팝업이나 화면 경고가 아니며 이동 결과와 경로를 바꾸지 않습니다.</p>'+pins([
        ('NoPawn / NoMovementControl','제어 중인 Pawn 또는 Movement Control Component가 없습니다. 빙의·Auto Possess AI·클래스와 컴포넌트를 확인합니다.'),('InvalidGoal / InvalidDestination','Goal Actor가 없거나 제거됨 / 좌표에 유효하지 않은 수치가 있음.'),('NoNavMesh','해당 Agent가 사용할 Navigation Data가 없습니다. NavMesh 생성·관리는 맵 제작자가 담당하므로 맵 제작자에게 문의하세요.'),('StartOffNavMesh / GoalOffNavMesh','현재 위치 또는 목표 위치를 NavMesh에 투영하지 못함. 배치 위치를 확인하고 맵의 경로 영역은 맵 제작자에게 문의하세요. 특정 메시가 원인이라고 확정하는 경고는 아닙니다.'),('MoveFailed','정확한 원인을 확정하지 못한 일반 실패. 경로 연결·필터·이동 컴포넌트 상태 확인.')])+note('같은 Controller의 같은 원인은 최대 5초마다 한 번 안내합니다. 추가 NavMesh 진단도 5초 간격으로 제한합니다. CC·사망·액션 이동 잠금에 의한 정상 거절은 경고하지 않으며 Already At Goal·Request Successful도 경고하지 않습니다. Shipping 빌드에서는 이 진단을 실행하지 않습니다.')+'<p>이 경고는 C++ 이동 요청까지 도달한 경우만 검사합니다. 실행선 미연결, 실패한 Cast, 빈 Target 때문에 노드가 호출되지 않은 상황이나 이동 수락 후의 장기 정체까지 자동 발견하는 기능은 아닙니다.</p>')))
    s.append(('repeat','반복 요청',chapter('repeat','반복 요청·난수·성능 주의',
        '<p>매 Tick마다 새로운 AI 경로를 강제로 재발행하면 불필요한 경로 계산·중단·재시작이 생길 수 있습니다. 타깃 변경·일정 주기·기존 이동 완료/중단 등 재요청 기준을 정하세요. 단, Request Basic Move는 방향 입력 방식이므로 지속 이동에는 반복 입력이 필요합니다.</p>'+note('두 이동 방식을 동시에 계속 호출하거나 공격 중 추적 루프가 경로를 재시작하지 않게 합니다. 공격 거리와 Acceptance Radius는 Capsule 크기 때문에 같지 않을 수 있습니다.'))))
    save('precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html',page('precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html','precautions','적 이동 주의점·진단','이동 상세 문서와 별도로, 제작 중 자주 생기는 연결·CC·경로 오류를 빠르게 점검합니다.',s,[('Source/Action_RogueLike/Private/Foundation/AI/ARAIController.cpp','경로 요청·CC 정지'),('Source/Action_RogueLike/Private/Foundation/Components/ARMovementControlComponent.cpp','일반/액션 이동 검사')]))

class DetailBlocks(HTMLParser):
    """Locate complete nested details blocks without regex-matching nested closes."""
    def __init__(self, text):
        super().__init__(convert_charrefs=False)
        self.text=text
        self.offsets=[0]
        for m in re.finditer('\n',text): self.offsets.append(m.end())
        self.stack=[]
        self.blocks={}
        self.feed(text)
    def absolute_offset(self):
        line,col=self.getpos()
        return self.offsets[line-1]+col
    def handle_starttag(self, tag, attrs):
        if tag=='details': self.stack.append((self.absolute_offset(),dict(attrs)))
    def handle_endtag(self, tag):
        if tag=='details':
            start,attrs=self.stack.pop()
            if attrs.get('id'):
                self.blocks[attrs['id']]=(start,self.absolute_offset()+len('</details>'),attrs)

def block(text, key):
    start,end,_=DetailBlocks(text).blocks[key]
    return text[start:end]

def replace_block(text, key, replacement):
    start,end,_=DetailBlocks(text).blocks[key]
    return text[:start]+replacement+text[end:]

def rewrite_links(text, old_path, new_path):
    def repl(match):
        quote,href=match.group(1),match.group(2)
        url=urlsplit(href)
        if url.scheme or url.netloc: return match.group(0)
        target=(old_path.parent/unquote(url.path)).resolve() if url.path else old_path.resolve()
        target=MOVE_MAP.get(target,target)
        if url.fragment:
            target=ANCHOR_MAP.get((target,unquote(url.fragment)),target)
        newhref=rel(new_path,target) if target!=new_path.resolve() else ''
        if url.query: newhref+='?'+url.query
        if url.fragment: newhref+='#'+url.fragment
        return f'href={quote}{newhref}{quote}'
    return re.sub(r'href=([\"\x27])([^\"\x27]*)\1',repl,text)

def extract_enemy():
    enemy=GUIDES/'hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html'
    movement=GUIDES/'hhc/ENEMY_MOVEMENT_GUIDE_KO.html'
    action=GUIDES/'hhc/ENEMY_ACTION_GUIDE_KO.html'
    original=enemy.read_text(encoding='utf-8')
    # Existing extracted pages supply the original detailed blocks on later runs.
    movement_origin=movement if movement.exists() else enemy
    action_origin=action if action.exists() else enemy
    movement_source=movement_origin.read_text(encoding='utf-8')
    action_source=action_origin.read_text(encoding='utf-8')
    groups=[(movement,['movement','navigation'],movement_source),(action,['action','delay','damage','cancel'],action_source)]
    for target,keys,text in groups:
        for key in keys:
            for item_id in re.findall(r'\bid="([^\"]+)"',block(text,key)):
                ANCHOR_MAP[(enemy.resolve(),item_id)]=target
    # Removed DA-specific self-stun reference anchors continue at the simple CC guide.
    for key in ['self-stun','stun-definition']:
        ANCHOR_MAP[(enemy.resolve(),key)]=GUIDES/'common/CC_STAGGER_GUIDE_KO.html'
    # Detailed movement page with engine navigation setup and exact controller chain.
    msections=[('navmesh','NavMesh 담당·문의',chapter('navmesh','NavMesh · 맵 제작자에게 문의',NAV_REQUIREMENT,True)),
               ('controller','컨트롤러 연결',chapter('controller','정확한 노드 연결·검색 위치',
                '<ol><li>적 BP에서 <code>Get Controller</code> 노드를 놓습니다. Target은 적 Self입니다.</li><li>Return Value를 <code>Cast To ARAIController</code>의 Object에 연결합니다.</li><li>As ARAIController 출력에서 선을 끌어 <code>AR AI Move To Actor</code> 또는 <code>AR AI Move To Location</code>을 검색합니다.</li><li>Actor 추적이면 Goal에 저장한 TargetActor, 좌표 이동이면 Destination에 월드 Vector를 연결합니다.</li><li>흰 실행선은 판단 이벤트 → Cast → 이동 노드로 이어줍니다. Cast Failed도 확인합니다.</li></ol>'+note('노드 검색이 안 뜨면 빈 그래프의 검색보다 ARAIController 참조에서 선을 끌어 검색하세요. 실제 적의 AI Controller Class와 Auto Possess AI(Placed in World or Spawned) 설정도 확인합니다. 컨트롤러 캐시는 BeginPlay에 준비할 수 있지만 Spawn/빙의 시점에 없으면 이후 유효할 때 다시 얻습니다.')))]
    for key,title in [('movement','직선·정지·잠금'),('navigation','경로 요청')]:
        raw=block(movement_source,key)
        if key=='navigation':
            raw=replace_block(raw,'nav-setup',node('nav-setup','NavMesh·Controller 준비 요약','AR 경로 이동은 실제 ARAIController 소유와 이동 가능한 NavMesh 영역이 필요합니다. NavMesh는 맵 제작자가 담당하며, 없거나 경로가 연결되지 않으면 맵 제작자에게 문의하세요.','<p><a href="#navmesh-build">NavMesh 담당·문의</a> · <a href="#controller">정확한 노드 연결</a></p>','설정'))
        raw=rewrite_links(raw,movement_origin,movement)
        msections.append((key,title,raw))
    msections.append(('recovery','CC 이후 재요청',chapter('recovery','CC가 끝난 뒤 추적 재요청',
        '<p>ARAIController는 공통 이동 잠금이 생기면 진행 중 경로를 Stop합니다. 상태가 제거됐다고 중단한 경로가 자동 재개되지는 않습니다. AI 판단 루프에서 대상 유효성·Can Basic Move·공격 여부를 확인하고 다시 요청하세요.</p><p><a href="../precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html">이동 실패 점검표</a> · <a href="ENEMY_ACTION_GUIDE_KO.html">공격 중 이동 잠금</a></p>')))
    save('hhc/ENEMY_MOVEMENT_GUIDE_KO.html',page('hhc/ENEMY_MOVEMENT_GUIDE_KO.html','enemy','적 이동·NavMesh 참고서','장애물 우회 경로 추적과 일반 방향 입력을 구분하고, 컨트롤러·목표·정지·CC 이후 재요청을 연결합니다.',msections,[('Source/Action_RogueLike/Public/Foundation/AI/ARAIController.h','정확한 경로 노드명'),('Source/Action_RogueLike/Private/Foundation/AI/ARAIController.cpp','CC 이동 정지'),('Source/Action_RogueLike/Public/Foundation/Components/ARMovementControlComponent.h','일반/액션 이동')]))
    asections=[('managed-timers','권장 액션 타이머',managed_timers('enemy'))]
    for key,title in [('action','시작·종료'),('delay','선딜·후딜'),('damage','공격·판정'),('cancel','취소·효과')]:
        raw=block(action_source,key)
        if key=='cancel' and '취소 후 직접 정리할 것:' not in raw:
            event=block(raw,'action-events')
            first_end=event.index('</p>')+len('</p>')
            raw=replace_block(raw,'action-events',event[:first_end]+cleanup('enemy')+event[first_end:])
        # From Result is the recommended direct-damage path; no old request node duplication.
        asections.append((key,title,refresh_cleanup(rewrite_links(raw,action_origin,action),'enemy')))
    save('hhc/ENEMY_ACTION_GUIDE_KO.html',page('hhc/ENEMY_ACTION_GUIDE_KO.html','enemy','적 액션·공격 참고서','적이 직접 시작하는 행동의 선딜·공격·취소·종료입니다. 아이템 Execute Item Skill 경로와 비용·쿨타임 책임이 다릅니다.',asections,[(ACTION,'액션 API'),('Source/Action_RogueLike/Public/Foundation/Actions/ARActionTypes.h','Request·취소 규칙'),(COMBAT,'피해·경직')]))
    # Overview keeps the complete workflow and event/groggy/death content, not duplicate pin manuals.
    replacements={
        'movement':('일반 이동 · 상세 문서','ENEMY_MOVEMENT_GUIDE_KO.html#movement','직선 방향 입력, 순간 정지, 이동 잠금·조회. 장애물 우회와는 다른 목적입니다.'),
        'navigation':('경로 이동 · NavMesh','ENEMY_MOVEMENT_GUIDE_KO.html#navmesh-build','NavMesh는 맵 제작자가 준비합니다. 없거나 경로가 연결되지 않아 적이 움직이지 않으면 맵 제작자에게 문의하세요. Get Controller 연결, AR AI Move To Actor/Location의 핀과 재요청은 이동 참고서에서 확인합니다.'),
        'action':('액션 시작·종료','ENEMY_ACTION_GUIDE_KO.html#action','직접 Try Start Action에서 성공 핸들을 저장하고 모든 완료 경로를 End Action으로 끝냅니다.'),
        'delay':('공격 타이머·선딜·후딜','ENEMY_ACTION_GUIDE_KO.html#managed-timers','선딜·후딜·반복 공격은 Set Action Timer by Event를 우선 사용합니다. Event 콜백에서 판정하고 마지막에 End Action. 순차 대기 분기가 필요하면 Action Delay도 사용할 수 있습니다.'),
        'damage':('피해·원 판정·히트박스','ENEMY_ACTION_GUIDE_KO.html#damage','공격력 비례 피해, 결과→경직/그로기, 단일/다중 대상 판정과 액션 귀속 Actor.'),
        'cancel':('경직·기절 취소·자체 정리','ENEMY_ACTION_GUIDE_KO.html#action-events','액션 타이머는 실제 취소 때 자동 Clear됩니다. 취소 이벤트에서 자기 핸들을 확인하여 기존 일반 Timer·변수·연출을 정리합니다.'),
        'spawn':('스폰·확률','../environment/ENEMY_SPAWN_GUIDE_KO.html','배치·Spawn Actor·생성 실패, 확률/가중치 선택과 스포너 역할.')}
    for key,(title,href,description) in replacements.items():
        original=replace_block(original,key,chapter(key,title,f'<p>{description}</p><p>{link(href,"상세 노드·연결 방법 보기 →")}</p>'))
    if 'self-stun' in DetailBlocks(original).blocks:
        original=replace_block(original,'self-stun','')
    original=replace_block(original,'apply-crowd-control',node('apply-crowd-control','Apply Crowd Control · 그로기 소진 반응','소진 이벤트 자체는 기절을 만들지 않습니다. 간편 CC 노드로 자기 기절을 적용하고 성공 핸들을 저장합니다.',
        '<div class="flow">Event On Groggy Gauge Depleted → Apply Crowd Control<br>Target=Self / CC Type=Stun / Duration=원하는 초<br>Success → Return Value를 GroggyStunHandle에 저장</div>'+note('정확한 고정 3초를 의도하면 Affected By Tenacity를 해제합니다. CC 면역 등에 의해 실패할 수 있으므로 실패 시 게이지 복구/행동 정책도 정하세요.')+'<p>CC 전체 핀과 상태 갱신은 <a href="../common/CC_STAGGER_GUIDE_KO.html#apply-crowd-control">간편 CC 참고서</a>, 실제 종료 뒤 게이지 복구는 <a href="#stun-recovery">On Status Removed</a>를 보세요. 게이지를 즉시 최대값으로 복구할지, 기절 종료 때 복구할지는 콘텐츠 정책입니다.</p>'))
    # Place cleanup advice immediately beneath the overview event purpose too.
    if '취소 후 직접 정리할 것:' not in original:
        overview=block(original,'direct-events')
        pos=overview.index('</p>')+len('</p>')
        original=replace_block(original,'direct-events',overview[:pos]+cleanup('enemy')+overview[pos:])
    original=original.replace('2026-10-02 코드 기준','2026-10-03 코드 기준')
    save('hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html',refresh_cleanup(rewrite_links(original,enemy,enemy),'enemy'))
    # Aliases for removed DA-specific anchors resolve to the simple CC node, without a second tutorial.
    cc=GUIDES/'common/CC_STAGGER_GUIDE_KO.html'
    text=cc.read_text(encoding='utf-8')
    text=text.replace('<details class="node" id="apply-crowd-control">','<span id="self-stun"></span><span id="stun-definition"></span><details class="node" id="apply-crowd-control">')
    save('common/CC_STAGGER_GUIDE_KO.html',text)

def enhance_item():
    url='nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html'
    text=(GUIDES/url).read_text(encoding='utf-8')
    if '취소 후 직접 정리할 것:' not in block(text,'item-skill-cancelled'):
        event=block(text,'item-skill-cancelled')
        pos=event.index('</p>')+len('</p>')
        text=replace_block(text,'item-skill-cancelled',event[:pos]+cleanup('item')+event[pos:])
    text=text.replace('2026-10-02 코드 기준','2026-10-03 코드 기준')
    if 'SKILL_PRIORITY_GUIDE_KO.html#rule' not in text:
        text=text.replace('<h3>우선도와 거래 규칙</h3>','<h3>우선도와 거래 규칙</h3><p><a href="../precautions/SKILL_PRIORITY_GUIDE_KO.html#rule">같은 우선순위 묶음·누적 비용·중단 규칙 상세</a></p>')
    # Prefer the item-specific direct cancellation event; keep a short pointer for other purposes.
    common_block=re.search(r'<details class="node"><summary><span class="tag event">공용 알림</span> On Action Ended / On Action Cancelled</summary>',text)
    if common_block:
        pos=common_block.start()
        # Anonymous details has no indexed id; exact existing end is after three paragraphs.
        end=text.index('</div></details>',pos)+len('</div></details>')
        text=text[:pos]+node('shared-action-notification','직접 시작한 액션·정상 종료 알림이 필요한 경우','아이템 스킬 취소는 위 On Item Skill Cancelled가 가장 간편합니다. 직접 액션/정상 종료의 외부 알림은 별도 책임으로 확인합니다.','<p><a href="../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#cancel-events">액션 알림과 정리 책임 보기</a></p>','추가 참고')+text[end:]
    save(url,add_managed_timers(refresh_cleanup(text,'item')))
    runtime='nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html'
    text=(GUIDES/runtime).read_text(encoding='utf-8')
    text=text.replace('<h1>유물 런타임 블루프린트 노드 가이드</h1>','<h1>아이템 런타임 기본 참고서</h1>')
    text=text.replace('<title>유물 런타임 블루프린트 노드 가이드 · Action RogueLike</title>','<title>아이템 런타임 기본 참고서 · Action RogueLike</title>')
    if 'id="pickup-interaction"' not in text:
        addition=chapter('pickup-interaction','픽업 객체·상호작용 연결',
            node('pickup-class','픽업 BP의 기본 설정','월드에 놓는 픽업 Actor와 획득 뒤 실행되는 런타임 UObject는 별개입니다. ARLoadoutItemPickup 자식 BP에 외형을 추가하고 Item Definition에 아이템 DA를 지정합니다.',pins([('Item Definition','획득할 데이터 에셋. 런타임 클래스는 DA의 Runtime Behavior Class에서 지정합니다.'),('Interaction Volume','탐지용 상속 볼륨. 외형 Mesh를 대신 표시하지 않습니다.'),('외형 Mesh','Static Mesh와 Materials를 설정합니다. Actor/컴포넌트/레벨 인스턴스 스케일 덮어쓰기를 구분합니다. 물리 필요 없으면 Simulate Physics를 끕니다.')]))+
            node('try-interact','Try Interact','플레이어의 현재 상호작용 후보에게 획득을 요청합니다. 픽업 액터의 입력을 각각 활성화하는 방식이 아닙니다.',pins([('Target','플레이어 Get Interaction Component 반환 참조.'),('입력 연결','플레이어 측 상호작용 입력 이벤트 → Try Interact. 시스템의 기존 입력 연결이 있으면 BP에서 같은 요청을 중복 호출하지 않습니다.'),('Return Value','Request Status. Result=Success 여부 확인. 슬롯/교체 등으로 실패할 수 있습니다.'),('Get Current Candidate / Get Current Prompt','같은 Interaction Component의 현재 후보/표시 문구 조회. 후보 조회는 실제 획득 성공과 다릅니다.')])+ '<p><a href="../environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html">DA 검색 결과로 픽업 소환</a></p>'))
        text=text.replace('</main>',addition+'\n</main>')
        text=text.replace('<a href="#check">','<a href="#pickup-interaction">픽업·상호작용</a><a href="#check">',1)
    if 'STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html#spec' not in text:
        text=text.replace('<h3>먼저 블루프린트에서 펼치는 방법</h3>','<p><a href="../precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html#spec">일반·아이템·액션 노드의 전체 필드와 수명 비교</a></p><h3>먼저 블루프린트에서 펼치는 방법</h3>')
    save(runtime,add_managed_timers(text))

def portal_index():
    path=GUIDE_HOME
    bodies=[]
    intros={'common':'기본 조작과 모든 콘텐츠가 공유하는 스탯·피해·상태 규칙.','item':'데이터 에셋 → 런타임 기본 → 아이템 내부 스킬 순서로 제작합니다.','enemy':'전체 흐름은 유지하고 이동과 액션 핀 설명은 독립 참고서로 분리했습니다.','environment':'스포너·보상 후보 검색·월드 픽업 생성의 노드 조합.','precautions':'제작 중 실수하기 쉬운 규칙을 별도 상세 문서로 확인합니다.'}
    for key,name in CATEGORIES:
        cards=''.join(f'<a class="guide-card" href="{url}"><strong>{title}</strong><span>{desc}</span></a>' for cat,url,title,desc in PAGES if cat==key)
        bodies.append(f'<section class="category-section" id="{key}" aria-labelledby="heading-{key}"><h2 id="heading-{key}">{name} 가이드</h2><p class="muted">{intros[key]}</p><div class="guide-cards">{cards}</div></section>')
    return f'''<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>가이드 메인 페이지 · Action RogueLike</title><link rel="stylesheet" href="assets/guide.css"></head><body>
{navigation(path,'')}
<header><div class="wrap"><div class="eyebrow">Action RogueLike · Blueprint 제작 참고서</div><h1>가이드 메인 페이지</h1><p class="lead">구현할 분야를 선택하고, 필요한 노드와 핀만 펼쳐보세요.</p><p class="muted">{len(PAGES)}개 참고서 · 로컬 HTML · 인터넷/서버 없이 탐색 가능 · 2026-10-04</p></div></header>
<main class="wrap"><p class="guide-portal-hint">모든 문서 상단에서 다른 분야와 같은 분야의 문서로 바로 이동할 수 있습니다. 노드 설명은 접이식이며 문서 안 검색·전체 펼치기·인쇄를 지원합니다.</p>
<p class="note"><a href="common/BASIC_CONTROLS_GUIDE_KO.html#keys">기본 조작표 열기</a> · 같은 키의 중복 지정은 가능하지만 충돌에 주의하세요. 키 추가·변경·건의는 이정환에게 문의하세요.</p>
<div class="note good">처음 제작한다면 <a href="nsh/ITEM_ASSET_CREATION_GUIDE_KO.html">아이템 데이터 에셋</a> 또는 <a href="hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html">적 전체 가이드</a>에서 시작하세요. 진행 중 막히면 <a href="#precautions">주의점 참고서</a>로 이동하세요.</div>
{''.join(bodies)}
</main><footer><div class="wrap">가이드는 공통 시스템을 사용하는 콘텐츠 개발자용입니다. 중복 API의 상세 설명은 간편한 경로를 우선 안내하며, 실제 게임 노드·C++ API를 삭제한 것은 아닙니다.</div></footer></body></html>'''

def inject_portal(url, category):
    path=GUIDES/url
    text=path.read_text(encoding='utf-8')
    text=re.sub(r'\s*<!-- GUIDE PORTAL START -->.*?<!-- GUIDE PORTAL END -->\s*','\n',text,flags=re.S)
    css=rel(path,GUIDES/'assets/guide.css')
    if f'href="{css}"' not in text:
        text=text.replace('</head>',f'<link rel="stylesheet" href="{css}">\n</head>',1)
    # Marker makes mechanical navigation updates idempotent.
    insertion='<!-- GUIDE PORTAL START -->'+navigation(path,category)+'<!-- GUIDE PORTAL END -->'
    text=re.sub(r'(<body\b[^>]*>)',lambda m:m.group(1)+'\n'+insertion,text,count=1)
    # These older pages only had expand/collapse or custom search. Share one
    # search implementation while retaining dynamic stat filtering and damage pin UI.
    if url in ('nsh/ITEM_ASSET_CREATION_GUIDE_KO.html','nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html','hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html'):
        text=re.sub(r'<body\b[^>]*>','<body data-guide-ui="shared">',text,count=1)
        if 'id="node-search"' not in text:
            tools='<div class="guide-tools"><label for="node-search">노드·내용 찾기</label><input id="node-search" type="search" placeholder="노드 이름, 필드, 수명…" autocomplete="off"><button id="clear-search" type="button">검색 지우기</button></div><p id="search-result" class="search-result" role="status" aria-live="polite"></p>'
            text=re.sub(r'(<main\b[^>]*>)',lambda m:m.group(1)+tools,text,count=1)
        text=re.sub(r'<script\b[^>]*>.*?</script>','',text,flags=re.S)
        text=text.replace('</body>',f'<script src="{rel(path,GUIDES/"assets/guide.js")}"></script></body>')
    # Compact overview numbering after chapters have been split.
    index=iter(range(100))
    text=re.sub(r'(<span class="chapter-number">)\d+(</span>)',lambda m:m.group(1)+f'{next(index):02d}'+m.group(2),text)
    save(url,text)

def simplify_damage():
    path=GUIDES/'common/DAMAGE_NODES_FORMULA_GUIDE_KO.html'
    text=path.read_text(encoding='utf-8')
    text=text.replace('<span id="stagger-formula"></span>','')
    if 'apply-stagger-context' in DetailBlocks(text).blocks:
        replacement='<span id="apply-stagger-context"></span><span id="stagger-result"></span>'+node('stagger-template','Stagger Template · DOT 경직 수치','DOT의 Apply Stagger And Groggy Each Tick을 체크한 뒤 지정할 경직·그로기 수치 구조체입니다. 직접 피해에서는 위 From Result 노드의 개별 핀을 사용합니다.',pins([
            ('Base Stagger Damage','틱의 기본 경직력. 0이면 경직 시도 없음.'),('Stagger Multiplier','경직 피해 배율, 기본1. 그로기 피해 배율이 아닙니다.'),('Base Groggy Damage','틱의 기본 그로기 피해. 0이면 게이지 피해 없음. 대상이 게이지를 사용해야 실제 감소.'),('Source / Effect Name','출처 구조체와 효과 이름. 피해 이름 제한을 대신하지 않습니다.')])+note('DOT 등록 핸들을 Damage Result에 연결하지 않습니다. 실제 적용된 틱의 경직 요청은 시스템이 처리하므로 같은 틱을 별도 콜백에서도 다시 처리하지 마세요. 저수준 Hit Context 요청 조립 노드는 새 콘텐츠의 일반 경로에서 안내하지 않습니다.'),'DOT 구조체')
        text=replace_block(text,'apply-stagger-context',replacement)
        save('common/DAMAGE_NODES_FORMULA_GUIDE_KO.html',text)
    elif 'id="stagger-result"' not in text:
        text=text.replace('<span id="apply-stagger-context"></span>','<span id="apply-stagger-context"></span><span id="stagger-result"></span>')
        save('common/DAMAGE_NODES_FORMULA_GUIDE_KO.html',text)
    else:
        save('common/DAMAGE_NODES_FORMULA_GUIDE_KO.html',text)
    text=text.replace('<span id="stagger-result"></span>','')
    text=text.replace('<details class="node" id="apply-stagger">','<span id="stagger-result"></span><details class="node" id="apply-stagger">')
    save('common/DAMAGE_NODES_FORMULA_GUIDE_KO.html',text)

def update_timer_references():
    """Keep retained manuals and generated pages consistent with lifetime timers."""
    replacements={
        '타이머는 자동 정리 대상이 아닙니다. BP에서 보관한 Timer Handle은 이 이벤트에서 <code>Clear and Invalidate Timer by Handle</code>로 멈추세요.':
            '<code>Set Item Timer by Event</code>는 이 이벤트 전에 자동 Clear됩니다. 기존 일반 <code>Set Timer by Event</code>만 저장한 Timer Handle을 여기서 직접 Clear하세요. 해제 중에는 새 아이템 타이머를 만들 수 없습니다.',
        '반복 타이머 해제, 직접 묶은 이벤트 해제, 직접 만든 별도 액터 정리 등에 사용합니다.':
            '아이템 전용 타이머가 이미 정리된 뒤 자체 상태 초기화, 일반 타이머 해제, 직접 묶은 이벤트 해제, 독립 액터 정리 등에 사용합니다.',
        '타이머 시작, 이벤트 구독, 초기 변수 설정에 사용합니다.':
            '보유 중 효과의 Set Item Timer by Event 시작, 이벤트 구독, 초기 변수 설정에 사용합니다. 스킬 예약은 Execute Item Skill에서 액션 타이머로 시작합니다.',
        '시전이 끝나면 소유자의 Action Component에서 받은 핸들로 <code>End Action</code>을 호출하고, 취소 시 정리가 필요한 타이머·연출은 행동 종료/취소 이벤트에 연결합니다.':
            '스킬 예약은 <code>Set Action Timer by Event</code>를 사용하고, 시전이 끝나면 같은 Action Component·핸들로 <code>End Action</code>을 호출합니다. 액션 타이머는 종료/취소 때 자동 제거됩니다. 일반 타이머·자체 변수·연출 정리는 취소 이벤트에 연결합니다.',
        'Switch on Name으로 스킬을 구분하여 자체 타이머·상태·연출을 정리합니다.':
            'Switch on Name과 실행 핸들로 스킬을 구분하여 자체 상태·연출·기존 일반 타이머를 정리합니다. 액션 타이머는 이미 자동 제거된 상태이며, 보유 중 아이템 타이머는 스킬 취소만으로 제거되지 않습니다.',
        '<code>On Item Unregistered</code>에서 해당 Timer Handle을 해제했는지.':
            'Set Item Timer by Event를 썼는지, 정상 아이템 해제 경로인지 확인. 기존 일반 Timer라면 On Item Unregistered에서 직접 Clear했는지 확인.',
        '저장한 핸들 Get → Action Delay·Apply Action·End Action의 Handle':
            '저장한 핸들 Get → Set Action Timer by Event·Action Delay·Apply Action·End Action의 Handle',
        'Event On Item Skill Cancelled → Switch on Name(Skill Id) → 해당 스킬의 Timer Handle Clear → 자체 상태·연출 정리':
            'Event On Item Skill Cancelled → Switch on Name(Skill Id) + 실행 핸들 비교 → 자체 상태·연출 정리<br>Set Action Timer by Event는 이미 자동 Clear / 기존 일반 Timer만 수동 Clear',
        '런타임은 자체 타이머·구독·외부 작업·참조를 정리합니다.':
            'Set Item Timer by Event는 이 이벤트 전에 자동 Clear됩니다. 런타임은 기존 일반 타이머·구독·외부 작업·참조를 정리합니다. 해제 이벤트에서는 새 아이템 예약이 거절됩니다.',
        '자체 타이머를 썼다면 종료·취소·아이템 해제 시점의 책임을 정합니다. 런타임 해제는 임의의 BP 타이머·외부 Actor를 모두 자동 삭제하는 기능이 아닙니다.':
            '스킬은 Set Action Timer by Event, 보유 효과는 Set Item Timer by Event를 사용하세요. 귀속 수명이 끝날 때 예약은 자동 정리됩니다. 기존 일반 BP 타이머·외부 Actor·변수·연출은 별도 책임이며, 조기 중단할 때는 저장한 Timer Handle로 Clear할 수 있습니다.',
        '<tr><td>외부 타이머·이벤트 구독</td><td>아이템 관리 목록에 자동 등록되지 않음</td><td>타이머 Clear, 자신이 Bind한 연결 해제, 늦은 콜백 방어</td></tr>':
            '<tr><td>Set Action Timer by Event</td><td>해당 액션 End/Cancel/Owner EndPlay</td><td>콜백에서 정상 완료 End Action, 자체 변수·연출 정리</td></tr><tr><td>Set Item Timer by Event</td><td>아이템 해제/교체/정상 소유자 EndPlay</td><td>보유 중 효과용. 스킬 취소만으로 중단되지 않음</td></tr><tr><td>일반 엔진 Timer·이벤트 구독</td><td>액션/아이템 자동 귀속 없음</td><td>일반 Timer Clear, 자신이 Bind한 연결 해제, 늦은 콜백 방어</td></tr>',
        '외부 타이머·구독·독립 Actor의 수동 정리 책임':
            '전용 타이머의 귀속 수명·정상 해제 확인. 기존 일반 Timer·구독·독립 Actor는 수동 정리',
        'Action Delay가 자신의 대기를 정리해도 다른 타이머·연출은 자동으로 정리하지 않습니다.':
            'Action Delay의 정리는 자신의 대기에 한정됩니다. 액션/아이템 전용 타이머는 각 귀속 수명에 따라 별도로 자동 정리되며, 일반 타이머·연출은 직접 정리합니다.',
        'Action Delay.Completed가 아니라 즉시 출력 사용, Duration':
            'Timer 예약 직후 출력에서 공격하지 않았는지 / Action Delay는 Completed 사용 / 시간 핀 확인',
        '선딜/효과 → End Action':
            'Set Action Timer by Event → Event 콜백의 판정/효과 → End Action',
        '일반 Delay나 직접 만든 Timer는 Action Handle을 모릅니다. 경직으로 행동이 취소되어도 예약 콜백이 뒤늦게 공격을 실행하지 않도록 직접 방어해야 합니다.':
            '새 공격의 예약은 Set Action Timer by Event를 사용하세요. 실제 액션 취소 때 자동 제거됩니다. 기존 일반 Delay / Set Timer by Event는 Action Handle을 모르므로 수동 정리와 콜백 방어가 필요합니다.',
        '<dt>자체 Timer 사용 시</dt>': '<dt>기존 일반 Timer 사용 시</dt>',
        '<tr><td>직접 만든 Timer·외부 이벤트 구독</td><td>액션 목록에 자동 등록 안 됨</td><td>Clear / 자신이 Bind한 연결 해제</td></tr>':
            '<tr><td>Set Action Timer by Event</td><td>액션 종료/취소 시 자동 Clear</td><td>콜백 정상 완료에서 End Action / 변수·연출 별도 정리</td></tr><tr><td>일반 엔진 Timer·외부 이벤트 구독</td><td>액션 자동 귀속 없음</td><td>Clear / 자신이 Bind한 연결 해제</td></tr>',
        '<tr><td>선딜이 취소되는데 뒤늦게 피해</td><td>일반 Timer/Delay 콜백, 활성 핸들 재검사</td>':
            '<tr><td>선딜이 취소되는데 뒤늦게 피해</td><td>Set Action Timer by Event로 교체 / 기존 일반 Timer Clear / 실제 취소 규칙 확인</td>',
        '<tr><td>Set Timer by Event의 Timer Handle</td><td>자동 액션 귀속 없음. 제작자가 Clear</td></tr>':
            '<tr><td>Set Action Timer by Event</td><td>해당 액션 End/Cancel/Owner EndPlay 때 자동 Clear</td></tr><tr><td>Set Item Timer by Event</td><td>아이템 해제 때 자동 Clear. 스킬 취소만으로는 유지</td></tr><tr><td>일반 Set Timer by Event</td><td>자동 액션/아이템 귀속 없음. 제작자가 Clear</td></tr>',
    }
    for category,url,_,_ in PAGES:
        if url.endswith(('BLUEPRINT_NODE_SEARCH_KO.html','TIME_GROUP_GUIDE_KO.html')): continue
        text=(GUIDES/url).read_text(encoding='utf-8')
        text=text.replace('2026-10-03 코드 기준','2026-10-04 코드 기준')
        for old,new in replacements.items(): text=text.replace(old,new)
        # Keep alternative Action Delay examples, but mark them as alternatives.
        if url=='hhc/ENEMY_ACTION_GUIDE_KO.html':
            text=text.replace('01 선딜 · 후딜 · 액션에 묶인 대기','01 Action Delay · 순차 대기 옵션')
            text=text.replace('<div class="flow">Try Start Action Success → Set AttackHandle → Action Delay',
                '<div class="flow">순차 대기 옵션: Try Start Action Success → Set AttackHandle → Action Delay')
        if url in ('nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html','nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html','hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html'):
            choice=('적 공격 예약은 Set Action Timer by Event로, 상시 AI 판단 루프는 별도로 구성했는가?' if category=='enemy' else
                    '스킬에는 Set Action Timer by Event, 보유 중 효과에는 Set Item Timer by Event를 골랐는가?')
            text=upsert_reference(text,'timer-checklist','타이머 제작 점검',
                f'<ul><li>{choice}</li><li>Time&gt;0, Event 연결, Success 확인과 예약 실패 출구를 만들었는가?</li><li>일반 흰 실행 출력이 아니라 Event 콜백에서 후속 효과를 실행하는가?</li><li>정상 스킬 완료의 End Action과 취소 이벤트의 변수·연출 정리를 분리했는가?</li><li>중복 예약·실행 핸들 덮어쓰기·삭제된 대상 참조를 막았는가?</li><li>기존 일반 Timer는 해제/EndPlay에서 Clear하는가?</li></ul>')
        save(url,text)

    # Short cross-links where the timer is not the subject of the page.
    references={
        'nsh/ITEM_ASSET_CREATION_GUIDE_KO.html': ('runtime-timing','Runtime에서 시간 작업을 만들 때',
            '<p>DA의 쿨타임·기본 효과 Duration과 BP 예약은 서로 다릅니다. 보유 주기 효과는 런타임의 <code>On Item Registered → Set Item Timer by Event</code>, 스킬 선딜·반복 효과는 <code>Execute Item Skill → Set Action Timer by Event</code>로 연결하세요. DA에 시간을 적는 것만으로 BP 콜백이 생성되지는 않습니다.</p><p><a href="RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html#managed-timers">보유 타이머 핀·등록/해제</a> · <a href="SKILL_RUNTIME_ACTION_GUIDE_KO.html#managed-timers">스킬 타이머 핀·완료/취소</a></p>'),
        'common/OBJECT_STAT_GUIDE_KO.html': ('stat-timing','시간 만료와 반복 스탯 적용 구분',
            '<p>스탯 보정의 양수 Duration 만료는 시스템이 처리하므로 제거용 BP 타이머를 따로 만들 필요가 없습니다. 일정 주기로 새 효과를 적용할 때만 예약이 필요합니다. 보유 효과는 <code>Set Item Timer by Event</code>, 한 행동의 효과는 <code>Set Action Timer by Event</code>를 사용하세요. 예약이 정리된다고 일반 보정이나 Permanent Flat 변경까지 되돌아가지는 않습니다.</p><p><a href="../precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html#timer-stat-lifetime">보정과 예약의 수명 차이</a></p>'),
        'common/DAMAGE_NODES_FORMULA_GUIDE_KO.html': ('damage-timing','예약 공격·주기 회복·DOT 구분',
            '<p>스킬/적의 선딜·반복 직접 피해·반복 회복은 <code>Set Action Timer by Event</code>, 무기/유물 보유 중 주기 회복은 <code>Set Item Timer by Event</code>를 사용하세요. 실제 피해/회복은 Event 콜백에서 적용하며 마지막 스킬 단계 뒤 End Action을 호출합니다.</p><p>등록한 DOT는 이미 자체 기간·틱 간격을 처리합니다. 틱마다 DOT 등록 노드를 다시 부르는 타이머를 추가하지 마세요. 동일 Damage Name 제한도 피해 요청의 설정이며 별도 BP 타이머가 필요 없습니다. 타이머 제거는 이미 등록한 독립 DOT·실드·보정을 제거하지 않습니다.</p><p><a href="../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#managed-timers">타이머 핀·수명·콜백 연결</a> · <a href="../precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html">효과 수명과 제거</a></p>'),
        'precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html': ('timer-stat-lifetime','보정의 수명과 적용 타이머의 수명은 별개',
            '<p>유한 Duration의 스탯 보정은 시스템이 만료시킵니다. BP 타이머는 앞으로 적용할 효과를 예약할 때만 필요합니다. 보유 중 주기 효과는 Set Item Timer by Event, 스킬 중 효과는 Set Action Timer by Event를 사용하세요.</p><p>예를 들어 아이템 타이머의 콜백에서 일반 Apply Stat Modifier를 호출하면, 버릴 때 다음 호출은 멈추지만 이미 생긴 일반 보정은 남을 수 있습니다. 함께 회수할 효과는 Apply Item Stat Modifier를 사용합니다. 액션도 같은 원칙으로 Apply Action Stat Modifier를 사용합니다. Permanent Flat은 타이머 정리나 아이템 해제로 되돌아가지 않습니다.</p><p><a href="ACTION_LIFECYCLE_GUIDE_KO.html#managed-timers">예약 노드·입력 핀·중복 방지</a></p>'),
        'hhc/ENEMY_MOVEMENT_GUIDE_KO.html': ('ai-timer-lifetime','상시 AI 판단과 공격 예약을 분리',
            '<p>상시 추적 판단은 Tick 또는 일반 엔진 Timer로 구성하고, 반복 Timer Handle은 Event EndPlay에서 Clear하세요. 한 공격의 Action Handle에 전체 판단 루프를 묶으면 공격 완료/취소 때 추적 판단까지 멈춥니다.</p><p>선딜·후딜·반복 공격 예약만 <code>Set Action Timer by Event</code>로 구성하세요. CC 후에는 중단된 경로를 판단 루프가 다시 요청해야 합니다. 타이머 실행 여부와 이동 허용 여부는 다르므로 공통 이동 검사와 공격 상태를 유지합니다.</p><p><a href="ENEMY_ACTION_GUIDE_KO.html#managed-timers">적 공격용 타이머 연결</a> · <a href="../precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html#ai-timer-lifetime">판단 루프 주의점</a></p>'),
        'precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html': ('ai-timer-lifetime','AI 루프와 공격 타이머 점검',
            '<ul><li>공격 선딜/후딜은 Set Action Timer by Event에 성공한 이번 Action Handle을 연결했는가?</li><li>공격 정상 완료에서 End Action을 호출하고 취소 이벤트에서 자체 Bool·연출을 정리하는가?</li><li>상시 판단 루프를 공격 액션에 묶어 공격 취소 후 판단까지 멈추지 않았는가?</li><li>상시 일반 Timer는 EndPlay에서 Clear하고, CC 해제 후 유효 목표로 경로를 재요청하는가?</li><li>매 Tick에 새 Timer를 만들거나 불필요한 경로 재계산을 반복하지 않는가?</li></ul><p><a href="../hhc/ENEMY_ACTION_GUIDE_KO.html#managed-timers">권장 공격 타이머</a> · <a href="ACTION_LIFECYCLE_GUIDE_KO.html#timer-cleanup">일반 Timer 예외</a></p>'),
        'environment/ENEMY_SPAWN_GUIDE_KO.html': ('spawner-timing','스포너·웨이브 예약의 수명',
            '<p>맵의 독립 스포너 Actor는 일반 Set Timer by Event로 시도 주기를 만들고 Return Value를 저장해 EndPlay에서 Clear합니다. 스포너 Self는 아이템 런타임이 아니므로 Set Item Timer by Event를 사용할 수 없습니다. 전체 웨이브를 한 적의 공격 액션에 묶지도 않습니다.</p><p>실제로 한 액션의 일부인 소환 스킬만 Set Action Timer by Event를 사용합니다. 예약 취소는 이미 생성한 적을 자동 삭제하지 않으며, 생성 Actor의 사망/삭제 정책은 별도입니다.</p><p><a href="../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#timer-cleanup">일반 Timer의 정리 예외</a></p>'),
        'environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html': ('pickup-timing','픽업 Actor와 아이템 타이머 구분',
            '<p>Set Item Timer by Event는 획득 뒤 등록되는 무기/유물 런타임의 노드입니다. 월드 픽업 Actor Self에서는 사용할 수 없습니다. 픽업의 독립 연출·수명 예약은 엔진 Timer와 EndPlay 정리로 구현합니다. 스킬 중 드롭 예약은 실제 활성 Action Handle이 있을 때 Set Action Timer by Event를 사용합니다. 타이머 Clear만으로 이미 생성한 픽업이 삭제되지는 않습니다.</p><p><a href="../nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html#managed-timers">획득 후 런타임의 권장 타이머</a></p>'),
    }
    for url,(key,title,body) in references.items():
        text=(GUIDES/url).read_text(encoding='utf-8')
        text=upsert_reference(text,key,title,body)
        save(url,text)

def upsert_reference(text,key,title,body):
    section=chapter(key,title,body)
    if key in DetailBlocks(text).blocks: text=replace_block(text,key,section)
    else: text=text.replace('</main>',section+'\n</main>')
    if f'href="#{key}"' not in text:
        # The stat reference uses a different local navigation; it still gets a
        # visible entry beside its tools, without changing the stat filters.
        match=re.search(r'(<nav (?:class="local-toc"[^>]*|aria-label="목차")>\s*<div class="wrap">)',text)
        if match: text=text[:match.end()]+link('#'+key,title)+text[match.end():]
        else: text=text.replace('<main',f'<p class="wrap">{link("#"+key,title)}</p><main',1)
    return text

def main():
    build_controls()
    build_cc()
    build_priority()
    build_lifecycle()
    build_stat_lifecycle()
    build_environment()
    build_movement_checklist()
    extract_enemy()
    from generate_enemy_details_guide import generate as generate_enemy_details
    generate_enemy_details()
    enhance_item()
    simplify_damage()
    from update_time_group_guides import update
    update_timer_references()
    update()
    from generate_node_search_guide import generate
    generate()
    save(HOME_NAME,portal_index())
    # Rewrite moved common references and split enemy anchors across every canonical page.
    for category,url,_,_ in PAGES:
        path=GUIDES/url
        text=rewrite_links(path.read_text(encoding='utf-8'),path,path)
        text=text.replace('47개 스탯 목록에 포함되지 않음','스탯 목록에 포함되지 않음')
        save(url,text)
        inject_portal(url,category)
    print(f'Generated {HOME_NAME} + {len(PAGES)} categorized references; no root-level redirect pages.')

if __name__=='__main__': main()
