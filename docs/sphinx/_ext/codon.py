from collections import defaultdict

import re

from typing import Any,Dict,Iterable,Iterator,List,NamedTuple,Tuple
from typing import cast

from docutils.parsers.rst import directives
from sphinx import addnodes
from sphinx.directives import ObjectDescription
from sphinx.domains import Domain
from sphinx.domains import Index
from sphinx.roles import XRefRole
from sphinx.util.nodes import make_refnode
from sphinx.locale import _,__
from docutils import nodes
from docutils.nodes import Element,Node
from docutils.parsers.rst import directives

from sphinx import addnodes
from sphinx.addnodes import pending_xref,desc_signature
from sphinx.application import Sphinx
from sphinx.builders import Builder
from sphinx.directives import ObjectDescription
from sphinx.domains import Domain,ObjType,Index,IndexEntry
from sphinx.environment import BuildEnvironment
from sphinx.locale import _,__
from sphinx.pycode.ast import ast,parse as ast_parse
from sphinx.roles import XRefRole
from sphinx.util import logging
from sphinx.util.docfields import Field,GroupedField,TypedField
from sphinx.util.docutils import SphinxDirective
from sphinx.util.inspect import signature_from_str
from sphinx.util.nodes import make_id,make_refnode
from sphinx.util.typing import TextlikeNode

import sphinx.domains.python

codon_sig_re=re.compile(r'''^([\w.]*\.)?(\w+)\s*(?:\[\s*(.*)\s*\])?\s*(?:\(\s*(.*)\s*\)(?:\s*->\s*(.*))?)?$''',re.VERBOSE)


def handle_signature(self,sig: str,signode: desc_signature) -> Tuple[str,str]:
    m=codon_sig_re.match(sig)
    if m is None:
        raise ValueError
    prefix,name,generics,arglist,retann=m.groups()

    # determine module and class name (if applicable), as well as full name
    modname=self.options.get('module',self.env.ref_context.get('py:module'))
    classname=self.env.ref_context.get('py:class')
    isextension=self.objtype=='extension'
    if classname:
        add_module=False
        if prefix and (prefix==classname or prefix.startswith(classname+".")):
            fullname=prefix+name
            # class name is given again in the signature
            prefix=prefix[len(classname):].lstrip('.')
        elif prefix:
            # class name is given in the signature, but different
            # (shouldn't happen)
            fullname=classname+'.'+prefix+name
        else:
            # class name is not given in the signature
            fullname=classname+'.'+name
    else:
        add_module=True
        if prefix:
            classname=prefix.rstrip('.')
            fullname=prefix+name
        else:
            classname=''
            fullname=name

    signode['module']=modname
    if modname.startswith('..'):  # HACK: no idea why this happens
        modname=modname[2:]
    signode['class']=classname
    signode['fullname']=fullname

    sig_prefix=self.get_signature_prefix(sig)
    if sig_prefix:
        signode+=addnodes.desc_annotation(sig_prefix,sig_prefix)

    if not isextension:
        if prefix:
            signode+=addnodes.desc_addname(prefix,prefix)
        elif add_module and self.env.config.add_module_names:
            if modname and modname!='exceptions':
                # exceptions are a special case, since they are documented in the
                # 'exceptions' module.
                nodetext=modname+'.'
                signode+=addnodes.desc_addname(nodetext,nodetext)
        signode+=addnodes.desc_name(name,name)
    else:
        ref=type_to_xref(str(name),self.env)
        signode+=addnodes.desc_name(name,name)

    if generics:
        signode+=addnodes.desc_name("generics","["+generics+"]")
    if arglist:
        try:
            signode+=sphinx.domains.python._parse_arglist(arglist,self.env)
        except SyntaxError:
            # fallback to parse arglist original parser.
            # it supports to represent optional arguments (ex. "func(foo [, bar])")
            sphinx.domains.python._pseudo_parse_arglist(signode,arglist)
        except NotImplementedError as exc:
            logger.warning("could not parse arglist (%r): %s",arglist,exc,location=signode)
            sphinx.domains.python._pseudo_parse_arglist(signode,arglist)
    else:
        if self.needs_arglist():
            # for callables, add an empty parameter list
            signode+=addnodes.desc_parameterlist()

    if retann:
        children=sphinx.domains.python._parse_annotation(retann,self.env)
        signode+=addnodes.desc_returns(retann,'',*children)

    anno=self.options.get('annotation')
    if anno:
        signode+=addnodes.desc_annotation(' '+anno,' '+anno)

    return fullname,prefix


sphinx.domains.python.PyObject.handle_signature=handle_signature

from sphinx.domains.python import *

logger=logging.getLogger(__name__)


class CodonDomain(PythonDomain):
    name='codon'
    label='Codon'
    object_types={'function':ObjType(_('function'),'func','obj'),# 'data':         ObjType(_('data'),        'data', 'obj'),
        'class':ObjType(_('class'),'class','exc','obj'),'type':ObjType(_('class'),'exc','class','obj'),'extension':ObjType(_('class'),'exc','class','obj'),
        'exception':ObjType(_('exception'),'exc','class','obj'),'method':ObjType(_('method'),'meth','obj'),# 'classmethod':  ObjType(_('class method'),  'meth', 'obj'),
        # 'staticmethod': ObjType(_('static method'), 'meth', 'obj'),
        # 'attribute':    ObjType(_('attribute'),     'attr', 'obj'),
        'module':ObjType(_('module'),'mod','obj'),}  # type: Dict[str, ObjType]

    directives={'function':PyFunction,'data':PyVariable,'class':PyClasslike,'exception':PyClasslike,'type':PyClasslike,'extension':PyClasslike,'method':PyMethod,'classmethod':PyClassMethod,
        'staticmethod':PyStaticMethod,'attribute':PyAttribute,'module':PyModule,'currentmodule':PyCurrentModule,'decorator':PyDecoratorFunction,'decoratormethod':PyDecoratorMethod,}
    roles={'data':PyXRefRole(),'exc':PyXRefRole(),'func':PyXRefRole(fix_parens=True),'class':PyXRefRole(),# 'const': PyXRefRole(),
        'attr':PyXRefRole(),'meth':PyXRefRole(fix_parens=True),'mod':PyXRefRole(),'obj':PyXRefRole(),}
    initial_data={'objects':{},  # fullname -> docname, objtype
        'modules':{},  # modname -> docname, synopsis, platform, deprecated
    }  # type: Dict[str, Dict[str, Tuple[Any]]]
    indices=[PythonModuleIndex,]


def setup(app):
    app.add_domain(CodonDomain)
    # app.connect('object-description-simplify', filter_meta_fields)

    return {'version':'0.1','parallel_read_safe':True,'parallel_write_safe':True,}
