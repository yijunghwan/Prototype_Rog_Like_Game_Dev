"""Offline node reference, extracted from current Blueprint-exposed declarations.

No Unreal execution or game asset writes. Fail on unparsed exposed declarations.
The catalogue's Korean descriptions are reused, but names/pins/defaults come from
current source, not the old markdown snapshot. Run this or organize_development_guides.
"""
from pathlib import Path
from html import escape as e
from collections import Counter
import json
import re
import os
import generate_foundation_html_reference as reference

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Docs/Guides/common/BLUEPRINT_NODE_SEARCH_KO.html'
HEADERS = ROOT / 'Source/Action_RogueLike'
ENUMS = {}

def words(name):
    name = re.sub(r'^b(?=[A-Z])', '', name)
    name = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1 \2', name)
    return re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', name).replace('_', ' ')

def balanced(text, start, opening='(', closing=')'):
    """Return closing position, ignoring strings and C++ comments."""
    depth = 0
    token = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*|[\s\S]', re.S)
    for m in token.finditer(text, start):
        c = m.group()
        if c == opening: depth += 1
        elif c == closing:
            depth -= 1
            if depth == 0: return m.start()
    raise ValueError('unbalanced source at '+str(start))

def meta_value(meta, key):
    m = re.search(r'\b'+re.escape(key)+r'\s*=\s*"([^"]*)"', meta)
    return m.group(1) if m else ''

def annotations(text, macro):
    # Anchored to source lines so mentions in comments are not declarations.
    for m in re.finditer(r'^\s*'+macro+r'\s*\(', text, re.M):
        start = text.index('(', m.start())
        end = balanced(text, start)
        yield m.start(), end+1, text[start+1:end]

def containers(text):
    out = []
    for m in re.finditer(r'\b(class|struct)\s+(?:\w+_API\s+)?(\w+)\s*(?:\s*:\s*[^;{]+)?\s*\{', text):
        start = text.index('{', m.start())
        out.append((m.group(2), m.start(), balanced(text, start, '{', '}')))
    return out

def owner_at(groups, pos):
    matches = [g for g in groups if g[1] < pos < g[2]]
    if not matches: raise ValueError('no owner for declaration at '+str(pos))
    return min(matches, key=lambda g:g[2]-g[1])[0]

def parameter(raw, meta='', direction=None):
    is_ref = 'UPARAM(ref)' in raw or bool(re.search(r'UPARAM\s*\(\s*ref\s*\)', raw))
    raw = re.sub(r'UPARAM\s*\([^)]*\)\s*', '', raw).strip()
    decl, _, default = raw.partition('=')
    m = re.fullmatch(r'(.+?[\s*&])(\w+)\s*', decl.strip())
    if not m: raise ValueError('unparsed parameter '+raw)
    typ, name = m.groups()
    return {'name':name, 'label':words(name), 'type':typ.strip(),
            'default':default.strip() or meta_value(meta, 'CPP_Default_'+name),
            'direction':direction or ('출력' if '&' in typ and 'const' not in typ and not is_ref else '입력'),
            'advanced':False, 'automatic':False}

