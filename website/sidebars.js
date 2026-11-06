const {withIcon} = require('@exaloop/theme/sidebar');

const category = (label, items, collapsed = false) => ({type: 'category', label, items, collapsed});

module.exports = {
  docs: [
    withIcon({type: 'doc', id: 'index', label: 'Overview'}, 'house'),
    withIcon(category('Get started', ['start/install', 'start/usage', 'start/options', {type: 'doc', id: 'start/faq', label: 'FAQ'}]), 'rocket'),
    withIcon(category('Language', ['language/overview', 'language/classes', 'language/generics', 'language/meta', 'language/lowlevel', 'language/llvm']), 'code-xml'),
    withIcon(category('Libraries', ['libraries/stdlib', 'libraries/numpy', {type: 'doc', id: 'libraries/api/index', label: 'API reference'}]), 'library'),
    withIcon(category('Integrations', [
      category('Python', ['integrations/python/python-from-codon', 'integrations/python/codon-from-python', 'integrations/python/extensions']),
      category('C / C++', ['integrations/cpp/cpp-from-codon', 'integrations/cpp/codon-from-cpp', 'integrations/cpp/jit']),
      'integrations/jupyter',
    ], true), 'blocks'),
    withIcon(category('Parallelism', ['parallel/multithreading', 'parallel/gpu', 'parallel/simd'], true), 'cpu'),
    withIcon(category('Labs', ['labs/index', 'labs/catalog/start'], true), 'flask-conical'),
    withIcon(category('Developers', ['developers/compilation', 'developers/ir', 'developers/extend', 'developers/build', 'developers/contribute', 'developers/roadmap'], true), 'square-terminal'),
  ],
};
