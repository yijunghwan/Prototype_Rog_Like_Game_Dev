"""Content regressions for timer guidance (not a browser/runtime test)."""
from pathlib import Path
from organize_development_guides import DetailBlocks, block
from validate_development_guides import Document

ROOT = Path(__file__).resolve().parents[1]
GUIDES = ROOT / 'Docs/Guides'


def read(url):
    return (GUIDES / url).read_text(encoding='utf-8')


def main():
    manuals = [
        'nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html',
        'nsh/SKILL_RUNTIME_ACTION_GUIDE_KO.html',
        'hhc/ENEMY_ACTION_GUIDE_KO.html',
        'precautions/ACTION_LIFECYCLE_GUIDE_KO.html',
    ]
    for url in manuals:
        text = read(url)
        chapter = block(text, 'managed-timers')
        assert len([key for key in DetailBlocks(text).blocks if key == 'managed-timers']) == 1
        assert ('#managed-timers', True) in Document(GUIDES / url).links, url
        assert 'Set Action Timer by Event' in chapter
        for phrase in ['Time', 'Looping', 'Initial Start Delay', 'Max Once Per Frame',
                       'Success / Return Value', '예약 직후', '실제 취소', 'End Action',
                       '중복 예약', '이미 적용한 효과', 'Cancelled 출력이 없습니다']:
            assert phrase in chapter, (url, phrase)
        if '/hhc/' in '/' + url:
            assert 'set-item-timer' not in DetailBlocks(chapter).blocks
            assert '적 Self에는 사용할 수 없습니다' in chapter
        else:
            assert 'set-item-timer' in DetailBlocks(chapter).blocks
            assert 'On Item Unregistered 호출 전에' in chapter

    runtime = read(manuals[0])
    assert '타이머는 자동 정리 대상이 아닙니다.' not in runtime
    assert runtime.index('id="managed-timers"') < runtime.index('id="lifecycle"')
    assert '기존 일반' in runtime and '이미 자동 제거' in runtime
    skill = read(manuals[1])
    assert skill.index('id="managed-timers"') < skill.index('id="handle"')
    assert '해당 스킬의 Timer Handle Clear' not in skill
    assert 'Set Action Timer by Event' in block(skill, 'cleanup')
    lifecycle = read(manuals[3])
    assert '일반 엔진 Timer가 필요한 예외' in block(lifecycle, 'timer-cleanup')
    assert 'Set Item Timer by Event' in block(lifecycle, 'owned-resources')

    expected = {
        'nsh/ITEM_ASSET_CREATION_GUIDE_KO.html': 'runtime-timing',
        'common/OBJECT_STAT_GUIDE_KO.html': 'stat-timing',
        'common/DAMAGE_NODES_FORMULA_GUIDE_KO.html': 'damage-timing',
        'precautions/STAT_MODIFIER_LIFECYCLE_GUIDE_KO.html': 'timer-stat-lifetime',
        'hhc/ENEMY_MOVEMENT_GUIDE_KO.html': 'ai-timer-lifetime',
        'precautions/ENEMY_MOVEMENT_CHECKLIST_KO.html': 'ai-timer-lifetime',
        'environment/ENEMY_SPAWN_GUIDE_KO.html': 'spawner-timing',
        'environment/ITEM_PICKUP_SPAWN_GUIDE_KO.html': 'pickup-timing',
    }
    for url, key in expected.items():
        assert key in DetailBlocks(read(url)).blocks, url
        assert ('#' + key, True) in Document(GUIDES / url).links, url
    damage = block(read('common/DAMAGE_NODES_FORMULA_GUIDE_KO.html'), 'damage-timing')
    assert '틱마다 DOT 등록 노드를 다시 부르는 타이머를 추가하지' in damage
    assert '별도 BP 타이머가 필요 없습니다' in damage
    catalog = read('common/BLUEPRINT_NODE_SEARCH_KO.html')
    assert '../precautions/ACTION_LIFECYCLE_GUIDE_KO.html#managed-timers' in catalog
    assert '../nsh/RELIC_RUNTIME_BLUEPRINT_NODE_GUIDE_KO.html#managed-timers' in catalog
    print('PASS: timer recommendations, TOC entries, callback/cleanup rules, enemy scope, '
          'independent AI/spawner/pickup exceptions and DOT/stat lifetime boundaries.')


if __name__ == '__main__':
    main()
