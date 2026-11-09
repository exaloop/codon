const fs = require('node:fs/promises');
const path = require('node:path');
const {load} = require('cheerio');
const Turndown = require('turndown');
const {gfm} = require('turndown-plugin-gfm');

function markdownPage(html, pageUrl) {
  const document = load(html);
  const article = document('article').first().clone();
  if (!article.length) return null;

  article.find('.codon-pipeline').each((index, element) => {
    const diagram = document(element);
    const stages = document('<ol/>');
    diagram.find('[data-pipeline-label]').each((stageIndex, label) => {
      const link = document(label);
      const stage = document('<li/>');
      stage.append(document('<a/>').attr('href', link.attr('href')).text(link.find('strong').text()));
      const kind = link.attr('data-pipeline-label');
      const description = diagram.find('[data-pipeline-info]').filter((infoIndex, info) => document(info).attr('data-pipeline-info') === kind).text();
      stage.append(document('<p/>').text(description));
      stages.append(stage);
    });
    diagram.replaceWith(stages);
  });
  article.find('[role="tabpanel"]').each((index, element) => {
    const panel = document(element);
    const labelId = panel.attr('aria-labelledby');
    const tabs = panel.parent().prevAll('[role="tablist"]').first().find('[role="tab"]');
    const label = (labelId
      ? article.find('[role="tab"]').filter((tabIndex, tab) => document(tab).attr('id') === labelId)
      : tabs.eq(panel.parent().children('[role="tabpanel"]').index(element))).text().trim();
    panel.removeAttr('hidden');
    if (label) panel.prepend(document('<h3/>').text(label));
  });
  article.find('script, style, button, svg, [hidden], [role="tablist"], .hash-link, .doc-page-actions, .theme-doc-footer, .theme-doc-breadcrumbs, .api-breadcrumbs').remove();

  article.find('pre').each((index, element) => {
    const block = document(element);
    block.find('[class*="codeLineNumber"]').remove();
    const lines = block.find('.token-line');
    const code = lines.length ? lines.map((lineIndex, line) => document(line).text()).get().join('\n') : block.text().replace(/\n$/, '');
    const classes = `${block.attr('class') || ''} ${block.closest('.theme-code-block').attr('class') || ''} ${block.find('code').attr('class') || ''}`;
    block.attr('data-code', code);
    block.attr('data-language', classes.match(/\blanguage-([\w-]+)/)?.[1] || 'text');
    block.attr('data-title', block.closest('.theme-code-block').find('.code-example__title, [class*="codeBlockTitle"]').first().text());
  });
  article.find('.code-example__header, [class*="codeBlockTitle"]').remove();
  article.find('a[href], img[src]').each((index, element) => {
    const attribute = element.tagName === 'img' ? 'src' : 'href';
    const value = document(element).attr(attribute);
    if (value) document(element).attr(attribute, new URL(value, pageUrl).href);
  });

  const heading = article.find('h1').first();
  const title = heading.text().trim() || heading.attr('aria-label') || document('title').text().trim();
  if (title) heading.text(title);
  const converter = new Turndown({headingStyle: 'atx', codeBlockStyle: 'fenced', bulletListMarker: '-'});
  converter.use(gfm);
  converter.addRule('codeExample', {
    filter: 'pre',
    replacement(content, node) {
      const code = node.getAttribute('data-code') || '';
      const longest = Math.max(2, ...(code.match(/`+/g) || []).map(value => value.length));
      const fence = '`'.repeat(longest + 1);
      const filename = node.getAttribute('data-title');
      return `\n\n${fence}${node.getAttribute('data-language')}${filename ? ` title=${JSON.stringify(filename)}` : ''}\n${code}\n${fence}\n\n`;
    },
  });
  converter.addRule('math', {
    filter: node => node.classList?.contains('katex-display') || node.classList?.contains('katex'),
    replacement(content, node) {
      const formula = node.querySelector('annotation[encoding="application/x-tex"]')?.textContent;
      if (!formula) return content;
      return node.classList.contains('katex-display') ? `\n\n$$\n${formula}\n$$\n\n` : `$${formula}$`;
    },
  });
  return {title, markdown: `${converter.turndown(document.html(article)).trim()}\n\nSource: ${pageUrl}\n`};
}

function outputPath(outDir, baseUrl, route) {
  if (!route.startsWith(baseUrl)) throw new Error(`Route outside base URL: ${route}`);
  const relative = decodeURIComponent(route.slice(baseUrl.length));
  if (relative.split(/[\\/]/).includes('..')) throw new Error(`Unsafe route: ${route}`);
  return path.join(outDir, relative, 'index.md');
}

module.exports = function markdownPages() {
  return {
    name: 'markdown-pages',
    async postBuild({outDir, siteConfig, routesPaths}) {
      let count = 0;
      for (const route of [...new Set(routesPaths)].sort()) {
        if (route.endsWith('.html')) continue;
        const destination = outputPath(outDir, siteConfig.baseUrl, route);
        let html;
        try { html = await fs.readFile(destination.replace(/index\.md$/, 'index.html'), 'utf8'); }
        catch (error) { if (error.code === 'ENOENT') continue; throw error; }
        const page = markdownPage(html, new URL(route, siteConfig.url).href);
        if (!page) continue;
        await fs.writeFile(destination, page.markdown);
        count++;
      }
      if (!count) throw new Error('No documentation pages exported');
      console.log(`[markdown-pages] Exported ${count} Markdown pages.`);
    },
  };
};
module.exports.markdownPage = markdownPage;
module.exports.outputPath = outputPath;
