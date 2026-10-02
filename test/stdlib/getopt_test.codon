import os
import getopt


@test
def test_empty():
    opts, args = getopt.getopt([], "")
    assert not opts
    assert not args

    opts, args = getopt.gnu_getopt([], "")
    assert not opts
    assert not args


@test
def test_short_options():
    opts, args = getopt.getopt(["-a"], "a")
    assert opts == [("-a", "")]
    assert not args

    opts, args = getopt.getopt(["-a", "-b"], "ab")
    assert opts == [("-a", ""), ("-b", "")]
    assert not args

    opts, args = getopt.getopt(["-abc"], "abc")
    assert opts == [("-a", ""), ("-b", ""), ("-c", "")]
    assert not args


@test
def test_short_attached_arguments():
    opts, args = getopt.getopt(["-afoo"], "a:")
    assert opts == [("-a", "foo")]
    assert not args

    opts, args = getopt.getopt(["-a="], "a:")
    assert opts == [("-a", "=")]
    assert not args

    opts, args = getopt.getopt(["-a-"], "a:")
    assert opts == [("-a", "-")]
    assert not args


@test
def test_short_separate_arguments():
    opts, args = getopt.getopt(["-a", "foo"], "a:")
    assert opts == [("-a", "foo")]
    assert not args

    # Option-looking strings are still consumed as arguments.
    opts, args = getopt.getopt(["-a", "-b"], "a:b")
    assert opts == [("-a", "-b")]
    assert not args

    # "--" is an argument here, not a terminator.
    opts, args = getopt.getopt(["-a", "--", "-b"], "a:b")
    assert opts == [("-a", "--"), ("-b", "")]
    assert not args

    opts, args = getopt.getopt(["-a", "-", "rest"], "a:")
    assert opts == [("-a", "-")]
    assert args == ["rest"]


@test
def test_short_empty_argument():
    opts, args = getopt.getopt(["-a", ""], "a:")
    assert opts == [("-a", "")]
    assert not args


@test
def test_short_cluster_argument():
    opts, args = getopt.getopt(["-abc"], "ab:")
    assert opts == [
        ("-a", ""),
        ("-b", "c"),
    ]
    assert not args

    # Once an option requiring an argument is encountered, the entire
    # remainder of the cluster is its argument.
    opts, args = getopt.getopt(["-abc"], "a:b:c")
    assert opts == [("-a", "bc")]
    assert not args


@test
def test_short_repeated():
    opts, args = getopt.getopt(["-a", "-a", "-aaa"], "a")
    assert opts == [
        ("-a", ""),
        ("-a", ""),
        ("-a", ""),
        ("-a", ""),
        ("-a", ""),
    ]
    assert not args


@test
def test_short_argument_special_characters():
    opts, args = getopt.getopt(["-a=foo"], "a:")
    assert opts == [("-a", "=foo")]
    assert not args

    opts, args = getopt.getopt(["-a:::"], "a:")
    assert opts == [("-a", ":::")]
    assert not args


@test
def test_short_errors():
    try:
        getopt.getopt(["-x"], "abc")
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option -x not recognized"
        assert e.opt == "x"

    try:
        getopt.getopt(["-a"], "a:")
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option -a requires argument"
        assert e.opt == "a"

    try:
        getopt.getopt(["-:"], ":")
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option -: not recognized"
        assert e.opt == ":"


@test
def test_short_option_colon_positions():
    # ':' is syntax, never itself an option.
    for shortopts in [":a", "a:", "::a", "a::"]:
        try:
            getopt.getopt(["-:"], shortopts)
            assert False
        except getopt.GetoptError as e:
            assert e.msg == "option -: not recognized"
            assert e.opt == ":"


@test
def test_short_double_colon_is_still_required_argument():
    # In Python 3.13, "::" has no optional-argument meaning.
    opts, args = getopt.getopt(
        ["-a", "value"],
        "a::",
    )
    assert opts == [("-a", "value")]
    assert not args

    opts, args = getopt.getopt(
        ["-avalue"],
        "a::",
    )
    assert opts == [("-a", "value")]
    assert not args

    try:
        getopt.getopt(["-a"], "a::")
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option -a requires argument"
        assert e.opt == "a"