PIN = {
    'Event':'실행할 매개변수 없는 Custom Event / Create Event를 연결합니다. 예약 노드는 콜백 즉시 실행과 다릅니다.',
    'Time':'타이머 실행 간격(초). 양수만 허용하며 Looping=false면 한 번 실행 뒤 끝납니다.',
    'bLooping':'true면 반복, false면 한 번 실행합니다. 타이머 완료가 액션 자체를 종료하지는 않습니다.',
    'InitialStartDelay':'첫 실행 대기(초). 기본 -1은 Time 사용, 0은 다음 타이머 처리 때, 양수는 지정 시간. -1 외 음수는 실패.',
    'bMaxOncePerFrame':'기본 true. 지연된 반복 콜백을 같은 프레임에 여러 번 실행하지 않습니다.',
    'Target':'효과를 받거나 조회할 대상 Actor. 함수의 자동 Target(노드를 소유한 컴포넌트)과 별개의 명시적 대상일 수 있습니다.',
    'Attacker':'피해를 가하는 공격원 Actor. 진영·공격 스탯·공격 출처 판정에 사용합니다.',
    'DamageCauser':'직접 피해를 발생시킨 Actor(투사체·판정체 등). 공격 스탯을 읽을 Attacker와 구분합니다.',
    'WorldContextObject':'현재 월드를 찾는 문맥 객체. 보통 Blueprint의 Self에서 자동으로 채워져 표시되지 않습니다.',
    'ActionComponent':'액션을 관리하는 대상의 AR Action Component 참조.',
    'ActionHandle':'현재 실행의 액션 식별 핸들. 시작 노드 또는 Execute Item Skill에서 받은 원본을 사용합니다.',
    'Handle':'적용·등록 시 발급받은 식별 핸들. 이름이나 배열 인덱스가 아니며 같은 종류의 적용 결과를 전달해야 합니다.',
    'Reason':'종료·취소·제거의 사유 열거형. 해당 함수의 타입에 맞는 사유를 선택합니다.',
    'FailureReason':'요청이 거절되거나 무효였을 때의 결과 코드. 성공 여부와 함께 확인합니다.',
    'FailureTag':'콘텐츠 사전 조건 실패 이유를 돌려줄 태그. 입력 키를 등록하는 핀이 아닙니다.',
    'Status':'요청의 성공 여부·실패 이유를 담는 결과 구조체.',
    'bSuccess':'적용·등록 요청이 성공했는지 나타냅니다. 실패했을 때 반환 핸들을 유효하다고 가정하지 마세요.',
    'SkillId':'아이템 내부에서 스킬을 구별하는 이름. 키 이름이나 장착 슬롯 번호와 다릅니다.',
    'SkillHandle':'장비 시스템에 등록된 특정 스킬의 식별 핸들.',
    'ItemInstanceId':'실제 보유 아이템 인스턴스의 고유 ID. 같은 DA를 가진 다른 아이템과 구분합니다.',
    'InputTag':'이번 입력 요청에 해당하는 태그. 등록된 스킬 후보를 찾는 데 사용합니다.',
    'ActiveRelicSlot':'장착된 액티브 유물 슬롯 번호. 유물의 고정 ID가 아닙니다.',
    'SkillIndex':'해당 아이템 내 스킬의 로컬 인덱스. 입력 슬롯과 구분합니다.',
    'Duration':'지속·대기 시간(초). 0/음수의 의미는 해당 노드마다 다르므로 아래 기본값·상세 안내를 확인하세요.',
    'TickInterval':'DOT 틱 사이의 시간(초). 지속시간과 별개입니다.',
    'AppliedDuration':'이번 요청에 강인함 등을 반영한 지속시간(초). 이미 유지 중인 효과의 남은 시간이 아닙니다.',
    'CCType':'Stun(기절) / Root(속박) 중 적용할 CC를 선택합니다.',
    'bAffectedByTenacity':'강인함에 따라 유한 효과 지속시간을 줄일지 선택합니다. CC 면역을 무시하는 옵션이 아닙니다.',
    'Source':'Category·Source Id·Display Name으로 이루어진 출처 정보. 제거 조건은 분류와 ID를 함께 맞춥니다.',
    'Category':'효과 출처 분류. 제거·검색에서는 Source Id와 함께 범위를 지정합니다. All 선택 시 여러 출처를 대상으로 할 수 있으므로 제거 범위를 확인하세요.',
    'SourceId':'효과 출처 이름. 동일한 분류와 ID로 생성된 효과를 검색하거나 제거합니다.',
    'DisplayName':'표시용 이름. Source Id와 다른 값이며 효과 식별·제거 키가 아닙니다.',
    'StatType':'읽거나 변경할 스탯 종류. 현재 체력 같은 자원값과 최대 스탯값은 구분합니다.',
    'Operation':'스탯 연산 방식. Flat·Additive Percent·Multiplicative·독립 피해 감소·Permanent Flat을 구분합니다.',
    'Value':'선택한 스탯 연산에 사용할 수치. Multiplicative의 1.2는 20% 증가입니다.',
    'Amount':'해당 노드가 적용·회복·소비할 양. 실제 적용량은 상한·부족 여부 등에 따라 달라질 수 있습니다.',
    'Count':'제거할 스택 그룹 개수. 하나의 그룹에 속한 효과는 함께 제거됩니다.',
    'RemovedCount':'이번 요청으로 실제 제거된 스택 그룹 개수.',
    'RemovedAmount':'이번 요청에서 실제 제거된 실드 양.',
    'bRequireFullCount':'체크하면 요청 개수가 부족할 때 아무것도 제거하지 않고 실패합니다. 해제하면 가능한 개수만 제거할 수 있습니다.',
    'Policy':'적용·제거 정책 열거형. 스택 제거에서는 Oldest(오래된 그룹부터)/Newest(최근 그룹부터)를 선택합니다.',
    'LifetimeFilter':'실드 제거 시 시간제·무기한 등 수명 유형으로 범위를 좁힙니다.',
    'bStackOnly':'수치 계산 없이 스택 카운트로만 사용할 효과입니다.',
    'StackGroupHandle':'여러 보정을 하나의 스택 그룹으로 묶는 식별자. 동일 출처 규칙도 충족해야 합니다.',
    'HUDDisplay':'버프/디버프/숨김 등 HUD 표시 방식을 선택합니다.',
    'HUDName':'HUD에 표시할 효과 이름. 효과 제거 키와 다릅니다.',
    'HUDIcon':'HUD에 표시할 아이콘 텍스처.',
    'bGuaranteeInvulnerability':'효과가 유지되는 동안 무적 보장을 제공할지 선택합니다.',
    'bGuaranteeEvasion':'효과가 유지되는 동안 회피 보장을 제공할지 선택합니다.',
    'Goal':'추적할 Actor. 목표 좌표가 아니라 목표 객체 자체를 전달합니다.',
    'Destination':'이동할 월드 좌표(Vector). 로컬 좌표와 구분합니다.',
    'AcceptanceRadius':'목표 도착 허용 반경(cm/uu). 기본 -1은 엔진의 기본 허용값을 사용합니다.',
    'bStopOnOverlap':'도착 판정에 Pawn의 반경을 고려할지 선택합니다.',
    'bUsePathfinding':'NavMesh 경로 계산 사용 여부. 해제하면 장애물을 우회하는 경로를 기대할 수 없습니다.',
    'bAllowPartialPath':'완전한 목표까지 닿지 않는 부분 경로를 허용할지 선택합니다.',
    'Direction':'이동·조준 방향 벡터. 위치 좌표가 아니며 해당 함수의 월드/로컬 기준을 확인하세요.',
    'WorldDirection':'월드 기준 방향 벡터.',
    'Velocity':'목표 속도 벡터(cm/s). 위치나 거리와 다릅니다.',
    'Speed':'이동 속도(cm/s). 방향·지속시간과 별개입니다.',
    'bBlocked':'해당 기능을 차단할지 나타내는 Bool.',
    'Definition':'사용할 데이터 에셋 참조. 문자열 ID나 에셋 이름이 아닌 실제 DA 객체입니다.',
    'ItemDefinition':'아이템 데이터 에셋 참조. 지급할 종류·런타임·스킬 정보를 담습니다.',
    'CandidateDefinition':'선택한 진화 후보 DA 참조.',
    'PickupClass':'생성할 픽업 Actor의 클래스. 아이템 DA와 별도로 외형·픽업 동작을 선택합니다.',
    'DesiredLocation':'생성을 원하는 월드 위치. 충돌 조건에 따라 보정되거나 생성이 실패할 수 있습니다.',
    'SpawnedPickup':'성공적으로 생성한 픽업 Actor 참조. 실패 시 유효하지 않을 수 있습니다.',
    'DropOwner':'드롭의 소유·출처 Actor.',
    'ItemId':'아이템 유형 내에서 유일한 양의 정수 ID.',
    'ItemTypeTag':'무기·액티브 유물·패시브 유물·소모품의 정확한 유형 태그.',
    'OptionalItemTypeTag':'유형 필터. 유효하지 않은 태그는 유형 전체를 뜻합니다.',
    'AdditionalTag':'DA의 문자열 Additional Tags 라벨. Gameplay Tag와 구분하며 빈 문자열은 검색하지 않습니다.',
    'StateId':'아이템 UI 상태를 구별하는 태그. 실제 스탯·스킬을 자동 변경하지는 않습니다.',
    'CurrentValue':'현재 상태 수치. 표시용인지 실제 자원값인지 대상 노드를 확인하세요.',
    'MaximumValue':'표시·계산에 사용할 최대 수치.',
    'DamageResult':'이미 실행한 피해 노드의 결과. 경직 From Result에 그대로 연결하며 같은 결과를 반복 처리하지 않습니다.',
    'KillingDamage':'사망을 일으킨 피해 결과.',
    'BaseDamage':'공격 스탯 계수 적용 전 기본 피해량.',
    'AttackPowerCoefficient':'공격력에 곱해서 피해에 더할 계수.',
    'SpellPowerCoefficient':'주문력에 곱해서 피해에 더할 계수.',
    'DamageMultiplier':'기본 피해와 공격·주문력 계수 합에 적용하는 피해 배율.',
    'Attribute':'Physical / Fire / Magic / Void 등 피해 속성.',
    'DeliveryType':'직접(단타) 또는 지속(DOT) 피해 구분.',
    'DamageName':'대상별 동일 이름 피해의 재적용 제한 키. None은 이름 제한을 사용하지 않습니다.',
    'DamageNameInterval':'같은 대상·Damage Name 피해를 무시하는 시간(초), 기본 0.2. 피해를 모아 나중에 주는 옵션이 아닙니다.',
    'bIgnoreDamageNameInterval':'이번 요청은 이름 간격 제한을 우회합니다. 기존 제한 창을 연장·초기화하지 않습니다.',
    'AttackTag':'콘텐츠에서 타격을 분류하는 태그. 입력 태그와 다릅니다.',
    'bCanEvade':'피해 요청에서 회피 판정을 허용할지 선택합니다.',
    'bCanCritical':'피해 요청에서 치명타 판정을 허용할지 선택합니다.',
    'bIgnoreShield':'실드를 우회하여 체력에 피해를 적용할지 선택합니다. 우회만으로 실드가 파괴되는 것은 아닙니다.',
    'bApplyStaggerAndGroggyEachTick':'DOT의 성공 틱마다 경직·그로기 처리를 함께 요청합니다.',
    'StaggerTemplate':'DOT 성공 틱마다 사용할 경직·그로기 기본 수치. Target/Attacker는 해당 틱의 타격에서 채워집니다.',
    'BaseStaggerDamage':'기본 경직 피해. 최종값이 경직 저항보다 클 때 경직이 가능하며 슈퍼아머 등의 조건도 적용됩니다.',
    'StaggerMultiplier':'경직 피해 배율, 기본 1.',
    'BaseGroggyDamage':'기본 그로기 게이지 피해. 게이지를 사용하지 않는 대상은 별도 규칙을 따릅니다.',
    'EffectName':'경직 등의 콘텐츠 효과 이름. Damage Name 간격 제한 키를 대신하지 않습니다.',
    'DotName':'같은 DOT 등록·갱신을 구별하는 이름. 직접 피해의 Damage Name과 별개입니다.',
    'StackPolicy':'DOT 독립 생성/갱신 등의 정책.',
    'Outcome':'Applied/Blocked/Evaded/Invalid 등 피해 처리 결과. Applied는 검색할 별도 노드명이 아닌 열거형 값입니다.',
    'FinalDamage':'최종 피해 정수. 실드와 체력에 분배될 합계.',
    'ShieldDamage':'실드에 실제 적용된 피해량.',
    'HealthDamage':'체력에 실제 적용된 피해량.',
    'HitContext':'적용된 타격의 공격원·대상·결과 정보. 직접 피해 From Result 경로에서는 수동 조립할 필요가 없습니다.',
    'bCritical':'이번 피해의 치명타 여부.',
    'bKilledTarget':'이번 피해로 대상이 사망했는지 나타냅니다.',
    'bOnHitEffectsTriggered':'직접 타격 On Hit 경로가 발동했는지 나타냅니다.',
    'PreviousShield':'실드 파괴 직전의 총 실드 양.',
    'Id':'시스템이 발급하는 고유 식별자. 임의로 번호를 만들어 유효한 핸들을 대체하지 마세요.',
    'Serial':'액션 실행 세대를 구별하는 값. 재사용한 오래된 핸들 오인 방지에 사용합니다.',
    'Owner':'해당 핸들·정보를 소유한 객체 참조.',
    'ManaCost':'소비할 현재 마나 양.',
    'StaminaCost':'소비할 현재 스태미나 양.',
    'Cooldown':'재사용 대기시간(초). 쿨다운 감소·최소 쿨다운 설정과 구분합니다.',
    'Priority':'작은 숫자부터 검토합니다. 같은 우선순위 후보는 비용을 묶어 한꺼번에 판정합니다.',
}

