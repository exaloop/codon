Language Primer
===============

If you know Python, you already know 95% of Seq. The following primer
assumes some familiarity with Python or at least one "modern"
programming language (QBASIC doesn't count).

Printing
--------

.. code:: seq

    print('hello world')

    from sys import stderr
    print('hello world', end='', file=stderr)

Comments
--------

.. code:: seq

    # Seq comments start with "# 'and go until the end of the line

    """
    There are no multi-line comments. You can (ab)use the docstring operator (''')
    if you really need them.
    """

Literals
--------

Seq is a strongly typed language like C++, Java, or C#. That means each
expression must have a type that can be inferred at the compile-time.

.. code:: seq

    # Booleans
    True  # type: bool
    False

    # Numbers
    a = 1  # type: int. It's 64-bit signed integer.
    b = 1.12  # type: float. Seq's float is identical to C's double.
    c = 5u  # type: int, but unsigned
    d = Int[8](12)  # 8-bit signed integer; you can go all the way to Int[2048]
    e = UInt[8](200)  # 8-bit unsigned integer
    f = byte(3)  # a byte is C's char; equivalent to Int[8]

    h = 0x12AF  # hexadecimal integers are also welcome
    h = 0XAF12
    g = 3.11e+9  # scientific notation is also supported
    g = .223  # and this is also float
    g = .11E-1  # and this as well

    # Strings
    s = 'hello! "^_^" '  # type: str.
    t = "hello there! \t \\ '^_^' "  # \t is a tab character; \\ stands for \
    raw = r"hello\n"  # raw strings do not escape slashes; this would print "hello\n"
    fstr = f"a is {a + 1}"  # an F-string; prints "a is 2"
    fstr = f"hi! {a+1=}"  # an F-string; prints "hi! a+1=2"
    t = """
    hello!
    multiline string
    """

    # The following escape sequences are supported:
    #   \\, \', \", \a, \b, \f, \n, \r, \t, \v,
    #   \xHHH (HHH is hex code), \OOO (OOO is octal code)

    # Sequence types
    dna = s"ACGT"  # type: seq. These are DNA sequences.
    prt = p"MYX"  # type: bio.pseq. These are protein sequences.
    kmer = k"ACGT"  # type: Kmer[4]. Note that Kmer[5] is different than Kmer[12].

Tuples
~~~~~~

.. code:: seq

    # Tuples
    t = (1, 2.3, 'hi')  # type: Tuple[int, float, str].
    t[1]  # type: float
    u = (1, )  # type: Tuple[int]

As all types must be known at compile time, tuple indexing works
only if a tuple is homogenous (all types are the same) or if the value
of the index is known at compile-time.

You can, however, iterate over heterogenous tuples in Seq. This is achieved
by unrolling the loop to accommodate the different types.

.. code:: seq

    t = (1, 2.3, 'hi')
    t[1]  # works because 1 is a constant int

    x = int(argv[1])
    t[x]  # compile error: x is not known at compile time

    # This is a homogenous tuple (all member types are the same)
    u = (1, 2, 3)  # type: Tuple[int, int, int].
    u[x]  # works because tuple members share the same type regardless of x
    for i in u:  # works
        print(i)

    # Also works
    v = (42, 'x', 3.14)
    for i in v:
        print(i)

.. note::
    Tuples are **immutable**. ``a = (1, 2); a[1] = 1`` will not
    compile.

Containers
~~~~~~~~~~

.. code:: seq

    l = [1, 2, 3]  # type: List[int]; a list of integers
    s = {1.1, 3.3, 2.2, 3.3}  # type: Set[float]; a set of floats
    d = {1: 'hi', 2: 'ola', 3: 'zdravo'}  # type: Dict[int, str]; a simple dictionary

    ln = []  # an empty list whose type is inferred based on usage
    ln = List[int]()  # an empty list with explicit element types
    dn = {}  # an empty dict whose type is inferred based on usage
    dn = Dict[int, float]()  # an empty dictionary with explicit element types

Because Seq is strongly typed, these won't compile:

.. code:: seq

    l = [1, 's']  # is it a List[int] or List[str]? you cannot mix-and-match types
    d = {1: 'hi'}
    d[2] = 3  # d is a Dict[int, str]; the assigned value must be a str

    t = (1, 2.2)  # Tuple[int, float]
    lt = list(t)  # compile error: t is not homogenous

    lp = [1, 2.1, 3, 5]  # compile error: Seq will not automatically cast a float to an int

This will work, though:

.. code:: seq

    u = (1, 2, 3)
    lu = list(u)  # works: u is homogenous

.. note::
    Dictionaries and sets are unordered and are based on
    `klib <https://github.com/attractivechaos/klib>`__.

.. _operators:

Assignments and operators
-------------------------

.. code:: seq

    a = 1 + 2  # this is 3
    a = (1).__add__(2)  # you can use a function call instead of an operator; this is also 3
    a = int.__add__(1, 2)  # this is equivalent to the previous line
    b = 5 / 2.0  # this is 2.5
    c = 5 // 2  # this is 2; // is an integer division
    a *= 2  # a is now 6

This is the list of binary operators and their magic methods:

======== ================ ==================================================
Operator Magic method     Description
======== ================ ==================================================
``+``    ``__add__``      addition
``-``    ``__sub__``      subtraction
``*``    ``__mul__``      multiplication
``/``    ``__truediv__``  float (true) division
``//``   ``__floordiv__`` integer (floor) division
``**``   ``__pow__``      exponentiation
``%``    ``__mod__``      modulo
``@``    ``__matmul__``   matrix multiplication;
                                  sequence alignment
``&``    ``__and__``      bitwise and
``|``    ``__or__``       bitwise or
``^``    ``__xor__``      bitwise xor
``<<``   ``__lshift__``   left bit shift
``>>``   ``__rshift__``   right bit shift
``<``    ``__lt__``       less than
``<=``   ``__le__``       less or equal than
``>``    ``__gt__``       greater than
``>=``   ``__ge__``       greater or equal than
``==``   ``__eq__``       equal to
``!=``   ``__ne__``       not equal to
``in``   ``__contains__`` belongs to
``and``  none             boolean and (short-circuits)
``or``   none             boolean or (short-circuits)
======== ================ ==================================================

Seq also has the following unary operators:

======== ================ =============================
Operator Magic method     Description
======== ================ =============================
``~``    ``__invert__``   bitwise inversion;
                                  reverse complement;
                                  ``Optional[T]`` unpacking
``+``    ``__pos__``      unary positive
``-``    ``__neg__``      unary negation
``not``  none             boolean negation
======== ================ =============================

Tuple unpacking
~~~~~~~~~~~~~~~

Seq supports most of Python's tuple unpacking syntax:

.. code:: seq

    x, y = 1, 2  # x is 1, y is 2
    (x, (y, z)) = 1, (2, 3)  # x is 1, y is 2, z is 3
    [x, (y, z)] = (1, [2, 3])  # x is 1, y is 2, z is 3

    l = range(1, 8)  # l is [1, 2, 3, 4, 5, 6, 7]
    a, b, *mid, c = l  # a is 1, b is 2, mid is [3, 4, 5, 6], c is 7
    a, *end = l  # a is 1, end is [2, 3, 4, 5, 6, 7]
    *beg, c = l  # c is 7, beg is [1, 2, 3, 4, 5, 6]
    (*x, ) = range(3)  # x is [0, 1, 2]
    *x = range(3)  # error: this does not work

    *sth, a, b = (1, 2, 3, 4)  # sth is (1, 2), a is 3, b is 4
    *sth, a, b = (1.1, 2, 3.3, 4)  # error: this only works on homogenous tuples for now

    (x, y), *pff, z = [1, 2], 'this'
    print(x, y, pff, z) # x is 1, y is 2, pff is an empty tuple --- () ---, and z is "this"

    s, *q = 'XYZ'  # works on strings as well; s is "X" and q is "YZ"

Control flow
------------

Conditionals
~~~~~~~~~~~~

Seq supports the standard Python conditional syntax:

.. code:: seq

    if a or b or some_cond():
        print(1)
    elif whatever() or 1 < a <= b < c < 4:  # chained comparisons are supported
        print('meh...')
    else:
        print('lo and behold!')

    if x: y()

    a = b if sth() else c  # ternary conditional operator

Seq extends the Python conditional syntax with a ``match`` statement, which is 
inspired by Rust's:

.. code:: seq

    match a + some_heavy_expr():  # assuming that the type of this expression is int
        case 1:  # is it 1?
            print('hi')
        case 2 ... 10:  # is it 2, 3, 4, 5, 6, 7, 8, 9 or 10?
            print('wow!')
        case _:  # "default" case
            print('meh...')

    match bool_expr():  # now it's a bool expression
        case True: ...
        case False: ...

    match str_expr():  # now it's a str expression
        case 'abc': print("it's ABC time!")
        case 'def' | 'ghi':  # you can chain multiple rules with the "|" operator
            print("it's not ABC time!")
        case s if len(s) > 10: print("so looong!")  # conditional match expression
        case _: assert False

    match some_tuple:  # assuming type of some_tuple is Tuple[int, int]
        case (1, 2): ...
        case (a, _) if a == 42:  # you can do away with useless terms with an underscore
            print('hitchhiker!')
        case (a, 50 ... 100) | (10 ... 20, b):  # you can nest match expressions
            print('complex!')

    match list_foo():
        case []:  # [] matches an empty list
            ...
        case [1, 2, 3]:  # make sure that list_foo() returns List[int] though!
            ...
        case [1, 2, ..., 5]:  # matches any list that starts with 1 and 2 and ends with 5
            ...
        case [..., 6] | [6, ...]:  # matches a list that starts or ends with 6
            ...
        case [..., w] if w < 0:  # matches a list that ends with a negative integer
            ...
        case [...]:  # any other list
            ...

    match sequence:  # of type seq
        case 'ACGT': ...
        case 'AC_T': ...  # _ is a wildcard character and it can be anything
        case 'A_C_T_': ...  # a spaced k-mer AxCxTx
        case 'AC*T': ...  # matches a sequence that starts with AC and ends with T

You can mix, match and chain match rules as long as the match type
matches the expression type.

Loops
~~~~~

Standard fare:

.. code:: seq

    a = 10
    while a > 0:  # prints even numbers from 9 to 1
        a -= 1
        if a % 2 == 1:
            continue
        print(a)

    for i in range(10):  # prints numbers from 0 to 7, inclusive
        print(i)
        if i > 6: break

``for`` construct can iterate over any generator, which means any object
that implements the ``__iter__`` magic method. In practice, generators,
lists, sets, dictionaries, homogenous tuples, ranges, and many more types
implement this method, so you don't need to worry. If you need to
implement one yourself, just keep in mind that ``__iter__`` is a
generator and not a function.

Comprehensions
~~~~~~~~~~~~~~

Technically, comprehensions are not statements (they are expressions).
Comprehensions are a nifty, Pythonic way to create collections:

.. code:: seq

    l = [i for i in range(5)]  # type: List[int]; l is [0, 1, 2, 3, 4]
    l = [i for i in range(15) if i % 2 == 1 if i > 10]  # type: List[int]; l is [11, 13]
    l = [i * j for i in range(5) for j in range(5) if i == j]  # l is [0, 1, 4, 9, 16]

    s = {abs(i - j) for i in range(5) for j in range(5)}  # s is {0, 1, 2, 3, 4}
    d = {i: f'item {i+1}' for i in range(3)}  # d is {0: "item 1", 1: "item 2", 2: "item 3"}

You can also use collections to create generators (more about them later
on):

.. code:: seq

    g = (i for i in range(10))
    print(list(g))  # prints number from 0 to 9, inclusive

Exception handling
~~~~~~~~~~~~~~~~~~

Again, if you know how to do this in Python, you know how to do it in
Seq:

.. code:: seq

    def throwable():
         raise ValueError("doom and gloom")

    try:
        throwable()
    except ValueError as e:
        print("we caught the exception")
    except:
        print("ouch, we're in deep trouble")
    finally:
        print("whatever, it's done")

.. note::
    Right now, Seq cannot catch multiple exceptions in one
    statement. Thus ``catch (Exc1, Exc2, Exc3) as var`` will not compile.

If you have an object that implements ``__enter__`` and ``__exit__``
methods to manage its lifetime (say, a ``File``), you can use a ``with``
statement to make your life easy:

.. code:: seq

    with open('foo.txt') as f, open('foo_copy.txt', 'w') as fo:
        for l in f:
            fo.write(l)

Variables and scoping
---------------------

You have probably noticed by now that blocks in Seq are defined by their
indentation level (as in Python). We recommend using 2 or 4 spaces to
indent blocks. Do not mix indentation levels, and do not mix tabs and spaces;
stick to any *consistent* way of indenting your code.

One of the major differences between Seq and Python lies in variable
scoping rules. Seq variables cannot *leak* to outer blocks. Every
variable is accessible only within its own block (after the variable is
defined, of course), and within any block that is nested within the
variable's own block.

That means that the following Pythonic pattern won't compile:

.. code:: seq

    if cond():
         x = 1
    else:
         x = 2
    print(x)  # x is defined separately in if/else blocks; it is not accessible here!

    for i in range(10):
         ...
    print(i)  # error: i is only accessible within the for loop block

In Seq, you can rewrite that as:

.. code:: seq

    x = 2
    if cond():
         x = 1

    # or even better
    x = 1 if cond() else 2

    print(x)

Another important difference between Seq and Python is that, in Seq, variables
cannot change types. So this won't compile:

.. code:: seq

    a = 's'
    a = 1  # error: expected string, but got int

A note about function scoping: functions cannot modify variables that
are not defined within the function block. You need to use ``global`` to
modify such variables:

.. code:: seq

    g = 5
    def foo():
        print(g)
    foo()  # works, prints 5

    def foo2():
        g += 2  # error: cannot access g
        print(g)

    def foo3():
        global g
        g += 2
        print(g)
    foo3()  # works, prints 7
    foo3()  # works, prints 9

As a rule, use ``global`` whenever you need to access variables that
are not defined within the function.

Imports
-------

You can import functions and classes from another Seq module by doing:

.. code:: seq

    # Create foo.seq with a bunch of useful methods
    import foo

    foo.useful1()
    p = foo.FooType()

    # Create bar.seq with a bunch of useful methods
    from bar import x, y
    x(y)

    from bar import z as bar_z
    bar_z()

``import foo`` looks for ``foo.seq`` or ``foo/__init__.seq`` in the
current directory.

Functions
---------

Functions are defined as follows:

.. code:: seq

    def foo(a, b, c):
        return a + b + c
    print(foo(1, 2, 3))  # prints 6

How about procedures? Well, don't return anything meaningful:

.. code:: seq

    def proc(a, b):
        print(a, 'followed by', b)
    proc(1, 's')

    def proc2(a, b):
        if a == 5:
            return
        print(a, 'followed by', b)
    proc2(1, 's')
    proc2(5, 's')  # this prints nothing

Seq is a strongly-typed language, so you can restrict argument and
return types:

.. code:: seq

    def fn(a: int, b: float):
        return a + b  # this works because int implements __add__(float)
    fn(1, 2.2)  # 3.2
    fn(1.1, 2)  # error: 1.1. is not an int

    def fn2(a: int, b):
        return a - b
    fn2(1, 2)  # -1
    fn2(1, 1.1)  # -0.1; works because int implements __sub__(float)
    fn2(1, 's')  # error: there is no int.__sub__(str)!

    def fn3(a, b) -> int:
        return a + b
    fn3(1, 2)  # works, as 1 + 2 is integer
    fn3('s', 'u')  # error: 's'+'u' returns 'su' which is str,
                   # but the signature indicates that it must return int

Default arguments? Named arguments? You bet:

.. code:: seq

    def foo(a, b: int, c: float = 1.0, d: str = 'hi'):
        print(a, b, c, d)
    foo(1, 2)  # prints "1 2 1 hi"
    foo(1, d='foo', b=1)  # prints "1 1 1 foo"

How about optional arguments? We support that too:

.. code:: seq

    # type of b promoted to Optional[int]
    def foo(a, b: int = None):
        print(a, b + 1)

    foo(1, 2)  # prints "1 3"
    foo(1)  # raises ValueError, since b is None

Generics
~~~~~~~~

As we've said several times: Seq is a strongly typed language. As
such, it is not as flexible as Python when it comes to types (e.g. you
can't have lists with elements of different types). However,
Seq tries to mimic Python's *"I don't care about types until I do"*
attitude as much as possible by utilizing a technique known as
*monomorphization*. If there is a function that has an argument
without a type definition, Seq will consider it a *generic* function,
and will generate different functions for each invocation of
that generic function:

.. code:: seq

    def foo(x):
        print(x)  # print relies on typeof(x).__str__(x) method to print the representation of x
    foo(1)  # Seq automatically generates foo(x: int) and calls int.__str__ when needed
    foo('s')  # Seq automatically generates foo(x: str) and calls str.__str__ when needed
    foo([1, 2])  # Seq automatically generates foo(x: List[int]) and calls List[int].__str__ when needed

But what if you need to mix type definitions and generic types? Say, your
function can take a list of *anything*? Well, you can use generic
specifiers:

.. code:: seq

    def foo(x: List[T], T: type):
        print(x)
    foo([1, 2])           # prints [1, 2]
    foo(['s', 'u'])       # prints [s, u]
    foo(5)                # error: 5 is not a list!
    foo(['s', 'u'], int)  # fails: T is int, so foo expects List[int] but it got List[str]

    def foo(x, R: type) -> R:
        print(x)
        return 1
    foo(4, int)  # prints 4, returns 1
    foo(4, str)  # error: return type is str, but foo returns int!


.. note::
    Coming from C++? ``foo(x: List[T], T: type): ...`` is roughly the same as
    ``template<typename T, typename U> U foo(T x) { ... }``.

Generators
~~~~~~~~~~

Seq supports generators, and they are fast! Really, really fast!

.. code:: seq

    def gen(i):
        while i < 10:
            yield i
            i += 1
    print(list(gen(0)))  # prints [0, 1, ..., 9]
    print(list(gen(10)))  # prints []

You can also use ``yield`` to implement coroutines: ``yield``
suspends the function, while ``(yield)`` (yes, parentheses are required)
receives a value, as in Python.

.. code:: seq

    def mysum[T](start: T):
        m = start
        while True:
            a = (yield)  # receives the input of coroutine.send() call
            if a == -1:
                break  # exits the coroutine
            m += a
        yield m
    iadder = mysum(0)  # assign a coroutine
    next(iadder)  # activate it
    for i in range(10):
        iadder.send(i)  # send a value to coroutine
    print(iadder.send(-1))  # prints 45

Pipelines
~~~~~~~~~

Seq extends the core Python language with a pipe operator, which is
similar to bash pipes (or F#'s ``|>`` operator). You can chain multiple
functions and generators to form a pipeline:

.. code:: seq

    def add1(x):
        return x + 1

    2 |> add1  # 3; equivalent to add1(2)

    def calc(x, y):
        return x + y**2
    2 |> calc(3)  # 11; equivalent to calc(2, 3)
    2 |> calc(..., 3)  # 11; equivalent to calc(2, 3)
    2 |> calc(3, ...)  # 7; equivalent to calc(3, 2)

    def gen(i):
        for i in range(i):
            yield i
    5 |> gen |> print # prints 0 1 2 3 4 separated by newline
    range(1, 4) |> iter |> gen |> print(end=' ')  # prints 0 0 1 0 1 2 without newline
    [1, 2, 3] |> print   # prints [1, 2, 3]
    range(100000000) |> print  # prints range(0, 100000000)
    range(100000000) |> iter |> print  # not only prints all those numbers, but it uses almost no memory at all

Seq will chain anything that implements ``__iter__``; however, use
generators as much as possible because the compiler will optimize out
generators whenever possible. Combinations of pipes and generators can be
used to implement efficient streaming pipelines.

Seq can also execute pipelines in parallel via the parallel pipe (``||>``)
operator:

.. code:: seq

    range(100000) |> iter ||> print  # prints all these numbers, probably in random order
    range(100000) |> iter ||> process ||> clean  # runs process in parallel, and then cleans data in parallel

In the last example, Seq will automatically schedule the ``process`` and
``clean`` functions to execute as soon as possible. You can control the
number of threads via the ``OMP_NUM_THREADS`` environment variable.

.. _interop:

Foreign function interface (FFI)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Seq can easily call functions from C and Python.

Let's import some C functions:

.. code:: seq

    from C import pow(float) -> float
    pow(2.0)  # 4.0

    # Import and rename function
    from C import puts(cobj) -> void as print_line  # type cobj is C's pointer (void*, char*, etc.)
    print_line("hi!".c_str())  # prints "hi!".
                               # Note .ptr at the end of string--- needed to cast Seq's string to char*.

``from C import`` only works if the symbol is available to the program. If you
are running your programs via ``seqc``, you can link dynamic libraries
by running ``seqc run -l path/to/dynamic/library.so ...``.

Hate linking? You can also use dyld library loading as follows:

.. code:: seq


    LIBRARY = "mycoollib.so"
    from C import LIBRARY.mymethod(int, float) -> cobj
    from C import LIBRARY.myothermethod(int, float) -> cobj as my2
    foo = mymethod(1, 2.2)
    foo2 = my2(4, 3.2)

.. note::
    When importing external non-Seq functions, you must explicitly specify
    argument and return types.

How about Python? If you have set the ``SEQ_PYTHON`` environment variable as
described in the first section, you can do:

.. code:: seq

    from python import mymodule.myfunction(str) -> int as foo
    print(foo("bar"))

Often you want to execute more complex Python code within Seq. To that
end, you can use Seq's ``@python`` annotation:

.. code:: seq

    @python
    def scipy_here_i_come(i: List[List[float]]) -> List[float]:
        # Code within this block is executed by the Python interpreter,
        # and as such it must be valid Python code
        import scipy.linalg
        import numpy as np
        data = np.array(i)
        eigenvalues, _ = scipy.linalg.eig(data)
        return list(eigenvalues)
    print(scipy_here_i_come([[1.0, 2.0], [3.0, 4.0]]))  # [-0.372281, 5.37228] with some warnings...

Seq will automatically bridge any object that implements the ``__to_py__``
and ``__from_py__`` magic methods. All standard Seq types already
implement these methods.

Classes and types
-----------------

Of course, Seq supports classes! However, you must declare class members
and their types in the preamble of each class (like you would do with
Python's dataclasses).

.. code:: seq

    class Foo:
        x: int
        y: int

        def __init__(self, x: int, y: int):  # constructor
            self.x, self.y = x, y

        def method(self):
            print(self.x, self.y)

    f = Foo(1, 2)
    f.method()  # prints "1 2"

.. note::
    Seq does not (yet!) support inheritance and polymorphism.

Unlike Python, Seq supports method overloading:

.. code:: seq

    class Foo:
        x: int
        y: int

        def __init__(self, x: int, y: int):  # constructor
            self.x, self.y = 0, 0
        def __init__(self, x: int, y: int):  # another constructor
            self.x, self.y = x, y
        def __init__(self, x: int, y: float):  # another constructor
            self.x, self.y = x, int(y)
        def __init__(self):
            self.x, self.y = 0, 0

        def method(self: Foo):
            print(self.x, self.y)

    Foo().method()  # prints "0 0"
    Foo(1, 2).method()  # prints "1 2"
    Foo(1, 2.3).method()  # prints "1 2"
    Foo(1.1, 2.3).method()  # error: there is no Foo.__init__(float, float)

Classes can also be generic:

.. code:: seq

    class Container[T]:
        l: List[T]
        def __init__(self, l: List[T]):
            self.l = l
        ...

Classes create objects that are passed by reference:

.. code:: seq

    class Point:
        x: int
        y: int
        ...

    p = Point(1, 2)
    q = p  # this is a reference!
    p.x = 2
    print((p.x, p.y), (q.x, q.y))  # (2, 2), (2, 2)

If you need to copy an object's contents, implement the ``__copy__`` magic
method and use ``q = copy(p)`` instead.

Seq also supports pass-by-value types via the ``@tuple`` annotation:

.. code:: seq

    @tuple
    class Point:
        x: int
        y: int

    p = Point(1, 2)
    q = p  # this is a copy!
    print((p.x, p.y), (q.x, q.y))  # (1, 2), (1, 2)

However, **by-value objects are immutable!**. The following code will
not compile:

.. code:: seq

    p = Point(1, 2)
    p.x = 2  # error! immutable type

Under the hood, types are basically named tuples (equivalent to Python's
``collections.namedtuple``).

You can also add methods to types:

.. code:: seq

    @tuple
    class Point:
        x: int
        y: int

        def __new__():          # types are constructed via __new__, not __init__
            return Point(0, 1) # and __new__ returns a tuple representation of type's members

        def some_method(self):
            return self.x + self.y

    p = Point()  # p is (0, 1)
    print(p.some_method())  # 1

Type extensions
~~~~~~~~~~~~~~~

Suppose you have a class that lacks a method or an operator that might
be really useful. You can extend that class and add the method at compile time:

.. code:: seq

    class Foo:
        ...

    f = Foo(...)

    # we need foo.cool() but it does not exist... not a problem for Seq
    @extend
    class Foo:
        def cool(self: Foo):
            ...

    f.cool()  # works!

    # how about we add support for adding integers and strings?
    @extend
    class int:
        def __add__(self: int, other: str) -> int:
            return self + int(other)

    print(5 + '4')  # 9

Magic methods
~~~~~~~~~~~~~

Here is a list of useful magic methods that you might want to add and
overload:

================ =============================================
Magic method     Description
================ =============================================
operators        overload unary and binary operators (see :ref:`operators`)
``__copy__``     copy-constructor for ``copy`` method
``__len__``      for ``len`` method
``__bool__``     for ``bool`` method and condition checking
``__getitem__``  overload ``obj[key]``
``__setitem__``  overload ``obj[key] = value``
``__delitem__``  overload ``del obj[key]``
``__iter__``     support iterating over the object
``__str__``      support printing and ``str`` method
================ =============================================

LLVM functions
~~~~~~~~~~~~~~

In certain cases, you might want to use LLVM features that are not directly
accessible with Seq. This can be done with the ``@llvm`` attribute:

.. code:: seq

    @llvm
    def llvm_add[T](a: T, b: T) -> T:
        %res = add {=T} %a, %b
        ret {=T} %res

    print(llvm_add(3, 4))  # 7
    print(llvm_add(i8(5), i8(6)))  # 11

--------------

Issues, feedback, or comments regarding this tutorial? Let us know `on GitHub <https://github.com/seq-lang/seq>`__.
