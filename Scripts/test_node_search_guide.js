/* Pure offline search tests, not a browser or rendering test. */
'use strict';
const fs=require('fs'),vm=require('vm'),assert=require('assert/strict'),path=require('path');
const root=path.resolve(__dirname,'..');
const context={};vm.createContext(context);
vm.runInContext(fs.readFileSync(path.join(root,'Docs/Guides/assets/node-search.js'),'utf8'),context);
const {matches,rank,normal}=context.NodeReferenceSearch;
const base={query:'',scope:'name',kind:'',category:'',legacy:false};
const node={name:'Apply Stat Modifier ApplyStatModifier',label:'Apply Stat Modifier',text:'Apply Stat Modifier 영구 성장 Permanent Flat 수치 조정',kind:'호출',category:'AR|Stats',legacy:false};
assert(matches(node,{...base,query:'applystatmodifier'}));
assert(matches(node,{...base,query:'APPLY STAT MODIFIER'}));
assert(matches(node,{...base,query:'Apply_Stat-Modifier'}));
assert(!matches(node,{...base,query:'Permanent Flat'}));
assert(matches(node,{...base,query:'Permanent Flat',scope:'all'}));
assert(!matches(node,{...base,kind:'조회'}));
assert(!matches(node,{...base,category:'AR|Combat'}));
assert(!matches({...node,legacy:true},base));
assert(matches({...node,legacy:true},{...base,legacy:true}));
assert(!matches(node,{...base,query:'Stat Absent'}));
assert.equal(rank(node,'Apply Stat Modifier'),0);
assert.equal(normal('Ａｐｐｌｙ Stat_Modifier'),'applystatmodifier');
const data=JSON.parse(fs.readFileSync(path.join(root,'Saved/GuideQA/node-reference.json'),'utf8'));
assert.equal(new Set(data.nodes.map(n=>n.id)).size,data.nodes.length);
for(const [owner,name] of [['UARCombatBlueprintLibrary','ApplyCrowdControl'],['UARStatsBlueprintLibrary','RemoveStatModifierStacks'],['UARLoadoutItemInstance','ReceiveItemSkillCancelled'],['AARAIController','ARMoveToActor']])assert(data.nodes.some(n=>n.owner===owner&&n.name===name));
assert(data.nodes.filter(n=>n.name==='RestoreHealth').length>=2);
const damage=data.structs.FARCombatDamageRequest.fields;
assert(damage.some(p=>p.name==='DamageNameInterval'&&p.default==='0.2f'));
assert(damage.some(p=>p.name==='bIgnoreDamageNameInterval'));
const cc=data.nodes.find(n=>n.name==='ApplyCrowdControl');
assert(cc.inputs.some(p=>p.name==='Duration'&&p.default==='1.0'));
assert(cc.outputs.some(p=>p.name==='AppliedDuration'));
assert(!cc.inputs.some(p=>p.name==='bSuccess'));
const onItem=data.nodes.find(n=>n.name==='ReceiveItemSkillCancelled');
assert.equal(onItem.inputs.length,0);assert(onItem.outputs.some(p=>p.name==='SkillId'));
const can=data.nodes.find(n=>n.name==='CanExecuteItemSkill');
assert(can.inputs.some(p=>p.name==='SkillId'));assert(can.outputs.some(p=>p.name==='FailureTag'));
assert(data.nodes.find(n=>n.name==='RemoveOneStatModifierStack').hidden);
for (const name of ['SetGroupedItemTimerByEvent','SetGroupedActionTimerByEvent']) {
  const n=data.nodes.find(n=>n.name===name);assert(n&&!n.hidden);
  for(const [key,value] of [['TimeGroup','EARTimeGroup::World'],['InitialStartDelay','0.0f'],['InitialStartDelayVariance','0.0f'],['bMaxOncePerFrame','false']]) {
    assert.equal(n.inputs.find(p=>p.name===key).default,value);
  }
}
for(const name of ['SetItemTimerByEvent','SetActionTimerByEvent'])assert(!data.nodes.some(n=>n.name===name));
for(const name of ['SetTimeGroupRate','PauseTimeGroupTimer','UnpauseTimeGroupTimer','ClearTimeGroupTimer','GetTimeGroupTimerRemaining'])assert(data.nodes.find(n=>n.name===name&&!n.hidden));

