from lfortran.ast import parse, dump

def test_dump_expr():
    assert dump(parse("1+1", False)) == \
            "BinOp(left=Num(n='1'), op=Add(), right=Num(n='1'))"
    assert dump(parse("1+x", False)) == \
            "BinOp(left=Num(n='1'), op=Add(), right=Name(id='x'))"
    assert dump(parse("(x+y)**2", False)) == \
            "BinOp(left=BinOp(left=Name(id='x'), op=Add(), " \
            "right=Name(id='y')), op=Pow(), right=Num(n='2'))"

def test_dump_statements():
    assert dump(parse("if (x == 1) stop\n", False)) == \
            "If(test=Compare(left=Name(id='x'), op=Eq(), right=Num(n='1')), " \
            "body=[Stop(code=None)], orelse=[])"
    assert dump(parse("x == 1\n", False)) == \
            "Compare(left=Name(id='x'), op=Eq(), right=Num(n='1'))"

def test_dump_subroutines():
    assert dump(parse("""\
subroutine f
integer :: a, b
b = (1+2+a)*3
end subroutine
""", False)) == "Subroutine(name='f', args=[], decl=[Declaration(vars=[decl(sym='a', sym_type='integer', dims=[], attrs=[]), decl(sym='b', sym_type='integer', dims=[], attrs=[])])], body=[Assignment(target=Name(id='b'), value=BinOp(left=BinOp(left=BinOp(left=Num(n='1'), op=Add(), right=Num(n='2')), op=Add(), right=Name(id='a')), op=Mul(), right=Num(n='3')))], contains=[])"
    assert dump(parse("""\
subroutine f(x, y)
integer, intent(out) :: x, y
x = 1
end subroutine
""", False)) == "Subroutine(name='f', args=[arg(arg='x'), arg(arg='y')], decl=[Declaration(vars=[decl(sym='x', sym_type='integer', dims=[], attrs=[Attribute(name='intent', args=[attribute_arg(arg='out')])]), decl(sym='y', sym_type='integer', dims=[], attrs=[Attribute(name='intent', args=[attribute_arg(arg='out')])])])], body=[Assignment(target=Name(id='x'), value=Num(n='1'))], contains=[])"

def test_dump_programs():
    assert dump(parse("""\
program a
integer :: b
b = 1
end program
""")) == "Program(name='a', decl=[Declaration(vars=[decl(sym='b', sym_type='integer', dims=[], attrs=[])])], body=[Assignment(target=Name(id='b'), value=Num(n='1'))], contains=[])"
