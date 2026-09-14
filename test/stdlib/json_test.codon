import json
from json import JSON

@test
def test_container():
    assert JSON().is_null()
    assert JSON(True).as_bool()
    assert int(JSON(42)) == 42
    assert float(JSON(1.25)) == 1.25
    assert JSON("text").as_str() == "text"
    for value in [JSON(), JSON(True), JSON(42), JSON(1.25), JSON("text"), JSON.array(), JSON.object()]:
        assert value.is_bool() == (value.kind == "bool")
        assert value.is_str() == (value.kind == "str")
        assert value.is_array() == (value.kind == "array")
        assert value.is_object() == (value.kind == "object")
    value = JSON((1, "two", None, True))
    assert len(value) == 4 and value[1].as_str() == "two"
    value[0] = 5
    value.append(6.5)
    assert value[-1].as_float() == 6.5
    del value[1]
    assert value[1].is_null()
    obj = JSON.object()
    obj["array"] = value
    obj["name"] = "example"
    assert obj["array"][0].as_int() == 5
    assert obj.get("missing").is_null()
    assert obj.get("missing", 7).as_int() == 7
    assert list(obj.keys()) == ["array", "name"]
    assert [item.as_int() for item in JSON([1, 2, 3]).values()] == [1, 2, 3]
    try:
        JSON(True).as_int()
        assert False
    except TypeError:
        pass

test_container()

@test
def test_decode():
    value = json.loads(' {"name": "test", "values": [1, -2, 3.5, true, null], "name": "last"} ')
    assert value["name"].as_str() == "last"
    assert value["values"][2].as_float() == 3.5
    assert value["values"][3].as_bool()
    assert value["values"][4].is_null()
    assert json.loads('"a\\n\\t\\b\\f\\r\\/\\\\\\\""').as_str() == 'a\n\t\b\f\r/\\"'
    assert json.loads('"\\ud83d\\ude00"').as_str() == chr(0x1F600)
    assert json.loads('"\\ud800x"').as_str() == chr(0xD800) + "x"
    assert json.loads('"a\nb"', strict=False).as_str() == "a\nb"
    assert json.loads("9223372036854775807").as_int() == 9223372036854775807
    assert json.loads("-9223372036854775808").as_int() == -9223372036854775808
    for source in ["9223372036854775808", "-9223372036854775809"]:
        try:
            json.loads(source)
            assert False
        except OverflowError:
            pass
    for encoding in ["utf-8", "utf-16", "utf-16-be", "utf-32", "utf-32-le"]:
        assert json.loads('["text"]'.encode(encoding))[0].as_str() == "text"
    assert json.loads(bytearray(b"true")).as_bool()
    for source in ["", "[1,]", "{a:1}", '"bad\\q"', '"bad\n"', "01", "1e", "true false", "[", "{", '"\\u123z"']:
        try:
            json.loads(source)
            assert False, source
        except json.JSONDecodeError as error:
            assert error.doc == source and 0 <= error.pos <= len(source)
    try:
        json.loads('[\n 1,\n bad]')
        assert False
    except json.JSONDecodeError as error:
        assert error.pos == 7 and error.lineno == 3 and error.colno == 2
    try:
        json.loads("[[[]]]", max_depth=2)
        assert False
    except RecursionError:
        pass
    value, end = json.JSONDecoder().raw_decode("  [1] trailing", 2)
    assert value[0].as_int() == 1 and end == 5

    for literal in ["null", "true", "false", "NaN", "Infinity", "-Infinity"]:
        for prefix in ["", " ", chr(0xE9), chr(0x1234), chr(0x1F600)]:
            value, end = json.JSONDecoder().raw_decode(prefix + literal + "!", len(prefix))
            assert json.dumps(value) == literal and end == len(prefix) + len(literal)
        for length in range(1, len(literal)):
            try:
                json.loads(literal[:length])
                assert False
            except json.JSONDecodeError:
                pass

    for source, lineno, line_start in [("", 1, 0), ("123", 1, 0),
                                       ("a\nb\n", 3, 4), (chr(0x1F600) + "\n" + chr(0xE9), 2, 2)]:
        for position in [len(source), 100, 1000000]:
            error = json.JSONDecodeError("test", source, position)
            assert error.pos == position and error.doc == source
            assert error.lineno == lineno and error.colno == position - line_start + 1
            try:
                json.JSONDecoder().raw_decode(source, position)
                assert False
            except json.JSONDecodeError as error:
                assert error.msg == "Expecting value" and error.pos == position
                assert error.lineno == lineno and error.colno == position - line_start + 1

