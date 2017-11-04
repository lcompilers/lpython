import os

from liblfort.ast import parse, dump, SyntaxErrorException

def to_tuple(t):
    if t is None or isinstance(t, (str, int, complex)):
        return t
    elif isinstance(t, list):
        return [to_tuple(e) for e in t]
    result = [t.__class__.__name__]
    if hasattr(t, 'lineno') and hasattr(t, 'col_offset'):
        result.append((t.lineno, t.col_offset))
    if t._fields is None:
        return tuple(result)
    for f in t._fields:
        result.append(to_tuple(getattr(t, f)))
    return tuple(result)

def run_tests(tests, filename):
    results = []
    for s in tests:
        results.append(to_tuple(parse(s)))

    here = os.path.dirname(__file__)
    results_filename = os.path.join(here, filename)
    try:
        with open(results_filename) as f:
            d = {}
            exec(f.read(), d)
            results_ref = d["results"]
        equal = (results == results_ref)
        report = True
    except FileNotFoundError:
        equal = False
        report = False
        print("Results file does not exist.")
    if not equal:
        results_str = "results = [\n"
        for r in results:
            results_str += "    %r,\n" % (r,)
        results_str += "]\n"
        with open(results_filename+".latest", "w") as f:
            f.write(results_str)
        print("Results file generated. If you want to use it, copy " \
              "'{0}.latest' to '{0}'.".format(filename))
        if report:
            print()
            if (len(results) == len(results_ref)):
                print("REPORT:")
                print("Printing failed tests:")
                for i, t, s, l in zip(range(len(tests)), tests, results,
                                        results_ref):
                    if s != l:
                        print("n test")
                        print("%d: %s" % (i, t))
                        print(s)
                        print(l)
                        print()
            else:
                print("Results lists have different lengths.")
    assert equal

def parses(test):
    """
    Returns True if and only if the string `test` parses to AST.
    """
    try:
        s = parse(test)
        passed = True
    except SyntaxErrorException:
        passed = False
    return passed

def all_fail(tests):
    for test in tests:
        assert not parses(test)


# -------------------------------------------------------------------------
# Tests:

def test_dump_expr():
    assert dump(parse("1+1")) == \
            "BinOp(left=Num(n='1'), op=Add(), right=Num(n='1'))"
    assert dump(parse("1+x")) == \
            "BinOp(left=Num(n='1'), op=Add(), right=Name(id='x'))"
    assert dump(parse("(x+y)**2")) == \
            "BinOp(left=BinOp(left=Name(id='x'), op=Add(), " \
            "right=Name(id='y')), op=Pow(), right=Num(n='2'))"

def test_to_tuple():
    ast_tree = parse("2+3")
    t = to_tuple(ast_tree)
    t_ref = ('BinOp', (1, 1), ('Num', (1, 1), '2'), ('Add',), ('Num', (1, 1),
        '3'))
    assert t == t_ref

    ast_tree = parse("2+x")
    t = to_tuple(ast_tree)
    t_ref = ('BinOp', (1, 1), ('Num', (1, 1), '2'), ('Add',), ('Name', (1, 1),
        'x'))
    assert t == t_ref

    ast_tree = parse("(x+y)**2")
    t = to_tuple(ast_tree)
    t_ref = ('BinOp', (1, 1), ('BinOp', (1, 1), ('Name', (1, 1), 'x'),
        ('Add',), ('Name', (1, 1), 'y')), ('Pow',), ('Num', (1, 1), '2'))
    assert t == t_ref

def test_dump_statements():
    assert dump(parse("if (x == 1) stop\n")) == \
            "If(test=Compare(left=Name(id='x'), op=Eq(), right=Num(n='1')), " \
            "body=[Stop(code=None)], orelse=[])"
    assert dump(parse("x == 1\n")) == \
            "Compare(left=Name(id='x'), op=Eq(), right=Num(n='1'))"

