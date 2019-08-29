from lfortran.parser.cparser import parse

def test_parse1():
    assert parse("1+1") == "(+ 1 1)"
    assert parse("""\
if (x > 0) then
    a = 1
else
    a = 2
end if""") == "(if (> x 0) [(= a 1)] [(= a 2)])"
