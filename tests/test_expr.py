from fortran_parser import parse

def test_var():
    r = "var_sym_decl"
    assert parse("x", r)
    assert parse("yx", r)
    assert not parse("1x", r)
    assert not parse("2", r)

def test_expr1():
    r = "expr"
    assert parse("1", r)
    assert parse("x+y", r)
    assert parse("(1+3)*4", r)
    assert parse("1+3*4", r)
    assert not parse("(1+", r)