def test_dump_subroutines():
    assert dump(parse("""\
subroutine a
integer :: a, b
a = 1+2*3
b = (1+2+a)*3
end subroutine
""")) == "Subroutine(name='a', args=[], " \
    "body=[Declaration(vars=[decl(sym='a', " \
    "sym_type='integer'), decl(sym='b', sym_type='integer')]), " \
    "Assignment(target='a', value=BinOp(left=Num(n='1'), op=Add(), " \
    "right=BinOp(left=Num(n='2'), op=Mul(), right=Num(n='3')))), " \
    "Assignment(target='b', " \
    "value=BinOp(left=BinOp(left=BinOp(left=Num(n='1'), " \
    "op=Add(), right=Num(n='2')), op=Add(), right=Name(id='a')), op=Mul(), " \
    "right=Num(n='3')))])"
    assert  dump(parse("""\
subroutine f(x, y)
integer, intent(out) :: x, y
x = 1
end subroutine
""")) == "Subroutine(name='f', args=[arg(arg='x'), arg(arg='y')], " \
    "body=[Declaration(vars=[decl(sym='x', sym_type='integer'), decl(sym='y', "\
    "sym_type='integer')]), Assignment(target='x', value=Num(n='1'))])"

def test_dump_programs():
    assert dump(parse("""\
program a
integer :: b
b = 1
end program
""")) == "Program(name='a', body=[Declaration(vars=[decl(sym='b', " \
    "sym_type='integer')]), Assignment(target='b', value=Num(n='1'))])"


def test_expr1():
    tests = [
        "1",
        "2+3",
        "(1+3)*4",
        "1+3*4",
        "x",
        "yx",
        "x+y",
        "2+x",
        "(x+y)**2",
        "(x+y)*3",
        "x+y*3",
        "(1+2+a)*3",
        "f(3)+6",
        "f(3+6)",
        "real(b, dp)",
        "2*u-1",
        "sum(u**2)",
        "u(2)",
        "u * sqrt(-2*log(r2)/r2)",
        ".not. first",
        "a - 1._dp/3",
        "1/sqrt(9*d)",
        "(1 + c*x)**3",
        "i + 1",
        '"s"',
        '"some text"',
        #"a(3:5,i:j)",
        #"b(:)",
        #"a(:5,i:j) + b(1:)",
        #"[1, 2, 3, i]",
        #"f()",
        #"x%a",
        #"x%a()",
        #"x%b(i, j)",
        #"y%c(5, :)",
        #"x%f%a",
        #"x%g%b(i, j)",
        #"y%h%c(5, :)",
        """ "a'b'c" """,
        """ 'a"b"c'""",
        """ 'a""bc""x'""",
        """ "a""c" """,
        """ "a""b""c" """,
        """ \"\"\"zippo\"\"\" """,
        """ 'a''c'""",
        """ 'a''b''c'""",
        """ '''zippo'''""",
        """ "aaa" // str(x) // "bb" """,
        """ "a" // "b" """,
        ]
    run_tests(tests, "expr_results.py")

def test_expr2():
    tests = [
        "1x",
        "1+",
        "(1+",
        "(1+2",
        "1+2*",
        "f(3+6",
        ]
    all_fail(tests)

