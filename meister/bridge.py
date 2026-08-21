if not hasattr(str, "from_ptr"):
    from typing import Literal

__python__: Literal[bool] = not hasattr(str, "from_ptr")

if __python__:
    from .shim import *
else:
    from python import argparse

    # TODO: needed to avoid bug when one-branch-dominated type becomes non-type here due to dangling declaration

    class SyntaxError(Exception):
        def __init__(self, message: str = ""):
            super().__init__(message)

    class Codon:
        def unwrap(x, T: type = NoneType):
            return unwrap(x, T)

        def return_type(fn, *args) -> type:
            return type(fn(*args))

        def any_members(self: Any):
            yield from self.members

    CODON: Literal[bool] = True

    def class_name(o):
        if isinstance(o, Any):
            return o._real_typeinfo().nice_name
        elif static.has_rtti(o):
            return RTTIType._typeinfo(o).nice_name
        return o.__class__.__name__

    @extend
    class Any:
        def is_tuple(self):
            return isinstance(self, List[Any]) and class_name(self).startswith("Tuple")
        def is_list(self):
            return isinstance(self, List[Any])
        def is_optional(self):
            return isinstance(self, Optional[Any])


