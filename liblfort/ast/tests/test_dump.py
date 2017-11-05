from liblfort.ast import parse, dump

def test_dump_expr():
    assert dump(parse("1+1")) == \
            "BinOp(left=Num(n='1'), op=Add(), right=Num(n='1'))"
    assert dump(parse("1+x")) == \
            "BinOp(left=Num(n='1'), op=Add(), right=Name(id='x'))"
    assert dump(parse("(x+y)**2")) == \
            "BinOp(left=BinOp(left=Name(id='x'), op=Add(), " \
            "right=Name(id='y')), op=Pow(), right=Num(n='2'))"

def test_dump_statements():
    assert dump(parse("if (x == 1) stop\n")) == \
            "If(test=Compare(left=Name(id='x'), op=Eq(), right=Num(n='1')), " \
            "body=[Stop(code=None)], orelse=[])"
    assert dump(parse("x == 1\n")) == \
            "Compare(left=Name(id='x'), op=Eq(), right=Num(n='1'))"

def _test_dump_subroutines():
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
    "right=Num(n='3')))], contains=[])"
    assert  dump(parse("""\
subroutine f(x, y)
integer, intent(out) :: x, y
x = 1
end subroutine
""")) == "Subroutine(name='f', args=[arg(arg='x'), arg(arg='y')], " \
    "body=[Declaration(vars=[decl(sym='x', sym_type='integer'), decl(sym='y', "\
    "sym_type='integer')]), Assignment(target='x', value=Num(n='1'))], " \
    "contains=[])"

def _test_dump_programs():
    assert dump(parse("""\
program a
integer :: b
b = 1
end program
""")) == "Program(name='a', body=[Declaration(vars=[decl(sym='b', " \
    "sym_type='integer')]), Assignment(target='b', value=Num(n='1'))], " \
    "contains=[])"
