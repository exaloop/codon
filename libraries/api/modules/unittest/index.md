# unittest

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/unittest.codon)

class TestCase[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase "Copy/link to this API element")

```python
class TestCase
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/unittest/TestCase/)

Methods 23

setUp[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-setup "Copy/link to this API element")

```python
setUp(self: Self)
```

tearDown[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-teardown "Copy/link to this API element")

```python
tearDown(self: Self)
```

fail[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-fail "Copy/link to this API element")

```python
fail(self: Self, standard_message: str, special_message: str = "")
```

assertTrue[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-asserttrue "Copy/link to this API element")

```python
assertTrue(self: Self, obj, msg = "")
```

assertFalse[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertfalse "Copy/link to this API element")

```python
assertFalse(self: Self, obj, msg = "")
```

assertEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertequal "Copy/link to this API element")

```python
assertEqual(self: Self, first, second, msg = "")
```

assertNotEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertnotequal "Copy/link to this API element")

```python
assertNotEqual(self: Self, first, second, msg = "")
```

assertSequenceEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertsequenceequal "Copy/link to this API element")

```python
assertSequenceEqual(self: Self, seq1, seq2, msg = "")
```

assertIn[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertin "Copy/link to this API element")

```python
assertIn(self: Self, member, container, msg = "")
```

assertNotIn[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertnotin "Copy/link to this API element")

```python
assertNotIn(self: Self, member, container, msg = "")
```

assertIs[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertis "Copy/link to this API element")

```python
assertIs(self: Self, expr1, expr2, msg = "")
```

```python
assertIs(self: Self, expr1, expr2, msg = "")
```

assertIsNot[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertisnot "Copy/link to this API element")

```python
assertIsNot(self: Self, expr1, expr2, msg = "")
```

```python
assertIsNot(self: Self, expr1, expr2, msg = "")
```

assertCountEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertcountequal "Copy/link to this API element")

```python
assertCountEqual(self: Self, first, second, msg = "")
```

assertLess[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertless "Copy/link to this API element")

```python
assertLess(self: Self, a, b, msg = "")
```

assertLessEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertlessequal "Copy/link to this API element")

```python
assertLessEqual(self: Self, a, b, msg = "")
```

assertGreater[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertgreater "Copy/link to this API element")

```python
assertGreater(self: Self, a, b, msg = "")
```

assertGreaterEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertgreaterequal "Copy/link to this API element")

```python
assertGreaterEqual(self: Self, a, b, msg = "")
```

assertIsNone[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertisnone "Copy/link to this API element")

```python
assertIsNone(self: Self, obj, msg = "")
```

assertIsNotNone[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertisnotnone "Copy/link to this API element")

```python
assertIsNotNone(self: Self, obj, msg = "")
```

assertRaises[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertraises "Copy/link to this API element")

```python
assertRaises[exception: type](self: Self, function, *args, **kwargs)
```

assertAlmostEqual[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertalmostequal "Copy/link to this API element")

```python
assertAlmostEqual(self: Self, first, second, places: int = 0, msg = "", delta = None)
```

assertIsInstance[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertisinstance "Copy/link to this API element")

```python
assertIsInstance[T: type](self: Self, obj, msg = "")
```

assertNotIsInstance[](https://docs.exaloop.io/libraries/api/modules/unittest/#class-testcase-assertnotisinstance "Copy/link to this API element")

```python
assertNotIsInstance[T: type](self: Self, obj, msg = "")
```

function main[](https://docs.exaloop.io/libraries/api/modules/unittest/#function-main "Copy/link to this API element")

```python
main()
```

Source: https://docs.exaloop.io/libraries/api/modules/unittest/
