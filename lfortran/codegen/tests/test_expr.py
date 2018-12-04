from lfortran.codegen.evaluator import FortranEvaluator

def test_if_conditions():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: i
i = 0
if (.false.) i = 1
if (1 == 2) i = 1
if (1 /= 1) i = 1
if (1 > 2) i = 1
if (1 >= 2) i = 1
if (2 < 1) i = 1
if (2 <= 1) i = 1
""")
    assert e.evaluate("i") == 0

def test_expr():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: x, i
i = 0
x = (2+3)*5
if (x /= 25) i = 1

x = (2+3)*4
if (x /= 20) i = 1

x = (2+3)*(2+3)
if (x /= 25) i = 1

x = (2+3)*(2+3)*4*2*(1+2)
if (x /= 600) i = 1

x = x / 60
if (x /= 10) i = 1

x = x + 1
if (x /= 11) i = 1

x = x - 1
if (x /= 10) i = 1

x = -2
if (x /= -2) i = 1

x = -2*3
if (x /= -6) i = 1

x = -2*(-3)
if (x /= 6) i = 1

x = 3 - 1
if (x /= 2) i = 1

x = 1 - 3
if (x /= -2) i = 1
if (x /= (-2)) i = 1

x = 1 - (-3)
if (x /= 4) i = 1
if (x /= +4) i = 1
if (x /= (+4)) i = 1
""")
    assert e.evaluate("i") == 0