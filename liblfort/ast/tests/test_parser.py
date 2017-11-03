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