TYPE_HELP = {
    'bool':'체크 여부 또는 참/거짓 상태.', 'float':'소수 수치.', 'double':'소수 수치.',
    'int32':'정수 수치.', 'int64':'정수 수치.', 'FName':'이름 식별 값.',
    'FText':'사용자에게 표시하는 문구.', 'FString':'문자열 값.',
    'FGameplayTag':'등록된 Gameplay Tag 값.', 'FVector':'3축 벡터(X/Y/Z).',
    'FVector2D':'2축 벡터(X/Y).', 'FGuid':'고유 식별 값.',
}

# Content-specific descriptions: do not infer units or lifecycle from types alone.
PIN.update({
    'ResourceType':'마나/스태미나 등 해당 자원의 종류. 최대 스탯을 변경하는 선택값이 아니라 현재 자원 처리를 구분합니다.',
    'AxisX':'평면 X축 이동 입력값.', 'AxisY':'평면 Y축 이동 입력값.',
    'DeltaSeconds':'이번 프레임 경과 시간(초). 시간에 비례한 처리에 사용합니다.',
    'HitboxActor':'해당 액션 종료·취소 시 정리할 히트박스 Actor.',
    'InActionHandle':PIN['ActionHandle'], 'InCombatSource':'히트박스의 공격 출처 Actor.',
    'InHitPolicy':'히트박스의 중복 대상 수락 정책.',
    'InstanceId':PIN['ItemInstanceId'], 'Interactor':'상호작용을 시도하는 Actor.',
    'Lifetime':'보장 효과의 유지 시간(초). 해당 적용 노드의 수명 규칙을 따릅니다.',
    'LockType':'기본 이동만 또는 전체 이동을 막을 잠금 종류.',
    'NewAimDirection':'새로 지정할 월드 조준 방향.',
    'NewCurrentValue':'현재 자원에 설정할 값. 증가량이 아니라 새 값입니다.',
    'NewSlotCount':'기본 소모품 슬롯 용량. 초과 보유 아이템 드롭 처리가 발생할 수 있습니다.',
    'Percentage':'해당 노드에서 사용하는 비율 값. 템플릿 카메라 확대값은 Foundation 스탯 증폭과 별개입니다.',
    'PlaneZ':'마우스 투영에 사용할 게임플레이 평면의 월드 Z 높이.',
    'Player':'보유 상태를 조회하거나 아이템을 받을 AR Player Character 참조.',
    'ReductionStat':'독립 피해 감소 연산을 확인·계산할 감소율 스탯 종류.',
    'Request':'해당 기능에 필요한 요청 데이터 묶음. 아래에서 실제 대상·수치·옵션을 펼쳐 확인합니다.',
    'Spec':'효과의 수치·시간·출처·옵션 묶음. 아래 펼침은 이 노드가 실제로 받는 구조체 타입을 기준으로 합니다.',
    'Rules':'액션을 어떤 사유에 취소할지 설정하는 취소 규칙.',
    'Scale':'기본 이동 방향 입력에 곱할 크기. Actor의 표시 스케일이 아닙니다.',
    'Screen':'열거나 관리할 UI 화면 종류.',
    'SlotIndex':'해당 시스템의 슬롯 번호. 빈 슬롯·유효 범위와 슬롯 종류를 확인하세요.',
    'Tag':'이 함수가 검사하는 태그. 상태 태그·추가 문자열 라벨·입력 태그는 서로 다른 값입니다.',
    'Token':'시스템이 발급한 요청·획득·진화 식별 토큰. 해당 요청에서 받은 원본으로 확정/취소합니다.',
    'TypeFilter':'검색할 아이템의 정확한 유형 태그. 생략·무효 값의 의미는 검색 노드에서 확인하세요.',
    'WorldLocation':'월드 위치(Vector). 방향·로컬 위치와 구분합니다.',
    'bForceMouseDirection':'이동 입력 대신 마우스 조준 방향으로 구르기를 강제할지 선택합니다.',
    'bUIOpen':'UI가 열려 있다는 상태를 전달합니다. 수동 게임플레이 입력 잠금과 별개입니다.',
    'Cost':'마나·스태미나 자원 비용 구조체. 현재 자원에서 소비할 양을 지정합니다.',
    'Candidate':'처리·검사할 상호작용 후보 Actor.',
    'ContextObject':'요청을 발생시키거나 처리하는 문맥 객체 참조.',
    'ActionRequest':'스킬 실행 시 시작할 액션의 취소·이동 차단·소유 설정.',
    'ResourceCost':'스킬·행동에서 소비할 현재 마나와 스태미나 비용.',
    'BaseCooldown':'스킬 기본 쿨다운(초). 쿨다운 감소를 적용하기 전 값.',
    'MinimumCooldown':'쿨다운 감소를 적용해도 내려가지 않을 최소 쿨다운(초).',
    'CooldownRemaining':'재사용까지 남은 시간(초).', 'CooldownTotal':'이번에 적용한 전체 쿨다운 시간(초).',
    'CancelRules':'기본 이동·구르기·경직·기절 중 어떤 사유에 이 액션을 취소할지 선택합니다.',
    'bCancelOnBasicMovementInput':'허용된 기본 이동 입력이 들어오면 이 액션을 취소할지 선택합니다.',
    'bCancelOnRoll':'구르기 시작 요청으로 이 액션을 취소할지 선택합니다.',
    'bCancelOnStagger':'경직에 의해 이 액션을 취소할지 선택합니다.',
    'bCancelOnStun':'기절 적용에 의해 이 액션을 취소할지 선택합니다.',
    'bBlockBasicMovementWhileActive':'이 액션 동안 일반 이동을 차단할지 선택합니다.',
    'bBlockRollWhileActive':'이 액션 동안 구르기를 차단할지 선택합니다.',
    'bIsRollAction':'이 실행을 구르기 액션으로 분류합니다. 입력 태그로 구르기를 판별하는 옵션이 아닙니다.',
    'OwningItemInstanceId':'이 액션/등록 스킬을 소유한 아이템 인스턴스 ID. 아이템 해제 시 정리에 사용합니다.',
    'DamageRequest':'DOT 틱에 사용할 피해 요청 구조체.',
    'Delivery':'직접(Direct) / 지속(Damage Over Time) 피해 방식. 속성과 별개입니다.',
    'bCanCrit':'이번 피해에서 치명타 판정을 허용할지 선택합니다.',
    'bApplyEvasion':'이번 피해에서 회피 판정을 적용할지 선택합니다.',
    'bApplyAmplification':'공격자의 전체·직접/지속·속성 피해 증폭을 적용할지 선택합니다.',
    'bApplyAbsorption':'성공 피해에 공격자의 흡수 회복을 적용할지 선택합니다.',
    'bApplyOnHitEffects':'직접 타격의 On Hit 효과 경로를 허용할지 선택합니다. DOT는 해당 경로를 사용하지 않습니다.',
    'bIgnoreDefense':'이번 피해에서 방어력 계산을 우회할지 선택합니다.',
    'bGuaranteedHit':'해당 타격을 확정 명중으로 처리할지 선택합니다. 무적 등 모든 규칙 우회와 동일하지 않습니다.',
    'bValidHit':'타격 문맥이 실제 적용된 유효 타격인지 나타냅니다.',
    'HitId':'성공 타격의 식별 ID. 경직 노드 반복 호출을 자동으로 중복 제거해주는 보장은 아닙니다.',
    'bValidRequest':'경직 요청이 유효한 대상으로 처리되었는지 나타냅니다.',
    'bStaggerAttempted':'경직 판정을 시도했는지 나타냅니다.',
    'bStaggered':'이번 요청으로 실제 경직이 발생했는지 나타냅니다.',
    'bBlockedBySuperArmor':'슈퍼아머 때문에 경직이 차단되었는지 나타냅니다.',
    'bGroggyDepleted':'이번 요청으로 그로기 게이지가 소진되었는지 나타냅니다.',
    'FinalStaggerDamage':'공격자의 경직력과 배율을 반영한 최종 경직 피해.',
    'FinalGroggyDamage':'그로기 피해 증폭을 반영한 최종 게이지 피해.',
    'CurrentGroggy':'대상에게 남은 현재 그로기 게이지.',
    'StaggerPowerSnapshot':'공격자의 경직력 스냅샷. 공격원 소멸 등 이후 처리를 위해 저장한 값.',
    'GroggyDamageAmplificationSnapshot':'공격자의 그로기 피해 증폭 스냅샷.',
    'StaggerPower':'공격자의 경직력 값.',
    'GroggyDamageAmplification':'공격자의 그로기 피해 증폭 값.',
    'AttackPower':'공격력 값. 공격 계수와 곱해 피해를 계산합니다.',
    'SpellPower':'주문력 값. 주문 계수와 곱해 피해를 계산합니다.',
    'Absorption':'성공 피해량에 비례한 흡수 회복 수치.',
    'CriticalChance':'치명타 확률(%). 대상·요청 규칙에 따라 판정됩니다.',
    'CriticalDamage':'치명타 피해 배율(%). 150은 1.5배입니다.',
    'DefensePenetration':'양수 속성 방어력을 무시할 비율(%).',
    'OverallAmplification':'공격자의 전체 피해 증폭(%).',
    'DeliveryAmplification':'직접/지속 방식에 대응하는 피해 증폭(%).',
    'AttributeAmplification':'선택한 피해 속성에 대응하는 증폭(%).',
    'BaseValue':'스탯의 기본 수치. 현재 자원량이나 보정 포함 최종값과 구분합니다.',
    'FinalValue':'보정과 제한을 적용한 최종 스탯 수치.',
    'FlatTotal':'일반 Flat 보정의 합계.',
    'AdditivePercentTotal':'합연산 퍼센트 보정의 합계.',
    'MultiplicativeProduct':'곱연산 배율을 모두 곱한 값.',
    'Breakdown':'기본값·Flat·퍼센트·곱연산·최종값의 계산 내역.',
    'StatName':'해당 스탯의 표시 이름.',
    'ProvidedStats':'이 표시 효과가 제공하는 스탯 항목 목록.',
    'StackCount':'조회 범위의 적용/스택 그룹 개수.',
    'bHasPermanent':'Duration 음수인 무기한 버프가 있는지 나타냅니다. Permanent Flat 수정 기록이 아닙니다.',
    'LongestRemainingTime':'시간제 효과 중 가장 오래 남은 시간(초). 무기한 효과 여부는 별도 값으로 확인합니다.',
    'RemainingTime':'해당 시간제 효과의 남은 시간(초).',
    'DurationOverride':'DA 기본 지속시간 대신 요청별 시간을 지정하는 값. 기본/특수 값의 의미는 상태 적용 노드를 확인합니다.',
    'bRefreshedExisting':'새 효과가 아니라 기존 동일 상태를 갱신했는지 나타냅니다.',
    'Result':'해당 처리의 결과 코드 또는 결과 데이터 묶음.',
    'Message':'실패·요청 결과의 설명 문구.',
    'bExists':'해당 조회 대상이 실제로 존재하는지 나타냅니다.',
    'bReady':'해당 스킬/상태가 사용 가능한 준비 상태인지 나타냅니다.',
    'Current':'해당 자원의 현재 수치.', 'Maximum':'해당 자원의 최대 수치.',
    'Ratio':'현재값/최대값의 비율. 최대값이 0인 경우의 안전 처리는 시스템 결과를 사용하세요.',
    'Health':'현재/최대 체력 표시 데이터.', 'Shield':'현재 총 실드 표시 데이터.',
    'Mana':'마나 자원값 또는 비용. 소유 구조체의 용도를 확인합니다.',
    'Stamina':'스태미나 자원값 또는 비용. 소유 구조체의 용도를 확인합니다.',
    'AimDirection':'현재 월드 조준 방향 벡터.',
    'AimWorldLocation':'현재 월드 조준 위치.',
    'bHasAimWorldLocation':'월드 조준 위치를 유효하게 계산했는지 나타냅니다.',
    'Candidates':'요청에서 선택할 후보 DA/객체의 배열.',
    'bRequiresSelection':'확정을 위해 콘텐츠 UI 등에서 후보 선택이 필요한지 나타냅니다.',
    'bWillReplaceWeapon':'이 획득 확정이 기존 장착 무기를 교체할지 나타냅니다.',
    'PreviousSlotIndex':'이전 장착/보유 슬롯 번호.',
    'EquippedWeapon':'현재 장착된 무기 정보.',
    'bHasEquippedWeapon':'장착된 무기가 실제로 있는지 나타냅니다.',
    'ActiveRelics':'장착된 액티브 유물 표시/보유 정보 배열.',
    'Consumables':'소모품 슬롯 정보 배열.',
    'Skills':'등록된 스킬 표시/쿨다운 정보 배열.',
    'UIStates':'콘텐츠가 지정한 아이템 UI 상태 목록.',
    'StatEffects':'HUD에 표시할 스탯 효과 목록.',
    'StatusEffects':'현재 상태 효과 정보 목록.',
    'RegisteredHandle':'등록된 스킬을 구별하는 핸들.',
    'SkillGroupHandle':'하나의 입력으로 실행한 스킬 그룹 식별 핸들.',
    'EvolvedInstance':'진화에 성공한 뒤 생성/등록한 아이템 인스턴스 참조.',
    'SourceDefinition':'이 결과/스킬/표시 정보의 원본 아이템 DA.',
    'SuggestedLocation':'드롭 생성에 사용할 제안 월드 위치.',
    'bCausedBySlotReduction':'슬롯 용량 감소 때문에 발생한 초과 드롭인지 나타냅니다.',
    'bOccupied':'해당 슬롯에 아이템이 들어 있는지 나타냅니다.',
    'bOverflowSlot':'현재 용량을 초과한 슬롯인지 나타냅니다.',
    'SkillDisplayName':'스킬의 표시 이름.',
    'SkillDescription':'스킬의 표시 설명.',
    'SkillIcon':'스킬의 HUD 아이콘.',
    'SlotSkillIndex':'해당 유물 슬롯에서 사용할 로컬 스킬 인덱스.',
    'InputAction':'플레이어 입력을 받을 Enhanced Input Action 에셋 참조.',
    'InputMode':'태그 입력/슬롯 입력 등 해당 스킬의 요청 방식.',
    'InputPriority':'같은 입력 후보의 처리 우선순위. 작은 값부터, 같은 값끼리 비용을 묶어 판정합니다.',
    'HUDSortOrder':'HUD 표시 순서. 스킬 실행 우선순위와 다릅니다.',
    'MaxDisplaySlots':'HUD에 표시할 슬롯 수 설정.',
    'Kind':'해당 UI/아이템 정보의 종류.',
    'Placement':'슬롯 배치/획득 결과 정보.',
    'Display':'이 상태의 HUD 표시 설정.',
    'DisplayType':'숫자·막대 등 표시 방식.',
    'EffectiveDisplayType':'최종적으로 결정된 실제 표시 방식.',
    'DisplayText':'HUD에 표시할 문구.',
    'Description':'표시용 설명 문구. 실행 조건을 자동 적용하는 설정이 아닙니다.',
    'Template':'실제 Widget 표시 제작에 사용할 템플릿 참조/설정.',
    'bShowNumberAlongside':'다른 표시와 함께 숫자를 표시할지 선택합니다.',
    'AdditionalTags':'아이템 카탈로그 검색용 문자열 라벨 배열. Gameplay Tag와 다릅니다.',
})

