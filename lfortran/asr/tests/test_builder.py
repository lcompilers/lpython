from lfortran.ast.fortran_printer import print_fortran
from lfortran.asr import asr, asr_to_ast
from lfortran.asr.asr_check import verify_asr
from lfortran.asr.builder import (make_type_integer, TranslationUnit,
        make_function, function_make_var)

def test_builder_module1():
    integer = make_type_integer()
    unit = TranslationUnit()
    m = unit.make_module(name="mod1")

    f = make_function(m, "f", args=["a", "b"], return_var="c")
    a, b = f.args
    a.intent = "in"; a.type = integer
    b.intent = "in"; b.type = integer
    c = f.return_var
    c.type = integer
    d = function_make_var(f, name="d", type=integer)
    f.body.extend([
        asr.Assignment(d, asr.Num(n=5, type=integer)),
        asr.Assignment(c, asr.BinOp(d, asr.Mul(),
            asr.BinOp(a, asr.Add(), b, integer), integer))
    ])

    a = asr.Variable(name="a", intent="in", type=integer)
    b = asr.Variable(name="b", intent="in", type=integer)
    c = asr.Variable(name="c", type=integer)
    f = make_function(m, name="g", args=[a, b], return_var=c)

    verify_asr(m)
    a = asr_to_ast.asr_to_ast(m)
    s = print_fortran(a)
    print(s)