// Event wiring and view-state tests with a DOM-shaped model. No rendering.
class Element {
  constructor() {this.events={};this.value='';this.checked=false;this.open=false;this.hidden=false;this.textContent='';this.children=[];this.dataset={};}
  addEventListener(name,fn){this.events[name]=fn;}
  focus(){this.focused=true;}
  scrollIntoView(){this.scrolled=true;}
  appendChild(el){this.children=this.children.filter(x=>x!==el);this.children.push(el);}
  querySelector(selector){if(selector==='strong')return {textContent:this.label};throw Error(selector);}
  querySelectorAll(selector){
    if(selector==='.node-entry')return this.children;
    if(selector==='details')return this.children.flatMap(x=>[x,...x.querySelectorAll('details')]);
    throw Error(selector);
  }
}
const controls={};
for(const id of ['node-search','search-scope','node-kind','node-category','show-legacy','search-result','load-more','node-results','clear-search','expand-all','collapse-all'])controls[id]=new Element();
controls['search-scope'].value='name';
const cards=[];
for(let i=0;i<51;i++){
  const el=new Element();el.id='node-test-'+i;el.label='Test Node '+i;el.textContent=el.label+' 비용 Cost';
  el.dataset={name:el.label,kind:'호출',category:'AR|Test',legacy:i===50?'true':'false'};
  el.classList={contains:name=>name==='node-entry'};
  el.children=[new Element()];el.children[0].textContent='nested struct';
  cards.push(el);controls[el.id]=el;
}
controls['node-results'].children=[...cards];
const win={events:{},addEventListener(name,fn){this.events[name]=fn;}};
const domContext={document:{getElementById:id=>controls[id]},window:win,location:{hash:''}};
vm.createContext(domContext);vm.runInContext(fs.readFileSync(path.join(root,'Docs/Guides/assets/node-search.js'),'utf8'),domContext);
const visible=()=>cards.filter(x=>!x.hidden).length;
assert.equal(visible(),40);assert.equal(controls['load-more'].hidden,false);
controls['load-more'].events.click();assert.equal(visible(),50);assert.equal(cards[50].hidden,true);
controls['node-search'].value='TestNode49';controls['node-search'].events.input();
assert.equal(visible(),1);assert(cards[49].open);
controls['node-search'].value='Absent';controls['node-search'].events.input();assert.equal(visible(),0);assert(controls['search-result'].textContent.includes('없습니다'));
controls['clear-search'].events.click();assert.equal(visible(),40);assert.equal(cards[49].open,false);assert(controls['node-search'].focused);
controls['expand-all'].events.click();assert(cards.slice(0,40).every(c=>c.open));assert(!cards[40].open);
controls['collapse-all'].events.click();assert(cards.every(c=>!c.open));
domContext.location.hash='#node-test-50';win.events.hashchange();assert.equal(visible(),51);assert(cards[50].open&&cards[50].scrolled);
controls['node-search'].value='TestNode49';controls['node-search'].events.input();
const before=cards.map(c=>[c.open,c.hidden,c.children[0].open]);
win.events.beforeprint();assert(cards[49].children[0].open);win.events.afterprint();
assert.deepEqual(cards.map(c=>[c.open,c.hidden,c.children[0].open]),before);
controls['clear-search'].events.click();assert.equal(visible(),40);assert.equal(controls['show-legacy'].checked,false);
console.log(`PASS: ${data.nodes.length} unique records, search/filter/legacy, current pins/defaults, event/override directions; controls, pagination, hash reveal, print restore.`);