PURPOSE_EXTRA = {
    'SetActionTimerByEvent':'스킬·적 공격의 선딜/후딜/반복 콜백에 권장합니다. 한 액션에 1회 또는 반복 타이머를 귀속하여 정상 종료·실제 취소·소유자 EndPlay 때 자동 Clear합니다. 콜백 완료만으로 액션은 끝나지 않습니다. 호출마다 독립 타이머입니다.',
    'SetItemTimerByEvent':'무기·유물 보유 중 주기 효과/1회 지연에 권장합니다. 등록된 아이템에 예약을 귀속하여 버림·교체·소유자 EndPlay 때 On Item Unregistered 전에 자동 Clear합니다. 스킬 취소만으로는 멈추지 않습니다. 적/픽업 Actor나 소모품 런타임의 노드가 아닙니다.',
    'ReceiveItemUnregistered':'아이템 해제 때 자기 변수·연출·구독·독립 작업을 정리합니다. 아이템 전용 타이머는 이 이벤트 전에 자동 Clear되어 있으며 새 아이템 타이머 등록은 거절됩니다. 기존 일반 엔진 타이머는 직접 Clear해야 합니다.',
    'AddCCImmunity':'이 상태 컴포넌트에 새 CC를 막는 보장 효과를 적용하고 핸들을 발급합니다. 기존 CC를 제거하지는 않습니다.',
    'ApplyCCImmunity':'Actor에게 CC 면역을 적용합니다. 새 기절·속박 등을 차단하고 이미 걸린 CC는 자동 해제하지 않습니다.',
    'ApplyActionCCImmunity':'실행 중 액션에 CC 면역을 귀속합니다. 해당 액션 종료·취소 시 자동 회수됩니다.',
    'ApplyItemCCImmunity':'보유 아이템에 CC 면역을 귀속합니다. 해당 아이템 해제 시 자동 회수됩니다.',
    'ApplyItemSuperArmor':'보유 아이템에 경직 방지 슈퍼아머를 귀속합니다. 해당 아이템 해제 시 회수되며 CC·그로기 통합 면역은 아닙니다.',
    'CountOwnedItemsByAdditionalTag':'플레이어 보유 아이템 중 Additional Tags 문자열 라벨과 유형 필터에 맞는 개수를 조회합니다.',
    'FindItemByKey':'정확한 아이템 유형 태그와 양의 Item Id로 DA 하나를 찾습니다. 없거나 동일 키가 중복이면 유효한 결과를 반환하지 않습니다.',
    'FindItemsByAdditionalTag':'Additional Tags 문자열 라벨에 맞는 아이템 DA 배열을 찾습니다. 선택적 유형 필터로 범위를 좁힙니다.',
    'GetVisibleStatEffects':'HUD 표시가 허용된 활성 스탯 효과 목록을 읽습니다. 스탯을 적용하거나 HUD 위젯을 생성하지 않습니다.',
    'HandleActiveRelicSlotInput':'지정 액티브 유물 슬롯과 로컬 스킬 인덱스로 사용 요청을 처리합니다. 조건·비용·쿨다운 검사 후 런타임 스킬을 실행합니다.',
    'HasAdditionalTag':'이 아이템 DA가 지정한 Additional Tags 문자열 라벨을 가지고 있는지 확인합니다.',
    'IsCCImmunityActive':'이 대상에게 새 CC 적용을 막는 보장이 유지 중인지 조회합니다.',
    'IsConsumable':'이 DA의 유형이 소모품인지 확인합니다.',
    'RemoveCCImmunity':'발급받은 CC 면역 핸들 하나를 회수합니다. 다른 면역 핸들이 남으면 보장은 유지됩니다.',
    'RemoveCCImmunityBySource':'출처 조건에 맞는 CC 면역 보장을 제거합니다. 기존 상태이상 제거와는 별개입니다.',
    'RemoveOneModifierStack':'이전 저장 그래프 호환용 1개 스택 제거입니다. 새 콘텐츠는 Remove Stat Modifier Stacks의 Count를 사용하세요.',
    'RemoveOneStatModifierStack':'이전 저장 그래프 호환용 1개 스택 제거입니다. 새 콘텐츠는 Remove Stat Modifier Stacks의 Count를 사용하세요.',
    'RemoveOwnItemCCImmunity':'이 아이템이 적용한 CC 면역 핸들 하나를 조기 회수합니다.',
    'RemoveOwnItemSuperArmor':'이 아이템이 적용한 슈퍼아머 핸들 하나를 조기 회수합니다.',
    'TryAcquireItem':'DA 유형에 따라 플레이어의 장비/소모품 획득 요청을 처리합니다. 월드 픽업을 생성하는 함수가 아닙니다.',
    'ValidateItemCatalog':'아이템 카탈로그의 ID·유형·정의 중복 등 저작 오류를 검사합니다. 에셋을 자동 수정하지 않습니다.',
}

