# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from enum import Enum


class Op(Enum):
    """NumPy opcodes and cost estimates from codon/cir/transform/numpy/expr.cpp."""

    NONE = ('a', 0)
    POS = ('pos', 0)
    NEG = ('neg', 0)
    INVERT = ('invert', 0)
    ABS = ('abs', 1)
    TRANSPOSE = ('transpose', 0)
    ADD = ('add', 1)
    SUB = ('sub', 1)
    MUL = ('mul', 1)
    MATMUL = ('matmul', 20)
    TRUE_DIV = ('true_div', 8)
    FLOOR_DIV = ('floor_div', 8)
    MOD = ('mod', 8)
    FMOD = ('fmod', 8)
    POW = ('pow', 8)
    LSHIFT = ('lshift', 1)
    RSHIFT = ('rshift', 1)
    AND = ('and', 1)
    OR = ('or', 1)
    XOR = ('xor', 1)
    LOGICAL_AND = ('logical_and', 1)
    LOGICAL_OR = ('logical_or', 1)
    LOGICAL_XOR = ('logical_xor', 1)
    EQ = ('eq', 1)
    NE = ('ne', 1)
    LT = ('lt', 1)
    LE = ('le', 1)
    GT = ('gt', 1)
    GE = ('ge', 1)
    MIN = ('minimum', 3)
    MAX = ('maximum', 3)
    FMIN = ('fmin', 3)
    FMAX = ('fmax', 3)
    SIN = ('sin', 10)
    COS = ('cos', 10)
    TAN = ('tan', 10)
    ARCSIN = ('arcsin', 20)
    ARCCOS = ('arccos', 20)
    ARCTAN = ('arctan', 20)
    ARCTAN2 = ('arctan2', 35)
    HYPOT = ('hypot', 5)
    SINH = ('sinh', 10)
    COSH = ('cosh', 10)
    TANH = ('tanh', 10)
    ARCSINH = ('arcsinh', 10)
    ARCCOSH = ('arccosh', 10)
    ARCTANH = ('arctanh', 10)
    CONJ = ('conj', 1)
    EXP = ('exp', 5)
    EXP2 = ('exp2', 5)
    LOG = ('log', 5)
    LOG2 = ('log2', 5)
    LOG10 = ('log10', 5)
    EXPM1 = ('expm1', 5)
    LOG1P = ('log1p', 5)
    SQRT = ('sqrt', 2)
    SQUARE = ('square', 1)
    CBRT = ('cbrt', 5)
    LOGADDEXP = ('logaddexp', 10)
    LOGADDEXP2 = ('logaddexp2', 10)
    RECIPROCAL = ('reciprocal', 1)
    RINT = ('rint', 1)
    FLOOR = ('floor', 1)
    CEIL = ('ceil', 1)
    TRUNC = ('trunc', 1)
    ISNAN = ('isnan', 1)
    ISINF = ('isinf', 1)
    ISFINITE = ('isfinite', 1)
    SIGN = ('sign', 1)
    SIGNBIT = ('signbit', 1)
    COPYSIGN = ('copysign', 1)
    SPACING = ('spacing', 1)
    NEXTAFTER = ('nextafter', 1)
    DEG2RAD = ('deg2rad', 2)
    RAD2DEG = ('rad2deg', 2)
    HEAVISIDE = ('heaviside', 3)
    ROLL = ('roll', 1)
    CAST = ('cast', 1)
    ZEROS_LIKE = ('zeros_like', 1)
    ONES_LIKE = ('ones_like', 1)
    WHERE = ('where', 1)
    CLIP = ('clip', 2)
    CLIP_MIN = ('clip_min', 1)
    CLIP_MAX = ('clip_max', 1)
    SUM = ('sum', 1)
    MEAN = ('mean', 1)
    PROD = ('prod', 1)
    ANY = ('any', 1)
    ALL = ('all', 1)
    AMIN = ('amin', 1)
    AMAX = ('amax', 1)

    def __init__(self, op_name, cost):
        self.op_name = op_name
        self.cost = cost


