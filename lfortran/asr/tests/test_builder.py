def test_builder_module1():
    from lfortran.ast.fortran_printer import print_fortran
    from lfortran.asr import asr, asr_to_ast
    from lfortran.asr.builder import (make_type_integer, TranslationUnit,
            FunctionBuilder)


    integer = make_type_integer()
    unit = TranslationUnit()
    m = unit.make_module(name="mod1")


    a = asr.Variable(name="a", intent="in", type=integer)
    b = asr.Variable(name="b", intent="in", type=integer)
    c = asr.Variable(name="c", type=integer)
    f = FunctionBuilder(m, "f", args=[a, b], return_var=c)
    d = f.make_var(name="d", type=integer)
    f.add_statements([
        asr.Assignment(d, asr.Num(n=5, type=integer)),
        asr.Assignment(c, asr.BinOp(d, asr.Mul(),
            asr.BinOp(a, asr.Add(), b, integer), integer))
    ])
    f.finalize()




    a = asr.Variable(name="a", intent="in", type=integer)
    b = asr.Variable(name="b", intent="in", type=integer)
    c = asr.Variable(name="c", type=integer)
    f = FunctionBuilder(m, name="g", args=[a, b], return_var=c)
    f.finalize()



    a = asr_to_ast.asr_to_ast(m)
    s = print_fortran(a)
    print(s)
