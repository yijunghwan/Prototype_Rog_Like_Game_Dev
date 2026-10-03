/* Offline search: no fetch, external assets, injected HTML or storage. */
(() => {
  'use strict';
  const normal = value => String(value).normalize('NFKC').toLocaleLowerCase().replace(/[\s_.|/\-]+/g, '');
  function matches(record, state) {
    if (record.legacy && !state.legacy) return false;
    if (state.kind && record.kind !== state.kind) return false;
    if (state.category && record.category !== state.category) return false;
    const haystack = normal(state.scope === 'all' ? record.text : record.name);
    return state.query.trim().split(/\s+/).filter(Boolean).every(token => haystack.includes(normal(token)));
  }
  function rank(record, query) {
    const needle = normal(query);
    const variants = [record.label, record.name].map(normal);
    return needle && variants.some(x => x === needle) ? 0 : needle && variants.some(x => x.startsWith(needle)) ? 1 : 2;
  }
  globalThis.NodeReferenceSearch = { normal, matches, rank };
  if (typeof document === 'undefined') return; // Pure tests use the same functions.
  const ids = id => document.getElementById(id);
  const search=ids('node-search'), scope=ids('search-scope'), kind=ids('node-kind'), category=ids('node-category'), legacy=ids('show-legacy');
  const result=ids('search-result'), more=ids('load-more'), container=ids('node-results');
  const records=Array.from(container.querySelectorAll('.node-entry')).map((el,index)=>({
    el,index,name:el.dataset.name,label:el.querySelector('strong').textContent,
    text:el.textContent,kind:el.dataset.kind,category:el.dataset.category,legacy:el.dataset.legacy==='true'
  }));
  let limit=40, selected=[];
  const state=()=>({query:search.value,scope:scope.value,kind:kind.value,category:category.value,legacy:legacy.checked});
  function paint() {
    const visible=new Set(selected.slice(0,limit));
    records.forEach(record=>record.el.hidden=!visible.has(record));
    result.textContent=selected.length ? `${selected.length}개 검색 결과 · ${Math.min(limit,selected.length)}개 표시 / 전체 ${records.length}개` : '검색 결과가 없습니다. 이름을 줄이거나 검색 범위를 「이름 + 용도 + 핀 설명」으로 바꿔보세요.';
    more.hidden=selected.length<=limit;
  }
  function filter() {
    const current=state();limit=40;
    selected=records.filter(record=>matches(record,current)).sort((a,b)=>rank(a,current.query)-rank(b,current.query)||a.index-b.index);
    selected.forEach(record=>container.appendChild(record.el));
    paint();
    // Do not expand hundreds of hits. A unique/exact hit is ready to read.
    if (current.query.trim()) selected.slice(0,3).filter(r=>selected.length===1 || rank(r,current.query)===0).forEach(r=>r.el.open=true);
  }
  [search,scope,kind,category,legacy].forEach(el=>el.addEventListener(el===search?'input':'change',filter));
  more.addEventListener('click',()=>{limit+=40;paint();});
  ids('clear-search').addEventListener('click',()=>{search.value='';scope.value='name';kind.value='';category.value='';legacy.checked=false;records.forEach(r=>r.el.open=false);filter();search.focus();});
  ids('expand-all').addEventListener('click',()=>records.filter(r=>!r.el.hidden).forEach(r=>r.el.open=true));
  ids('collapse-all').addEventListener('click',()=>container.querySelectorAll('details').forEach(el=>el.open=false));
  function revealHash() {
    let target;try {target=ids(decodeURIComponent(location.hash.slice(1)));} catch {return;}
    if (!target?.classList.contains('node-entry')) return;
    search.value='';kind.value='';category.value='';legacy.checked=true;filter();
    const at=selected.findIndex(r=>r.el===target);limit=Math.max(limit,at+1);paint();
    target.open=true;target.scrollIntoView({block:'start'});
  }
  window.addEventListener('hashchange',revealHash);
  let printState;
  window.addEventListener('beforeprint',()=>{printState=Array.from(container.querySelectorAll('details')).map(el=>[el,el.open,el.hidden]);selected.forEach(r=>r.el.hidden=false);selected.forEach(r=>r.el.querySelectorAll('details').forEach(el=>el.open=true));selected.forEach(r=>r.el.open=true);});
  window.addEventListener('afterprint',()=>{printState?.forEach(([el,open,hidden])=>{el.open=open;el.hidden=hidden;});});
  filter();if(location.hash)revealHash();
})();
