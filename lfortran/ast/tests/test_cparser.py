from lfortran.parser.cparser import parse

def test_parse1():
    assert parse("1+1") == "(+ 1 1)"
