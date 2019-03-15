from lfortran.ast.ast_to_src import ast_to_src
from lfortran.asr import asr, asr_to_ast
from lfortran.asr.asr_check import verify_asr
from lfortran.asr.builder import (make_type_integer, make_translation_unit,
        translation_unit_make_module, scope_add_function, function_make_var)

def test_builder_module1():
    integer = make_type_integer()
    unit = make_translation_unit()
    m = translation_unit_make_module(unit, "mod1")

    f = scope_add_function(unit.global_scope, "f", args=["a", "b"],
        return_var="c")
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
    f = scope_add_function(m.symtab, name="g", args=[a, b], return_var=c)

    verify_asr(unit)
    a = asr_to_ast.asr_to_ast(unit)
    s = ast_to_src(a)
    # Check that the generated source code contains a few "key" parts, which
    # are reasonably robust against changes in the way the source code is
    # generated from ASR.
    def has(s, a):
        assert s.find(a) > 0
    has(s, "function f(a, b)")
    has(s, "d = 5\n")
    has(s, "(a + b)")
    has(s, "interface\n")
    has(s, "function g(a, b)")
