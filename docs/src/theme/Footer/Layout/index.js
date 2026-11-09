import React from 'react';
import Link from '@docusaurus/Link';
import {ThemeClassNames} from '@docusaurus/theme-common';
import {ArrowUpRight} from 'lucide';
import {faGithub, faDiscord, faXTwitter, faLinkedin} from '@fortawesome/free-brands-svg-icons';

const socials = [
  {name: 'GitHub', href: 'https://github.com/exaloop', icon: faGithub},
  {name: 'Discord', href: 'https://discord.gg/HeWRhagCmP', icon: faDiscord},
  {name: 'X (Twitter)', href: 'https://x.com/exaloop', icon: faXTwitter},
  {name: 'LinkedIn', href: 'https://linkedin.com/company/exaloop', icon: faLinkedin},
];

export default function FooterLayout({links, logo, copyright}) {
  return (
    <footer className={`${ThemeClassNames.layout.footer.container} footer codon-footer`}>
      <div className="codon-footer__inner">
        <div className="codon-footer__main">
          <div className="codon-footer__brand">
            {logo}
            <p className="codon-footer__tagline">Python simplicity, native performance.</p>
            <nav className="codon-footer__socials" aria-label="Exaloop social links">
              {socials.map(({name, href, icon}) => (
                <Link key={name} href={href} target="_blank" rel="noopener noreferrer"
                  className="codon-footer__social" title={name} aria-label={`${name} (opens in a new tab)`}>
                  <svg width="20" height="20" viewBox={`0 0 ${icon.icon[0]} ${icon.icon[1]}`}
                    fill="currentColor" aria-hidden="true" focusable="false">
                    {[icon.icon[4]].flat().map((path, index) => <path key={index} d={path} />)}
                  </svg>
                </Link>
              ))}
            </nav>
          </div>
          <nav className="codon-footer__navigation" aria-label="Footer navigation">{links}</nav>
        </div>
        <div className="codon-footer__bottom">
          {copyright}
          <Link className="codon-footer__credit" href="https://exaloop.io" target="_blank" rel="noopener noreferrer">
            An Exaloop project
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor"
              strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true" focusable="false">
              {ArrowUpRight.map(([tag, attributes], index) => React.createElement(tag, {...attributes, key: index}))}
            </svg>
          </Link>
        </div>
      </div>
    </footer>
  );
}
