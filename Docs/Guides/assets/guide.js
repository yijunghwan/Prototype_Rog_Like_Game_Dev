/* Interactions for new reference pages; legacy pages retain their own search logic. */
(() => {
  'use strict';
  if (!document.body.hasAttribute('data-guide-ui')) return;
  const chapters = [...document.querySelectorAll('main details.chapter')];
  const input = document.getElementById('node-search');
  const result = document.getElementById('search-result');
  let searchSnapshot = null;
  const allDetails = () => [...document.querySelectorAll('main details')];
  const normal = s => s.normalize('NFKC').toLocaleLowerCase().replace(/\s+/g, ' ').trim();
  function revealHash() {
    let target;
    try { target = document.getElementById(decodeURIComponent(location.hash.slice(1))); } catch (_) { return; }
    if (!target) return;
    if (input && input.value) { input.value = ''; filter(); }
    for (let el = target; el; el = el.parentElement) {
      if (el.tagName === 'DETAILS') el.open = true;
      el.hidden = false;
    }
    requestAnimationFrame(() => target.scrollIntoView({block:'start'}));
  }
  function filter() {
    const query = normal(input ? input.value : '');
    if (query && !searchSnapshot) searchSnapshot = allDetails().map(el => [el, el.open]);
    let matches = 0;
    chapters.forEach(chapter => {
      const nodes = [...chapter.querySelectorAll(':scope > .chapter-body > details.node, :scope > .chapter-content > details.node')];
      const directText = [...chapter.children].filter(el => el.tagName !== 'DETAILS')
        .map(el => el.tagName === 'SUMMARY' ? el.textContent : [...el.childNodes]
          .filter(n => !(n.nodeType === 1 && n.matches('details.node'))).map(n => n.textContent).join(' ')).join(' ');
      const headerMatch = query && normal(directText).includes(query);
      let visible = 0;
      nodes.forEach(node => {
        const hit = !query || headerMatch || normal(node.textContent).includes(query);
        node.hidden = !hit;
        if (hit) { visible++; if (query) { node.open = true; node.querySelectorAll('details').forEach(el => el.open = true); } }
      });
      const hit = !query || headerMatch || visible > 0 || (!nodes.length && normal(chapter.textContent).includes(query));
      chapter.hidden = !hit;
      if (hit) { matches++; if (query) chapter.open = true; }
    });
    if (!query && searchSnapshot) { searchSnapshot.forEach(([el, open]) => el.open = open); searchSnapshot = null; }
    if (result) result.textContent = query ? (matches ? `${matches}개 항목에서 찾았습니다.` : '검색 결과가 없습니다. 검색어를 줄이거나 지워보세요.') : '필요한 항목만 펼쳐 보세요. 검색은 접힌 설명까지 확인합니다.';
  }
  input?.addEventListener('input', filter);
  document.getElementById('clear-search')?.addEventListener('click', () => {input.value = ''; filter(); input.focus();});
  document.getElementById('expand-all')?.addEventListener('click', () => allDetails().filter(el => !el.hidden).forEach(el => el.open = true));
  document.getElementById('collapse-all')?.addEventListener('click', () => allDetails().forEach(el => el.open = false));
  window.addEventListener('hashchange', revealHash);
  let printSnapshot;
  window.addEventListener('beforeprint', () => { printSnapshot = allDetails().map(el => [el,el.open,el.hidden]); allDetails().forEach(el => {el.open=true;el.hidden=false;}); });
  window.addEventListener('afterprint', () => printSnapshot?.forEach(([el,open,hidden]) => {el.open=open;el.hidden=hidden;}));
  filter();
  if (location.hash) revealHash();
})();
