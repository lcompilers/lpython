from lfortran.parser.cparser import parse, Parser, AST

def test_parse1():
    assert parse("1+1") == "(+ 1 1)"
    assert parse("""\
if (x > 0) then
    a = 1
else
    a = 2
end if""") == "(if (> x 0) [(= a 1)] [(= a 2)])"

def test_parse2():
    p = Parser()
    a = p.parse("1+1")
    assert a.pickle() == "(+ 1 1)"
    a = p.parse("""\
if (x > 0) then
    a = 1
else
    a = 2
end if""")
    assert a.pickle() == "(if (> x 0) [(= a 1)] [(= a 2)])"