# Function names and arities from numpy.cpp.
MAGICS = {
    'POS_MAGIC_NAME': (Op.POS, 1, False),
    'NEG_MAGIC_NAME': (Op.NEG, 1, False),
    'INVERT_MAGIC_NAME': (Op.INVERT, 1, False),
    'ABS_MAGIC_NAME': (Op.ABS, 1, False),
    'ADD_MAGIC_NAME': (Op.ADD, 2, False),
    'SUB_MAGIC_NAME': (Op.SUB, 2, False),
    'MUL_MAGIC_NAME': (Op.MUL, 2, False),
    'MATMUL_MAGIC_NAME': (Op.MATMUL, 2, False),
    'TRUE_DIV_MAGIC_NAME': (Op.TRUE_DIV, 2, False),
    'FLOOR_DIV_MAGIC_NAME': (Op.FLOOR_DIV, 2, False),
    'MOD_MAGIC_NAME': (Op.MOD, 2, False),
    'POW_MAGIC_NAME': (Op.POW, 2, False),
    'LSHIFT_MAGIC_NAME': (Op.LSHIFT, 2, False),
    'RSHIFT_MAGIC_NAME': (Op.RSHIFT, 2, False),
    'AND_MAGIC_NAME': (Op.AND, 2, False),
    'OR_MAGIC_NAME': (Op.OR, 2, False),
    'XOR_MAGIC_NAME': (Op.XOR, 2, False),
    'RADD_MAGIC_NAME': (Op.ADD, 2, True),
    'RSUB_MAGIC_NAME': (Op.SUB, 2, True),
    'RMUL_MAGIC_NAME': (Op.MUL, 2, True),
    'RMATMUL_MAGIC_NAME': (Op.MATMUL, 2, True),
    'RTRUE_DIV_MAGIC_NAME': (Op.TRUE_DIV, 2, True),
    'RFLOOR_DIV_MAGIC_NAME': (Op.FLOOR_DIV, 2, True),
    'RMOD_MAGIC_NAME': (Op.MOD, 2, True),
    'RPOW_MAGIC_NAME': (Op.POW, 2, True),
    'RLSHIFT_MAGIC_NAME': (Op.LSHIFT, 2, True),
    'RRSHIFT_MAGIC_NAME': (Op.RSHIFT, 2, True),
    'RAND_MAGIC_NAME': (Op.AND, 2, True),
    'ROR_MAGIC_NAME': (Op.OR, 2, True),
    'RXOR_MAGIC_NAME': (Op.XOR, 2, True),
    'EQ_MAGIC_NAME': (Op.EQ, 2, False),
    'NE_MAGIC_NAME': (Op.NE, 2, False),
    'LT_MAGIC_NAME': (Op.LT, 2, False),
    'LE_MAGIC_NAME': (Op.LE, 2, False),
    'GT_MAGIC_NAME': (Op.GT, 2, False),
    'GE_MAGIC_NAME': (Op.GE, 2, False),
}

UFUNCS = {
    'positive': (Op.POS, 1),
    'negative': (Op.NEG, 1),
    'invert': (Op.INVERT, 1),
    'abs': (Op.ABS, 1),
    'absolute': (Op.ABS, 1),
    'add': (Op.ADD, 2),
    'subtract': (Op.SUB, 2),
    'multiply': (Op.MUL, 2),
    'divide': (Op.TRUE_DIV, 2),
    'floor_divide': (Op.FLOOR_DIV, 2),
    'remainder': (Op.MOD, 2),
    'fmod': (Op.FMOD, 2),
    'power': (Op.POW, 2),
    'left_shift': (Op.LSHIFT, 2),
    'right_shift': (Op.RSHIFT, 2),
    'bitwise_and': (Op.AND, 2),
    'bitwise_or': (Op.OR, 2),
    'bitwise_xor': (Op.XOR, 2),
    'logical_and': (Op.LOGICAL_AND, 2),
    'logical_or': (Op.LOGICAL_OR, 2),
    'logical_xor': (Op.LOGICAL_XOR, 2),
    'equal': (Op.EQ, 2),
    'not_equal': (Op.NE, 2),
    'less': (Op.LT, 2),
    'less_equal': (Op.LE, 2),
    'greater': (Op.GT, 2),
    'greater_equal': (Op.GE, 2),
    'minimum': (Op.MIN, 2),
    'maximum': (Op.MAX, 2),
    'fmin': (Op.FMIN, 2),
    'fmax': (Op.FMAX, 2),
    'sin': (Op.SIN, 1),
    'cos': (Op.COS, 1),
    'tan': (Op.TAN, 1),
    'arcsin': (Op.ARCSIN, 1),
    'arccos': (Op.ARCCOS, 1),
    'arctan': (Op.ARCTAN, 1),
    'arctan2': (Op.ARCTAN2, 2),
    'hypot': (Op.HYPOT, 2),
    'sinh': (Op.SINH, 1),
    'cosh': (Op.COSH, 1),
    'tanh': (Op.TANH, 1),
    'arcsinh': (Op.ARCSINH, 1),
    'arccosh': (Op.ARCCOSH, 1),
    'arctanh': (Op.ARCTANH, 1),
    'conjugate': (Op.CONJ, 1),
    'exp': (Op.EXP, 1),
    'exp2': (Op.EXP2, 1),
    'log': (Op.LOG, 1),
    'log2': (Op.LOG2, 1),
    'log10': (Op.LOG10, 1),
    'expm1': (Op.EXPM1, 1),
    'log1p': (Op.LOG1P, 1),
    'sqrt': (Op.SQRT, 1),
    'square': (Op.SQUARE, 1),
    'cbrt': (Op.CBRT, 1),
    'logaddexp': (Op.LOGADDEXP, 2),
    'logaddexp2': (Op.LOGADDEXP2, 2),
    'reciprocal': (Op.RECIPROCAL, 1),
    'rint': (Op.RINT, 1),
    'floor': (Op.FLOOR, 1),
    'ceil': (Op.CEIL, 1),
    'trunc': (Op.TRUNC, 1),
    'isnan': (Op.ISNAN, 1),
    'isinf': (Op.ISINF, 1),
    'isfinite': (Op.ISFINITE, 1),
    'sign': (Op.SIGN, 1),
    'signbit': (Op.SIGNBIT, 1),
    'copysign': (Op.COPYSIGN, 2),
    'spacing': (Op.SPACING, 1),
    'nextafter': (Op.NEXTAFTER, 2),
    'deg2rad': (Op.DEG2RAD, 1),
    'radians': (Op.DEG2RAD, 1),
    'rad2deg': (Op.RAD2DEG, 1),
    'degrees': (Op.RAD2DEG, 1),
    'heaviside': (Op.HEAVISIDE, 2),
}
