import React, {useEffect, useState} from 'react';
import {Star} from 'lucide';
import ComponentTypes from '@theme-original/NavbarItem/ComponentTypes';
import DefaultNavbarItem from '@theme/NavbarItem/DefaultNavbarItem';
import useBaseUrl from '@docusaurus/useBaseUrl';
import {formatStarCount, loadStarCount} from './github-stars.mjs';

let starsRequest;

function GitHubStarsNavbarItem(props) {
  const [count, setCount] = useState(null);
  const mark = useBaseUrl('/img/github-mark.svg');
  useEffect(() => {
    let mounted = true;
    let storage;
    try {
      storage = window.sessionStorage;
    } catch {}
    starsRequest ||= loadStarCount({storage});
    starsRequest.then(value => { if (mounted) setCount(value); });
    return () => { mounted = false; };
  }, []);
  const description = count === null ? 'Star Codon on GitHub' : `${count.toLocaleString('en-US')} stars on GitHub`;
  return (
    <DefaultNavbarItem
      {...props}
      href="https://github.com/exaloop/codon"
      className="github-stars"
      title={description}
      aria-label={`${description} (opens in a new tab)`}
      label={<>
        <img className="github-mark" src={mark} width="18" height="18" alt="" />
        <span className="github-star-count" aria-hidden="true">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round">
            {Star.map(([tag, attributes], index) => React.createElement(tag, {...attributes, key: index}))}
          </svg>
          <span className="github-star-number">{count === null ? 'Star' : formatStarCount(count)}</span>
        </span>
      </>}
    />
  );
}

export default {...ComponentTypes, 'custom-githubStars': GitHubStarsNavbarItem};