@test
def test_long_options():
    opts, args = getopt.getopt(["--foo"], "", ["foo"])
    assert opts == [("--foo", "")]
    assert not args

    opts, args = getopt.getopt(
        ["--foo", "--bar"],
        "",
        ["foo", "bar"],
    )
    assert opts == [
        ("--foo", ""),
        ("--bar", ""),
    ]
    assert not args


@test
def test_long_attached_arguments():
    opts, args = getopt.getopt(["--foo=bar"], "", ["foo="])
    assert opts == [("--foo", "bar")]
    assert not args

    # Explicit empty argument.
    opts, args = getopt.getopt(["--foo="], "", ["foo="])
    assert opts == [("--foo", "")]
    assert not args

    # Split only on the first '='.
    opts, args = getopt.getopt(
        ["--foo=a=b=c"],
        "",
        ["foo="],
    )
    assert opts == [("--foo", "a=b=c")]
    assert not args

    opts, args = getopt.getopt(
        ["--foo=="],
        "",
        ["foo="],
    )
    assert opts == [("--foo", "=")]
    assert not args


@test
def test_long_separate_arguments():
    opts, args = getopt.getopt(["--foo", "bar"], "", ["foo="])
    assert opts == [("--foo", "bar")]
    assert not args

    opts, args = getopt.getopt(["--foo", ""], "", ["foo="])
    assert opts == [("--foo", "")]
    assert not args

    # Option-looking strings are valid arguments.
    opts, args = getopt.getopt(
        ["--foo", "--bar"],
        "",
        ["foo=", "bar"],
    )
    assert opts == [("--foo", "--bar")]
    assert not args

    # "--" is consumed as the required argument.
    opts, args = getopt.getopt(
        ["--foo", "--", "-a"],
        "a",
        ["foo="],
    )
    assert opts == [
        ("--foo", "--"),
        ("-a", ""),
    ]
    assert not args

    opts, args = getopt.getopt(
        ["--foo", "-", "rest"],
        "",
        ["foo="],
    )
    assert opts == [("--foo", "-")]
    assert args == ["rest"]


@test
def test_long_argument_errors():
    try:
        getopt.getopt(["--foo"], "", ["foo="])
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foo requires argument"
        assert e.opt == "foo"

    try:
        getopt.getopt(["--foo=x"], "", ["foo"])
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foo must not have an argument"
        assert e.opt == "foo"

    # An explicitly empty argument is still an argument.
    try:
        getopt.getopt(["--foo="], "", ["foo"])
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foo must not have an argument"
        assert e.opt == "foo"


@test
def test_long_unrecognized():
    try:
        getopt.getopt(["--foo"], "", ["bar"])
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foo not recognized"
        assert e.opt == "foo"

    try:
        getopt.getopt(["--foo=x"], "", ["bar="])
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foo not recognized"
        assert e.opt == "foo"


@test
def test_long_unique_prefix_no_argument():
    opts, args = getopt.getopt(
        ["--fo"],
        "",
        ["foobar"],
    )
    assert opts == [("--foobar", "")]
    assert not args


