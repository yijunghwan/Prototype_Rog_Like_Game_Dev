"""Content checks for the team's control reference; does not alter input assets."""
from html import escape
from pathlib import Path

from organize_development_guides import DetailBlocks, HOME_NAME, block
from validate_development_guides import Document


def main():
    guides = Path(__file__).resolve().parents[1] / 'Docs/Guides'
    path = guides / 'common/BASIC_CONTROLS_GUIDE_KO.html'
    text = path.read_text(encoding='utf-8')
    keys = block(text, 'keys')
    for name, key in [('이동', 'W / A / S / D'), ('구르기 · 회피', 'Space'),
                      ('인벤토리', 'I'), ('액티브 유물 1번 슬롯', 'E'),
                      ('액티브 유물 2번 슬롯', 'R'), ('소모품 1번 슬롯', '1'),
                      ('소모품 2번 슬롯', '2'), ('소모품 3번 슬롯', '3'),
                      ('상호작용', 'F'), ('무기 1번 스킬', 'Shift'),
                      ('무기 2번 스킬', 'Q'), ('무기 3번 스킬', 'X')]:
        assert f'<th scope="row">{escape(name)}</th><td><code>{escape(key)}</code>' in keys, (name, key)
    assert keys.startswith('<details class="chapter" id="keys" open>')
    assignment = block(text, 'assignment')
    for phrase in ['중복 지정', '충돌에 주의', '<code>Z</code>', '<code>C</code>',
                   '기존 동작이 자동으로 대체', '별개의 상호작용이나 슬롯 입력']:
        assert phrase in assignment, phrase
    for phrase in ['키를 추가하거나 변경', '건의', '이정환에게 문의']:
        assert phrase in block(text, 'contact'), phrase
    assert '실제 입력 설정이나 게임 에셋이 변경되지는 않습니다' in text
    doc = Document(path)
    for key in ['keys', 'assignment', 'contact']:
        assert ('#' + key, True) in doc.links
        assert list(DetailBlocks(text).blocks).count(key) == 1
    home = (guides / HOME_NAME).read_text(encoding='utf-8')
    assert 'common/BASIC_CONTROLS_GUIDE_KO.html#keys' in home
    assert '이정환에게 문의' in home
    print('PASS: basic controls, slot mappings, duplicate-key warning, free keys, contact and portal link.')


if __name__ == '__main__':
    main()
