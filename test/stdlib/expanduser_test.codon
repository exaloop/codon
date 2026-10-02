import os
import os.path as path
import internal.static as static

lookup_error = 0
lookup_calls = 0
lookup_name = ""
lookup_home: Optional[str] = None

if static.platform() != "win32":
    @export
    def seq_os_home(name: cobj) -> cobj:
        global lookup_calls, lookup_name
        lookup_calls += 1
        lookup_name = bytes.from_ptr(name).decode() if name else ""
        if static.platform() == "darwin":
            from C import __error() -> Ptr[i32]
            __error()[0] = i32(lookup_error)
        else:
            from C import __errno_location() -> Ptr[i32]
            __errno_location()[0] = i32(lookup_error)
        if lookup_home is not None:
            from C import strdup(cobj) -> cobj
            return strdup(os._cstring(unwrap(lookup_home)))
        return cobj()


@test
def test_expanduser_lookup_failures():
    global lookup_error
    old_home = os.environ.get("HOME")
    try:
        os.environ.pop("HOME", None)
        for error in [0, 22, 5]:
            lookup_error = error
            for original, name in [
                ("~", ""),
                ("~/child", ""),
                ("~missing", "missing"),
                ("~missing/child", "missing"),
                ("~missing//child/", "missing"),
            ]:
                before = lookup_calls
                assert path.expanduser(original) == original
                assert lookup_calls == before + 1
                assert lookup_name == name

        os.environ["HOME"] = "/configured/home/"
        before = lookup_calls
        assert path.expanduser("~") == "/configured/home"
        assert path.expanduser("~/child") == "/configured/home/child"
        assert path.expanduser("relative/~missing") == "relative/~missing"
        assert lookup_calls == before
        assert path.expanduser("~missing/child") == "~missing/child"
        assert lookup_calls == before + 1 and lookup_name == "missing"
    finally:
        if old_home is None:
            os.environ.pop("HOME", None)
        else:
            os.environ["HOME"] = unwrap(old_home)


@test
def test_expanduser_lookup_success():
    global lookup_error, lookup_home
    old_home = os.environ.get("HOME")
    try:
        os.environ.pop("HOME", None)
        lookup_error = 0
        lookup_home = "/lookup/home/"
        for original, name, expected in [
            ("~", "", "/lookup/home"),
            ("~/child", "", "/lookup/home/child"),
            ("~known", "known", "/lookup/home"),
            ("~known/child", "known", "/lookup/home/child"),
            ("~known//child/", "known", "/lookup/home//child/"),
        ]:
            before = lookup_calls
            assert path.expanduser(original) == expected
            assert lookup_calls == before + 1
            assert lookup_name == name

        os.environ["HOME"] = "/configured/home/"
        before = lookup_calls
        assert path.expanduser("~") == "/configured/home"
        assert path.expanduser("~/child") == "/configured/home/child"
        assert lookup_calls == before
        assert path.expanduser("~known/child") == "/lookup/home/child"
        assert lookup_calls == before + 1 and lookup_name == "known"
        lookup_home = "/"
        assert path.expanduser("~known") == "/"
        assert path.expanduser("~known/child") == "/child"
    finally:
        lookup_home = None
        if old_home is None:
            os.environ.pop("HOME", None)
        else:
            os.environ["HOME"] = unwrap(old_home)


if static.platform() != "win32":
    test_expanduser_lookup_failures()
    test_expanduser_lookup_success()
