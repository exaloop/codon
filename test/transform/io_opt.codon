class DummyFile:
    write_count: int = 0
    write_gen_count: int = 0

    def write(self, s: str):
        self.write_count += 1
    def __file_write_gen__(self, g):
        self.write_gen_count += 1

@test
def test_file_io():
    f = DummyFile()
    f.write('')                                     # opt not applied
    f.write('hello world')                          # opt not applied
    f.write(str.cat("hello ", "world"))             # opt applied
    a, b, c = 3.14, 'xyz', 42
    f.write(f'hello {a} world {b=} abc {a+c} zzz')  # opt applied
    f.write(f'hello world')                         # opt not applied
    assert f.write_count == 3
    assert f.write_gen_count == 2
test_file_io()

@test
def test_write_call_shapes():
    @noinline
    def write():
        return "free function"

    class Writer:
        @noinline
        def write(self):
            return "method"

    class RepeatedWriter:
        @noinline
        def write(self, text: str, count: int):
            return text * count

        def __file_write_gen__(self, values):
            return "unexpected rewrite"

    assert write() == "free function"
    assert Writer().write() == "method"
    assert RepeatedWriter().write(str.cat("a", "b"), 2) == "abab"
test_write_call_shapes()

@test
def test_unrelated_writes():
    class CustomWriter:
        @noinline
        def write(self, text: str):
            return "method:" + text

        def __file_write_gen__(self, values):
            return "method:" + str.cat(values)

    @noinline
    def write(receiver, text: str):
        return "function:" + text

    writer = CustomWriter()
    assert write(writer, str.cat("a", "b")) == "function:ab"
    assert writer.write(str.cat("a", "b")) == "method:ab"
test_unrelated_writes()

@test
def test_write_contract():
    class CountingWriter:
        data: str = ""
        write_count: int = 0
        write_gen_count: int = 0

        @noinline
        def write(self, text: str) -> int:
            self.write_count += 1
            self.data += text
            return len(text)

        def __file_write_gen__(self, values: Generator[str]) -> int:
            self.write_gen_count += 1
            count = 0
            for value in values:
                self.data += value
                count += len(value)
            return count

    @noinline
    def receiver(writer: CountingWriter, events: list[str]):
        events.append("receiver")
        return writer

    @noinline
    def piece(text: str, events: list[str]):
        events.append(text)
        return text

    @noinline
    def write(writer: CountingWriter, text: str):
        return "function:" + text

    @noinline
    def cat():
        return "zero"

    class OtherString:
        @noinline
        def cat(text: str):
            return "other:" + text

    writer = CountingWriter()
    events: list[str] = []
    count = receiver(writer, events).write(
        str.cat(piece("ab", events), piece("cd", events))
    )
    assert count == 4
    assert events == ["receiver", "ab", "cd"]
    assert writer.data == "abcd"
    assert writer.write_count == 0 and writer.write_gen_count == 1
    assert writer.write(str.cat()) == 0
    assert writer.write_count == 1 and writer.write_gen_count == 1
    assert write(writer, str.cat("a", "b")) == "function:ab"
    assert writer.data == "abcd"
    assert writer.write(cat()) == 4
    assert writer.write(OtherString.cat("x")) == 7
    assert writer.write(str.cat(["e", "f"])) == 2
    assert writer.data == "abcdzeroother:xef"
    assert writer.write_count == 4 and writer.write_gen_count == 1

    def failing_pieces(events: list[str]):
        events.append("first")
        yield "prefix"
        events.append("error")
        raise ValueError("late failure")

    events.clear()
    writer = CountingWriter()
    try:
        writer.write(str.cat(failing_pieces(events)))
        assert False
    except ValueError as error:
        assert str(error) == "late failure"
    assert events == ["first", "error"]
    assert writer.data == ""
    assert writer.write_count == 0 and writer.write_gen_count == 0

    class MismatchedWriter:
        @noinline
        def write(self, text: str) -> int:
            return len(text)

        def __file_write_gen__(self, values: Generator[str]):
            pass

    class MissingHelperWriter:
        @noinline
        def write(self, text: str) -> int:
            return len(text)

    assert MismatchedWriter().write(str.cat("ab", "cd")) == 4
    assert MissingHelperWriter().write(str.cat("ab", "cd")) == 4
test_write_contract()

@test
def test_print():
    from sys import stdout
    print('hello world')               # EXPECT: hello world
    print(str.cat("hello ", "world"))  # EXPECT: hello world
    a, b, c = 3.14, 'xyz', 42
    print(f'hello {a} world {b=} abc {a+c} zzz', file=stdout, sep='x')  # EXPECT: hello 3.14 world b='xyz' abc 45.14 zzz
    print(f'hello', f'world', sep='x')  # EXPECT: helloxworld
test_print()
