class Type:
    pass

class Function:
    pass

@dataclass(init=False)
class PyModule:
    types: List[object]
    functions: List[object]

    def __init__(
        self,
        types: List[object] | None = None,
        functions: List[object] | None = None,
    ):
        self.types = [] if types is None else types
        self.functions = [] if functions is None else functions