test_decode()

@test
def test_encode():
    assert json.dumps((None, True, False, 12, -1.25, "text")) == '[null, true, false, 12, -1.25, "text"]'
    value = json.loads('{"z": [1, 2], "a": {"b": true}}')
    assert json.dumps(value, sort_keys=True, separators=(",", ":")) == '{"a":{"b":true},"z":[1,2]}'
    assert json.dumps(value, indent=2) == '{\n  "z": [\n    1,\n    2\n  ],\n  "a": {\n    "b": true\n  }\n}'
    assert json.dumps([1, 2], indent="\t") == '[\n\t1,\n\t2\n]'
    assert json.dumps([1, 2], indent=0) == '[\n1,\n2\n]'
    assert json.dumps({"x": 1}) == '{"x": 1}'
    assert json.dumps(chr(0x1F600) + chr(0xE9)) == '"\\ud83d\\ude00\\u00e9"'
    assert json.dumps(chr(0x1F600), ensure_ascii=False) == '"' + chr(0x1F600) + '"'
    assert json.dumps('"\\\b\t\n\f\r\x00') == '"\\\"\\\\\\b\\t\\n\\f\\r\\u0000"'
    assert json.dumps(float("nan")) == "NaN"
    assert json.dumps(float("inf")) == "Infinity"
    assert json.dumps(-float("inf")) == "-Infinity"
    assert json.dumps(-0.0) == "-0.0"
    try:
        json.dumps(float("nan"), allow_nan=False)
        assert False
    except ValueError:
        pass
    array = JSON.array()
    array.append(array)
    try:
        json.dumps(array)
        assert False
    except ValueError as error:
        assert str(error) == "Circular reference detected"
    try:
        json.dumps(array, check_circular=False, max_depth=8)
        assert False
    except RecursionError:
        pass
    shared = JSON([1, 2])
    assert json.dumps(JSON((shared, shared))) == '[[1, 2], [1, 2]]'
    assert str(JSON(1)) == "1" and repr(JSON(1)) == "JSON(1)"

test_encode()

@test
def test_storage():
    assert JSON.__elemsize__ == 16 and not JSON.__atomic__
    for value in [0, 1, -1, -9223372036854775808, 9223372036854775807]:
        scalar = JSON(value)
        assert scalar.kind == "int" and scalar.as_int() == value
        assert scalar.as_float() == float(value)
        assert bool(scalar) == bool(value)
    for magnitude in [0, 1, 0x000FFFFFFFFFFFFF, 0x0010000000000000,
                      0x7FEFFFFFFFFFFFFF, 0x7FF0000000000000,
                      0x7FF8000000001234, 0x7FFFFFFFFFFFFFFF]:
        for negative in [False, True]:
            bits = magnitude | (-9223372036854775808 if negative else 0)
            scalar = JSON(json._bits_float(bits))
            assert scalar.kind == "float"
            assert json._float_bits(scalar.as_float()) == bits
    for text in ["", "a\0b", chr(0xE9), chr(0x1234), chr(0x1F600),
                 (chr(0x1F600) + "abc" + chr(0x1F600))[1:-1]]:
        scalar = JSON(text)
        restored = scalar.as_str()
        assert scalar.kind == "str" and len(scalar) == len(text)
        assert restored == text and restored._ptr == text._ptr
        assert restored._meta == text._meta
    assert not JSON() and not JSON(False) and not JSON(0.0) and not JSON(-0.0)
    assert not JSON(False).as_bool() and JSON(True).as_bool()
    assert JSON(0.0) == JSON(-0.0)
    assert JSON(float("nan")) != JSON(float("nan"))
    array = JSON.array()
    alias = array
    for index in range(4097):
        array.append(JSON((str(index), index)))
    native_list = [_decode_temporary() for index in range(64)]
    native_dict = {index: JSON((str(index), {"value": str(index)})) for index in range(64)}
    from C import GC_gcollect()
    GC_gcollect()
    assert len(alias) == 4097
    for index in range(4097):
        assert alias[index][0].as_str() == str(index)
        assert alias[index][1].as_int() == index
    for index in range(64):
        assert native_list[index][0].as_str() == "long value"
        assert native_list[index][1].as_int() == 2
        assert native_dict[index][0].as_str() == str(index)
        assert native_dict[index][1]["value"].as_str() == str(index)
    alias[100] = "changed"
    assert array[100].as_str() == "changed"
    obj = JSON({"one": 1, "two": 2})
    obj["one"] = 3
    del obj["two"]
    obj["three"] = 4
    assert list(obj.keys()) == ["one", "three"]
    assert JSON("same") == JSON("same")
    assert JSON([1, 2]) == JSON((1, 2))
    assert JSON({"a": 1, "b": 2}) == JSON({"b": 2, "a": 1})
    assert JSON(1) != JSON(1.0) and JSON(True) != JSON(1)
    optional: Optional[int] = 42
    assert JSON(optional).as_int() == 42
    optional = None
    assert JSON(optional).is_null()
    assert json.dumps(_decode_temporary()) == '["long value", 2]'