def describe(name, typ, func='', owner=''):
    if name == 'Duration' and func == 'ActionDelay':
        return '액션에 귀속된 대기 시간(초). 유효한 액션에서 0 이하는 즉시 완료하며 액션 종료·취소 시 대기는 Cancelled입니다.'
    if name == 'Duration' and func == 'ApplyCrowdControl':
        return 'CC 지속시간(초), 양수만 허용. 기본 1초이며 강인함 적용 여부는 별도 체크합니다.'
    if name == 'Duration' and ('Spec' in owner):
        if owner == 'FARDamageOverTimeSpec':return 'DOT 유지 시간(초), 양수.'
        if owner == 'FARStatModifierSpec':return '음수=무기한 버프, 양수=시간제, 0=거절. Permanent Flat에는 무시됩니다.'
        if any(x in owner for x in ('Shield','Armor','Immunity')):return '음수=무기한, 양수=시간제, 0=거절.'
    if name == 'Target' and owner == 'AARAIController':return '노드의 Target은 적의 AR AI Controller입니다. 목표 Actor는 별도의 Goal에 연결합니다.'
    if name == 'Handle' and 'Action' in owner:return PIN['ActionHandle']
    if name in PIN:return PIN[name]
    if name in reference.FIELD_PURPOSE:return reference.FIELD_PURPOSE[name]
    base = re.sub(r'\bconst\b|[&*]', '', typ).strip()
    if base.startswith('TArray<'):return '여러 '+words(name)+' 값을 담는 배열. 비어 있을 수 있으며 각 요소의 타입은 아래 표시를 따릅니다.'
    if '*' in typ or base.startswith(('TObjectPtr<','TWeakObjectPtr<','TSubclassOf<')):
        return words(name)+' 참조. 해당 타입의 유효한 객체/클래스를 사용하며 None·파괴된 참조를 먼저 검사하세요.'
    if base.startswith('EAR'):return words(name)+' 정책·상태를 선택하는 열거형. 실제 선택값은 Blueprint 해당 핀 목록을 확인하세요.'
    if base.startswith('FAR'):return words(name)+' 데이터 묶음. 내부 필드는 아래 펼침에서 확인합니다.'
    return words(name)+' 값. '+TYPE_HELP.get(base, '선언 타입에 맞는 데이터를 전달합니다.')

