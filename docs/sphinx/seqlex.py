# Seq Pygments lexer based on Python's

import re

from pygments.lexer import Lexer, RegexLexer, include, bygroups, using, \
    default, words, combined, do_insertions
from pygments.util import get_bool_opt, shebang_matches
from pygments.token import Text, Comment, Operator, Keyword, Name, String, \
    Number, Punctuation, Generic, Other, Error
from pygments import unistring as uni

__all__ = ['SeqLexer']

line_re = re.compile('.*?\n')

class SeqLexer(RegexLexer):
    name = 'Seq'
    aliases = ['seq']
    filenames = ['*.seq']
    mimetypes = []

    flags = re.MULTILINE | re.UNICODE

    uni_name = "[%s][%s]*" % (uni.xid_start, uni.xid_continue)

    def innerstring_rules(ttype):
        return [
            # the old style '%s' % (...) string formatting (still valid in Py3)
            (r'%(\(\w+\))?[-#0 +]*([0-9]+|[*])?(\.([0-9]+|[*]))?'
             '[hlL]?[E-GXc-giorsux%]', String.Interpol),
            # the new style '{}'.format(...) string formatting
            (r'\{'
             '((\w+)((\.\w+)|(\[[^\]]+\]))*)?'  # field name
             '(\![sra])?'                       # conversion
             '(\:(.?[<>=\^])?[-+ ]?#?0?(\d+)?,?(\.\d+)?[E-GXb-gnosx%]?)?'
             '\}', String.Interpol),

            # backslashes, quotes and formatting signs must be parsed one at a time
            (r'[^\\\'"%{\n]+', ttype),
            (r'[\'"\\]', ttype),
            # unhandled string formatting sign
            (r'%|(\{{1,2})', ttype)
            # newlines are an error (use "nl" state)
        ]

    tokens = {
        'root': [
            (r'\n', Text),
            (r'^(\s*)([rRuUbB]{,2})("""(?:.|\n)*?""")',
             bygroups(Text, String.Affix, String.Doc)),
            (r"^(\s*)([rRuUbB]{,2})('''(?:.|\n)*?''')",
             bygroups(Text, String.Affix, String.Doc)),
            (r'[^\S\n]+', Text),
            (r'\A#!.+$', Comment.Hashbang),
            (r'#.*$', Comment.Single),
            (r'[]{}:(),;[]', Punctuation),
            (r'\\\n', Text),
            (r'\\', Text),
            (r'(in|is|and|or|not)\b', Operator.Word),
            (r'!=|==|<<|>>|[-~+/*%=<>&^|.]', Operator),
            include('keywords'),
            (r'(def)((?:\s|\\\s)+)', bygroups(Keyword, Text), 'funcname'),
            (r'(class)((?:\s|\\\s)+)', bygroups(Keyword, Text), 'classname'),
            (r'(from)((?:\s|\\\s)+)', bygroups(Keyword.Namespace, Text),
             'fromimport'),
            (r'(import)((?:\s|\\\s)+)', bygroups(Keyword.Namespace, Text),
             'import'),
            include('builtins'),
            include('magicfuncs'),
            include('magicvars'),
            include('backtick'),
            ('([skprR]|[uUbB][rR]|[rR][uUbB])(""")',
             bygroups(String.Affix, String.Double), 'tdqs'),
            ("([skprR]|[uUbB][rR]|[rR][uUbB])(''')",
             bygroups(String.Affix, String.Single), 'tsqs'),
            ('([skprR]|[uUbB][rR]|[rR][uUbB])(")',
             bygroups(String.Affix, String.Double), 'dqs'),
            ("([skprR]|[uUbB][rR]|[rR][uUbB])(')",
             bygroups(String.Affix, String.Single), 'sqs'),
            ('([uUbB]?)(""")', bygroups(String.Affix, String.Double),
             combined('stringescape', 'tdqs')),
            ("([uUbB]?)(''')", bygroups(String.Affix, String.Single),
             combined('stringescape', 'tsqs')),
            ('([uUbB]?)(")', bygroups(String.Affix, String.Double),
             combined('stringescape', 'dqs')),
            ("([uUbB]?)(')", bygroups(String.Affix, String.Single),
             combined('stringescape', 'sqs')),
            include('name'),
            include('numbers'),
        ],
        'funcname': [
            include('magicfuncs'),
            ('[a-zA-Z_]\w*', Name.Function, '#pop'),
            default('#pop'),
        ],
        'stringescape': [
            (r'\\([\\abfnrtv"\']|\n|N\{.*?\}|u[a-fA-F0-9]{4}|'
             r'U[a-fA-F0-9]{8}|x[a-fA-F0-9]{2}|[0-7]{1,3})', String.Escape)
        ],
        'strings-single': innerstring_rules(String.Single),
        'strings-double': innerstring_rules(String.Double),
        'dqs': [
            (r'"', String.Double, '#pop'),
            (r'\\\\|\\"|\\\n', String.Escape),  # included here for raw strings
            include('strings-double')
        ],
        'sqs': [
            (r"'", String.Single, '#pop'),
            (r"\\\\|\\'|\\\n", String.Escape),  # included here for raw strings
            include('strings-single')
        ],
        'tdqs': [
            (r'"""', String.Double, '#pop'),
            include('strings-double'),
            (r'\n', String.Double)
        ],
        'tsqs': [
            (r"'''", String.Single, '#pop'),
            include('strings-single'),
            (r'\n', String.Single)
        ],
    }

    tokens['keywords'] = [
        (words((
            'assert', 'async', 'await', 'break', 'continue', 'del', 'elif',
            'else', 'except', 'finally', 'for', 'global', 'if', 'lambda', 'pass',
            'raise', 'nonlocal', 'return', 'try', 'while', 'yield', 'yield from',
            'as', 'with', 'match', 'case', 'pydef',
            'type', 'extend', 'print', 'cimport', 'pyimport'), suffix=r'\b'),
         Keyword),
        (words((
            'True', 'False', 'None'), suffix=r'\b'),
         Keyword.Constant),
    ]

    seqwords = ['__import__', 'abs', 'all', 'any', 'bin', 'bool', 'bytearray', 'bytes',
                'chr', 'classmethod', 'cmp', 'compile', 'complex', 'delattr', 'dict',
                'dir', 'divmod', 'enumerate', 'eval', 'filter', 'float', 'format',
                'frozenset', 'getattr', 'globals', 'hasattr', 'hash', 'hex', 'id',
                'input', 'int', 'isinstance', 'issubclass', 'iter', 'len', 'list',
                'locals', 'map', 'max', 'memoryview', 'min', 'object', 'oct',
                'open', 'ord', 'pow', 'property', 'range', 'repr', 'reversed',
                'round', 'set', 'setattr', 'slice', 'sorted', 'staticmethod', 'str',
                'sum', 'super', 'tuple', 'vars', 'zip', 'seq', 'byte', 'ptr', 'array',
                'Kmer', 'Int', 'UInt', 'optional']

    tokens['builtins'] = [
        (words(seqwords, prefix=r'(?<!\.)', suffix=r'\b'),
         Name.Builtin),
        (r'(?<!\.)(self|Ellipsis|NotImplemented|cls)\b', Name.Builtin.Pseudo),
        (words((
            'ArithmeticError', 'AssertionError', 'AttributeError',
            'BaseException', 'BufferError', 'BytesWarning', 'DeprecationWarning',
            'EOFError', 'EnvironmentError', 'Exception', 'FloatingPointError',
            'FutureWarning', 'GeneratorExit', 'IOError', 'ImportError',
            'ImportWarning', 'IndentationError', 'IndexError', 'KeyError',
            'KeyboardInterrupt', 'LookupError', 'MemoryError', 'NameError',
            'NotImplementedError', 'OSError', 'OverflowError',
            'PendingDeprecationWarning', 'ReferenceError', 'ResourceWarning',
            'RuntimeError', 'RuntimeWarning', 'StopIteration',
            'SyntaxError', 'SyntaxWarning', 'SystemError', 'SystemExit', 'TabError',
            'TypeError', 'UnboundLocalError', 'UnicodeDecodeError',
            'UnicodeEncodeError', 'UnicodeError', 'UnicodeTranslateError',
            'UnicodeWarning', 'UserWarning', 'ValueError', 'VMSError', 'Warning',
            'WindowsError', 'ZeroDivisionError',
            # new builtin exceptions from PEP 3151
            'BlockingIOError', 'ChildProcessError', 'ConnectionError',
            'BrokenPipeError', 'ConnectionAbortedError', 'ConnectionRefusedError',
            'ConnectionResetError', 'FileExistsError', 'FileNotFoundError',
            'InterruptedError', 'IsADirectoryError', 'NotADirectoryError',
            'PermissionError', 'ProcessLookupError', 'TimeoutError'),
            prefix=r'(?<!\.)', suffix=r'\b'),
         Name.Exception),
    ]
    tokens['magicfuncs'] = [
        (words((
            '__abs__', '__add__', '__aenter__', '__aexit__', '__aiter__', '__and__',
            '__anext__', '__await__', '__bool__', '__bytes__', '__call__',
            '__complex__', '__contains__', '__del__', '__delattr__', '__delete__',
            '__delitem__', '__dir__', '__divmod__', '__enter__', '__eq__', '__exit__',
            '__float__', '__floordiv__', '__format__', '__ge__', '__get__',
            '__getattr__', '__getattribute__', '__getitem__', '__gt__', '__hash__',
            '__iadd__', '__iand__', '__ifloordiv__', '__ilshift__', '__imatmul__',
            '__imod__', '__import__', '__imul__', '__index__', '__init__',
            '__instancecheck__', '__int__', '__invert__', '__ior__', '__ipow__',
            '__irshift__', '__isub__', '__iter__', '__itruediv__', '__ixor__',
            '__le__', '__len__', '__length_hint__', '__lshift__', '__lt__',
            '__matmul__', '__missing__', '__mod__', '__mul__', '__ne__', '__neg__',
            '__new__', '__next__', '__or__', '__pos__', '__pow__', '__prepare__',
            '__radd__', '__rand__', '__rdivmod__', '__repr__', '__reversed__',
            '__rfloordiv__', '__rlshift__', '__rmatmul__', '__rmod__', '__rmul__',
            '__ror__', '__round__', '__rpow__', '__rrshift__', '__rshift__',
            '__rsub__', '__rtruediv__', '__rxor__', '__set__', '__setattr__',
            '__setitem__', '__str__', '__sub__', '__subclasscheck__', '__truediv__',
            '__xor__'), suffix=r'\b'),
         Name.Function.Magic),
    ]
    tokens['magicvars'] = [
        (words((
            '__annotations__', '__bases__', '__class__', '__closure__', '__code__',
            '__defaults__', '__dict__', '__doc__', '__file__', '__func__',
            '__globals__', '__kwdefaults__', '__module__', '__mro__', '__name__',
            '__objclass__', '__qualname__', '__self__', '__slots__', '__weakref__'),
            suffix=r'\b'),
         Name.Variable.Magic),
    ]
    tokens['numbers'] = [
        (r'(\d+\.\d*|\d*\.\d+)([eE][+-]?[0-9]+)?', Number.Float),
        (r'\d+[eE][+-]?[0-9]+j?', Number.Float),
        (r'0[oO][0-7]+', Number.Oct),
        (r'0[bB][01]+', Number.Bin),
        (r'0[xX][a-fA-F0-9]+', Number.Hex),
        (r'\d+', Number.Integer)
    ]
    tokens['backtick'] = []
    tokens['name'] = [
        (r'@\w+', Name.Decorator),
        (r'@', Operator),  # new matrix multiplication operator
        (uni_name, Name),
    ]
    tokens['funcname'] = [
        (uni_name, Name.Function, '#pop')
    ]
    tokens['classname'] = [
        (uni_name, Name.Class, '#pop')
    ]
    tokens['import'] = [
        (r'(\s+)(as)(\s+)', bygroups(Text, Keyword, Text)),
        (r'\.', Name.Namespace),
        (uni_name, Name.Namespace),
        (r'(\s*)(,)(\s*)', bygroups(Text, Operator, Text)),
        default('#pop')  # all else: go back
    ]
    tokens['fromimport'] = [
        (r'(\s+)(import)\b', bygroups(Text, Keyword), '#pop'),
        (r'\.', Name.Namespace),
        (uni_name, Name.Namespace),
        default('#pop'),
    ]
    tokens['strings-single'] = innerstring_rules(String.Single)
    tokens['strings-double'] = innerstring_rules(String.Double)

    def analyse_text(text):
        return shebang_matches(text, r'seq')