def _decode_temporary():
    return json.loads('["long value", 2]')

test_storage()

@test
def test_string_boundaries():
    for code, escaped in [(0xD800, '"\\ud800"'), (0xDC00, '"\\udc00"')]:
        value = JSON(chr(code))
        assert json.loads(escaped) == value
        assert json.dumps(value, ensure_ascii=True) == escaped
        assert json.dumps(value, ensure_ascii=False) == '"' + chr(code) + '"'
    assert json.loads('"\\ud800\\udc00"').as_str() == chr(0x10000)
    for offset in range(24):
        for code in range(256):
            text = "a" * offset + chr(code) + "tail" * 8
            assert json.loads(json.dumps(text)).as_str() == text
            assert json.loads(json.dumps(text, ensure_ascii=False)).as_str() == text
            if code < 32:
                source = '"' + text + '"'
                assert json.loads(source, strict=False).as_str() == text
                try:
                    json.loads(source)
                    assert False
                except json.JSONDecodeError as error:
                    assert error.pos == offset + 1
    for code in [0x100, 0x1234, 0xD800, 0xDC00, 0xFFFF, 0x10000, 0x10FFFF]:
        text = "ordinary text" * 20 + chr(code) + "\\\n\x00" + "ending" * 50
        assert json.loads(json.dumps(text, ensure_ascii=False)).as_str() == text
        assert json.loads(json.dumps(text)).as_str() == text
        assert json.dumps(text)._is_ascii()
    for size in [0, 1, 7, 8, 9, 15, 16, 31, 32, 255, 256, 257, 4096]:
        text = "x" * size
        assert json.loads('"' + text + '"').as_str() == text
        try:
            json.loads('"' + text)
            assert False
        except json.JSONDecodeError as error:
            assert error.pos == 0
    try:
        json.loads('"\\u00e9X\\u00e9')
        assert False
    except json.JSONDecodeError as error:
        assert error.msg == "Invalid \\uXXXX escape" and error.pos == 9

@test
def test_streams_and_options():
    from os import getpid, remove
    path = f"build/json-test-{getpid()}.json"
    value = JSON(("text", True, None, {"field": 10}))
    try:
        with open(path, "w", encoding="utf-8") as stream:
            json.dump(value, stream, indent=2)
        with open(path, "r", encoding="utf-8") as stream:
            assert json.load(stream) == value
        with open(path, "rb") as stream:
            assert json.load(stream) == value
    finally:
        remove(path)
    assert json.JSONEncoder().encode(value) == json.dumps(value)
    indent: Optional[int] = 2
    assert json.dumps([1], indent=indent) == '[\n  1\n]'
    indent = None
    assert json.dumps([1], indent=indent) == '[1]'
    assert json.dumps({"text": [1, 2]}, indent=chr(0x100), separators=(",", chr(0x1F600))).count(chr(0x1F600)) == 1
    assert json.loads(b"\xef\xbb\xbf[1]")[0].as_int() == 1
    assert json.loads('1'.encode("utf-16-be")).as_int() == 1
    assert json.loads('1'.encode("utf-16-le")).as_int() == 1
    try:
        json.loads(chr(0xFEFF) + "{}")
        assert False
    except json.JSONDecodeError as error:
        assert error.pos == 0
    try:
        json.loads(b'"\xff"')
        assert False
    except UnicodeDecodeError:
        pass
    try:
        json.loads("[" * 513 + "]" * 513)
        assert False
    except RecursionError:
        pass
    try:
        json.JSONDecoder().raw_decode("0", -1)
        assert False
    except ValueError:
        pass
    object_cycle = JSON.object()
    object_cycle["self"] = object_cycle
    try:
        json.dumps(object_cycle)
        assert False
    except ValueError:
        pass

test_string_boundaries()
test_streams_and_options()
