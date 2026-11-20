import React, {useEffect, useRef, useState} from 'react';
import {useDoc} from '@docusaurus/plugin-content-docs/client';
import Head from '@docusaurus/Head';
import {Check, Copy, Download, Link2, Pencil} from 'lucide';

function ActionIcon({icon}) {
  return (
    <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor"
      strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true" focusable="false">
      {icon.map(([tag, attributes], index) => React.createElement(tag, {...attributes, key: index}))}
    </svg>
  );
}

export default function DocumentationActions() {
  const {metadata} = useDoc();
  const [state, setState] = useState('idle');
  const copyGeneration = useRef(0);
  const markdownUrl = `${metadata.permalink.replace(/\/$/, '')}/index.md`;
  useEffect(() => () => { copyGeneration.current++; }, []);

  async function copyPage(format) {
    const generation = ++copyGeneration.current;
    setState('copying');
    try {
      let text = new URL(metadata.permalink, window.location.origin).href;
      if (format === 'markdown') {
        const response = await fetch(markdownUrl);
        if (!response.ok || response.headers.get('content-type')?.includes('text/html')) {
          throw new Error('Markdown unavailable');
        }
        text = await response.text();
      }
      if (generation !== copyGeneration.current) return;
      await navigator.clipboard.writeText(text);
      if (generation === copyGeneration.current) setState(format === 'markdown' ? 'copied' : 'link-copied');
    } catch {
      if (generation === copyGeneration.current) setState('error');
    }
  }

  return <>
    <Head><link rel="alternate" type="text/markdown" href={markdownUrl} /></Head>
    <div className="doc-page-actions" role="group" aria-label="Page actions">
      <span className="doc-page-actions__label">Markdown</span>
      <button type="button" onClick={() => copyPage('markdown')} disabled={state === 'copying'}
        aria-label="Copy page as Markdown" title="Copy page as Markdown">
        <ActionIcon icon={state === 'copied' ? Check : Copy} />
      </button>
      <a href={markdownUrl} download={`${metadata.id.replace(/\//g, '-')}.md`}
        aria-label="Download page as Markdown" title="Download page as Markdown"><ActionIcon icon={Download} /></a>
      <button type="button" onClick={() => copyPage('link')} disabled={state === 'copying'}
        aria-label="Copy page link" title="Copy page link"><ActionIcon icon={state === 'link-copied' ? Check : Link2} /></button>
      {metadata.editUrl && <a href={metadata.editUrl} target="_blank" rel="noopener noreferrer"
        aria-label="Edit this page" title="Edit this page"><ActionIcon icon={Pencil} /></a>}
      <span className="doc-page-actions__status" role="status">
        {state === 'copied' || state === 'link-copied' ? 'Copied' : state === 'error' ? 'Copy unavailable' : state === 'copying' ? 'Copying...' : ''}
      </span>
    </div>
  </>;
}
