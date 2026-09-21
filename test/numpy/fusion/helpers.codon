import numpy as np

L: List[str] = []

@pure
def f(x, label):
    L.append(label)
    return x

def eq(got, exp, dtype: type):
    if got.dtype is not dtype:
        print('-> dtype mismatch: got', got.dtype.__name__, 'but expected', dtype.__name__)
        return False
    return np.allclose(got, np.asarray(exp, dtype), equal_nan=True, atol=1e-10)

@noinline
def materialize(array):
    return array

events: List[int] = []

def record(value, label):
    events.append(label)
    return value

def mutate_initial(array):
    array[0] = 100
    return 2

def mutate_array(array):
    array[0] = 100
    return array

def effectful_none():
    events.append(7)

def check_axis_result(actual, expected):
    assert np.array_equal(actual, expected), (actual, expected)
    if hasattr(actual, 'shape'):
        assert actual.shape == expected.shape, (actual.shape, expected.shape)
        assert actual.strides == expected.strides, (actual.strides, expected.strides)

@noinline
def matmul_add(left, right, bias, mode: int = 0):
    if mode == 0:
        return left @ right + bias
    elif mode == 1:
        return bias + left @ right
    elif mode == 2:
        return left @ right - bias
    else:
        return bias - left @ right

@noinline
def matmul_add_reference(left, right, bias, mode: int = 0):
    product = materialize(left @ right)
    if mode == 0:
        return product + bias
    elif mode == 1:
        return bias + product
    elif mode == 2:
        return product - bias
    else:
        return bias - product

@noinline
def matmul_add_borrowed(left, right, output, mode: int = 0):
    if mode == 0:
        output[:] = left @ right + output
    elif mode == 1:
        output[:] = output + left @ right
    elif mode == 2:
        output[:] = left @ right - output
    else:
        output[:] = output - left @ right
    return output

def check_matmul_add_result(actual, expected):
    assert actual.shape == expected.shape
    assert actual.dtype is expected.dtype
    assert np.array_equal(np.isnan(actual), np.isnan(expected))
    assert np.array_equal(np.isposinf(actual), np.isposinf(expected))
    assert np.array_equal(np.isneginf(actual), np.isneginf(expected))
    finite = np.isfinite(expected)
    assert np.allclose(actual[finite], expected[finite], rtol=2e-5, atol=2e-6)