def extract():
    nodes, structs, delegates = [], {}, {}
    files = sorted(HEADERS.rglob('*.h'))
    texts = {p:p.read_text(encoding='utf-8-sig') for p in files}
    ENUMS.clear()
    for path,text in texts.items():
        for pos,end,meta in annotations(text,'UENUM'):
            if 'BlueprintType' not in meta:continue
            match=re.search(r'enum\s+(?:class\s+)?(\w+)\s*(?::\s*\w+)?\s*\{',text[end:])
            if not match:raise ValueError('unparsed enum '+str(path))
            start=end+match.end()-1;finish=balanced(text,start,'{','}')
            body=re.sub(r'/\*.*?\*/|//[^\n]*','',text[start+1:finish],flags=re.S)
            values=[]
            for raw in reference.split_params(body):
                if 'Hidden' in raw:continue
                found=re.match(r'\s*(\w+)',raw)
                if found:
                    name=found.group(1);values.append((name,meta_value(raw,'DisplayName') or words(name)))
            ENUMS[match.group(1)] = values
        for m in re.finditer(r'\bDECLARE_DYNAMIC_MULTICAST_DELEGATE(?:_\w+)?\s*\(', text):
            start=text.index('(',m.start());end=balanced(text,start)
            args=reference.split_params(text[start+1:end]); params=[]
            if (len(args)-1)%2:raise ValueError('bad delegate '+args[0])
            for typ,name in zip(args[1::2],args[2::2]):params.append(parameter(typ+' '+name,direction='출력'))
            delegates[args[0]]=params
    for path,text in texts.items():
        groups=containers(text)
        for pos,end,meta in annotations(text,'USTRUCT'):
            if 'BlueprintType' not in meta:continue
            group=next(g for g in groups if g[1]>=end)
            structs[group[0]]={'fields':[], 'source':str(path.relative_to(ROOT)), 'start':group[1], 'end':group[2]}
        for pos,end,meta in annotations(text,'UPROPERTY'):
            if not any(x in meta for x in ('BlueprintReadOnly','BlueprintReadWrite','BlueprintAssignable')):continue
            owner=owner_at(groups,pos)
            semi=text.index(';',end)
            raw=' '.join(text[end:semi].split())
            field=parameter(raw)
            if owner in structs:structs[owner]['fields'].append(field)
            elif 'BlueprintAssignable' in meta:
                nodes.append({'owner':owner,'name':field['name'],'label':words(field['name']), 'kind':'디스패처',
                    'category':meta_value(meta,'Category') or 'Events', 'inputs':[], 'outputs':delegates[field['type']],
                    'source':str(path.relative_to(ROOT)), 'line':text.count('\n',0,pos)+1, 'meta':meta,'signature':raw, 'hidden':False})
        for pos,end,meta in annotations(text,'UFUNCTION'):
            if not any(x in meta for x in ('BlueprintCallable','BlueprintPure','BlueprintImplementableEvent','BlueprintNativeEvent')):continue
            owner=owner_at(groups,pos)
            stop=min(x for x in (text.find(';',end),text.find('{',end)) if x>=0)
            sig=' '.join(text[end:stop].split())
            m=re.match(r'(?:(?:static|virtual)\s+)*(.*?)\s+(\w+)\s*\((.*)\)\s*(?:const)?\s*(?:override)?\s*(?:=\s*0)?$',sig)
            if not m:raise ValueError(str(path)+': unparsed Blueprint declaration '+sig)
            ret,name,args=m.groups()
            params=[parameter(a,meta) for a in reference.split_params(args)]
            world=meta_value(meta,'WorldContext')
            advanced=meta_value(meta,'AdvancedDisplay').split(',')
            for i,p in enumerate(params):
                p['automatic']=p['name']==world
                p['advanced']=p['name'] in advanced or (len(advanced)==1 and advanced[0].isdigit() and i>=int(advanced[0]))
            impl='BlueprintImplementableEvent' in meta or 'BlueprintNativeEvent' in meta
            kind='구현 이벤트' if impl else ('조회' if 'BlueprintPure' in meta else '호출')
            is_event=impl and ret=='void'
            inputs=[] if is_event else [p for p in params if p['direction']=='입력']
            outputs=params if is_event else [p for p in params if p['direction']=='출력']
            if is_event:
                for p in outputs:p['direction']='출력'
            if ret!='void':outputs.insert(0,{'name':'ReturnValue','label':'Return Value','type':ret,'default':'','direction':'출력','advanced':False,'automatic':False})
            if name=='ActionDelay':
                kind='비동기'
                outputs=[parameter('exec Completed',direction='출력'),parameter('exec Cancelled',direction='출력')]
            nodes.append({'owner':owner,'name':name,'label':meta_value(meta,'DisplayName') or words(name),
                'kind':kind,'event_form':is_event,'category':meta_value(meta,'Category') or '기타', 'inputs':inputs,'outputs':outputs,
                'source':str(path.relative_to(ROOT)), 'line':text.count('\n',0,pos)+1,'meta':meta,'signature':sig,
                'hidden':name!='ActionDelay' and ('BlueprintInternalUseOnly' in meta or 'DeprecatedFunction' in meta)})
    for name,st in structs.items():
        nodes.append({'owner':name,'name':name,'label':'Make / Break '+words(name[1:]),'kind':'구조체',
                      'category':'구조체 | '+('Handle' if name.endswith('Handle') else 'Data'),
                      'inputs':[],'outputs':[],'source':st['source'],'line':0,'meta':'','signature':'','hidden':False})
    for n in nodes:
        n['id']='node-'+n['owner']+'-'+n['name']
    keys=[n['id'] for n in nodes]
    if len(set(keys))!=len(keys):raise ValueError('duplicate IDs')
    return sorted(nodes,key=lambda n:(n['label'].casefold(),n['owner'])),structs

