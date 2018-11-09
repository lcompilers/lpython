from liblfort.codegen.evaluator import FortranEvaluator

def test_fn_dummy():
    e = FortranEvaluator()
    e.evaluate("""\
function f(a)
integer :: a
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
integer :: a
f2 = a + b
end function
""")
    assert e.evaluate("f2(2)") == 7
    e.evaluate("b = 6")
    assert e.evaluate("f2(2)") == 8