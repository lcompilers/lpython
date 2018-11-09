from liblfort.codegen.evaluator import FortranEvaluator

def test_vars1():
    e = FortranEvaluator()
    e.evaluate("""\
function f(a)
integer :: a
f = a + 1
end function
""")
    assert e.evaluate("f(2)") == 3