RELATED = {
    'Combat':'DAMAGE_NODES_FORMULA_GUIDE_KO.html', 'CC':'CC_STAGGER_GUIDE_KO.html',
    'Stats':'../precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html',
    'Action':'../precautions/ACTION_LIFECYCLE_GUIDE_KO.html',
    'Movement':'../hhc/ENEMY_MOVEMENT_GUIDE_KO.html',
    'Loadout':'../nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html',
    'Item':'../nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html',
    'Health':'DAMAGE_NODES_FORMULA_GUIDE_KO.html', 'Resources':'DAMAGE_NODES_FORMULA_GUIDE_KO.html',
    'World':'../environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html',
    'Catalog':'../environment/ITEM_SEARCH_GUIDE_KO.html',
}

def nested(typ, structs, chain=()):
    # Arrays show their element fields; do not invent pins for UObject pointers.
    names=re.findall(r'\bFAR\w+',typ)
    if not names or names[0] not in structs:
        enum_type=next((name for name in ENUMS if re.search(r'\b'+name+r'\b',typ)),None)
        if enum_type:
            return '<details class="pin-structure"><summary>선택 가능한 값 펼치기</summary><div class="node-body"><ul>'+''.join('<li><code>'+e(name)+'</code> · '+e(label)+'</li>' for name,label in ENUMS[enum_type])+'</ul><p class="muted">표시명은 소스 Display Name을 우선합니다. 값의 판정·사용 규칙은 위 핀 설명과 관련 가이드를 따릅니다.</p></div></details>'
        return ''
    name=names[0]
    if name in chain or len(chain)>=4:return '<p class="muted">중첩 구조체: '+e(name)+' — 구조체 이름으로 검색하여 별도 항목에서 확인하세요.</p>'
    fields=structs[name]['fields']
    contents=''.join(pin_html(p,structs,owner=name,chain=chain+(name,)) for p in fields)
    return '<details class="pin-structure"><summary>'+e(words(name[1:]))+' 내부 '+str(len(fields))+'개 값 펼치기</summary><div class="node-body"><p class="muted">구조체 핀으로 통째로 연결하거나, 허용되는 경우 Split Struct Pin / Make / Break로 개별 값을 연결합니다.</p><dl class="pins">'+contents+'</dl></div></details>'

def pin_html(p,structs,func='',owner='',chain=()):
    name,typ=p['name'],p['type']
    label=p['label']
    explanation=describe(name,typ,func,owner)
    if name=='ReturnValue':
        if typ=='FTimerHandle':explanation='Success=true일 때 실제 Timer Handle. 기존 Pause Timer by Handle / Unpause Timer by Handle / Clear and Invalidate Timer by Handle에 연결합니다. Action Handle과 다른 타입입니다.'
        elif typ=='bool':explanation='이 함수의 성공 여부 또는 조회 조건 결과. 노드 용도의 조건을 참/거짓으로 돌려줍니다.'
        elif 'Handle' in typ:explanation='이번 등록·적용에서 발급하거나 조회한 핸들. 이후 제거·종료·조회용으로 보관합니다. 별도 Success 출력이 있다면 먼저 확인하세요.'+(' Permanent Flat 성공은 빈 핸들입니다.' if func=='ApplyStatModifier' else '')
        elif typ in ('int32','float'):explanation='이 함수가 계산·조회한 수치 또는 실제 처리한 양/개수. 노드 용도와 선언 타입을 함께 확인하세요.'
        else:explanation='이 노드의 용도에 대한 결과 값/객체입니다. 배열은 여러 결과, 구조체는 묶음 결과이며 아래 내부 값을 펼쳐 확인하세요.'
    if name=='Completed':explanation='액션이 유효한 상태에서 대기 시간이 끝난 실행 출력. 공격 등의 후속 동작을 여기에 연결합니다.'
    if name=='Cancelled':explanation='대기 중 액션 종료·취소 또는 무효화로 중단된 실행 출력. 후속 공격을 하지 않고 자체 상태를 정리합니다.'
    badges=(' · 자동 문맥' if p['automatic'] else '')+(' · 고급 핀' if p['advanced'] else '')
    default='<span class="pin-default">선언 기본값: <code>'+e(p['default'])+'</code></span>' if p['default'] else ''
    return '<dt>'+e(label)+'</dt><dd><span class="pin-type">'+e(typ)+e(badges)+'</span>'+default+'<p>'+e(explanation)+'</p>'+nested(typ,structs,chain)+'</dd>'

def render_node(n,structs):
    is_struct=n['kind']=='구조체';event=n['kind']=='구현 이벤트';dispatch=n['kind']=='디스패처'
    desc=reference.EVENT_PURPOSE.get(n['name'],'이 인스턴스의 '+n['label']+' 발생을 구독하는 알림입니다.') if dispatch else PURPOSE_EXTRA.get(n['name'],reference.purpose(n['name'],n['category'],n['kind']))
    if n['owner']=='UARAsyncActionDelay' and n['name']=='Cancelled':
        desc='Action Delay 대기 중 소유 액션이 종료·취소되거나 무효해지면 받는 출력입니다. 정상 End Action도 미완료 대기는 Cancelled로 끝내므로 실제 액션 취소 전용 이벤트와 구분합니다.'
    if is_struct:desc='여러 값을 한 핀으로 전달·해석하는 데이터 묶음입니다. 아래 필드는 현재 헤더의 Blueprint 노출 필드이며, 읽기 전용 결과/핸들은 시스템이 발급한 값을 Break로 읽는 용도로 사용하세요.'
    if is_struct:target='Actor/Component Target 없음. 구조체 값 자체를 연결합니다. 아래 목록은 자동 Make/Break 파생 노드의 데이터 사전이며, 타입에 따라 Make 핀 노출이 제한될 수 있습니다.'
    elif dispatch:target=n['owner']+' 인스턴스에서 Bind Event / Assign으로 콜백을 연결합니다. 아래 전달값은 이벤트가 발생할 때 출력됩니다.'
    elif event:target=n['owner']+'의 자식 Blueprint에서 Overrides/이벤트로 구현합니다. void 이벤트의 매개변수는 발생 시 출력, 반환값이 있는 오버라이드는 함수 입력·Return Node 출력입니다.'
    elif 'static ' in n['signature']:target='함수 라이브러리의 static 노드: 자동 객체 Target 없음. 아래 명시적 Target/Action Component/월드 문맥을 구분해서 연결합니다.'
    else:target='Target: '+n['owner']+' 참조. 해당 타입에서 핀을 끌어 검색하세요. 효과를 받을 Actor가 별도 입력으로 있으면 이 Target과 구분합니다.'
    warning=''
    if n['hidden']:warning='<p class="note">호환용/내부용 선언입니다. 새 그래프의 검색 메뉴에 표시되지 않을 수 있으므로 일반 노드를 우선 사용하세요.</p>'
    if n['name'] in ('ReceiveItemSkillCancelled','ReceiveActionCancelled','OnActionCancelled'):
        warning+='<p class="note">실제 액션 취소 알림입니다. Set Action Timer by Event와 Action Delay는 자동 정리됩니다. 일반 Set Timer by Event·독립 Actor는 별도 정리하세요. Set Item Timer by Event는 아이템 해제에 귀속되며 스킬 취소만으로 멈추지 않습니다. 정상 종료 알림이 아닙니다.</p>'
    if n['name']=='ExecuteItemSkill':warning+='<p class="note">입력·비용 판정과 액션 시작 후 호출됩니다. 이 스킬에서 Try Start Action을 다시 하지 말고 전달된 Action Handle을 사용해 정상 완료에서 End Action 하세요.</p>'
    if n['name'] in ('SetActionTimerByEvent','SetItemTimerByEvent'):
        warning+='<p class="note">실행 출력은 예약 직후이며 실제 효과는 Event 콜백에 연결합니다. Success를 확인하고 Time은 양수로 지정하세요. 호출마다 새 타이머가 생기므로 Tick에서 반복 등록하지 않습니다. 자동 제거는 예약만 정리하며 이미 적용한 일반 버프·DOT·독립 Actor·Bool은 되돌리지 않습니다. 취소 출력이 없으므로 자체 상태 정리는 수명 이벤트에서 처리하세요.</p>'
    if is_struct:body=nested(n['name'],structs)
    else:
        inp=''.join(pin_html(p,structs,n['name'],n['owner']) for p in n['inputs'])
        out=''.join(pin_html(p,structs,n['name'],n['owner']) for p in n['outputs'])
        flow='실행 입력 → 실행 출력' if n['kind']=='호출' else ('실행 출력: 이벤트 발생' if n.get('event_form') else ('오버라이드 함수: 입력을 읽고 Return Node로 결과를 반환' if event else '순수 조회는 실행선 없음; const 호출의 순수 표시 여부는 에디터에서 확인'))
        if n['kind']=='비동기':flow='실행 입력 → Completed / Cancelled 분기'
        if dispatch:flow='Bind/Assign의 실행선과 콜백 이벤트의 실행선은 별개입니다.'
        body='<p class="muted">'+e(flow)+'</p><h3>입력값</h3><dl class="pins">'+(inp or '<dt>별도 데이터 입력 없음</dt><dd>위 연결 대상 설명을 확인하세요.</dd>')+'</dl><h3>출력값 · 반환값</h3><dl class="pins">'+(out or '<dt>별도 데이터 출력 없음</dt><dd>호출 노드는 다음 실행선으로 연결합니다.</dd>')+'</dl>'
    related=next((url for key,url in reversed(list(RELATED.items())) if key in n['category'] or key in n['owner']),None)
    if n['name']=='SetActionTimerByEvent':related='../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#managed-timers'
    if n['name']=='SetItemTimerByEvent':related='../nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html#managed-timers'
    links='<a href="'+e(related)+'">관련 제작 가이드</a> · ' if related else ''
    source=os.path.relpath(ROOT/n['source'],OUT.parent).replace('\\','/')
    technical='<details class="declaration"><summary>개발자용 선언·원본 보기</summary><p><a href="'+e(source)+'">'+e(n['source'])+'</a> · 줄 '+str(n['line'])+'</p><pre>'+e(n['signature'] or n['name'])+'</pre></details>'
    aliases=n['label']+' '+n['name']
    if is_struct:aliases+=' Make '+words(n['name'][1:])+' Break '+words(n['name'][1:])
    return '<details class="node node-entry" id="'+e(n['id'])+'" data-kind="'+e(n['kind'])+'" data-category="'+e(n['category'])+'" data-legacy="'+str(n['hidden']).lower()+'" data-name="'+e(aliases)+'" data-owner="'+e(n['owner'])+'"><summary><span><strong>'+e(n['label'])+'</strong><span class="node-meta">'+e(n['kind']+' · '+n['category']+' · '+n['owner'])+'</span></span></summary><div class="node-body"><p class="node-purpose">'+e(desc)+'</p><p class="note good">'+e(target)+'</p>'+warning+body+'<p>'+links+'<a href="#'+e(n['id'])+'">이 노드 바로가기</a></p>'+technical+'</div></details>'

