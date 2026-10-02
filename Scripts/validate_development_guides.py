"""Static checks for the offline development reference portal."""
from collections import Counter
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit, unquote
import json
import sys

ROOT=Path(__file__).resolve().parents[1]
GUIDES=ROOT/'Docs/Guides'
VOID={'area','base','br','col','embed','hr','img','input','link','meta','param','source','track','wbr'}

class Document(HTMLParser):
    def __init__(self,path):
        super().__init__(convert_charrefs=True)
        self.path=path;self.stack=[];self.ids=[];self.links=[];self.errors=[]
        self.chapters=0;self.nodes=0;self.portals=0;self.scripts=[];self.current_script=None
        self.shared=False
        self.feed(path.read_text(encoding='utf-8'))
        if self.stack:self.errors.append(f'unclosed: {self.stack}')
        for key,count in Counter(self.ids).items():
            if count>1:self.errors.append(f'duplicate id: {key}')
    def handle_starttag(self,tag,attrs):
        a=dict(attrs);classes=a.get('class','').split()
        if a.get('id'):self.ids.append(a['id'])
        if tag=='a' and a.get('href') is not None:self.links.append((a['href'],True))
        if tag=='link' and a.get('href'):self.links.append((a['href'],False))
        if tag=='script':
            if a.get('src'):self.links.append((a['src'],False))
            else:self.current_script=''
        self.chapters+='chapter' in classes;self.nodes+='node' in classes
        self.portals+='guide-nav' in classes
        if tag=='body':self.shared='data-guide-ui' in a
        if tag not in VOID:self.stack.append(tag)
    def handle_endtag(self,tag):
        if tag=='script' and self.current_script is not None:
            self.scripts.append(self.current_script);self.current_script=None
        if not self.stack or self.stack[-1]!=tag:self.errors.append(f'{self.getpos()}: unexpected closing {tag}')
        else:self.stack.pop()
    def handle_data(self,data):
        if self.current_script is not None:self.current_script+=data

def main():
    files=sorted(GUIDES.rglob('*.html'))
    docs={path.resolve():Document(path) for path in files}
    failures=[];link_count=0
    for path,d in docs.items():
        failures.extend(f'{path.relative_to(GUIDES)}: {error}' for error in d.errors)
        legacy=path.parent==GUIDES and path.name in ('OBJECT_STAT_GUIDE_KO.html','DAMAGE_NODES_FORMULA_GUIDE_KO.html')
        if not legacy and d.portals!=1:failures.append(f'{path.name}: {d.portals} global navigation bars, expected one')
        for href,anchor in d.links:
            u=urlsplit(href)
            if u.scheme or u.netloc:continue
            target=(path.parent/unquote(u.path)).resolve() if u.path else path
            link_count+=1
            if not target.is_file():failures.append(f'{path.name}: missing {href}');continue
            if anchor and u.fragment:
                target_doc=docs.get(target)
                if target_doc is None and target.suffix=='.html':target_doc=Document(target)
                if target_doc and unquote(u.fragment) not in target_doc.ids:failures.append(f'{path.name}: missing anchor {href}')
        if d.shared and not legacy:
            if not {'node-search','search-result','expand-all','collapse-all','clear-search'}.issubset(d.ids):
                failures.append(f'{path.name}: missing shared search controls')
        print(f'{path.relative_to(GUIDES)}: chapters={d.chapters}, foldouts={d.nodes}, links={len(d.links)}')
    if '--js-json' in sys.argv:
        dest=ROOT/'Saved/GuideQA/inline-scripts.json'
        dest.parent.mkdir(parents=True,exist_ok=True)
        dest.write_text(json.dumps([{'file':str(path),'script':script} for path,d in docs.items() for script in d.scripts],ensure_ascii=False),encoding='utf-8')
    if failures:
        print('\n'.join(failures));return 1
    print(f'PASS: {len(files)} HTML files, {link_count} local links/assets, unique IDs and balanced markup.')
    return 0

if __name__=='__main__':raise SystemExit(main())
