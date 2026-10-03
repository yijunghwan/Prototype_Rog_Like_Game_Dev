"""Reference-content regressions; no Blueprint or gameplay mutation."""
from pathlib import Path
import re

from organize_development_guides import HOME_NAME, block
from validate_development_guides import Document

ROOT = Path(__file__).resolve().parents[1]
GUIDES = ROOT / 'Docs/Guides'


def main():
    path = GUIDES / 'hhc/ENEMY_DETAILS_GUIDE_KO.html'
    text = path.read_text(encoding='utf-8')
    for key in ['where', 'team-settings', 'ai-settings', 'stat-settings', 'health-settings',
                'groggy-settings', 'stagger-settings', 'movement-settings',
                'collision-settings', 'tick-settings', 'check']:
        assert ('#' + key, True) in Document(path).links, key
        assert block(text, key), key
    assert '설정·내용 찾기' in text
    assert '노드의 용도 → 연결 대상' not in text
    assert 'EditDefaultsOnly' in block(text, 'where')
    team = block(text, 'combat-team')
    for phrase in ['Player', 'Enemy', 'Environment', 'Same Team', '피해 대상에서 제외',
                   '조종은 Possess/Controller', '충돌의 차이']:
        assert phrase in team, phrase
    for phrase in ['Disabled', 'Placed in World', 'Spawned', 'Placed in World or Spawned']:
        assert phrase in block(text, 'auto-possess-ai'), phrase
    for phrase in ['기본 false', 'Can Be Staggered와는 독립', '체크만으로 완성되지 않음']:
        assert phrase in block(text, 'use-groggy-gauge'), phrase
    groggy = block(text, 'groggy-settings')
    for phrase in ['Max Groggy', 'Groggy Recovery Delay', 'Groggy Recovery Per Second',
                   '자동으로 기절하지 않습니다', '자연 회복과 추가 그로기 차감을 멈추며',
                   'Reset Groggy Gauge', 'New Current Value=-1', 'CC 면역', 'HP 피해만으로']:
        assert phrase in groggy, phrase
    assert '기절 시간 설정이 아닙니다' in block(text, 'groggy-stats')
    assert '최종 스탯으로 다시 설정' in block(text, 'walk-speed')
    assert '캡슐 중심' in block(text, 'plane-settings')
    assert '컴포넌트 Tick을 일괄 꺼 버리지' in block(text, 'actor-tick')
    assert 'Base Stats와 보정으로 결정' in block(text, 'stat-caps')
    assert 'AI Controller Class' in text and 'Damage System Enabled' in text
    assert 'Stun 면역' in block(text, 'immune-status-tags')
    overview = (GUIDES / 'hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html').read_text(encoding='utf-8')
    assert 'ENEMY_DETAILS_GUIDE_KO.html#where' in block(overview, 'setup')
    assert 'ENEMY_DETAILS_GUIDE_KO.html#groggy-settings' in block(overview, 'groggy')
    assert 'hhc/ENEMY_DETAILS_GUIDE_KO.html' in (GUIDES / HOME_NAME).read_text(encoding='utf-8')

    # Enemy authors consume map navigation; do not reintroduce map-building steps.
    movement = (GUIDES / 'hhc/ENEMY_MOVEMENT_GUIDE_KO.html').read_text(encoding='utf-8')
    navmesh = block(movement, 'navmesh')
    for phrase in ['맵 제작자가 담당', '맵 제작자에게 문의', 'NavMesh가 없으면',
                   'Request Basic Move', '직선 이동']:
        assert phrase in navmesh, phrase
    assert '<ol>' not in navmesh
    for guide in ['hhc/ENEMY_MOVEMENT_GUIDE_KO.html',
                  'hhc/ENEMY_BLUEPRINT_CREATION_GUIDE_KO.html',
                  'hhc/ENEMY_DETAILS_GUIDE_KO.html',
                  'precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html',
                  'environment/ENEMY_SPAWN_GUIDE_KO.html', HOME_NAME]:
        guide_text = (GUIDES / guide).read_text(encoding='utf-8')
        for obsolete in ['NavMesh 만들기', 'NavMesh 만드는 법', 'NavMesh 생성 단계',
                         'Place Actors', 'Navigation 빌드/자동 갱신']:
            assert obsolete not in guide_text, (guide, obsolete)
        if guide != HOME_NAME:
            assert '맵 제작자에게 문의' in guide_text, guide

    # Fail on source drift so the displayed native defaults are reviewed too.
    public = ROOT / 'Source/Action_RogueLike/Public/Foundation'
    private = ROOT / 'Source/Action_RogueLike/Private/Foundation'
    stagger = (public / 'Components/ARStaggerComponent.h').read_text(encoding='utf-8')
    for expression in ['bCanBeStaggered = true', 'BaseStaggerDuration = 0.2f',
                       'PostStaggerImmunityDuration = 0.1f', 'bUseGroggyGauge = false']:
        assert expression in stagger, expression
    stats = (public / 'Components/ARStatsComponent.h').read_text(encoding='utf-8')
    for expression in ['MaxEvasion = 50.0f', 'MaxCriticalChance = 100.0f',
                       'MaxTenacity = 100.0f', 'MaxCooldownReduction = 40.0f']:
        assert expression in stats, expression
    base = (public / 'Characters/ARBaseCharacter.h').read_text(encoding='utf-8')
    assert re.search(r'UPROPERTY\(EditDefaultsOnly[^\n]+\) EARCombatTeam CombatTeam', base)
    enemy = (private / 'Characters/ARBaseEnemy.cpp').read_text(encoding='utf-8')
    for expression in ['CombatTeam = EARCombatTeam::Enemy',
                       'AIControllerClass = AARAIController::StaticClass()',
                       'AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned']:
        assert expression in enemy, expression
    print('PASS: inherited Details, native defaults, combat teams, groggy setup/lock/recovery, '
          'movement/collision/tick boundaries and guide links.')


if __name__ == '__main__':
    main()