def generate():
    from organize_development_guides import navigation
    nodes,structs=extract()
    cats=sorted({n['category'] for n in nodes})
    counts=Counter(n['kind'] for n in nodes)
    options=''.join('<option value="'+e(c)+'">'+e(c)+'</option>' for c in cats)
    cards='\n'.join(render_node(n,structs) for n in nodes)
    html='''<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>전체 노드 검색 · Action RogueLike</title><link rel="stylesheet" href="../assets/guide.css"><link rel="stylesheet" href="../assets/node-search.css"></head><body data-guide-ui="catalogue">
<!-- GUIDE PORTAL START -->'''+navigation(OUT,'common')+'''<!-- GUIDE PORTAL END -->
<header><div class="wrap"><div class="eyebrow">Action RogueLike · 공통 개발 참고서</div><h1>전체 노드 검색</h1><p class="lead">이름으로 찾고, 펼쳐서 용도 → 연결 대상 → 입력값 → 출력값을 확인합니다.</p><p>현재 프로젝트의 Blueprint 노출 함수·구현 이벤트·디스패처와 구조체 필드를 현재 소스에서 추출했습니다. 언리얼 엔진/플러그인의 모든 기본 노드와 에셋별 사용자 Custom Event·Get/Set 노드까지 검색하는 엔진 전체 사전은 아닙니다.</p><p class="muted">'''+e(' · '.join(f'{k} {v}개' for k,v in counts.items()))+''' · 생성 기준 2026-10-03</p></div></header>
<main class="wrap"><section class="catalogue-tools" aria-label="노드 검색 필터"><div class="guide-tools"><label for="node-search">노드 이름</label><input id="node-search" type="search" autocomplete="off" placeholder="Apply Stat Modifier, ApplyCombatDamage, On Shield Broken…"><button id="clear-search" type="button">초기화</button></div>
<div class="filter-row"><label for="search-scope">검색 범위</label><select id="search-scope"><option value="name">이름만</option><option value="all">이름 + 용도 + 핀 설명</option></select><label for="node-kind">종류</label><select id="node-kind"><option value="">전체</option>'''+''.join('<option>'+e(k)+'</option>' for k in counts)+'''</select><label for="node-category">분야</label><select id="node-category"><option value="">전체</option>'''+options+'''</select></div>
<div class="filter-row"><label><input id="show-legacy" type="checkbox">호환용·숨김 선언도 표시</label><button id="expand-all" type="button">보이는 노드 펼치기</button><button id="collapse-all" type="button">모두 접기</button></div>
<p id="search-result" class="search-result" role="status" aria-live="polite"></p></section>
<p class="guide-contract">띄어쓰기·대소문자 없이도 검색할 수 있습니다. 동일 이름은 Target 타입별로 구분합니다. 초록색 조회 / 호출 / 구현 이벤트 / Bind 디스패처를 구별하세요. 기본값 없는 핀을 0/None으로 써도 안전하다는 뜻은 아닙니다.</p>
<noscript><p class="note">JavaScript가 꺼져 있습니다. 아래 전체 목록과 브라우저 찾기(Ctrl+F)를 사용하세요.</p></noscript><div id="node-results">'''+cards+'''</div><button id="load-more" type="button" hidden>다음 40개 보기</button>
</main><footer><div class="wrap"><p>구조체 입력은 통째로 연결하거나 Split Struct Pin / Make로 작성할 수 있습니다. 읽기 전용 결과·핸들은 Break로 읽고 시스템이 발급한 원본을 보관하세요. 오래된 템플릿 노드는 Foundation 시스템과 자동 호환되지 않습니다.</p><p><a href="../../../Scripts/generate_node_search_guide.py">참고서 생성기</a> · <a href="../../Foundation/FOUNDATION_BLUEPRINT_NODE_CATALOG.md">기존 선언 목록</a> · <a href="../precautions/ACTION_LIFECYCLE_GUIDE_KO.html">액션·정리 책임</a></p></div></footer><script src="../assets/node-search.js"></script></body></html>'''
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(html,encoding='utf-8',newline='\n')
    qa=ROOT/'Saved/GuideQA/node-reference.json';qa.parent.mkdir(parents=True,exist_ok=True)
    qa.write_text(json.dumps({'counts':counts,'nodes':nodes,'structs':structs},ensure_ascii=False,indent=2),encoding='utf-8')
    print('Node reference:',dict(counts),'total',len(nodes))
    return nodes,structs

if __name__=='__main__':generate()
