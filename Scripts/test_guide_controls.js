/* Non-rendering unit tests: shared controls operate on a small DOM-shaped model.
 * This is not a browser/visual test and does not navigate to local files. */
'use strict';
const fs = require('fs');
const vm = require('vm');
const assert = require('assert/strict');
const path = require('path');
const root = path.resolve(__dirname, '..');
class Element {
  constructor(tag, text = '') { this.tagName=tag;this.label=text;this.children=[];this.childNodes=[];this.parentElement=null;this.open=false;this.hidden=false;this.nodeType=1;this.events={}; }
  get textContent() { return this.label + this.children.map(el=>el.textContent).join(' '); }
  set textContent(value) { this.label=value; }
  add(child) { this.children.push(child);this.childNodes.push(child);child.parentElement=this;return child; }
  addEventListener(name,fn) { this.events[name]=fn; }
  matches(selector) { return selector==='details.node' && this.kind==='node'; }
  querySelectorAll(selector) {
    if (selector.startsWith(':scope')) return this.children.flatMap(el=>el.children).filter(el=>el.kind==='node');
    if (selector==='details') return this.children.flatMap(el=>[...(el.tagName==='DETAILS'?[el]:[]),...el.querySelectorAll('details')]);
    throw Error('Unexpected selector '+selector);
  }
  scrollIntoView() { this.scrolled=true; }
  focus() { this.focused=true; }
}
const main=new Element('MAIN');
function makeChapter(title, description, nodes) {
  const ch=main.add(new Element('DETAILS'));ch.kind='chapter';
  ch.add(new Element('SUMMARY',title));const body=ch.add(new Element('DIV',description));
  nodes.forEach(([id,title,bodyText])=>{const n=body.add(new Element('DETAILS'));n.kind='node';n.id=id;n.add(new Element('SUMMARY',title));n.add(new Element('DIV',bodyText));});
  return ch;
}
const cc=makeChapter('CC', '상태 적용', [['cc-node','Apply Crowd Control','강인함은 지속시간에 적용됩니다.']]);
const act=makeChapter('액션','타이머 정리 책임',[['timer-node','Clear and Invalidate Timer by Handle','취소된 실행의 별도 타이머를 정리합니다.']]);
cc.open=true;
const details=()=>main.querySelectorAll('details');
const controls=Object.fromEntries(['node-search','search-result','clear-search','expand-all','collapse-all'].map(id=>[id,new Element('BUTTON')]));
controls['node-search'].value='';
const windowEvents={};
const context={document:{body:{hasAttribute:()=>true},getElementById:id=>controls[id]||details().find(el=>el.id===id),querySelectorAll:selector=>selector==='main details.chapter'?[cc,act]:details()},
  location:{hash:''},window:{addEventListener:(name,fn)=>windowEvents[name]=fn},requestAnimationFrame:fn=>fn()};
vm.runInNewContext(fs.readFileSync(path.join(root,'Docs/Guides/assets/guide.js'),'utf8'),context);
const input=controls['node-search'];
input.value='apply crowd';input.events.input();
assert.equal(cc.hidden,false);assert.equal(act.hidden,true);assert.equal(cc.children[1].children[0].open,true);
assert.match(controls['search-result'].textContent,/1개/);
input.value='없는 검색어';input.events.input();
assert.equal(cc.hidden,true);assert.equal(act.hidden,true);assert.match(controls['search-result'].textContent,/검색 결과가 없습니다/);
controls['clear-search'].events.click();
assert.equal(cc.hidden,false);assert.equal(act.hidden,false);assert.equal(cc.open,true);assert.equal(act.open,false);assert.equal(input.focused,true);
controls['expand-all'].events.click();assert(details().every(el=>el.open));
controls['collapse-all'].events.click();assert(details().every(el=>!el.open));
context.location.hash='#timer-node';windowEvents.hashchange();
assert.equal(act.open,true);assert.equal(act.children[1].children[0].open,true);assert.equal(act.children[1].children[0].scrolled,true);
windowEvents.beforeprint();assert(details().every(el=>el.open&&!el.hidden));
windowEvents.afterprint();assert.equal(cc.open,false);assert.equal(act.open,true);
const report=JSON.parse(fs.readFileSync(path.join(root,'Saved/GuideQA/inline-scripts.json'),'utf8'));
for(const {file,script} of report) new vm.Script(script,{filename:file});
console.log(`PASS: search (including closed content), no results, reset/restore, expand/collapse, hash reveal, print restore; ${report.length+1} script syntax checks.`);
