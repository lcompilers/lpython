from liblfort.codegen.evaluator import FortranEvaluator

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
