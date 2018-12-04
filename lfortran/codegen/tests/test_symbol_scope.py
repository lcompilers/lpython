from lfortran.codegen.evaluator import FortranEvaluator

def test_fn_dummy():
    e = FortranEvaluator()
    e.evaluate("""\
function f(a)
integer, intent(in) :: a
f = a + 1
end function
""")
    assert e.evaluate("f(2)") == 3

def test_fn_global():
    e = FortranEvaluator()
    e.evaluate("integer :: b")
    e.evaluate("b = 5")
    e.evaluate("""\
function f2(a)
integer, intent(in) :: a
f2 = a + b
end function
""")
    assert e.evaluate("f2(2)") == 7
    e.evaluate("b = 6")
    assert e.evaluate("f2(2)") == 8

def test_fn_global_set():
    e = FortranEvaluator()
    e.evaluate("integer :: b")
    e.evaluate("""\
function f(a)
integer, intent(in) :: a
b = a
f = 0
end function
""")
    e.evaluate("f(2)")
    assert e.evaluate("b") == 2
    e.evaluate("f(5)")
    assert e.evaluate("b") == 5

def test_fn_local():
    e = FortranEvaluator()
    e.evaluate("""\
function f3(a)
integer, intent(in) :: a
integer :: b
b = 5
f3 = a + b
end function
""")
    assert e.evaluate("f3(2)") == 7

def test_fn_local_shadow():
    e = FortranEvaluator()
    e.evaluate("integer :: b")
    e.evaluate("b = 1")
    e.evaluate("""\
function f1(a)
integer, intent(in) :: a
integer :: b
b = 5
f1 = a + b
end function
""")
    e.evaluate("""\
function f2(a)
integer, intent(in) :: a
f2 = a + b
end function
""")
    assert e.evaluate("f1(2)") == 7
    assert e.evaluate("f2(2)") == 3