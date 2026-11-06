const {themes} = require('prism-react-renderer');
const redirects = require('./redirects');

module.exports = async function config() {
  const {default: remarkMath} = await import('remark-math');
  const {default: rehypeKatex} = await import('rehype-katex');
  return {
    title: 'Codon',
    tagline: 'Python simplicity, native performance.',
    url: 'https://docs.exaloop.io',
    baseUrl: '/',
    trailingSlash: true,
    favicon: 'img/favicon.png',
    organizationName: 'exaloop',
    projectName: 'codon',
    onBrokenLinks: 'throw',
    onBrokenAnchors: 'ignore',
    onDuplicateRoutes: 'throw',
    clientModules: [require.resolve('./js/api-search.js'), require.resolve('./js/codon-pipeline.js')],
    plugins: [require.resolve('./plugins/markdown-pages'), ['@docusaurus/plugin-client-redirects', {redirects}]],
    markdown: {
      format: 'detect',
      mermaid: true,
      hooks: {onBrokenMarkdownLinks: 'throw', onBrokenMarkdownImages: 'throw'},
    },
    presets: [['classic', {
      docs: {
        path: '.',
        include: ['index.md', '{start,language,libraries,integrations,parallel,labs,developers}/**/*.{md,mdx}'],
        exclude: [],
        routeBasePath: '/',
        sidebarPath: require.resolve('./sidebars.js'),
        editUrl: ({docPath}) => `https://github.com/exaloop/codon/edit/develop/docs/${docPath}`,
        remarkPlugins: [remarkMath],
        rehypePlugins: [rehypeKatex],
      },
      blog: false,
      theme: {customCss: [require.resolve('katex/dist/katex.min.css'), require.resolve('./src/css/custom.css')]},
    }]],
    themes: [
      '@exaloop/theme/docusaurus',
      '@docusaurus/theme-mermaid',
      ['@easyops-cn/docusaurus-search-local', {
        hashed: true,
        indexBlog: false,
        docsDir: '.',
        docsRouteBasePath: '/',
        highlightSearchTermsOnTargetPage: true,
      }],
    ],
    themeConfig: {
      image: 'img/codon-black.png',
      colorMode: {defaultMode: 'light', respectPrefersColorScheme: true},
      docs: {sidebar: {autoCollapseCategories: false}},
      navbar: {
        title: 'Documentation',
        logo: {alt: 'Codon', src: 'img/codon.svg', srcDark: 'img/codon-white.svg', width: 108, height: 30},
        items: [
          {to: '/start/install/', label: 'Guides', position: 'left', activeBaseRegex: '^/(start/(?!changelog(?:/|$))|language/|integrations/|parallel/)'},
          {to: '/libraries/api/', label: 'API reference', position: 'left', activeBasePath: 'libraries/api'},
          {to: '/start/changelog/', label: 'Releases', position: 'left'},
          {href: 'https://platform.exaloop.io', label: 'Exaloop', position: 'left'},
          {type: 'custom-githubStars', position: 'right'},
        ],
      },
      footer: {
        logo: {alt: 'Codon', src: 'img/codon.svg', srcDark: 'img/codon-white.svg', href: '/', width: 180, height: 43},
        links: [
          {title: 'Learn', items: [{label: 'Installation', to: '/start/install/'}, {label: 'Language guide', to: '/language/overview/'}, {label: 'API reference', to: '/libraries/api/'}]},
          {title: 'Explore', items: [{label: 'Labs', to: '/labs/'}, {label: 'Releases', to: '/start/changelog/'}, {label: 'Blog', href: 'https://exaloop.io/blog'}]},
          {title: 'Project', items: [{label: 'Source code', href: 'https://github.com/exaloop/codon'}, {label: 'Contributing', to: '/developers/contribute/'}, {label: 'Exaloop Platform', href: 'https://platform.exaloop.io'}]},
        ],
        copyright: 'Copyright &copy; 2026 Exaloop, Inc.',
      },
      prism: {
        theme: {...themes.github, plain: {...themes.github.plain, backgroundColor: '#f6f7f6'}},
        darkTheme: {...themes.dracula, plain: {...themes.dracula.plain, backgroundColor: '#202427'}},
        additionalLanguages: ['python', 'bash', 'cpp', 'c', 'cmake', 'llvm', 'toml', 'json'],
      },
      tableOfContents: {minHeadingLevel: 2, maxHeadingLevel: 3},
    },
  };
};
