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
    f.write(f'hello world')                         # opt applied
    assert f.write_count == 2
    assert f.write_gen_count == 3
test_file_io()

@test
def test_print():
    from sys import stdout
    print('hello world')               # EXPECT: hello world
    print(str.cat("hello ", "world"))  # EXPECT: hello world
    a, b, c = 3.14, 'xyz', 42
    print(f'hello {a} world {b=} abc {a+c} zzz', file=stdout, sep='x')  # EXPECT: hello 3.14 world b=xyz abc 45.14 zzz
    print(f'hello', f'world', sep='x')  # EXPECT: helloxworld
test_print()