def test_statements1():
    tests = [
        "call random_number(u)",
        "u = 2*u-1",
        "r2 = sum(u**2)",
        "u = u * sqrt(-2*log(r2)/r2)",
        "x = u(1)",
        "x = u(2)",
        "first = .not. first",
        "d = a - 1._dp/3",
        "c = 1/sqrt(9*d)",
        "v = (1 + c*x)**3",
        "fn_val = d*v",
        "exit",
        "call randn(x(i))",
        "call randn(x)",
        "call random_number(U)",
        "call rand_gamma0(a, .true., x)",
        "call rand_gamma0(a, .true., x(1))",
        "call rand_gamma0(a, .false., x(i))",
        "call rand_gamma_vector_n(a, size(x), x)",
        "call f(a=4, b=6, c=i)",
        "open(newunit=a, b, c)",
        "allocate(c(4), d(4))",
        "close(u)",
        "x = 1; y = 2",
        "y = 5; a = 1; x = u(2)",
        "a = 5",
        "x = 1; y = 2;",
        "y = 5; a = 1; x = u(2);",
        "a = 5;",
        "; ;",
        'stop "OK"',
        'write (*,"(i4)") 45',
        'write (*,*) 45, "ss"',
        'print "(i4)", 45',
        'print *, 45, "sss", a+1',
        #"x => y",
        #"x => y(1:4, 5)",
        #"call g(a(3:5,i:j), b(:))",
        #"call g(a(:5,i:j), b(1:))",
        #"c = [1, 2, 3, i]",
        "a = x%a",
        #"b = x%b(i, j)",
        #"c = y%c(5, :)",
        "a = x%f%a",
        #"b = x%g%b(i, j)",
        #"c = y%h%c(5, :)",
        "x%f%a = a",
        "x%g%b(i, j) = b",
        #"y%h%c(5, :) = c",
        "call x%f%e()",
    ]
    tests = [x+"\n" for x in tests]
    run_tests(tests, "statements_results.py")

def _test_control_flow1():
    tests = ["""\
do while(x == y)
    i = i +1
    cycle
    exit
end do
""",
        """\
subroutine a
if (a) then
    x = 1
else
    x = 2
end if
end subroutine
""",
        """\
subroutine a
if (a) then
    x = 1
else if (b) then
    x = 2
end if
end subroutine
""",
        """\
subroutine a
if (a) then
    x = 1
else if (b) then
    x = 2
else if (c) then
    x = 2
end if
end subroutine
""",
        """\
subroutine a
if (a) then
    x = 1
end if
end subroutine
""",
        """\
subroutine a
if (a) x = 1
end subroutine
""",
        """\
subroutine a
if (a) &
    x = 1
end subroutine
""",
        """\
subroutine a
if (a) &     ! if statement
    x = 1
end subroutine
""",
        """\
subroutine a
do
    x = 1
end do
end subroutine
""",
        """\
subroutine a
do i = 1, 5
    x = x + i
end do
end subroutine
""",
        """\
select case(k)
    case(1)
        call a()
    case(i)
        call b()
end select
""",
        """\
select case(k)
    case(1)
        call a()
    case(i)
        call b()
    case default
        call c()
end select
""",
    ]
    run_tests(tests, "controlflow_results.py")

def test_decl():
    tests = [
        "integer x",
        "integer :: x",
        "integer :: a(10,10), b(10)",
        "integer, dimension(10,10) :: c",
        "integer, dimension(:,:), intent(in) :: e",
        "integer(c_int) :: i",

        "real a",
        "real :: a",
        "real(dp) :: a, b",
        "real(dp) y = 5",
        "real(c_double) :: f",

        "character(len=*) :: c",

        "type(xx), intent(inout) :: x, y",
    ]
    tests = [x+"\n" for x in tests]
    run_tests(tests, "decl_results.py")

def test_use():
    tests = [
        "use b",
        "use a, b, c",
        "use a, only: b, c",
        "use a, only: x => b, c, d => a",
    ]
    tests = [x+"\n" for x in tests]
    run_tests(tests, "use_results.py")

def test_subroutines_functions():
    tests = [
        """\
real(dp) pure function f(e) result(r)
r = 1
end function
""",
        """\
real(dp) recursive function f(e) result(r)
r = 1
end function
""",
        """\
function f(e)
f = 1
end function
""",
        """\
subroutine f
integer :: x
end subroutine
""",
        """\
subroutine f()
! Some comment
integer :: x
! Some other comment
end subroutine
""",
        """\
subroutine f(a, b, c, d)
integer, intent(in) :: a, b
integer, intent ( in ) :: c, d
integer :: z
integer::y
end subroutine
""",
        """\
subroutine f(a, b, c, d)
integer, intent(out) :: a, b
integer, intent(inout) :: c, d
integer :: z
integer::y
end subroutine
""",
    ]
    run_tests(tests, "subroutine_results.py")