@test
def test_long_unique_prefix_required_argument():
    # Prefix expansion does not change whether an argument is required.
    try:
        getopt.getopt(
            ["--fo"],
            "",
            ["foobar="],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foobar requires argument"
        assert e.opt == "foobar"

    opts, args = getopt.getopt(
        ["--fo", "value"],
        "",
        ["foobar="],
    )
    assert opts == [("--foobar", "value")]
    assert not args

    opts, args = getopt.getopt(
        ["--fo=value"],
        "",
        ["foobar="],
    )
    assert opts == [("--foobar", "value")]
    assert not args

    opts, args = getopt.getopt(
        ["--fo="],
        "",
        ["foobar="],
    )
    assert opts == [("--foobar", "")]
    assert not args


@test
def test_long_ambiguous_prefix():
    try:
        getopt.getopt(
            ["--fo"],
            "",
            ["foo", "foobar"],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --fo not a unique prefix"
        assert e.opt == "fo"

    try:
        getopt.getopt(
            ["--fo=x"],
            "",
            ["foobar=", "foobaz="],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --fo not a unique prefix"
        assert e.opt == "fo"


@test
def test_long_exact_match_wins():
    # Exact "foo" wins over prefix "foobar".
    opts, args = getopt.getopt(
        ["--foo"],
        "",
        ["foo", "foobar"],
    )
    assert opts == [("--foo", "")]
    assert not args

    # Exact "foo=" wins over "foobar=".
    opts, args = getopt.getopt(
        ["--foo=x"],
        "",
        ["foo=", "foobar="],
    )
    assert opts == [("--foo", "x")]
    assert not args


@test
def test_long_exact_match_overrides_ambiguity():
    opts, args = getopt.getopt(
        ["--foo"],
        "",
        ["foobar", "foobaz", "foo", "fooz"],
    )
    assert opts == [("--foo", "")]
    assert not args

    opts, args = getopt.getopt(
        ["--foo=x"],
        "",
        ["foobar=", "foo=", "foobaz="],
    )
    assert opts == [("--foo", "x")]
    assert not args


@test
def test_long_noarg_and_arg_same_name():
    # "foo" is checked before "foo=".
    opts, args = getopt.getopt(
        ["--foo"],
        "",
        ["foo", "foo="],
    )
    assert opts == [("--foo", "")]
    assert not args

    # Even when an argument is supplied, the exact "foo" entry wins.
    try:
        getopt.getopt(
            ["--foo=x"],
            "",
            ["foo", "foo="],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --foo must not have an argument"
        assert e.opt == "foo"


@test
def test_long_prefix_nonalpha_regression():
    opts, args = getopt.getopt(
        ["--foo=42"],
        "",
        ["foo-bar", "foo="],
    )
    assert opts == [("--foo", "42")]
    assert not args


@test
def test_long_prefix_mixed_argument_requirements():
    # Both possibilities match and neither is exact.
    try:
        getopt.getopt(
            ["--fo"],
            "",
            ["foo", "foobar="],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --fo not a unique prefix"
        assert e.opt == "fo"


@test
def test_long_prefix_with_equals_mixed_argument_requirements():
    # Supplying an argument does not resolve an otherwise ambiguous prefix.
    try:
        getopt.getopt(
            ["--fo=x"],
            "",
            ["foo=", "foobar"],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --fo not a unique prefix"
        assert e.opt == "fo"


@test
def test_long_option_prefix_is_full_name():
    opts, args = getopt.getopt(
        ["--foo"],
        "",
        ["foo", "foobar"],
    )
    assert opts == [("--foo", "")]
    assert not args

    opts, args = getopt.getopt(
        ["--foobar"],
        "",
        ["foo", "foobar"],
    )
    assert opts == [("--foobar", "")]
    assert not args


@test
def test_long_option_argument_prefix_is_full_name():
    opts, args = getopt.getopt(
        ["--foo=x"],
        "",
        ["foo=", "foobar="],
    )
    assert opts == [("--foo", "x")]
    assert not args

    opts, args = getopt.getopt(
        ["--foobar=x"],
        "",
        ["foo=", "foobar="],
    )
    assert opts == [("--foobar", "x")]
    assert not args


@test
def test_duplicate_long_options():
    opts, args = getopt.getopt(
        ["--foo"],
        "",
        ["foo", "foo"],
    )
    assert opts == [("--foo", "")]
    assert not args

    opts, args = getopt.getopt(
        ["--foo=x"],
        "",
        ["foo=", "foo="],
    )
    assert opts == [("--foo", "x")]
    assert not args


@test
def test_long_repeated():
    opts, args = getopt.getopt(
        ["--foo", "--foo", "--foo"],
        "",
        ["foo"],
    )
    assert opts == [
        ("--foo", ""),
        ("--foo", ""),
        ("--foo", ""),
    ]
    assert not args


@test
def test_longopts_string():
    opts, args = getopt.getopt(["--foo"], "", "foo")
    assert opts == [("--foo", "")]
    assert not args

    opts, args = getopt.getopt(["--foo=x"], "", "foo=")
    assert opts == [("--foo", "x")]
    assert not args


@test
def test_extra_leading_hyphens():
    opts, args = getopt.getopt(
        ["---foo"],
        "",
        ["-foo"],
    )
    assert opts == [("---foo", "")]
    assert not args

    opts, args = getopt.getopt(
        ["---foo=x"],
        "",
        ["-foo="],
    )
    assert opts == [("---foo", "x")]
    assert not args


@test
def test_empty_long_option():
    # "--" itself is always the terminator.
    opts, args = getopt.getopt(
        ["--", "foo"],
        "",
        [""],
    )
    assert not opts
    assert args == ["foo"]


@test
def test_option_argument_can_be_empty_string():
    opts, args = getopt.getopt(
        ["-a", ""],
        "a:",
    )
    assert opts == [("-a", "")]
    assert not args

    opts, args = getopt.getopt(
        ["--foo", ""],
        "",
        ["foo="],
    )
    assert opts == [("--foo", "")]
    assert not args


@test
def test_mixed_options():
    opts, args = getopt.getopt(
        ["-a", "--foo", "-bbar", "--baz=qux"],
        "ab:",
        ["foo", "baz="],
    )
    assert opts == [
        ("-a", ""),
        ("--foo", ""),
        ("-b", "bar"),
        ("--baz", "qux"),
    ]
    assert not args


@test
def test_double_dash():
    opts, args = getopt.getopt(
        ["-a", "--", "-b", "--foo", "bar"],
        "ab",
        ["foo"],
    )
    assert opts == [("-a", "")]
    assert args == ["-b", "--foo", "bar"]

    opts, args = getopt.getopt(
        ["--", "-a", "foo"],
        "a",
    )
    assert not opts
    assert args == ["-a", "foo"]


@test
def test_single_dash():
    opts, args = getopt.getopt(["-"], "a")
    assert not opts
    assert args == ["-"]

    # getopt stops at the first positional argument.
    opts, args = getopt.getopt(["-", "-a"], "a")
    assert not opts
    assert args == ["-", "-a"]


@test
def test_getopt_stops_at_first_positional():
    opts, args = getopt.getopt(
        ["-a", "first", "-b", "--foo"],
        "ab",
        ["foo"],
    )
    assert opts == [("-a", "")]
    assert args == ["first", "-b", "--foo"]

    opts, args = getopt.getopt(
        ["first", "-a"],
        "a",
    )
    assert not opts
    assert args == ["first", "-a"]

    opts, args = getopt.getopt(
        ["", "-a"],
        "a",
    )
    assert not opts
    assert args == ["", "-a"]


@test
def test_getopt_plus_has_no_special_meaning():
    # Leading '+' is a gnu_getopt feature, not a getopt feature.
    opts, args = getopt.getopt(
        ["-a", "first", "-b"],
        "+ab",
    )
    assert opts == [("-a", "")]
    assert args == ["first", "-b"]


@test
def test_getopt_cpython_example():
    cmdline = [
        "-a", "1",
        "-b",
        "--alpha=2",
        "--beta",
        "-a", "3",
        "-a", "",
        "--beta",
        "arg1", "arg2",
    ]

    opts, args = getopt.getopt(
        cmdline,
        "a:b",
        ["alpha=", "beta"],
    )

    assert opts == [
        ("-a", "1"),
        ("-b", ""),
        ("--alpha", "2"),
        ("--beta", ""),
        ("-a", "3"),
        ("-a", ""),
        ("--beta", ""),
    ]
    assert args == ["arg1", "arg2"]


@test
def test_getopt_library_example_short():
    args = "-a -b -cfoo -d bar a1 a2".split()

    opts, args = getopt.getopt(args, "abc:d:")

    assert opts == [
        ("-a", ""),
        ("-b", ""),
        ("-c", "foo"),
        ("-d", "bar"),
    ]
    assert args == ["a1", "a2"]


@test
def test_getopt_library_example_long():
    args = (
        "--condition=foo --testing "
        "--output-file abc.def -x a1 a2"
    ).split()

    opts, args = getopt.getopt(
        args,
        "x",
        ["condition=", "output-file=", "testing"],
    )

    assert opts == [
        ("--condition", "foo"),
        ("--testing", ""),
        ("--output-file", "abc.def"),
        ("-x", ""),
    ]
    assert args == ["a1", "a2"]


@test
def test_gnu_intermixed():
    opts, args = getopt.gnu_getopt(
        ["first", "-a", "second", "--foo", "third"],
        "a",
        ["foo"],
    )
    assert opts == [
        ("-a", ""),
        ("--foo", ""),
    ]
    assert args == [
        "first",
        "second",
        "third",
    ]


@test
def test_gnu_preserves_positional_order():
    opts, args = getopt.gnu_getopt(
        [
            "one",
            "-a",
            "two",
            "three",
            "-b",
            "four",
            "--foo",
            "five",
        ],
        "ab",
        ["foo"],
    )

    assert opts == [
        ("-a", ""),
        ("-b", ""),
        ("--foo", ""),
    ]
    assert args == [
        "one",
        "two",
        "three",
        "four",
        "five",
    ]


@test
def test_gnu_arguments():
    opts, args = getopt.gnu_getopt(
        [
            "one",
            "-a", "short-value",
            "two",
            "--foo", "long-value",
            "three",
        ],
        "a:",
        ["foo="],
    )

    assert opts == [
        ("-a", "short-value"),
        ("--foo", "long-value"),
    ]
    assert args == [
        "one",
        "two",
        "three",
    ]


@test
def test_gnu_attached_arguments():
    opts, args = getopt.gnu_getopt(
        [
            "one",
            "-avalue",
            "two",
            "--foo=value",
            "three",
        ],
        "a:",
        ["foo="],
    )

    assert opts == [
        ("-a", "value"),
        ("--foo", "value"),
    ]
    assert args == ["one", "two", "three"]


@test
def test_gnu_double_dash():
    opts, args = getopt.gnu_getopt(
        [
            "one",
            "-a",
            "two",
            "--",
            "-b",
            "--foo",
            "three",
        ],
        "ab",
        ["foo"],
    )

    assert opts == [("-a", "")]
    assert args == [
        "one",
        "two",
        "-b",
        "--foo",
        "three",
    ]


@test
def test_gnu_single_dash():
    opts, args = getopt.gnu_getopt(
        ["-", "-a", "-", "--foo"],
        "a",
        ["foo"],
    )

    assert opts == [
        ("-a", ""),
        ("--foo", ""),
    ]
    assert args == ["-", "-"]


@test
def test_gnu_single_dash_as_option_argument():
    opts, args = getopt.gnu_getopt(
        ["-a", "-", "-b", "-"],
        "ab:",
    )

    assert opts == [
        ("-a", ""),
        ("-b", "-"),
    ]
    assert args == ["-"]


@test
def test_gnu_single_dash_before_terminator():
    opts, args = getopt.gnu_getopt(
        ["-", "-a", "--", "-b"],
        "ab",
    )
    assert opts == [("-a", "")]
    assert args == ["-", "-b"]


@test
def test_gnu_plus_mode():
    opts, args = getopt.gnu_getopt(
        ["-a", "first", "-b", "--foo"],
        "+ab",
        ["foo"],
    )

    assert opts == [("-a", "")]
    assert args == [
        "first",
        "-b",
        "--foo",
    ]

    opts, args = getopt.gnu_getopt(
        ["first", "-a"],
        "+a",
    )
    assert not opts
    assert args == ["first", "-a"]


@test
def test_gnu_plus_removed_from_shortopts():
    # '+' controls parsing but is not itself part of the short option spec.
    try:
        getopt.gnu_getopt(
            ["-+"],
            "+a",
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option -+ not recognized"
        assert e.opt == "+"


@test
def test_gnu_cpython_example():
    cmdline = [
        "-a",
        "arg1",
        "-b", "1",
        "--alpha",
        "--beta=2",
    ]

    opts, args = getopt.gnu_getopt(
        cmdline,
        "ab:",
        ["alpha", "beta="],
    )

    assert opts == [
        ("-a", ""),
        ("-b", "1"),
        ("--alpha", ""),
        ("--beta", "2"),
    ]
    assert args == ["arg1"]


@test
def test_gnu_longopts_string():
    opts, args = getopt.gnu_getopt(
        ["one", "--foo", "two"],
        "",
        "foo",
    )

    assert opts == [("--foo", "")]
    assert args == ["one", "two"]


@test
def test_gnu_errors():
    try:
        getopt.gnu_getopt(["foo", "-x"], "a")
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option -x not recognized"
        assert e.opt == "x"

    try:
        getopt.gnu_getopt(
            ["foo", "--bar"],
            "",
            ["baz"],
        )
        assert False
    except getopt.GetoptError as e:
        assert e.msg == "option --bar not recognized"
        assert e.opt == "bar"


@test
def test_gnu_empty_long_argument():
    opts, args = getopt.gnu_getopt(
        ["before", "--foo=", "after"],
        "",
        ["foo="],
    )

    assert opts == [("--foo", "")]
    assert args == ["before", "after"]


@test
def test_gnu_posixly_correct():
    existed = "POSIXLY_CORRECT" in os.environ
    old = os.environ.get("POSIXLY_CORRECT", "")

    os.environ["POSIXLY_CORRECT"] = "1"

    opts, args = getopt.gnu_getopt(
        ["-a", "first", "-b", "--foo"],
        "ab",
        ["foo"],
    )

    assert opts == [("-a", "")]
    assert args == [
        "first",
        "-b",
        "--foo",
    ]

    if existed:
        os.environ["POSIXLY_CORRECT"] = old
    else:
        del os.environ["POSIXLY_CORRECT"]


@test
def test_gnu_posixly_correct_empty():
    # An empty value is false because CPython uses
    # os.environ.get("POSIXLY_CORRECT").
    existed = "POSIXLY_CORRECT" in os.environ
    old = os.environ.get("POSIXLY_CORRECT", "")

    os.environ["POSIXLY_CORRECT"] = ""

    opts, args = getopt.gnu_getopt(
        ["first", "-a"],
        "a",
    )

    assert opts == [("-a", "")]
    assert args == ["first"]

    if existed:
        os.environ["POSIXLY_CORRECT"] = old
    else:
        del os.environ["POSIXLY_CORRECT"]


@test
def test_gnu_posixly_correct_arbitrary_value():
    # Any nonempty value is true, including "0".
    existed = "POSIXLY_CORRECT" in os.environ
    old = os.environ.get("POSIXLY_CORRECT", "")

    os.environ["POSIXLY_CORRECT"] = "0"

    opts, args = getopt.gnu_getopt(
        ["first", "-a"],
        "a",
    )

    assert not opts
    assert args == ["first", "-a"]

    if existed:
        os.environ["POSIXLY_CORRECT"] = old
    else:
        del os.environ["POSIXLY_CORRECT"]


@test
def test_gnu_plus_with_posixly_correct():
    existed = "POSIXLY_CORRECT" in os.environ
    old = os.environ.get("POSIXLY_CORRECT", "")

    os.environ["POSIXLY_CORRECT"] = "1"

    opts, args = getopt.gnu_getopt(
        ["-a", "first", "-b"],
        "+ab",
    )

    assert opts == [("-a", "")]
    assert args == ["first", "-b"]

    if existed:
        os.environ["POSIXLY_CORRECT"] = old
    else:
        del os.environ["POSIXLY_CORRECT"]


@test
def test_error():
    e = getopt.GetoptError(
        "option -a not recognized",
        "a",
    )

    assert e.msg == "option -a not recognized"
    assert e.opt == "a"
    assert str(e) == "option -a not recognized"

    e = getopt.GetoptError("hello")
    assert e.msg == "hello"
    assert e.opt == ""
    assert str(e) == "hello"

    assert getopt.error is getopt.GetoptError


@test
def test_getopt_error_args():
    e = getopt.GetoptError("message", "x")

    assert e.msg == "message"
    assert e.opt == "x"
    assert str(e) == "message"
    assert len(e.args) == 2
    assert e.args[0] == "message"
    assert e.args[1] == "x"


@test
def test_getopt_error_args_default_opt():
    e = getopt.GetoptError("message")

    assert len(e.args) == 2
    assert e.args[0] == "message"
    assert e.args[1] == ""


@test
def test_input_not_modified():
    argv = ["-a", "foo"]
    opts, args = getopt.getopt(argv, "a")

    assert opts == [("-a", "")]
    assert args == ["foo"]
    assert argv == ["-a", "foo"]

    argv = ["foo", "-a", "bar"]
    opts, args = getopt.gnu_getopt(argv, "a")

    assert opts == [("-a", "")]
    assert args == ["foo", "bar"]
    assert argv == ["foo", "-a", "bar"]


@test
def test_unicode():
    opts, args = getopt.getopt(
        ["--name=こんにちは", "🙂"],
        "",
        ["name="],
    )
    assert opts == [("--name", "こんにちは")]
    assert args == ["🙂"]

    opts, args = getopt.getopt(
        ["--名前=値"],
        "",
        ["名前="],
    )
    assert opts == [("--名前", "値")]
    assert not args

    opts, args = getopt.getopt(
        ["-é"],
        "é",
    )
    assert opts == [("-é", "")]
    assert not args


@test
def test_complex_getopt():
    opts, args = getopt.getopt(
        [
            "-abc",
            "-dvalue",
            "--foo",
            "--bar=value",
            "--baz",
            "first",
            "-x",
            "--ignored",
        ],
        "abcd:",
        ["foo", "bar=", "baz"],
    )

    assert opts == [
        ("-a", ""),
        ("-b", ""),
        ("-c", ""),
        ("-d", "value"),
        ("--foo", ""),
        ("--bar", "value"),
        ("--baz", ""),
    ]
    assert args == [
        "first",
        "-x",
        "--ignored",
    ]


@test
def test_complex_gnu_getopt():
    opts, args = getopt.gnu_getopt(
        [
            "first",
            "-abc",
            "second",
            "-dvalue",
            "--foo",
            "third",
            "--bar=value",
            "fourth",
            "--baz",
            "fifth",
        ],
        "abcd:",
        ["foo", "bar=", "baz"],
    )

    assert opts == [
        ("-a", ""),
        ("-b", ""),
        ("-c", ""),
        ("-d", "value"),
        ("--foo", ""),
        ("--bar", "value"),
        ("--baz", ""),
    ]
    assert args == [
        "first",
        "second",
        "third",
        "fourth",
        "fifth",
    ]


test_empty()
test_short_options()
test_short_attached_arguments()
test_short_separate_arguments()
test_short_empty_argument()
test_short_cluster_argument()
test_short_repeated()
test_short_argument_special_characters()
test_short_errors()
test_short_option_colon_positions()
test_short_double_colon_is_still_required_argument()
test_long_options()
test_long_attached_arguments()
test_long_separate_arguments()
test_long_argument_errors()
test_long_unrecognized()
test_long_unique_prefix_no_argument()
test_long_unique_prefix_required_argument()
test_long_ambiguous_prefix()
test_long_exact_match_wins()
test_long_exact_match_overrides_ambiguity()
test_long_noarg_and_arg_same_name()
test_long_prefix_nonalpha_regression()
test_long_prefix_mixed_argument_requirements()
test_long_prefix_with_equals_mixed_argument_requirements()
test_long_option_prefix_is_full_name()
test_long_option_argument_prefix_is_full_name()
test_duplicate_long_options()
test_long_repeated()
test_longopts_string()
test_extra_leading_hyphens()
test_empty_long_option()
test_option_argument_can_be_empty_string()
test_mixed_options()
test_double_dash()
test_single_dash()
test_getopt_stops_at_first_positional()
test_getopt_plus_has_no_special_meaning()
test_getopt_cpython_example()
test_getopt_library_example_short()
test_getopt_library_example_long()
test_gnu_intermixed()
test_gnu_preserves_positional_order()
test_gnu_arguments()
test_gnu_attached_arguments()
test_gnu_double_dash()
test_gnu_single_dash()
test_gnu_single_dash_as_option_argument()
test_gnu_single_dash_before_terminator()
test_gnu_plus_mode()
test_gnu_plus_removed_from_shortopts()
test_gnu_cpython_example()
test_gnu_longopts_string()
test_gnu_errors()
test_gnu_empty_long_argument()
test_gnu_posixly_correct()
test_gnu_posixly_correct_empty()
test_gnu_posixly_correct_arbitrary_value()
test_gnu_plus_with_posixly_correct()
test_error()
test_getopt_error_args()
test_getopt_error_args_default_opt()
test_input_not_modified()
test_unicode()
test_complex_getopt()
test_complex_gnu_getopt()
