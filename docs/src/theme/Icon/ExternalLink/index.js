import React from 'react';
import {ArrowUpRight} from 'lucide';
import {translate} from '@docusaurus/Translate';

export default function IconExternalLink({width = 14, height = 14}) {
  return (
    <svg className="external-link-icon" width={width} height={height} viewBox="0 0 24 24"
      fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round"
      role="img" aria-label={translate({id: 'theme.IconExternalLink.ariaLabel', message: '(opens in new tab)'})}>
      {ArrowUpRight.map(([tag, attributes], index) => React.createElement(tag, {...attributes, key: index}))}
    </svg>
  );
}
