def test_builder_module1():
    from lfortran.ast.fortran_printer import print_fortran
    from lfortran.asg import asg, asg_to_ast
    from lfortran.asg.builder import (make_type_integer, TranslationUnit,
            FunctionBuilder)


    integer = make_type_integer()
    unit = TranslationUnit()
    m = unit.make_module(name="mod1")


    a = asg.Variable(name="a", intent="in", type=integer)
    b = asg.Variable(name="b", intent="in", type=integer)
    c = asg.Variable(name="c", type=integer)
    f = FunctionBuilder(m, "f", args=[a, b], return_var=c)
    d = f.make_var(name="d", type=integer)
    f.add_statements([
        asg.Assignment(d, asg.Num(n=5, type=integer)),
        asg.Assignment(c, asg.BinOp(d, asg.Mul(),
            asg.BinOp(a, asg.Add(), b, integer), integer))
    ])
    f.finalize()




    a = asg.Variable(name="a", intent="in", type=integer)
    b = asg.Variable(name="b", intent="in", type=integer)
    c = asg.Variable(name="c", type=integer)
    f = FunctionBuilder(m, name="g", args=[a, b], return_var=c)
    f.finalize()



    a = asg_to_ast.asg_to_ast(m)
    s = print_fortran(a)
    print(s)
